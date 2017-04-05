#ifndef SUBEVENT_EVENT_LOOP_HPP
#define SUBEVENT_EVENT_LOOP_HPP

#include <map>
#include <atomic>

#include <subevent/std.hpp>
#include <subevent/event.hpp>
#include <subevent/timer_manager.hpp>

SEV_NS_BEGIN

class EventController;

//----------------------------------------------------------------------------//
// EventLoop
//----------------------------------------------------------------------------//

class EventLoop
{
public:
    SEV_DECL EventLoop();
    SEV_DECL ~EventLoop();

    enum class Status
    {
        Init, Running, Waiting, Exit
    };

public:
    SEV_DECL bool push(Event* event);

    // Event Handler
    SEV_DECL void setHandler(
        const Event::Id& id, const EventHandler& handler);
    SEV_DECL void removeHandler(
        const Event::Id& id);

    // Timer
    SEV_DECL void startTimer(Timer* timer);
    SEV_DECL void cancelTimer(Timer* timer);
    SEV_DECL void cancelAllTimer();

    SEV_DECL bool run();
    SEV_DECL void stop();

    SEV_DECL Status getStatus() const
    {
        return mStatus.load();
    }

    SEV_DECL EventController* getController();
    SEV_DECL const EventController* getController() const;
    SEV_DECL void setController(EventController* controller);

    SEV_DECL bool onInit();
    SEV_DECL void onExit();

private:
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    SEV_DECL bool dispatch(Event* event);

    EventController* mController;

    TimerManager mTimerManager;
    std::atomic<Status> mStatus;
    std::map<Event::Id, EventHandler> mHandlerMap;
};

SEV_NS_END

#endif // SUBEVENT_EVENT_LOOP_HPP
