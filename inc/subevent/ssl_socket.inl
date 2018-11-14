#ifndef SUBEVENT_SSL_SOCKET_INL
#define SUBEVENT_SSL_SOCKET_INL

#ifdef SEV_SUPPORTS_SSL

#include <openssl/err.h>

#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SslContext
//---------------------------------------------------------------------------//

SslContext::SslContext(const SSL_METHOD* method)
{
    OpenSsl::init();

    mHandle = SSL_CTX_new(method);
}

SslContext::~SslContext()
{
    SSL_CTX_free(mHandle);
}

unsigned long SslContext::setOptions(long options)
{
    return SSL_CTX_set_options(mHandle, options);
}

unsigned long SslContext::clearOptions(long options)
{
    return SSL_CTX_clear_options(mHandle, options);
}

unsigned long SslContext::getOptions() const
{
    return SSL_CTX_get_options(mHandle);
}

bool SslContext::setCertificateFile(
    const std::string& fileName, int type)
{
    int result = SSL_CTX_use_certificate_file(
        mHandle, fileName.c_str(), type);

    if (result != 1)
    {
        return false;
    }

    return true;
}

bool SslContext::setPrivateKeyFile(
    const std::string& fileName, int type)
{
    int result = SSL_CTX_use_PrivateKey_file(
        mHandle, fileName.c_str(), type);

    if (result != 1)
    {
        return false;
    }

    return true;
}

bool SslContext::loadVerifyLocations(
    const std::string& fileName, const std::string& path)
{
    const char* caPathAddr = nullptr;

    if (!path.empty())
    {
        caPathAddr = path.c_str();
    }

    int result = SSL_CTX_load_verify_locations(
        mHandle, fileName.c_str(), caPathAddr);

    if (result != 1)
    {
        return false;
    }

    return true;
}

void SslContext::setVerify(
    int mode, int(*verify_callback)(int, X509_STORE_CTX*))
{
    SSL_CTX_set_verify(mHandle, mode, verify_callback);
}

void SslContext::setVerifyDepth(int depth)
{
    SSL_CTX_set_verify_depth(mHandle, depth);
}

//----------------------------------------------------------------------------//
// SecureSocket
//----------------------------------------------------------------------------//

SecureSocket::SecureSocket(const SslContextPtr& sslCtx, Handle handle)
    : Socket(handle)
{
    OpenSsl::init();

    mSslCtx = sslCtx;
    mSsl = nullptr;
}

SecureSocket::~SecureSocket()
{
}

Socket* SecureSocket::accept()
{
    Socket* socket = nullptr;

    Handle handle = ::accept(
        getHandle(), nullptr, nullptr);
    if (handle != InvalidHandle)
    {
        socket = new SecureSocket(mSslCtx, handle);
    }

    mErrorCode = Socket::getLastError();

    return socket;
}

int32_t SecureSocket::send(
    const void* data, uint32_t size, int32_t /* flags */)
{
    int result = SSL_write(mSsl, data, size);

    mErrorCode = SSL_get_error(mSsl, result);

    return result;
}

int32_t SecureSocket::receive(
    void* buff, uint32_t size, int32_t flags)
{
    int result;
    
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

bool SecureSocket::onAccept()
{
    mSsl = SSL_new(mSslCtx->getHandle());

    if (SSL_set_fd(mSsl,
        static_cast<int>(getHandle())) != 1)
    {
        return false;
    }

    int result = SSL_accept(mSsl);

    mErrorCode = SSL_get_error(mSsl, result);

    if (result != 1)
    {
        return false;
    }

    return true;
}

bool SecureSocket::onConnect()
{
    mSsl = SSL_new(mSslCtx->getHandle());
    
    if (SSL_set_fd(mSsl,
        static_cast<int>(getHandle())) != 1)
    {
        return false;
    }
    
    int result = SSL_connect(mSsl);

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

#ifdef SEV_OS_WIN
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

#endif // SEV_SUPPORTS_SSL

#endif // SUBEVENT_SSL_SOCKET_INL
