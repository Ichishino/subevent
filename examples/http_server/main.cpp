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
        res.setStatusCode(HttpStatusCode::Ok);
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

    // server stop handler
    server->setRequestHandler(
        "/stop", [&](const HttpChannelPtr& channel) {

        // http://127.0.0.1:9000/stop

        channel->sendHttpResponse(
            HttpStatusCode::Ok, "OK",
            "<html><body>OK</body></html>");
        channel->close();

        // stop server
        app.stop();
    });

    return app.run();
}
