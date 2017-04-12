#include <iostream>

#include <subevent/subevent.hpp>

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

        IpEndPoint server("127.0.0.1", 9000);

        mTcpClient = TcpClient::newInstance();

        // connect
        mTcpClient->connect(server,
            [&](const TcpClientPtr&, int errorCode) {

            std::cout << "connect: " << errorCode << std::endl;

            if (errorCode != 0)
            {
                // connect error
                stop();
                return;
            }

            std::cout <<
                "from " << mTcpClient->getLocalEndPoint().toString() <<
                " to " << mTcpClient->getPeerEndPoint().toString() <<
                std::endl;

            // repeat timer
            mSendTimer.start(1000, true, [&](Timer*) {
            
                const char* msg = "hello";
                size_t size = strlen(msg) + 1;

                std::cout << "send: " << msg << std::endl;

                // send
                mTcpClient->send(msg, size);
            });
        });

        // data received
        mTcpClient->setReceiveHandler(
            [&](const TcpChannelPtr& channel) {

            auto data = channel->receiveAll();

            if (data.empty())
            {
                return;
            }

            std::cout << "recv: " << &data[0] << std::endl;
        });

        // server closed
        mTcpClient->setCloseHandler(
            [&](const TcpChannelPtr&) {

            mSendTimer.cancel();
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // close
        mTcpClient->close();

        NetApplication::onExit();
    }

private:
    TcpClientPtr mTcpClient;
    Timer mSendTimer;
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
