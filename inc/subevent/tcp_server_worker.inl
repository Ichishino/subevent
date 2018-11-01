#ifndef SUBEVENT_TCP_SERVER_WORKER_INL
#define SUBEVENT_TCP_SERVER_WORKER_INL

#include <subevent/tcp_server_worker.hpp>
#include <subevent/utility.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// TcpChannelWorker
//----------------------------------------------------------------------------//

TcpChannelWorker::TcpChannelWorker(Thread* thread)
    : NetWorker(thread)
{
    mThread->setEventHandler(
        TcpEventId::Accept, [&](const Event* event) {

        TcpChannelPtr newChannel = TcpServer::accept(event);

        if (newChannel != nullptr)
        {
            newChannel->setReceiveHandler(
                [&](const TcpChannelPtr& channel) {

                auto buffer = channel->receiveAll();

                if (!buffer.empty())
                {
                    onReceive(channel, std::move(buffer));
                }
            });

            newChannel->setCloseHandler(
                [&](const TcpChannelPtr& channel) {

                onClose(channel);
            });

            onAccept(newChannel);
        }
    });
}

TcpChannelWorker::~TcpChannelWorker()
{
}

//----------------------------------------------------------------------------//
// TcpServerWorker
//----------------------------------------------------------------------------//

TcpServerWorker::TcpServerWorker(Thread* thread)
    : NetWorker(thread)
    , mThreadIndex(-1)
{
}

TcpServerWorker::~TcpServerWorker()
{
}

void TcpServerWorker::close()
{
    if (mTcpServer != nullptr)
    {
        mTcpServer->close();
    }
}

void TcpServerWorker::setCpuAffinity()
{
    // CPU affinity
    Processor::bind(mThread, 0);

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

TcpChannelThread* TcpServerWorker::nextThread()
{
    if (mThreadPool.empty())
    {
        return nullptr;
    }

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

        if (!thread->isChannelFull() &&
            !thread->isSocketFull())
        {
            // found
            return thread;
        }
    }

    return nullptr;
}

SEV_NS_END

#endif // SUBEVENT_TCP_SERVER_WORKER_INL
