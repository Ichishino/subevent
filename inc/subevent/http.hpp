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

namespace HttpMethod
{
    static const std::string Get = "GET";
    static const std::string Post = "POST";
    static const std::string Put = "PUT";
    static const std::string Delete = "DELETE";
    static const std::string Patch = "PATCH";
    static const std::string Head = "HEAD";
};

namespace HttpProtocol
{
    static const std::string v1_0 = "HTTP/1.0";
    static const std::string v1_1 = "HTTP/1.1";
}

namespace HttpHeaderField
{
    static const std::string Accept = "Accept";
    static const std::string AcceptEncoding = "Accept-Encoding";
    static const std::string Connection = "Connection";
    static const std::string ContentLength = "Content-Length";
    static const std::string ContentType = "Content-Type";
    static const std::string Host = "Host";
    static const std::string TransferEncoding = "Transfer-Encoding";
    static const std::string Cookie = "Cookie";
    static const std::string SetCookie = "Set-Cookie";
    static const std::string Location = "Location";
    static const std::string UserAgent = "User-Agent";
}

namespace HttpCookieAttr
{
    static const std::string Expires = "Expires";
    static const std::string MaxAge = "Max-Age";
    static const std::string Domain = "Domain";
    static const std::string Path = "Path";
    static const std::string Secure = "Secure";
    static const std::string HttpOnly = "HttpOnly";
}

typedef std::shared_ptr<HttpClient> HttpClientPtr;

typedef std::function<
    void(const HttpClientPtr&, int32_t)> HttpResponseHandler;

//----------------------------------------------------------------------------//
// HttpUrl
//----------------------------------------------------------------------------//

class HttpUrl
{
public:
    SEV_DECL HttpUrl();
    SEV_DECL HttpUrl(const HttpUrl& other);
    SEV_DECL HttpUrl(HttpUrl&& other);
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
    SEV_DECL std::string composePath() const;
    SEV_DECL void clear();

    SEV_DECL HttpUrl& operator=(const HttpUrl& other);
    SEV_DECL HttpUrl& operator=(HttpUrl&& other);

public:
    SEV_DECL static std::string encode(
        const std::string& src, const std::string& ignoreChars = "");
    SEV_DECL static std::string decode(
        const std::string& src);

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
// HttpCookie
//----------------------------------------------------------------------------//

class HttpCookie
{
public:
    SEV_DECL HttpCookie();
    SEV_DECL HttpCookie(const HttpCookie& other);
    SEV_DECL HttpCookie(HttpCookie&& other);
    SEV_DECL ~HttpCookie();

    struct Attribute
    {
        std::string name;
        std::string value;

        std::string compose() const
        {
            return (name + "=" + value);
        }
    };

public:
    SEV_DECL void set(
        const std::string& name,
        const std::string& value);
    SEV_DECL const std::string& get(
        const std::string& name) const;

    SEV_DECL const std::list<Attribute>& getAll() const
    {
        return mAttributes;
    }

    SEV_DECL void add(
        const std::string& name, const std::string& value);
    SEV_DECL void remove(
        const std::string& name);
    SEV_DECL std::list<std::string> find(
        const std::string& name) const;

    SEV_DECL bool isExists(
        const std::string& name) const;
    SEV_DECL bool isEmpty() const;
    SEV_DECL void clear();

public:
    SEV_DECL void setExipires(const std::string& expires)
    {
        mExpires = expires;
    }

    SEV_DECL const std::string& getExpires() const
    {
        return mExpires;
    }

    SEV_DECL void setMaxAge(const std::string& maxAge)
    {
        mMaxAge = maxAge;
    }

    SEV_DECL const std::string& getMaxAge() const
    {
        return mMaxAge;
    }

    SEV_DECL void setDomain(const std::string& domain)
    {
        mDomain = domain;
    }

    SEV_DECL const std::string& getDomain() const
    {
        return mDomain;
    }

    SEV_DECL void setPath(const std::string& path)
    {
        mPath = path;
    }

    SEV_DECL const std::string& getPath() const
    {
        return mPath;
    }

    SEV_DECL void setSecure(bool secure)
    {
        mSecure = secure;
    }

    SEV_DECL bool isSecure() const
    {
        return mSecure;
    }

    SEV_DECL void setHttpOnly(bool httpOnly)
    {
        mHttpOnly = httpOnly;
    }

    SEV_DECL bool isHttpOnly() const
    {
        return mHttpOnly;
    }

public:
    SEV_DECL bool parse(const std::string& url);
    SEV_DECL std::string compose() const;

    SEV_DECL HttpCookie& operator=(const HttpCookie& other);
    SEV_DECL HttpCookie& operator=(HttpCookie&& other);

private:
    std::list<Attribute> mAttributes;
    std::string mExpires;
    std::string mMaxAge;
    std::string mDomain;
    std::string mPath;
    bool mSecure;
    bool mHttpOnly;
};

//----------------------------------------------------------------------------//
// HttpHeader
//----------------------------------------------------------------------------//

class HttpHeader
{
public:
    SEV_DECL HttpHeader();
    SEV_DECL HttpHeader(const HttpHeader& other);
    SEV_DECL HttpHeader(HttpHeader&& other);
    SEV_DECL ~HttpHeader();

    struct Field
    {
        std::string name;
        std::string value;
    };

public:
    SEV_DECL void set(
        const std::string& name,
        const std::string& value);
    SEV_DECL const std::string& get(
        const std::string& name) const;

    SEV_DECL const std::list<Field>& getAll() const
    {
        return mFields;
    }

    SEV_DECL void add(
        const std::string& name, const std::string& value);
    SEV_DECL void remove(
        const std::string& name);
    SEV_DECL std::list<std::string> find(
        const std::string& name) const;

    SEV_DECL bool isExists(
        const std::string& name) const;
    SEV_DECL bool isEmpty() const;
    SEV_DECL void clear();

public:
    SEV_DECL void setContentLength(size_t contentLength);
    SEV_DECL size_t getContentLength() const;

public:
    SEV_DECL void serialize(OStringStream& oss) const;
    SEV_DECL bool deserialize(IStringStream& iss);

    SEV_DECL HttpHeader& operator=(const HttpHeader& other);
    SEV_DECL HttpHeader& operator=(HttpHeader&& other);

private:
    std::list<Field> mFields;
};

//----------------------------------------------------------------------------//
// HttpMessage
//----------------------------------------------------------------------------//

class HttpMessage
{
public:
    SEV_DECL virtual ~HttpMessage();

public:
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

    SEV_DECL virtual bool isEmpty() const;
    SEV_DECL virtual void clear();

public:
    SEV_DECL virtual void serializeMessage(OStringStream& oss) const;
    SEV_DECL virtual bool deserializeMessage(IStringStream& iss);

    SEV_DECL virtual void serializeBody(OBufferStream& obs) const;
    SEV_DECL virtual bool deserializeBody(IBufferStream& ibs);

protected:
    SEV_DECL HttpMessage();
    SEV_DECL HttpMessage(const HttpMessage& other);
    SEV_DECL HttpMessage(HttpMessage&& other);

    SEV_DECL void addCookie(
        const std::string& headerName, const HttpCookie& cookie);
    SEV_DECL std::list<HttpCookie> getCookies(
        const std::string& headerName) const;
    SEV_DECL void removeCookies(
        const std::string& headerName);

    SEV_DECL HttpMessage& operator=(const HttpMessage& other);
    SEV_DECL HttpMessage& operator=(HttpMessage&& other);

private:
    HttpHeader mHeader;
    std::vector<char> mBody;
};

//----------------------------------------------------------------------------//
// HttpRequest
//----------------------------------------------------------------------------//

class HttpRequest : public HttpMessage
{
public:
    SEV_DECL HttpRequest();
    SEV_DECL HttpRequest(const HttpRequest& other);
    SEV_DECL HttpRequest(HttpRequest&& other);
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

    SEV_DECL bool isEmpty() const override;
    SEV_DECL void clear() override;

public:
    SEV_DECL void addCookie(const HttpCookie& cookie)
    {
        return HttpMessage::addCookie(
            HttpHeaderField::Cookie, cookie);
    }

    SEV_DECL std::list<HttpCookie> getCookies() const
    {
        return HttpMessage::getCookies(
            HttpHeaderField::Cookie);
    }

    SEV_DECL void removeCookies()
    {
        return HttpMessage::removeCookies(
            HttpHeaderField::Cookie);
    }

public:
    SEV_DECL void serializeMessage(OStringStream& oss) const override;
    SEV_DECL bool deserializeMessage(IStringStream& iss) override;

    SEV_DECL HttpRequest& operator=(const HttpRequest& other);
    SEV_DECL HttpRequest& operator=(HttpRequest&& other);

private:
    std::string mMethod;
    std::string mPath;
    std::string mProtocol;
};

//----------------------------------------------------------------------------//
// HttpResponse
//----------------------------------------------------------------------------//

class HttpResponse : public HttpMessage
{
public:
    SEV_DECL HttpResponse();
    SEV_DECL HttpResponse(const HttpResponse& other);
    SEV_DECL HttpResponse(HttpResponse&& other);
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

    SEV_DECL void clear();
    SEV_DECL bool isEmpty() const;

public:
    SEV_DECL void addCookie(const HttpCookie& cookie)
    {
        return HttpMessage::addCookie(
            HttpHeaderField::SetCookie, cookie);
    }

    SEV_DECL std::list<HttpCookie> getCookies() const
    {
        return HttpMessage::getCookies(
            HttpHeaderField::SetCookie);
    }

    SEV_DECL void removeCookies()
    {
        return HttpMessage::removeCookies(
            HttpHeaderField::SetCookie);
    }

public:
    SEV_DECL void serializeMessage(OStringStream& oss) const;
    SEV_DECL bool deserializeMessage(IStringStream& iss);

    SEV_DECL HttpResponse& operator=(const HttpResponse& other);
    SEV_DECL HttpResponse& operator=(HttpResponse&& other);

private:
    std::string mProtocol;
    uint16_t mStatusCode;
    std::string mMessage;
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

#endif // SUBEVENT_HTTP_HPP
