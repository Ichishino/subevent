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

        IpEndPoint receiver("127.0.0.1", 9001);

        // create
        mUdpSender.create(receiver);

        // repeat timer
        mSendTimer.start(1000, true, [&](Timer*) {

            const char* msg = "hello";
            uint32_t size = (uint32_t)(strlen(msg) + 1);

            std::cout << "send: " << msg <<
                " to " << mUdpSender.getReceiverEndPoint().toString() <<
                std::endl;

            // send
            mUdpSender.send(msg, size);
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
        mUdpSender.close();

        Application::onExit();
    }

private:
    UdpSender mUdpSender;

    Timer mSendTimer;
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
