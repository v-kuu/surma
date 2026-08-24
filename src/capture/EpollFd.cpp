#include "EpollFd.hpp"
#include <cerrno>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace surma::capture
{

EpollFd::~EpollFd()
{
    if (fd_ >= 0)
        close(fd_);
}

std::expected<EpollFd, EpollErr> EpollFd::init(int xsk_fd)
{
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0)
    {
        spdlog::error("epoll_create1 failed: {} ({})", errno,
                      std::strerror(errno));
        return std::unexpected(EpollErr::CreateErr);
    }

    struct epoll_event ev{};
    // TODO: consider edge-triggered (EPOLLET)
    ev.events = EPOLLIN;
    ev.data.fd = xsk_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, xsk_fd, &ev) < 0)
    {
        spdlog::error("epoll_ctl failed: {} ({})", errno, std::strerror(errno));
        close(epfd);
        return std::unexpected(EpollErr::CtlErr);
    }

    return EpollFd{epfd};
}

} // namespace surma::capture
