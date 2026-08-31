#include "capture/RxLoop.hpp"
#include "capture/Socket.hpp"
#include "capture/Umem.hpp"
#include "capture/XdpProgram.hpp"
#include <catch2/catch_test_macros.hpp>
#include <net/if.h>

using namespace surma::capture;

namespace surma::test
{

TEST_CASE(
    "real Linux platform can create and destroy UMEM",
    "[integration][umem]")
{
	LinuxPlatform platform;

	REQUIRE_NOTHROW([&] {
		auto umem = Umem::init(platform);
		REQUIRE(umem.has_value());
	}());
}

/*
 *	Due to the nature of the test setup, having multiple integration tests
 *	bind and unbind the same socket leads to EBUSY returns
 */
TEST_CASE(
    "capture layer can init and cleanup in a real environment",
    "[integration][socket][xdpprogram][epollfd][rxloop]")
{
	LinuxPlatform platform;
	auto umem = Umem::init(platform);
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

		surma::queue::SpscQueue<PacketDescriptor, FRAME_COUNT> RxQ;
		surma::queue::SpscQueue<uint64_t, FRAME_COUNT> CompQ;
		auto loop =
		    RxLoop::init(platform, socket.value(), umem.value(), RxQ, CompQ);
		REQUIRE(loop.has_value());
	}
}

} // namespace surma::test
