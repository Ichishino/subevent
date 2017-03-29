#ifndef SUBEVENT_COMMON_HPP
#define SUBEVENT_COMMON_HPP

#include <atomic>
#include <memory>

#include <subevent/std.hpp>
#include <subevent/event.hpp>
#include <subevent/thread.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Internal Event
//---------------------------------------------------------------------------//

typedef UserEvent<0xFA000001> StopEvent;
typedef UserEvent<0xFA000002, std::function<void()>> TaskEvent;
typedef UserEvent<0xFA000003, Thread*> ChildFinishedEvent;

//---------------------------------------------------------------------------//
// Cancelable Task
//---------------------------------------------------------------------------//

class TaskCanceller
{
public:
    SEV_DECL TaskCanceller()
    {
        reset();
    }

    SEV_DECL ~TaskCanceller()
    {
    }

public:
    SEV_DECL void reset()
    {
        mExecuted = false;
        mCanceled = false;
    }

    SEV_DECL void cancel()
    {
        mCanceled = true;
    }

    SEV_DECL bool isCanceled() const
    {
        return mCanceled;
    }

    SEV_DECL void executed()
    {
        mExecuted = true;
    }

    SEV_DECL bool isExecuted() const
    {
        return mExecuted;
    }

private:
    std::atomic<bool> mExecuted;
    std::atomic<bool> mCanceled;
};

typedef std::shared_ptr<TaskCanceller> TaskCancellerPtr;

inline TaskCancellerPtr postCancelableTask(
    Thread* thread, const std::function<void()>& task)
{
    TaskCancellerPtr canceller(new TaskCanceller());

    bool result = thread->post([task, canceller]() {

        if (!canceller->isCanceled())
        {
            canceller->executed();

            task();
        }
    });

    if (!result)
    {
        canceller.reset();
    }

    return canceller;
}

SEV_NS_END

#endif // SUBEVENT_COMMON_HPP
