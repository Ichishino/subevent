#ifndef SUBEVENT_HTTP_INL
#define SUBEVENT_HTTP_INL

#include <subevent/network.hpp>
#include <subevent/http.hpp>
#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

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

    if (!mFragment.empty())
    {
        url += "#" + mFragment;
    }

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

std::string HttpUrl::encode(
    const std::string& src, const std::string& ignoreChars)
{
    const std::string unencode =
        "abcdefghijklmnopqrstuvwxyz" \
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
        "0123456789" \
        "-_.~" + ignoreChars;

    std::string dest;
    size_t start = 0;

    for (;;)
    {
        size_t pos = src.find_first_not_of(unencode, start);

        dest += src.substr(start, pos - start);

        if (pos == std::string::npos)
        {
            break;
        }

        char encoded[4];
#ifdef _WIN32
        sprintf_s(encoded, "%%%02X", (unsigned char)src[pos]);
#else
        std::sprintf(encoded, "%%%02X", (unsigned char)src[pos]);
#endif
        dest += encoded;
        start = pos + 1;
    }

    return dest;
}

std::string HttpUrl::decode(const std::string& src)
{
    std::string dest;
    size_t start = 0;

    for (;;)
    {
        size_t pos = src.find_first_of('%', start);

        dest += src.substr(start, pos - start);

        if (pos == std::string::npos)
        {
            break;
        }

        if ((pos + 2) >= src.length())
        {
            // error
            dest.clear();
            break;
        }

        int ch;
#ifdef _WIN32
        int result = sscanf_s(&src[pos + 1], "%02X", &ch);
#else
        int result = std::sscanf(&src[pos + 1], "%02X", &ch);
#endif
        if ((result == EOF) || (result == 0))
        {
            // error
            dest.clear();
            break;
        }

        dest += static_cast<char>(ch);
        start = pos + 3;
    }

    return dest;
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

void HttpHeader::set(
    const std::string& name, const std::string& value)
{
    remove(name);
    add(name, value);
}

const std::string& HttpHeader::get(
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

std::list<std::string> HttpHeader::find(
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

bool HttpHeader::isEmpty() const
{
    return mFields.empty();
}

void HttpHeader::clear()
{
    mFields.clear();
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
        bool readError = false;
        
        if (!iss.readString(line, "\r\n", 10240, &readError))
        {
            if (readError)
            {
                throw std::invalid_argument("Too long header line");
            }

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
    set(HttpHeaderField::ContentLength,
        std::to_string(contentLength));
}

size_t HttpHeader::getContentLength() const
{
    size_t contentLength = 0;

    std::string contentLengthStr =
        get(HttpHeaderField::ContentLength);

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

void HttpCookie::set(
    const std::string& name, const std::string& value)
{
    remove(name);
    add(name, value);
}

const std::string& HttpCookie::get(
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

std::list<std::string> HttpCookie::find(
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

bool HttpCookie::isEmpty() const
{
    return mAttributes.empty();
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

        if (icompString(name, HttpCookieAttr::Expires))
        {
            mExpires = value;
        }
        else if (icompString(name, HttpCookieAttr::MaxAge))
        {
            mMaxAge = value;
        }
        else if (icompString(name, HttpCookieAttr::Domain))
        {
            mDomain = value;
        }
        else if (icompString(name, HttpCookieAttr::Path))
        {
            mPath = value;
        }
        else if (icompString(name, HttpCookieAttr::Secure))
        {
            mSecure = true;
        }
        else if (icompString(name, HttpCookieAttr::HttpOnly))
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
        cookie += HttpCookieAttr::Expires + "=" + mExpires;
        cookie += "; ";
    }

    if (!mMaxAge.empty())
    {
        cookie += HttpCookieAttr::MaxAge + "=" + mMaxAge;
        cookie += "; ";
    }

    if (!mDomain.empty())
    {
        cookie += HttpCookieAttr::Domain + "=" + mDomain;
        cookie += "; ";
    }

    if (!mPath.empty())
    {
        cookie += HttpCookieAttr::Path + "=" + mPath;
        cookie += "; ";
    }

    if (mSecure)
    {
        cookie += HttpCookieAttr::Secure;
        cookie += "; ";
    }

    if (mHttpOnly)
    {
        cookie += HttpCookieAttr::HttpOnly;
        cookie += "; ";
    }

    if (!cookie.empty())
    {
        cookie.resize(cookie.size() - 2);
    }

    return cookie;
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

    for (auto& field : mHeader.find(headerName))
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
    bool readError = false;

    if (!iss.readString(line, "\r\n", 10240, &readError))
    {
        if (readError)
        {
            throw std::invalid_argument("Too long request line");
        }

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
        throw std::invalid_argument("Invalid method");
    }

    size_t pos2 = line.find_first_of(' ', pos1 + 1);

    if (pos2 == std::string::npos)
    {
        throw std::invalid_argument("Invalid path 1");
    }

    // path
    mPath = line.substr(pos1 + 1, pos2 - pos1 - 1);

    if (mPath.empty())
    {
        throw std::invalid_argument("Invalid path 2");
    }

    // protocol
    mProtocol = line.substr(pos2 + 1);

    if (mProtocol.empty())
    {
        throw std::invalid_argument("Invalid protocol");
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
    bool readError = false;

    if (!iss.readString(line, "\r\n", 10240, &readError))
    {
        if (readError)
        {
            throw std::invalid_argument("Too long response line");
        }

        return false;
    }
    iss.seekCur(+2);

    size_t pos1 = line.find_first_of(' ');

    if (pos1 == std::string::npos)
    {
        throw std::invalid_argument("Invalid protocol 1");
    }

    // protocol
    mProtocol = line.substr(0, pos1);

    if (mProtocol.empty())
    {
        throw std::invalid_argument("Invalid protocol 2");
    }

    size_t pos2 = line.find_first_of(' ', pos1 + 1);

    if (pos2 == std::string::npos)
    {
        throw std::invalid_argument("Invalid status code 1");
    }

    std::string statusCode =
        line.substr(pos1 + 1, pos2 - pos1 - 1);

    if (statusCode.empty())
    {
        throw std::invalid_argument("Invalid status code 2");
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

SEV_NS_END

#endif // SUBEVENT_HTTP_INL
