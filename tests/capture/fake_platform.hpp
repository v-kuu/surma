#pragma once
#include "capture/Platform.hpp"
#include <cstring>

namespace surma::test
{

struct FakePlatform : surma::capture::Platform
{
    // umem controls
    void *mmap_return = nullptr;
    bool mmap_hugepage_fails = false;
    int umem_create_return = 0;
    bool munmap_called = false;
    bool umem_delete_called = false;
    int mmap_call_count = 0;

    // socket controls
    int socket_create_return = 0;
    int socket_fd_return = 42;
    bool socket_delete_called = false;
    int socket_delete_count = 0;
    struct xsk_socket *socket_delete_arg = nullptr;
    const char *last_iface = nullptr;
    uint32_t last_queue_id = 0;

    // umem fakes
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

    // socket fakes
    int xsk_socket__create(struct xsk_socket **xsk, const char *iface,
                           uint32_t queue_id, struct xsk_umem *,
                           struct xsk_ring_cons *, struct xsk_ring_prod *,
                           const struct xsk_socket_config *) override
    {
        last_iface = iface;
        last_queue_id = queue_id;

        if (socket_create_return == 0)
        {
            *xsk = reinterpret_cast<struct xsk_socket *>(0xDEAD2000);
        }

        return socket_create_return;
    }

    int xsk_socket__fd(struct xsk_socket *) override
    {
        return socket_fd_return;
    }

    void xsk_socket__delete(struct xsk_socket *xsk) override
    {
        socket_delete_called = true;
        socket_delete_arg = xsk;
        socket_delete_count++;
    }
};

} // namespace surma::test
