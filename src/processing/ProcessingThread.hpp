#pragma once
#include "capture/Umem.hpp"
#include <atomic>
#include <spdlog/spdlog.h>
#include <thread>

namespace surma::processing
{

template<typename RxQ, typename CompQ>
class ProcessingThread
{
  public:
	ProcessingThread() = delete;
	~ProcessingThread() { stop(); }
	ProcessingThread(const ProcessingThread &) = delete;
	ProcessingThread &operator=(const ProcessingThread &) = delete;
	ProcessingThread(ProcessingThread &&) = delete;
	ProcessingThread &operator=(ProcessingThread &&) = delete;

	explicit ProcessingThread(
	    surma::capture::Umem &umem,
	    RxQ &rx_queue,
	    CompQ &comp_queue)
	    : umem_(umem),
	      rx_queue_(rx_queue),
	      comp_queue_(comp_queue),
	      running_(false)
	{}

	void start()
	{
		running_.store(true, std::memory_order_relaxed);
		thread_ = std::thread(&ProcessingThread::run_, this);
	}

	void stop()
	{
		running_.store(false, std::memory_order_relaxed);
		if (thread_.joinable())
			thread_.join();
	}

  private:
	void run_()
	{
		auto *umem_area = static_cast<uint8_t *>(umem_.area());

		while (running_.load(std::memory_order_relaxed))
		{
			auto desc = rx_queue_.pop();
			if (!desc.has_value())
				continue;

			uint8_t *pkt = umem_area + desc->addr;
			uint32_t len = desc->len;

			process_packet_(pkt, len);

			// handle backpressure
			uint32_t retries = 0;
			while (!comp_queue_.push(desc->addr) &&
			       running_.load(std::memory_order_relaxed))
			{
				if (++retries > 1000)
				{
					spdlog::warn(
					    "completion queue full after {} retries, dropping "
					    "frame "
					    "{:x}",
					    retries,
					    desc->addr);
					break;
				}
				std::this_thread::yield();
			}
		}

		while (true)
		{
			auto desc = rx_queue_.pop();
			if (!desc.has_value())
				break;
			process_packet_(umem_area + desc->addr, desc->len);
			comp_queue_.push(desc->addr);
		}
	}

	void process_packet_(uint8_t *pkt, uint32_t len)
	{
		// TODO: hand to packet parser
		(void)pkt;
		spdlog::debug("processing packet len={}", len);
	}

	surma::capture::Umem &umem_;
	RxQ &rx_queue_;
	CompQ &comp_queue_;
	std::atomic<bool> running_;
	std::thread thread_;
};

} // namespace surma::processing
