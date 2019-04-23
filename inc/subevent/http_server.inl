#ifndef SUBEVENT_HTTP_SERVER_INL
#define SUBEVENT_HTTP_SERVER_INL

#include <vector>
#include <subevent/http_server.hpp>
#include <subevent/ws.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// HttpHandlerMap
//----------------------------------------------------------------------------//

HttpHandlerMap::HttpHandlerMap()
{
}

HttpHandlerMap::~HttpHandlerMap()
{
}

void HttpHandlerMap::setHandler(
    const std::string& path,
    const HttpRequestHandler& handler)
{
    if (isDirectory(path))
    {
        mDirMap[path] = handler;
    }
    else
    {
        mFileMap[path] = handler;
    }
}

HttpRequestHandler HttpHandlerMap::getHandler(
    const std::string& path) const
{
    HttpRequestHandler handler;

    if (isDirectory(path))
    {
        handler = findHandler(mDirMap, path);
    }
    else
    {
        handler = findHandler(mFileMap, path);

        if (handler == nullptr)
        {
            size_t pos = path.find_last_of('/');

            if (pos == std::string::npos)
            {
                handler = findHandler(
                    mFileMap, path);
            }
            else
            {
                handler = findHandler(
                    mDirMap, path.substr(0, pos + 1));
            }
        }
    }

    if (handler != nullptr)
    {
        return handler;
    }
    else
    {
        return mDefaultHandler;
    }
}

void HttpHandlerMap::removeHandler(const std::string& path)
{
    if (isDirectory(path))
    {
        mDirMap.erase(path);
    }
    else
    {
        mFileMap.erase(path);
    }
}

void HttpHandlerMap::clear()
{
    mDirMap.clear();
    mFileMap.clear();
}

const HttpRequestHandler& HttpHandlerMap::findHandler(
    const HandlerMap& map, const std::string& path) const
{
    auto it = map.find(path);

    if (it == map.end())
    {
        static const HttpRequestHandler empty;
        return empty;
    }
    else
    {
        return it->second;
    }
}

void HttpHandlerMap::onRequest(const HttpChannelPtr& httpChannel)
{
    try
    {
        HttpUrl url(httpChannel->getRequest().getPath());

        const HttpRequestHandler& handler =
            getHandler(url.getPath());

        if (handler != nullptr)
        {
            // call
            handler(httpChannel);

            httpChannel->getRequest().clear();
        }
        else
        {
            httpChannel->close();
        }
    }
    catch (...)
    {
        httpChannel->close();
    }
}

//----------------------------------------------------------------------------//
// HttpChannel
//----------------------------------------------------------------------------//

HttpChannel::HttpChannel(Socket* socket)
    : TcpChannel(socket)
{
    setReceiveHandler(SEV_BIND_1(this, HttpChannel::onTcpReceive));
}

HttpChannel::~HttpChannel()
{
}

void HttpChannel::close()
{
    if (mWsChannel != nullptr)
    {
        mWsChannel->close();
        mWsChannel = nullptr;
    }

    TcpChannel::close();
}

void HttpChannel::onTcpReceive(const TcpChannelPtr& channel)
{
    auto request = channel->receiveAll();

    if (request.empty())
    {
        return;
    }

    if (!mRequestTempBuffer.empty())
    {
        std::copy(
            request.begin(),
            request.end(),
            std::back_inserter(mRequestTempBuffer));

        request = std::move(mRequestTempBuffer);
    }

    StringReader reader(request);

    if (!onHttpRequest(reader))
    {
        // header is not completed
        mRequestTempBuffer = std::move(request);
    }
}

bool HttpChannel::onHttpRequest(StringReader& reader)
{
    // header
    if (mRequest.isEmpty())
    {
        try
        {
            if (!mRequest.deserializeMessage(reader))
            {
                // not completed
                mRequest.clear();
                return false;
            }

            if (!mContentReceiver.init(mRequest))
            {
                // too much data
                close();
                return true;
            }
        }
        catch (...)
        {
            // invalid data
            close();
            return true;
        }
    }

    // body
    if (!mContentReceiver.onReceive(reader))
    {
        close();
        return true;
    }

    if (isRequestCompleted())
    {
        mRequest.setBody(mContentReceiver.getData());

        onRequestCompleted();
    }

    return true;
}

bool HttpChannel::isRequestCompleted() const
{
    if (mRequest.isEmpty())
    {
        return false;
    }

    if (!mContentReceiver.isCompleted())
    {
        return false;
    }

    return true;
}

int32_t HttpChannel::sendHttpResponse(
    HttpResponse& response, const TcpSendHandler& sendHandler)
{
    std::vector<char> responseData;

    // Content-Length
    response.getHeader().setContentLength(
        response.getBody().size());

    // serialize
    StringWriter writer(responseData);
    response.serializeMessage(writer);

    if (!response.getBody().empty())
    {
        response.serializeBody(writer);
    }
    else
    {
        // cut null
        responseData.resize(responseData.size() - 1);
    }

    // send
    int32_t result = send(
        std::move(responseData), sendHandler);

    return result;
}

int32_t HttpChannel::sendHttpResponse(
    uint16_t statusCode,
    const std::string& message,
    const std::string& body,
    const TcpSendHandler& sendHandler)
{
    HttpResponse res;
    res.setStatusCode(statusCode);
    res.setMessage(message);
    res.setBody(body);

    return sendHttpResponse(res, sendHandler);
}

void HttpChannel::onTcpSend(
    const TcpChannelPtr& /* channel */, int32_t /* errorCode */)
{
}

void HttpChannel::onRequestCompleted()
{
    if (mRequestHandler != nullptr)
    {
        HttpChannelPtr self(
            std::dynamic_pointer_cast<HttpChannel>(shared_from_this()));

        mRequestHandler(self);
    }
}

int32_t HttpChannel::sendWsHandshakeResponse(
    const std::string& protocol,
    const TcpSendHandler& handler)
{
    const std::string data =
        getRequest().getHeader().get(
            HttpHeaderField::SecWebSocketKey) + SEV_WS_KEY_SUFFIX;

    auto hash = Sha1::digest(
        data.c_str(), static_cast<uint32_t>(data.length()));
    const std::string b64 = Base64::encode(&hash[0], hash.size());

    HttpResponse httpResponse;
    httpResponse.setStatusCode(
        HttpStatusCode::SwitchingProtocols);
    httpResponse.setMessage("Switching Protocols");
    httpResponse.getHeader().set(
        HttpHeaderField::Upgrade, "websocket");
    httpResponse.getHeader().set(
        HttpHeaderField::Connection, "Upgrade");
    httpResponse.getHeader().set(
        HttpHeaderField::SecWebSocketAccept, b64);
    httpResponse.getHeader().set(
        HttpHeaderField::SecWebSocketProtocol, protocol);

    int32_t result =
        sendHttpResponse(httpResponse, handler);

    return result;
}

WsChannelPtr HttpChannel::upgradeToWebSocket()
{
    if (mWsChannel != nullptr)
    {
        return nullptr;
    }

    mWsChannel = WsChannel::newInstance(shared_from_this(), false);

    return mWsChannel;
}

//----------------------------------------------------------------------------//
// HttpServer
//----------------------------------------------------------------------------//

HttpServer::HttpServer(NetWorker* netWorker)
    : TcpServer(netWorker)
{
    mHandlerMap.setDefaultHandler(HttpServer::defaultHandler);
}

HttpServer::~HttpServer()
{
}

void HttpServer::defaultHandler(const HttpChannelPtr& httpChannel)
{
    httpChannel->sendHttpResponse(
        HttpStatusCode::NotFound, "Not Found");
    httpChannel->close();
}

Socket* HttpServer::createSocket(
    const IpEndPoint& localEndPoint, int32_t& errorCode)
{
    Socket* socket;

#ifdef SEV_SUPPORTS_SSL
    if (mSslContext == nullptr)
    {
        socket = new Socket();
    }
    else
    {
        socket = new SecureSocket(mSslContext);
    }
#else
    socket = new Socket();
#endif

    // create
    if (!socket->create(
        localEndPoint.getFamily(),
        Socket::Type::Tcp,
        Socket::Protocol::Tcp))
    {
        errorCode = socket->getErrorCode();
        delete socket;
        return nullptr;
    }

    // option
    socket->setOption(getSocketOption());

    errorCode = 0;

    return socket;
}

TcpChannelPtr HttpServer::createChannel(Socket* socket)
{
    return TcpChannelPtr(new HttpChannel(socket));
}

bool HttpServer::open(
    const IpEndPoint& localEndPoint,
#ifdef SEV_SUPPORTS_SSL
    const SslContextPtr& sslCtx,
#endif
    const TcpAcceptHandler& acceptHandler,
    int32_t listenBacklog)
{
#ifdef SEV_SUPPORTS_SSL
    mSslContext = sslCtx;
#endif

    TcpAcceptHandler handler = acceptHandler;

    if (handler == nullptr)
    {
        handler = [this](
            const TcpServerPtr&,
            const TcpChannelPtr& channel) {

            if (!accept(mNetWorker, channel))
            {
                return;
            }

            HttpChannelPtr httpChannel =
                std::dynamic_pointer_cast<HttpChannel>(channel);

            httpChannel->setRequestHandler(
                SEV_BIND_1(this, HttpServer::onRequest));
        };
    }

    bool result = TcpServer::open(
        localEndPoint, handler, listenBacklog);

    return result;
}

void HttpServer::onRequest(const HttpChannelPtr& httpChannel)
{
    mHandlerMap.onRequest(httpChannel);
}

void HttpServer::setRequestHandler(
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

void HttpServer::setDefaultRequestHandler(
    const HttpRequestHandler& handler)
{
    mHandlerMap.setDefaultHandler(handler);
}

SEV_NS_END

#endif // SUBEVENT_HTTP_SERVER_INL

