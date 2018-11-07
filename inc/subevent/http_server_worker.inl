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
    try
    {
        HttpUrl url(httpChannel->getRequest().getPath());

        const HttpRequestHandler& handler =
            mHandlerMap.getHandler(url.getPath());

        if (handler != nullptr)
        {
            handler(httpChannel);
        }
        else
        {
            httpChannel->close();
        }
    }
    catch (...)
    {
        onError(httpChannel, -1);
    }
}

void HttpChannelWorker::onError(
    const HttpChannelPtr& httpChannel, int32_t /* errorCode */)
{
    httpChannel->close();
    onHttpClose(httpChannel);
}

void HttpChannelWorker::onAccept(const TcpChannelPtr& channel)
{
    HttpChannelPtr httpChannel =
        std::dynamic_pointer_cast<HttpChannel>(channel);

    httpChannel->setRequestHandler(
        SEV_BIND_1(this, HttpChannelWorker::onRequest));
    httpChannel->setErrorHandler(
        SEV_BIND_2(this, HttpChannelWorker::onError));

    onHttpAccept(httpChannel);
}

void HttpChannelWorker::onReceive(
    const TcpChannelPtr& channel, std::vector<char>&& message)
{
    HttpChannelPtr httpChannel =
        std::dynamic_pointer_cast<HttpChannel>(channel);

    httpChannel->onTcpReceive(std::move(message));
}

void HttpChannelWorker::onClose(const TcpChannelPtr& channel)
{
    HttpChannelPtr httpChannel =
        std::dynamic_pointer_cast<HttpChannel>(channel);

    onHttpClose(httpChannel);
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
    const SslContextPtr& sslCtx,
    int32_t listenBacklog)
{
    HttpServerPtr httpServer =
        std::dynamic_pointer_cast<HttpServer>(getTcpServer());
    httpServer->setSslContext(sslCtx);

    return TcpServerWorker::open(localEndPoint, listenBacklog);
}

SEV_NS_END

#endif // SUBEVENT_HTTP_SERVER_WORKER_INL
