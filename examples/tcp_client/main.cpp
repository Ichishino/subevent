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
    TcpClientPtr client = TcpClient::newInstance(&app);

    IpEndPoint server("127.0.0.1", 9000);

    // connect
    client->connect(server,
        [&app](const TcpClientPtr& channel, int errorCode) {

        if (errorCode != 0)
        {
            app.stop();
            return;
        }

        channel->sendString("hello");
    });

    // receive handler
    client->setReceiveHandler(
        [](const TcpChannelPtr& channel) {

        auto message = channel->receiveAll();

        std::cout << channel->getPeerEndPoint().toString()
            << " receive " << message.size() << "bytes" << std::endl;
    });

    // close handler
    client->setCloseHandler(
        [&app](const TcpChannelPtr& channel) {

        std::cout << channel->getPeerEndPoint().toString()
            << " server close" << std::endl;

        app.stop();
    });

    return app.run();
}
