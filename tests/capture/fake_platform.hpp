#pragma once
#include "capture/Platform.hpp"
#include <cstring>

namespace surma::test
{

struct FakePlatform : surma::capture::Platform
{
    void *mmap_return = nullptr;
    bool mmap_hugepage_fails = false;
    int umem_create_return = 0;

    bool munmap_called = false;
    bool umem_delete_called = false;
    int mmap_call_count = 0;

    void *mmap(void *, size_t, int, int flags, int, off_t) override
    {
        mmap_call_count++;
        if ((flags & MAP_HUGETLB) && mmap_hugepage_fails)
            return MAP_FAILED;
        return mmap_return;
    }

    int munmap(void *, size_t) override
    {
        munmap_called = true;
        return 0;
    }

    int xsk_umem__create(struct xsk_umem **umem, void *, uint64_t,
                         struct xsk_ring_prod *, struct xsk_ring_cons *,
                         const struct xsk_umem_config *) override
    {
        if (umem_create_return == 0)
            *umem = reinterpret_cast<struct xsk_umem *>(0xDEAD1000);
        return umem_create_return;
    }

    int xsk_umem__delete(struct xsk_umem *) override
    {
        umem_delete_called = true;
        return 0;
    }
};

} // namespace surma::test
