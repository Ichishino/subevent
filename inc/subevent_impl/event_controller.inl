#ifndef SUBEVENT_EVENT_CONTROLLER_INL
#define SUBEVENT_EVENT_CONTROLLER_INL

#include <subevent/event_controller.hpp>
#include <subevent/event.hpp>
#include <subevent/common.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// EventController
//----------------------------------------------------------------------------//

EventController::EventController()
{
    mStopPosted = false;
}

EventController::~EventController()
{
    clear();
}

void EventController::clear()
{
    std::lock_guard<std::mutex> lock(mMutex);

    while (!mQueue.empty())
    {
        Event* event = mQueue.front();
        mQueue.pop();
        delete event;
    }

    mStopPosted = false;
}

bool EventController::push(Event* event)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (mStopPosted)
    {
        delete event;
        return false;
    }
    else if (event->getId() == StopEvent::getId())
    {
        mStopPosted = true;
    }

    mQueue.push(event);
    wakeup();

    return true;
}

Event* EventController::pop()
{
    std::lock_guard<std::mutex> lock(mMutex);

    Event* event = nullptr;

    if (!mQueue.empty())
    {
        event = mQueue.front();
        mQueue.pop();
    }

    return event;
}

uint32_t EventController::getQueuedEventCount() const
{
    std::lock_guard<std::mutex> lock(mMutex);

    return static_cast<uint32_t>(mQueue.size());
}

WaitResult EventController::wait(uint32_t msec, Event*& event)
{
    WaitResult result = mSem.wait(msec);

    switch (result)
    {
    case WaitResult::Success:
        event = pop();
        break;
    case WaitResult::Timeout:
        break;
    case WaitResult::Cancel:
        break;
    case WaitResult::Error:
        break;
    }

    return result;
}

void EventController::wakeup()
{
    mSem.post();
}

bool EventController::onInit()
{
    return true;
}

void EventController::onExit()
{
    clear();
}

SEV_NS_END

#endif // SUBEVENT_EVENT_CONTROLLER_INL
