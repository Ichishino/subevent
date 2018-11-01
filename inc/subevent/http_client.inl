#ifndef SUBEVENT_HTTP_CLIENT_INL
#define SUBEVENT_HTTP_CLIENT_INL

#include <cstdio>
#include <fstream>
#include <cassert>
#include <iterator>

#include <subevent/http_client.hpp>
#include <subevent/network.hpp>
#include <subevent/http.hpp>
#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

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
    if (req.getMethod().empty())
    {
        return -8601;
    }

    int32_t result = -1;

    NetThread thread;
    HttpClientPtr http = newInstance(&thread);

    if (!http->mUrl.parse(url))
    {
        return -8602;
    }

    if (!thread.start())
    {
        return -8603;
    }

    res.clear();

    Semaphore sem;

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

    return result;
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
        mResponse.getHeader().get(HttpHeaderField::Location);

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

        mNetWorker->postTask([self, handler, errorCode]() {
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
        std::copy(
            response.begin(),
            response.end(),
            std::back_inserter(mResponseTempBuffer));

        response = std::move(mResponseTempBuffer);
    }

    IStringStream iss(response);

    if (!onHttpResponse(iss))
    {
        mResponseTempBuffer = std::move(response);
    }
}

void HttpClient::onTcpClose(const TcpChannelPtr& /* channel */)
{
    if (!isResponseCompleted())
    {
        onResponse(-1);
    }
}

bool HttpClient::onHttpResponse(IStringStream& iss)
{
    if (mResponse.isEmpty())
    {
        try
        {
            if (!mResponse.deserializeMessage(iss))
            {
                // not completed
                mResponse.clear();
                return false;
            }
        }
        catch (...)
        {
            // invalid data
            onResponse(-8501);
            return true;
        }

        std::string transferEncoding =
            mResponse.getHeader().get(
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
                    return true;
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
            return true;
        }
    }

    if (isResponseCompleted())
    {
        // success
        onResponse(0);
    }

    return true;
}

SEV_NS_END

#endif // SUBEVENT_HTTP_CLIENT_INL
