#ifndef SUBEVENT_HTTP_CLIENT_INL
#define SUBEVENT_HTTP_CLIENT_INL

#include <cstdio>
#include <cassert>
#include <iterator>

#include <subevent/http_client.hpp>
#include <subevent/network.hpp>
#include <subevent/http.hpp>
#include <subevent/ssl_socket.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// HttpClient
//----------------------------------------------------------------------------//

HttpClient::HttpClient(NetWorker* netWorker)
    : TcpClient(netWorker)
{
    mRunning = false;
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
    mContentReceiver.clear();
    mResponseTempBuffer.clear();
    mOption.clear();
    mRedirectHashes.clear();
    mSslContext.reset();

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
    mContentReceiver.setFileName(mOption.outputFileName);

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
    http->mContentReceiver.setFileName(option.outputFileName);
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
            SEV_BIND_2(this, HttpClient::onTcpConnect));

        setReceiveHandler(
            SEV_BIND_1(this, HttpClient::onTcpReceive));
        setCloseHandler(
            SEV_BIND_1(this, HttpClient::onTcpClose));
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

    if (mRequest.getMethod() == HttpMethod::Head)
    {
        return true;
    }

    if (!mContentReceiver.isCompleted())
    {
        return false;
    }

    return true;
}

void HttpClient::sendHttpRequest()
{
    // path
    mRequest.setPath(mUrl.composePath());

    // Host
    if (!mRequest.getHeader().has(HttpHeaderField::Host))
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
    StringWriter writer(requestData);
    mRequest.serializeMessage(writer);

    if (!mRequest.getBody().empty())
    {
        mRequest.serializeBody(writer);
    }
    else
    {
        // cut null
        requestData.resize(requestData.size() - 1);
    }

    // send
    int32_t result = send(
        std::move(requestData),
        SEV_BIND_2(this, HttpClient::onTcpSend));
    if (result < 0)
    {
        // internal error
        onResponse(result);
    }
}

Socket* HttpClient::createSocket(
    const IpEndPoint& peerEndPoint, int32_t& errorCode)
{
    Socket* socket;

    if (mUrl.getScheme() == "https")
    {
        mSslContext = mOption.sslCtx;

        if (mSslContext == nullptr)
        {
            mSslContext =
                SslContext::newInstance(SSLv23_client_method());
        }

        socket = new SecureSocket(mSslContext);
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
    mContentReceiver.clear();
    mResponseTempBuffer.clear();

    if (!mOption.outputFileName.empty())
    {
        std::remove(mOption.outputFileName.c_str());
        mContentReceiver.setFileName(mOption.outputFileName);
    }

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
            std::dynamic_pointer_cast<HttpClient>(shared_from_this()));
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
    auto response = channel->receiveAll();

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

    StringReader reader(response);

    if (!onHttpResponse(reader))
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

bool HttpClient::onHttpResponse(StringReader& reader)
{
    // header
    if (mResponse.isEmpty())
    {
        try
        {
            if (!mResponse.deserializeMessage(reader))
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

        mContentReceiver.init(mResponse);
    }

    // body
    if (!mContentReceiver.onReceive(reader))
    {
        onResponse(-8502);
        return true;
    }

    if (isResponseCompleted())
    {
        mResponse.setBody(mContentReceiver.getData());

        // success
        onResponse(0);
    }

    return true;
}

SEV_NS_END

#endif // SUBEVENT_HTTP_CLIENT_INL
