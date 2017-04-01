#include <set>
#include <iostream>

#include "subevent.hpp"

SEV_USING_NS

//---------------------------------------------------------------------------//
// MySimpleBalancer
//---------------------------------------------------------------------------//

template <typename ThreadType>
class MySimpleBalancer
{
public:
    MySimpleBalancer()
    {
        mIndex = (size_t)-1;
        mMaxOfEachThread = 0;
    }

    void createThreads(
        Thread* parent, size_t threadCount, size_t maxOfEachThread)
    {
        // Windows: (maxOfEachThread <= 63)
        // Linux: (maxOfEachThread <= 1024)

        mMaxOfEachThread = maxOfEachThread;

        for (size_t index = 0; index < threadCount; ++index)
        {
            // automatically deleted by parent
            auto thread = new ThreadType(parent);
            if (!thread->start())
            {
                delete thread;
                break;
            }

            mThreadPool.push_back(thread);
        }

        // CPU affinity
        uint16_t cpuCount = Processor::getCount();
        if (cpuCount > 1)
        {
            uint16_t cpu = 1;
            for (auto thread : mThreadPool)
            {
                if (cpu >= cpuCount)
                {
                    cpu = 1;
                }

                Processor::bind(thread, cpu);
                ++cpu;
            }
        }
    }

    ThreadType* find()
    {
        if (mThreadPool.empty())
        {
            return nullptr;
        }

        size_t startIndex = mIndex;

        for (;;)
        {
            ++mIndex;

            if (mIndex >= mThreadPool.size())
            {
                mIndex = 0;
            }
            if (startIndex == mIndex)
            {
                break;
            }

            auto thread = mThreadPool[mIndex];

            if (Network::getSocketCount(thread) < mMaxOfEachThread)
            {
                // found
                return thread;
            }
        }

        // not found
        return nullptr;
    }

    size_t getClientCount() const
    {
        size_t clients = 0;

        for (auto thread : mThreadPool)
        {
            clients += Network::getSocketCount(thread);
        }

        return clients;
    }

private:
    size_t mIndex;
    size_t mMaxOfEachThread;
    std::vector<ThreadType*> mThreadPool;
};

//---------------------------------------------------------------------------//
// MyClientThread
//---------------------------------------------------------------------------//

class MyClientThread : public Thread
{
public:
    MyClientThread(Thread* parent)
        : Thread(parent)
    {
    }

protected:
    bool onInit() override
    {
        Thread::onInit();
        Network::init(this);

        setEventHandler(
            TcpEventId::Accept, [&](const Event* event) {

            // accept
            TcpChannelPtr newChannel = TcpServer::accept(event);

            mTcpChannels.insert(newChannel);

            // data received
            newChannel->setReceiveHandler([&](TcpChannelPtr channel) {

                for (;;)
                {
                    char buff[256];
                    int res = channel->receive(buff, sizeof(buff));
                    if (res <= 0)
                    {
                        break;
                    }

                    // send
                    channel->send(buff, res);
                }
            });

            // client closed
            newChannel->setCloseHandler([&](TcpChannelPtr channel) {

                mTcpChannels.erase(channel);
            });
        });

        return true;
    }

    void onExit() override
    {
        // close all clients
        for (auto channel : mTcpChannels)
        {
            channel->close();
        }
        mTcpChannels.clear();

        Thread::onExit();
    }

private:
    std::set<TcpChannelPtr> mTcpChannels;
};

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public Application
{
protected:
    bool onInit() override
    {
        Application::onInit();
        Network::init(this);

        // CPU affinity
        Processor::bind(this, 0);

        // init sub threads
        mBalancer.createThreads(this, 10, 50); // in this case limit is 500 clients (10 * 50)

        IpEndPoint local(9000);

        mTcpServer = TcpServer::newInstance();

        // option
        mTcpServer->getSocketOption().setReuseAddress(true);

        std::cout << "open: " <<
            local.toString() << std::endl;

        // listen
        mTcpServer->open(local, [&](TcpServerPtr, TcpChannelPtr newChannel) {

            Thread* thread = mBalancer.find();
            if (thread == nullptr)
            {
                // reached limit

                std::cout << "reached limit: " <<
                    mBalancer.getClientCount() << std::endl;

                newChannel->close();

                return;
            }

            std::cout << "accept: " <<
                newChannel->getPeerEndPoint().toString() << std::endl;

            // entrust this client to sub thread
            mTcpServer->accept(thread, newChannel);

        });

        // end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // server close
        mTcpServer->close();

        Application::onExit();
    }

private:
    TcpServerPtr mTcpServer;
    MySimpleBalancer<MyClientThread> mBalancer;

    Timer mEndTimer;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    MyApp app;
    return app.run();
}
