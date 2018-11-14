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
    UdpSenderPtr sender = UdpSender::newInstance(&app);

    IpEndPoint receiver("127.0.0.1", 9001);

    sender->create(receiver);

    // test send timer
    int count = 10;
    Timer timer;
    timer.start(1000, true,
        [&app, &sender, &count](const Timer*) {

        if (count == 0)
        {
            // stop app
            app.stop();
            return;
        }

        std::string message = "hello";

        std::cout << "send " << message.size() + 1
            << "bytes" << std::endl;

        sender->send(message.c_str(), message.size() + 1);

        --count;
    });

    return app.run();
}
