#ifndef SUBEVENT_SOCKET_SELECTOR_LINUX_INL
#define SUBEVENT_SOCKET_SELECTOR_LINUX_INL

#ifdef SEV_OS_LINUX

#include <cassert>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <subevent/socket_selector.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketSelector
//---------------------------------------------------------------------------//

SocketSelector::SocketSelector()
{
    mErrorCode = 0;
    mEpollFd = -1;
    mCancelFd = -1;
    mSocketCount = 0;

    mEpollFd = epoll_create1(0);
    if (mEpollFd == -1)
    {
        mErrorCode = errno;
        assert(false);
        return;
    }

    mCancelFd = eventfd(0, (EFD_NONBLOCK | EFD_SEMAPHORE));
    if (mCancelFd == -1)
    {
        mErrorCode = errno;
        assert(false);
        return;
    }

    struct epoll_event e;
    memset(&e, 0x00, sizeof(e));

    e.data.fd = mCancelFd;
    e.events = EPOLLIN;

    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mCancelFd, &e) != 0)
    {
        mErrorCode = errno;
        assert(false);
        return;
    }
}

SocketSelector::~SocketSelector()
{
    assert(mSocketCount == 0);

    if ((mEpollFd >= 0) && (mCancelFd >= 0))
    {
        struct epoll_event e;
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, mCancelFd, &e);
    }

    if (mCancelFd >= 0)
    {
        close(mCancelFd);
    }

    if (mEpollFd >= 0)
    {
        close(mEpollFd);
    }
}

bool SocketSelector::registerSocket(
    Socket::Handle sockHandle, uint32_t eventFlags, RegKey& key)
{
    struct epoll_event e;
    memset(&e, 0x00, sizeof(e));

    e.data.fd = sockHandle;
    e.events = eventFlags | EPOLLET;

    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, sockHandle, &e) != 0)
    {
        mErrorCode = errno;
        assert(false);
        return false;
    }

    int flag = fcntl(sockHandle, F_GETFL, 0);
    fcntl(sockHandle, F_SETFL, (flag | O_NONBLOCK));

    key.sockHandle = sockHandle;
    ++mSocketCount;

    return true;
}

void SocketSelector::unregisterSocket(const RegKey& key)
{
    assert(mSocketCount > 0);

    struct epoll_event e;
    if (epoll_ctl(mEpollFd, EPOLL_CTL_DEL, key.sockHandle, &e) != 0)
    {
        mErrorCode = errno;
        assert(false);
    }

    int flag = fcntl(key.sockHandle, F_GETFL, 0);
    fcntl(key.sockHandle, F_SETFL, (flag & ~O_NONBLOCK));

    --mSocketCount;
}

WaitResult SocketSelector::wait(uint32_t msec, SocketEvents& sockEvents)
{
    WaitResult result = WaitResult::Success;

    bool canceled = false;
    struct epoll_event e[10240];

    int count = epoll_wait(
        mEpollFd, e, 10240, static_cast<int>(msec));

    if (count > 0)
    {
        for (int i = 0; i < count; ++i)
        {
            if (e[i].data.fd == mCancelFd)
            {
                canceled = true;

                eventfd_t val;
                eventfd_read(mCancelFd, &val);
            }
            else
            {
                int32_t errorCode = 0;

                if (e[i].events & EPOLLERR)
                {
                    socklen_t len = sizeof(errorCode);
                    getsockopt(
                        e[i].data.fd, SOL_SOCKET, SO_ERROR,
                        &errorCode, &len);
                }

                if ((e[i].events & EPOLLRDHUP) || (e[i].events & EPOLLHUP))
                {
                    sockEvents.close.push_back(
                        { e[i].data.fd, errorCode });
                }

                if (e[i].events & EPOLLIN)
                {
                    sockEvents.read.push_back(
                        { e[i].data.fd, errorCode });
                }

                if (e[i].events & EPOLLOUT)
                {
                    sockEvents.write.push_back(
                        { e[i].data.fd, errorCode });
                }
            }
        }

        if (!sockEvents.isEmpty())
        {
            result = WaitResult::Success;
        }
    }
    else if (count == 0)
    {
        // timeout
        result = WaitResult::Timeout;
    }
    else if (errno != EINTR)
    {
        assert(false);

        // error
        result = WaitResult::Error;

        mErrorCode = errno;
    }

    return (canceled ? WaitResult::Cancel : result);
}

void SocketSelector::cancel()
{
    eventfd_write(mCancelFd, 1);
}

SEV_NS_END

#endif // SEV_OS_WIN

#endif // SUBEVENT_SOCKET_SELECTOR_LINUX_INL
