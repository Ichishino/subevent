#ifndef SUBEVENT_TCP_SERVER_WORKER_HPP
#define SUBEVENT_TCP_SERVER_WORKER_HPP

#include <vector>

#include <subevent/std.hpp>
#include <subevent/thread.hpp>
#include <subevent/application.hpp>
#include <subevent/network.hpp>
#include <subevent/tcp.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// TcpChannelWorker
//----------------------------------------------------------------------------//

class TcpChannelWorker : public NetWorker
{
public:
    SEV_DECL virtual ~TcpChannelWorker() override;

public:
    SEV_DECL bool isChannelFull() const
    {
        return (getSocketCount() >= getMaxChannels());
    }

protected:
    SEV_DECL TcpChannelWorker(Thread* thread);

    SEV_DECL virtual uint16_t getMaxChannels() const
    {
#ifdef _WIN32
        return 50;
#else
        return 100;
#endif
    }

    SEV_DECL virtual void onAccept(
        const TcpChannelPtr& /* channel */) {}
    SEV_DECL virtual void onReceive(
        const TcpChannelPtr& /* channel */,
        std::vector<char>&& /* message */) {}
    SEV_DECL virtual void onClose(
        const TcpChannelPtr& /* channel */) {}

private:
    TcpChannelWorker() = delete;
};

//---------------------------------------------------------------------------//
// TcpChannelThread
//---------------------------------------------------------------------------//

typedef NetTask<Thread, TcpChannelWorker> TcpChannelThread;

//----------------------------------------------------------------------------//
// TcpServerWorker
//----------------------------------------------------------------------------//

class TcpServerWorker : public NetWorker
{
public:
    SEV_DECL virtual ~TcpServerWorker() override;

public:
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        int32_t listenBacklog = SOMAXCONN);

    SEV_DECL void close();

    template<typename ThreadType>
    SEV_DECL bool createThread(size_t threads)
    {
        for (size_t index = 0;
            index < threads; ++index)
        {
            // create thread

            ThreadType* thread =
                new ThreadType(mThread);

            if (!thread->start())
            {
                delete thread;
                break;
            }

            mWorkerPool.push_back(thread);
        }

        if (mWorkerPool.empty())
        {
            return false;
        }

        setCpuAffinity();

        return true;
    }

    SEV_DECL virtual void createTcpServer()
    {
        if (mTcpServer == nullptr)
        {
            mTcpServer = TcpServer::newInstance(this);
        }
    }

    SEV_DECL const TcpServerPtr& getTcpServer()
    {
        createTcpServer();
        return mTcpServer;
    }

protected:
    SEV_DECL TcpServerWorker(Thread* thread);
    SEV_DECL void onTcpAccept(
        const TcpServerPtr& server, const TcpChannelPtr& channel);

    TcpServerPtr mTcpServer;

private:
    TcpServerWorker() = delete;

    SEV_DECL void setCpuAffinity();
    SEV_DECL TcpChannelWorker* nextWorker();

    int32_t mWorkerIndex;
    std::vector<TcpChannelWorker*> mWorkerPool;
};

//---------------------------------------------------------------------------//
// TcpServerApp / TcpServerThread
//---------------------------------------------------------------------------//

typedef NetTask<Application, TcpServerWorker> TcpServerApp;
typedef NetTask<Thread, TcpServerWorker> TcpServerThread;

SEV_NS_END

#endif // SUBEVENT_TCP_SERVER_WORKER_HPP
