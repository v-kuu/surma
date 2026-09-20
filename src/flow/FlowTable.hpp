#pragma once
#include "FlowEntry.hpp"
#include <chrono>
#include <cstdint>
#include <expected>

namespace surma::flow
{

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
	uint64_t misses_;
	uint64_t insertions_;
	uint64_t evictions_;
	uint64_t collisions_;

	friend class ReaperThread;
};

} // namespace surma::flow
