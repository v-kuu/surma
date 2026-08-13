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

TEST_CASE("real Linux platform can create and destroy UMEM",
          "[integration][umem]")
{
    surma::capture::LinuxPlatform platform;

    REQUIRE_NOTHROW(
        [&]
        {
            auto umem = surma::capture::Umem::init(platform);
            REQUIRE(umem.has_value());
        }());
}
