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
        setRequestHandler("/", SEV_BIND_1(this, MyThread::onMyHandler));
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
        res.setStatusCode(200);
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

    void onHttpRequest(const HttpChannelPtr& channel) override
    {
        // default handler
        HttpChannelThread::onHttpRequest(channel);
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

    // app end timer
    Timer timer;
    timer.start(60 * 1000, false, [&](Timer*) {
        app.stop();
    });

    return app.run();
}
