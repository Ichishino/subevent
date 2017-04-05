#ifndef SUBEVENT_EVENT_CONTROLLER_HPP
#define SUBEVENT_EVENT_CONTROLLER_HPP

#include <queue>
#include <mutex>

#include <subevent/std.hpp>
#include <subevent/semaphore.hpp>

SEV_NS_BEGIN

class Event;

//----------------------------------------------------------------------------//
// EventController
//----------------------------------------------------------------------------//

class EventController
{
public:
    SEV_DECL EventController();
    SEV_DECL virtual ~EventController();

public:
    SEV_DECL bool push(Event* event);
    SEV_DECL void clear();
    SEV_DECL uint32_t getQueuedEventCount() const;

public:
    SEV_DECL virtual WaitResult wait(uint32_t msec, Event*& event);
    SEV_DECL virtual void wakeup();
    SEV_DECL virtual bool onInit();
    SEV_DECL virtual void onExit();

protected:
    SEV_DECL Event* pop();

private:
    mutable std::mutex mMutex;
    std::queue<Event*> mQueue;
    bool mStopPosted;
    Semaphore mSem;
};

SEV_NS_END

#endif // SUBEVENT_EVENT_CONTROLLER_HPP
