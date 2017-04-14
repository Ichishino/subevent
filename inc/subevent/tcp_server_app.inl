#ifndef SUBEVENT_TCP_SERVER_APP_INL
#define SUBEVENT_TCP_SERVER_APP_INL

#include <subevent/tcp_server_app.hpp>
#include <subevent/utility.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// TcpChannelWorker
//----------------------------------------------------------------------------//

TcpChannelWorker::TcpChannelWorker(Thread* parent)
    : NetThread(parent)
{
}

TcpChannelWorker::~TcpChannelWorker()
{
}

void TcpChannelWorker::onTcpAccept(const TcpChannelPtr& channel)
{
    channel->setReceiveHandler(
        [&](const TcpChannelPtr& channel) {

        auto buffer = channel->receiveAll();

        if (!buffer.empty())
        {
            onReceive(channel, std::move(buffer));
        }
    });

    channel->setCloseHandler(
        [&](const TcpChannelPtr& channel) {

        onClose(channel);
    });
}

//----------------------------------------------------------------------------//
// TcpServerApp
//----------------------------------------------------------------------------//

TcpServerApp::TcpServerApp(const std::string& name)
    : TcpServerApp(0, nullptr, name)
{
}

TcpServerApp::TcpServerApp(
    int32_t argc, char* argv[], const std::string& name)
    : NetApplication(argc, argv, name)
    , mThreadIndex(-1)
{
}

TcpServerApp::~TcpServerApp()
{
}

bool TcpServerApp::open(
    const IpEndPoint& localEndPoint, int32_t backlog)
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

        mTcpServer->accept(thread, channel);

    }, backlog);

    return result;
}

void TcpServerApp::close()
{
    if (mTcpServer != nullptr)
    {
        mTcpServer->close();
    }
}

void TcpServerApp::setCpuAffinity()
{
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

#endif // SUBEVENT_TCP_SERVER_APP_INL
