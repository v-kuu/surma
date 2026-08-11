#pragma once

#include <spdlog/spdlog.h>
#include <xdp/xsk.h>

#define UMEM_SIZE (1 << 23)
#define FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define FRAME_COUNT (UMEM_SIZE / FRAME_SIZE)
#define RX_RING_SIZE XSK_RING_CONS__DEFAULT_NUM_DESCS
#define FILL_RING_SIZE XSK_RING_PROD__DEFAULT_NUM_DESCS
#define BATCH_SIZE 64

struct umem
{
    int init();
    void destroy();

    void *area;
    struct xsk_umem *umem;
    struct xsk_ring_prod fq;
    struct xsk_ring_cons cq;
}
