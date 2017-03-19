#ifndef SUBEVENT_TIMER_MANAGER_HPP
#define SUBEVENT_TIMER_MANAGER_HPP

#include <set>
#include <list>
#include <chrono>

#include <subevent/std.hpp>

SEV_NS_BEGIN

class Timer;
class Event;

//----------------------------------------------------------------------------//
// TimerManager
//----------------------------------------------------------------------------//

class TimerManager
{
public:
    SEV_DECL TimerManager();
    SEV_DECL ~TimerManager();

public:
    SEV_DECL void start(Timer* timer);
    SEV_DECL void cancel(Timer* timer);
    SEV_DECL void cancelAll();

    SEV_DECL uint32_t nextTimeout();
    SEV_DECL void expire();

private:

    struct Item
    {
        Timer* timer;
        std::chrono::system_clock::time_point end;
    };

    std::list<Item> mItems;
    std::set<Timer*> mExpired;
};

SEV_NS_END

#endif // SUBEVENT_TIMER_MANAGER_HPP
