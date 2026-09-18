#pragma once
#include <chrono>
#include <cstdint>
#include <expected>

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
