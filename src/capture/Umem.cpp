#include "Umem.hpp"
#include <cerrno>
#include <spdlog/spdlog.h>

namespace surma::capture
{

Umem::~Umem()
{
    if (umem_ != nullptr)
    {
        platform_.xsk_umem__delete(umem_);
        umem_ = nullptr;
    }
    if (area_ != nullptr)
    {
        platform_.munmap(area_, UMEM_SIZE);
        area_ = nullptr;
    }
}

std::expected<Umem, UmemError> Umem::init(Platform &platform)
{
    Umem ret(platform);

    struct xsk_umem_config cfg = {
        .fill_size = FILL_RING_SIZE,
        .comp_size = RX_RING_SIZE,
        .frame_size = FRAME_SIZE,
        .frame_headroom = 0,
        .flags = 0,
    };

    ret.area_ = platform.mmap(nullptr, UMEM_SIZE, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (ret.area_ == MAP_FAILED)
    {
        spdlog::warn("MAP_HUGETLB not available for UMEM");
        ret.area_ = platform.mmap(nullptr, UMEM_SIZE, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ret.area_ == MAP_FAILED)
        {
            spdlog::error("failed to map UMEM area");
            return std::unexpected(UmemError::MapErr);
        }
    }

    int res = platform.xsk_umem__create(&ret.umem_, ret.area_, UMEM_SIZE,
                                        &ret.fq_, &ret.cq_, &cfg);

    if (res != 0)
    {
        spdlog::error("xsk_umem__create failed: ret={}, errno={}, {}", res,
                      errno, std::strerror(errno));
        platform.munmap(ret.area_, UMEM_SIZE);
        return std::unexpected(UmemError::XskErr);
    }

    auto success = ret.populate_fill_queue_();
    if (!success)
        return std::unexpected(success.error());
    return ret;
}

std::expected<void, UmemError> Umem::populate_fill_queue_()
{
    uint32_t idx;
    uint32_t n = platform_.xsk_ring_prod__reserve(&fq_, FILL_RING_SIZE, &idx);
    if (n == 0)
    {
        spdlog::error("fill queue reserve failed: ring full at bootstrap");
        return std::unexpected(UmemError::FqErr);
    }
    if (n < FILL_RING_SIZE)
        spdlog::warn("fill queue reserve returned {} of {} slots", n,
                     FILL_RING_SIZE);

    for (uint32_t i = 0; i < n; i++)
    {
        *platform_.xsk_ring_prod__fill_addr(&fq_, idx++) =
            static_cast<uint64_t>(i) * FRAME_SIZE;
    }

    platform_.xsk_ring_prod__submit(&fq_, n);
    return {};
}

} // namespace surma::capture
