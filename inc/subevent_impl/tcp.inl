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
    const SocketOption& option,
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
    socket->setOption(option);

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

    return Network::getController()->registerTcpServer(this);
}

void TcpServer::close()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return;
    }

    Network::getController()->unregisterTcpServer(this);

    delete mSocket;
    mSocket = nullptr;

    mLocalEndPoint.clear();
    mAcceptHandler = nullptr;
}

bool TcpServer::accept(Thread* thread, TcpChannel* channel)
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

TcpChannel* TcpServer::accept(const Event* event)
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    const TcpAcceptEvent* acceptEvent =
        dynamic_cast<const TcpAcceptEvent*>(event);

    TcpChannel* channel = acceptEvent->mTcpChannel;
    acceptEvent->mTcpChannel = nullptr;

    if (!Network::getController()->registerTcpChannel(channel))
    {
        delete channel;
        return nullptr;
    }

    return channel;
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

        TcpChannel* channel = new TcpChannel(socket);

        TcpAcceptHandler handler = mAcceptHandler;

        bool result = Thread::getCurrent()->post(
            [this, handler, channel]() {
                handler(this, channel);
            });

        if (result == false)
        {
            delete channel;
        }
    }
}

void TcpServer::onClose()
{
    delete mSocket;
    mSocket = nullptr;

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
    mSocket = socket;
    mSocket->getLocalEndPoint(mLocalEndPoint);
    mSocket->getPeerEndPoint(mPeerEndPoint);
}

void TcpChannel::close()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    if (isClosed())
    {
        return;
    }

    mReceiveHandler = nullptr;
    mCloseHandler = nullptr;
    mSendHandlers.clear();

    Network::getController()->requestTcpChannelClose(this);
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
        return mSocket->send(data, size, Socket::SendFlags);
    }

    mSendHandlers.push_back(sendHandler);

    if (!Network::getController()->requestTcpSend(this, data, size))
    {
        return -1;
    }

    return 0;
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
        Network::getController()->onTcpReceiveEof(this);
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

    return Network::getController()->cancelTcpSend(this);
}

void TcpChannel::setReceiveHandler(const TcpReceiveHandler& receiveHandler)
{
    mReceiveHandler = receiveHandler;
}

void TcpChannel::setCloseHandler(const TcpCloseHandler& closeHandler)
{
    mCloseHandler = closeHandler;
}

void TcpChannel::setSocketOption(const SocketOption& option)
{
    if (isClosed())
    {
        return;
    }

    mSocket->setOption(option);
}

void TcpChannel::onReceive()
{
    if (mReceiveHandler != nullptr)
    {
        TcpReceiveHandler handler = mReceiveHandler;

        Thread::getCurrent()->post([this, handler]() {
            handler(this);
        });
    }
}

void TcpChannel::onSend(int32_t errorCode)
{
    if (!mSendHandlers.empty())
    {
        TcpSendHandler handler = mSendHandlers.front();
        mSendHandlers.pop_front();

        Thread::getCurrent()->post([this, handler, errorCode]() {
            handler(this, errorCode);
        });
    }
}

void TcpChannel::onClose()
{
    delete mSocket;
    mSocket = nullptr;

    mReceiveHandler = nullptr;
    mSendHandlers.clear();

    if (mCloseHandler != nullptr)
    {
        TcpCloseHandler handler = mCloseHandler;
        mCloseHandler = nullptr;

        Thread::getCurrent()->post([this, handler]() {
            handler(this);
        });
    }
}

//----------------------------------------------------------------------------//
// TcpClient
//----------------------------------------------------------------------------//

TcpClient::TcpClient()
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
        onConnect(nullptr, -101);
        return;
    }

    std::list<IpEndPoint> endPointList;
    endPointList.push_back(peerEndPoint);

    Network::getController()->requestTcpConnect(
        this, endPointList, msecTimeout);
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
            onConnect(nullptr, -110);
            return;
        }
    }
    else
    {
        endPointList.push_back(peerEndPoint);
    }

    Network::getController()->requestTcpConnect(
        this, endPointList, msecTimeout);
}

bool TcpClient::cancelConnect()
{
    assert(Thread::getCurrent() != nullptr);
    assert(Network::getController() != nullptr);

    mConnectHandler = nullptr;

    return Network::getController()->cancelTcpConnect(this);
}

void TcpClient::onConnect(Socket* socket, int32_t errorCode)
{
    if (socket != nullptr)
    {
        create(socket);
        Network::getController()->registerTcpChannel(this);
    }

    if (mConnectHandler != nullptr)
    {
        TcpConnectHandler handler = mConnectHandler;
        mConnectHandler = nullptr;

        Thread::getCurrent()->post([this, handler, errorCode]() {
            handler(this, errorCode);
        });
    }
}

SEV_NS_END

#endif // SUBEVENT_TCP_INL
