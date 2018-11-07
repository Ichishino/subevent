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
        const TcpChannelPtr& /* channel */) override
    {
    }

    void onReceive(
        const TcpChannelPtr& channel,
        std::vector<char>&& message) override
    {
        channel->send(&message[0], message.size());
    }

    void onClose(
        const TcpChannelPtr& /* channel */) override
    {
    }
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
