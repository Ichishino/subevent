#ifndef SUBEVENT_THREAD_INL
#define SUBEVENT_THREAD_INL

#include <cassert>
#include <algorithm>

#include <subevent/thread.hpp>
#include <subevent/common.hpp>
#include <subevent/event.hpp>
#include <subevent/event_controller.hpp>
#include <subevent/semaphore.hpp>

#ifdef SEV_OS_WIN
#include <windows.h>
#endif

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Thread
//----------------------------------------------------------------------------//

#ifndef SEV_HEADER_ONLY
SEV_TLS Thread* Thread::gThread = nullptr;
#endif

Thread::Thread(Thread* parent)
    : Thread("", parent)
{
}

Thread::Thread(const std::string& name, Thread* parent)
{
    mHandle = 0;
    mExitCode = 0;

    setName(name);

    mParent = parent;
    mInitResult = false;

    if (mParent != nullptr)
    {
        mParent->mChilds.push_back(this);
    }

    setEventHandler(
        ChildFinishedEvent::getId(),
        SEV_BIND_1(this, Thread::onChildFinished));
    setEventHandler(
        TaskEvent::getId(),
        SEV_BIND_1(this, Thread::onTaskEvent));
}

Thread::~Thread()
{
    if (gThread == this)
    {
        gThread = nullptr;
    }
}

Thread* Thread::getCurrent()
{
    return gThread;
}

bool Thread::onInit()
{
    return mEventLoop.onInit();
}

void Thread::onExit()
{
    mEventLoop.cancelAllTimer();

    std::for_each(mChilds.rbegin(), mChilds.rend(),
        [](Thread* thread)
        {
            thread->stop();
        });

    while (!mChilds.empty())
    {
        Thread* child = mChilds.back();
        mChilds.pop_back();

        childFinished(child);
    }

    mEventLoop.onExit();

    if (mParent != nullptr)
    {
        if (mInitResult)
        {
            if (mParent->getStatus() != EventLoop::Status::Exit)
            {
                mParent->post(new ChildFinishedEvent(this));
            }
        }
        else
        {
            mParent->mChilds.remove(this);
            mParent = nullptr;
        }
    }
}

struct ThreadParam
{
    Thread* thread;
    Semaphore sem;
};

int32_t SEV_THREAD Thread::main(void* param)
{
    assert(gThread == nullptr);

    ThreadParam* threadParam =
        reinterpret_cast<ThreadParam*>(param);

    gThread = threadParam->thread;
    gThread->mId = std::this_thread::get_id();
#ifdef SEV_OS_WIN
    gThread->mHandle = GetCurrentThread();
#else
    gThread->mHandle = pthread_self();
#endif

    // init
    gThread->mInitResult = gThread->onInit();

    if (!gThread->mInitResult)
    {
        // error
        gThread->mExitCode = -11;

        gThread->onExit();
        threadParam->sem.post();

        // thread end
    }
    else
    {
        threadParam->sem.post();

        // run
        if (!gThread->mEventLoop.run())
        {
            gThread->mExitCode = -12;
        }

        // exit
        gThread->onExit();
    }

    return gThread->mExitCode;
}

bool Thread::start()
{
    ThreadParam threadParam;
    threadParam.thread = this;

    // thread start
    mThread = std::thread(Thread::main, &threadParam);

    threadParam.sem.wait();

    if (!mInitResult)
    {
        // init error

        wait();
    }

    return mInitResult;
}

void Thread::stop()
{
    mEventLoop.stop();
}

void Thread::wait()
{
    if (mThread.joinable())
    {
        mThread.join();
    }
}

void Thread::setEventHandler(
    const Event::Id& id, const EventHandler& handler)
{
    if (handler != nullptr)
    {
        mEventLoop.setHandler(id, handler);
    }
    else
    {
        mEventLoop.removeHandler(id);
    }
}

bool Thread::post(Event* event)
{
    return mEventLoop.push(event);
}

bool Thread::post(const Event::Id& id)
{
    return post(new Event(id));
}

bool Thread::post(const std::function<void()>& task)
{
    return post(new TaskEvent(task));
}

uint32_t Thread::getQueuedEventCount() const
{
    return getEventController()->getQueuedEventCount();
}

void Thread::setChildFinishedHandler(const ChildFinishedHandler& handler)
{
    mChildFinishedHandler = handler;
}

void Thread::childFinished(Thread* child)
{
    child->wait();

    if (mChildFinishedHandler != nullptr)
    {
        mChildFinishedHandler(child);
    }
    else
    {
        delete child;
    }
}

void Thread::onChildFinished(const Event* event)
{
    const ChildFinishedEvent* childFinishedEvent =
        dynamic_cast<const ChildFinishedEvent*>(event);

    Thread* child;
    childFinishedEvent->getParams(child);
    assert(this == child->mParent);

    mChilds.remove(child);

    childFinished(child);
}

void Thread::onTaskEvent(const Event* event)
{
    const TaskEvent* taskEvent =
        dynamic_cast<const TaskEvent*>(event);

    std::function<void()> task;
    taskEvent->getParams(task);

    task();
}

SEV_NS_END

#endif // SUBEVENT_THREAD_INL
