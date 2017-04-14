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

    IpEndPoint server("127.0.0.1", 9000);

    TcpClientPtr client = TcpClient::newInstance(&app);

    // connect
    client->connect(server,
        [&](const TcpClientPtr&, int errorCode) {

        if (errorCode != 0)
        {
            app.stop();
            return;
        }

        client->sendString("hello");
    });

    // receive handler
    client->setReceiveHandler(
        [&](const TcpChannelPtr&) {

        auto message = client->receiveAll();
        std::cout << &message[0] << std::endl;
    });

    // close handler
    client->setCloseHandler(
        [&](const TcpChannelPtr&) {

        app.stop();
    });

    return app.run();
}
