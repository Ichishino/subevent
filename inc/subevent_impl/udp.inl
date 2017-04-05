#ifndef SUBEVENT_UDP_INL
#define SUBEVENT_UDP_INL

#include <cassert>

#include <subevent/udp.hpp>
#include <subevent/thread.hpp>
#include <subevent/socket.hpp>
#include <subevent/socket_controller.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// UdpReceiver
//----------------------------------------------------------------------------//

UdpReceiver::UdpReceiver()
{
    mSocket = nullptr;
}

UdpReceiver::~UdpReceiver()
{
    delete mSocket;
}

bool UdpReceiver::open(
    const IpEndPoint& localEndPoint,
    const UdpReceiveHandler& receiveHandler)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (!isClosed())
    {
        return false;
    }

    if (localEndPoint.isUnspec())
    {
        return false;
    }

    Socket* socket = new Socket();

    // create
    if (!socket->create(
        localEndPoint.getFamily(),
        Socket::Type::Udp,
        Socket::Protocol::Udp))
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

    mSocket = socket;
    mLocalEndPoint = localEndPoint;
    mReceiveHandler = receiveHandler;

    return SocketController::getInstance()->
        registerUdpReceiver(shared_from_this());
}

void UdpReceiver::close()
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return;
    }

    SocketController::getInstance()->
        unregisterUdpReceiver(shared_from_this());

    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mLocalEndPoint.clear();
    mReceiveHandler = nullptr;
}

SocketOption& UdpReceiver::getSocketOption()
{
    if (!isClosed())
    {
        mSockOption = mSocket->getOption();
    }

    return mSockOption;
}

void UdpReceiver::onReceive()
{
    if (mReceiveHandler != nullptr)
    {
        UdpReceiverPtr self(shared_from_this());
        UdpReceiveHandler handler = mReceiveHandler;

        Thread::getCurrent()->post([self, handler]() {
            handler(self);
        });
    }
}

int32_t UdpReceiver::receive(
    void* buff, size_t size, IpEndPoint& senderEndPoint)
{
    assert(Thread::getCurrent() != nullptr);
    assert(SocketController::getInstance() != nullptr);

    if (isClosed())
    {
        return -1;
    }

    if (size > INT32_MAX)
    {
        return -6301;
    }

    // receive
    return mSocket->receiveFrom(
        senderEndPoint, buff, static_cast<int32_t>(size));
}

void UdpReceiver::onClose()
{
    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mReceiveHandler = nullptr;
}

//----------------------------------------------------------------------------//
// UdpSender
//----------------------------------------------------------------------------//

UdpSender::UdpSender()
{
    mSocket = nullptr;
}

UdpSender::~UdpSender()
{
    delete mSocket;
}

bool UdpSender::create(const IpEndPoint& receiverEndPoint)
{
    if (receiverEndPoint.isUnspec())
    {
        return false;
    }

    Socket* socket = new Socket();

    // create
    if (!socket->create(
        receiverEndPoint.getFamily(),
        Socket::Type::Udp,
        Socket::Protocol::Udp))
    {
        assert(false);
        delete socket;

        return false;
    }

    // option
    socket->setOption(mSockOption);

    mSocket = socket;
    mReceiverEndPoint = receiverEndPoint;

    return true;
}

int32_t UdpSender::send(const void* data, size_t size)
{
    if (isClosed())
    {
        return -1;
    }

    if (size > INT32_MAX)
    {
        return -6201;
    }

    // send (need async?)
    return mSocket->sendTo(
        mReceiverEndPoint, data, static_cast<int32_t>(size));
}

void UdpSender::close()
{
    if (isClosed())
    {
        return;
    }

    delete mSocket;
    mSocket = nullptr;

    mSockOption.clear();
    mReceiverEndPoint.clear();
}

SocketOption& UdpSender::getSocketOption()
{
    if (!isClosed())
    {
        mSockOption = mSocket->getOption();
    }

    return mSockOption;
}

SEV_NS_END

#endif // SUBEVENT_UDP_INL
