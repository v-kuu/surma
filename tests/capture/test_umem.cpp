#include "capture/Umem.hpp"
#include "fake_platform.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace surma::test;

void *const sentinel_pointer = reinterpret_cast<void *>(0xDEAD0000);

TEST_CASE("umem init succeeds with hugepages", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());
    REQUIRE(platform.mmap_call_count == 1);
}

TEST_CASE("umem init falls back when hugepages unavailable", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_hugepage_fails = true;
    platform.mmap_return = sentinel_pointer;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());
    REQUIRE(platform.mmap_call_count == 2);
}

TEST_CASE("umem init fails when both mmap calls fail", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_hugepage_fails = true;
    platform.mmap_return = MAP_FAILED;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.error() == surma::capture::UmemError::MapErr);
}

TEST_CASE("umem init fails when xsk_umem_create fails", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;
    platform.umem_create_return = -1;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.error() == surma::capture::UmemError::XskErr);
}

TEST_CASE("umem destructor calls delete and unmap", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;

    {
        auto umem = surma::capture::Umem::init(platform);
        REQUIRE(umem.has_value());
    }

    REQUIRE(platform.umem_delete_called);
    REQUIRE(platform.munmap_called);
}

TEST_CASE("fill queue is populated on init", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());

    REQUIRE(platform.reserve_call_count == 1);
    REQUIRE(platform.submit_call_count == 1);
    REQUIRE(platform.last_submit_count == platform.reserve_return);
    REQUIRE(platform.fill_addrs.size() == FILL_RING_SIZE);
}

TEST_CASE("fill queue frame offests are sequential and aligned", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());

    for (uint32_t i = 0; i < platform.fill_addrs.size(); i++)
    {
        unsigned long long expected =
            static_cast<unsigned long long>(i) * FRAME_SIZE;
        REQUIRE(platform.fill_addrs[i] == expected);
    }
}

TEST_CASE("fill queue init fails when reserve returns zero", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;
    platform.reserve_return = 0;

    auto umem = surma::capture::Umem::init(platform);

    REQUIRE_FALSE(umem.has_value());
    REQUIRE(umem.error() == surma::capture::UmemError::FqErr);
    REQUIRE(platform.submit_call_count == 0);
}

TEST_CASE("fill queue partial reserve logs warning but succeeds",
          "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;
    platform.reserve_return = FILL_RING_SIZE / 2;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE(umem.has_value());

    REQUIRE(platform.fill_addrs.size() == FILL_RING_SIZE / 2);
    REQUIRE(platform.last_submit_count == FILL_RING_SIZE / 2);
}

TEST_CASE("fill queue not populated when mmap fails", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_hugepage_fails = true;
    platform.mmap_return = MAP_FAILED;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE_FALSE(umem.has_value());
    REQUIRE(platform.reserve_call_count == 0);
    REQUIRE(platform.submit_call_count == 0);
}

TEST_CASE("fill queue not populated when xsk_umem_create fails", "[unit][umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;
    platform.umem_create_return = -1;

    auto umem = surma::capture::Umem::init(platform);
    REQUIRE_FALSE(umem.has_value());
    REQUIRE(platform.reserve_call_count == 0);
    REQUIRE(platform.submit_call_count == 0);
}
