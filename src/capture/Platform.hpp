#pragma once
#include <cstdint>
#include <sys/mman.h>
#include <xdp/xsk.h>

namespace surma::capture
{

/*
 *	A thin virtual interface to enable testing of UMEM functions.
 *	Introduces some overhead to program setup, none to runtime hot path.
 */
struct Platform
{
    virtual ~Platform() = default;

    virtual void *mmap(void *addr, size_t length, int prot, int flags, int fd,
                       off_t offset) = 0;

    virtual int munmap(void *addr, size_t length) = 0;

    virtual int xsk_umem__create(struct xsk_umem **umem, void *umem_area,
                                 uint64_t size, struct xsk_ring_prod *fq,
                                 struct xsk_ring_cons *cq,
                                 const struct xsk_umem_config *config) = 0;

    virtual int xsk_umem__delete(struct xsk_umem *umem) = 0;

    virtual void xsk_socket__delete(struct xsk_socket *xsk) = 0;

    virtual int xsk_socket__create(struct xsk_socket **xsk, const char *iface,
                                   uint32_t queue_id, struct xsk_umem *umem,
                                   struct xsk_ring_cons *rx,
                                   struct xsk_ring_prod *tx,
                                   const struct xsk_socket_config *cfg) = 0;

    virtual int xsk_socket__fd(struct xsk_socket *xsk) = 0;
};

// Production implementation simply calls the real functions
struct LinuxPlatform : Platform
{
    void *mmap(void *addr, size_t length, int prot, int flags, int fd,
               off_t offset) override
    {
        return ::mmap(addr, length, prot, flags, fd, offset);
    }

    int munmap(void *addr, size_t length) override
    {
        return ::munmap(addr, length);
    }

    int xsk_umem__create(struct xsk_umem **umem, void *umem_area, uint64_t size,
                         struct xsk_ring_prod *fq, struct xsk_ring_cons *cq,
                         const struct xsk_umem_config *config) override
    {
        return ::xsk_umem__create(umem, umem_area, size, fq, cq, config);
    }

    int xsk_umem__delete(struct xsk_umem *umem) override
    {
        return ::xsk_umem__delete(umem);
    }

    void xsk_socket__delete(struct xsk_socket *xsk) override
    {
        return ::xsk_socket__delete(xsk);
    }

    int xsk_socket__create(struct xsk_socket **xsk, const char *iface,
                           uint32_t queue_id, struct xsk_umem *umem,
                           struct xsk_ring_cons *rx, struct xsk_ring_prod *tx,
                           const struct xsk_socket_config *cfg) override
    {
        return ::xsk_socket__create(xsk, iface, queue_id, umem, rx, tx, cfg);
    }

    int xsk_socket__fd(struct xsk_socket *xsk) override
    {
        return ::xsk_socket__fd(xsk);
    }
};

} // namespace surma::capture
