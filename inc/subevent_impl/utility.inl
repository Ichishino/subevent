#ifndef SUBEVENT_UTILITY_INL
#define SUBEVENT_UTILITY_INL

#include <subevent/utility.hpp>
#include <subevent/thread.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Processor
//----------------------------------------------------------------------------//

namespace Processor
{
    uint16_t getCount()
    {
#ifdef _WIN32
        SYSTEM_INFO si;
        GetSystemInfo(&si);

        return static_cast<uint16_t>(si.dwNumberOfProcessors);
#else
        return sysconf(_SC_NPROCESSORS_CONF);
#endif
    }

    bool bind(Thread* thread, uint16_t cpu)
    {
#ifdef _WIN32
        return (SetThreadIdealProcessor(
            thread->getHandle(), cpu) != -1);
#else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);

        return (pthread_setaffinity_np(
            thread->getHandle(), sizeof(cpu_set_t), &cpuset) == 0);
#endif
    }
}

SEV_NS_END

#endif // SUBEVENT_UTILITY_INL
