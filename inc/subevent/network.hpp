#ifndef SUBEVENT_NETWORK_HPP
#define SUBEVENT_NETWORK_HPP

#include <subevent/std.hpp>
#include <subevent/thread.hpp>
#include <subevent/application.hpp>
#include <subevent/socket_controller.hpp>
#include <subevent/tcp.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Network
//---------------------------------------------------------------------------//

class Network
{
public:
    SEV_DECL static bool init();

private:
    SEV_DECL Network();
    SEV_DECL ~Network();

    int32_t mErrorCode;
};

//---------------------------------------------------------------------------//
// NetWorker
//---------------------------------------------------------------------------//

class NetWorker
{
public:
    SEV_DECL SocketController* getSocketController() const
    {
        return dynamic_cast<SocketController*>(
            mThread->getEventController());
    }

    SEV_DECL uint32_t getSocketCount() const
    {
        return getSocketController()->getSocketCount();
    }

    SEV_DECL bool isSocketFull() const
    {
        return getSocketController()->isFull();
    }

    SEV_DECL static NetWorker* getCurrent()
    {
        return dynamic_cast<NetWorker*>(
            Thread::getCurrent());
    }

public:
    SEV_DECL bool requestTcpAccept(const TcpChannelPtr& newChannel);

protected:
    SEV_DECL NetWorker(Thread* thread);
    SEV_DECL virtual ~NetWorker();

    SEV_DECL virtual void onTcpAccept(const TcpChannelPtr& newChannel);

private:
    NetWorker() = delete;

    Thread* mThread;
};

//---------------------------------------------------------------------------//
// NetApplication
//---------------------------------------------------------------------------//

class NetApplication : public Application, public NetWorker
{
public:
    SEV_DECL explicit NetApplication(const std::string& name = "")
        : Application(name), NetWorker(this)
    {
    }

    SEV_DECL NetApplication(
        int32_t argc, char* argv[], const std::string& name = "")
        : Application(argc, argv, name), NetWorker(this)
    {
    }
};

//---------------------------------------------------------------------------//
// NetThread
//---------------------------------------------------------------------------//

class NetThread : public Thread, public NetWorker
{
public:
    SEV_DECL explicit NetThread(Thread* parent = nullptr)
        : Thread(parent), NetWorker(this)
    {
    }
    
    SEV_DECL explicit NetThread(
        const std::string& name, Thread* parent = nullptr)
        : Thread(name, parent), NetWorker(this)
    {
    }
};

SEV_NS_END

#endif // SUBEVENT_NETWORK_HPP
