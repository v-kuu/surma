#include "ReaperThread.hpp"

namespace surma::flow
{

void ReaperThread::start()
{
	running_.store(true, std::memory_order_relaxed);
	thread_ = std::thread(&ReaperThread::run_, this);
}

void ReaperThread::stop()
{
	running_.store(false, std::memory_order_relaxed);
	if (thread_.joinable())
		thread_.join();
}

using std::chrono::steady_clock;
void ReaperThread::run_()
{
	while (running_.load(std::memory_order_relaxed))
	{
		std::this_thread::sleep_for(interval_);
		if (!running_.load(std::memory_order_relaxed))
			break;

		steady_clock::time_point now = steady_clock::now();
		uint32_t expired = 0;

		for (uint32_t i = 0; i < ft_.capacity_; i++)
		{
			auto *e = &ft_.slots_[i];
			if (!e->occupied)
				continue;

			if (now - e->last_seen > e->timeout)
			{
				// flow_table_remove(ft_, i);
				expired++;
				i--;
			}
		}

		// TODO: log expired
		(void)expired;
	}
}

} // namespace surma::flow
