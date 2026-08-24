#include "capture/Socket.hpp"
#include "capture/Umem.hpp"
#include "fake_platform.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace surma::test;
using namespace surma::capture;

void *const sentinel_pointer = reinterpret_cast<void *>(0xDEAD0000);

TEST_CASE("socket init succeeds", "[unit][socket]")
{
	FakePlatform platform;
	platform.mmap_return = sentinel_pointer;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());

	auto socket = Socket::init(platform, umem.value(), "eth0", 0);

	REQUIRE(socket.has_value());
	REQUIRE(socket->fd() == platform.socket_fd_return);
	REQUIRE(platform.last_queue_id == 0);
	REQUIRE(std::string(platform.last_iface) == "eth0");
}

TEST_CASE("socket init fails when xsk_socket_create fails", "[unit][socket]")
{
	FakePlatform platform;
	platform.mmap_return = sentinel_pointer;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());

	platform.socket_create_return = -1;

	auto socket = Socket::init(platform, umem.value(), "eth0", 0);

	REQUIRE_FALSE(socket.has_value());
	REQUIRE_FALSE(platform.socket_delete_called);
}

TEST_CASE("socket destructor deletes xsk", "[unit][socket]")
{
	FakePlatform platform;
	platform.mmap_return = sentinel_pointer;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());

	{
		auto socket = Socket::init(platform, umem.value(), "eth0", 0);
		REQUIRE(socket.has_value());
	}

	REQUIRE(platform.socket_delete_called);
}

TEST_CASE("socket destructor does not delete null xsk", "[unit][socket]")
{
	FakePlatform platform;
	platform.mmap_return = sentinel_pointer;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());

	platform.socket_create_return = -1;
	auto socket = Socket::init(platform, umem.value(), "eth0", 0);
	REQUIRE_FALSE(socket.has_value());

	REQUIRE_FALSE(platform.socket_delete_called);
}

TEST_CASE("moved-from socket does not delete xsk", "[unit][socket]")
{
	FakePlatform platform;
	platform.mmap_return = sentinel_pointer;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());

	auto socket = Socket::init(platform, umem.value(), "eth0", 0);
	REQUIRE(socket.has_value());

	{
		Socket moved = std::move(*socket);
	}

	REQUIRE(platform.socket_delete_count == 1);
}

TEST_CASE("socket init passes queue id correctly", "[unit][socket]")
{
	FakePlatform platform;
	platform.mmap_return = sentinel_pointer;
	auto umem = surma::capture::Umem::init(platform);
	REQUIRE(umem.has_value());

	auto socket = Socket::init(platform, umem.value(), "eth0", 3);

	REQUIRE(socket.has_value());
	REQUIRE(platform.last_queue_id == 3);
}
