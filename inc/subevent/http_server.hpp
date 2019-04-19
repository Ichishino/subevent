#ifndef SUBEVENT_HTTP_SERVER_HPP
#define SUBEVENT_HTTP_SERVER_HPP

#include <map>
#include <string>
#include <memory>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/tcp.hpp>
#include <subevent/http.hpp>
#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

class HttpChannel;
class HttpServer;

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

typedef std::shared_ptr<HttpChannel> HttpChannelPtr;
typedef std::shared_ptr<HttpServer> HttpServerPtr;

typedef std::function<
    void(const HttpChannelPtr&)> HttpRequestHandler;

//----------------------------------------------------------------------------//
// HttpHandlerMap
//----------------------------------------------------------------------------//

class HttpHandlerMap
{
public:
    SEV_DECL HttpHandlerMap();
    SEV_DECL ~HttpHandlerMap();

public:
    SEV_DECL void setDefaultHandler(
        const HttpRequestHandler& handler)
    {
        mDefaultHandler = handler;
    }

    SEV_DECL void setHandler(
        const std::string& path,
        const HttpRequestHandler& handler);

    SEV_DECL HttpRequestHandler getHandler(
        const std::string& path) const;

    SEV_DECL void removeHandler(
        const std::string& path);

    SEV_DECL void clear();

public:
    SEV_DECL void onRequest(const HttpChannelPtr& httpChannel);

private:
    SEV_DECL bool isDirectory(const std::string& path) const
    {
        return (path[path.length() - 1] == '/');
    }

    typedef std::map<std::string, HttpRequestHandler> HandlerMap;

    SEV_DECL const HttpRequestHandler& findHandler(
        const HandlerMap& map, const std::string& path) const;

    HandlerMap mFileMap;
    HandlerMap mDirMap;

    HttpRequestHandler mDefaultHandler;
};

//----------------------------------------------------------------------------//
// HttpChannel
//----------------------------------------------------------------------------//

class HttpChannel : public TcpChannel
{
public:
    SEV_DECL virtual ~HttpChannel();

    SEV_DECL int32_t sendHttpResponse(
        HttpResponse& response,
        const TcpSendHandler& sendHandler = nullptr);

    SEV_DECL int32_t sendHttpResponse(
        uint16_t statusCode,
        const std::string& message,
        const std::string& body = "",
        const TcpSendHandler& sendHandler = nullptr);

    SEV_DECL HttpRequest& getRequest()
    {
        return mRequest;
    }

    SEV_DECL void close();

public:

    // WebSocket

    SEV_DECL int32_t sendWsHandshakeResponse(
        const std::string& protocol = "",
        const TcpSendHandler& handler = nullptr);

    SEV_DECL WsChannelPtr upgradeToWebSocket();

public:
    SEV_DECL void setRequestHandler(
        const HttpRequestHandler& requestHandler)
    {
        mRequestHandler = requestHandler;
    }

protected:
    SEV_DECL void onTcpReceive(
        const TcpChannelPtr& channel);
    SEV_DECL void onTcpSend(
        const TcpChannelPtr& channel, int32_t errorCode);

private:
    SEV_DECL HttpChannel(Socket* socket);

    SEV_DECL bool isRequestCompleted() const;
    SEV_DECL bool onHttpRequest(StringReader& reader);
    SEV_DECL void onRequestCompleted();

    HttpChannel() = delete;
    HttpChannel(const HttpChannel&) = delete;
    HttpChannel& operator=(const HttpChannel&) = delete;

    HttpRequest mRequest;
    HttpContentReceiver mContentReceiver;
    std::vector<char> mRequestTempBuffer;
    HttpRequestHandler mRequestHandler;
    WsChannelPtr mWsChannel;

    friend class HttpServer;
};

//----------------------------------------------------------------------------//
// HttpServer
//----------------------------------------------------------------------------//

class HttpServer : public TcpServer
{
public:
    SEV_DECL static HttpServerPtr newInstance(NetWorker* netWorker)
    {
        return HttpServerPtr(new HttpServer(netWorker));
    }

    SEV_DECL ~HttpServer() override;

public:

#ifdef SEV_SUPPORTS_SSL
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const SslContextPtr& sslCtx = nullptr,
        const TcpAcceptHandler& acceptHandler = nullptr,
        int32_t listenBacklog = SOMAXCONN);
#else
    SEV_DECL bool open(
        const IpEndPoint& localEndPoint,
        const TcpAcceptHandler& acceptHandler = nullptr,
        int32_t listenBacklog = SOMAXCONN);

#endif

    SEV_DECL void setRequestHandler(
        const std::string& path,
        const HttpRequestHandler& handler);

    SEV_DECL void setDefaultRequestHandler(
        const HttpRequestHandler& handler);

public:
    SEV_DECL static void defaultHandler(
        const HttpChannelPtr& httpChannel);

protected:
    SEV_DECL void onRequest(
        const HttpChannelPtr& httpChannel);

private:
    SEV_DECL HttpServer(NetWorker* netWorker);

    SEV_DECL virtual Socket* createSocket(
        const IpEndPoint& localEndPoint, int32_t& errorCode) override;
    SEV_DECL TcpChannelPtr createChannel(Socket* socket) override;

    HttpServer() = delete;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    TcpAcceptHandler mAcceptHandler;
    TcpCloseHandler mCloseHandler;
    HttpHandlerMap mHandlerMap;

#ifdef SEV_SUPPORTS_SSL
    SslContextPtr mSslContext;
#endif
};

SEV_NS_END

#endif // SUBEVENT_HTTP_SERVER_HPP
