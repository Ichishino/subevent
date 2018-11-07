#ifndef SUBEVENT_HTTP_SERVER_INL
#define SUBEVENT_HTTP_SERVER_INL

#include <vector>
#include <subevent/http_server.hpp>

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

//----------------------------------------------------------------------------//
// HttpChannel
//----------------------------------------------------------------------------//

HttpChannel::HttpChannel(Socket* socket)
    : TcpChannel(socket)
{
}

HttpChannel::~HttpChannel()
{
}

void HttpChannel::onTcpReceive(std::vector<char>&& message)
{
    auto request = std::move(message);

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

    IStringStream iss(request);

    if (!onHttpRequest(iss))
    {
        // header is not completed
        mRequestTempBuffer = std::move(request);
    }
}

bool HttpChannel::onHttpRequest(IStringStream& iss)
{
    // header
    if (mRequest.isEmpty())
    {
        try
        {
            if (!mRequest.deserializeMessage(iss))
            {
                // not completed
                mRequest.clear();
                return false;
            }
        }
        catch (...)
        {
            // invalid data
            onError(-9001);
            return true;
        }

        mContentReceiver.init(mRequest);
    }

    // body
    if (!mContentReceiver.onReceive(iss))
    {
        onError(-9002);
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
    OStringStream oss(responseData);
    response.serializeMessage(oss);

    if (!response.getBody().empty())
    {
        response.serializeBody(oss);
    }
    else
    {
        // cut null
        responseData.resize(responseData.size() - 1);
    }

    // send
    int32_t result = send(
        std::move(responseData), sendHandler);
    if (result < 0)
    {
    }

    return result;
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

void HttpChannel::onError(int32_t errorCode)
{
    if (mErrorHandler != nullptr)
    {
        HttpChannelPtr self(
            std::dynamic_pointer_cast<HttpChannel>(shared_from_this()));

        mErrorHandler(self, errorCode);
    }
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
    HttpResponse res;
    res.setStatusCode(404);
    res.setMessage("Not Found");

    httpChannel->sendHttpResponse(res);
    httpChannel->close();
}

Socket* HttpServer::createSocket(
    const IpEndPoint& localEndPoint, int32_t& errorCode)
{
    Socket* socket;

    if (mSslContext == nullptr)
    {
        socket = new Socket();
    }
    else
    {
        socket = new SecureSocket(mSslContext);
    }

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
    const TcpAcceptHandler& acceptHandler,
    int32_t listenBacklog)
{
    mAcceptHandler = acceptHandler;

    bool result = TcpServer::open(
        localEndPoint,
        SEV_BIND_2(this, HttpServer::onTcpAccept),
        listenBacklog);

    return result;
}

void HttpServer::onTcpAccept(
    const TcpServerPtr& server, const TcpChannelPtr& channel)
{
    HttpChannelPtr httpChannel =
        std::dynamic_pointer_cast<HttpChannel>(channel);

    if (!accept(mNetWorker, channel))
    {
        return;
    }

    channel->setReceiveHandler(
        SEV_BIND_1(this, HttpServer::onTcpReceive));
    channel->setCloseHandler(
        SEV_BIND_1(this, HttpServer::onTcpClose));

    httpChannel->setRequestHandler(
        SEV_BIND_1(this, HttpServer::onRequest));
    httpChannel->setErrorHandler(
        SEV_BIND_2(this, HttpServer::onError));

    if (mAcceptHandler != nullptr)
    {
        mAcceptHandler(server, channel);
    }
}

void HttpServer::onTcpReceive(const TcpChannelPtr& channel)
{
    auto request = channel->receiveAll();

    if (request.empty())
    {
        return;
    }

    HttpChannelPtr httpChannel =
        std::dynamic_pointer_cast<HttpChannel>(channel);

    httpChannel->onTcpReceive(std::move(request));
}

void HttpServer::onTcpClose(const TcpChannelPtr& channel)
{
    if (mCloseHandler != nullptr)
    {
        mCloseHandler(channel);
    }
}

void HttpServer::onRequest(const HttpChannelPtr& httpChannel)
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
        httpChannel->close();
        onTcpClose(httpChannel);
    }
}

void HttpServer::onError(
    const HttpChannelPtr& httpChannel, int32_t /* errorCode */)
{
    httpChannel->close();
    onTcpClose(httpChannel);
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

void HttpServer::setCloseHandler(
    const TcpCloseHandler& handler)
{
    mCloseHandler = handler;
}

SEV_NS_END

#endif // SUBEVENT_HTTP_SERVER_INL

