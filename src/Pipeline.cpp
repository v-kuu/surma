#include "Pipeline.hpp"
#include "capture/XdpProgram.hpp"
#include <net/if.h>

namespace surma
{

std::expected<Pipeline, PipelineError> Pipeline::init(Config &cfg)
{
	Pipeline ret;
	ret.rx_queue_ = std::make_unique<RxQueue>();
	ret.comp_queue_ = std::make_unique<CompQueue>();

	auto umem = capture::Umem::init(*cfg.platform);
	if (!umem.has_value())
		return std::unexpected(PipelineError::UmemInitFailed);
	ret.umem_ = std::make_unique<capture::Umem>(std::move(umem.value()));

	auto socket = capture::Socket::init(
	    *cfg.platform, *ret.umem_, cfg.iface.c_str(), cfg.queue_id);
	if (!socket.has_value())
		return std::unexpected(PipelineError::SocketInitFailed);
	ret.socket_ = std::make_unique<capture::Socket>(std::move(socket.value()));

	unsigned int ifindex = if_nametoindex(cfg.iface.c_str());
	auto xdp =
	    capture::XdpProgram::load(*cfg.platform, static_cast<int>(ifindex));

	if (!xdp.has_value())
		return std::unexpected(PipelineError::XdpProgramFailed);

	if (auto result = xdp.value().attach(*cfg.platform, *ret.socket_); !result)
		return std::unexpected(PipelineError::XdpProgramFailed);

	auto loop = capture::RxLoop::init(
	    *cfg.platform,
	    *ret.socket_,
	    *ret.umem_,
	    *ret.rx_queue_,
	    *ret.comp_queue_);
	if (!loop.has_value())
		return std::unexpected(PipelineError::RxLoopInitFailed);
	ret.loop_ = std::make_unique<capture::RxLoop>(std::move(loop.value()));
	ret.processor_ = std::make_unique<processing::ProcessingThread>(
	    *ret.umem_, *ret.rx_queue_, *ret.comp_queue_);

	return ret;
}

void Pipeline::run()
{
	processor_->start();
	loop_->run(); // blocking
}

void Pipeline::stop()
{
	loop_->stop();
	processor_->stop();
}

} // namespace surma
