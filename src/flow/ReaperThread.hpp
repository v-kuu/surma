#pragma once
#include "FlowTable.hpp"
#include <atomic>
#include <chrono>
#include <thread>

namespace surma::flow
{

class ReaperThread
{
  public:
	ReaperThread() = delete;
	~ReaperThread() { stop(); }
	ReaperThread(const ReaperThread &) = delete;
	ReaperThread &operator=(const ReaperThread &) = delete;
	ReaperThread(ReaperThread &&) = delete;
	ReaperThread &operator=(ReaperThread &&) = delete;

	explicit ReaperThread(FlowTable &ft, std::chrono::seconds interval)
	    : ft_(ft),
	      running_(false),
	      interval_(interval)
	{}

	void start();
	void stop();

  private:
	FlowTable &ft_;
	std::atomic<bool> running_;
	std::thread thread_;
	std::chrono::seconds interval_{ 10 };

	void run_();
};

} // namespace surma::flow
