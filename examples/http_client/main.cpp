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
    HttpClientPtr http = HttpClient::newInstance(&app);

    std::string url = "http://example.com";

    // GET request
    http->requestGet(url, [&](const HttpClientPtr&, int errorCode) {

        if (errorCode == 0)
        {
            if (http->getResponse().getStatusCode() == 200)
            {
                std::string response =
                    http->getResponse().getBodyAsString();

                std::cout << response << std::endl;
            }
        }

        app.stop();
    });

    return app.run();
}
