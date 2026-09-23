#include "FlowTable.hpp"
#include <netinet/ip.h>
#include <sys/random.h>

namespace surma::flow
{

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

struct FlowEntry *FlowTable::lookup(const struct FlowKey *raw)
{
	lookups_++;

	struct FlowKey normalized;
	bool is_initiator;
	FlowKey::normalize(normalized, *raw, is_initiator);

	HHResult64 hash = normalized.hash(seed_);
	auto index = static_cast<uint32_t>(hash & mask_);

	for (uint32_t i = 0; i < capacity_; i++)
	{
		uint32_t slot = (index + i) & mask_;
		struct FlowEntry *e = &slots_[slot];

		if (!e->occupied)
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
struct FlowEntry *FlowTable::insert(const struct FlowKey *raw, uint8_t action)
{
	if (count_ >= limit_)
		return nullptr;

	struct FlowKey normalized;
	bool is_initiator;
	FlowKey::normalize(normalized, *raw, is_initiator);

	HHResult64 hash = normalized.hash(seed_);
	auto index = static_cast<uint32_t>(hash & mask_);

	for (uint32_t i = 0; i < capacity_; i++)
	{
		uint32_t slot = (index + i) & mask_;
		struct FlowEntry *e = &slots_[slot];

		if (!e->occupied)
		{
			memset(e, 0, sizeof(*e));
			e->key = normalized;
			e->hash = hash;
			e->action = action;
			e->occupied = true;
			e->created_at = steady_clock::now();
			e->last_seen = e->created_at;
			e->timeout = timeout::other;

			if (normalized.proto == IPPROTO_TCP)
			{
				e->src.state = TcpState::SynSent;
				e->dst.state = TcpState::Closed;
				e->timeout = timeout::tcp_syn_sent;
			}
			else if (normalized.proto == IPPROTO_UDP)
				e->timeout = timeout::udp;
			else if (normalized.proto == IPPROTO_ICMP)
				e->timeout = timeout::icmp;

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
		struct FlowEntry *ne = &slots_[next];

		if (!ne->occupied)
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
    struct FlowEntry &e,
    const struct FlowKey *raw,
    uint8_t tcp_flags,
    uint32_t pkt_len)
{
	struct FlowKey normalized;
	bool is_initiator;
	FlowKey::normalize(normalized, *raw, is_initiator);

	e.last_seen = steady_clock::now();
	e.packets++;
	e.bytes += pkt_len;

	if (e.key.proto == IPPROTO_TCP)
		e.update_tcp_state(tcp_flags, is_initiator);
	else
		e.timeout = e.select_timeout();
}

FlowVerdict FlowTable::process(
    const struct FlowKey *key,
    uint8_t tcp_flags,
    uint32_t pkt_len)
{
	struct FlowEntry *e = lookup(key);

	if (e)
	{
		update(*e, key, tcp_flags, pkt_len);
		return e->action == FlowAction::Pass ? FlowVerdict::Pass
		                                     : FlowVerdict::Drop;
	}
}

} // namespace surma::flow
