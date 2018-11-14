#ifndef SUBEVENT_HTTP_SERVER_WORKER_INL
#define SUBEVENT_HTTP_SERVER_WORKER_INL

#include <memory>
#include <subevent/http_server_worker.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// HttpChannelWorker
//----------------------------------------------------------------------------//

HttpChannelWorker::HttpChannelWorker(Thread* thread)
    : TcpChannelWorker(thread)
{
    mHandlerMap.setDefaultHandler(
        SEV_BIND_1(this, HttpChannelWorker::onHttpRequest));

    mThread->setEventHandler(
        TcpEventId::Accept, [&](const Event* event) {

        TcpChannelPtr newChannel = TcpServer::accept(event);

        if (newChannel != nullptr)
        {
            newChannel->setCloseHandler(
                [&](const TcpChannelPtr& channel) {

                onClose(channel);
            });

            HttpChannelPtr httpChannel =
                std::dynamic_pointer_cast<HttpChannel>(newChannel);

            httpChannel->setRequestHandler(
                SEV_BIND_1(this, HttpChannelWorker::onRequest));

            onAccept(newChannel);
        }
    });
}

HttpChannelWorker::~HttpChannelWorker()
{
}

void HttpChannelWorker::setRequestHandler(
    const std::string& path,
    const HttpRequestHandler& handler)
{
    if (handler != nullptr)
    {
        mHandlerMap.setHandler(path, handler);
    }
    else
    {
        mHandlerMap.removeHandler(path);
    }
}

void HttpChannelWorker::onHttpRequest(const HttpChannelPtr& httpChannel)
{
    // default handler

    HttpServer::defaultHandler(httpChannel);
}

void HttpChannelWorker::onRequest(const HttpChannelPtr& httpChannel)
{
    mHandlerMap.onRequest(httpChannel);
}

//----------------------------------------------------------------------------//
// HttpServerWorker
//----------------------------------------------------------------------------//

HttpServerWorker::HttpServerWorker(Thread* thread)
    : TcpServerWorker(thread)
{
}

HttpServerWorker::~HttpServerWorker()
{
}

bool HttpServerWorker::open(
    const IpEndPoint& localEndPoint,
#ifdef SEV_SUPPORTS_SSL
    const SslContextPtr& sslCtx,
#endif
    int32_t listenBacklog)
{
    HttpServerPtr httpServer =
        std::dynamic_pointer_cast<HttpServer>(getTcpServer());

    bool result = httpServer->open(
        localEndPoint,
#ifdef SEV_SUPPORTS_SSL
        sslCtx,
#endif
        SEV_BIND_2(this, HttpServerWorker::onTcpAccept),
        listenBacklog);

    return result;
}

SEV_NS_END

#endif // SUBEVENT_HTTP_SERVER_WORKER_INL
