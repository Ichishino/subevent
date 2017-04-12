#include <iostream>

#include "subevent.hpp"
#include "subevent_http.hpp"

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public NetApplication
{
protected:
    bool onInit() override
    {
        NetApplication::onInit();

        mHttpClient = HttpClient::newInstance();

        std::string url = "http://example.com";

        // GET request
        mHttpClient->requestGet(
            url, [](const HttpClientPtr& client, int errorCode) {
        
            if (errorCode != 0)
            {
                // network error
                return;
            }

            uint16_t statusCode =
                client->getResponse().getStatusCode();
            
            if (statusCode == 200)
            {
                std::string response =
                    client->getResponse().getBodyAsString();

                std::cout << response << std::endl;
            }
        });

        // app end timer
        mEndTimer.start(60 * 1000, false, [&](Timer*) {
            stop();
        });

        return true;
    }

    void onExit() override
    {
        mHttpClient->close();

        NetApplication::onExit();
    }

private:
    HttpClientPtr mHttpClient;
    Timer mEndTimer;
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    MyApp app;
    return app.run();
}
