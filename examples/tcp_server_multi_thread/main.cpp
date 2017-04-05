#include <set>
#include <iostream>

#include "subevent.hpp"

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyThreadPool
//---------------------------------------------------------------------------//

template <typename ThreadType>
class MyThreadPool
{
public:
    MyThreadPool()
    {
        mIndex = (size_t)-1;
        mMaxSocketsOfEachThread = 0;
    }

    void createThreads(
        Thread* parent, size_t threadCount, size_t maxSocketsOfEachThread)
    {
        // Windows: (maxSocketsOfEachThread <= 63)
        // Linux: (maxSocketsOfEachThread <= 1024)

        mMaxSocketsOfEachThread = maxSocketsOfEachThread;

        for (size_t index = 0; index < threadCount; ++index)
        {
            // automatically deleted by parent
            auto thread = new ThreadType(parent);
            if (!thread->start())
            {
                delete thread;
                break;
            }

            mThreads.push_back(thread);
        }

        // CPU affinity
        uint16_t cpuCount = Processor::getCount();
        if (cpuCount > 1)
        {
            uint16_t cpu = 1;
            for (auto thread : mThreads)
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
        if (mThreads.empty())
        {
            return nullptr;
        }

        size_t startIndex = mIndex;

        for (;;)
        {
            ++mIndex;

            if (mIndex >= mThreads.size())
            {
                mIndex = 0;
            }
            if (startIndex == mIndex)
            {
                break;
            }

            auto thread = mThreads[mIndex];

            if (thread->getSocketCount() < mMaxSocketsOfEachThread)
            {
                // found
                return thread;
            }
        }

        // not found
        return nullptr;
    }

    size_t getChannelCount() const
    {
        size_t clients = 0;

        for (auto thread : mThreads)
        {
            clients += thread->getSocketCount();
        }

        return clients;
    }

private:
    size_t mIndex;
    size_t mMaxSocketsOfEachThread;
    std::vector<ThreadType*> mThreads;
};

//---------------------------------------------------------------------------//
// MyThread
//---------------------------------------------------------------------------//

class MyThread : public NetThread
{
public:
    MyThread(Thread* parent)
        : NetThread(parent)
    {
    }

protected:
    bool onInit() override
    {
        NetThread::onInit();
        return true;
    }

    void onTcpAccept(const TcpChannelPtr& newChannel) override
    {
        mTcpChannels.insert(newChannel);

        // data received
        newChannel->setReceiveHandler(
            [&](const TcpChannelPtr& channel) {

            auto data = channel->receiveAll();

            if (data.empty())
            {
                return;
            }

            // send
            channel->send(&data[0], data.size());
        });

        // client closed
        newChannel->setCloseHandler(
            [&](const TcpChannelPtr& channel) {

            mTcpChannels.erase(channel);
        });
    }

    void onExit() override
    {
        // close all clients
        for (auto channel : mTcpChannels)
        {
            channel->close();
        }
        mTcpChannels.clear();

        NetThread::onExit();
    }

private:
    std::set<TcpChannelPtr> mTcpChannels;
};

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public NetApplication
{
protected:
    bool onInit() override
    {
        NetApplication::onInit();

        // CPU affinity
        Processor::bind(this, 0);

        // init sub threads
        mThreadPool.createThreads(this, 10, 50); // in this case limit is 500 clients (10 * 50)

        IpEndPoint local(9000);

        mTcpServer = TcpServer::newInstance();

        // option
        mTcpServer->getSocketOption().setReuseAddress(true);

        std::cout << "open: " <<
            local.toString() << std::endl;

        // listen
        mTcpServer->open(local,
            [&](const TcpServerPtr&, const TcpChannelPtr& newChannel) {

            Thread* thread = mThreadPool.find();
            if (thread == nullptr)
            {
                // reached limit

                std::cout << "reached limit: " <<
                    mThreadPool.getChannelCount() << std::endl;

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

        NetApplication::onExit();
    }

private:
    TcpServerPtr mTcpServer;
    MyThreadPool<MyThread> mThreadPool;

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
