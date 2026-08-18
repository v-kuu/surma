#pragma once

#include "Config.hpp"
#include "capture/Socket.hpp"
#include "capture/Umem.hpp"

#include <expected>
#include <memory>

namespace surma
{

enum class PipelineError
{
    UmemInitFailed,
    SocketInitFailed,
    XdpProgramFailed,
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

  private:
    Pipeline() = default;

    std::unique_ptr<capture::Platform> platform_;
    std::unique_ptr<capture::Umem> umem_;
    std::unique_ptr<capture::Socket> socket_;
};

} // namespace surma
