#ifndef SUBEVENT_NETWORK_INL
#define SUBEVENT_NETWORK_INL

#include <cassert>

#include <subevent/network.hpp>

#ifdef _WIN32
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

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

Network::Network()
{
#ifdef _WIN32
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
#ifdef _WIN32
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

    // tcp accept for multithreading
    mThread->setEventHandler(
        TcpEventId::Accept, [&](const Event* event) {

        TcpChannelPtr channel = TcpServer::accept(event);

        if (channel != nullptr)
        {
            onTcpAccept(channel);
        }
    });
}

NetWorker::~NetWorker()
{
}

bool NetWorker::requestTcpAccept(const TcpChannelPtr& newChannel)
{
    return mThread->post(new TcpAcceptEvent(newChannel));
}

void NetWorker::onTcpAccept(const TcpChannelPtr& /* newChannel */)
{
    // for multithreading
}

SEV_NS_END

#endif // SUBEVENT_NETWORK_INL
