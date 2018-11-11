#include <iostream>

#include <subevent/subevent.hpp>
#include <subevent/subevent_http.hpp>

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyThread
//---------------------------------------------------------------------------//

class MyThread : public HttpChannelThread
{
public:
    MyThread(Thread* parent)
        : HttpChannelThread(parent)
    {
        // ex.
        //
        // 1. "/"
        // 2. "/bbb1"
        // 3. "/ccc1/"
        //
        // http://127.0.0.1:9000/ -> 1
        // http://127.0.0.1:9000/aaa1 -> 1
        // http://127.0.0.1:9000/bbb1 -> 2
        // http://127.0.0.1:9000/ccc1/ccc2 -> 3
        // http://127.0.0.1:9000/ccc1/ccc2/ -> default handler
        // http://127.0.0.1:9000/ccc1/ccc2/ccc3 -> default handler

        setRequestHandler("/", SEV_BIND_1(this, MyThread::onMyHandler));
        setRequestHandler("/stop", SEV_BIND_1(this, MyThread::onMyServerStop));
    }

protected:
    uint16_t getMaxChannels() const override
    {
#ifdef _WIN32
        return 50;
#else
        return 100;
#endif
    }

    void onHttpRequest(const HttpChannelPtr& channel) override
    {
        // default handler
        HttpChannelThread::onHttpRequest(channel);
    }

protected:

    void onMyHandler(const HttpChannelPtr& channel)
    {
        if (channel->getRequest().isWsHandshakeRequest())
        {
            // websocket handshake accept
            channel->sendWsHandshakeResponse();

            std::cout << channel->getPeerEndPoint().toString()
                << " websocket open" << std::endl;
                
            WsChannelPtr wsChannel =
                channel->upgradeToWebSocket();
            mChannelList.push_back(wsChannel);

            // handler settings
            wsChannel->setDataFrameHandler(
                SEV_BIND_2(this, MyThread::onMyDataFrameReceive));
            wsChannel->setControlFrameHandler(
                SEV_BIND_2(this, MyThread::onMyControlFrameReceive));
            wsChannel->setCloseHandler(
                SEV_BIND_1(this, MyThread::onMyTcpClose));
        }
        else
        {
            channel->sendHttpResponse(
                HttpStatusCode::BadRequest, "Bad Request");
            channel->close();
        }
    }

    // data frame receive handler
    void onMyDataFrameReceive(
        const WsChannelPtr& wsChannel, const WsFramePtr& frame)
    {
        // user data
        const std::vector<char>& data = frame->getPayload();

        std::cout << wsChannel->getTcpChannel()->
            getPeerEndPoint().toString()
            << " receive " << data.size() << "bytes"
            << std::endl;

        WsFrame response(*frame);
        response.setMask(false);

        // echo
        wsChannel->send(response);
    }

    // control frame receive handler
    void onMyControlFrameReceive(
        const WsChannelPtr& wsChannel, const WsFramePtr& frame)
    {
        if (frame->getOpCode() == WsFrame::OpCode::ConnectionClose)
        {
            std::cout << wsChannel->getTcpChannel()->
                getPeerEndPoint().toString()
                << " websocket close" << std::endl;
        }

        wsChannel->onControlFrame(frame);
    }

    // close handler
    void onMyTcpClose(const TcpChannelPtr& channel)
    {
        std::cout << channel->getPeerEndPoint().toString()
            << " close connection" << std::endl;

        mChannelList.remove_if(
            [&channel](const WsChannelPtr& ws) { return channel == ws; });
    }

    void onMyServerStop(const HttpChannelPtr& channel)
    {
        // http://127.0.0.1:9000/stop

        channel->sendHttpResponse(
            HttpStatusCode::Ok, "OK",
            "<html><body>OK</body></html>");
        channel->close();

        // stop server
        Application::getCurrent()->stop();
    }

    // thread exit
    void onExit() override
    {
        for (WsChannelPtr& wsChannel : mChannelList)
        {
            wsChannel->close();
        }
        mChannelList.clear();

        HttpChannelThread::onExit();
    }

private:
    std::list<WsChannelPtr> mChannelList;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    HttpServerApp app;
    app.getTcpServer()->getSocketOption().setReuseAddress(true);

    size_t numberOfThreads = 10;

    app.createThread<MyThread>(numberOfThreads);

    uint16_t port = 9000;

    // start
    app.open(IpEndPoint(port));

    return app.run();
}
