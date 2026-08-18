#include "Pipeline.hpp"
#include "capture/XdpProgram.hpp"
#include <net/if.h>

namespace surma
{

std::expected<Pipeline, PipelineError> Pipeline::init(Config &cfg)
{
    Pipeline ret;

    auto umem = capture::Umem::init(*cfg.platform)
                    .transform_error([](capture::UmemError)
                                     { return PipelineError::UmemInitFailed; });
    if (!umem.has_value())
        return std::unexpected(umem.error());
    ret.umem_ = std::make_unique<capture::Umem>(std::move(umem.value()));

    auto socket =
        capture::Socket::init(*cfg.platform, *ret.umem_, cfg.iface.c_str(),
                              cfg.queue_id)
            .transform_error([](capture::SocketError)
                             { return PipelineError::SocketInitFailed; });
    if (!socket.has_value())
        return std::unexpected(socket.error());
    ret.socket_ = std::make_unique<capture::Socket>(std::move(socket.value()));

    unsigned int ifindex = if_nametoindex(cfg.iface.c_str());
    auto xdp =
        capture::XdpProgram::load(*cfg.platform, static_cast<int>(ifindex))
            .transform_error([](capture::XdpError)
                             { return PipelineError::XdpProgramFailed; });

    if (!xdp.has_value())
        return std::unexpected(xdp.error());

    if (auto result = xdp.value().attach(*cfg.platform, *ret.socket_); !result)
        return std::unexpected(PipelineError::XdpProgramFailed);
    return ret;
}

} // namespace surma
