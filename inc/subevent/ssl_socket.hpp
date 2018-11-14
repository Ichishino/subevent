#ifndef SUBEVENT_SSL_SOCKET_HPP
#define SUBEVENT_SSL_SOCKET_HPP

#ifdef SEV_SUPPORTS_SSL

#include <memory>

#include <openssl/ssl.h>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

class SslContext;

typedef std::shared_ptr<SslContext> SslContextPtr;

//---------------------------------------------------------------------------//
// SslContext
//---------------------------------------------------------------------------//

class SslContext
{
public:
    SEV_DECL static SslContextPtr newInstance(const SSL_METHOD* method)
    {
        return std::make_shared<SslContext>(method);
    }

    SEV_DECL ~SslContext();

public:

    // settings

    SEV_DECL unsigned long setOptions(long options);
    SEV_DECL unsigned long clearOptions(long options);
    SEV_DECL unsigned long getOptions() const;

    SEV_DECL bool setCertificateFile(
        const std::string& fileName, int type = SSL_FILETYPE_PEM);

    SEV_DECL bool setPrivateKeyFile(
        const std::string& path, int type = SSL_FILETYPE_PEM);

    SEV_DECL bool loadVerifyLocations(
        const std::string& fileName, const std::string& caPath = "");

    SEV_DECL void setVerify(
        int mode, int(*verify_callback)(int, X509_STORE_CTX*));
    SEV_DECL void setVerifyDepth(int depth);

public:
    SEV_DECL SSL_CTX* getHandle() const
    {
        return mHandle;
    }

    SEV_DECL SslContext(const SSL_METHOD* method);

private:
    SslContext() = delete;

    SSL_CTX* mHandle;
};

//---------------------------------------------------------------------------//
// SecureSocket
//---------------------------------------------------------------------------//

class SecureSocket : public Socket
{
public:
    SEV_DECL SecureSocket(
        const SslContextPtr& sslCtx, Handle handle = InvalidHandle);
    SEV_DECL ~SecureSocket() override;

public:
    SEV_DECL Socket* accept() override;

    SEV_DECL int32_t send(
        const void* data, uint32_t size, int32_t flags = 0) override;
    SEV_DECL int32_t receive(
        void* buff, uint32_t size, int32_t flags = 0) override;

    SEV_DECL void close() override;

public:
    SEV_DECL bool onAccept() override;
    SEV_DECL bool onConnect() override;

private:
    SecureSocket() = delete;

    SSL* mSsl;
    SslContextPtr mSslCtx;
};

//---------------------------------------------------------------------------//
// OpenSsl
//---------------------------------------------------------------------------//

class OpenSsl
{
public:
    SEV_DECL static void init();

private:
    SEV_DECL OpenSsl();
    SEV_DECL ~OpenSsl();
};

SEV_NS_END

#endif // SEV_SUPPORTS_SSL

#endif // SUBEVENT_SSL_SOCKET_HPP
