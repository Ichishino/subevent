#include <set>
#include <iostream>

#include "subevent.hpp"

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public NetApplication
{
protected:
    bool onInit() override
    {
        NetApplication::onInit();

        IpEndPoint local(9000);

        mTcpServer = TcpServer::newInstance();

        // option
        mTcpServer->getSocketOption().setReuseAddress(true);

        std::cout << "open: " <<
            local.toString() << std::endl;

        // listen
        mTcpServer->open(local, [&](TcpServerPtr, TcpChannelPtr newChannel) {

            // accept
            if (!mTcpServer->accept(this, newChannel))
            {
                // reached limit
                std::cout << "reached limit" << std::endl;

                newChannel->close();

                return;
            }

            std::cout << "accept: " <<
                newChannel->getPeerEndPoint().toString() << std::endl;

            mTcpChannels.insert(newChannel);

            // data received
            newChannel->setReceiveHandler([&](TcpChannelPtr channel) {

                for (;;)
                {
                    char buff[256];
                    int res = channel->receive(buff, sizeof(buff));
                    if (res <= 0)
                    {
                        break;
                    }

                    std::cout << "recv: " << buff <<
                        " from " << channel->getPeerEndPoint().toString() <<
                        std::endl;
                    std::cout << "send: " << buff <<
                        " to " << channel->getPeerEndPoint().toString() <<
                        std::endl;

                    // send
                    channel->send(buff, res);
                }
            });

            // client closed
            newChannel->setCloseHandler([&](TcpChannelPtr channel) {

                std::cout << "closed: " <<
                    channel->getPeerEndPoint().toString() << std::endl;

                mTcpChannels.erase(channel);
            });

        });

        // end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // server close
        mTcpServer->close();

        // client close
        for (auto channel : mTcpChannels)
        {
            channel->close();
        }
        mTcpChannels.clear();

        NetApplication::onExit();
    }

private:
    TcpServerPtr mTcpServer;
    std::set<TcpChannelPtr> mTcpChannels;

    Timer mEndTimer;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    MyApp app;
    return app.run();
}
