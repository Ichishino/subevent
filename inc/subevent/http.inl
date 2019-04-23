#ifndef SUBEVENT_HTTP_INL
#define SUBEVENT_HTTP_INL

#include <fstream>
#include <algorithm>

#include <subevent/network.hpp>
#include <subevent/http.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// HttpParams
//----------------------------------------------------------------------------//

HttpParams::HttpParams()
{
    clear();
}

HttpParams::HttpParams(const std::string& params)
{
    if (!parse(params))
    {
        throw std::invalid_argument("HttpParams parse error");
    }
}

HttpParams::HttpParams(const HttpParams& other)
{
    operator=(other);
}

HttpParams::HttpParams(HttpParams&& other)
{
    operator=(std::move(other));
}

HttpParams::~HttpParams()
{
}

void HttpParams::add(
    const std::string& name, const std::string& value)
{
    if (!value.empty())
    {
        mParams.push_back({ name, value });
    }
}

void HttpParams::remove(const std::string& name)
{
    auto it = std::remove_if(
        mParams.begin(), mParams.end(),
        [&](const Param& param) {
        return String::iequals(param.name, name);
    });

    if (it == mParams.end())
    {
        return;
    }

    mParams.erase(it, mParams.end());
}

void HttpParams::set(
    const std::string& name, const std::string& value)
{
    remove(name);
    add(name, value);
}

const std::string& HttpParams::get(
    const std::string& name) const
{
    for (const auto& param : mParams)
    {
        if (String::iequals(param.name, name))
        {
            return param.value;
        }
    }

    static const std::string emptyString;

    return emptyString;
}

std::list<std::string> HttpParams::find(
    const std::string& name) const
{
    std::list<std::string> results;

    for (const auto& param : mParams)
    {
        if (String::iequals(param.name, name))
        {
            results.push_back(param.name);
        }
    }

    return results;
}

bool HttpParams::has(
    const std::string& name) const
{
    for (const auto& param : mParams)
    {
        if (String::iequals(param.name, name))
        {
            return true;
        }
    }

    return false;
}

bool HttpParams::isEmpty() const
{
    return mParams.empty();
}

bool HttpParams::parse(const std::string& params)
{
    clear();

    if (params.empty())
    {
        return true;
    }

    size_t head = params.find_first_of('?');

    if (head == std::string::npos)
    {
        head = 0;
    }
    else
    {
        ++head;
    }

    for (;;)
    {
        size_t pos = params.find_first_of('=', head);

        if (pos == std::string::npos)
        {
            return false;
        }

        size_t tail = params.find_first_of('&', head);

        Param param;
        param.name = params.substr(head, pos - head);

        if (param.name.empty())
        {
            return false;
        }

        ++pos;

        if (tail == std::string::npos)
        {
            param.value = params.substr(pos);
        }
        else
        {
            param.value = params.substr(pos, tail - pos);
        }

        mParams.push_back(std::move(param));

        if (tail == std::string::npos)
        {
            break;
        }

        head = tail + 1;
    }

    return true;
}

std::string HttpParams::compose() const
{
    std::string params;

    if (!mParams.empty())
    {
        for (const auto& param : mParams)
        {
            params += "&" + param.compose();
        }

        params[0] = '?';
    }

    return params;
}

void HttpParams::clear()
{
    mParams.clear();
}

HttpParams& HttpParams::operator=(const HttpParams& other)
{
    mParams = other.mParams;

    return *this;
}

HttpParams& HttpParams::operator=(HttpParams&& other)
{
    mParams = std::move(other.mParams);

    return *this;
}

//----------------------------------------------------------------------------//
// HttpUrl
//----------------------------------------------------------------------------//

HttpUrl::HttpUrl()
{
    clear();
}

HttpUrl::HttpUrl(const std::string& url)
{
    if (!parse(url))
    {
        throw std::invalid_argument("HttpUrl parse error");
    }
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
        if (isSecureScheme())
        {
            mPort = 443;
        }
        else
        {
            mPort = 80;
        }
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

std::string HttpUrl::composeOrigin() const
{
    std::string origin;

    if (isSecureScheme())
    {
        origin = "https://" + mHost;
    }
    else
    {
        origin = "http://" + mHost;
    }

    if ((mPort != 80) && (mPort != 443))
    {
        origin += ":" + std::to_string(mPort);
    }

    return origin;
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
#ifdef SEV_OS_WIN
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
#ifdef SEV_OS_WIN
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
            return String::iequals(field.name, name);
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
        if (String::iequals(field.name, name))
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
        if (String::iequals(field.name, name))
        {
            values.push_back(field.value);
        }
    }

    return values;
}

bool HttpHeader::has(
    const std::string& name) const
{
    for (const auto& field : mFields)
    {
        if (String::iequals(field.name, name))
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

void HttpHeader::serialize(StringWriter& writer) const
{
    for (const auto& field : mFields)
    {
        writer << field.name << ":" << field.value << "\r\n";
    }

    writer << "\r\n";
}

bool HttpHeader::deserialize(StringReader& reader)
{
    clear();

    for (;;)
    {
        std::string line;
        bool readError = false;
        
        if (!reader.readString(line, "\r\n", 10240, &readError))
        {
            if (readError)
            {
                throw std::invalid_argument("Too long header line");
            }

            return false;
        }
        reader.seekCur(+2);

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

        String::trim(name);

        if (name.empty())
        {
            return false;
        }

        // value
        std::string value = line.substr(pos + 1);

        String::trim(value);

        if (value.empty())
        {
            return false;
        }

        add(name, value);
    }

    return true;
}

void HttpHeader::setContentLength(uintmax_t contentLength)
{
    set(HttpHeaderField::ContentLength,
        std::to_string(contentLength));
}

uintmax_t HttpHeader::getContentLength() const
{
    uintmax_t contentLength = 0;

    std::string contentLengthStr =
        get(HttpHeaderField::ContentLength);

    if (!contentLengthStr.empty())
    {
        contentLength = std::stoll(contentLengthStr);
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
            return String::iequals(attr.name, name);
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
        if (String::iequals(attr.name, name))
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
        if (String::iequals(attr.name, name))
        {
            results.push_back(attr.name);
        }
    }

    return results;
}

bool HttpCookie::has(
    const std::string& name) const
{
    for (const auto& attr : mAttributes)
    {
        if (String::iequals(attr.name, name))
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
            String::trim(name);

            // value
            value = nameAndValue.substr(pos + 1);
            String::trim(value);
        }
        else
        {
            // name only
            name = nameAndValue;
            String::trim(name);
        }

        if (name.empty())
        {
            return false;
        }

        if (String::iequals(name, HttpCookieAttr::Expires))
        {
            mExpires = value;
        }
        else if (String::iequals(name, HttpCookieAttr::MaxAge))
        {
            mMaxAge = value;
        }
        else if (String::iequals(name, HttpCookieAttr::Domain))
        {
            mDomain = value;
        }
        else if (String::iequals(name, HttpCookieAttr::Path))
        {
            mPath = value;
        }
        else if (String::iequals(name, HttpCookieAttr::Secure))
        {
            mSecure = true;
        }
        else if (String::iequals(name, HttpCookieAttr::HttpOnly))
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

void HttpMessage::serializeMessage(StringWriter& writer) const
{
    // header
    mHeader.serialize(writer);
}

bool HttpMessage::deserializeMessage(StringReader& reader)
{
    // header
    if (!mHeader.deserialize(reader))
    {
        return false;
    }

    return true;
}

void HttpMessage::serializeBody(ByteWriter& writer) const
{
    // body
    if (!mBody.empty())
    {
        writer.writeBytes(&mBody[0], mBody.size());
    }
}

bool HttpMessage::deserializeBody(ByteReader& reader)
{
    // body
    if (!reader.isEnd())
    {
        mBody.resize(reader.getReadableSize());

        if (!reader.readBytes(&mBody, mBody.size()))
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

void HttpRequest::serializeMessage(StringWriter& writer) const
{
    // method + path + protocol
    writer << mMethod << " "
        << mPath << " "
        << mProtocol << "\r\n";

    HttpMessage::serializeMessage(writer);
}

bool HttpRequest::deserializeMessage(StringReader& reader)
{
    std::string line;
    bool readError = false;

    if (!reader.readString(line, "\r\n", 10240, &readError))
    {
        if (readError)
        {
            throw std::invalid_argument("Too long request line");
        }

        return false;
    }
    reader.seekCur(+2);
        
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

    if (!HttpMessage::deserializeMessage(reader))
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

bool HttpRequest::isWsHandshakeRequest() const
{
    // Host
    const std::string& host =
        getHeader().get(HttpHeaderField::Host);

    if (host.empty())
    {
        return false;
    }

    // Upgrade
    const std::string& upgrade =
        getHeader().get(HttpHeaderField::Upgrade);

    if (upgrade.empty() || !String::iequals(upgrade, "websocket"))
    {
        return false;
    }

    // Connection
    const std::string& connection =
        getHeader().get(HttpHeaderField::Connection);

    auto values = String::split(connection, " ,");
    auto it = std::find_if(
        values.begin(), values.end(), [](const std::string& s) {
        return String::iequals(s, "Upgrade");
    });

    if (it == values.end())
    {
        return false;
    }

    // Sec-WebSocket-Key
    if (!getHeader().has(HttpHeaderField::SecWebSocketKey))
    {
        return false;
    }

    // Sec-WebSocket-Version
    const std::string& wsVersion =
        getHeader().get(HttpHeaderField::SecWebSocketVersion);

    if (wsVersion != "13")
    {
        return false;
    }

    return true;
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

void HttpResponse::serializeMessage(StringWriter& writer) const
{
    // protocol + status code + message
    writer << mProtocol << " "
        << mStatusCode << " "
        << mMessage << "\r\n";

    HttpMessage::serializeMessage(writer);
}

bool HttpResponse::deserializeMessage(StringReader& reader)
{
    std::string line;
    bool readError = false;

    if (!reader.readString(line, "\r\n", 10240, &readError))
    {
        if (readError)
        {
            throw std::invalid_argument("Too long response line");
        }

        return false;
    }
    reader.seekCur(+2);

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

    if (!HttpMessage::deserializeMessage(reader))
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
// HttpContentReceiver::ChunkWork
//----------------------------------------------------------------------------//

HttpContentReceiver::ChunkWork::ChunkWork()
{
    clear();
}

HttpContentReceiver::ChunkWork::~ChunkWork()
{
}

bool HttpContentReceiver::ChunkWork::setChunkSize(StringReader& reader)
{
    std::string chunkSizeStr;

    if (!reader.readString(chunkSizeStr, "\r\n"))
    {
        return false;
    }

    reader.seekCur(+2);

    try
    {
        mChunkSize = std::stoul(chunkSizeStr, nullptr, 16);
    }
    catch (...)
    {
        return false;
    }

    mReceiveSize = 0;

    return true;
}

//----------------------------------------------------------------------------//
// HttpContentReceiver
//----------------------------------------------------------------------------//

HttpContentReceiver::HttpContentReceiver()
{
    clear();
}

HttpContentReceiver::~HttpContentReceiver()
{
}

bool HttpContentReceiver::init(const HttpMessage& message)
{
    std::string transferEncoding =
        message.getHeader().get(
            HttpHeaderField::TransferEncoding);

    if (String::iequals(transferEncoding, "chunked"))
    {
        startChunk();
    }
    else
    {
        uintmax_t contentLength =
            message.getHeader().getContentLength();

        if (contentLength > SIZE_MAX)
        {
            // too much
            return false;
        }

        setContentLength((size_t)contentLength);
    }

    return true;
}

bool HttpContentReceiver::onReceive(StringReader& reader)
{
    while (!reader.isEnd())
    {
        if (mChunkWork.isRunning())
        {
            if (mChunkWork.getChunkSize() == 0)
            {
                if (!mChunkWork.setChunkSize(reader))
                {
                    return false;
                }

                if (mChunkWork.getChunkSize() == 0)
                {
                    // done!!!
                    mChunkWork.clear();
                    break;
                }
            }
        }
        else if (mReceiveSize >= mSize)
        {
            break;
        }

        if (!deserialize(reader))
        {
            return false;
        }
    }

    return true;
}

bool HttpContentReceiver::deserialize(ByteReader& reader)
{
    size_t size = reader.getReadableSize();

    if (mChunkWork.isRunning())
    {
        size_t chunkSize =
            mChunkWork.getRemaining();

        if (size > chunkSize)
        {
            size = chunkSize;
        }

        mChunkWork.updateSize(size);
    }
    else
    {
        if (mSize != 0)
        {
            size_t remaining = mSize - mReceiveSize;

            if (size > remaining)
            {
                size = remaining;
            }
        }

        mReceiveSize += size;
    }

    if (size == 0)
    {
        return true;
    }

    if (!mFileName.empty())
    {
        // output to file

        bool result = false;

        std::ofstream ofs(mFileName,
            (std::ios::out | std::ios::app | std::ios::binary));

        if (ofs.is_open())
        {
            ofs.write(reader.getPtr(), size);

            result = !ofs.bad();
            ofs.close();
        }

        if (!result)
        {
            std::remove(mFileName.c_str());
            return false;
        }
        else
        {
            reader.seekCur(static_cast<int32_t>(size));
        }
    }
    else
    {
        // output to memory buffer

        try
        {
            size_t index = mData.size();

            mData.resize(mData.size() + size);
            reader.readBytes(&mData[index], size);
        }
        catch (...)
        {
            return false;
        }
    }

    if (!reader.isEnd() && mChunkWork.isRunning())
    {
        // skip CRLF
        reader.seekCur(+2);
    }

    return true;
}

SEV_NS_END

#endif // SUBEVENT_HTTP_INL
