#ifndef SUBEVENT_NETWORK_INL
#define SUBEVENT_NETWORK_INL

#include <subevent/network.hpp>
#include <subevent/thread.hpp>
#include <subevent/socket_controller.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Network
//---------------------------------------------------------------------------//

void Network::init(Thread* thread)
{
#ifdef _WIN32
    WinSock::init();
#endif

    // set async socket controller
    SocketController* sockController = getController(thread);
    if (sockController == nullptr)
    {
        thread->setEventController(new SocketController());
    }
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
