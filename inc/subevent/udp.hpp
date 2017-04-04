#ifndef SUBEVENT_UDP_HPP
#define SUBEVENT_UDP_HPP

#include <memory>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

class UdpReceiver;
class UdpSender;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::shared_ptr<UdpReceiver> UdpReceiverPtr;
typedef std::shared_ptr<UdpSender> UdpSenderPtr;

typedef std::function<void(const UdpReceiverPtr&)> UdpReceiveHandler;

//----------------------------------------------------------------------------//
// UdpReceiver
//----------------------------------------------------------------------------//

class UdpReceiver : public std::enable_shared_from_this<UdpReceiver>
{
public:
    SEV_DECL static UdpReceiverPtr newInstance()
    {
        return UdpReceiverPtr(new UdpReceiver());
    }

    SEV_DECL ~UdpReceiver();

public:
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const UdpReceiveHandler& receiveHandler);

    SEV_DECL void close();

    SEV_DECL int32_t receive(void* buff, uint32_t size,
        IpEndPoint& senderEndPoint);
    SEV_DECL std::vector<char> receiveAll(
        IpEndPoint& senderEndPoint, uint32_t reserveSize = 256);

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
    SEV_DECL UdpReceiver();

    SEV_DECL void onReceive();
    SEV_DECL void onClose();

    UdpReceiver(const UdpReceiver&) = delete;
    UdpReceiver& operator=(const UdpReceiver&) = delete;

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mLocalEndPoint;

    UdpReceiveHandler mReceiveHandler;

    friend class SocketController;
};

//----------------------------------------------------------------------------//
// UdpSender
//----------------------------------------------------------------------------//

class UdpSender : public std::enable_shared_from_this<UdpSender>
{
public:
    SEV_DECL static UdpSenderPtr newInstance()
    {
        return UdpSenderPtr(new UdpSender());
    }

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
    SEV_DECL UdpSender();

    UdpSender(const UdpSender&) = delete;
    UdpSender& operator=(const UdpSender&) = delete;

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mReceiverEndPoint;
};

SEV_NS_END

#endif // SUBEVENT_UDP_HPP
