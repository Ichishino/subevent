#include <iostream>

#include <subevent/subevent.hpp>

SEV_USING_NS

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    NetApplication app;
    UdpReceiverPtr receiver = UdpReceiver::newInstance(&app);

    receiver->getSocketOption().setReuseAddress(true);

    IpEndPoint local(9001);

    // open
    receiver->open(local,
        [](const UdpReceiverPtr& receiver) {

        for (;;)
        {
            char message[8192];
            IpEndPoint sender;

            // receive
            int32_t res = receiver->receive(
                message, sizeof(message), sender);
            if (res <= 0)
            {
                break;
            }

            std::cout << sender.toString()
                << " receive " << res << "bytes" << std::endl;
        }
    });

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false, [&](Timer*) {
        app.stop();
    });

    return app.run();
}
