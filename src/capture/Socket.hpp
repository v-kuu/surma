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
        : platform(other.platform), xsk(std::exchange(other.xsk, nullptr)),
          rx(other.rx), umem(other.umem), fd(other.fd)
    {
    }
    Socket &operator=(Socket &&) = delete;

    static std::expected<Socket, SocketError> init(Platform &platform, Umem &u,
                                                   const char *iface,
                                                   uint32_t queue_id);

  private:
    explicit Socket(Platform &platform, Umem &umem)
        : platform(platform), xsk(nullptr), rx({}), umem(umem), fd(-1)
    {
    }

    Platform &platform;
    struct xsk_socket *xsk;
    struct xsk_ring_cons rx;
    struct Umem &umem;
    int fd;
};

} // namespace surma::capture
