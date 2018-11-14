#ifndef SUBEVENT_HTTP_CLIENT_HPP
#define SUBEVENT_HTTP_CLIENT_HPP

#include <list>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/string_io.hpp>
#include <subevent/tcp.hpp>
#include <subevent/http.hpp>

SEV_NS_BEGIN

class NetWorker;
class HttpClient;

typedef std::shared_ptr<HttpClient> HttpClientPtr;

typedef std::function<
    void(const HttpClientPtr&, int32_t)> HttpResponseHandler;

//----------------------------------------------------------------------------//
// HttpClient
//----------------------------------------------------------------------------//

class HttpClient : public TcpClient
{
public:
    SEV_DECL static HttpClientPtr newInstance(NetWorker* netWorker)
    {
        return HttpClientPtr(new HttpClient(netWorker));
    }

    SEV_DECL ~HttpClient() override;

    struct RequestOption
    {
        SEV_DECL RequestOption()
        {
            clear();
        }

        SEV_DECL void clear()
        {
            allowRedirect = true;
            timeout = 60 * 1000;
            outputFileName.clear();
            sockOption.clear();
#ifdef SEV_SUPPORTS_SSL
            sslCtx.reset();
#endif
        }

        bool allowRedirect;
        std::string outputFileName;
        uint32_t timeout;
        SocketOption sockOption;
#ifdef SEV_SUPPORTS_SSL
        SslContextPtr sslCtx;
#endif
    };

public:
    SEV_DECL bool request(
        const std::string& url,
        const HttpResponseHandler& responseHandler,
        const RequestOption& option = RequestOption());

    SEV_DECL static int32_t request(
        const std::string& url,
        const HttpRequest& req,
        HttpResponse& res,
        const RequestOption& option = RequestOption());

public:

    // WebSocket

    SEV_DECL bool requestWsHandshake(
        const std::string& url,
        const std::string& protocols,
        const HttpResponseHandler& responseHandler,
        const RequestOption& option = RequestOption());

    SEV_DECL bool verifyWsHandshakeResponse() const;

    SEV_DECL WsChannelPtr upgradeToWebSocket();

public:
    SEV_DECL const HttpUrl& getUrl() const
    {
        return mUrl;
    }

    SEV_DECL HttpRequest& getRequest()
    {
        return mRequest;
    }

    SEV_DECL const HttpRequest& getRequest() const
    {
        return mRequest;
    }

    SEV_DECL HttpResponse& getResponse()
    {
        return mResponse;
    }

    SEV_DECL const HttpResponse& getResponse() const
    {
        return mResponse;
    }

private:
    SEV_DECL HttpClient(NetWorker* netWorker);

    SEV_DECL void start();
    SEV_DECL void sendHttpRequest();
    SEV_DECL bool isResponseCompleted() const;
    SEV_DECL bool onHttpResponse(StringReader& reader);
    SEV_DECL int32_t redirect();

    SEV_DECL void onTcpConnect(const TcpClientPtr& client, int32_t errorCode);
    SEV_DECL void onTcpSend(const TcpChannelPtr& channel, int32_t errorCode);
    SEV_DECL void onTcpReceive(const TcpChannelPtr& channel);
    SEV_DECL void onTcpClose(const TcpChannelPtr& channel);
    SEV_DECL void onResponse(int32_t errorCode);

    SEV_DECL Socket* createSocket(
        const IpEndPoint& peerEndPoint, int32_t& errorCode) override;

    HttpClient() = delete;
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

private:
    bool mRunning;

    HttpUrl mUrl;
    HttpResponseHandler mResponseHandler;
    RequestOption mOption;

    HttpRequest mRequest;
    HttpResponse mResponse;

    HttpContentReceiver mContentReceiver;
    std::vector<char> mResponseTempBuffer;
    std::list<std::string> mRedirectHashes;

    WsChannelPtr mWsChannel;

#ifdef SEV_SUPPORTS_SSL
    SslContextPtr mSslContext;
#endif
};

SEV_NS_END

#endif // SUBEVENT_HTTP_CLIENT_HPP
