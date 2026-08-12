#include "capture/umem.hpp"
#include "fake_platform.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace surma::test;

void *const sentinel_pointer = reinterpret_cast<void *>(0xDEAD0000);

TEST_CASE("umem init succeeds with hugepages", "[umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;

    surma::capture::Umem umem(platform);
    REQUIRE(umem.init() == 0);
    REQUIRE(platform.mmap_call_count == 1);
}

TEST_CASE("umem init falls back when hugepages unavailable", "[umem]")
{
    FakePlatform platform;
    platform.mmap_hugepage_fails = true;
    platform.mmap_return = sentinel_pointer;

    surma::capture::Umem umem(platform);
    REQUIRE(umem.init() == 0);
    REQUIRE(platform.mmap_call_count == 2);
}

TEST_CASE("umem init fails when both mmap calls fail", "[umem]")
{
    FakePlatform platform;
    platform.mmap_hugepage_fails = true;
    platform.mmap_return = MAP_FAILED;

    surma::capture::Umem umem(platform);
    REQUIRE(umem.init() != 0);
}

TEST_CASE("umem init fails when xsk_umem_create fails", "[umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;
    platform.umem_create_return = -1;

    surma::capture::Umem umem(platform);
    REQUIRE(umem.init() != 0);
}

TEST_CASE("umem destroy calls delete and unmap", "[umem]")
{
    FakePlatform platform;
    platform.mmap_return = sentinel_pointer;

    surma::capture::Umem umem(platform);
    REQUIRE(umem.init() == 0);

    umem.destroy();
    REQUIRE(platform.umem_delete_called);
    REQUIRE(umem.umem == nullptr);
    REQUIRE(platform.munmap_called);
    REQUIRE(umem.area == nullptr);
}

TEST_CASE("umem destroy is safe before init", "[umem]")
{
    FakePlatform platform;
    surma::capture::Umem umem(platform);

    REQUIRE_NOTHROW(umem.destroy());
    REQUIRE_FALSE(platform.munmap_called);
    REQUIRE_FALSE(platform.umem_delete_called);
}
