#pragma once

#include "Platform.hpp"
#include "Socket.hpp"
#include <expected>

namespace surma::capture
{

enum class XdpError
{
    LoadError,
    AttachError,
};

class XdpProgram
{
  public:
    static std::expected<XdpProgram, XdpError> load(Platform &platform,
                                                    int ifindex);

    std::expected<void, XdpError> attach(Platform &platform,
                                         Socket &socket) const;

    [[nodiscard]] int xsks_map_fd() const
    {
        return xsks_map_fd_;
    };

  private:
    int xsks_map_fd_ = -1;
};

} // namespace surma::capture
