#include "surma.hpp"
#include "Config.hpp"
#include "Pipeline.hpp"
#include "capture/Platform.hpp"

#include <spdlog/spdlog.h>

namespace surma
{

void run()
{
    spdlog::info("surma starting up");

    Config cfg = {
        .platform = std::make_unique<capture::LinuxPlatform>(),
        .iface = "enp6s0",
        .queue_id = 0,
    };

    auto pipeline = Pipeline::init(cfg);
    if (!pipeline.has_value())
    {
        spdlog::error("pipeline failed to initialize");
        return;
    }
    spdlog::info("pipeline initialized");
}

} // namespace surma
