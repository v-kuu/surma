#include "capture/EpollFd.hpp"
#include "fake_platform.hpp"
#include <catch2/catch_test_macros.hpp>
#include <sys/epoll.h>

using namespace surma::test;
using namespace surma::capture;

TEST_CASE("epoll init succeeds", "[unit][epoll]")
{
	FakePlatform platform;

	auto epoll = EpollFd::init(platform, 10);

	REQUIRE(epoll.has_value());
	REQUIRE(platform.epoll_create_called);
	REQUIRE(platform.epoll_ctl_called);
}

TEST_CASE("epoll init fails when epoll_create1 fails", "[unit][epoll]")
{
	FakePlatform platform;
	platform.epoll_create_return = -1;

	auto epoll = EpollFd::init(platform, 10);

	REQUIRE_FALSE(epoll.has_value());
	REQUIRE(epoll.error() == EpollErr::CreateErr);
	REQUIRE_FALSE(platform.epoll_ctl_called);
}

TEST_CASE("epoll init fails when epoll_ctl fails", "[unit][epoll]")
{
	FakePlatform platform;
	platform.epoll_ctl_return = -1;

	auto epoll = EpollFd::init(platform, 10);

	REQUIRE_FALSE(epoll.has_value());
	REQUIRE(epoll.error() == EpollErr::CtlErr);
	REQUIRE(platform.close_call_count == 1);
	REQUIRE(platform.last_close_fd == platform.epoll_create_return);
}

TEST_CASE("epoll init registers correct fd", "[unit][epoll]")
{
	FakePlatform platform;

	auto epoll = EpollFd::init(platform, 99);

	REQUIRE(epoll.has_value());
	REQUIRE(platform.last_epoll_ctl_fd == 99);
	REQUIRE(platform.last_epoll_ctl_op == EPOLL_CTL_ADD);
}

TEST_CASE("epoll destructor closes fd", "[unit][epoll]")
{
	FakePlatform platform;

	{
		auto epoll = EpollFd::init(platform, 10);
		REQUIRE(epoll.has_value());
	}

	REQUIRE(platform.close_call_count == 1);
	REQUIRE(platform.last_close_fd == platform.epoll_create_return);
}

TEST_CASE("moved-from epoll does not close fd", "[unit][epoll]")
{
	FakePlatform platform;

	auto epoll = EpollFd::init(platform, 10);

	REQUIRE(epoll.has_value());
	{
		EpollFd moved = std::move(*epoll);
	}

	REQUIRE(platform.close_call_count == 1);
}

TEST_CASE("moved-from epoll destructor does not double close", "[unit][epoll]")
{
	FakePlatform platform;

	{
		auto epoll = EpollFd::init(platform, 10);
		REQUIRE(epoll.has_value());
		EpollFd moved = std::move(*epoll);
	}

	REQUIRE(platform.close_call_count == 1);
}
