#ifndef SUBEVENT_TCP_INL
#define SUBEVENT_TCP_INL

#include <cassert>
#include <cstring>

#include <subevent/tcp.hpp>
#include <subevent/thread.hpp>
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
    assert(SocketController::getInstance() != nullptr);

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

    return SocketController::getInstance()->
        registerTcpServer(shared_from_this());
}

void TcpServer::close()
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return;
    }

    SocketController::getInstance()->
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
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return false;
    }

    if (thread == Thread::getCurrent())
    {
        if (!SocketController::getInstance()->
            registerTcpChannel(channel))
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
    assert(SocketController::getInstance() != nullptr);

    const TcpAcceptEvent* acceptEvent =
        dynamic_cast<const TcpAcceptEvent*>(event);

    TcpChannelPtr channel;
    acceptEvent->getParams(channel);

    if (!SocketController::getInstance()->
        registerTcpChannel(channel))
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
        for (;;)
        {
            // accept
            Socket* socket = mSocket->accept();

            if (socket == nullptr)
            {
                break;
            }

            TcpServerPtr self(shared_from_this());
            TcpChannelPtr channel(new TcpChannel(socket));
            TcpAcceptHandler handler = mAcceptHandler;

            Thread::getCurrent()->post([self, channel, handler]() {
                handler(self, channel);
            });
        }
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
    assert(SocketController::getInstance() != nullptr);

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

    SocketController::getInstance()->
        requestTcpChannelClose(shared_from_this());

    delete mSocket;
    mSocket = nullptr;
}

int32_t TcpChannel::send(
    const void* data, size_t size,
    const TcpSendHandler& sendHandler)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (size > INT32_MAX)
    {
        return -5201;
    }

    if (sendHandler == nullptr)
    {
        // sync

        return mSocket->send(
            data, static_cast<int32_t>(size), Socket::SendFlags);
    }
    else
    {
        // async

        std::vector<char> buff;
        buff.resize(size);
        memcpy(&buff[0], data, size);

        return send(std::move(buff), sendHandler);
    }
}

int32_t TcpChannel::sendString(
    const std::string& data,
    const TcpSendHandler& sendHandler)
{
    return send(
        data.c_str(),
        sizeof(std::string::value_type) * (data.size() + 1),
        sendHandler);
}

int32_t TcpChannel::send(
    std::vector<char>&& data,
    const TcpSendHandler& sendHandler)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    mSendHandlers.push_back(sendHandler);

    if (!SocketController::getInstance()->requestTcpSend(
        shared_from_this(), std::forward<std::vector<char>>(data)))
    {
        return -1;
    }

    return 0;
}

int32_t TcpChannel::receive(void* buff, size_t size)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (size > INT32_MAX)
    {
        return -5301;
    }

    int32_t result = mSocket->receive(buff, static_cast<int32_t>(size));
    if ((result == 0) && (size > 0))
    {
        SocketController::getInstance()->
            onTcpReceiveEof(shared_from_this());
    }

    return result;
}

std::vector<unsigned char> TcpChannel::receiveAll(size_t reserveSize)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (reserveSize > INT32_MAX)
    {
        reserveSize = INT32_MAX;
    }

    size_t total = 0;
    std::vector<unsigned char> buff;

    buff.resize(reserveSize);

    for (;;)
    {
        // receive
        int32_t size = receive(&buff[total], reserveSize);

        if (size > 0)
        {
            total += size;
        }

        if ((size <= 0) ||
            (static_cast<size_t>(size) < reserveSize))
        {
            buff.resize(total);
            break;
        }

        buff.resize(total + reserveSize);
    }

    return buff;
}

bool TcpChannel::cancelSend()
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return false;
    }

    return SocketController::getInstance()->
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

        Thread::getCurrent()->post([=]() {
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
    assert(SocketController::getInstance() != nullptr);

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

    SocketController::getInstance()->
        requestTcpConnect(
            shared_from_this(), endPointList, msecTimeout);
}

void TcpClient::connect(
    const std::string& address, uint16_t port,
    const TcpConnectHandler& connectHandler,
    uint32_t msecTimeout)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

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

    SocketController::getInstance()->
        requestTcpConnect(
            shared_from_this(), endPointList, msecTimeout);
}

bool TcpClient::cancelConnect()
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    mConnectHandler = nullptr;

    return SocketController::getInstance()->
        cancelTcpConnect(shared_from_this());
}

void TcpClient::onConnect(Socket* socket, int32_t errorCode)
{
    if (socket != nullptr)
    {
        mChannel->create(socket);

        if (!SocketController::getInstance()->
            registerTcpChannel(mChannel))
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
