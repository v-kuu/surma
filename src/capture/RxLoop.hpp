#pragma once
#include "EpollFd.hpp"
#include "Platform.hpp"
#include "Socket.hpp"
#include "Umem.hpp"
#include <atomic>
#include <expected>

namespace surma::capture
{

enum class RxLoopError
{
	EpollFdErr,
	RxLoopErr,
};

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
	      running_(other.running_.load())
	{}

	static std::expected<RxLoop, RxLoopError> init(
	    Platform &platform,
	    Socket &socket,
	    Umem &umem);

	void run();
	void stop();

  private:
	RxLoop(Socket &socket, Umem &umem, EpollFd epollfd)
	    : socket_(socket),
	      umem_(umem),
	      epoll_(std::move(epollfd)),
	      running_(false)
	{}

	void refill_(uint32_t idx_rx, uint32_t n);
	void process_batch_(uint32_t idx_rx, uint32_t n);

	Socket &socket_;
	Umem &umem_;
	EpollFd epoll_;
	std::atomic<bool> running_;
};

} // namespace surma::capture
