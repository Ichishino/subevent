#include <iostream>

#include "subevent.hpp"

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

        IpEndPoint local(9001);

        mUdpReceiver = UdpReceiver::newInstance();

        // option
        mUdpReceiver->getSocketOption().setReuseAddress(true);

        std::cout << "open: " <<
            local.toString() << std::endl;

        // open
        mUdpReceiver->open(local, [&](UdpReceiverPtr) {

            // data received
            for (;;)
            {
                char buff[256];
                IpEndPoint sender;

                int res = mUdpReceiver->receive(buff, sizeof(buff), sender);
                if (res <= 0)
                {
                    break;
                }

                std::cout << "recv: " << buff <<
                    " from " << sender.toString() << std::endl;
            }
        });

        // end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // receiver close
        mUdpReceiver->close();

        Application::onExit();
    }

private:
    UdpReceiverPtr mUdpReceiver;

    Timer mEndTimer;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int argc, char** argv)
{
    MyApp app(argc, argv);
    return app.run();
}
