#include "RxLoop.hpp"
#include <cerrno>
#include <cstring>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <xdp/xsk.h>

namespace surma::capture
{

std::expected<RxLoop, RxLoopError> RxLoop::init(
    Platform &platform,
    Socket &socket,
    Umem &umem,
    RxQueue &rx_queue,
    CompQueue &comp_queue)
{
	auto epoll = EpollFd::init(platform, socket.fd());
	if (!epoll.has_value())
		return std::unexpected(RxLoopError::EpollFdErr);

	return RxLoop(socket, umem, std::move(epoll.value()), rx_queue, comp_queue);
}

void RxLoop::run()
{
	running_.store(true, std::memory_order_relaxed);

	struct epoll_event events[1];

	while (running_.load(std::memory_order_relaxed))
	{
		int ready = epoll_wait(epoll_.fd(), events, 1, -1);

		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			spdlog::error(
			    "epoll_wait failed: {} ({})", errno, std::strerror(errno));
			break;
		}
		if (ready == 0)
			continue;

		if (xsk_ring_prod__needs_wakeup(&umem_.fq()))
			recv(socket_.fd(), nullptr, 0, MSG_DONTWAIT);

		uint32_t idx_rx;
		uint32_t n = xsk_ring_cons__peek(&socket_.rx(), BATCH_SIZE, &idx_rx);
		if (n == 0)
			continue;

		for (uint32_t i = 0; i < n; i++)
		{
			const struct xdp_desc *desc =
			    xsk_ring_cons__rx_desc(&socket_.rx(), idx_rx + i);
			PacketDescriptor pd{ .addr = desc->addr, .len = desc->len };
			if (!rx_queue_.push(pd))
				spdlog::warn(
				    "rx queue full, dropping packet addr={:#x}", desc->addr);
		}
		xsk_ring_cons__release(&socket_.rx(), n);
		drain_completions_();
	}
}

void RxLoop::stop() { running_.store(false, std::memory_order_relaxed); }

void RxLoop::drain_completions_()
{
	uint32_t idx_fq;
	uint32_t count = 0;

	uint64_t completed[BATCH_SIZE];
	while (count < BATCH_SIZE)
	{
		auto addr = comp_queue_.pop();
		if (!addr.has_value())
			break;
		completed[count++] = *addr;
	}

	if (count == 0)
		return;

	uint32_t reserved = xsk_ring_prod__reserve(&umem_.fq(), count, &idx_fq);

	if (reserved < count)
		spdlog::warn(
		    "fill queue reserve returned {}/{} slots - draining",
		    reserved,
		    count);

	for (uint32_t i = 0; i < reserved; i++)
	{
		*xsk_ring_prod__fill_addr(&umem_.fq(), idx_fq + i) = completed[i];
	}

	xsk_ring_prod__submit(&umem_.fq(), reserved);
}

} // namespace surma::capture
