#ifndef SUBEVENT_UDP_HPP
#define SUBEVENT_UDP_HPP

#include <memory>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

class NetWorker;
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
    SEV_DECL static UdpReceiverPtr newInstance(NetWorker* netWorker)
    {
        return UdpReceiverPtr(new UdpReceiver(netWorker));
    }

    SEV_DECL ~UdpReceiver();

public:
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const UdpReceiveHandler& receiveHandler);

    SEV_DECL void close();

    SEV_DECL int32_t receive(void* buff, size_t size,
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
    SEV_DECL UdpReceiver(NetWorker* netWorker);

    SEV_DECL void onReceive();
    SEV_DECL void onClose();

    UdpReceiver() = delete;
    UdpReceiver(const UdpReceiver&) = delete;
    UdpReceiver& operator=(const UdpReceiver&) = delete;

    NetWorker* mNetWorker;

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
    SEV_DECL static UdpSenderPtr newInstance(NetWorker* netWorker)
    {
        return UdpSenderPtr(new UdpSender(netWorker));
    }

    SEV_DECL ~UdpSender();

public:
    SEV_DECL bool create(const IpEndPoint& receiverEndPoint);

    SEV_DECL void close();

    SEV_DECL int32_t send(const void* data, size_t size);

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
    SEV_DECL UdpSender(NetWorker* netWorker);

    UdpSender() = delete;
    UdpSender(const UdpSender&) = delete;
    UdpSender& operator=(const UdpSender&) = delete;

    NetWorker* mNetWorker;

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mReceiverEndPoint;
};

SEV_NS_END

#endif // SUBEVENT_UDP_HPP
