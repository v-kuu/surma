#pragma once
#include "EpollFd.hpp"
#include "PacketDescriptor.hpp"
#include "Platform.hpp"
#include "Socket.hpp"
#include "Umem.hpp"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <expected>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <xdp/xsk.h>

namespace surma::capture
{

enum class RxLoopError
{
	EpollFdErr,
	RxLoopErr,
};

template<typename RxQ, typename CompQ>
class RxLoop
{
  public:
	RxLoop() = delete;
	~RxLoop() = default;
	RxLoop(const RxLoop &) = delete;
	RxLoop &operator=(const RxLoop &) = delete;
	RxLoop(RxLoop &&other) noexcept
	    : socket_(other.socket_),
	      umem_(other.umem_),
	      epoll_(std::move(other.epoll_)),
	      rx_queue_(other.rx_queue_),
	      comp_queue_(other.comp_queue_),
	      running_(other.running_.load())
	{}

	static std::expected<RxLoop, RxLoopError> init(
	    Platform &platform,
	    Socket &socket,
	    Umem &umem,
	    RxQ &rx_queue,
	    CompQ &comp_queue)
	{
		auto epoll = EpollFd::init(platform, socket.fd());
		if (!epoll.has_value())
			return std::unexpected(RxLoopError::EpollFdErr);

		return RxLoop(
		    socket, umem, std::move(epoll.value()), rx_queue, comp_queue);
	}

	void run()
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
			uint32_t n =
			    xsk_ring_cons__peek(&socket_.rx(), BATCH_SIZE, &idx_rx);
			if (n == 0)
				continue;

			for (uint32_t i = 0; i < n; i++)
			{
				const struct xdp_desc *desc =
				    xsk_ring_cons__rx_desc(&socket_.rx(), idx_rx + i);
				PacketDescriptor pd{ .addr = desc->addr, .len = desc->len };
				if (!rx_queue_.push(pd))
					spdlog::warn(
					    "rx queue full, dropping packet addr={:#x}",
					    desc->addr);
			}
			xsk_ring_cons__release(&socket_.rx(), n);
			drain_completions_();
		}
	}

	void stop() { running_.store(false, std::memory_order_relaxed); }

  private:
	RxLoop(
	    Socket &socket,
	    Umem &umem,
	    EpollFd epollfd,
	    RxQ &rx_queue,
	    CompQ &comp_queue)
	    : socket_(socket),
	      umem_(umem),
	      epoll_(std::move(epollfd)),
	      rx_queue_(rx_queue),
	      comp_queue_(comp_queue),
	      running_(false)
	{}

	void drain_completions_()
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

	Socket &socket_;
	Umem &umem_;
	EpollFd epoll_;
	RxQ &rx_queue_;
	CompQ &comp_queue_;
	std::atomic<bool> running_;
};

} // namespace surma::capture
