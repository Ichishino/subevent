#ifndef SUBEVENT_TCP_INL
#define SUBEVENT_TCP_INL

#include <cassert>

#include <subevent/network.hpp>
#include <subevent/tcp.hpp>
#include <subevent/thread.hpp>
#include <subevent/socket_controller.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// TcpServer
//----------------------------------------------------------------------------//

TcpServer::TcpServer(NetWorker* netWorker)
{
    assert(netWorker != nullptr);
    assert(netWorker->getSocketController() != nullptr);

    mNetWorker = netWorker;
    mSocket = nullptr;
}

TcpServer::~TcpServer()
{
    delete mSocket;
}

Socket* TcpServer::createSocket(
    const IpEndPoint& localEndPoint, int32_t& errorCode)
{
    Socket* socket = new Socket();

    // create
    if (!socket->create(
        localEndPoint.getFamily(),
        Socket::Type::Tcp,
        Socket::Protocol::Tcp))
    {
        errorCode = socket->getErrorCode();
        delete socket;
        return nullptr;
    }

    // option
    socket->setOption(getSocketOption());

    errorCode = 0;

    return socket;
}

bool TcpServer::open(
    const IpEndPoint& localEndPoint,
    const TcpAcceptHandler& acceptHandler,
    int32_t listenBacklog)
{
    assert(NetWorker::getCurrent() != nullptr);

    if (!isClosed())
    {
        return false;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return false;
    }

    int32_t errorCode = 0;

    // create
    Socket* socket =
        createSocket(localEndPoint, errorCode);
    if (socket == nullptr)
    {
        assert(false);
        return false;
    }

    // bind
    if (!socket->bind(localEndPoint))
    {
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

    return mNetWorker->getSocketController()->
        registerTcpServer(shared_from_this());
}

void TcpServer::close()
{
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        return;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return;
    }

    mNetWorker->getSocketController()->
        unregisterTcpServer(shared_from_this());

    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mLocalEndPoint.clear();
    mAcceptHandler = nullptr;
}

bool TcpServer::accept(
    NetWorker* netWorker, const TcpChannelPtr& channel)
{
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        return false;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return false;
    }

    channel->mNetWorker = netWorker;

    if (netWorker == NetWorker::getCurrent())
    {
        if (!mNetWorker->getSocketController()->
            registerTcpChannel(channel))
        {
            return false;
        }
    }
    else
    {
        if (!netWorker->postEvent(new TcpAcceptEvent(channel)))
        {
            channel->mNetWorker = NetWorker::getCurrent();
            return false;
        }
    }

    return true;
}

TcpChannelPtr TcpServer::accept(const Event* event)
{
    assert(NetWorker::getCurrent() != nullptr);

    const TcpAcceptEvent* acceptEvent =
        dynamic_cast<const TcpAcceptEvent*>(event);

    TcpChannelPtr channel;
    acceptEvent->getParams(channel);

    if (channel->mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return nullptr;
    }

    if (!channel->mNetWorker->
        getSocketController()->registerTcpChannel(channel))
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

TcpChannelPtr TcpServer::createChannel(Socket* socket)
{
    return std::make_shared<TcpChannel>(socket);
}

void TcpServer::onAccept()
{
    if (mAcceptHandler == nullptr)
    {
        return;
    }

    std::list<TcpChannelPtr> channels;

    for (;;)
    {
        // accept
        Socket* socket = mSocket->accept();

        if (socket == nullptr)
        {
            break;
        }

        if (!socket->onAccept())
        {
            socket->close();
            delete socket;
            break;
        }

        channels.push_back(
            createChannel(socket));
    }

    if (channels.empty())
    {
        return;
    }

    TcpServerPtr self(shared_from_this());
    TcpAcceptHandler handler = mAcceptHandler;

    mNetWorker->postTask([self, handler, channels]() {

        for (auto& channel : channels)
        {
            handler(self, channel);
        }
    });
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

TcpChannel::TcpChannel(NetWorker* netWorker)
{
    assert(netWorker != nullptr);
    assert(netWorker->getSocketController() != nullptr);

    mNetWorker = netWorker;
    mSocket = nullptr;
}

TcpChannel::TcpChannel(Socket* socket)
{
    mNetWorker = nullptr;
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
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        if (mCloseCanceller != nullptr)
        {
            mCloseCanceller->cancel();
        }

        return;
    }

    if ((mNetWorker != nullptr) &&
        (mNetWorker != NetWorker::getCurrent()))
    {
        assert(false);
        return;
    }

    mSockOption.clear();
    mReceiveHandler = nullptr;
    mCloseHandler = nullptr;
    mCloseCanceller.reset();
    mSendHandlers.clear();

    if (mNetWorker != nullptr)
    {
        mNetWorker->getSocketController()->
            requestTcpChannelClose(shared_from_this());
    }

    delete mSocket;
    mSocket = nullptr;
}

int32_t TcpChannel::send(
    const void* data, size_t size,
    const TcpSendHandler& sendHandler)
{
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return -5200;
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

        const char* bytes =
            reinterpret_cast<const char*>(data);

        return send(
            std::vector<char>(bytes, bytes + size),
            sendHandler);
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
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return -5250;
    }

    if (sendHandler == nullptr)
    {
        return mSocket->send(
            &data[0], static_cast<int32_t>(data.size()),
            Socket::SendFlags);
    }
    else
    {
        mSendHandlers.push_back(sendHandler);
    }

    if (!mNetWorker->getSocketController()->
        requestTcpSend(
            shared_from_this(),
            std::forward<std::vector<char>>(data)))
    {
        return -1;
    }

    return 0;
}

int32_t TcpChannel::receive(void* buff, size_t size)
{
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return -5300;
    }

    if (size > INT32_MAX)
    {
        return -5301;
    }

    int32_t result = mSocket->receive(
        buff, static_cast<int32_t>(size));

    if ((result == 0) && (size > 0))
    {
        mNetWorker->getSocketController()->
            onTcpReceiveEof(shared_from_this());
    }

    return result;
}

std::vector<char> TcpChannel::receiveAll(size_t reserveSize)
{
    assert(NetWorker::getCurrent() != nullptr);

    if (reserveSize > INT32_MAX)
    {
        reserveSize = INT32_MAX;
    }

    size_t total = 0;
    std::vector<char> buff;

    try
    {
        buff.resize(reserveSize);

        for (;;)
        {
            // receive
            int32_t size = receive(&buff[total], reserveSize);

            if (size > 0)
            {
                total += size;
            }
            else
            {
                buff.resize(total);
                break;
            }

            buff.resize(total + reserveSize);
        }
    }
    catch (...)
    {
        buff.clear();
        close();
    }

    return buff;
}

bool TcpChannel::cancelSend()
{
    assert(NetWorker::getCurrent() != nullptr);

    if (isClosed())
    {
        return false;
    }

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return false;
    }

    return mNetWorker->getSocketController()->
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
    if (mReceiveHandler == nullptr)
    {
        return;
    }

    TcpChannelPtr self(shared_from_this());
    TcpReceiveHandler handler = mReceiveHandler;

    mNetWorker->postTask([self, handler]() {

        handler(self);

        if (self->isClosed())
        {
            return;
        }

        char eof[1];
        int32_t result =
            self->mSocket->receive(eof, sizeof(eof), MSG_PEEK);
        if (result == 0)
        {
            self->mNetWorker->getSocketController()->
                onTcpReceiveEof(self);
        }
    });
}

void TcpChannel::onSend(int32_t errorCode)
{
    if (mSendHandlers.empty())
    {
        return;
    }

    TcpChannelPtr self(shared_from_this());
    TcpSendHandler handler = mSendHandlers.front();
    mSendHandlers.pop_front();

    mNetWorker->postTask(
        [self, handler, errorCode]() {
            handler(self, errorCode);
        });
}

void TcpChannel::onClose()
{
    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mReceiveHandler = nullptr;
    mSendHandlers.clear();

    if (mCloseHandler == nullptr)
    {
        return;
    }

    TcpChannelPtr self(shared_from_this());
    TcpCloseHandler handler = mCloseHandler;
    mCloseHandler = nullptr;

    mCloseCanceller = postCancelableTask(
        dynamic_cast<Thread*>(mNetWorker),
        [self, handler]() {
            handler(self);
        });
}

//----------------------------------------------------------------------------//
// TcpClient
//----------------------------------------------------------------------------//

TcpClient::TcpClient(NetWorker* netWorker)
    : TcpChannel(netWorker)
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
    assert(NetWorker::getCurrent() != nullptr);

    if (!isClosed())
    {
        assert(false);
        return;
    }

    if (mNetWorker != NetWorker::getCurrent())
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

    TcpClientPtr self(
        std::dynamic_pointer_cast<TcpClient>(shared_from_this()));

    std::list<IpEndPoint> endPointList;
    endPointList.push_back(peerEndPoint);

    mNetWorker->getSocketController()->
        requestTcpConnect(self, endPointList, msecTimeout);
}

void TcpClient::connect(
    const std::string& address, uint16_t port,
    const TcpConnectHandler& connectHandler,
    uint32_t msecTimeout)
{
    assert(NetWorker::getCurrent() != nullptr);

    if (!isClosed())
    {
        assert(false);
        return;
    }

    if (mNetWorker != NetWorker::getCurrent())
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

    TcpClientPtr self(
        std::dynamic_pointer_cast<TcpClient>(shared_from_this()));

    mNetWorker->getSocketController()->
        requestTcpConnect(self, endPointList, msecTimeout);
}

bool TcpClient::cancelConnect()
{
    assert(NetWorker::getCurrent() != nullptr);

    if (mNetWorker != NetWorker::getCurrent())
    {
        assert(false);
        return false;
    }

    mConnectHandler = nullptr;

    TcpClientPtr self(
        std::dynamic_pointer_cast<TcpClient>(shared_from_this()));

    return mNetWorker->getSocketController()->
        cancelTcpConnect(self);
}

Socket* TcpClient::createSocket(
    const IpEndPoint& peerEndPoint, int32_t& errorCode)
{
    Socket* socket = new Socket();

    // create
    if (!socket->create(
        peerEndPoint.getFamily(),
        Socket::Type::Tcp,
        Socket::Protocol::Tcp))
    {
        errorCode = socket->getErrorCode();
        delete socket;
        return nullptr;
    }

    // option
    socket->setOption(getSocketOption());

    errorCode = 0;

    return socket;
}

void TcpClient::onConnect(Socket* socket, int32_t errorCode)
{
    TcpClientPtr self(
        std::dynamic_pointer_cast<TcpClient>(shared_from_this()));

    if (socket != nullptr)
    {
        create(socket);

        if (!mSocket->onConnect())
        {
            // error
            create(nullptr);
            errorCode = -5121;
        }
        else if (
            !mNetWorker->getSocketController()->
            registerTcpChannel(self))
        {
            // error
            create(nullptr);
            errorCode = -5122;
        }
    }

    if (mConnectHandler == nullptr)
    {
        return;
    }

    TcpConnectHandler handler = mConnectHandler;
    mConnectHandler = nullptr;

    mNetWorker->postTask(
        [self, handler, errorCode]() {
            handler(self, errorCode);
        });
}

SEV_NS_END

#endif // SUBEVENT_TCP_INL
