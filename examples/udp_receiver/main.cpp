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

        IpEndPoint local(9001);

        mUdpReceiver = UdpReceiver::newInstance();

        // option
        mUdpReceiver->getSocketOption().setReuseAddress(true);

        std::cout << "open: " <<
            local.toString() << std::endl;

        // open
        mUdpReceiver->open(local,
            [&](const UdpReceiverPtr& receiver) {

            // data received

            for (;;)
            {
                char buff[256];
                IpEndPoint sender;

                int32_t res = receiver->receive(buff, sizeof(buff), sender);

                if (res <= 0)
                {
                    break;
                }

                std::cout << "recv: " << buff <<
                    " from " << sender.toString() << std::endl;
            }
        });

        // app end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // receiver close
        mUdpReceiver->close();

        NetApplication::onExit();
    }

private:
    UdpReceiverPtr mUdpReceiver;

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
