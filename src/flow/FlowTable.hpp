#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <highwayhash/highwayhash.h>
#include <highwayhash/instruction_sets.h>

namespace surma::flow
{

namespace timeout
{
using namespace std::chrono_literals;

constexpr auto tcp_established = 24h;
constexpr auto tcp_syn_sent = 2min;
constexpr auto tcp_syn_rcvd = 1min;
constexpr auto tcp_fin_wait = 2min;
constexpr auto tcp_time_wait = 90s;
constexpr auto tcp_closed = 90s;
constexpr auto udp = 1min;
constexpr auto icmp = 20s;
constexpr auto other = 1min;
} // namespace timeout

enum class TcpState
{
	Closed,
	SynSent,
	SynRcvd,
	Established,
	FinWait1,
	FinWait2,
	Closing,
	TimeWait,
	CloseWait,
	LastAck,
};

struct FlowKey
{
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t proto;
	std::array<std::byte, 3> pad_;
};

using namespace highwayhash;
HHResult64 hash_key(const FlowKey &key, HH_ALIGNAS(32) const HHKey seed)
{
	HHResult64 result;
	HHStateT<HH_TARGET> state(seed);
	HighwayHashT(
	    &state, reinterpret_cast<const char *>(&key), sizeof(key), &result);
	return result;
}

struct TcpPeer
{
	uint32_t seqno;
	uint32_t max_win;
	uint8_t state;
	uint8_t wscale;
	std::array<std::byte, 2> pad_;
};

using std::chrono::steady_clock;

struct FlowEntry
{
	struct FlowKey key;
	uint64_t hash;

	struct TcpPeer src;
	struct TcpPeer dst;

	steady_clock::time_point created_at;
	steady_clock::time_point last_seen;
	std::chrono::seconds timeout;

	uint64_t packets;
	uint64_t bytes;

	uint8_t action;
	bool occupied;

	std::array<std::byte, 6> pad_;
};
static_assert(sizeof(FlowEntry) == 96);

enum class FlowAction
{
	Pass,
	Drop,
};

enum class FlowError
{
	Error,
};

#define FLOW_TABLE_DEFAULT_SIZE (1 << 22) // 4M slots
#define FLOW_TABLE_MAX_LOAD 75            // percent

class FlowTable
{
  public:
	FlowTable() = delete;
	~FlowTable() = default;
	FlowTable(const FlowTable &) = delete;
	FlowTable &operator=(const FlowTable &) = delete;
	FlowTable(FlowTable &&) = delete;
	FlowTable &operator=(FlowTable &&) = delete;

	std::expected<FlowTable, FlowError> init();

  private:
	struct FlowEntry *slots_;
	uint32_t capacity_;
	uint32_t mask_;
	uint32_t count;
	uint32_t limit_;

	uint64_t key0_;
	uint64_t key1_;

	uint64_t lookups_;
	uint64_t hits_;
	uint64_t misses;
	uint64_t insertions;
	uint64_t evictions;
	uint64_t collisions;

	friend class ReaperThread;
};

} // namespace surma::flow
