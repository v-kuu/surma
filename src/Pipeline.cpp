#include "Pipeline.hpp"

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
        capture::Socket::init(*cfg.platform, *ret.umem_, cfg.iface.data(),
                              cfg.queue_id)
            .transform_error([](capture::SocketError)
                             { return PipelineError::SocketInitFailed; });
    if (!socket.has_value())
        return std::unexpected(socket.error());
    ret.socket_ = std::make_unique<capture::Socket>(std::move(socket.value()));

    return ret;
}

} // namespace surma
