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

        IpEndPoint server("127.0.0.1", 9000);

        // connect
        mTcpClient.connect(server, [&](TcpClient*, int errorCode) {

            std::cout << "connect: " << errorCode << std::endl;

            if (errorCode != 0)
            {
                // connect error
                stop();
                return;
            }

            std::cout <<
                "from " << mTcpClient.getLocalEndPoint().toString() <<
                " to " << mTcpClient.getPeerEndPoint().toString() <<
                std::endl;

            // repeat timer
            mSendTimer.start(1000, true, [&](Timer*) {
            
                const char* msg = "hello";
                uint32_t size = static_cast<uint32_t>(strlen(msg) + 1);

                std::cout << "send: " << msg << std::endl;

                // send
                mTcpClient.send(msg, size);
            });
        });

        // data received
        mTcpClient.setReceiveHandler([&](TcpChannel*) {

            for (;;)
            {
                char buff[256];
                int res = mTcpClient.receive(buff, sizeof(buff));
                if (res <= 0)
                {
                    break;
                }

                std::cout << "recv: " << buff << std::endl;
            }
        });

        // server closed
        mTcpClient.setCloseHandler([&](TcpChannel*) {

            mSendTimer.cancel();
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // close
        mTcpClient.close();

        Application::onExit();
    }

private:
    TcpClient mTcpClient;
    Timer mSendTimer;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

int main(int argc, char** argv)
{
    MyApp app(argc, argv);
    return app.run();
}
