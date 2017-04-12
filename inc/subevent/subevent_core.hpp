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
#include <subevent/buffer_stream.hpp>
#include <subevent/utility.hpp>

#ifdef SEV_HEADER_ONLY
#include <subevent_impl/application.inl>
#include <subevent_impl/thread.inl>
#include <subevent_impl/event.inl>
#include <subevent_impl/timer.inl>
#include <subevent_impl/semaphore.inl>
#include <subevent_impl/event_controller.inl>
#include <subevent_impl/event_loop.inl>
#include <subevent_impl/timer_manager.inl>
#include <subevent_impl/utility.inl>
#endif

#endif // SUBEVENT_CORE_HEADERS_HPP
