#ifndef SUBEVENT_NETWORK_HPP
#define SUBEVENT_NETWORK_HPP

#include <subevent/std.hpp>
#include <subevent/thread.hpp>
#include <subevent/application.hpp>
#include <subevent/socket.hpp>
#include <subevent/socket_controller.hpp>
#include <subevent/tcp.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// NetWorker
//---------------------------------------------------------------------------//

template <typename BaseClass>
class NetWorker : public BaseClass
{
public:
    using BaseClass::BaseClass;

protected:
    bool onInit() override
    {
#ifdef _WIN32
        if (!WinSock::init())
        {
            return false;
        }
#endif
        if (!BaseClass::onInit())
        {
            return false;
        }

        // async socket controller
        SocketController* sockController = new SocketController();

        if (!sockController->onInit())
        {
            delete sockController;
            return false;
        }

        this->setEventController(sockController);

        // tcp accept for multithreading
        this->setEventHandler(
            TcpEventId::Accept, [&](const Event* event) {

            TcpChannelPtr channel = TcpServer::accept(event);

            if (channel != nullptr)
            {
                onTcpAccept(channel);
            }
        });

        return true;
    }

    virtual void onTcpAccept(TcpChannelPtr channel)
    {
        // for multithreading
    }

public:
    uint32_t getSocketCount() const
    {
        return dynamic_cast<const SocketController*>(
            this->getEventController())->getSocketCount();
    }

    bool isSocketFull() const
    {
        return dynamic_cast<const SocketController*>(
            this->getEventController())->isFull();
    }
};

//---------------------------------------------------------------------------//
// NetApplication
//---------------------------------------------------------------------------//

typedef NetWorker<Application> NetApplication;

//---------------------------------------------------------------------------//
// NetThread
//---------------------------------------------------------------------------//

typedef NetWorker<Thread> NetThread;

SEV_NS_END

#endif // SUBEVENT_NETWORK_HPP
