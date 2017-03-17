#ifndef SUBEVENT_TIMER_MANAGER_INL
#define SUBEVENT_TIMER_MANAGER_INL

#include <algorithm>
#include <functional>

#include <subevent/timer_manager.hpp>
#include <subevent/common.hpp>
#include <subevent/timer.hpp>
#include <subevent/thread.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

typedef UserEvent<CommEventId::Timer, Timer*> TimerEvent;
typedef std::function<void(const TimerEvent*)> TimerEventHandler;

//----------------------------------------------------------------------------//
// TimerManager
//----------------------------------------------------------------------------//

TimerManager::TimerManager()
{
}

TimerManager::~TimerManager()
{
    cancelAll();
}

void TimerManager::start(Timer* timer)
{
    timer->mRunning = true;

    Item item;
    item.end = std::chrono::system_clock::now() +
        std::chrono::milliseconds(timer->getInterval());
    item.timer = timer;
    item.event = nullptr;

    for (auto it = mItems.begin(); it != mItems.end(); ++it)
    {
        if (it->end > item.end)
        {
            mItems.insert(it, std::move(item));
            return;
        }
    }
    mItems.push_back(std::move(item));
}

void TimerManager::cancel(Timer* timer)
{
    timer->mRunning = false;

    auto pred = [&](const Item& item)
    {
        return (item.timer == timer);
    };

    auto it1 = std::find_if(
        mItems.begin(), mItems.end(), pred);
    if (it1 != mItems.end())
    {
        mItems.erase(it1);
        return;
    }

    auto it2 = std::find_if(
        mExpiredItems.begin(), mExpiredItems.end(), pred);
    if (it2 != mExpiredItems.end())
    {
        mExpiredItems.erase(it2);
        return;
    }
}

void TimerManager::cancelAll()
{
    while (!mItems.empty())
    {
        Item& item = mItems.front();
        item.timer->mRunning = false;
        mItems.pop_front();
    }

    while (!mExpiredItems.empty())
    {
        Item& item = mExpiredItems.front();
        item.timer->mRunning = false;
        mExpiredItems.pop_front();
    }
}

uint32_t TimerManager::nextTimeout()
{
    if (mItems.empty())
    {
        return UINT32_MAX;
    }
    else
    {
        auto now = std::chrono::system_clock::now();
        if (mItems.front().end <= now)
        {
            return 0;
        }

        auto remain = mItems.front().end - now;
        return static_cast<uint32_t>(std::chrono::duration_cast<
            std::chrono::milliseconds>(remain).count());
    }
}

void TimerManager::checkExpired(std::list<Event*>& events)
{
    auto now = std::chrono::system_clock::now();

    for (auto it = mItems.begin(); it != mItems.end();)
    {
        Item& item = *it;
        if (item.end <= now)
        {
            item.event = new TimerEvent(item.timer);
            events.push_back(item.event);

            mExpiredItems.push_back(std::move(item));

            it = mItems.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TimerManager::onEvent(const Event* event)
{
    const TimerEvent* timerEvent =
        dynamic_cast<const TimerEvent*>(event);

    Timer* timer;
    timerEvent->getParams(timer);

    for (auto it = mExpiredItems.begin();
        it != mExpiredItems.end(); ++it)
    {
        if (it->event != event)
        {
            continue;
        }
        it = mExpiredItems.erase(it);

        if (timer->isRunning())
        {
            timer->mRunning = false;

            if (timer->isRepeat())
            {
                start(timer);
            }

            TimerHandler handler = timer->mHandler;

            Thread::getCurrent()->post([handler, timer]() {
                handler(timer);
            });
        }

        break;
    }
}

SEV_NS_END

#endif // SUBEVENT_TIMER_MANAGER_INL
