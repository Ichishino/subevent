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

    // handler
    server->setRequestHandler(
        "/", [](const HttpChannelPtr& channel) {
    
        HttpUrl url(channel->getRequest().getPath());

        // Method
        const std::string& method = channel->getRequest().getMethod();

        // Request path
        const std::string& path = url.getPath();

        // Parameters
        HttpParams params(url.getQuery());

        // Body
        std::string body = channel->getRequest().getBodyAsString();

        HttpResponse res;
        res.setStatusCode(200);
        res.setMessage("OK");
        res.setBody(
            "<html><body>"
            "Method: " + method + "<br>"
            "Request Path: " + path + "<br>"
            "Parameters: " + params.compose() + "<br>"
            "Body: " + body + "<br>"
            "</body></html>");

        channel->sendHttpResponse(res);
        channel->close();
    });

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false, [&](Timer*) {
        app.stop();
    });

    return app.run();
}
