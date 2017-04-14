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

    std::string message = "hello";

    // send
    sender->send(message.c_str(), message.size() + 1);

    app.stop();

    return app.run();
}
