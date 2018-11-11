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

    HttpRequest req;
    req.setMethod("GET");

    HttpResponse res;

    // GET request
    int result = HttpClient::request(url, req, res);

    if (result == 0)
    {
        if (res.getStatusCode() == HttpStatusCode::Ok)
        {
            std::cout << res.getBodyAsString() << std::endl;
        }
    }

    return 0;
}
