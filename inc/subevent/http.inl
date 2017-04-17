#ifndef SUBEVENT_HTTP_INL
#define SUBEVENT_HTTP_INL

#include <cstdio>
#include <fstream>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <cctype>

#include <subevent/network.hpp>
#include <subevent/http.hpp>
#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

inline void trimString(std::string& str)
{
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
}

inline bool icompString(const std::string& left, const std::string& right)
{
    return ((left.size() == right.size()) &&
        std::equal(left.begin(), left.end(), right.begin(),
            [](char l, char r) ->bool {
                return (std::tolower(l) == std::tolower(r));
            }));
}

//----------------------------------------------------------------------------//
// HttpUrl
//----------------------------------------------------------------------------//

HttpUrl::HttpUrl()
{
    clear();
}

HttpUrl::HttpUrl(const HttpUrl& other)
{
    operator=(other);
}

HttpUrl::HttpUrl(HttpUrl&& other)
{
    operator=(std::move(other));
}

HttpUrl::~HttpUrl()
{
}

bool HttpUrl::parse(const std::string& url)
{
    clear();

    std::string src = url;
    size_t pos;

    // fragment
    pos = src.find_last_of('#');

    if (pos != std::string::npos)
    {
        mFragment = src.substr(pos + 1);
        src = src.substr(0, pos);
    }

    // query
    pos = src.find_last_of('?');

    if (pos != std::string::npos)
    {
        mQuery = src.substr(pos + 1);
        src = src.substr(0, pos);
    }

    // scheme
    pos = src.find("://");

    if (pos != std::string::npos)
    {
        mScheme = src.substr(0, pos);
        src = src.substr(pos + 3);
    }

    // auth
    pos = src.find("@");

    if (pos != std::string::npos)
    {
        std::string auth = src.substr(0, pos);
        src = src.substr(pos + 1);

        pos = auth.find_first_of(':');

        if (pos != std::string::npos)
        {
            mPassword = auth.substr(pos + 1);
        }

        mUser = auth.substr(0, pos);

        if (mUser.empty())
        {
            return false;
        }
    }

    // path
    pos = src.find("/");

    if (pos != std::string::npos)
    {
        mPath = src.substr(pos);
    }

    src = src.substr(0, pos);

    // port
    pos = src.find_first_of(':');

    if (pos != std::string::npos)
    {
        std::string portStr = src.substr(pos + 1);
        src = src.substr(0, pos);

        if (portStr.empty())
        {
            return false;
        }

        try
        {
            int32_t port = std::stoi(portStr);

            if ((port <= 0) || (port > 65535))
            {
                return false;
            }

            mPort = static_cast<uint16_t>(port);
        }
        catch (...)
        {
            return false;
        }
    }
    else
    {
        if (mScheme == "https")
        {
            mPort = 443;
        }
        else
        {
            mPort = 80;
        }
    }

    if (src.empty())
    {
        return false;
    }

    // host
    mHost = src;

    return true;
}

std::string HttpUrl::compose() const
{
    std::string url;
    
    url = mScheme + "://";
    
    if (!mUser.empty() && !mPassword.empty())
    {
        url += mUser + ":" + mPassword + "@";
    }

    url += mHost;

    if ((mPort != 80) && (mPort != 443))
    {
        url += ":" + std::to_string(mPort);
    }

    url += composePath();

    return url;
}

std::string HttpUrl::composePath() const
{
    std::string path;

    path += mPath;

    if (!mQuery.empty())
    {
        path += "?" + mQuery;
    }

    if (!mFragment.empty())
    {
        path += "#" + mFragment;
    }

    return path;
}

void HttpUrl::clear()
{
    mScheme = "http";
    mUser.clear();
    mPassword.clear();
    mHost.clear();
    mPort = 0;
    mPath = "/";
    mQuery.clear();
    mFragment.clear();
}

HttpUrl& HttpUrl::operator=(const HttpUrl& other)
{
    mScheme = other.mScheme;
    mUser = other.mUser;
    mPassword = other.mPassword;
    mHost = other.mHost;
    mPort = other.mPort;
    mPath = other.mPath;
    mQuery = other.mQuery;
    mFragment = other.mFragment;

    return *this;
}

HttpUrl& HttpUrl::operator=(HttpUrl&& other)
{
    mScheme = std::move(other.mScheme);
    mUser = std::move(other.mUser);
    mPassword = std::move(other.mPassword);
    mHost = std::move(other.mHost);
    mPort = std::move(other.mPort);
    mPath = std::move(other.mPath);
    mQuery = std::move(other.mQuery);
    mFragment = std::move(other.mFragment);

    other.clear();

    return *this;
}

//----------------------------------------------------------------------------//
// HttpHeader
//----------------------------------------------------------------------------//

HttpHeader::HttpHeader()
{
}

HttpHeader::HttpHeader(const HttpHeader& other)
{
    operator=(other);
}

HttpHeader::HttpHeader(HttpHeader&& other)
{
    operator=(std::move(other));
}

HttpHeader::~HttpHeader()
{
}

void HttpHeader::add(
    const std::string& name, const std::string& value)
{
    if (!value.empty())
    {
        mFields.push_back({ name, value });
    }
}

void HttpHeader::remove(const std::string& name)
{
    auto it = std::remove_if(
        mFields.begin(), mFields.end(),
        [&](const Field& field) {
            return icompString(field.name, name);
        });

    if (it == mFields.end())
    {
        return;
    }

    mFields.erase(it, mFields.end());
}

void HttpHeader::clear()
{
    mFields.clear();
}

bool HttpHeader::isEmpty() const
{
    return mFields.empty();
}

bool HttpHeader::isExists(
    const std::string& name) const
{
    for (const auto& field : mFields)
    {
        if (icompString(field.name, name))
        {
            return true;
        }
    }

    return false;
}

const std::string& HttpHeader::findValue(
    const std::string& name) const
{
    for (const auto& field : mFields)
    {
        if (icompString(field.name, name))
        {
            return field.value;
        }
    }

    static const std::string emptyString;

    return emptyString;
}

std::list<std::string> HttpHeader::findValues(
    const std::string& name) const
{
    std::list<std::string> values;

    for (const auto& field : mFields)
    {
        if (icompString(field.name, name))
        {
            values.push_back(field.value);
        }
    }

    return values;
}

void HttpHeader::serialize(OStringStream& oss) const
{
    for (const auto& field : mFields)
    {
        oss << field.name << ":" << field.value << "\r\n";
    }

    oss << "\r\n";
}

bool HttpHeader::deserialize(IStringStream& iss)
{
    clear();

    for (;;)
    {
        std::string line;
        
        if (!iss.readString(line, "\r\n"))
        {
            return false;
        }
        iss.seekCur(+2);

        if (line.empty())
        {
            // success
            break;
        }

        size_t pos = line.find_first_of(':');

        if (pos == std::string::npos)
        {
            return false;
        }

        // name
        std::string name = line.substr(0, pos);

        trimString(name);

        if (name.empty())
        {
            return false;
        }

        // value
        std::string value = line.substr(pos + 1);

        trimString(value);

        if (value.empty())
        {
            return false;
        }

        add(name, value);
    }

    return true;
}

void HttpHeader::setContentLength(size_t contentLength)
{
    remove(HttpHeaderField::ContentLength);

    add(HttpHeaderField::ContentLength,
        std::to_string(contentLength));
}

size_t HttpHeader::getContentLength() const
{
    size_t contentLength = 0;

    std::string contentLengthStr =
        findValue(HttpHeaderField::ContentLength);

    if (!contentLengthStr.empty())
    {
        contentLength = std::stol(contentLengthStr);
    }

    return contentLength;
}

HttpHeader& HttpHeader::operator=(const HttpHeader& other)
{
    mFields = other.mFields;

    return *this;
}

HttpHeader& HttpHeader::operator=(HttpHeader&& other)
{
    mFields = std::move(other.mFields);

    other.clear();

    return *this;
}

//----------------------------------------------------------------------------//
// HttpCookie
//----------------------------------------------------------------------------//

HttpCookie::HttpCookie()
{
    clear();
}

HttpCookie::HttpCookie(const HttpCookie& other)
{
    operator=(other);
}

HttpCookie::HttpCookie(HttpCookie&& other)
{
    operator=(std::move(other));
}

HttpCookie::~HttpCookie()
{
}

void HttpCookie::add(
    const std::string& name, const std::string& value)
{
    if (!value.empty())
    {
        mAttributes.push_back({ name, value });
    }
}

void HttpCookie::remove(const std::string& name)
{
    auto it = std::remove_if(
        mAttributes.begin(), mAttributes.end(),
        [&](const Attribute& attr) {
            return icompString(attr.name, name);
        });

    if (it == mAttributes.end())
    {
        return;
    }

    mAttributes.erase(it, mAttributes.end());
}

bool HttpCookie::isExists(
    const std::string& name) const
{
    for (const auto& attr : mAttributes)
    {
        if (icompString(attr.name, name))
        {
            return true;
        }
    }

    return false;
}

const std::string& HttpCookie::findValue(
    const std::string& name) const
{
    for (const auto& attr : mAttributes)
    {
        if (icompString(attr.name, name))
        {
            return attr.value;
        }
    }

    static const std::string emptyString;

    return emptyString;
}

std::list<std::string> HttpCookie::findValues(
    const std::string& name) const
{
    std::list<std::string> results;

    for (const auto& attr : mAttributes)
    {
        if (icompString(attr.name, name))
        {
            results.push_back(attr.name);
        }
    }

    return results;
}

bool HttpCookie::parse(const std::string& cookie)
{
    std::string src = cookie;

    while (!src.empty())
    {
        std::string nameAndValue;
        size_t pos;

        pos = src.find_first_of(';');

        if (pos == std::string::npos)
        {
            nameAndValue = src;
            src.clear();
        }
        else
        {
            nameAndValue = src.substr(0, pos);
            src = src.substr(pos + 1);
        }

        std::string name;
        std::string value;

        // name
        pos = nameAndValue.find_first_of('=');

        if (pos != std::string::npos)
        {
            name = nameAndValue.substr(0, pos);
            trimString(name);

            // value
            value = nameAndValue.substr(pos + 1);
            trimString(value);
        }
        else
        {
            // name only
            name = nameAndValue;
            trimString(name);
        }

        if (name.empty())
        {
            return false;
        }

        if (icompString(name, HttpCookieName::Expires))
        {
            mExpires = value;
        }
        else if (icompString(name, HttpCookieName::MaxAge))
        {
            mMaxAge = value;
        }
        else if (icompString(name, HttpCookieName::Domain))
        {
            mDomain = value;
        }
        else if (icompString(name, HttpCookieName::Path))
        {
            mPath = value;
        }
        else if (icompString(name, HttpCookieName::Secure))
        {
            mSecure = true;
        }
        else if (icompString(name, HttpCookieName::HttpOnly))
        {
            mHttpOnly = true;
        }
        else if (!value.empty())
        {
            add(name, value);
        }
    }

    return true;
}

std::string HttpCookie::compose() const
{
    std::string cookie;

    for (auto& attr : mAttributes)
    {
        cookie += attr.name + "=" + attr.value;
        cookie += "; ";
    }

    if (!mExpires.empty())
    {
        cookie += HttpCookieName::Expires + "=" + mExpires;
        cookie += "; ";
    }

    if (!mMaxAge.empty())
    {
        cookie += HttpCookieName::MaxAge + "=" + mMaxAge;
        cookie += "; ";
    }

    if (!mDomain.empty())
    {
        cookie += HttpCookieName::Domain + "=" + mDomain;
        cookie += "; ";
    }

    if (!mPath.empty())
    {
        cookie += HttpCookieName::Path + "=" + mPath;
        cookie += "; ";
    }

    if (mSecure)
    {
        cookie += HttpCookieName::Secure;
        cookie += "; ";
    }

    if (mHttpOnly)
    {
        cookie += HttpCookieName::HttpOnly;
        cookie += "; ";
    }

    if (!cookie.empty())
    {
        cookie.resize(cookie.size() - 2);
    }

    return cookie;
}

void HttpCookie::clear()
{
    mAttributes.clear();
    mExpires.clear();
    mMaxAge.clear();
    mDomain.clear();
    mPath.clear();
    mSecure = false;
    mHttpOnly = false;
}

HttpCookie& HttpCookie::operator=(const HttpCookie& other)
{
    mAttributes = other.mAttributes;
    mExpires = other.mExpires;
    mMaxAge = other.mMaxAge;
    mDomain = other.mDomain;
    mPath = other.mPath;
    mSecure = other.mSecure;
    mHttpOnly = other.mHttpOnly;

    return *this;
}

HttpCookie& HttpCookie::operator=(HttpCookie&& other)
{
    mAttributes = std::move(other.mAttributes);
    mExpires = std::move(other.mExpires);
    mMaxAge = std::move(other.mMaxAge);
    mDomain = std::move(other.mDomain);
    mPath = std::move(other.mPath);
    mSecure = std::move(other.mSecure);
    mHttpOnly = std::move(other.mHttpOnly);

    other.clear();

    return *this;
}

//----------------------------------------------------------------------------//
// HttpMessage
//----------------------------------------------------------------------------//

HttpMessage::HttpMessage()
{
    clear();
}

HttpMessage::HttpMessage(const HttpMessage& other)
{
    operator=(other);
}

HttpMessage::HttpMessage(HttpMessage&& other)
{
    operator=(std::move(other));
}

HttpMessage::~HttpMessage()
{
}

void HttpMessage::clear()
{
    mHeader.clear();
    mBody.clear();
}

bool HttpMessage::isEmpty() const
{
    return (mHeader.isEmpty() && mBody.empty());
}

void HttpMessage::addCookie(
    const std::string& headerName, const HttpCookie& cookie)
{
    mHeader.add(headerName, cookie.compose());
}

std::list<HttpCookie> HttpMessage::getCookies(
    const std::string& headerName) const
{
    std::list<HttpCookie> results;

    for (auto& field : mHeader.findValues(headerName))
    {
        HttpCookie cookie;

        if (cookie.parse(field))
        {
            results.push_back(std::move(cookie));
        }
    }

    return results;
}

void HttpMessage::removeCookies(
    const std::string& headerName)
{
    mHeader.remove(headerName);
}

void HttpMessage::serializeMessage(OStringStream& oss) const
{
    // header
    mHeader.serialize(oss);
}

bool HttpMessage::deserializeMessage(IStringStream& iss)
{
    // header
    if (!mHeader.deserialize(iss))
    {
        return false;
    }

    return true;
}

void HttpMessage::serializeBody(OBufferStream& obs) const
{
    // body
    if (!mBody.empty())
    {
        obs.writeBytes(&mBody[0], mBody.size());
    }
}

bool HttpMessage::deserializeBody(IBufferStream& ibs)
{
    // body
    if (!ibs.isEnd())
    {
        mBody.resize(ibs.getReadableSize());

        if (!ibs.readBytes(&mBody, mBody.size()))
        {
            return false;
        }
    }

    return true;
}

HttpMessage& HttpMessage::operator=(const HttpMessage& other)
{
    mHeader = other.mHeader;
    mBody = other.mBody;

    return *this;
}

HttpMessage& HttpMessage::operator=(HttpMessage&& other)
{
    mHeader = std::move(other.mHeader);
    mBody = std::move(other.mBody);

    return *this;
}

//----------------------------------------------------------------------------//
// HttpRequest
//----------------------------------------------------------------------------//

HttpRequest::HttpRequest()
{
    clear();
}

HttpRequest::HttpRequest(const HttpRequest& other)
{
    operator=(other);
}

HttpRequest::HttpRequest(HttpRequest&& other)
{
    operator=(std::move(other));
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::clear()
{
    HttpMessage::clear();

    mMethod.clear();
    mPath = "/";
    mProtocol = HttpProtocol::v1_1;
}

bool HttpRequest::isEmpty() const
{
    if (!HttpMessage::isEmpty())
    {
        return false;
    }

    return mMethod.empty();
}

void HttpRequest::serializeMessage(OStringStream& oss) const
{
    // method + path + protocol
    oss << mMethod << " "
        << mPath << " "
        << mProtocol << "\r\n";

    HttpMessage::serializeMessage(oss);
}

bool HttpRequest::deserializeMessage(IStringStream& iss)
{
    std::string line;

    if (!iss.readString(line, "\r\n"))
    {
        return false;
    }
    iss.seekCur(+2);
        
    size_t pos1 = line.find_first_of(' ');

    if (pos1 == std::string::npos)
    {
        return false;
    }
    
    // method
    mMethod = line.substr(0, pos1);

    if (mMethod.empty())
    {
        return false;
    }

    size_t pos2 = line.find_first_of(' ', pos1 + 1);

    if (pos2 == std::string::npos)
    {
        return false;
    }

    // path
    mPath = line.substr(pos1 + 1, pos2 - pos1 - 1);

    if (mPath.empty())
    {
        return false;
    }

    // protocol
    mProtocol = line.substr(pos2 + 1);

    if (mProtocol.empty())
    {
        return false;
    }

    if (!HttpMessage::deserializeMessage(iss))
    {
        return false;
    }
    
    return true;
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
    HttpMessage::operator=(other);

    mMethod = other.mMethod;
    mPath = other.mPath;
    mProtocol = other.mProtocol;

    return *this;
}

HttpRequest& HttpRequest::operator=(HttpRequest&& other)
{
    HttpMessage::operator=(std::move(other));

    mMethod = std::move(other.mMethod);
    mPath = std::move(other.mPath);
    mProtocol = std::move(other.mProtocol);

    other.clear();

    return *this;
}

//----------------------------------------------------------------------------//
// HttpResponse
//----------------------------------------------------------------------------//

HttpResponse::HttpResponse()
{
    clear();
}

HttpResponse::HttpResponse(const HttpResponse& other)
{
    operator=(other);
}

HttpResponse::HttpResponse(HttpResponse&& other)
{
    operator=(std::move(other));
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::clear()
{
    HttpMessage::clear();

    mProtocol = HttpProtocol::v1_1;
    mStatusCode = 0;
    mMessage.clear();
}

bool HttpResponse::isEmpty() const
{
    if (!HttpMessage::isEmpty())
    {
        return false;
    }

    return (mStatusCode == 0);
}

void HttpResponse::serializeMessage(OStringStream& oss) const
{
    // protocol + status code + message
    oss << mProtocol << " "
        << mStatusCode << " "
        << mMessage << "\r\n";

    HttpMessage::serializeMessage(oss);
}

bool HttpResponse::deserializeMessage(IStringStream& iss)
{
    std::string line;

    if (!iss.readString(line, "\r\n"))
    {
        return false;
    }
    iss.seekCur(+2);

    size_t pos1 = line.find_first_of(' ');

    if (pos1 == std::string::npos)
    {
        return false;
    }

    // protocol
    mProtocol = line.substr(0, pos1);

    if (mProtocol.empty())
    {
        return false;
    }

    size_t pos2 = line.find_first_of(' ', pos1 + 1);

    if (pos2 == std::string::npos)
    {
        return false;
    }

    std::string statusCode =
        line.substr(pos1 + 1, pos2 - pos1 - 1);

    if (statusCode.empty())
    {
        return false;
    }

    // status code
    mStatusCode =
        static_cast<uint16_t>(std::stoi(statusCode));

    // message
    mMessage = line.substr(pos2 + 1);

    if (!HttpMessage::deserializeMessage(iss))
    {
        return false;
    }

    return true;
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other)
{
    HttpMessage::operator=(other);

    mProtocol = other.mProtocol;
    mStatusCode = other.mStatusCode;
    mMessage = other.mMessage;

    return *this;
}

HttpResponse& HttpResponse::operator=(HttpResponse&& other)
{
    HttpMessage::operator=(std::move(other));

    mProtocol = std::move(other.mProtocol);
    mStatusCode = other.mStatusCode;
    mMessage = std::move(other.mMessage);

    other.clear();

    return *this;
}

//----------------------------------------------------------------------------//
// HttpClient::ChunkedResponse
//----------------------------------------------------------------------------//

HttpClient::ChunkedResponse::ChunkedResponse()
{
    clear();
}

void HttpClient::ChunkedResponse::clear()
{
    mRunning = false;
    mChunkSize = 0;
    mReceivedSize = 0;
}

bool HttpClient::ChunkedResponse::setChunkSize(IStringStream& iss)
{
    std::string chunkSizeStr;

    if (!iss.readString(chunkSizeStr, "\r\n"))
    {
        return false;
    }

    iss.seekCur(+2);

    try
    {
        mChunkSize = std::stoul(chunkSizeStr, nullptr, 16);
    }
    catch (...)
    {
        return false;
    }

    mReceivedSize = 0;

    return true;
}

//----------------------------------------------------------------------------//
// HttpClient
//----------------------------------------------------------------------------//

HttpClient::HttpClient(NetWorker* netWorker)
    : TcpClient(netWorker)
{
    mRunning = false;
    mReceivedResponseBodySize = 0;
}

HttpClient::~HttpClient()
{
}

bool HttpClient::request(
    const std::string& url,
    const HttpResponseHandler& responseHandler,
    const RequestOption& option)
{
    if (mRunning)
    {
        return false;
    }

    mUrl.clear();
    mResponse.clear();
    mReceivedResponseBodySize = 0;
    mChunkedResponse.clear();
    mResponseTempBuffer.clear();
    mOption.clear();
    mRedirectHashes.clear();

    if (mRequest.getMethod().empty())
    {
        return false;
    }

    HttpUrl httpUrl;

    if (!httpUrl.parse(url))
    {
        return false;
    }

    if ((httpUrl.getScheme() != mUrl.getScheme()) ||
        (httpUrl.getHost() != mUrl.getHost()) ||
        (httpUrl.getPort() != mUrl.getPort()))
    {
        close();
    }

    mUrl = std::move(httpUrl);
    mResponseHandler = responseHandler;
    mOption = option;

    start();

    return true;
}

int32_t HttpClient::request(
    const std::string& url,
    const HttpRequest& req,
    HttpResponse& res,
    const RequestOption& option)
{
    res.clear();

    int32_t result = -1;

    NetThread thread;

    if (!thread.start())
    {
        return -8601;
    }

    Semaphore sem;

    HttpClientPtr http = newInstance(&thread);

    if (req.getMethod().empty())
    {
        return -8602;
    }

    if (!http->mUrl.parse(url))
    {
        return -8603;
    }

    http->mRequest = req;
    http->mOption = option;
    http->mResponseHandler =
        [&, http](const HttpClientPtr&, int32_t errorCode) {

        // response
        result = errorCode;
        res = std::move(http->mResponse);

        sem.post();
    };

    // start
    thread.post([http]() {
        http->start();
    });

    // wait
    if (sem.wait(option.timeout) == WaitResult::Timeout)
    {
        result = -8604;
    }

    thread.stop();
    thread.wait();

    http->mResponseHandler = nullptr;

    return  result;
}

void HttpClient::start()
{
    mRunning = true;

    getSocketOption() = mOption.sockOption;

    if (isClosed())
    {
        // connect
        connect(mUrl.getHost(), mUrl.getPort(),
            SEV_MFN2(HttpClient::onTcpConnect));

        setReceiveHandler(
            SEV_MFN1(HttpClient::onTcpReceive));
        setCloseHandler(
            SEV_MFN1(HttpClient::onTcpClose));
    }
    else
    {
        // send
        sendHttpRequest();
    }
}

bool HttpClient::isResponseCompleted() const
{
    if (mResponse.isEmpty())
    {
        return false;
    }

    if (mChunkedResponse.isRunning())
    {
        return false;
    }

    if (mReceivedResponseBodySize <
        mResponse.getHeader().getContentLength())
    {
        if (mRequest.getMethod() != HttpMethod::Head)
        {
            return false;
        }
    }

    return true;
}

void HttpClient::sendHttpRequest()
{
    // path
    mRequest.setPath(mUrl.composePath());

    // Host
    if (!mRequest.getHeader().isExists(HttpHeaderField::Host))
    {
        mRequest.getHeader().add(
            HttpHeaderField::Host, mUrl.getHost());
    }

    // Content-Length
    if (!mRequest.getBody().empty())
    {
        mRequest.getHeader().setContentLength(
            mRequest.getBody().size());
    }

    std::vector<char> requestData;

    // serialize
    OStringStream oss(requestData);
    mRequest.serializeMessage(oss);

    if (!mRequest.getBody().empty())
    {
        mRequest.serializeBody(oss);
    }
    else
    {
        // cut null
        requestData.resize(requestData.size() - 1);
    }

    // send
    int32_t result = send(
        std::move(requestData),
        SEV_MFN2(HttpClient::onTcpSend));
    if (result < 0)
    {
        // internal error
        onResponse(result);
    }
}

bool HttpClient::deserializeResponseBody(IBufferStream& ibs)
{
    size_t size = ibs.getReadableSize();

    if (mChunkedResponse.isRunning())
    {
        size_t chunkSize =
            mChunkedResponse.getRemaining();

        if (size > chunkSize)
        {
            size = chunkSize;
        }

        mChunkedResponse.received(size);
    }
    else
    {
        size_t contentLength =
            mResponse.getHeader().getContentLength();

        if (contentLength != 0)
        {
            size_t remaining =
                contentLength - mReceivedResponseBodySize;

            if (size > remaining)
            {
                size = remaining;
            }
        }

        mReceivedResponseBodySize += size;
    }

    if (size == 0)
    {
        return true;
    }

    if (!mOption.outputFileName.empty())
    {
        // output to file

        bool result = false;

        std::ofstream ofs(
            mOption.outputFileName,
            (std::ios::out | std::ios::app | std::ios::binary));

        if (ofs.is_open())
        {
            ofs.write(ibs.getPtr(), size);

            result = !ofs.bad();
            ofs.close();
        }

        if (!result)
        {
            std::remove(mOption.outputFileName.c_str());
            onResponse(-8011);
            return false;
        }
        else
        {
            ibs.seekCur(static_cast<int32_t>(size));
        }
    }
    else
    {
        // output to memory buffer

        try
        {
            auto& body = mResponse.getBody();

            size_t index = body.size();

            body.resize(body.size() + size);
            ibs.readBytes(&body[index], size);
        }
        catch (...)
        {
            onResponse(-8012);
            return false;
        }
    }

    if (!ibs.isEnd() && mChunkedResponse.isRunning())
    {
        // skip CRLF
        ibs.seekCur(+2);
    }

    return true;
}

Socket* HttpClient::createSocket(
    const IpEndPoint& peerEndPoint, int32_t& errorCode)
{
    Socket* socket;

    if (mUrl.getScheme() == "https")
    {
        socket = new SecureSocket();
    }
    else
    {
        socket = new Socket();
    }

    // create
    if (!socket->create(
        peerEndPoint.getFamily(),
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

int32_t HttpClient::redirect()
{
    std::string redirctHash =
        mUrl.getScheme() +
        mUrl.getHost() +
        std::to_string(mUrl.getPort()) +
        mUrl.getPath();

    if (std::find(
        mRedirectHashes.begin(),
        mRedirectHashes.end(),
        redirctHash) != mRedirectHashes.end())
    {
        // loop
        return -8801;
    }

    mRedirectHashes.push_back(redirctHash);

    const std::string& location =
        mResponse.getHeader().findValue(HttpHeaderField::Location);

    if (!mUrl.parse(location))
    {
        return -8802;
    }

    uint16_t statusCode = mResponse.getStatusCode();

    if (statusCode == 303)
    {
        mRequest.setMethod(HttpMethod::Get);
        mRequest.getBody().clear();
    }

    mRequest.setPath("");
    mRequest.getHeader().remove(HttpHeaderField::Host);

    mResponse.clear();
    mReceivedResponseBodySize = 0;
    mChunkedResponse.clear();
    mResponseTempBuffer.clear();

    close();
    start();

    return 0;
}

void HttpClient::onResponse(int32_t errorCode)
{
    if (errorCode != 0)
    {
        close();
    }
    else if ((errorCode == 0) && mOption.allowRedirect)
    {
        uint16_t statusCode = mResponse.getStatusCode();

        if ((statusCode == 301) ||
            (statusCode == 302) ||
            (statusCode == 303) ||
            (statusCode == 307) ||
            (statusCode == 308))
        {
            // redirect
            errorCode = redirect();

            if (errorCode == 0)
            {
                return;
            }
        }
    }

    mRunning = false;

    if (mResponseHandler != nullptr)
    {
        HttpClientPtr self(
            std::static_pointer_cast<HttpClient>(shared_from_this()));
        HttpResponseHandler handler = mResponseHandler;

        Thread::getCurrent()->post([self, handler, errorCode]() {
            handler(self, errorCode);
        });
    }
}

void HttpClient::onTcpConnect(
    const TcpClientPtr& /* client */, int32_t errorCode)
{
    if (errorCode != 0)
    {
        // connect error
        onResponse(errorCode);
        return;
    }

    // send
    sendHttpRequest();
}

void HttpClient::onTcpSend(
    const TcpChannelPtr& /* channel */, int32_t errorCode)
{
    if (errorCode != 0)
    {
        onResponse(errorCode);
    }
}

void HttpClient::onTcpReceive(const TcpChannelPtr& channel)
{
    auto response = channel->receiveAll(10240);

    if (isResponseCompleted() || response.empty())
    {
        return;
    }

    if (!mResponseTempBuffer.empty())
    {
        std::vector<char> temp;

        std::copy(
            mResponseTempBuffer.begin(),
            mResponseTempBuffer.end(),
            std::back_inserter(temp));
        std::copy(
            response.begin(),
            response.end(),
            std::back_inserter(temp));

        IStringStream iss(temp);
        onHttpResponse(iss);
    }
    else
    {
        IStringStream iss(response);
        onHttpResponse(iss);
    }
}

void HttpClient::onTcpClose(const TcpChannelPtr& /* channel */)
{
    if (!isResponseCompleted())
    {
        onResponse(-1);
    }
}

void HttpClient::onHttpResponse(IStringStream& iss)
{
    if (mResponse.isEmpty())
    {
        if (!mResponse.deserializeMessage(iss))
        {
            // parse error
            mResponse.clear();

            // keep
            const auto& buff = iss.getBuffer();
            std::copy(buff.begin(), buff.end(),
                std::back_inserter(mResponseTempBuffer));

            return;
        }

        mResponseTempBuffer.clear();

        std::string transferEncoding =
            mResponse.getHeader().findValue(
                HttpHeaderField::TransferEncoding);
        if (icompString(transferEncoding, "chunked"))
        {
            mChunkedResponse.start();
        }
    }

    // body
    while (!iss.isEnd())
    {
        if (mChunkedResponse.isRunning())
        {
            if (mChunkedResponse.getChunkSize() == 0)
            {
                if (!mChunkedResponse.setChunkSize(iss))
                {
                    onResponse(-8502);
                    return;
                }

                if (mChunkedResponse.getChunkSize() == 0)
                {
                    // done!!!
                    mChunkedResponse.clear();
                    break;
                }
            }
        }
        else if (mReceivedResponseBodySize >=
                 mResponse.getHeader().getContentLength())
        {
            break;
        }

        if (!deserializeResponseBody(iss))
        {
            return;
        }
    }

    if (isResponseCompleted())
    {
        // success
        onResponse(0);
    }
}

SEV_NS_END

#endif // SUBEVENT_HTTP_INL
