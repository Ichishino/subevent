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

public:
    SEV_DECL bool postEvent(Event* event)
    {
        return mThread->post(event);
    }

    SEV_DECL bool postTask(const std::function<void()>& task)
    {
        return mThread->post(task);
    }

public:
    static NetWorker* getCurrent()
    {
        return dynamic_cast<NetWorker*>(
            Thread::getCurrent());
    }

protected:
    SEV_DECL NetWorker(Thread* thread);
    SEV_DECL virtual ~NetWorker();

    Thread* mThread;

private:
    NetWorker() = delete;
};

template<typename NetWorkerType>
class NetWorkerApp : public Application, public NetWorkerType
{
public:
    SEV_DECL explicit NetWorkerApp(const std::string& name = "")
        : Application(name), NetWorkerType(this)
    {
    }

    SEV_DECL NetWorkerApp(
        int32_t argc, char* argv[], const std::string& name = "")
        : Application(argc, argv, name), NetWorkerType(this)
    {
    }
};

template<typename NetWorkerType>
class NetWorkerThread : public Thread, public NetWorkerType
{
public:
    SEV_DECL explicit NetWorkerThread(Thread* parent = nullptr)
        : Thread(parent), NetWorkerType(this)
    {
    }

    SEV_DECL explicit NetWorkerThread(
        const std::string& name, Thread* parent = nullptr)
        : Thread(name, parent), NetWorkerType(this)
    {
    }
};

//---------------------------------------------------------------------------//
// NetApplication
//---------------------------------------------------------------------------//

typedef NetWorkerApp<NetWorker> NetApplication;

//---------------------------------------------------------------------------//
// NetThread
//---------------------------------------------------------------------------//

typedef NetWorkerThread<NetWorker> NetThread;

SEV_NS_END

#endif // SUBEVENT_NETWORK_HPP
