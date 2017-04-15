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
    std::string url = "http://example.com";

    HttpHeader reqHeader;
    HttpResponse res;

    // GET request
    int result = HttpClient::requestGet(url, reqHeader, res);

    if (result == 0)
    {
        if (res.getStatusCode() == 200)
        {
            std::cout << res.getBodyAsString() << std::endl;
        }
    }

    return 0;
}
