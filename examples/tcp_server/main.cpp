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

    std::list<TcpChannelPtr> channelList;

    // listen
    bool result = server->open(local,
        [&app, &channelList](
            const TcpServerPtr& server,
            const TcpChannelPtr& newChannel) {

        if (server->accept(&app, newChannel))
        {
            channelList.push_back(newChannel);

            std::cout << newChannel->getPeerEndPoint().toString()
                << " accept" << std::endl;

            // receive handler
            newChannel->setReceiveHandler(
                [&](const TcpChannelPtr& channel) {

                auto message = channel->receiveAll();

                std::cout << channel->getPeerEndPoint().toString()
                    << " receive " << message.size() << "bytes" << std::endl;

                // echo
                channel->send(std::move(message));
            });

            // close handler
            newChannel->setCloseHandler(
                [&channelList](const TcpChannelPtr& channel) {

                std::cout << channel->getPeerEndPoint().toString()
                    << " close" << std::endl;

                channelList.remove(channel);
            });
        }
    });

    if (!result)
    {
        // listen error
        app.stop();
    }

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false,
        [&app, &channelList](Timer*) {

        for (TcpChannelPtr& channel : channelList)
        {
            channel->close();
        }
        channelList.clear();

        app.stop();
    });

    return app.run();
}
