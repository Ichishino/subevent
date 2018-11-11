#ifndef SUBEVENT_CRYPTO_INL
#define SUBEVENT_CRYPTO_INL

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>

#include <subevent/std.hpp>
#include <subevent/crypto.hpp>

SEV_NS_BEGIN

#ifdef _WIN32
#pragma comment(lib, "libcrypto.lib")
#endif

//---------------------------------------------------------------------------//
// Base64
//---------------------------------------------------------------------------//

std::string Base64::encode(const void* data, size_t size)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    for (;;)
    {
        int res = BIO_write(
            b64, data, static_cast<int>(size));

        if (res > 0)
        {
            // success
            break;
        }

        if (BIO_should_retry(b64))
        {
            continue;
        }
        else
        {
            // error
            BIO_free_all(b64);
            return std::string();
        }
    }

    (void)BIO_flush(b64);

    BUF_MEM* buff;
    BIO_get_mem_ptr(b64, &buff);

    std::string result(buff->data, buff->length);

    BIO_free_all(b64);

    return result;
}

//---------------------------------------------------------------------------//
// Hash
//---------------------------------------------------------------------------//

namespace Hash
{
    std::vector<unsigned char> sha1(const void* data, size_t size)
    {
        std::vector<unsigned char> hash(SHA_DIGEST_LENGTH);

        SHA1((const unsigned char*)data, size, &hash[0]);

        return hash;
    }
}

SEV_NS_END

#endif // SUBEVENT_CRYPTO_INL
