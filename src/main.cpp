#include "Pipeline.hpp"
#include "capture/Platform.hpp"
#include <cerrno>
#include <csignal>
#include <spdlog/spdlog.h>

namespace
{

surma::Pipeline *g_pipeline = nullptr;

void signal_handler(int sig)
{
	spdlog::info("caugh signal {}, shutting down", sig);
	if (g_pipeline)
		g_pipeline->stop();
}

void install_signal_handler()
{
	struct sigaction sa{};
	sa.sa_handler = signal_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
}

} // anonymous namespace

int main()
{
	try
	{
		spdlog::info("surma starting up");

		surma::Config cfg = {
			.platform = std::make_unique<surma::capture::LinuxPlatform>(),
			.iface = "enp6s0",
			.queue_id = 0,
		};

		auto pipeline = surma::Pipeline::init(cfg);
		if (!pipeline.has_value())
		{
			spdlog::error("pipeline failed to initialize");
			return 1;
		}
		spdlog::info("pipeline initialized");
		g_pipeline = &pipeline.value();

		install_signal_handler();

		pipeline->run();
	}
	catch (std::exception &e)
	{
		spdlog::error(
		    "Uncaught exeption \"{}\", errno={}",
		    e.what(),
		    std::strerror(errno));
		return 1;
	}
	return 0;
}
