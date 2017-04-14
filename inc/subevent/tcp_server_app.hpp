#ifndef SUBEVENT_TCP_SERVER_APP_HPP
#define SUBEVENT_TCP_SERVER_APP_HPP

#include <vector>
#include <string>

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
    SEV_DECL TcpChannelWorker(Thread* parent);
    SEV_DECL virtual ~TcpChannelWorker();

public:
    SEV_DECL bool isChannelFull() const
    {
        return (getSocketCount() >= getMaxChannels());
    }

protected:
    SEV_DECL virtual void onAccept(
        const TcpChannelPtr& /* channel */) {}
    SEV_DECL virtual void onReceive(
        const TcpChannelPtr& /* channel */,
        std::vector<char>&& /* message */) {}
    SEV_DECL virtual void onClose(
        const TcpChannelPtr& /* channel */) {}

    SEV_DECL virtual uint16_t getMaxChannels() const
    {
#ifdef _WIN32
        return 50;
#else
        return 100;
#endif
    }

private:
    SEV_DECL void onTcpAccept(
        const TcpChannelPtr& channel) override;
};

//----------------------------------------------------------------------------//
// TcpServerApp
//----------------------------------------------------------------------------//

class TcpServerApp : public NetApplication
{
public:
    SEV_DECL explicit TcpServerApp(const std::string& name = "");
    SEV_DECL TcpServerApp(
        int32_t argc, char* argv[], const std::string& name = "");
    SEV_DECL virtual ~TcpServerApp() override;

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
                new ThreadType(this);

            if (!thread->start())
            {
                delete thread;
                break;
            }

            mThreadPool.push_back(thread);
        }

        if (mThreadPool.empty())
        {
            return false;
        }

        setCpuAffinity();

        return true;
    }

    SEV_DECL const TcpServerPtr& getTcpServer() const
    {
        return mTcpServer;
    }

private:
    SEV_DECL void setCpuAffinity();
    SEV_DECL NetThread* nextThread();

    TcpServerPtr mTcpServer;

    int32_t mThreadIndex;
    std::vector<TcpChannelWorker*> mThreadPool;
};

SEV_NS_END

#endif // SUBEVENT_TCP_SERVER_APP_HPP
