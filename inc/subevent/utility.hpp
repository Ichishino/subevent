#ifndef SUBEVENT_UTILITY_HPP
#define SUBEVENT_UTILITY_HPP

#include <list>
#include <vector>
#include <string>
#include <utility>
#include <cctype>

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

//---------------------------------------------------------------------------//
// Endian
//---------------------------------------------------------------------------//

namespace Endian
{
    template <typename ValueType>
    inline ValueType swapByteOrder(ValueType value)
    {
        union
        {
            ValueType v;
            uint8_t b[sizeof(ValueType)];
        } u;

        u.v = value;

        for (size_t index = 0; index < (sizeof(u) / 2); ++index)
        {
            std::swap(u.b[sizeof(u) - index - 1], u.b[index]);
        }

        return u.v;
    }

    template <typename ValueType>
    inline ValueType hostToNetwork(ValueType value)
    {
#ifdef SEV_LITTLE_ENDIAN
        return swapByteOrder(value);
#else
        return value;
#endif
    }

    template <typename ValueType>
    inline ValueType networkToHost(ValueType value)
    {
#ifdef SEV_LITTLE_ENDIAN
        return swapByteOrder(value);
#else
        return value;
#endif
    }
}

//----------------------------------------------------------------------------//
// String
//----------------------------------------------------------------------------//

namespace String
{
    inline void trimLeft(std::string& str, const char* ws = " \t\n\v\f\r")
    {
        str.erase(0, str.find_first_not_of(ws));
    }

    inline void trimRight(std::string& str, const char* ws = " \t\n\v\f\r")
    {
        str.erase(str.find_last_not_of(ws) + 1);
    }

    inline void trim(std::string& str, const char* ws = " \t\n\v\f\r")
    {
        trimRight(str, ws);
        trimLeft(str, ws);
    }

    inline bool iequals(const std::string& left, const std::string& right)
    {
        return ((left.size() == right.size()) &&
            std::equal(left.begin(), left.end(), right.begin(),
                [](char l, char r) ->bool {
            return (std::tolower(l) == std::tolower(r));
        }));
    }

    inline std::list<std::string>
        split(const std::string& src, const char* delm)
    {
        std::list<std::string> result;

        size_t first = 0;

        for (;;)
        {
            size_t pos = src.find_first_of(delm, first);

            if (pos == std::string::npos)
            {
                if (src.size() > first)
                {
                    result.push_back(std::string(src, first));
                }

                break;
            }
            else if (pos > first)
            {
                result.push_back(std::string(src, first, pos - first));
            }

            first = pos + 1;
        }

        return result;
    }
}

//----------------------------------------------------------------------------//
// Random
//----------------------------------------------------------------------------//

namespace Random
{
    SEV_DECL uint32_t generate32();
    SEV_DECL std::vector<unsigned char> generateBytes(size_t length);
}

SEV_NS_END

#endif // SUBEVENT_UTILITY_HPP
