#ifndef SUBEVENT_UDP_HPP
#define SUBEVENT_UDP_HPP

#include <functional>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

class UdpReceiver;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::function<void(UdpReceiver*)> UdpReceiveHandler;

//----------------------------------------------------------------------------//
// UdpReceiver
//----------------------------------------------------------------------------//

class UdpReceiver
{
public:
    SEV_DECL UdpReceiver();
    SEV_DECL ~UdpReceiver();

public:
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const UdpReceiveHandler& receiveHandler);

    SEV_DECL void close();

    SEV_DECL int32_t receive(void* buff, uint32_t size,
        IpEndPoint& senderEndPoint);

    SEV_DECL SocketOption& getSocketOption();

    SEV_DECL bool isClosed() const
    {
        return (mSocket == nullptr);
    }

    SEV_DECL const IpEndPoint& getLocalEndPoint() const
    {
        return mLocalEndPoint;
    }

private:
    UdpReceiver(const UdpReceiver&) = delete;
    UdpReceiver& operator=(const UdpReceiver&) = delete;

    SEV_DECL void onReceive();
    SEV_DECL void onClose();

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mLocalEndPoint;

    UdpReceiveHandler mReceiveHandler;

    friend class SocketController;
};

//----------------------------------------------------------------------------//
// UdpSender
//----------------------------------------------------------------------------//

class UdpSender
{
public:
    SEV_DECL UdpSender();
    SEV_DECL ~UdpSender();

public:
    SEV_DECL bool create(const IpEndPoint& receiverEndPoint);

    SEV_DECL void close();

    SEV_DECL int32_t send(const void* data, uint32_t size);

    SEV_DECL SocketOption& getSocketOption();

    SEV_DECL bool isClosed() const
    {
        return (mSocket == nullptr);
    }

    SEV_DECL const IpEndPoint& getReceiverEndPoint() const
    {
        return mReceiverEndPoint;
    }

private:
    UdpSender(const UdpSender&) = delete;
    UdpSender& operator=(const UdpSender&) = delete;

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mReceiverEndPoint;
};

SEV_NS_END

#endif // SUBEVENT_UDP_HPP
