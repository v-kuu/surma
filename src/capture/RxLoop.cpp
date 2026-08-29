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
    Umem &umem)
{
	auto epoll = EpollFd::init(platform, socket.fd());
	if (!epoll.has_value())
		return std::unexpected(RxLoopError::EpollFdErr);

	return RxLoop(socket, umem, std::move(epoll.value()));
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

		process_batch_(idx_rx, n);
		xsk_ring_cons__release(&socket_.rx(), n);
		refill_(idx_rx, n);
	}
}

void RxLoop::stop() { running_.store(false, std::memory_order_relaxed); }

void RxLoop::process_batch_(uint32_t idx_rx, uint32_t n)
{
	auto umem_area = static_cast<uint8_t *>(umem_.area());

	for (uint32_t i = 0; i < n; i++)
	{
		const struct xdp_desc *desc =
		    xsk_ring_cons__rx_desc(&socket_.rx(), idx_rx + i);
		uint8_t *pkt = umem_area + desc->addr;
		uint32_t len = desc->len;

		// TODO: process packet
		spdlog::debug("rx packet len={} addr={:#x}", len, *pkt);
	}
}

void RxLoop::refill_(uint32_t idx_rx, uint32_t n)
{
	uint32_t idx_fq;
	uint32_t reserved = xsk_ring_prod__reserve(&umem_.fq(), n, &idx_fq);

	if (reserved < n)
		spdlog::warn(
		    "fill queue reserve returned {}/{} slots - fill queue draining",
		    reserved,
		    n);

	for (uint32_t i = 0; i < reserved; i++)
	{
		*xsk_ring_prod__fill_addr(&umem_.fq(), idx_fq + i) =
		    static_cast<uint64_t>(idx_rx + i) * FRAME_SIZE;
	}

	xsk_ring_prod__submit(&umem_.fq(), reserved);
}

} // namespace surma::capture
