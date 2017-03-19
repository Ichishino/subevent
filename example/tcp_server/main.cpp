#include <set>
#include <iostream>

#include "subevent.hpp"

SEV_IMPL_GLOBAL

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public Application
{
public:
    using Application::Application;

protected:
    bool onInit() override
    {
        Application::onInit();
        Network::init(this);

        IpEndPoint local(9000);

        SocketOption option;
        option.setReuseAddr(true);

        std::cout << "open: " <<
            local.toString() << std::endl;

        // listen
        mTcpServer.open(local, [&](TcpServer*, TcpChannel* newChannel) {

            // accept
            if (!mTcpServer.accept(this, newChannel))
            {
                // reached limit
                std::cout << "reached limit" << std::endl;

                newChannel->close();
                delete newChannel;

                return;
            }

            std::cout << "accept: " <<
                newChannel->getPeerEndPoint().toString() << std::endl;

            mTcpChannels.insert(newChannel);

            // data received
            newChannel->setReceiveHandler([&](TcpChannel* channel) {

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
            newChannel->setCloseHandler([&](TcpChannel* channel) {

                std::cout << "closed: " <<
                    channel->getPeerEndPoint().toString() << std::endl;

                mTcpChannels.erase(channel);

                channel->close();
                delete channel;
            });

        }, option);

        // end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // server close
        mTcpServer.close();

        // client close
        for (TcpChannel* channel : mTcpChannels)
        {
            channel->close();
            delete channel;
        }
        mTcpChannels.clear();

        Application::onExit();
    }

private:
    TcpServer mTcpServer;
    std::set<TcpChannel*> mTcpChannels;

    Timer mEndTimer;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

int main(int argc, char** argv)
{
    MyApp app(argc, argv);
    return app.run();
}
