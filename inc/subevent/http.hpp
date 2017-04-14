#ifndef SUBEVENT_HTTP_HPP
#define SUBEVENT_HTTP_HPP

#include <list>
#include <vector>
#include <string>
#include <memory>
#include <iterator>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/buffer_stream.hpp>
#include <subevent/tcp.hpp>

SEV_NS_BEGIN

class NetWorker;
class HttpClient;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef std::shared_ptr<HttpClient> HttpClientPtr;

namespace HttpProtocol
{
    static const std::string v1_0 = "HTTP/1.0";
    static const std::string v1_1 = "HTTP/1.1";
}

typedef std::function<
    void(const HttpClientPtr&, int32_t)> HttpResponseHandler;

//----------------------------------------------------------------------------//
// HttpUrl
//----------------------------------------------------------------------------//

class HttpUrl
{
public:
    SEV_DECL HttpUrl();
    SEV_DECL ~HttpUrl();

public:
    SEV_DECL void setScheme(const std::string& scheme)
    {
        mScheme = scheme;
    }

    SEV_DECL const std::string& getScheme() const
    {
        return mScheme;
    }

    SEV_DECL void setUser(const std::string& user)
    {
        mUser = user;
    }

    SEV_DECL const std::string& getUser() const
    {
        return mUser;
    }

    SEV_DECL void setPassword(const std::string& password)
    {
        mPassword = password;
    }

    SEV_DECL const std::string& getPassword() const
    {
        return mPassword;
    }

    SEV_DECL void setHost(const std::string& host)
    {
        mHost = host;
    }

    SEV_DECL const std::string& getHost() const
    {
        return mHost;
    }

    SEV_DECL void setPort(uint16_t& port)
    {
        mPort = port;
    }

    SEV_DECL uint16_t getPort() const
    {
        return mPort;
    }

    SEV_DECL void setPath(const std::string& path)
    {
        mPath = path;
    }

    SEV_DECL const std::string& getPath() const
    {
        return mPath;
    }

    SEV_DECL void setQuery(const std::string& query)
    {
        mQuery = query;
    }

    SEV_DECL const std::string& getQuery() const
    {
        return mQuery;
    }

    SEV_DECL void setFragment(const std::string& fragment)
    {
        mFragment = fragment;
    }

    SEV_DECL const std::string& getFragment() const
    {
        return mFragment;
    }

public:
    SEV_DECL bool parse(const std::string& url);
    SEV_DECL std::string compose() const;
    SEV_DECL void clear();

private:
    std::string mScheme;
    std::string mUser;
    std::string mPassword;
    std::string mHost;
    uint16_t mPort;
    std::string mPath;
    std::string mQuery;
    std::string mFragment;
};

//----------------------------------------------------------------------------//
// HttpHeader
//----------------------------------------------------------------------------//

class HttpHeader
{
public:
    SEV_DECL HttpHeader();
    SEV_DECL ~HttpHeader();

    struct Field
    {
        std::string name;
        std::string value;
    };

public:
    SEV_DECL void add(
        const std::string& name, const std::string& value);
    SEV_DECL void remove(
        const std::string& name);

    SEV_DECL bool isExists(
        const std::string& name) const;

    SEV_DECL const std::string& findOne(
        const std::string& name) const;
    SEV_DECL std::list<std::string> find(
        const std::string& name) const;

    SEV_DECL const std::list<Field>& getFields() const
    {
        return mFields;
    }

    SEV_DECL void clear();
    SEV_DECL bool isEmpty() const;

public:
    SEV_DECL void setContentLength(size_t contentLength);
    SEV_DECL size_t getContentLength() const;

public:
    SEV_DECL void serialize(OStringStream& os) const;
    SEV_DECL bool deserialize(IStringStream& is);

private:
    std::list<Field> mFields;
};

//----------------------------------------------------------------------------//
// HttpRequest
//----------------------------------------------------------------------------//

class HttpRequest
{
public:
    SEV_DECL HttpRequest();
    SEV_DECL ~HttpRequest();

public:
    SEV_DECL void setMethod(const std::string& method)
    {
        mMethod = method;
    }

    SEV_DECL const std::string& getMethod() const
    {
        return mMethod;
    }

    SEV_DECL void setPath(const std::string& path)
    {
        mPath = path;
    }

    SEV_DECL const std::string& getPath() const
    {
        return mPath;
    }

    SEV_DECL void setProtocol(const std::string& protocol)
    {
        mProtocol = protocol;
    }

    SEV_DECL const std::string& getProtocol() const
    {
        return mProtocol;
    }

    SEV_DECL HttpHeader& getHeader()
    {
        return mHeader;
    }

    SEV_DECL const HttpHeader& getHeader() const
    {
        return mHeader;
    }

public:
    SEV_DECL void setBody(std::vector<char>&& body)
    {
        mBody = std::move(body);
    }

    SEV_DECL void setBody(const std::string& body)
    {
        mBody.clear();

        if (!body.empty())
        {
            std::copy(
                body.begin(), body.end(),
                std::back_inserter(mBody));
        }
    }

    SEV_DECL std::vector<char>& getBody()
    {
        return mBody;
    }

    SEV_DECL const std::vector<char>& getBody() const
    {
        return mBody;
    }

    SEV_DECL std::vector<char>&& moveBody()
    {
        return std::move(mBody);
    }

    SEV_DECL std::string getBodyAsString() const
    {
        return std::string(mBody.begin(), mBody.end());
    }

public:
    SEV_DECL bool isEmpty() const;
    SEV_DECL void clear();

    SEV_DECL void serializeMessage(OStringStream& oss) const;
    SEV_DECL bool deserializeMessage(IStringStream& iss);

    SEV_DECL void serializeBody(OBufferStream& obs) const;
    SEV_DECL bool deserializeBody(IBufferStream& ibs);

private:
    std::string mMethod;
    std::string mPath;
    std::string mProtocol;
    HttpHeader mHeader;
    std::vector<char> mBody;
};

//----------------------------------------------------------------------------//
// HttpResponse
//----------------------------------------------------------------------------//

class HttpResponse
{
public:
    SEV_DECL HttpResponse();
    SEV_DECL ~HttpResponse();

public:
    SEV_DECL void setProtocol(const std::string& protocol)
    {
        mProtocol = protocol;
    }

    SEV_DECL const std::string& getProtocol() const
    {
        return mProtocol;
    }

    SEV_DECL void setStatusCode(uint16_t statusCode)
    {
        mStatusCode = statusCode;
    }

    SEV_DECL uint16_t getStatusCode() const
    {
        return mStatusCode;
    }

    SEV_DECL void setMessage(const std::string& message)
    {
        mMessage = message;
    }

    SEV_DECL const std::string& getMessage() const
    {
        return mMessage;
    }

    SEV_DECL HttpHeader& getHeader()
    {
        return mHeader;
    }

    SEV_DECL const HttpHeader& getHeader() const
    {
        return mHeader;
    }

public:
    SEV_DECL void setBody(std::vector<char>&& body)
    {
        mBody = std::move(body);
    }

    SEV_DECL void setBody(const std::string& body)
    {
        mBody.clear();

        if (!body.empty())
        {
            std::copy(
                body.begin(), body.end(),
                std::back_inserter(mBody));
        }
    }

    SEV_DECL std::vector<char>& getBody()
    {
        return mBody;
    }

    SEV_DECL const std::vector<char>& getBody() const
    {
        return mBody;
    }

    SEV_DECL std::vector<char>&& moveBody()
    {
        return std::move(mBody);
    }

    SEV_DECL std::string getBodyAsString() const
    {
        return std::string(mBody.begin(), mBody.end());
    }

public:
    SEV_DECL void clear();
    SEV_DECL bool isEmpty() const;

    SEV_DECL void serializeMessage(OStringStream& oss) const;
    SEV_DECL bool deserializeMessage(IStringStream& iss);

    SEV_DECL void serializeBody(OBufferStream& obs) const;
    SEV_DECL bool deserializeBody(IBufferStream& ibs);

private:
    std::string mProtocol;
    uint16_t mStatusCode;
    std::string mMessage;
    HttpHeader mHeader;
    std::vector<char> mBody;
};

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

public:

    // GET
    SEV_DECL bool requestGet(
        const std::string& url,
        const HttpResponseHandler& responseHandler,
        const std::string& outputFileName = "");

    // POST
    SEV_DECL bool requestPost(
        const std::string& url,
        const std::string& body,
        const HttpResponseHandler& responseHandler);

    // PUT
    SEV_DECL bool requestPut(
        const std::string& url,
        const std::string& body,
        const HttpResponseHandler& responseHandler);

    // DELETE
    SEV_DECL bool requestDelete(
        const std::string& url,
        const HttpResponseHandler& responseHandler);

    // PATCH
    SEV_DECL bool requestPatch(
        const std::string& url,
        const std::string& body,
        const HttpResponseHandler& responseHandler);

    // HEAD
    SEV_DECL bool requestHead(
        const std::string& url,
        const HttpResponseHandler& responseHandler);

public:

    // Any
    SEV_DECL bool request(
        const std::string& method,
        const std::string& url,
        const std::string& body,
        const HttpResponseHandler& responseHandler,
        const std::string& outputFileName = "");

    SEV_DECL bool isResponseCompleted() const;

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

    SEV_DECL void sendHttpRequest();
    SEV_DECL bool deserializeResponseBody(IBufferStream& ibs);
    SEV_DECL void onHttpResponse(IStringStream& iss);

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
    
    HttpUrl mUrl;

    HttpRequest mRequest;
    HttpResponse mResponse;
    HttpResponseHandler mResponseHandler;

    size_t mReceivedResponseBodySize;
    ChunkedResponse mChunkedResponse;

    std::vector<char> mResponseTempBuffer;

    std::string mOutputFileName;
};

SEV_NS_END

#endif // SUBEVENT_HTTP_HPP
