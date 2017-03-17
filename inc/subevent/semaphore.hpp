#ifndef SUBEVENT_SEMAPHORE_HPP
#define SUBEVENT_SEMAPHORE_HPP

#include <subevent/std.hpp>

#if defined __APPLE__
#include <dispatch/dispatch.h>
#elif defined _WIN32
#else
#include <semaphore.h>
#endif

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Semaphore
//----------------------------------------------------------------------------//

class Semaphore
{
public:
    SEV_DECL explicit Semaphore(uint32_t count = 0);
    SEV_DECL ~Semaphore();

#if defined __APPLE__
    typedef dispatch_semaphore_t Handle;
#elif defined _WIN32
    typedef Win::Handle Handle;
#else
    typedef sem_t Handle;
#endif

public:
    SEV_DECL WaitResult wait(uint32_t msec = UINT32_MAX);
    SEV_DECL void post();

    SEV_DECL Handle getHandle()
    {
        return mSem;
    }

private:
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Handle mSem;
};

SEV_NS_END

#endif // SUBEVENT_SEMAPHORE_HPP
