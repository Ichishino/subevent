#ifndef SUBEVENT_NETWORK_HPP
#define SUBEVENT_NETWORK_HPP

#include <string>

#include <subevent/std.hpp>
#include <subevent/thread.hpp>
#include <subevent/application.hpp>
#include <subevent/socket_controller.hpp>

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
    SEV_DECL static NetWorker* getCurrent()
    {
        return dynamic_cast<NetWorker*>(
            Thread::getCurrent());
    }

    SEV_DECL SocketController* getSocketController() const
    {
        return dynamic_cast<SocketController*>(
            mThread->getEventController());
    }

    SEV_DECL Thread* getThread() const
    {
        return mThread;
    }

protected:
    SEV_DECL NetWorker(Thread* thread);
    SEV_DECL virtual ~NetWorker();

    Thread* mThread;

private:
    NetWorker() = delete;
};

//---------------------------------------------------------------------------//
// NetTask
//---------------------------------------------------------------------------//

template<typename ThreadType, typename NetWorkerType>
class NetTask : public ThreadType, public NetWorkerType
{
public:
    SEV_DECL explicit NetTask(Thread* parent = nullptr)
        : ThreadType(parent), NetWorkerType(this)
    {
    }

    SEV_DECL explicit NetTask(
        const std::string& name, Thread* parent = nullptr)
        : ThreadType(name, parent), NetWorkerType(this)
    {
    }
};

//---------------------------------------------------------------------------//
// NetApplication
//---------------------------------------------------------------------------//

typedef NetTask<Application, NetWorker> NetApplication;

//---------------------------------------------------------------------------//
// NetThread
//---------------------------------------------------------------------------//

typedef NetTask<Thread, NetWorker> NetThread;

SEV_NS_END

#endif // SUBEVENT_NETWORK_HPP
