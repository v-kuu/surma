#include "capture/Socket.hpp"
#include "capture/Umem.hpp"
#include "capture/XdpProgram.hpp"
#include <catch2/catch_test_macros.hpp>
#include <net/if.h>

using namespace surma::capture;

TEST_CASE(
    "real Linux platform can create and destroy UMEM",
    "[integration][umem]")
{
	surma::capture::LinuxPlatform platform;

	REQUIRE_NOTHROW([&] {
		auto umem = surma::capture::Umem::init(platform);
		REQUIRE(umem.has_value());
	}());
}

/*
 *	Due to the nature of the test setup, having multiple integration tests
 *	bind and unbind the same socket leads to EBUSY returns
 */
TEST_CASE(
    "capture layer can init and cleanup in real environment",
    "[integration][socket][xdpprogram]")
{
	LinuxPlatform platform;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());
	{
		auto socket = Socket::init(platform, umem.value(), "veth-test", 0);
		REQUIRE(socket.has_value());

		const auto ifindex = if_nametoindex("veth-test");
		REQUIRE(ifindex != 0);
		auto program = XdpProgram::load(platform, static_cast<int>(ifindex));
		REQUIRE(program.has_value());

		auto result = program->attach(platform, socket.value());
		REQUIRE(result.has_value());
	}
}
