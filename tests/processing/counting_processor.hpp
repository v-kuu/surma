#pragma once
#include "processing/ProcessingThread.hpp"
#include "queue/SpscQueue.hpp"
#include <atomic>

namespace surma::test
{

using RxQueue = queue::SpscQueue<capture::PacketDescriptor, FRAME_COUNT>;
using CompQueue = queue::SpscQueue<uint64_t, FRAME_COUNT>;

class CountingProcessor : public surma::processing::ProcessingThread
{
  public:
	explicit CountingProcessor(
	    surma::capture::Umem &umem,
	    RxQueue &rx_queue,
	    CompQueue &comp_queue,
	    std::atomic<int> &counter)
	    : ProcessingThread(umem, rx_queue, comp_queue),
	      counter_(counter)
	{}
	int count() const { return counter_.load(std::memory_order_relaxed); }

  protected:
	void process_packet_(uint8_t *pkt, uint32_t len) override
	{
		(void)pkt;
		(void)len;
		counter_.fetch_add(1, std::memory_order_relaxed);
	}

  private:
	std::atomic<int> &counter_;
};

} // namespace surma::test
