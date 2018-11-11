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
        // Method
        const std::string& method =
            channel->getRequest().getMethod();
         
        HttpUrl url(channel->getRequest().getPath());

        // Path
        std::string path = url.getPath();
            
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
                "Path: " + path + "<br>"
                "Parameters: " + params.compose() + "<br>"
                "Body: " + body + "<br>"
            "</body></html>");

        channel->sendHttpResponse(res);
        channel->close();
    }

    void onMyServerStop(const HttpChannelPtr& channel)
    {
        channel->sendHttpResponse(
            HttpStatusCode::Ok, "OK",
            "<html><body>OK</body></html>");
        channel->close();

        // stop server
        Application::getCurrent()->stop();
    }
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
