#include <iostream>
#include <subevent/subevent.hpp>

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyThread
//---------------------------------------------------------------------------//

class MyThread : public TcpChannelThread
{
public:
    MyThread(Thread* parent)
        : TcpChannelThread(parent) {}

protected:
    uint16_t getMaxChannels() const override
    {
#ifdef _WIN32
        return 50;
#else
        return 100;
#endif
    }

    void onAccept(
        const TcpChannelPtr& channel) override
    {
        std::cout << channel->getPeerEndPoint().toString()
            << " accept" << std::endl;

        mChannelList.push_back(channel);
    }

    void onReceive(
        const TcpChannelPtr& channel,
        std::vector<char>&& message) override
    {
        std::cout << channel->getPeerEndPoint().toString()
            << " receive " << message.size() << "bytes" << std::endl;

        // echo
        channel->send(std::move(message));
    }

    void onClose(
        const TcpChannelPtr& channel) override
    {
        std::cout << channel->getPeerEndPoint().toString()
            << " close" << std::endl;

        mChannelList.remove(channel);
    }

protected:
    void onExit() override
    {
        // thread end

        for (TcpChannelPtr& channel : mChannelList)
        {
            channel->close();
        }
        mChannelList.clear();

        TcpChannelThread::onExit();
    }

private:
    std::list<TcpChannelPtr> mChannelList;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    TcpServerApp app;
    app.getTcpServer()->getSocketOption().setReuseAddress(true);

    size_t numberOfThreads = 10;

    app.createThread<MyThread>(numberOfThreads);

    uint16_t port = 9000;

    // listen
    app.open(IpEndPoint(port));

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false, [&](Timer*) {
        app.stop();
    });

    return app.run();
}
