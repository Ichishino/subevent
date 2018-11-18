#ifndef SUBEVENT_STD_HPP
#define SUBEVENT_STD_HPP

#include <cstdint>

#ifdef SUBEVENT_DEBUG
#include <subevent/debug.hpp>
#endif

//----------------------------------------------------------------------------//
// Macro
//----------------------------------------------------------------------------//

// TODO
#if (__cplusplus >= 201703L) || (defined(_MSC_VER) && _MSC_VER >= 1911)
#   define SEV_CPP_VER    17
#elif (__cplusplus >= 201402L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
#   define SEV_CPP_VER    14
#else
#   define SEV_CPP_VER    11
#endif

// TODO
#if defined(_WIN32) && defined(_MSC_VER)
#   define SEV_OS_WIN
#elif defined(linux) || defined(__linux__)
#   define SEV_OS_LINUX
#elif defined(__APPLE__)
#   define SEV_OS_MAC
#else
#endif

// TODO
#ifdef SEV_OS_MAC
#   include <machine/endian.h>
#elif defined(__GNUC__)
#   include <endian.h>
#endif
#ifdef __BYTE_ORDER
#   if defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#       define SEV_LITTLE_ENDIAN
#   endif
#elif defined(_BYTE_ORDER)
#   if defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN)
#       define SEV_LITTLE_ENDIAN
#   endif
#elif defined(__BYTE_ORDER__)
#   if defined(__ORDER_LITTLE_ENDIAN__) && \
              (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#       define SEV_LITTLE_ENDIAN
#   endif
#elif defined(__LITTLE_ENDIAN__) || \
      defined(__ARMEL__) ||  \
      defined(__THUMBEL__) ||  \
      defined(__AARCH64EL__) ||  \
      defined(_MIPSEL) ||  \
      defined(__MIPSEL) ||  \
      defined(__MIPSEL__)
#   define SEV_LITTLE_ENDIAN
#elif defined(SEV_OS_WIN)
#   define SEV_LITTLE_ENDIAN
#endif

#ifdef SEV_OS_WIN
#   define SEV_THREAD __stdcall
#else
#   define SEV_THREAD
#endif

#ifdef SEV_OS_MAC
#   define SEV_TLS __thread
#else
#   define SEV_TLS thread_local
#endif

// namespace
#define SEV_NS subevent
#define SEV_NS_BEGIN namespace SEV_NS {
#define SEV_NS_END   }
#define SEV_USING_NS using namespace SEV_NS;

#ifdef SEV_HEADER_ONLY
#   define SEV_DECL inline
#   define SEV_IMPL_GLOBAL \
        SEV_TLS SEV_NS::Thread* SEV_NS::Thread::gThread = nullptr; \
        SEV_NS::Application* SEV_NS::Application::gApp = nullptr;
#else
#   define SEV_DECL
#   define SEV_IMPL_GLOBAL
#endif

#define SEV_BIND_1(p, f) \
    (std::bind(&f, p, std::placeholders::_1))
#define SEV_BIND_2(p, f) \
    (std::bind(&f, p, std::placeholders::_1, std::placeholders::_2))

//----------------------------------------------------------------------------//
// Type
//----------------------------------------------------------------------------//

SEV_NS_BEGIN

#ifdef SEV_OS_WIN
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
