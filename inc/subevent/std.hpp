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
#define SEV_NS subevent
#define SEV_NS_BEGIN namespace SEV_NS {
#define SEV_NS_END   }
#define SEV_USING_NS using namespace SEV_NS;

#ifdef SEV_HEADER_ONLY
#define SEV_DECL inline
#define SEV_IMPL_GLOBAL \
    SEV_TLS SEV_NS::Thread* SEV_NS::Thread::gThread = nullptr; \
    SEV_NS::Application* SEV_NS::Application::gApp = nullptr;
#else
#define SEV_DECL
#define SEV_IMPL_GLOBAL
#endif

#define SEV_MFN1(f) \
    (std::bind(&f, this, std::placeholders::_1))
#define SEV_MFN2(f) \
    (std::bind(&f, this, std::placeholders::_1, std::placeholders::_2))

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
