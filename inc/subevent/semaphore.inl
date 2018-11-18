#ifndef SUBEVENT_SEMAPHORE_INL
#define SUBEVENT_SEMAPHORE_INL

#include <subevent/semaphore.hpp>

#ifdef SEV_OS_WIN
#include <windows.h>
#elif defined(SEV_OS_MAC)
#elif defined(SEV_OS_LINUX)
#include <errno.h>
#include <time.h>
#endif

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Semaphore
//----------------------------------------------------------------------------//

Semaphore::Semaphore(uint32_t count)
{
#ifdef SEV_OS_WIN
    mSem = CreateSemaphore(NULL, count, LONG_MAX, NULL);
#elif defined(SEV_OS_MAC)
    mSem = dispatch_semaphore_create(count);
#elif defined(SEV_OS_LINUX)
    sem_init(&mSem, 0, count);
#endif
}

Semaphore::~Semaphore()
{
#ifdef SEV_OS_WIN
    CloseHandle(mSem);
#elif defined(SEV_OS_MAC)
    dispatch_release(mSem);
#elif defined(SEV_OS_LINUX)
    sem_destroy(&mSem);
#endif
}

WaitResult Semaphore::wait(uint32_t msec)
{
#ifdef SEV_OS_WIN

    DWORD result = WaitForSingleObject(mSem, msec);

    if (result == WAIT_OBJECT_0)
    {
        return WaitResult::Success;
    }
    else if (result == WAIT_TIMEOUT)
    {
        return WaitResult::Timeout;
    }

    return WaitResult::Error;

#elif defined(SEV_OS_MAC)

    dispatch_time_t timeout =
        dispatch_time(DISPATCH_TIME_NOW, msec * NSEC_PER_MSEC);

    return (dispatch_semaphore_wait(mSem, timeout) == 0) ?
        WaitResult::Success : WaitResult::Timeout;

#elif defined(SEV_OS_LINUX)

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_nsec += msec * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    int res;
    do
    {
        res = sem_timedwait(&mSem, &ts);
    }
    while (res == -1 && errno == EINTR);

    if (res == 0)
    {
        return WaitResult::Success;
    }
    else if (errno == ETIMEDOUT)
    {
        return WaitResult::Timeout;
    }
    else
    {
        return WaitResult::Error;
    }

#endif
}

void Semaphore::post()
{
#ifdef SEV_OS_WIN
    ReleaseSemaphore(mSem, 1, NULL);
#elif defined(SEV_OS_MAC)
    dispatch_semaphore_signal(mSem);
#elif defined(SEV_OS_LINUX)
    sem_post(&mSem);
#endif
}

SEV_NS_END

#endif // SUBEVENT_SEMAPHORE_INL
