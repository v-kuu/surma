#include "ProcessingThread.hpp"
#include <spdlog/spdlog.h>

namespace surma::processing
{

void ProcessingThread::start()
{
	running_.store(true, std::memory_order_relaxed);
	thread_ = std::thread(&ProcessingThread::run_, this);
}

void ProcessingThread::stop()
{
	running_.store(false, std::memory_order_relaxed);
	if (thread_.joinable())
		thread_.join();
}

void ProcessingThread::run_()
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
		while (!comp_queue_.push(desc->addr))
			;
	}
}

void ProcessingThread::process_packet_(uint8_t *pkt, uint32_t len)
{
	// TODO: hand to packet parser
	(void)pkt;
	spdlog::debug("processing packet len={}", len);
}

} // namespace surma::processing
