#ifndef SUBEVENT_HTTP_SERVER_WORKER_HPP
#define SUBEVENT_HTTP_SERVER_WORKER_HPP

#include <string>

#include <subevent/std.hpp>
#include <subevent/thread.hpp>
#include <subevent/application.hpp>
#include <subevent/tcp_server_worker.hpp>
#include <subevent/http_server.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// HttpChannelWorker
//----------------------------------------------------------------------------//

class HttpChannelWorker : public TcpChannelWorker
{
public:
    SEV_DECL virtual ~HttpChannelWorker() override;

protected:
    SEV_DECL void setRequestHandler(
        const std::string& path,
        const HttpRequestHandler& handler);

    // default handler
    SEV_DECL virtual void onHttpRequest(
        const HttpChannelPtr& httpChannel);

    SEV_DECL virtual void onHttpAccept(
        const HttpChannelPtr& /* httpChannel */) {}
    SEV_DECL virtual void onHttpClose(
        const HttpChannelPtr& /* httpChannel */) {}

protected:
    SEV_DECL HttpChannelWorker(Thread* thread);

    SEV_DECL void onRequest(
        const HttpChannelPtr& httpChannel);
    SEV_DECL void onError(
        const HttpChannelPtr& httpChannel,
        int32_t errorCode);

private:
    SEV_DECL void onAccept(
        const TcpChannelPtr& channel) override;
    SEV_DECL void onReceive(
        const TcpChannelPtr& channel,
        std::vector<char>&& message) override;
    SEV_DECL void onClose(
        const TcpChannelPtr& channel) override;

    HttpChannelWorker() = delete;

    HttpHandlerMap mHandlerMap;
};

//---------------------------------------------------------------------------//
// HttpChannelThread
//---------------------------------------------------------------------------//

typedef NetTask<Thread, HttpChannelWorker> HttpChannelThread;

//----------------------------------------------------------------------------//
// HttpServerWorker
//----------------------------------------------------------------------------//

class HttpServerWorker : public TcpServerWorker
{
public:
    SEV_DECL virtual ~HttpServerWorker() override;

    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const SslContextPtr& sslCtx = nullptr,
        int32_t listenBacklog = SOMAXCONN);

protected:
    SEV_DECL HttpServerWorker(Thread* thread);

    SEV_DECL void createTcpServer() override
    {
        if (mTcpServer == nullptr)
        {
            mTcpServer = HttpServer::newInstance(this);
        }
    }

private:
    HttpServerWorker() = delete;
};

//---------------------------------------------------------------------------//
// HttpServerApp / HttpServerThread
//---------------------------------------------------------------------------//

typedef NetTask<Application, HttpServerWorker> HttpServerApp;
typedef NetTask<Thread, HttpServerWorker> HttpServerThread;

SEV_NS_END

#endif // SUBEVENT_HTTP_SERVER_WORKER_HPP
