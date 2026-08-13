#include "Socket.hpp"

#include <cerrno>
#include <spdlog/spdlog.h>

namespace surma::capture
{

Socket::~Socket()
{
    if (xsk_ != nullptr)
        platform_.xsk_socket__delete(xsk_);
}

std::expected<Socket, SocketError> Socket::init(Platform &platform, Umem &u,
                                                const char *iface,
                                                uint32_t queue_id)
{
    Socket ret(platform, u);

    struct xsk_socket_config cfg = {
        .rx_size = RX_RING_SIZE,
        .tx_size = 0,
        .libbpf_flags = 0,
        .xdp_flags = XDP_FLAGS_DRV_MODE,
        .bind_flags = XDP_USE_NEED_WAKEUP,
    };

    int res = platform.xsk_socket__create(&ret.xsk_, iface, queue_id,
                                          ret.umem_.handle(), &ret.rx_, nullptr,
                                          &cfg);
    if (res != 0)
    {
        spdlog::error("xsk_socket__create failed: ret={}, errno={}, {}", res,
                      errno, std::strerror(errno));
        return std::unexpected(SocketError::SockErr);
    }
    ret.fd_ = platform.xsk_socket__fd(ret.xsk_);

    return ret;
}

} // namespace surma::capture
