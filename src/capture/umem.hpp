#pragma once

#include "platform.hpp"

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

enum UmemError
{
    MapErr,
    XskErr,
};

struct Umem
{
  private:
    explicit Umem(Platform &platform) : platform(platform)
    {
    }

  public:
    Umem() = delete;
    ~Umem();
    Umem(const Umem &other) = delete;
    Umem &operator=(const Umem &other) = delete;
    Umem(Umem &&other) noexcept
        : platform(other.platform), area(std::exchange(other.area, nullptr)),
          umem(std::exchange(other.umem, nullptr)), fq(other.fq), cq(other.cq)
    {
    }
    Umem &operator=(Umem &&) = delete;

    static std::expected<Umem, UmemError> init(Platform &platform);

    Platform &platform;
    void *area;
    struct xsk_umem *umem;
    struct xsk_ring_prod fq;
    struct xsk_ring_cons cq;
};

} // namespace surma::capture
