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
    HttpClientPtr httpClient = HttpClient::newInstance(&app);

    std::string url = "ws://127.0.0.1:9000";

    // websocket handshake
    httpClient->requestWsHandshake(
        url, "",
        [&app](const HttpClientPtr& httpClient,
               int errorCode) {

        if (errorCode != 0)
        {
            std::cout << "connect error "
                << errorCode << std::endl;

            // connect error
            app.stop();
            return;
        }

        if (httpClient->getResponse().getStatusCode() !=
            HttpStatusCode::SwitchingProtocols)
        {
            std::cout << "handshake is refused "
                << httpClient->getResponse().getStatusCode()
                << std::endl;

            // handshake error
            app.stop();
            return;
        }

        if (!httpClient->verifyWsHandshakeResponse())
        {
            std::cout << "handshake response error" << std::endl;

            // response error
            app.stop();
            return;
        }

        // handshake success

        WsChannelPtr wsChannel = httpClient->upgradeToWebSocket();

        wsChannel->send("hello");

        // data receive handler
        wsChannel->setDataFrameHandler(
            [](const WsChannelPtr& /* wsChannel */,
               const WsFramePtr& frame) {

            const std::vector<char>& data =
                frame->getPayload();

            std::cout << "receive " <<
                data.size() << "bytes" << std::endl;
        });

        // control frame receive handler
        wsChannel->setControlFrameHandler(
            [](const WsChannelPtr& wsChannel,
               const WsFramePtr& frame) {

            if (frame->getOpCode() ==
                WsFrame::OpCode::ConnectionClose)
            {
                std::cout << "websocket close" << std::endl;
            }

            wsChannel->onControlFrame(frame);
        });

        // close handler
        wsChannel->setCloseHandler(
            [&app](const TcpChannelPtr&) {

            std::cout <<
                "close connection" << std::endl;

            // quit application
            app.stop();
        });
    });

    return app.run();
}
