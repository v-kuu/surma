#pragma once

#include "Platform.hpp"

#include <expected>
#include <sys/mman.h>
#include <utility>
#include <xdp/xsk.h>

#define UMEM_SIZE (1 << 23)
#define FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define FRAME_COUNT (UMEM_SIZE / FRAME_SIZE)
#define RX_RING_SIZE XSK_RING_CONS__DEFAULT_NUM_DESCS
#define FILL_RING_SIZE XSK_RING_PROD__DEFAULT_NUM_DESCS
#define BATCH_SIZE 64

namespace surma::capture
{

enum class UmemError
{
    MapErr,
    XskErr,
};

class Umem
{
  public:
    Umem() = delete;
    ~Umem();
    Umem(const Umem &other) = delete;
    Umem &operator=(const Umem &other) = delete;
    Umem(Umem &&other) noexcept
        : platform_(other.platform_),
          area_(std::exchange(other.area_, nullptr)),
          umem_(std::exchange(other.umem_, nullptr)), fq_(other.fq_),
          cq_(other.cq_)
    {
    }
    Umem &operator=(Umem &&) = delete;

    xsk_umem *handle()
    {
        return umem_;
    };
    static std::expected<Umem, UmemError> init(Platform &platform);

  private:
    explicit Umem(Platform &platform) : platform_(platform)
    {
    }

    Platform &platform_;
    void *area_;
    struct xsk_umem *umem_;
    struct xsk_ring_prod fq_;
    struct xsk_ring_cons cq_;
};

} // namespace surma::capture
