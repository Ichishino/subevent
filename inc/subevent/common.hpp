#ifndef SUBEVENT_COMMON_HPP
#define SUBEVENT_COMMON_HPP

#include <subevent/std.hpp>
#include <subevent/event.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef UserEvent<0xFA000001> StopEvent;
typedef UserEvent<0xFA000002, Thread*> ChildFinishedEvent;
typedef UserEvent<0xFA000003, std::function<void()>> TaskEvent;

SEV_NS_END

#endif // SUBEVENT_COMMON_HPP
