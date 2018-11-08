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
    , mWorkerIndex(-1)
{
}

TcpServerWorker::~TcpServerWorker()
{
}

bool TcpServerWorker::open(
    const IpEndPoint& localEndPoint, int32_t listenBacklog)
{
    createTcpServer();

    // listen
    bool result = mTcpServer->open(
        localEndPoint,
        SEV_BIND_2(this, TcpServerWorker::onTcpAccept),
        listenBacklog);

    return result;
}

void TcpServerWorker::close()
{
    if (mTcpServer != nullptr)
    {
        mTcpServer->close();
    }
}

void TcpServerWorker::onTcpAccept(
    const TcpServerPtr& server, const TcpChannelPtr& channel)
{
    TcpChannelWorker* worker = nextWorker();

    if (worker == nullptr)
    {
        channel->close();
        return;
    }

    if (!server->accept(worker, channel))
    {
        channel->close();
        return;
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
        for (auto worker : mWorkerPool)
        {
            if (cpu >= cpuCount)
            {
                cpu = 1;
            }

            Processor::bind(worker->getThread(), cpu);
            ++cpu;
        }
    }
}

TcpChannelWorker* TcpServerWorker::nextWorker()
{
    if (mWorkerPool.empty())
    {
        return nullptr;
    }

    int32_t startIndex = mWorkerIndex;

    // round robin
    for (;;)
    {
        ++mWorkerIndex;

        if (mWorkerIndex >=
            static_cast<int32_t>(mWorkerPool.size()))
        {
            mWorkerIndex = 0;
        }

        if (startIndex == mWorkerIndex)
        {
            break;
        }

        auto worker = mWorkerPool[mWorkerIndex];

        if (!worker->isChannelFull() &&
            !worker->isSocketFull())
        {
            // found
            return worker;
        }
    }

    return nullptr;
}

SEV_NS_END

#endif // SUBEVENT_TCP_SERVER_WORKER_INL
