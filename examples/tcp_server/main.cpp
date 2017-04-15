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
    TcpServerPtr server = TcpServer::newInstance(&app);

    server->getSocketOption().setReuseAddress(true);

    IpEndPoint local(9000);

    // listen
    server->open(local, [&](const TcpServerPtr&,
        const TcpChannelPtr& newChannel) {

        if (server->accept(&app, newChannel))
        {
            // receive handler
            newChannel->setReceiveHandler(
                [&](const TcpChannelPtr& channel) {

                auto message = channel->receiveAll();

                std::cout << &message[0] << std::endl;

                channel->send(&message[0], message.size());
            });

            // close handler
            newChannel->setCloseHandler(
                [&](const TcpChannelPtr& /*channel*/) {
            });
        }
    });

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false, [&](Timer*) {
        app.stop();
    });

    return app.run();
}
