#ifndef SUBEVENT_EVENT_HPP
#define SUBEVENT_EVENT_HPP

#include <functional>
#include <tuple>

#include <subevent/std.hpp>

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
class UserEvent : public Event
{
public:
    static constexpr Event::Id getId() { return userEventId; }
    typedef std::function<
        void(const UserEvent<userEventId, ParamTypes ...>*)> Handler;

    explicit UserEvent(const ParamTypes& ... params)
        : Event(userEventId)
    {
        setParams(params ...);
    }

public:
    void setParams(const ParamTypes& ... params)
    {
        mParams = std::make_tuple(params ...);
    }

    void getParams(ParamTypes& ... params) const
    {
        std::tie(params ...) = mParams;
    }

    template<size_t index, typename ParamType>
    void setParam(const ParamType& param)
    {
        std::get<index>(mParams) = param;
    }

#if (__cplusplus >= 201402L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
    template<size_t index>
    const auto& getParam() const
    {
        return std::get<index>(mParams);
    }
#endif

private:
    std::tuple<ParamTypes ...> mParams;
};

SEV_NS_END

#endif // SUBEVENT_EVENT_HPP
