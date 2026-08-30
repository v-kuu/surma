#pragma once
#include "capture/PacketDescriptor.hpp"
#include "capture/Umem.hpp"
#include "queue/SpscQueue.hpp"
#include <atomic>
#include <thread>

namespace surma::processing
{

using RxQueue = queue::SpscQueue<capture::PacketDescriptor, FRAME_COUNT>;
using CompQueue = queue::SpscQueue<uint64_t, FRAME_COUNT>;

class ProcessingThread
{
  public:
	ProcessingThread() = delete;
	virtual ~ProcessingThread() { stop(); }
	ProcessingThread(const ProcessingThread &) = delete;
	ProcessingThread &operator=(const ProcessingThread &) = delete;
	ProcessingThread(ProcessingThread &&) = delete;
	ProcessingThread &operator=(ProcessingThread &&) = delete;

	explicit ProcessingThread(
	    surma::capture::Umem &umem,
	    RxQueue &rx_queue,
	    CompQueue &comp_queue)
	    : umem_(umem),
	      rx_queue_(rx_queue),
	      comp_queue_(comp_queue),
	      running_(false)
	{}

	void start();
	void stop();

  protected:
	virtual void process_packet_(uint8_t *pkt, uint32_t len);

  private:
	void run_();

	surma::capture::Umem &umem_;
	RxQueue &rx_queue_;
	CompQueue &comp_queue_;
	std::atomic<bool> running_;
	std::thread thread_;
};

} // namespace surma::processing
