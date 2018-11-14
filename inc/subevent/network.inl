#ifndef SUBEVENT_NETWORK_INL
#define SUBEVENT_NETWORK_INL

#include <cassert>

#include <subevent/network.hpp>

#ifdef SEV_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <signal.h>
#endif

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Network
//---------------------------------------------------------------------------//

#ifdef SEV_OS_WIN
#pragma comment(lib, "ws2_32.lib")
#endif

Network::Network()
{
#ifdef SEV_OS_WIN
    WORD sockVer = MAKEWORD(2, 2);
    WSADATA wsaData;
    mErrorCode = WSAStartup(sockVer, &wsaData);
#else
    signal(SIGPIPE, SIG_IGN);
    mErrorCode = 0;
#endif
}

Network::~Network()
{
    if (mErrorCode == 0)
    {
#ifdef SEV_OS_WIN
        WSACleanup();
#endif
    }
}

bool Network::init()
{
    static Network network;
    return (network.mErrorCode == 0);
}

//---------------------------------------------------------------------------//
// NetWorker
//---------------------------------------------------------------------------//

NetWorker::NetWorker(Thread* thread)
    : mThread(thread)
{
    Network::init();

    // async socket controller
    mThread->setEventController(new SocketController());
}

NetWorker::~NetWorker()
{
}

SEV_NS_END

#endif // SUBEVENT_NETWORK_INL
