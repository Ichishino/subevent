#ifndef SUBEVENT_COMMON_HPP
#define SUBEVENT_COMMON_HPP

#include <subevent/std.hpp>
#include <subevent/event.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Internal Event
//---------------------------------------------------------------------------//

typedef UserEvent<0xFA000001> StopEvent;
typedef UserEvent<0xFA000002, std::function<void()>> TaskEvent;
typedef UserEvent<0xFA000003, Thread*> ChildFinishedEvent;

SEV_NS_END

#endif // SUBEVENT_COMMON_HPP
