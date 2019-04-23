#ifndef SUBEVENT_THREAD_HPP
#define SUBEVENT_THREAD_HPP

#include <list>
#include <string>
#include <functional>
#include <thread>

#include <subevent/std.hpp>
#include <subevent/event_loop.hpp>

SEV_NS_BEGIN

class Thread;
class EventController;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::function<void(Thread*)> ChildFinishedHandler;

//----------------------------------------------------------------------------//
// Thread
//----------------------------------------------------------------------------//

class Thread
{
public:
    typedef std::thread::id Id;
    typedef std::thread::native_handle_type Handle;

    SEV_DECL explicit Thread(Thread* parent = nullptr);
    SEV_DECL explicit Thread(
        const std::string& name, Thread* parent = nullptr);
    SEV_DECL virtual ~Thread();

public:
    SEV_DECL bool start();
    SEV_DECL void wait();

    SEV_DECL virtual void stop();

    SEV_DECL void setEventHandler(
        const Event::Id& id, const EventHandler& handler);

    template<typename UserEventType>
    SEV_DECL void setUserEventHandler(
        const typename UserEventType::Handler& handler)
    {
        setEventHandler(
            UserEventType::getId(), [=](const Event* event) {
                handler(dynamic_cast<const UserEventType*>(event));
            });
    }

    SEV_DECL bool post(Event* event);
    SEV_DECL bool post(const Event::Id& id);
    SEV_DECL bool post(const std::function<void()>& task);

    SEV_DECL void setChildFinishedHandler(
        const ChildFinishedHandler& handler);

    SEV_DECL std::list<Thread*>& getChilds()
    {
        return mChilds;
    }

    SEV_DECL static Thread* getCurrent();

public:
    SEV_DECL void setName(const std::string& name)
    {
        mName = name;
    }

    SEV_DECL const std::string& getName() const
    {
        return mName;
    }

    SEV_DECL const Id& getId() const
    {
        return mId;
    }

    SEV_DECL const Handle& getHandle() const
    {
        return mHandle;
    }

    SEV_DECL Thread* getParent() const
    {
        return mParent;
    }

    SEV_DECL int32_t getExitCode() const
    {
        return mExitCode;
    }

    SEV_DECL EventLoop::Status getStatus() const
    {
        return mEventLoop.getStatus();
    }

    SEV_DECL uint32_t getQueuedEventCount() const;

public:
    SEV_DECL void setEventController(EventController* eventController)
    {
        mEventLoop.setController(eventController);
    }

    SEV_DECL EventController* getEventController()
    {
        return mEventLoop.getController();
    }

    SEV_DECL const EventController* getEventController() const
    {
        return mEventLoop.getController();
    }

protected:
    SEV_DECL virtual bool onInit();
    SEV_DECL virtual void onExit();

    SEV_DECL void setExitCode(int32_t exitCode)
    {
        mExitCode = exitCode;
    }

private:
    static SEV_TLS Thread* gThread;
    SEV_DECL static int32_t SEV_THREAD main(void* param);

    SEV_DECL void childFinished(Thread* child);
    SEV_DECL void onChildFinished(const Event* event);
    SEV_DECL void onTaskEvent(const Event* event);

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Id mId;
    Handle mHandle;
    std::thread mThread;
    std::string mName;

    Thread* mParent;
    std::list<Thread*> mChilds;
    ChildFinishedHandler mChildFinishedHandler;

    EventLoop mEventLoop;

    bool mInitResult;
    int32_t mExitCode;

    friend class Application;
    friend class Timer;
};

SEV_NS_END

#endif // SUBEVENT_THREAD_HPP
