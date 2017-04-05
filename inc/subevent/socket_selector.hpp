#ifndef SUBEVENT_SOCKET_SELECTOR_HPP
#define SUBEVENT_SOCKET_SELECTOR_HPP

#include <map>
#include <list>
#include <vector>
#include <atomic>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>
#include <subevent/semaphore.hpp>

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

#ifdef _WIN32
    static const uint16_t MaxSockets = WSA_MAXIMUM_WAIT_EVENTS - 1;
    static const uint32_t Receive = FD_READ;
    static const uint32_t Send = FD_WRITE;
    static const uint32_t Accept = FD_ACCEPT;
    static const uint32_t Connect = FD_CONNECT;
    static const uint32_t Close = FD_CLOSE;
#else
    static const uint16_t MaxSockets = 1024;
    static const uint32_t Receive = EPOLLIN;
    static const uint32_t Send = EPOLLOUT;
    static const uint32_t Accept = EPOLLIN;
    static const uint32_t Connect = EPOLLOUT;
    static const uint32_t Close = EPOLLRDHUP;
#endif

    struct RegKey
    {
        Socket::Handle sockHandle;
#ifdef _WIN32
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
        return mSocketCount.load();
    }

    SEV_DECL int32_t getErrorCode() const
    {
        return mErrorCode;
    }

private:

#ifdef _WIN32
    std::map<Win::Handle, Socket::Handle> mSockHandleMap;
    Win::Handle mEventHandles[MaxSockets + 1];
    Semaphore mCancelSem;
#else
    int32_t mEpollFd;
    int32_t mCancelFd;
#endif

    std::atomic<uint32_t> mSocketCount;
    int32_t mErrorCode;
};

SEV_NS_END

#endif // SUBEVENT_SOCKET_SELECTOR_HPP
