#ifndef SUBEVENT_TIMER_HPP
#define SUBEVENT_TIMER_HPP

#include <functional>

#include <subevent/std.hpp>

SEV_NS_BEGIN

class Timer;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::function<void(Timer*)> TimerHandler;

//----------------------------------------------------------------------------//
// Timer
//----------------------------------------------------------------------------//

class Timer
{
public:
    SEV_DECL Timer();
    SEV_DECL ~Timer();

public:
    SEV_DECL void start(
        uint32_t msec, bool repeat, const TimerHandler& timerHandler);
    SEV_DECL void cancel();

public:
    SEV_DECL uint32_t getInterval() const
    {
        return mInterval;
    }

    SEV_DECL bool isRepeat() const
    {
        return mRepeat;
    }

    SEV_DECL bool isRunning() const
    {
        return mRunning;
    }

private:
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    uint32_t mInterval;
    bool mRepeat;
    TimerHandler mHandler;
    bool mRunning;

    friend class TimerManager;
};

SEV_NS_END

#endif // SUBEVENT_TIMER_HPP
