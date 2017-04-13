#ifndef SUBEVENT_SSL_SOCKET_INL
#define SUBEVENT_SSL_SOCKET_INL

#include <openssl/err.h>

#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// SecureSocket
//----------------------------------------------------------------------------//

SecureSocket::SecureSocket()
{
    OpenSsl::init();

    mSsl = nullptr;
    mSslCtx = SSL_CTX_new(SSLv23_client_method());
}

SecureSocket::~SecureSocket()
{
    SSL_CTX_free(mSslCtx);
}

int32_t SecureSocket::send(
    const void* data, uint32_t size, int32_t /* flags */)
{
    int32_t result = SSL_write(mSsl, data, size);

    mErrorCode = SSL_get_error(mSsl, result);

    return result;
}

int32_t SecureSocket::receive(
    void* buff, uint32_t size, int32_t flags)
{
    int32_t result;
    
    if (flags & MSG_PEEK)
    {
        result = SSL_peek(mSsl, buff, size);

        mErrorCode = SSL_get_error(mSsl, result);

        if (result == -1)
        {
            if (mErrorCode == SSL_ERROR_WANT_READ)
            {
                result = static_cast<int32_t>(size);
            }
        }
    }
    else
    {
        result = SSL_read(mSsl, buff, size);

        mErrorCode = SSL_get_error(mSsl, result);
    }

    return result;
}

void SecureSocket::close()
{
    if (mSsl != nullptr)
    {
        SSL_shutdown(mSsl);

        SSL_free(mSsl);
        mSsl = nullptr;
    }
    
    Socket::close();
}

bool SecureSocket::onConnect()
{
    mSsl = SSL_new(mSslCtx);
    
    if (SSL_set_fd(mSsl,
        static_cast<int>(getHandle())) != 1)
    {
        return false;
    }
    
    int32_t result = SSL_connect(mSsl);

    mErrorCode = SSL_get_error(mSsl, result);

    if (result != 1)
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------//
// OpenSsl
//---------------------------------------------------------------------------//

#ifdef _WIN32
#pragma comment(lib, "libssl.lib")
#endif

OpenSsl::OpenSsl()
{
    SSL_load_error_strings();
    SSL_library_init();
}

OpenSsl::~OpenSsl()
{
    ERR_free_strings();
}

void OpenSsl::init()
{
    static OpenSsl openSsl;
}

SEV_NS_END

#endif // SUBEVENT_SSL_SOCKET_INL
