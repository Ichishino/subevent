#include <iostream>

#include <subevent/subevent.hpp>

SEV_USING_NS

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    uint16_t port = 9000;
    uint16_t threads = 100;

    TcpServerApp app(threads);

    // listen
    app.open(IpEndPoint(port));

    // receive handler
    app.onReceive([](
        const TcpChannelPtr& channel,
        std::vector<char>&& message) {

        channel->send(
            &message[0], message.size());
    });

    // close handler
    app.onClose([](
        const TcpChannelPtr& /*channel*/) {
    });

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false, [&](Timer*) {
        app.stop();
    });

    return app.run();
}
