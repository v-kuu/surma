#include "Pipeline.hpp"
#include "capture/fake_platform.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace surma;
using surma::test::FakePlatform;

void *const sentinel_pointer = reinterpret_cast<void *>(0xDEAD0000);

static Config make_config(FakePlatform *platform)
{
    return Config{
        .platform = std::unique_ptr<capture::Platform>(platform),
        .iface = "eth0",
        .queue_id = 0,
    };
}

static FakePlatform *make_platform()
{
    auto *p = new FakePlatform();
    p->mmap_return = sentinel_pointer;
    return p;
}

TEST_CASE("pipeline init succeeds", "[unit][pipeline]")
{
    auto *platform = make_platform();
    auto config = make_config(platform);

    auto result = Pipeline::init(config);

    REQUIRE(result.has_value());
}

TEST_CASE("pipeline init fails when umem fails", "[unit][pipeline]")
{
    auto *platform = make_platform();
    platform->mmap_hugepage_fails = true;
    platform->mmap_return = MAP_FAILED;
    auto config = make_config(platform);

    auto result = Pipeline::init(config);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == PipelineError::UmemInitFailed);
}

TEST_CASE("pipeline init fails when socket fails", "[unit][pipeline]")
{
    auto *platform = make_platform();
    platform->socket_create_return = -1;
    auto config = make_config(platform);

    auto result = Pipeline::init(config);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == PipelineError::SocketInitFailed);
}

TEST_CASE("pipeline init passes iface and queue id to socket",
          "[unit][pipeline]")
{
    auto *platform = make_platform();
    auto config = Config{
        .platform = std::unique_ptr<capture::Platform>(platform),
        .iface = "eth1",
        .queue_id = 3,
    };

    auto result = Pipeline::init(config);

    REQUIRE(result.has_value());
    REQUIRE(std::string(platform->last_iface) == "eth1");
    REQUIRE(platform->last_queue_id == 3);
}

TEST_CASE("pipeline socket fallback to SKB mode", "[unit][pipeline]")
{
    auto *platform = make_platform();
    platform->socket_drv_mode_fails = true;
    auto config = make_config(platform);

    auto result = Pipeline::init(config);

    REQUIRE(result.has_value());
    REQUIRE(platform->socket_create_call_count == 2);
}

TEST_CASE("pipeline is moveable", "[unit][pipeline]")
{
    auto *platform = make_platform();
    auto config = make_config(platform);

    auto result = Pipeline::init(config);
    REQUIRE(result.has_value());

    Pipeline moved = std::move(result.value());

    REQUIRE_NOTHROW([&moved]() { (void)moved; }());
}

TEST_CASE("pipeline cleanup on umem success then socket failure",
          "[unit][pipeline]")
{
    auto *platform = make_platform();
    platform->socket_create_return = -1;
    auto config = make_config(platform);

    {
        auto result = Pipeline::init(config);
        REQUIRE_FALSE(result.has_value());
    }

    REQUIRE(platform->munmap_called);
    REQUIRE(platform->umem_delete_called);
    REQUIRE_FALSE(platform->socket_delete_called);
}
