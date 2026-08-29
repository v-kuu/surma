#pragma once

#include "capture/Platform.hpp"
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
	    : platform_(other.platform_),
	      fd_(std::exchange(other.fd_, -1))
	{}

	static std::expected<EpollFd, EpollErr> init(
	    Platform &platform,
	    int xsk_fd);

	[[nodiscard]] int fd() const { return fd_; }

  private:
	explicit EpollFd(Platform &platform, int epfd)
	    : platform_(platform),
	      fd_(epfd)
	{}
	Platform &platform_;
	int fd_ = -1;
};

} // namespace surma::capture
