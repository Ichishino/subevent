#ifndef SUBEVENT_CORE_HEADERS_HPP
#define SUBEVENT_CORE_HEADERS_HPP

//----------------------------------------------------------------------------//
// Headers
//----------------------------------------------------------------------------//

#include <subevent/std.hpp>
#include <subevent/thread.hpp>
#include <subevent/application.hpp>
#include <subevent/event.hpp>
#include <subevent/timer.hpp>
#include <subevent/byte_io.hpp>
#include <subevent/string_io.hpp>
#include <subevent/utility.hpp>
#include <subevent/variadic.hpp>
#include <subevent/crypto.hpp>

#ifdef SEV_HEADER_ONLY
#include <subevent/application.inl>
#include <subevent/thread.inl>
#include <subevent/event.inl>
#include <subevent/timer.inl>
#include <subevent/semaphore.inl>
#include <subevent/event_controller.inl>
#include <subevent/event_loop.inl>
#include <subevent/timer_manager.inl>
#include <subevent/utility.inl>
#include <subevent/crypto.inl>
#endif

#endif // SUBEVENT_CORE_HEADERS_HPP
