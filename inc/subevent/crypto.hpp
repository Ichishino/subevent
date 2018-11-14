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
    SEV_DECL static std::string encode(const void* src, size_t size);

    template<class ContainerType>
    SEV_DECL static bool decode(const std::string& src, ContainerType& dest)
    {
        static const unsigned char table[256] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0x80, 0xFF, 0xFF, 0x80, 0xFF, 0xFF, // \n, \r
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF,   62, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // !
            0xFF, 0xFF, 0xFF,   62, 0xFF,   62, 0xFF,   63, // +, -, /
              52,   53,   54,   55,   56,   57,   58,   59, // 0-7
              60,   61,   63, 0xFF, 0xFF, 0x80, 0xFF, 0xFF, // 8, 9, :, =
            0xFF,    0,    1,    2,    3,    4,    5,    6, // A-G
               7,    8,    9,   10,   11,   12,   13,   14, // H-O
              15,   16,   17,   18,   19,   20,   21,   22, // P-W
              23,   24,   25, 0xFF, 0xFF, 0xFF, 0xFF,   63, // X-Z, _
            0xFF,   26,   27,   28,   29,   30,   31,   32, // a-g
              33,   34,   35,   36,   37,   38,   39,   40, // h-s
              41,   42,   43,   44,   45,   46,   47,   48, // p-w
              49,   50,   51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // x-z
        };

        size_t size = src.size();
        dest.reserve(size / 4 * 3);

        unsigned char b64[4];
        size_t count = 0;

        for (size_t index = 0; index < size; ++index)
        {
            unsigned char b = table[
                static_cast<unsigned char>(src[index])];

            if (b == 0xFF)
            {
                dest.clear();
                return false;
            }
            else if (b != 0x80)
            {
                b64[count++] = b;

                if (count == 2)
                {
                    dest.push_back(
                        ((b64[0] & 0x3F) << 2) | ((b64[1] & 0x30) >> 4));
                }
                else if (count == 3)
                {
                    dest.push_back(
                        ((b64[1] & 0x0F) << 4) | ((b64[2] & 0x3C) >> 2));
                }
                else if (count == 4)
                {
                    dest.push_back(
                        ((b64[2] & 0x03) << 6) | (b64[3] & 0x3F));
                    count = 0;
                }
            }
        }

        return true;
    }
};

//---------------------------------------------------------------------------//
// Hash
//---------------------------------------------------------------------------//

class Sha1
{
public:
    SEV_DECL static std::vector<unsigned char>
        digest(const void* data, uint32_t size);

    SEV_DECL static std::string
        digestHexString(const void* data, uint32_t size);

public:
    static const uint16_t HashSize = 20;

private:
    SEV_DECL Sha1();
    SEV_DECL ~Sha1();

    SEV_DECL void transform(const unsigned char* buffer);
    SEV_DECL void update(const unsigned char* data, uint32_t size);
    SEV_DECL void final(unsigned char* digest);

    uint32_t mState[5];
    uint32_t mCount[2];
    unsigned char mBuffer[64];
};

SEV_NS_END

#endif // SUBEVENT_CRYPTO_HPP
