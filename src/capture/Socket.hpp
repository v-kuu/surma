#pragma once

#include "Platform.hpp"
#include "Umem.hpp"

#include <expected>
#include <linux/if_link.h>
#include <xdp/xsk.h>

namespace surma::capture
{

enum class SocketError
{
	SockErr
};

class Socket
{
  public:
	Socket() = delete;
	~Socket();
	Socket(const Socket &other) = delete;
	Socket &operator=(const Socket &other) = delete;
	Socket(Socket &&other) noexcept
	    : platform_(other.platform_),
	      xsk_(std::exchange(other.xsk_, nullptr)),
	      rx_(std::exchange(other.rx_, {})),
	      umem_(other.umem_),
	      fd_(other.fd_),
	      native_xdp_(other.native_xdp_)
	{}
	Socket &operator=(Socket &&) = delete;

	static std::expected<Socket, SocketError> init(
	    Platform &platform,
	    Umem &u,
	    const char *iface,
	    uint32_t queue_id);
	[[nodiscard]] int fd() const { return fd_; };
	xsk_socket *xsk() { return xsk_; };

  private:
	explicit Socket(Platform &platform, Umem &umem)
	    : platform_(platform),
	      umem_(umem)
	{}

	Platform &platform_;
	struct xsk_socket *xsk_ = nullptr;
	struct xsk_ring_cons rx_ = {};
	Umem &umem_;
	int fd_ = -1;
	bool native_xdp_ = false;
};

} // namespace surma::capture
