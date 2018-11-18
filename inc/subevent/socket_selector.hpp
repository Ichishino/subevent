#ifndef SUBEVENT_SOCKET_SELECTOR_HPP
#define SUBEVENT_SOCKET_SELECTOR_HPP

#include <map>
#include <list>
#include <vector>
#include <atomic>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>
#include <subevent/semaphore.hpp>

#ifdef SEV_OS_WIN
#elif defined(SEV_OS_MAC)
#include <sys/event.h>
#include <sys/time.h>
#elif defined(SEV_OS_LINUX)
#include <sys/epoll.h>
#endif

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketSelector
//---------------------------------------------------------------------------//

class SocketSelector
{
public:
    SEV_DECL SocketSelector();
    SEV_DECL ~SocketSelector();

    struct SocketEvents
    {
        struct EventItem
        {
            Socket::Handle sockHandle;
            int32_t errorCode;
        };

        bool isEmpty() const
        {
            return read.empty() &&
                write.empty() &&
                close.empty();
        }

        std::list<EventItem> read;
        std::list<EventItem> write;
        std::list<EventItem> close;
    };

#ifdef SEV_OS_WIN
    static const uint16_t MaxSockets = WSA_MAXIMUM_WAIT_EVENTS - 1;
    static const uint32_t Receive = FD_READ;
    static const uint32_t Send = FD_WRITE;
    static const uint32_t Accept = FD_ACCEPT;
    static const uint32_t Connect = FD_CONNECT;
    static const uint32_t Close = FD_CLOSE;
#elif defined(SEV_OS_MAC)
    static const uint16_t MaxSockets = UINT16_MAX;
    static const uint32_t Receive = 0x01;
    static const uint32_t Send = 0x02;
    static const uint32_t Accept = 0x04;
    static const uint32_t Connect = 0x08;
    static const uint32_t Close = 0;
#elif defined(SEV_OS_LINUX)
    static const uint16_t MaxSockets = UINT16_MAX;
    static const uint32_t Receive = EPOLLIN;
    static const uint32_t Send = EPOLLOUT;
    static const uint32_t Accept = EPOLLIN;
    static const uint32_t Connect = EPOLLOUT;
    static const uint32_t Close = EPOLLRDHUP;
#endif

    struct RegKey
    {
        Socket::Handle sockHandle;
#ifdef SEV_OS_WIN
        Win::Handle eventHandle;
#endif
    };

public:
    SEV_DECL bool registerSocket(
        Socket::Handle sockHandle, uint32_t eventFlags, RegKey& key);
    SEV_DECL void unregisterSocket(const RegKey& key);

    SEV_DECL WaitResult wait(uint32_t msec, SocketEvents& sockEvents);
    SEV_DECL void cancel();

    SEV_DECL uint32_t getSocketCount() const
    {
#ifdef SEV_OS_MAC
        return mSocketCount.load() - 1;
#else
        return mSocketCount.load();
#endif
    }

    SEV_DECL int32_t getErrorCode() const
    {
        return mErrorCode;
    }

private:

#ifdef SEV_OS_WIN
    std::map<Win::Handle, Socket::Handle> mSockHandleMap;
    Win::Handle mEventHandles[MaxSockets + 1];
    Semaphore mCancelSem;
#elif defined(SEV_OS_MAC)
    std::map<Socket::Handle, uint32_t> mSockHandleMap;
    int mKqueueFd;
    int mCancelFds[2];
#elif defined(SEV_OS_LINUX)
    int32_t mEpollFd;
    int32_t mCancelFd;
#endif

    std::atomic<uint32_t> mSocketCount;
    int32_t mErrorCode;
};

SEV_NS_END

#endif // SUBEVENT_SOCKET_SELECTOR_HPP
