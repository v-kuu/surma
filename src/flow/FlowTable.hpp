#pragma once
#include "FlowEntry.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <expected>
#include <highwayhash/highwayhash.h>
#include <memory>
#include <sys/mman.h>
#include <utility>

namespace surma::flow
{

enum class FlowError
{
	CapacityNotP2,
	MmapFailed,
};

enum class FlowVerdict
{
	Pass,
	Drop,
	NewFlow,
};

struct MmapDeleter
{
	size_t size;
	void operator()(FlowEntry *ptr) const { munmap(ptr, size); }
};

constexpr uint32_t FLOW_TABLE_DEFAULT_SIZE = (1 << 22); // 4M slots
constexpr uint32_t FLOW_TABLE_MAX_LOAD = 75;            // percent

class FlowTable
{
  public:
	FlowTable() = delete;
	~FlowTable() = default;
	FlowTable(const FlowTable &) = delete;
	FlowTable &operator=(const FlowTable &) = delete;
	FlowTable(FlowTable &&other) noexcept
	    : slots_(std::exchange(other.slots_, nullptr)),
	      capacity_(other.capacity_),
	      mask_(other.mask_),
	      count_(other.count_),
	      limit_(other.limit_),
	      lookups_(other.lookups_),
	      hits_(other.hits_),
	      misses_(other.misses_),
	      insertions_(other.insertions_),
	      evictions_(other.evictions_),
	      collisions_(other.collisions_)
	{
		std::memcpy(seed_, other.seed_, sizeof(seed_));
		std::memset(other.seed_, 0, sizeof(other.seed_));
	}
	FlowTable &operator=(FlowTable &&) = delete;

	std::expected<FlowTable, FlowError> init(uint32_t capacity, uint32_t limit);
	struct FlowEntry *lookup(const struct FlowKey *raw);
	struct FlowEntry *insert(const struct FlowKey *raw, FlowAction action);
	void remove(uint32_t slot);
	void update(
	    struct FlowEntry &e,
	    const struct FlowKey *raw,
	    uint8_t tcp_flags,
	    uint32_t pkt_len);
	FlowVerdict process(
	    const struct FlowKey *key,
	    uint8_t tcp_flags,
	    uint32_t pkt_len);

  private:
	using SlotArray = std::unique_ptr<FlowEntry[], MmapDeleter>;
	SlotArray slots_;
	uint32_t capacity_;
	uint32_t mask_;
	uint32_t count_;
	uint32_t limit_;

	HH_ALIGNAS(32) highwayhash::HHKey seed_;

	uint64_t lookups_;
	uint64_t hits_;
	uint64_t misses_;
	uint64_t insertions_;
	uint64_t evictions_;
	uint64_t collisions_;

	explicit FlowTable(uint32_t capacity, uint32_t limit)
	    : capacity_(capacity),
	      limit_(limit)
	{}
	friend class ReaperThread;
};

} // namespace surma::flow
