#ifndef SUBEVENT_UTILITY_HPP
#define SUBEVENT_UTILITY_HPP

#include <subevent/std.hpp>

SEV_NS_BEGIN

class Thread;

//----------------------------------------------------------------------------//
// Processor
//----------------------------------------------------------------------------//

namespace Processor
{
    SEV_DECL uint16_t getCount();

    SEV_DECL bool bind(
        Thread* thread, uint16_t cpu /* starting from 0 */);
}

SEV_NS_END

#endif // SUBEVENT_UTILITY_HPP
