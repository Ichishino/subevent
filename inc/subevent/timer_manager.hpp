#ifndef SUBEVENT_TIMER_MANAGER_HPP
#define SUBEVENT_TIMER_MANAGER_HPP

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
    SEV_DECL void checkExpired(std::list<Event*>& events);
    SEV_DECL void onEvent(const Event* event);

private:

    struct Item
    {
        Timer* timer;
        std::chrono::system_clock::time_point end;
        Event* event;
    };

    std::list<Item> mItems;
    std::list<Item> mExpiredItems;
};

SEV_NS_END

#endif // SUBEVENT_TIMER_MANAGER_HPP
