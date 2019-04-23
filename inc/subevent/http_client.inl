#ifndef SUBEVENT_HTTP_CLIENT_INL
#define SUBEVENT_HTTP_CLIENT_INL

#include <cstdio>
#include <cassert>
#include <iostream>
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

#ifdef SEV_SUPPORTS_SSL
    mSslContext.reset();
#endif

    if (mRequest.getMethod().empty())
    {
        return false;
    }

    HttpUrl httpUrl;

    if (!httpUrl.parse(url))
    {
        return false;
    }

#ifndef SEV_SUPPORTS_SSL
    if (httpUrl.isSecureScheme())
    {
        std::cerr <<
            "[Subevent Error] OpenSSL is not installed." << std::endl;
        return false;
    }
#endif

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

#ifndef SEV_SUPPORTS_SSL
    if (http->mUrl.isSecureScheme())
    {
        std::cerr <<
            "[Subevent Error] OpenSSL is not installed." << std::endl;
        return -8603;
    }
#endif

    if (!thread.start())
    {
        return -8604;
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

#ifdef SEV_SUPPORTS_SSL
    if (mUrl.isSecureScheme())
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
#else
    socket = new Socket();
#endif

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

#ifndef SEV_SUPPORTS_SSL
    if (mUrl.isSecureScheme())
    {
        std::cerr <<
            "[Suvevent Error] OpenSSL is not installed." << std::endl;
        return -8803;
    }
#endif

    uint16_t statusCode = mResponse.getStatusCode();

    if (statusCode == HttpStatusCode::SeeOther)
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

        if ((statusCode == HttpStatusCode::MovedPermanently) ||
            (statusCode == HttpStatusCode::Found) ||
            (statusCode == HttpStatusCode::SeeOther) ||
            (statusCode == HttpStatusCode::TemporaryRedirect) ||
            (statusCode == HttpStatusCode::PermanentRedirect))
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

            if (!mContentReceiver.init(mResponse))
            {
                onResponse(-8500);
                return true;
            }
        }
        catch (...)
        {
            // invalid data
            onResponse(-8501);
            return true;
        }
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

bool HttpClient::requestWsHandshake(
    const std::string& url,
    const std::string& protocols,
    const HttpResponseHandler& responseHandler,
    const RequestOption& option)
{
    HttpRequest& httpRequest = getRequest();

    httpRequest.setMethod(HttpMethod::Get);
    httpRequest.getHeader().set(
        HttpHeaderField::Upgrade, "websocket");
    httpRequest.getHeader().set(
        HttpHeaderField::Connection, "Upgrade");
    httpRequest.getHeader().set(
        HttpHeaderField::SecWebSocketVersion, "13");
    httpRequest.getHeader().set(
        HttpHeaderField::SecWebSocketProtocol, protocols);

    HttpUrl httpUrl;

    if (!httpUrl.parse(url))
    {
        return false;
    }

    httpRequest.getHeader().set(
        HttpHeaderField::Origin, httpUrl.composeOrigin());

    auto bytes = Random::generateBytes(16);
    const std::string b64 = Base64::encode(&bytes[0], bytes.size());

    httpRequest.getHeader().set(
        HttpHeaderField::SecWebSocketKey, b64);

    return request(url, responseHandler, option);
}

bool HttpClient::verifyWsHandshakeResponse() const
{
    std::string res = getResponse().getHeader().get(
        HttpHeaderField::SecWebSocketAccept);

    if (res.empty())
    {
        // invalid response
        return false;
    }

    std::string data = getRequest().getHeader().get(
        HttpHeaderField::SecWebSocketKey) + SEV_WS_KEY_SUFFIX;
    auto hash = Sha1::digest(
        data.c_str(), static_cast<uint32_t>(data.length()));
    std::string req = Base64::encode(&hash[0], hash.size());

    if (req != res)
    {
        // not match
        return false;
    }

    return true;
}

WsChannelPtr HttpClient::upgradeToWebSocket()
{
    if (mWsChannel != nullptr)
    {
        return nullptr;
    }

    mWsChannel = WsChannel::newInstance(shared_from_this(), true);

    return mWsChannel;
}

SEV_NS_END

#endif // SUBEVENT_HTTP_CLIENT_INL
