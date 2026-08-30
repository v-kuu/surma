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
    "capture layer can init and cleanup in real environment",
    "[integration][socket][xdpprogram][epollfd][rxloop]")
{
	// constexpr int PACKET_COUNT = 10;
	// constexpr int TIMEOUT_MS = 3000;
	// std::atomic<int> received{0};

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

		/*
		std::thread loop_thread([&]() { loop->run(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		int ret = std::system(
		        "python3 -c \""
		        "from scapy.all import *; "
		        "pkts = [Ether()/IP(src='10.99.0.1', dst='10.99.0.2')/"
		        "UDP(sport=9999, dport=9999)/Raw(b'surma integration test') "
		        "for _ in range(10)]; "
		        "sendp(pkts, iface='veth-test', verbose=False)"
		        "\""
		        );
		REQUIRE(ret == 0);

		auto deadline = std::chrono::steady_clock::now() +
		    std::chrono::milliseconds(TIMEOUT_MS);
		while (received.load(std::memory_order_relaxed) < PACKET_COUNT)
		{
		    if (std::chrono::steady_clock::now() > deadline)
		        break;
		    std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		loop->stop();
		loop_thread.join();

		REQUIRE(received.load() == PACKET_COUNT);
		*/
	}
}

} // namespace surma::test
