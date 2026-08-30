#pragma once

#include "Config.hpp"
#include "capture/RxLoop.hpp"
#include "capture/Socket.hpp"
#include "capture/Umem.hpp"
#include "processing/ProcessingThread.hpp"
#include "queue/SpscQueue.hpp"

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

	using RxQueue = surma::queue::SpscQueue<PacketDescriptor, FRAME_COUNT>;
	using CompQueue = surma::queue::SpscQueue<uint64_t, FRAME_COUNT>;
	using LoopType = capture::RxLoop<RxQueue, CompQueue>;
	using ProcType = processing::ProcessingThread<RxQueue, CompQueue>;

	std::unique_ptr<RxQueue> rx_queue_;
	std::unique_ptr<CompQueue> comp_queue_;
	std::unique_ptr<capture::Platform> platform_;
	std::unique_ptr<capture::Umem> umem_;
	std::unique_ptr<capture::Socket> socket_;
	std::unique_ptr<LoopType> loop_;
	std::unique_ptr<ProcType> processor_;
};

} // namespace surma
