#ifndef SUBEVENT_CRYPTO_HPP
#define SUBEVENT_CRYPTO_HPP

#include <string>
#include <vector>
#include <utility>

#include <subevent/std.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Base64
//---------------------------------------------------------------------------//

class Base64
{
public:
    SEV_DECL static std::string encode(const void* data, size_t size);
};

//---------------------------------------------------------------------------//
// Hash
//---------------------------------------------------------------------------//

namespace Hash
{
    SEV_DECL std::vector<unsigned char> sha1(const void* data, size_t size);
}

SEV_NS_END

#endif // SUBEVENT_CRYPTO_HPP
