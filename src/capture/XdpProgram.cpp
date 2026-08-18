#include "XdpProgram.hpp"
#include <spdlog/spdlog.h>
#include <xdp/xsk.h>

namespace surma::capture
{

std::expected<XdpProgram, XdpError> XdpProgram::load(Platform &platform,
                                                     int ifindex)
{
    XdpProgram ret{};
    int res = platform.xsk_setup_xdp_prog(ifindex, &ret.xsks_map_fd_);
    if (res != 0)
    {
        spdlog::error("failed to load xdp program: {} ({})", -res,
                      std::strerror(-res));
        return std::unexpected(XdpError::LoadError);
    }
    return ret;
}

std::expected<void, XdpError> XdpProgram::attach(Platform &platform,
                                                 Socket &socket) const
{
    int res = platform.xsk_socket__update_xskmap(socket.xsk(), xsks_map_fd_);
    if (res != 0)
    {
        spdlog::error("failed to attach xdp program to socket: {} ({})", -res,
                      std::strerror(-res));
        return std::unexpected(XdpError::AttachError);
    }
    return {};
}

} // namespace surma::capture
