#ifndef SUBEVENT_TIMER_INL
#define SUBEVENT_TIMER_INL

#include <cassert>

#include <subevent/timer.hpp>
#include <subevent/thread.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Timer
//----------------------------------------------------------------------------//

Timer::Timer()
{
    mInterval = 0;
    mRepeat = false;
    mRunning = false;
}

Timer::~Timer()
{
    cancel();
}

void Timer::start(
    uint32_t msec, bool repeat, const TimerHandler& timerHandler)
{
    assert(Thread::getCurrent() != nullptr);

    if (isRunning())
    {
        return;
    }

    mInterval = msec;
    mRepeat = repeat;
    mHandler = timerHandler;

    Thread::getCurrent()->mEventLoop.startTimer(this);
}

void Timer::cancel()
{
    if (!isRunning())
    {
        return;
    }

    if (Thread::getCurrent() != nullptr)
    {
        Thread::getCurrent()->mEventLoop.cancelTimer(this);
    }
}

SEV_NS_END

#endif // SUBEVENT_TIMER_INL
