#ifndef SUBEVENT_EVENT_INL
#define SUBEVENT_EVENT_INL

#include <subevent/event.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Event
//----------------------------------------------------------------------------//

Event::Event(const Id& id)
    : mId(id)
{
}

Event::~Event()
{
}

SEV_NS_END

#endif // SUBEVENT_EVENT_INL
