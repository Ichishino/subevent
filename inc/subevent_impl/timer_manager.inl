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

    auto it = std::find_if(
        mItems.begin(), mItems.end(),
        [&](const Item& item) {
            return (item.timer == timer);
        });
    if (it != mItems.end())
    {
        mItems.erase(it);
        return;
    }

    mExpired.erase(timer);
}

void TimerManager::cancelAll()
{
    for (Item& item : mItems)
    {
        item.timer->mRunning = false;
    }
    mItems.clear();

    for (Timer* timer : mExpired)
    {
        timer->mRunning = false;
    }
    mExpired.clear();
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

void TimerManager::expire()
{
    auto now = std::chrono::system_clock::now();

    while (!mItems.empty())
    {
        Item& item = mItems.front();

        if (item.end > now)
        {
            break;
        }

        Timer* timer = item.timer;
        mItems.pop_front();

        if (!timer->isRunning())
        {
            continue;
        }

        mExpired.insert(timer);

        Thread::getCurrent()->post([this, timer]() {

            if (mExpired.erase(timer) == 0)
            {
                return;
            }

            if (!timer->isRunning())
            {
                return;
            }
            timer->mRunning = false;

            if (timer->isRepeat())
            {
                start(timer);
            }

            TimerHandler handler = timer->mHandler;
            handler(timer);
        });
    }
}

SEV_NS_END

#endif // SUBEVENT_TIMER_MANAGER_INL
