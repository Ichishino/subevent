#ifndef SUBEVENT_EVENT_CONTROLLER_INL
#define SUBEVENT_EVENT_CONTROLLER_INL

#include <subevent/event_controller.hpp>
#include <subevent/event.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// EventController
//----------------------------------------------------------------------------//

EventController::EventController()
{
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
}

void EventController::push(Event* event)
{
    std::lock_guard<std::mutex> lock(mMutex);

    mQueue.push(event);
    wakeup();
}

void EventController::push(const std::list<Event*>& events)
{
    std::lock_guard<std::mutex> lock(mMutex);

    for (Event* event : events)
    {
        mQueue.push(event);
        wakeup();
    }
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

void EventController::onExit()
{
    clear();
}

SEV_NS_END

#endif // SUBEVENT_EVENT_CONTROLLER_INL
