#ifndef SUBEVENT_STD_HPP
#define SUBEVENT_STD_HPP

#include <cstdint>

//----------------------------------------------------------------------------//
// Macro
//----------------------------------------------------------------------------//

#ifdef _WIN32
#define SEV_THREAD __stdcall
#else
#define SEV_THREAD
#endif

#ifdef __APPLE__
#define SEV_TLS __thread
#else
#define SEV_TLS thread_local
#endif

// namespace
#define SEV_NS_BEGIN namespace subevent {
#define SEV_NS_END   }
#define SEV_USING_NS using namespace subevent;

#define SEV_MFN(f) \
    (std::bind(&f, this, std::placeholders::_1))

#ifdef SEV_HEADER_ONLY
#define SEV_DECL inline
#define SEV_IMPL_GLOBALS \
    SEV_NS_BEGIN \
    SEV_TLS Thread* Thread::gThread = nullptr; \
    Application* Application::gApp = nullptr; \
    SEV_NS_END
#else
#define SEV_DECL
#define SEV_IMPL_GLOBALS
#endif

//----------------------------------------------------------------------------//
// Type
//----------------------------------------------------------------------------//

SEV_NS_BEGIN

#ifdef _WIN32
namespace Win
{
    typedef void* Handle;
}
#endif

enum class WaitResult
{
    Success,
    Error,
    Timeout,
    Cancel
};

SEV_NS_END

#endif // SUBEVENT_STD_HPP
