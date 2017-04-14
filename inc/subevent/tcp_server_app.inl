#ifndef SUBEVENT_TCP_SERVER_APP_INL
#define SUBEVENT_TCP_SERVER_APP_INL

#include <subevent/tcp_server_app.hpp>
#include <subevent/event.hpp>
#include <subevent/utility.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// TcpChannelWorker
//----------------------------------------------------------------------------//

TcpChannelWorker::TcpChannelWorker(Thread* parent)
    : NetThread(parent)
{
    mReceiveHandler =
        [](const TcpChannelPtr&, std::vector<char>&&) {
    };
    mCloseHandler =
        [](const TcpChannelPtr&) {
    };
}

void TcpChannelWorker::onTcpAccept(const TcpChannelPtr& channel)
{
    channel->setReceiveHandler(
        [&](const TcpChannelPtr& channel) {

        auto buffer = channel->receiveAll();

        if (!buffer.empty())
        {
            mReceiveHandler(channel, std::move(buffer));
        }
    });

    channel->setCloseHandler(
        [&](const TcpChannelPtr& channel) {

        mCloseHandler(channel);
    });
}

//----------------------------------------------------------------------------//
// TcpServerApp
//----------------------------------------------------------------------------//

typedef UserEvent<1> StopListeningEvent;

TcpServerApp::TcpServerApp(uint16_t threads)
    : mThreadIndex(-1), mThreads(threads)
{
    mAcceptHandler =
        [](const TcpChannelPtr&) {
        return true;
    };
    mReceiveHandler =
        [](const TcpChannelPtr&,
            std::vector<char>&&) {
    };
    mCloseHandler =
        [](const TcpChannelPtr&) {
    };

    setUserEventHandler<StopListeningEvent>(
        [&](const StopListeningEvent*) {
        if (mTcpServer != nullptr)
        {
            mTcpServer->close();
        }
    });
}

bool TcpServerApp::onInit()
{
    NetApplication::onInit();

    if (!createThread())
    {
        return false;
    }

    return true;
}

bool TcpServerApp::open(const IpEndPoint& localEndPoint)
{
    mTcpServer = TcpServer::newInstance(this);
    mTcpServer->getSocketOption().setReuseAddress(true);

    // listen
    bool result = mTcpServer->open(localEndPoint,
        [&](const TcpServerPtr&,
            const TcpChannelPtr& channel) {

        // accept

        NetThread* thread = nextThread();

        if (thread == nullptr)
        {
            channel->close();
            return;
        }

        if (!mAcceptHandler(channel))
        {
            channel->close();
            return;
        }

        mTcpServer->accept(thread, channel);
    });

    return result;
}

void TcpServerApp::close()
{
    if (mTcpServer == nullptr)
    {
        return;
    }

    if (this == Thread::getCurrent())
    {
        mTcpServer->close();
    }
    else
    {
        post(new StopListeningEvent());
    }
}

bool TcpServerApp::createThread()
{
    // create thread
    for (size_t index = 0;
        index < mThreads; ++index)
    {
        TcpChannelWorker* thread =
            new TcpChannelWorker(this);

        thread->setReceiveHandler(mReceiveHandler);
        thread->setCloseHandler(mCloseHandler);

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

    // CPU affinity
    Processor::bind(this, 0);

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

    return true;
}

NetThread* TcpServerApp::nextThread()
{
    int32_t startIndex = mThreadIndex;

    // round robin
    for (;;)
    {
        ++mThreadIndex;

        if (mThreadIndex >=
            static_cast<int32_t>(mThreadPool.size()))
        {
            mThreadIndex = 0;
        }

        if (startIndex == mThreadIndex)
        {
            break;
        }

        auto thread = mThreadPool[mThreadIndex];

        if (thread->getSocketCount() <
            TcpChannelWorker::MaxChannles)
        {
            // found
            return thread;
        }
    }

    return nullptr;
}

SEV_NS_END

#endif // SUBEVENT_TCP_SERVER_APP_INL
