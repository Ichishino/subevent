#ifndef SUBEVENT_SOCKET_SELECTOR_MAC_INL
#define SUBEVENT_SOCKET_SELECTOR_MAC_INL

#ifdef SEV_OS_MAC

#include <cassert>
#include <unistd.h>
#include <fcntl.h>

#include <subevent/socket_selector.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketSelector
//---------------------------------------------------------------------------//

SocketSelector::SocketSelector()
{
    mErrorCode = 0;
    mSocketCount = 0;
    mCancelFds[0] = -1;
    mCancelFds[1] = -1;
    mKqueueFd = -1;

    if (pipe(mCancelFds) != 0)
    {
        mErrorCode = errno;
        return;
    }
    
    int flag0 = fcntl(mCancelFds[0], F_GETFL);
    flag0 |= O_NONBLOCK;
    fcntl(mCancelFds[0], F_SETFL, flag0);
    int flag1 = fcntl(mCancelFds[1], F_GETFL);
    flag1 |= O_NONBLOCK;
    fcntl(mCancelFds[1], F_SETFL, flag1);
    
    mKqueueFd = kqueue();
    
    if (mKqueueFd == -1)
    {
        mErrorCode = errno;
        return;
    }
    
    RegKey key;
    if (!registerSocket(mCancelFds[0], Receive, key))
    {
        return;
    }
}

SocketSelector::~SocketSelector()
{
    assert(mSocketCount == 1);
    
    if (mCancelFds[0] != -1)
    {
        RegKey key;
        key.sockHandle = mCancelFds[0];
        unregisterSocket(key);

        close(mCancelFds[0]);
        mCancelFds[0] = -1;
    }

    if (mCancelFds[1] != -1)
    {
        close(mCancelFds[1]);
        mCancelFds[1] = -1;
    }
    
    if (mKqueueFd != -1)
    {
        close(mKqueueFd);
        mKqueueFd = -1;
    }
}

bool SocketSelector::registerSocket(
    Socket::Handle sockHandle, uint32_t eventFlags, RegKey& key)
{
    int count = 0;
    struct kevent ev[2];
    
    if ((eventFlags & Receive) ||
        (eventFlags & Accept))
    {
        EV_SET(&ev[count++], sockHandle,
            EVFILT_READ, (EV_ADD | EV_CLEAR), 0, 0, 0);
    }

    if ((eventFlags & Send) ||
        (eventFlags & Connect))
    {
        EV_SET(&ev[count++], sockHandle,
            EVFILT_WRITE, (EV_ADD | EV_CLEAR), 0, 0, 0);
    }

    if (kevent(mKqueueFd, ev, count, 0, 0, 0) == -1)
    {
        mErrorCode = errno;
        return false;
    }
    
    int flag = fcntl(sockHandle, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(sockHandle, F_SETFL, flag);
    
    mSockHandleMap[sockHandle] = eventFlags;
    key.sockHandle = sockHandle;
    ++mSocketCount;
    
    return true;
}

void SocketSelector::unregisterSocket(const RegKey& key)
{
    assert(mSocketCount > 0);

    auto it = mSockHandleMap.find(key.sockHandle);
    if (it == mSockHandleMap.end())
    {
        return;
    }
    
    Socket::Handle sockHandle = it->first;
    uint32_t eventFlags = it->second;

    int count = 0;
    struct kevent ev[2];

    if ((eventFlags & Receive) ||
        (eventFlags & Accept))
    {
        EV_SET(&ev[count++], sockHandle,
            EVFILT_READ, EV_DELETE, 0, 0, 0);
    }
    
    if ((eventFlags & Send) ||
        (eventFlags & Connect))
    {
        EV_SET(&ev[count++], sockHandle,
            EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    }
    
    if (kevent(mKqueueFd, ev, count, 0, 0, 0) == -1)
    {
        mErrorCode = errno;
        return;
    }
    
    int flag = fcntl(key.sockHandle, F_GETFL, 0);
    fcntl(key.sockHandle, F_SETFL, (flag & ~O_NONBLOCK));

    mSockHandleMap.erase(it);
    --mSocketCount;
}

WaitResult SocketSelector::wait(uint32_t msec, SocketEvents& sockEvents)
{
    WaitResult result = WaitResult::Success;

    struct timespec ts = {};
    
    if (msec != -1)
    {
        ts.tv_sec = msec / 1000;
        ts.tv_nsec = msec % 1000 * 1000000;
    }

    bool canceled = false;
    struct kevent e[10240];

    int count = kevent(
        mKqueueFd, 0, 0, e, 10240, ((msec == UINT32_MAX) ? NULL : &ts));

    if (count > 0)
    {
        for (int index = 0; index < count; ++index)
        {
            Socket::Handle sockHandle =
                static_cast<Socket::Handle>(e[index].ident);

            if (sockHandle == mCancelFds[0])
            {
                canceled = true;

                char val;
                read(mCancelFds[0], &val, sizeof(val));
            }
            else
            {
                int32_t errorCode = 0;
                uint32_t eventFlags =
                    mSockHandleMap.find(sockHandle)->second;

                if (e[index].flags & EV_ERROR)
                {
                    errorCode = static_cast<int32_t>(e[index].data);
                }

                if (e[index].flags & EV_EOF)
                {
                    if (errorCode == 0)
                    {
                        errorCode = e[index].fflags;
                    }

                    if (errorCode == 0)
                    {
                        socklen_t len = sizeof(errorCode);
                        getsockopt(
                            sockHandle, SOL_SOCKET, SO_ERROR,
                            &errorCode, &len);
                    }
                }

                if (e[index].filter == EVFILT_READ)
                {
                    if ((eventFlags & Accept) || (e[index].data > 0))
                    {
                        sockEvents.read.push_back({ sockHandle, errorCode });
                    }
                    else
                    {
                        sockEvents.close.push_back({ sockHandle, errorCode });
                    }
                }
                else if (e[index].filter == EVFILT_WRITE)
                {
                    sockEvents.write.push_back({ sockHandle, errorCode });
                }
            }
        }
    }
    else if (count == 0)
    {
        result = WaitResult::Timeout;
    }
    else
    {
        result = WaitResult::Error;
        mErrorCode = errno;
    }

    return (canceled ? WaitResult::Cancel : result);
}

void SocketSelector::cancel()
{
    char val = 1;
    write(mCancelFds[1], &val, sizeof(val));
}

SEV_NS_END

#endif // SEV_OS_MAC

#endif // SUBEVENT_SOCKET_SELECTOR_MAC_INL
