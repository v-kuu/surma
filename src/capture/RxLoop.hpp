#pragma once
#include "EpollFd.hpp"
#include "PacketDescriptor.hpp"
#include "Platform.hpp"
#include "Socket.hpp"
#include "Umem.hpp"
#include "queue/SpscQueue.hpp"
#include <atomic>
#include <expected>

namespace surma::capture
{

using RxQueue = surma::queue::SpscQueue<PacketDescriptor, FRAME_COUNT>;
using CompQueue = surma::queue::SpscQueue<uint64_t, FRAME_COUNT>;

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
	      rx_queue_(other.rx_queue_),
	      comp_queue_(other.comp_queue_),
	      running_(other.running_.load())
	{}

	static std::expected<RxLoop, RxLoopError> init(
	    Platform &platform,
	    Socket &socket,
	    Umem &umem,
	    RxQueue &rx_queue,
	    CompQueue &comp_queue);

	void run();
	void stop();

  private:
	RxLoop(
	    Socket &socket,
	    Umem &umem,
	    EpollFd epollfd,
	    RxQueue &rx_queue,
	    CompQueue &comp_queue)
	    : socket_(socket),
	      umem_(umem),
	      epoll_(std::move(epollfd)),
	      rx_queue_(rx_queue),
	      comp_queue_(comp_queue),
	      running_(false)
	{}

	void drain_completions_();

	Socket &socket_;
	Umem &umem_;
	EpollFd epoll_;
	RxQueue &rx_queue_;
	CompQueue &comp_queue_;
	std::atomic<bool> running_;
};

} // namespace surma::capture
