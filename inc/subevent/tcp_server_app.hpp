#ifndef SUBEVENT_TCP_SERVER_APP_HPP
#define SUBEVENT_TCP_SERVER_APP_HPP

#include <vector>

#include <subevent/std.hpp>
#include <subevent/network.hpp>
#include <subevent/tcp.hpp>

SEV_NS_BEGIN

class Thread;

//----------------------------------------------------------------------------//
// TcpChannelWorker
//----------------------------------------------------------------------------//

class TcpChannelWorker : public NetThread
{
public:
    typedef std::function<
        void(const TcpChannelPtr&, std::vector<char>&&)> ReceiveHandler;
    typedef std::function<
        void(const TcpChannelPtr&)> CloseHandler;

#ifdef _WIN32
    static const uint16_t MaxChannles = 50;
#else
    static const uint16_t MaxChannles = 100;
#endif

    TcpChannelWorker(Thread* parent);

public:
    void setReceiveHandler(const ReceiveHandler& handler)
    {
        mReceiveHandler = handler;
    }

    void setCloseHandler(const CloseHandler& handler)
    {
        mCloseHandler = handler;
    }

protected:
    void onTcpAccept(const TcpChannelPtr& channel) override;

private:
    ReceiveHandler mReceiveHandler;
    CloseHandler mCloseHandler;
};

//----------------------------------------------------------------------------//
// TcpServerApp
//----------------------------------------------------------------------------//

class TcpServerApp : public NetApplication
{
public:
    typedef std::function<
        bool(const TcpChannelPtr&)> AcceptHandler;

    TcpServerApp(uint16_t threads = 1);

public:
    bool open(const IpEndPoint& localEndPoint);
    void close();

    void onAccept(const AcceptHandler& handler)
    {
        mAcceptHandler = handler;
    }

    void onReceive(
        const TcpChannelWorker::ReceiveHandler& handler)
    {
        mReceiveHandler = handler;
    }

    void onClose(
        const TcpChannelWorker::CloseHandler& handler)
    {
        mCloseHandler = handler;
    }

protected:
    bool onInit() override;

private:
    bool createThread();
    NetThread* nextThread();

private:
    TcpServerPtr mTcpServer;

    AcceptHandler mAcceptHandler;
    TcpChannelWorker::ReceiveHandler mReceiveHandler;
    TcpChannelWorker::CloseHandler mCloseHandler;

    int32_t mThreadIndex;
    uint16_t mThreads;
    std::vector<NetThread*> mThreadPool;
};

SEV_NS_END

#endif // SUBEVENT_TCP_SERVER_APP_HPP
