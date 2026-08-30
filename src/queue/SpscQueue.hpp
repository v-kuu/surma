#pragma once
#include <atomic>
#include <cstdint>
#include <optional>

namespace surma::queue
{

// Single produced single consumer queue; producer pushes and consumer pops
template<typename T, uint32_t Capacity>
class SpscQueue
{
	static_assert(
	    (Capacity & (Capacity - 1)) == 0,
	    "Capacity must be a power of two");
	static_assert(Capacity >= 2, "Capacity must be at least 2");
	static constexpr uint32_t WRAP_MASK = Capacity - 1;

  public:
	SpscQueue() = default;
	~SpscQueue() = default;
	SpscQueue(const SpscQueue &) = delete;
	SpscQueue &operator=(const SpscQueue &) = delete;
	SpscQueue(SpscQueue &&) = delete;
	SpscQueue &operator=(SpscQueue &&) = delete;

	bool push(const T &item)
	{
		uint32_t head = head_.load(std::memory_order_relaxed);
		uint32_t next = (head + 1) & WRAP_MASK;

		if (next == tail_.load(std::memory_order_acquire))
			return false;

		buf_[head] = item;
		head_.store(next, std::memory_order_release);
		return true;
	}

	std::optional<T> pop()
	{
		uint32_t tail = tail_.load(std::memory_order_relaxed);

		if (tail == head_.load(std::memory_order_acquire))
			return std::nullopt;

		T item = buf_[tail];
		tail_.store((tail + 1) & WRAP_MASK, std::memory_order_release);
		return item;
	}

	[[nodiscard]] bool empty() const
	{
		return head_.load(std::memory_order_acquire) ==
		       tail_.load(std::memory_order_acquire);
	}

	[[nodiscard]] bool full() const
	{
		uint32_t head = head_.load(std::memory_order_acquire);
		uint32_t next = (head + 1) & WRAP_MASK;
		return next == tail_.load(std::memory_order_acquire);
	}

	[[nodiscard]] uint32_t size() const
	{
		uint32_t head = head_.load(std::memory_order_acquire);
		uint32_t tail = tail_.load(std::memory_order_acquire);
		return (head - tail) & WRAP_MASK;
	}

	static constexpr uint32_t capacity() { return Capacity; };

  private:
	alignas(64) std::atomic<uint32_t> head_{ 0 };
	alignas(64) std::atomic<uint32_t> tail_{ 0 };
	alignas(64) T buf_[Capacity];
};

} // namespace surma::queue
