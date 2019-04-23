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

    http->getRequest().setMethod("GET");

    // GET request
    bool result = http->request(
        url, [&app](const HttpClientPtr& http, int errorCode) {

        if (errorCode == 0)
        {
            if (http->getResponse().getStatusCode() == HttpStatusCode::Ok)
            {
                std::string response =
                    http->getResponse().getBodyAsString();

                std::cout << response << std::endl;
            }
        }

        app.stop();
    });

    if (!result)
    {
        // internal error
        app.stop();
    }

    return app.run();
}
