#ifndef SUBEVENT_EVENT_HPP
#define SUBEVENT_EVENT_HPP

#include <functional>
#include <tuple>

#include <subevent/std.hpp>
#include <subevent/variadic.hpp>

SEV_NS_BEGIN

class Event;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::function<void(const Event*)> EventHandler;

//----------------------------------------------------------------------------//
// Event
//----------------------------------------------------------------------------//

class Event
{
public:
    typedef uint32_t Id;

    SEV_DECL explicit Event(const Id& id);
    SEV_DECL virtual ~Event();

public:
    SEV_DECL const Id& getId() const
    {
        return mId;
    }

    SEV_DECL void setId(const Id& id)
    {
        mId = id;
    }

private:
    Id mId;
};

//----------------------------------------------------------------------------//
// UserEvent
//----------------------------------------------------------------------------//

template<Event::Id userEventId, typename ... ParamTypes>
class UserEvent : public Event, public Variadic<ParamTypes ...>
{
public:
    static constexpr Event::Id getId() { return userEventId; }

    typedef std::function<
        void(const UserEvent<userEventId, ParamTypes ...>*)> Handler;

    explicit UserEvent(const ParamTypes& ... params)
        : Event(userEventId), Variadic<ParamTypes ...>(params ...)
    {
    }
};

SEV_NS_END

#endif // SUBEVENT_EVENT_HPP
