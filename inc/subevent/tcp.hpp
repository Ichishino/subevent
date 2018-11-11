#ifndef SUBEVENT_TCP_HPP
#define SUBEVENT_TCP_HPP

#include <list>
#include <vector>
#include <memory>
#include <utility>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/common.hpp>
#include <subevent/event.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

class NetWorker;
class TcpServer;
class TcpClient;
class TcpChannel;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::shared_ptr<TcpServer> TcpServerPtr;
typedef std::shared_ptr<TcpChannel> TcpChannelPtr;
typedef std::shared_ptr<TcpClient> TcpClientPtr;

typedef std::function<
    void(const TcpServerPtr&, const TcpChannelPtr&)> TcpAcceptHandler;
typedef std::function<void(const TcpClientPtr&, int32_t)> TcpConnectHandler;
typedef std::function<void(const TcpChannelPtr&)> TcpReceiveHandler;
typedef std::function<void(const TcpChannelPtr&, int32_t)> TcpSendHandler;
typedef std::function<void(const TcpChannelPtr&)> TcpCloseHandler;

namespace TcpEventId
{
    static const Event::Id Accept = 0xFB000001;
}

typedef UserEvent<TcpEventId::Accept, TcpChannelPtr> TcpAcceptEvent;

//----------------------------------------------------------------------------//
// TcpServer
//----------------------------------------------------------------------------//

class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
    SEV_DECL static TcpServerPtr newInstance(NetWorker* netWorker)
    {
        return TcpServerPtr(new TcpServer(netWorker));
    }

    SEV_DECL virtual ~TcpServer();

public:
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const TcpAcceptHandler& acceptHandler,
        int32_t listenBacklog = SOMAXCONN);

    SEV_DECL void close();

    SEV_DECL bool accept(
        NetWorker* netWorker, const TcpChannelPtr& channel);

    SEV_DECL static TcpChannelPtr accept(
        const Event* delegateAcceptEvent);

    SEV_DECL SocketOption& getSocketOption();

    SEV_DECL bool isClosed() const
    {
        return (mSocket == nullptr);
    }

    SEV_DECL const IpEndPoint& getLocalEndPoint() const
    {
        return mLocalEndPoint;
    }

protected:
    SEV_DECL TcpServer(NetWorker* netWorker);

    NetWorker* mNetWorker;

private:
    SEV_DECL virtual Socket* createSocket(
        const IpEndPoint& localEndPoint, int32_t& errorCode);
    SEV_DECL virtual TcpChannelPtr createChannel(Socket* socket);

    SEV_DECL void onAccept();
    SEV_DECL void onClose();

    TcpServer() = delete;
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mLocalEndPoint;

    TcpAcceptHandler mAcceptHandler;

    friend class SocketController;
};

//----------------------------------------------------------------------------//
// TcpChannel
//----------------------------------------------------------------------------//

class TcpChannel : public std::enable_shared_from_this<TcpChannel>
{
public:
    SEV_DECL virtual ~TcpChannel();

public:
    SEV_DECL int32_t send(const void* data, size_t size,
        const TcpSendHandler& sendHandler = nullptr);
    SEV_DECL int32_t send(std::vector<char>&& data,
        const TcpSendHandler& sendHandler = nullptr);
    SEV_DECL int32_t sendString(const std::string& data,
        const TcpSendHandler& sendHandler = nullptr);

    SEV_DECL int32_t receive(void* buff, size_t size);
    SEV_DECL std::vector<char> receiveAll(size_t reserveSize = 8192);

    SEV_DECL void close();

    SEV_DECL bool cancelSend();

    SEV_DECL void setReceiveHandler(
        const TcpReceiveHandler& receiveHandler);
    SEV_DECL void setCloseHandler(
        const TcpCloseHandler& closeHandler);

    SEV_DECL SocketOption& getSocketOption();

    SEV_DECL bool isClosed() const
    {
        return (mSocket == nullptr);
    }

    SEV_DECL const IpEndPoint& getLocalEndPoint() const
    {
        return mLocalEndPoint;
    }

    SEV_DECL const IpEndPoint& getPeerEndPoint() const
    {
        return mPeerEndPoint;
    }

public:
    SEV_DECL TcpChannel(Socket* socket);

    SEV_DECL const TcpReceiveHandler& getReceiveHandler() const
    {
        return mReceiveHandler;
    }

    SEV_DECL const TcpCloseHandler& getCloseHandler() const
    {
        return mCloseHandler;
    }

protected:
    SEV_DECL TcpChannel(NetWorker* netWorker);

    NetWorker* mNetWorker;

private:
    SEV_DECL void create(Socket* socket);
    SEV_DECL void onReceive();
    SEV_DECL void onSend(int32_t errorCode);
    SEV_DECL void onClose();

    TcpChannel(const TcpChannel&) = delete;
    TcpChannel& operator=(const TcpChannel&) = delete;

    Socket* mSocket;
    SocketOption mSockOption;
    IpEndPoint mLocalEndPoint;
    IpEndPoint mPeerEndPoint;

    TcpCloseHandler mCloseHandler;
    TaskCancellerPtr mCloseCanceller;
    TcpReceiveHandler mReceiveHandler;
    std::list<TcpSendHandler> mSendHandlers;

    friend class TcpServer;
    friend class TcpClient;
    friend class SocketController;
};

//----------------------------------------------------------------------------//
// TcpClient
//----------------------------------------------------------------------------//

class TcpClient : public TcpChannel
{
public:
    SEV_DECL static TcpClientPtr newInstance(NetWorker* netWorker)
    {
        return TcpClientPtr(new TcpClient(netWorker));
    }

    SEV_DECL ~TcpClient() override;

    static const uint32_t DefaultTimeout = 15 * 1000;

public:
    SEV_DECL void connect(
        const IpEndPoint& peerEndPoint,
        const TcpConnectHandler& connectHandler,
        uint32_t msecTimeout = DefaultTimeout);

    SEV_DECL void connect(
        const std::string& address, uint16_t port,
        const TcpConnectHandler& connectHandler,
        uint32_t msecTimeout = DefaultTimeout);

    SEV_DECL bool cancelConnect();

protected:
    SEV_DECL TcpClient(NetWorker* netWorker);

private:
    SEV_DECL virtual Socket* createSocket(
        const IpEndPoint& peerEndPoint, int32_t& errorCode);

    SEV_DECL void onConnect(Socket* socket, int32_t errorCode);

    TcpClient() = delete;
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    TcpConnectHandler mConnectHandler;

    friend class SocketController;
};

SEV_NS_END

#endif // SUBEVENT_TCP_HPP
