#include "umem.hpp"

int umem::init()
{
    struct xsk_umem_config cfg = {
        .fill_size = FILL_RING_SIZE,
        .com_size = RX_RING_SIZE,
        .frame_size = FRAME_SIZE,
        .frame_headroom = 0,
        .flags = 0,
    };

    area = mmap(NULL, UMEM_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (area == MAP_FAILED)
    {
        spdlog::warn("MAP_HUGETLB not available for UMEM");
        area = mmap(NULL, UMEM_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (area == MAP_FAILED)
        {
            spdlog::error("failed to map UMEM area");
            return -1;
        }
    }

    return xsk_umem__create(umem, area, UMEM_SIZE, &fq, &cq, &cfg);
}

void umem::destroy()
{
    xsk_umem__delete(umem);
    munmap(area, UMEM_SIZE);
}
