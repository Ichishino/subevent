#ifndef SUBEVENT_EVENT_LOOP_INL
#define SUBEVENT_EVENT_LOOP_INL

#include <algorithm>
#include <iterator>

#include <subevent/event_loop.hpp>
#include <subevent/event_controller.hpp>
#include <subevent/common.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// EventLoop
//----------------------------------------------------------------------------//

EventLoop::EventLoop()
{
    mStopPosted = false;

    mController = new EventController();
    mStatus.store(Status::Init);

    setHandler(CommEventId::Timer, SEV_MFN(EventLoop::onTimerEvent));
}

EventLoop::~EventLoop()
{
    delete mController;
}

void EventLoop::onExit()
{
    mController->onExit();
}

void EventLoop::setHandler(const Event::Id& id,
	                       const EventHandler& handler)
{
    mHandlerMap[id] = handler;
}

void EventLoop::removeHandler(const Event::Id& id)
{
    mHandlerMap.erase(id);
}

bool EventLoop::push(Event* event)
{
    if (mStopPosted)
    {
        delete event;
        return false;
    }

    mController->push(event);
    
    return true;
}

bool EventLoop::dispatch(Event* event)
{
    Event::Id id = event->getId();

    auto it = mHandlerMap.find(id);
    if (it != mHandlerMap.end())
    {
        it->second(event);
    }
    else
    {
        // unknown event
    }

    delete event;

    if (id == CommEventId::Stop)
    {
        // thread end
        return false;
    }

    return true;
}

bool EventLoop::run()
{
    bool result = true;

    for (;;)
    {
        mStatus.store(Status::Waiting);

        Event* event = nullptr;
        WaitResult waitResult;

        uint32_t msec = mTimerManager.nextTimeout();
            
        if (msec > 0)
        {
            // wait
            waitResult = mController->wait(msec, event);
        }
        else
        {
            waitResult = WaitResult::Timeout;
        }
        
        mStatus.store(Status::Running);

        if (waitResult == WaitResult::Success)
        {
            if (event != nullptr)
            {
                if (!dispatch(event))
                {
                    break;
                }
            }
        }
        else if (waitResult == WaitResult::Timeout)
        {
            std::list<Event*> events;
            mTimerManager.checkExpired(events);

            mController->push(events);
        }
        else if (waitResult == WaitResult::Cancel)
        {
            // nop
        }
        else
        {
            result = false;
            break;
        }
    }

    mStatus.store(Status::Exit);

    return result;
}

void EventLoop::stop()
{
    push(new Event(CommEventId::Stop));
}

void EventLoop::startTimer(Timer* timer)
{
    mTimerManager.start(timer);
    mController->wakeup();
}

void EventLoop::cancelTimer(Timer* timer)
{
    mTimerManager.cancel(timer);
    mController->wakeup();
}

void EventLoop::cancelAllTimer()
{
    mTimerManager.cancelAll();
    mController->wakeup();
}

void EventLoop::onTimerEvent(const Event* event)
{
    mTimerManager.onEvent(event);
}

EventController* EventLoop::getController()
{
    return mController;
}

void EventLoop::setController(EventController* controller)
{
    delete mController;
    mController = controller;
}

uint32_t EventLoop::getQueuedEventCount() const
{
    return mController->getQueuedEventCount();
}

SEV_NS_END

#endif // SUBEVENT_EVENT_LOOP_INL
