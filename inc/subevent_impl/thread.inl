#ifndef SUBEVENT_THREAD_INL
#define SUBEVENT_THREAD_INL

#include <cassert>
#include <algorithm>

#include <subevent/thread.hpp>
#include <subevent/common.hpp>
#include <subevent/event.hpp>

#ifdef _WIN32
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

    mName = name;
    mParent = parent;

    if (mParent != nullptr)
    {
        mParent->mChilds.push_back(this);
    }

    setEventHandler(
        ChildFinishedEvent::Id(), SEV_MFN(Thread::onChildFinished));
    setEventHandler(
        TaskEvent::Id(), SEV_MFN(Thread::onTaskEvent));
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
    return true;
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

    if (mParent != nullptr)
    {
        if (mParent->getStatus() != EventLoop::Status::Exit)
        {
            mParent->post(new ChildFinishedEvent(this));
        }
    }
}

int32_t SEV_THREAD Thread::main(Thread* thread)
{
    assert(gThread == nullptr);

    gThread = thread;
    gThread->mId = std::this_thread::get_id();
#ifdef _WIN32
    gThread->mHandle = GetCurrentThread();
#else
    gThread->mHandle = pthread_self();
#endif

    return gThread->onRun();
}

void Thread::start()
{
    mThread = std::thread(Thread::main, this);
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

int32_t Thread::onRun()
{
    if (onInit())
    {
        // run
        if (!mEventLoop.run())
        {
            mExitCode = -1;
        }
    }
    else
    {
        mExitCode = -2;
    }

    onExit();

    mEventLoop.onExit();

    return mExitCode;
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
