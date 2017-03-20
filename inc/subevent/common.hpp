#ifndef SUBEVENT_COMMON_HPP
#define SUBEVENT_COMMON_HPP

#include <subevent/std.hpp>
#include <subevent/event.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

namespace CommEventId
{
    static const Event::Id Stop = 0xFA000001;
    static const Event::Id Task = 0xFA000002;
    static const Event::Id ChildFinished = 0xFA000003;
}

SEV_NS_END

#endif // SUBEVENT_COMMON_HPP
