#ifndef SUBEVENT_NETWORK_INL
#define SUBEVENT_NETWORK_INL

#include <subevent/network.hpp>
#include <subevent/thread.hpp>
#include <subevent/socket_controller.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Network
//---------------------------------------------------------------------------//

bool Network::init(Thread* thread)
{
#ifdef _WIN32
    if (!WinSock::init())
    {
        return false;
    }
#endif

    // set async socket controller
    SocketController* sockController = getController(thread);
    if (sockController == nullptr)
    {
        sockController = new SocketController();

        if (!sockController->onInit())
        {
            delete sockController;
            return false;
        }

        thread->setEventController(sockController);
    }

    return true;
}

SocketController* Network::getController(Thread* thread)
{
    return dynamic_cast<SocketController*>(
        thread->getEventController());
}

SocketController* Network::getController()
{
    return getController(Thread::getCurrent());
}

uint32_t Network::getSocketCount(Thread* thread)
{
    return getController(thread)->getSocketCount();
}

bool Network::isSocketFull(Thread* thread)
{
    return getController(thread)->isFull();
}

SEV_NS_END

#endif // SUBEVENT_NETWORK_INL
