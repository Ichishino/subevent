#ifndef SUBEVENT_HTTP_INL
#define SUBEVENT_HTTP_INL

#include <cstdio>
#include <fstream>
#include <iterator>
#include <algorithm>

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

HttpUrl::~HttpUrl()
{
}

bool HttpUrl::parse(const std::string& url)
{
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

    url += mPath;
    
    if (!mQuery.empty())
    {
        url += "?" + mQuery;
    }

    if (!mFragment.empty())
    {
        url += "#" + mFragment;
    }

    return url;
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

//----------------------------------------------------------------------------//
// HttpHeader
//----------------------------------------------------------------------------//

HttpHeader::HttpHeader()
{
}

HttpHeader::~HttpHeader()
{
}

void HttpHeader::add(
    const std::string& name, const std::string& value)
{
    mFields.push_back({ name, value });
}

void HttpHeader::remove(const std::string& name)
{
    auto it = std::remove_if(
        mFields.begin(), mFields.end(), [&](const Field& field) {
            return (field.name == name);
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

bool HttpHeader::isExists(const std::string& name) const
{
    for (const auto& field : mFields)
    {
        if (field.name == name)
        {
            return true;
        }
    }

    return false;
}

const std::string& HttpHeader::findOne(const std::string& name) const
{
    for (const auto& field : mFields)
    {
        if (field.name == name)
        {
            return field.value;
        }
    }

    static const std::string emptyString;

    return emptyString;
}

std::list<std::string> HttpHeader::find(const std::string& name) const
{
    std::list<std::string> values;

    for (const auto& field : mFields)
    {
        if (field.name == name)
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

        std::string name = line.substr(0, pos);

        name.erase(0, name.find_first_not_of(" "));
        name.erase(name.find_last_not_of(" ") + 1);

        if (name.empty())
        {
            return false;
        }

        std::string value = line.substr(pos + 1);

        value.erase(0, value.find_first_not_of(" "));
        value.erase(value.find_last_not_of(" ") + 1);

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
    remove("Content-Length");
    add("Content-Length", std::to_string(contentLength));
}

size_t HttpHeader::getContentLength() const
{
    size_t contentLength = 0;

    std::string contentLengthStr = findOne("Content-Length");

    if (!contentLengthStr.empty())
    {
        contentLength = std::stol(contentLengthStr);
    }

    return contentLength;
}

//----------------------------------------------------------------------------//
// HttpRequest
//----------------------------------------------------------------------------//

HttpRequest::HttpRequest()
{
    clear();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::clear()
{
    mMethod.clear();
    mPath = "/";
    mProtocol = HttpProtocol::v1_1;
    mHeader.clear();
    mBody.clear();
}

bool HttpRequest::isEmpty() const
{
    return (mMethod.empty() &&
        mHeader.isEmpty() &&
        mBody.empty());
}

void HttpRequest::serializeMessage(OStringStream& oss) const
{
    // method + path + protocol
    oss << mMethod << " "
        << mPath << " "
        << mProtocol << "\r\n";

    // header
    mHeader.serialize(oss);
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

    // header
    if (!mHeader.deserialize(iss))
    {
        return false;
    }
    
    return true;
}

void HttpRequest::serializeBody(OBufferStream& obs) const
{
    // body
    if (!mBody.empty())
    {
        obs.writeBytes(&mBody[0], mBody.size());
    }
}

bool HttpRequest::deserializeBody(IBufferStream& ibs)
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

//----------------------------------------------------------------------------//
// HttpResponse
//----------------------------------------------------------------------------//

HttpResponse::HttpResponse()
{
    mProtocol = HttpProtocol::v1_1;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::clear()
{
    mProtocol = HttpProtocol::v1_1;
    mStatusCode = 0;
    mMessage.clear();
    mHeader.clear();
    mBody.clear();
}

bool HttpResponse::isEmpty() const
{
    return ((mStatusCode == 0) &&
        mHeader.isEmpty() &&
        mBody.empty());
}

void HttpResponse::serializeMessage(OStringStream& oss) const
{
    // protocol + status code + message
    oss << mProtocol << " "
        << mStatusCode << " "
        << mMessage << "\r\n";

    // header
    mHeader.serialize(oss);
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

    // header
    if (!mHeader.deserialize(iss))
    {
        return false;
    }

    return true;
}

void HttpResponse::serializeBody(OBufferStream& obs) const
{
    // body
    if (!mBody.empty())
    {
        obs.writeBytes(&mBody[0], mBody.size());
    }
}

bool HttpResponse::deserializeBody(IBufferStream& ibs)
{
    // body
    if (!ibs.isEnd())
    {
        mBody.resize(ibs.getReadableSize());

        if (!ibs.readBytes(&mBody[0], mBody.size()))
        {
            return false;
        }
    }

    return true;
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

HttpClient::HttpClient()
{
}

HttpClient::~HttpClient()
{
}

bool HttpClient::request(
    const std::string& method,
    const std::string& url,
    const std::string& body,
    const HttpResponseHandler& responseHandler,
    const std::string& outputFileName)
{
    mUrl.clear();
    mRequest.clear();
    mResponse.clear();
    mReceivedResponseBodySize = 0;
    mChunkedResponse.clear();
    mResponseTempBuffer.clear();
    mOutputFileName.empty();

    if (!mUrl.parse(url))
    {
        return false;
    }

    mRequest.setMethod(method);

    if (!body.empty())
    {
        mRequest.setBody(body);
    }

    mResponseHandler = responseHandler;
    mOutputFileName = outputFileName;

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

    return true;
}

bool HttpClient::requestGet(
    const std::string& url,
    const HttpResponseHandler& responseHandler,
    const std::string& outputFileName)
{
    return request("GET", url, "", responseHandler, outputFileName);
}

bool HttpClient::requestPost(
    const std::string& url,
    const std::string& body,
    const HttpResponseHandler& responseHandler)
{
    return request("POST", url, body, responseHandler);
}

bool HttpClient::requestPut(
    const std::string& url,
    const std::string& body,
    const HttpResponseHandler& responseHandler)
{
    return request("PUT", url, body, responseHandler);
}

bool HttpClient::requestDelete(
    const std::string& url,
    const HttpResponseHandler& responseHandler)
{
    return request("DELETE", url, "", responseHandler);
}

bool HttpClient::requestPatch(
    const std::string& url,
    const std::string& body,
    const HttpResponseHandler& responseHandler)
{
    return request("PATCH", url, body, responseHandler);
}

bool HttpClient::requestHead(
    const std::string& url,
    const HttpResponseHandler& responseHandler)
{
    return request("HEAD", url, "", responseHandler);
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
        if (mRequest.getMethod() != "HEAD")
        {
            return false;
        }
    }

    return true;
}

void HttpClient::sendHttpRequest()
{
    mRequest.setPath(mUrl.getPath());

    // Host
    if (!mRequest.getHeader().isExists("Host"))
    {
        mRequest.getHeader().add("Host", mUrl.getHost());
    }

    // Content Length
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

    if (!mOutputFileName.empty())
    {
        // output to file

        bool result = false;

        std::ofstream ofs(
            mOutputFileName,
            (std::ios::out | std::ios::app | std::ios::binary));

        if (ofs.is_open())
        {
            ofs.write(ibs.getPtr(), size);

            result = !ofs.bad();
            ofs.close();
        }

        if (!result)
        {
            std::remove(mOutputFileName.c_str());
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

void HttpClient::onResponse(int32_t errorCode)
{
    if (errorCode != 0)
    {
        close();
    }

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
            mResponse.getHeader().findOne("Transfer-Encoding");
        if (transferEncoding == "chunked")
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
