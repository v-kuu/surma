#include "FlowTable.hpp"
#include <netinet/ip.h>
#include <sys/random.h>

namespace surma::flow
{

using namespace highwayhash;

std::expected<FlowTable, FlowError> FlowTable::init(
    uint32_t capacity,
    uint32_t limit)
{
	if (capacity & (capacity - 1))
		return std::unexpected(FlowError::CapacityNotP2);

	FlowTable ret(capacity, limit);

	void *mem = mmap(
	    nullptr,
	    capacity * sizeof(FlowEntry),
	    PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANONYMOUS,
	    -1,
	    0);
	if (mem == MAP_FAILED)
		return std::unexpected(FlowError::MmapFailed);

	ret.slots_ = SlotArray(
	    static_cast<FlowEntry *>(mem),
	    MmapDeleter{ capacity * sizeof(FlowEntry) });
	ret.capacity_ = capacity;
	ret.mask_ = capacity - 1;
	ret.limit_ = limit ? limit : (capacity * FLOW_TABLE_MAX_LOAD / 100);

	getrandom(seed_, sizeof(highwayhash::HHKey), 0);

	return ret;
}

FlowEntry *FlowTable::lookup(const FlowKey *raw)
{
	lookups_++;

	bool is_initiator;
	FlowKey normalized = FlowKey::normalized(*raw, is_initiator);

	HHResult64 hash = normalized.hash(seed_);
	auto index = static_cast<uint32_t>(hash & mask_);

	for (uint32_t i = 0; i < capacity_; i++)
	{
		uint32_t slot = (index + i) & mask_;
		FlowEntry *e = &slots_[slot];

		if (e->state != SlotState::Occupied)
		{
			misses_++;
			return nullptr;
		}

		if (e->hash == hash &&
		    memcmp(&e->key, &normalized, sizeof(normalized)) == 0)
		{
			hits_++;
			return e;
		}

		collisions_++;
	}

	misses_++;
	return nullptr;
}

using std::chrono::steady_clock;

FlowEntry *FlowTable::insert(const FlowKey *raw, FlowAction action)
{
	if (count_ >= limit_)
		return nullptr;

	bool is_initiator;
	FlowKey normalized = FlowKey::normalized(*raw, is_initiator);

	HHResult64 hash = normalized.hash(seed_);
	auto index = static_cast<uint32_t>(hash & mask_);

	for (uint32_t i = 0; i < capacity_; i++)
	{
		uint32_t slot = (index + i) & mask_;
		FlowEntry *e = &slots_[slot];

		if (e->state != SlotState::Occupied)
		{
			memset(e, 0, sizeof(*e));
			e->key = normalized;
			e->hash = hash;
			e->action = action;
			e->state = SlotState::Occupied;
			e->created_at = steady_clock::now();
			e->last_seen = e->created_at;
			e->expiry = timeout::other;

			if (normalized.proto == IPPROTO_TCP)
			{
				e->src.state = TcpState::SynSent;
				e->dst.state = TcpState::Closed;
				e->expiry = timeout::tcp_syn_sent;
			}
			else if (normalized.proto == IPPROTO_UDP)
				e->expiry = timeout::udp;
			else if (normalized.proto == IPPROTO_ICMP)
				e->expiry = timeout::icmp;

			count_++;
			insertions_++;
			return e;
		}
	}
	return nullptr;
}

void FlowTable::remove(uint32_t slot)
{
	uint32_t current = slot;

	uint32_t scanned = 0;
	while (scanned++ < mask_)
	{
		uint32_t next = (current + 1) & mask_;
		FlowEntry *ne = &slots_[next];

		if (ne->state != SlotState::Occupied)
			break;

		auto natural = static_cast<uint32_t>(ne->hash & mask_);
		if (natural == next)
			break;

		slots_[current] = *ne;
		memset(ne, 0, sizeof(*ne));
		current = next;
	}

	memset(&slots_[current], 0, sizeof(slots_[current]));
	count_--;
	evictions_++;
}

void FlowTable::update(
    FlowEntry &e,
    const FlowKey *raw,
    uint8_t tcp_flags,
    uint32_t pkt_len)
{
	bool is_initiator;
	FlowKey::normalized(*raw, is_initiator);

	e.last_seen = steady_clock::now();
	e.packets++;
	e.bytes += pkt_len;

	if (e.key.proto == IPPROTO_TCP)
		e.update_tcp_state(tcp_flags, is_initiator);
	else
		e.expiry = e.select_timeout();
}

FlowVerdict FlowTable::process(
    const FlowKey *key,
    uint8_t tcp_flags,
    uint32_t pkt_len)
{
	FlowEntry *e = lookup(key);

	if (e)
	{
		update(*e, key, tcp_flags, pkt_len);
		return e->action == FlowAction::Pass ? FlowVerdict::Pass
		                                     : FlowVerdict::Drop;
	}

	return FlowVerdict::NewFlow;
}

} // namespace surma::flow
