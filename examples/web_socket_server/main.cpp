#include <iostream>

#include <subevent/subevent.hpp>
#include <subevent/subevent_http.hpp>

SEV_USING_NS

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    NetApplication app;
    HttpServerPtr server = HttpServer::newInstance(&app);

    server->getSocketOption().setReuseAddress(true);

    uint16_t port = 9000;

    // start
    server->open(IpEndPoint(port));

    std::list<WsChannelPtr> wsChannelList;

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

    // handler
    server->setRequestHandler(
        "/", [&app, &wsChannelList](const HttpChannelPtr& channel) {
    
        if (channel->getRequest().isWsHandshakeRequest())
        {
            channel->sendWsHandshakeResponse();

            WsChannelPtr wsChannel =
                channel->upgradeToWebSocket();

            std::cout << channel->getPeerEndPoint().toString()
                << " websocket open" << std::endl;

            wsChannelList.push_back(wsChannel);

            // data frame receive handler
            wsChannel->setDataFrameHandler(
                [](const WsChannelPtr& wsChannel,
                   const WsFramePtr& frame) {

                // user data
                const std::vector<char>& data =
                    frame->getPayload();

                std::cout << wsChannel->getTcpChannel()->
                        getPeerEndPoint().toString()
                    << " receive " << data.size() << "bytes"
                    << std::endl;

                WsFrame response(*frame);
                response.setMask(false);

                // echo
                wsChannel->send(response);
            });

            // control frame receive handler
            wsChannel->setControlFrameHandler(
                [](const WsChannelPtr& wsChannel,
                   const WsFramePtr& frame) {

                if (frame->getOpCode() ==
                    WsFrame::OpCode::ConnectionClose)
                {
                    std::cout << wsChannel->getTcpChannel()->
                        getPeerEndPoint().toString()
                        << " websocket close" << std::endl;
                }

                wsChannel->onControlFrame(frame);
            });

            // close handler
            wsChannel->setCloseHandler(
                [&wsChannelList](const TcpChannelPtr& channel) {
 
                std::cout << channel->getPeerEndPoint().toString()
                    << " close connection" << std::endl;

                wsChannelList.remove_if(
                    [&channel](const WsChannelPtr& ws) { return channel == ws; });
            });
        }
        else
        {
            channel->sendHttpResponse(
                HttpStatusCode::BadRequest, "Bad Request");
            channel->close();
        }
    });

    // server stop handler
    server->setRequestHandler(
        "/stop", [&app, &wsChannelList](const HttpChannelPtr& channel) {

        // http://127.0.0.1:9000/stop

        channel->sendHttpResponse(
            HttpStatusCode::Ok, "OK",
            "<html><body>OK</body></html>");
        channel->close();

        for (WsChannelPtr& wsChannel : wsChannelList)
        {
            wsChannel->close();
        }
        wsChannelList.clear();

        // stop server
        app.stop();
    });

    return app.run();
}
