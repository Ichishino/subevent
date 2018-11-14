#ifndef SUBEVENT_CRYPTO_INL
#define SUBEVENT_CRYPTO_INL

#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

#include <subevent/std.hpp>
#include <subevent/crypto.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// Base64
//---------------------------------------------------------------------------//

std::string Base64::encode(const void* src, size_t size)
{
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/";
    static const char padding = '=';

    const unsigned char* data = (const unsigned char*)src;

    std::string dest;
    dest.resize((size_t)(std::ceil(((double)size) / 3) * 4));

    std::string::iterator it = dest.begin();

    for (size_t index = 0; index < (dest.size() / 4); ++index)
    {
        size_t srcIndex = index * 3;

        const unsigned char orig[3] = {
            (srcIndex < size) ? data[srcIndex] : (unsigned char)0,
            ((srcIndex + 1) < size) ? data[srcIndex + 1] : (unsigned char)0,
            ((srcIndex + 2) < size) ? data[srcIndex + 2] : (unsigned char)0
        };

        *it++ = table[((orig[0] & 0xFC) >> 2)];
        *it++ = table[((orig[0] & 0x03) << 4) | ((orig[1] & 0xF0) >> 4)];
        *it++ = table[((orig[1] & 0x0F) << 2) | ((orig[2] & 0xC0) >> 6)];
        *it++ = table[(orig[2] & 0x3F)];
    }

    it -= (3 - size % 3) % 3;

    while (it != dest.end())
    {
        *it++ = padding;
    }

    return dest;
}

//---------------------------------------------------------------------------//
// Sha1
//---------------------------------------------------------------------------//

#define SEV_SHA1_ROL(v, b) \
    (((v) << (b)) | ((v) >> (32 - (b))))

#ifdef SEV_LITTLE_ENDIAN
#define SEV_SHA1_BLOCK_0(b, i) \
    (b.l[i] = \
        (SEV_SHA1_ROL(b.l[i], 24) & 0xFF00FF00) | \
        (SEV_SHA1_ROL(b.l[i], 8) & 0x00FF00FF))
#else
#define SEV_SHA1_BLOCK_0(b, i) b.l[i]
#endif

#define SEV_SHA1_BLOCK(b, i) \
    (b.l[i & 15] = \
        SEV_SHA1_ROL(b.l[(i + 13) & 15] ^ b.l[(i + 8) & 15] ^ \
        b.l[(i + 2) & 15] ^ b.l[i & 15], 1))

#define SEV_SHA1_ROUND_0(b, v, w, x, y, z, i) \
    z += ((w & (x^y))^y) + SEV_SHA1_BLOCK_0(b, i) + \
        0x5A827999 + SEV_SHA1_ROL(v, 5); \
    w = SEV_SHA1_ROL(w, 30);
#define SEV_SHA1_ROUND_1(b, v, w, x, y, z, i) \
    z += ((w & (x^y))^y) + SEV_SHA1_BLOCK(b, i) + \
        0x5A827999 + SEV_SHA1_ROL(v, 5); \
    w = SEV_SHA1_ROL(w, 30);
#define SEV_SHA1_ROUND_2(b, v, w, x, y, z, i) \
    z += (w^x^y) + SEV_SHA1_BLOCK(b, i) + \
        0x6ED9EBA1 + SEV_SHA1_ROL(v, 5); \
    w = SEV_SHA1_ROL(w, 30);
#define SEV_SHA1_ROUND_3(b, v, w, x, y, z, i) \
    z += (((w | x) & y) | (w & x)) + SEV_SHA1_BLOCK(b, i) + \
        0x8F1BBCDC + SEV_SHA1_ROL(v, 5); \
    w = SEV_SHA1_ROL(w, 30);
#define SEV_SHA1_ROUND_4(b, v, w, x, y, z, i) \
    z += (w^x^y) + SEV_SHA1_BLOCK(b, i) + \
        0xCA62C1D6 + SEV_SHA1_ROL(v, 5); \
    w = SEV_SHA1_ROL(w, 30);

Sha1::Sha1()
{
    mState[0] = 0x67452301;
    mState[1] = 0xEFCDAB89;
    mState[2] = 0x98BADCFE;
    mState[3] = 0x10325476;
    mState[4] = 0xC3D2E1F0;

    mCount[0] = 0;
    mCount[1] = 0;
}

Sha1::~Sha1()
{
}

void Sha1::transform(const unsigned char* buffer)
{
    union
    {
        unsigned char c[64];
        uint32_t l[16];

    } block;

    memcpy(&block, buffer, 64);

    uint32_t a = mState[0];
    uint32_t b = mState[1];
    uint32_t c = mState[2];
    uint32_t d = mState[3];
    uint32_t e = mState[4];

    SEV_SHA1_ROUND_0(block, a, b, c, d, e, 0);
    SEV_SHA1_ROUND_0(block, e, a, b, c, d, 1);
    SEV_SHA1_ROUND_0(block, d, e, a, b, c, 2);
    SEV_SHA1_ROUND_0(block, c, d, e, a, b, 3);
    SEV_SHA1_ROUND_0(block, b, c, d, e, a, 4);
    SEV_SHA1_ROUND_0(block, a, b, c, d, e, 5);
    SEV_SHA1_ROUND_0(block, e, a, b, c, d, 6);
    SEV_SHA1_ROUND_0(block, d, e, a, b, c, 7);
    SEV_SHA1_ROUND_0(block, c, d, e, a, b, 8);
    SEV_SHA1_ROUND_0(block, b, c, d, e, a, 9);
    SEV_SHA1_ROUND_0(block, a, b, c, d, e, 10);
    SEV_SHA1_ROUND_0(block, e, a, b, c, d, 11);
    SEV_SHA1_ROUND_0(block, d, e, a, b, c, 12);
    SEV_SHA1_ROUND_0(block, c, d, e, a, b, 13);
    SEV_SHA1_ROUND_0(block, b, c, d, e, a, 14);
    SEV_SHA1_ROUND_0(block, a, b, c, d, e, 15);

    SEV_SHA1_ROUND_1(block, e, a, b, c, d, 16);
    SEV_SHA1_ROUND_1(block, d, e, a, b, c, 17);
    SEV_SHA1_ROUND_1(block, c, d, e, a, b, 18);
    SEV_SHA1_ROUND_1(block, b, c, d, e, a, 19);

    SEV_SHA1_ROUND_2(block, a, b, c, d, e, 20);
    SEV_SHA1_ROUND_2(block, e, a, b, c, d, 21);
    SEV_SHA1_ROUND_2(block, d, e, a, b, c, 22);
    SEV_SHA1_ROUND_2(block, c, d, e, a, b, 23);
    SEV_SHA1_ROUND_2(block, b, c, d, e, a, 24);
    SEV_SHA1_ROUND_2(block, a, b, c, d, e, 25);
    SEV_SHA1_ROUND_2(block, e, a, b, c, d, 26);
    SEV_SHA1_ROUND_2(block, d, e, a, b, c, 27);
    SEV_SHA1_ROUND_2(block, c, d, e, a, b, 28);
    SEV_SHA1_ROUND_2(block, b, c, d, e, a, 29);
    SEV_SHA1_ROUND_2(block, a, b, c, d, e, 30);
    SEV_SHA1_ROUND_2(block, e, a, b, c, d, 31);
    SEV_SHA1_ROUND_2(block, d, e, a, b, c, 32);
    SEV_SHA1_ROUND_2(block, c, d, e, a, b, 33);
    SEV_SHA1_ROUND_2(block, b, c, d, e, a, 34);
    SEV_SHA1_ROUND_2(block, a, b, c, d, e, 35);
    SEV_SHA1_ROUND_2(block, e, a, b, c, d, 36);
    SEV_SHA1_ROUND_2(block, d, e, a, b, c, 37);
    SEV_SHA1_ROUND_2(block, c, d, e, a, b, 38);
    SEV_SHA1_ROUND_2(block, b, c, d, e, a, 39);

    SEV_SHA1_ROUND_3(block, a, b, c, d, e, 40);
    SEV_SHA1_ROUND_3(block, e, a, b, c, d, 41);
    SEV_SHA1_ROUND_3(block, d, e, a, b, c, 42);
    SEV_SHA1_ROUND_3(block, c, d, e, a, b, 43);
    SEV_SHA1_ROUND_3(block, b, c, d, e, a, 44);
    SEV_SHA1_ROUND_3(block, a, b, c, d, e, 45);
    SEV_SHA1_ROUND_3(block, e, a, b, c, d, 46);
    SEV_SHA1_ROUND_3(block, d, e, a, b, c, 47);
    SEV_SHA1_ROUND_3(block, c, d, e, a, b, 48);
    SEV_SHA1_ROUND_3(block, b, c, d, e, a, 49);
    SEV_SHA1_ROUND_3(block, a, b, c, d, e, 50);
    SEV_SHA1_ROUND_3(block, e, a, b, c, d, 51);
    SEV_SHA1_ROUND_3(block, d, e, a, b, c, 52);
    SEV_SHA1_ROUND_3(block, c, d, e, a, b, 53);
    SEV_SHA1_ROUND_3(block, b, c, d, e, a, 54);
    SEV_SHA1_ROUND_3(block, a, b, c, d, e, 55);
    SEV_SHA1_ROUND_3(block, e, a, b, c, d, 56);
    SEV_SHA1_ROUND_3(block, d, e, a, b, c, 57);
    SEV_SHA1_ROUND_3(block, c, d, e, a, b, 58);
    SEV_SHA1_ROUND_3(block, b, c, d, e, a, 59);

    SEV_SHA1_ROUND_4(block, a, b, c, d, e, 60);
    SEV_SHA1_ROUND_4(block, e, a, b, c, d, 61);
    SEV_SHA1_ROUND_4(block, d, e, a, b, c, 62);
    SEV_SHA1_ROUND_4(block, c, d, e, a, b, 63);
    SEV_SHA1_ROUND_4(block, b, c, d, e, a, 64);
    SEV_SHA1_ROUND_4(block, a, b, c, d, e, 65);
    SEV_SHA1_ROUND_4(block, e, a, b, c, d, 66);
    SEV_SHA1_ROUND_4(block, d, e, a, b, c, 67);
    SEV_SHA1_ROUND_4(block, c, d, e, a, b, 68);
    SEV_SHA1_ROUND_4(block, b, c, d, e, a, 69);
    SEV_SHA1_ROUND_4(block, a, b, c, d, e, 70);
    SEV_SHA1_ROUND_4(block, e, a, b, c, d, 71);
    SEV_SHA1_ROUND_4(block, d, e, a, b, c, 72);
    SEV_SHA1_ROUND_4(block, c, d, e, a, b, 73);
    SEV_SHA1_ROUND_4(block, b, c, d, e, a, 74);
    SEV_SHA1_ROUND_4(block, a, b, c, d, e, 75);
    SEV_SHA1_ROUND_4(block, e, a, b, c, d, 76);
    SEV_SHA1_ROUND_4(block, d, e, a, b, c, 77);
    SEV_SHA1_ROUND_4(block, c, d, e, a, b, 78);
    SEV_SHA1_ROUND_4(block, b, c, d, e, a, 79);

    mState[0] += a;
    mState[1] += b;
    mState[2] += c;
    mState[3] += d;
    mState[4] += e;
}

void Sha1::update(const unsigned char* data, uint32_t size)
{
    uint32_t j = mCount[0];

    mCount[0] += (size << 3);

    if (mCount[0] < j)
    {
        ++mCount[1];
    }

    mCount[1] += (size >> 29);

    j = (j >> 3) & 63;

    uint32_t i = 0;

    if ((j + size) > 63)
    {
        i = 64 - j;

        memcpy(&mBuffer[j], data, i);

        transform(mBuffer);

        for (; i + 63 < size; i += 64)
        {
            transform(&data[i]);
        }

        j = 0;
    }

    memcpy(&mBuffer[j], &data[i], size - i);
}

void Sha1::final(unsigned char* digest)
{
    unsigned char fcount[8];

    for (uint32_t i = 0; i < 8; ++i)
    {
        fcount[i] =
            (unsigned char)((mCount[((i >= 4) ? 0 : 1)]
                >> ((3 - (i & 3)) * 8)) & 0xFF);
    }

    unsigned char c = 0x80;

    update(&c, 1);

    while ((mCount[0] & 0x000001F8) != 0x000001C0)
    {
        c = 0x00;
        update(&c, 1);
    }

    update(fcount, 8);

    for (uint32_t i = 0; i < HashSize; ++i)
    {
        digest[i] =
            (unsigned char)((mState[i >> 2]
                >> ((3 - (i & 3)) * 8)) & 0xFF);
    }
}

std::vector<unsigned char> Sha1::digest(const void* data, uint32_t size)
{
    std::vector<unsigned char> result(HashSize);

    Sha1 sha1;
    sha1.update((const unsigned char*)data, size);
    sha1.final(&result[0]);

    return result;
}

std::string Sha1::digestHexString(const void* data, uint32_t size)
{
    std::ostringstream oss;

    for (unsigned char b : Sha1::digest(data, size))
    {
        oss << std::setfill('0')
            << std::setw(2)
            << std::hex
            << (uint16_t)b;
    }

    return oss.str();
}

SEV_NS_END

#endif // SUBEVENT_CRYPTO_INL
