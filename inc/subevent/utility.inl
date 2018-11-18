#ifndef SUBEVENT_UTILITY_INL
#define SUBEVENT_UTILITY_INL

#include <random>

#include <subevent/utility.hpp>
#include <subevent/thread.hpp>

#ifdef SEV_OS_WIN
#include <windows.h>
#elif defined(SEV_OS_MAC)
#elif defined(SEV_OS_LINUX)
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
#ifdef SEV_OS_WIN
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return static_cast<uint16_t>(si.dwNumberOfProcessors);
#elif defined(SEV_OS_MAC)
        // TODO
        return 1;
#elif defined(SEV_OS_LINUX)
        return sysconf(_SC_NPROCESSORS_CONF);
#endif
    }

    bool bind(Thread* thread, uint16_t cpu)
    {
#ifdef SEV_OS_WIN
        return (SetThreadIdealProcessor(
            thread->getHandle(), cpu) != -1);
#elif defined(SEV_OS_MAC)
        // TODO
        return true;
#elif defined(SEV_OS_LINUX)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);

        return (pthread_setaffinity_np(
            thread->getHandle(), sizeof(cpu_set_t), &cpuset) == 0);
#endif
    }
}

//----------------------------------------------------------------------------//
// Random
//----------------------------------------------------------------------------//

namespace Random
{
    uint32_t generate32()
    {
        std::random_device seed;
        std::mt19937 mt(seed());

        return mt();
    }

    std::vector<unsigned char> generateBytes(size_t length)
    {
        std::vector<unsigned char> result(length);

        for (size_t index = 0; index < length; ++index)
        {
            result[index] = (unsigned char)(generate32() % 256);
        }

        return result;
    }
}

SEV_NS_END

#endif // SUBEVENT_UTILITY_INL
