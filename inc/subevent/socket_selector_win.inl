#ifndef SUBEVENT_SOCKET_SELECTOR_WIN_INL
#define SUBEVENT_SOCKET_SELECTOR_WIN_INL

#ifdef SEV_OS_WIN

#include <cassert>

#include <subevent/socket_selector.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketSelector
//---------------------------------------------------------------------------//

SocketSelector::SocketSelector()
{
    mErrorCode = 0;
    mSocketCount = 0;
    mEventHandles[0] = mCancelSem.getHandle();
}

SocketSelector::~SocketSelector()
{
    assert(mSockHandleMap.empty());
    assert(mSocketCount == 0);
}

bool SocketSelector::registerSocket(
    Socket::Handle sockHandle, uint32_t eventFlags, RegKey& key)
{
    if (mSocketCount >= MaxSockets)
    {
        return false;
    }

    WSAEVENT eventHandle = WSACreateEvent();

    if (WSAEventSelect(
        sockHandle, eventHandle, eventFlags) == SOCKET_ERROR)
    {
        assert(false);
        mErrorCode = WSAGetLastError();
        WSACloseEvent(eventHandle);

        return false;
    }

    ++mSocketCount;

    mSockHandleMap[eventHandle] = sockHandle;
    mEventHandles[mSocketCount] = eventHandle;

    key.sockHandle = sockHandle;
    key.eventHandle = eventHandle;

    return true;
}

void SocketSelector::unregisterSocket(const RegKey& key)
{
    Socket::Handle sockHandle = key.sockHandle;
    Win::Handle eventHandle = key.eventHandle;

    assert(mSockHandleMap.count(key.eventHandle) == 1);
    assert(mSocketCount > 0);

    for (uint32_t index = 1; index <= mSocketCount; ++index)
    {
        if (mEventHandles[index] == eventHandle)
        {
            mEventHandles[index] = mEventHandles[mSocketCount];
            mEventHandles[mSocketCount] = nullptr;

            --mSocketCount;

            WSAEventSelect(sockHandle, NULL, 0);
            WSACloseEvent(eventHandle);

            u_long value = 0;
            ioctlsocket(sockHandle, FIONBIO, &value);

            mSockHandleMap.erase(eventHandle);

            break;
        }
    }
}

WaitResult SocketSelector::wait(uint32_t msec, SocketEvents& sockEvents)
{
    DWORD result = WSAWaitForMultipleEvents(
        mSocketCount + 1, mEventHandles,
        FALSE, msec, FALSE);

    if (result == WSA_WAIT_EVENT_0)
    {
        return WaitResult::Cancel;
    }
    else if (result == WSA_WAIT_TIMEOUT)
    {
        return WaitResult::Timeout;
    }
    else if ((result >= (WSA_WAIT_EVENT_0 + 1)) &&
             (result <= (WSA_WAIT_EVENT_0 + mSocketCount)))
    {
        DWORD index = result - WSA_WAIT_EVENT_0;

        WSAEVENT eventHandle = mEventHandles[index];
        Socket::Handle sockHandle = mSockHandleMap[eventHandle];

        WSANETWORKEVENTS netEvents;

        if (WSAEnumNetworkEvents(
            sockHandle,
            eventHandle,
            &netEvents) == SOCKET_ERROR)
        {
            assert(false);
            mErrorCode = WSAGetLastError();

            return WaitResult::Error;
        }

        if (netEvents.lNetworkEvents & FD_READ)
        {
            sockEvents.read.push_back(
                { sockHandle, netEvents.iErrorCode[FD_READ_BIT] });
        }

        if (netEvents.lNetworkEvents & FD_WRITE)
        {
            sockEvents.write.push_back(
                { sockHandle, netEvents.iErrorCode[FD_WRITE_BIT] });
        }

        if (netEvents.lNetworkEvents & FD_CLOSE)
        {
            sockEvents.close.push_back(
                { sockHandle, netEvents.iErrorCode[FD_CLOSE_BIT] });
        }

        if (netEvents.lNetworkEvents & FD_CONNECT)
        {
            sockEvents.write.push_back(
                { sockHandle, netEvents.iErrorCode[FD_CONNECT_BIT] });
        }

        if (netEvents.lNetworkEvents & FD_ACCEPT)
        {
            sockEvents.read.push_back(
                { sockHandle, netEvents.iErrorCode[FD_ACCEPT_BIT] });
        }

        return WaitResult::Success;
    }
    else
    {
        assert(false);
        mErrorCode = WSAGetLastError();

        return WaitResult::Error;
    }
}

void SocketSelector::cancel()
{
    mCancelSem.post();
}

SEV_NS_END

#endif // SEV_OS_WIN

#endif // SUBEVENT_SOCKET_SELECTOR_WIN_INL
