#ifndef SUBEVENT_SSL_SOCKET_HPP
#define SUBEVENT_SSL_SOCKET_HPP

#include <openssl/ssl.h>

#include <subevent/std.hpp>
#include <subevent/socket.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SecureSocket
//---------------------------------------------------------------------------//

class SecureSocket : public Socket
{
public:
    SEV_DECL SecureSocket(Handle handle = InvalidHandle);
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
    SSL* mSsl;
    SSL_CTX* mSslCtx;
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

#endif // SUBEVENT_SSL_SOCKET_HPP
