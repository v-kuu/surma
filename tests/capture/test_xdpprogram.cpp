#include "capture/Socket.hpp"
#include "capture/Umem.hpp"
#include "capture/XdpProgram.hpp"
#include "fake_platform.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace surma::test;
using namespace surma::capture;

TEST_CASE("xdp program load fails when setup fails", "[unit][xdpprogram]")
{
    FakePlatform platform;
    platform.setup_xdp_prog_return = -1;

    auto program = XdpProgram::load(platform, 7);

    REQUIRE_FALSE(program.has_value());
    REQUIRE(program.error() == XdpError::LoadError);
    REQUIRE(platform.last_ifindex == 7);
}

TEST_CASE("xdp program attaches to socket", "[unit][xdpprogram]")
{
    FakePlatform platform;
    platform.update_xskmap_return = 0;
    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());
    auto socket = Socket::init(platform, umem.value(), "eth0", 0);
    REQUIRE(socket.has_value());

    auto program = XdpProgram::load(platform, 7);
    REQUIRE(program.has_value());
    auto result = program->attach(platform, socket.value());

    REQUIRE(result.has_value());
    REQUIRE(platform.update_xskmap_called);
    REQUIRE(platform.last_socket == socket->xsk());
    REQUIRE(platform.last_xsks_map_fd == program->xsks_map_fd());
}

TEST_CASE("xdp program attach fails when update_xskmap fails",
          "[unit][xdpprogram]")
{
    FakePlatform platform;
    platform.update_xskmap_return = -1;
    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());
    auto socket = Socket::init(platform, umem.value(), "eth0", 0);
    REQUIRE(socket.has_value());

    auto program = XdpProgram::load(platform, 7);
    REQUIRE(program.has_value());
    auto result = program->attach(platform, socket.value());

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == XdpError::AttachError);
}
