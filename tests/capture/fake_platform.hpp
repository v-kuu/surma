#pragma once
#include "capture/Platform.hpp"
#include <cstring>
#include <linux/if_link.h>

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
    bool socket_drv_mode_fails = false;
    int socket_create_call_count = 0;
    int xdp_flags = 0;

    // xdp prog controls
    int setup_xdp_prog_return = 0;
    int setup_xdp_prog_map_fd_return = 42;
    int last_ifindex = -1;

    int update_xskmap_return = 0;
    bool update_xskmap_called = false;
    int last_xsks_map_fd = -1;
    xsk_socket *last_socket = nullptr;

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
                           const struct xsk_socket_config *cfg) override
    {
        last_iface = iface;
        last_queue_id = queue_id;
        socket_create_call_count++;

        if (socket_drv_mode_fails && (cfg->xdp_flags & XDP_FLAGS_DRV_MODE))
            return -1;

        if (socket_create_return == 0)
            *xsk = reinterpret_cast<struct xsk_socket *>(0xDEAD2000);

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

    // xdp prog fakes
    int xsk_setup_xdp_prog(int ifindex, int *xsks_map_fd) override
    {
        last_ifindex = ifindex;
        if (xsks_map_fd != nullptr)
            *xsks_map_fd = setup_xdp_prog_map_fd_return;

        return setup_xdp_prog_return;
    }

    int xsk_socket__update_xskmap(xsk_socket *xsk, int xsks_map_fd) override
    {
        update_xskmap_called = true;
        last_socket = xsk;
        last_xsks_map_fd = xsks_map_fd;

        return update_xskmap_return;
    }
};

} // namespace surma::test
