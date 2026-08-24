#pragma once

#include <expected>
#include <utility>

namespace surma::capture
{

enum class EpollErr
{
	CreateErr,
	CtlErr,
	EpollErr,
};

class EpollFd
{
  public:
	EpollFd() = delete;
	~EpollFd();
	EpollFd(const EpollFd &other) = delete;
	EpollFd &operator=(const EpollFd &other) = delete;
	EpollFd(EpollFd &&other) noexcept
	    : fd_(std::exchange(other.fd_, -1))
	{}

	static std::expected<EpollFd, EpollErr> init(int xsk_fd);

  private:
	explicit EpollFd(int epfd)
	    : fd_(epfd)
	{}
	int fd_ = -1;
};

} // namespace surma::capture
