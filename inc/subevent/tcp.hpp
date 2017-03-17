#ifndef SUBEVENT_TCP_HPP
#define SUBEVENT_TCP_HPP

#include <list>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/event.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

class Thread;
class TcpServer;
class TcpClient;
class TcpChannel;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

namespace TcpEventId
{
    static const Event::Id Accept = 80001;
}

typedef std::function<void(TcpServer*, TcpChannel*)> TcpAcceptHandler;
typedef std::function<void(TcpClient*, int32_t)> TcpConnectHandler;
typedef std::function<void(TcpChannel*)> TcpReceiveHandler;
typedef std::function<void(TcpChannel*, int32_t)> TcpSendHandler;
typedef std::function<void(TcpChannel*)> TcpCloseHandler;

//----------------------------------------------------------------------------//
// TcpServer
//----------------------------------------------------------------------------//

class TcpServer
{
public:
    SEV_DECL TcpServer();
    SEV_DECL ~TcpServer();

public:
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const TcpAcceptHandler& acceptHandler,
        const SocketOption& option = SocketOption(),
        int32_t listenBacklog = 128);

    SEV_DECL void close();

    SEV_DECL bool accept(
        Thread* thread, TcpChannel* channel);

    SEV_DECL static TcpChannel* accept(
        const Event* delegateAcceptEvent);

    SEV_DECL bool isClosed() const
    {
        return (mSocket == nullptr);
    }

    SEV_DECL const IpEndPoint& getLocalEndPoint() const
    {
        return mLocalEndPoint;
    }

    SEV_DECL const Socket* getSocket() const
    {
        return mSocket;
    }

private:
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    SEV_DECL void onAccept();
    SEV_DECL void onClose();

    Socket* mSocket;
    IpEndPoint mLocalEndPoint;

    TcpAcceptHandler mAcceptHandler;

    friend class SocketController;
};

//----------------------------------------------------------------------------//
// TcpChannel
//----------------------------------------------------------------------------//

class TcpChannel
{
public:
    SEV_DECL TcpChannel();
    SEV_DECL TcpChannel(Socket* socket);
    SEV_DECL virtual ~TcpChannel();

public:
    SEV_DECL int32_t send(const void* data, uint32_t size,
        const TcpSendHandler& sendHandler = nullptr);

    SEV_DECL int32_t receive(void* buff, uint32_t size);

    SEV_DECL void close();

    SEV_DECL bool cancelSend();

    SEV_DECL void setReceiveHandler(
        const TcpReceiveHandler& receiveHandler);
    SEV_DECL void setCloseHandler(
        const TcpCloseHandler& closeHandler);

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

    SEV_DECL void setSocketOption(const SocketOption& option);

    SEV_DECL const Socket* getSocket() const
    {
        return mSocket;
    }

protected:
    SEV_DECL void create(Socket* socket);

private:
    TcpChannel(const TcpChannel&) = delete;
    TcpChannel& operator=(const TcpChannel&) = delete;

    SEV_DECL void onReceive();
    SEV_DECL void onSend(int32_t errorCode);
    SEV_DECL void onClose();

    Socket* mSocket;
    IpEndPoint mLocalEndPoint;
    IpEndPoint mPeerEndPoint;

    TcpCloseHandler mCloseHandler;
    TcpReceiveHandler mReceiveHandler;
    std::list<TcpSendHandler> mSendHandlers;

    friend class SocketController;
};

//----------------------------------------------------------------------------//
// TcpClient
//----------------------------------------------------------------------------//

class TcpClient : public TcpChannel
{
public:
    SEV_DECL TcpClient();
    SEV_DECL ~TcpClient();

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

private:
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    SEV_DECL void onConnect(Socket* socket, int32_t errorCode);

    TcpConnectHandler mConnectHandler;

    friend class SocketController;
};

//----------------------------------------------------------------------------//
// TcpAcceptEvent
//----------------------------------------------------------------------------//

class TcpAcceptEvent : public Event
{
public:
    SEV_DECL TcpAcceptEvent(TcpChannel* tcpChannel)
        : Event(TcpEventId::Accept),
        mTcpChannel(tcpChannel)
    {
    }

    SEV_DECL ~TcpAcceptEvent()
    {
        delete mTcpChannel;
    }

public:
    mutable TcpChannel* mTcpChannel;

private:
    TcpAcceptEvent(
        const TcpAcceptEvent&) = delete;
    TcpAcceptEvent& operator=(
        const TcpAcceptEvent&) = delete;
};

SEV_NS_END

#endif // SUBEVENT_TCP_HPP
