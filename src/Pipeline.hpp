#pragma once

#include "Config.hpp"
#include "capture/RxLoop.hpp"
#include "capture/Socket.hpp"
#include "capture/Umem.hpp"
#include "processing/ProcessingThread.hpp"

#include <expected>
#include <memory>

namespace surma
{

enum class PipelineError
{
	UmemInitFailed,
	SocketInitFailed,
	XdpProgramFailed,
	RxLoopInitFailed,
	PipelineError
};

using RxQueue = surma::queue::SpscQueue<PacketDescriptor, FRAME_COUNT>;
using CompQueue = surma::queue::SpscQueue<uint64_t, FRAME_COUNT>;

class Pipeline
{
  public:
	~Pipeline() = default;
	Pipeline(const Pipeline &other) = delete;
	Pipeline &operator=(const Pipeline &other) = delete;
	Pipeline(Pipeline &&other) noexcept = default;
	Pipeline &operator=(Pipeline &&) noexcept = default;

	static std::expected<Pipeline, PipelineError> init(Config &cfg);
	void run();
	void stop();

  private:
	Pipeline() = default;

	std::unique_ptr<RxQueue> rx_queue_;
	std::unique_ptr<CompQueue> comp_queue_;
	std::unique_ptr<capture::Platform> platform_;
	std::unique_ptr<capture::Umem> umem_;
	std::unique_ptr<capture::Socket> socket_;
	std::unique_ptr<capture::RxLoop> loop_;
	std::unique_ptr<processing::ProcessingThread> processor_;
};

} // namespace surma
