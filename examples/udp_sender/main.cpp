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

        IpEndPoint receiver("127.0.0.1", 9001);

        mUdpSender = UdpSender::newInstance();

        // create
        mUdpSender->create(receiver);

        // repeat timer
        mSendTimer.start(1000, true, [&](Timer*) {

            const char* msg = "hello";
            size_t size = strlen(msg) + 1;

            std::cout << "send: " << msg <<
                " to " << mUdpSender->getReceiverEndPoint().toString() <<
                std::endl;

            // send
            mUdpSender->send(msg, size);
        });

        // end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {

            mSendTimer.cancel();
            stop();
        });

        return true;
    }

    void onExit() override
    {
        // sender close
        mUdpSender->close();

        NetApplication::onExit();
    }

private:
    UdpSenderPtr mUdpSender;

    Timer mSendTimer;
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
