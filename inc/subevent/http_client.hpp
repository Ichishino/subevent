#ifndef SUBEVENT_HTTP_CLIENT_HPP
#define SUBEVENT_HTTP_CLIENT_HPP

#include <list>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <subevent/std.hpp>
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
        RequestOption()
        {
            clear();
        }

        void clear()
        {
            allowRedirect = true;
            timeout = 60 * 1000;
            outputFileName.clear();
            sockOption.clear();
        }

        bool allowRedirect;
        std::string outputFileName;
        uint32_t timeout;
        SocketOption sockOption;
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
    SEV_DECL const HttpUrl& getUrl() const
    {
        return mUrl;
    }

    SEV_DECL HttpRequest& getRequest()
    {
        return mRequest;
    }

    SEV_DECL HttpResponse& getResponse()
    {
        return mResponse;
    }

private:
    SEV_DECL HttpClient(NetWorker* netWorker);

    SEV_DECL void start();
    SEV_DECL void sendHttpRequest();
    SEV_DECL bool deserializeResponseBody(IBufferStream& ibs);
    SEV_DECL bool isResponseCompleted() const;
    SEV_DECL bool onHttpResponse(IStringStream& iss);
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

    class ChunkedResponse
    {
    public:
        SEV_DECL ChunkedResponse();

    public:
        SEV_DECL void start()
        {
            mRunning = true;
        }

        SEV_DECL void clear();

        SEV_DECL bool setChunkSize(IStringStream& iss);

        SEV_DECL size_t getChunkSize() const
        {
            return mChunkSize;
        }

        SEV_DECL bool isRunning() const
        {
            return mRunning;
        }

        SEV_DECL size_t getRemaining() const
        {
            return ((mChunkSize <= mReceivedSize) ?
                0 : (mChunkSize - mReceivedSize));
        }

        SEV_DECL void received(size_t size)
        {
            mReceivedSize += size;

            if (getRemaining() == 0)
            {
                mChunkSize = 0;
                mReceivedSize = 0;
            }
        }

    private:
        bool mRunning;
        size_t mChunkSize;
        size_t mReceivedSize;
    };

    bool mRunning;

    HttpUrl mUrl;
    HttpResponseHandler mResponseHandler;
    RequestOption mOption;

    HttpRequest mRequest;
    HttpResponse mResponse;

    size_t mReceivedResponseBodySize;
    ChunkedResponse mChunkedResponse;
    std::vector<char> mResponseTempBuffer;
    std::list<std::string> mRedirectHashes;
};

SEV_NS_END

#endif // SUBEVENT_HTTP_CLIENT_HPP
