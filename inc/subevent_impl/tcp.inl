#ifndef SUBEVENT_TCP_INL
#define SUBEVENT_TCP_INL

#include <cassert>

#include <subevent/tcp.hpp>
#include <subevent/thread.hpp>
#include <subevent/network.hpp>
#include <subevent/socket_controller.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// TcpServer
//----------------------------------------------------------------------------//

TcpServer::TcpServer()
{
    mSocket = nullptr;
}

TcpServer::~TcpServer()
{
    delete mSocket;
}

bool TcpServer::open(
    const IpEndPoint& localEndPoint,
    const TcpAcceptHandler& acceptHandler,
    int32_t listenBacklog)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (!isClosed())
    {
        return false;
    }

    Socket* socket = new Socket();

    // create
    if (!socket->create(
        localEndPoint.getFamily(),
        Socket::Type::Tcp,
        Socket::Protocol::Tcp))
    {
        assert(false);
        delete socket;

        return false;
    }

    // option
    socket->setOption(mSockOption);

    // bind
    if (!socket->bind(localEndPoint))
    {
        assert(false);
        delete socket;

        return false;
    }

    // listen
    if (!socket->listen(listenBacklog))
    {
        delete mSocket;

        return false;
    }

    mSocket = socket;
    mLocalEndPoint = localEndPoint;
    mAcceptHandler = acceptHandler;

    return Network::getController()->
        registerTcpServer(shared_from_this());
}

void TcpServer::close()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return;
    }

    Network::getController()->
        unregisterTcpServer(shared_from_this());

    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mLocalEndPoint.clear();
    mAcceptHandler = nullptr;
}

bool TcpServer::accept(Thread* thread, const TcpChannelPtr& channel)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return false;
    }

    if (thread == Thread::getCurrent())
    {
        if (!Network::getController()->registerTcpChannel(channel))
        {
            return false;
        }
    }
    else
    {
        thread->post(new TcpAcceptEvent(channel));
    }

    return true;
}

TcpChannelPtr TcpServer::accept(const Event* event)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    const TcpAcceptEvent* acceptEvent =
        dynamic_cast<const TcpAcceptEvent*>(event);

    TcpChannelPtr channel;
    acceptEvent->getParams(channel);

    if (!Network::getController()->registerTcpChannel(channel))
    {
        return nullptr;
    }

    return channel;
}

SocketOption& TcpServer::getSocketOption()
{
    if (!isClosed())
    {
        mSockOption = mSocket->getOption();
    }

    return mSockOption;
}

void TcpServer::onAccept()
{
    if (mAcceptHandler != nullptr)
    {
        Socket* socket = mSocket->accept();

        if (socket == nullptr)
        {
            return;
        }

        TcpServerPtr self(shared_from_this());
        TcpChannelPtr channel(new TcpChannel(socket));
        TcpAcceptHandler handler = mAcceptHandler;

        Thread::getCurrent()->post([self, handler, channel]() {
            handler(self, channel);
        });
    }
}

void TcpServer::onClose()
{
    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mAcceptHandler = nullptr;
}

//----------------------------------------------------------------------------//
// TcpChannel
//----------------------------------------------------------------------------//

TcpChannel::TcpChannel()
{
    mSocket = nullptr;
}

TcpChannel::TcpChannel(Socket* socket)
{
    mSocket = nullptr;
    create(socket);
}

TcpChannel::~TcpChannel()
{
    delete mSocket;
}

void TcpChannel::create(Socket* socket)
{
    delete mSocket;
    mLocalEndPoint.clear();
    mPeerEndPoint.clear();

    mSocket = socket;

    if (mSocket != nullptr)
    {
        mSocket->getLocalEndPoint(mLocalEndPoint);
        mSocket->getPeerEndPoint(mPeerEndPoint);
    }
}

void TcpChannel::close()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        if (mCloseCanceller != nullptr)
        {
            mCloseCanceller->cancel();
        }

        return;
    }

    mSockOption.clear();
    mReceiveHandler = nullptr;
    mCloseHandler = nullptr;
    mCloseCanceller.reset();
    mSendHandlers.clear();

    Network::getController()->
        requestTcpChannelClose(shared_from_this());

    delete mSocket;
    mSocket = nullptr;
}

int32_t TcpChannel::send(const void* data, uint32_t size,
    const TcpSendHandler& sendHandler)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (sendHandler == nullptr)
    {
        // sync

        return mSocket->send(data, size, Socket::SendFlags);
    }
    else
    {
        // async

        mSendHandlers.push_back(sendHandler);

        if (!Network::getController()->
            requestTcpSend(shared_from_this(), data, size))
        {
            return -1;
        }

        return 0;
    }
}

int32_t TcpChannel::receive(void* buff, uint32_t size)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    int32_t result = mSocket->receive(buff, size);
    if ((result == 0) && (size > 0))
    {
        Network::getController()->
            onTcpReceiveEof(shared_from_this());
    }

    return result;
}

bool TcpChannel::cancelSend()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return false;
    }

    return Network::getController()->
        cancelTcpSend(shared_from_this());
}

void TcpChannel::setReceiveHandler(
    const TcpReceiveHandler& receiveHandler)
{
    mReceiveHandler = receiveHandler;
}

void TcpChannel::setCloseHandler(
    const TcpCloseHandler& closeHandler)
{
    mCloseHandler = closeHandler;
    mCloseCanceller.reset();
}

SocketOption& TcpChannel::getSocketOption()
{
    if (!isClosed())
    {
        mSockOption = mSocket->getOption();
    }

    return mSockOption;
}

void TcpChannel::onReceive()
{
    if (mReceiveHandler != nullptr)
    {
        TcpChannelPtr self = shared_from_this();
        TcpReceiveHandler handler = mReceiveHandler;

        Thread::getCurrent()->post([self, handler]() {
            handler(self);
        });
    }
}

void TcpChannel::onSend(int32_t errorCode)
{
    if (!mSendHandlers.empty())
    {
        TcpChannelPtr self = shared_from_this();
        TcpSendHandler handler = mSendHandlers.front();
        mSendHandlers.pop_front();

        Thread::getCurrent()->post([self, handler, errorCode]() {
            handler(self, errorCode);
        });
    }
}

void TcpChannel::onClose()
{
    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mReceiveHandler = nullptr;
    mSendHandlers.clear();

    if (mCloseHandler != nullptr)
    {
        TcpChannelPtr self = shared_from_this();
        TcpCloseHandler handler = mCloseHandler;
        mCloseHandler = nullptr;

        mCloseCanceller = postCancelableTask(
            Thread::getCurrent(), [self, handler]() {
                handler(self);
            });
    }
}

//----------------------------------------------------------------------------//
// TcpClient
//----------------------------------------------------------------------------//

TcpClient::TcpClient()
    : mChannel(new TcpChannel())
{
}

TcpClient::~TcpClient()
{
}

void TcpClient::connect(
    const IpEndPoint& peerEndPoint,
    const TcpConnectHandler& connectHandler,
    uint32_t msecTimeout)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (!isClosed())
    {
        assert(false);
        return;
    }

    mConnectHandler = connectHandler;

    if (peerEndPoint.isUnspec())
    {
        onConnect(nullptr, -5101);
        return;
    }

    std::list<IpEndPoint> endPointList;
    endPointList.push_back(peerEndPoint);

    Network::getController()->requestTcpConnect(
        shared_from_this(), endPointList, msecTimeout);
}

void TcpClient::connect(
    const std::string& address, uint16_t port,
    const TcpConnectHandler& connectHandler,
    uint32_t msecTimeout)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (!isClosed())
    {
        assert(false);
        return;
    }

    mConnectHandler = connectHandler;

    IpEndPoint peerEndPoint(address, port);
    std::list<IpEndPoint> endPointList;

    if (peerEndPoint.isUnspec())
    {
        // resolve
        endPointList = IpEndPoint::resolveName(
            address, port,
            AddressFamily::Unspec,
            Socket::Type::Tcp);

        if (endPointList.empty())
        {
            onConnect(nullptr, -5110);
            return;
        }
    }
    else
    {
        endPointList.push_back(peerEndPoint);
    }

    Network::getController()->requestTcpConnect(
        shared_from_this(), endPointList, msecTimeout);
}

bool TcpClient::cancelConnect()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    mConnectHandler = nullptr;

    return Network::getController()->
        cancelTcpConnect(shared_from_this());
}

void TcpClient::onConnect(Socket* socket, int32_t errorCode)
{
    if (socket != nullptr)
    {
        mChannel->create(socket);

        if (!Network::getController()->registerTcpChannel(mChannel))
        {
            // error
            mChannel->create(nullptr);
            errorCode = -5120;
        }
    }

    if (mConnectHandler != nullptr)
    {
        TcpClientPtr self(shared_from_this());
        TcpConnectHandler handler = mConnectHandler;
        mConnectHandler = nullptr;

        Thread::getCurrent()->post([self, handler, errorCode]() {
            handler(self, errorCode);
        });
    }
}

SEV_NS_END

#endif // SUBEVENT_TCP_INL
