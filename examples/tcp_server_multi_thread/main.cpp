#include <subevent/subevent.hpp>

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyThread
//---------------------------------------------------------------------------//

class MyThread : public TcpChannelWorker
{
public:
    MyThread(Thread* parent)
        : TcpChannelWorker(parent) {}

protected:
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

int main(int argc, char** argv)
{
    TcpServerApp app(argc, argv);

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
