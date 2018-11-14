#ifndef SUBEVENT_HTTP_HEADERS_HPP
#define SUBEVENT_HTTP_HEADERS_HPP

//----------------------------------------------------------------------------//
// Headers
//----------------------------------------------------------------------------//

#if (SEV_CPP_VER >= 17)
#   if __has_include(<openssl/ssl.h>)
#       define SEV_SUPPORTS_SSL
#   else
#
#   endif
#else
#   define SEV_SUPPORTS_SSL
#endif

#include <subevent/ssl_socket.hpp>
#include <subevent/http.hpp>
#include <subevent/http_client.hpp>
#include <subevent/http_server.hpp>
#include <subevent/http_server_worker.hpp>
#include <subevent/ws.hpp>

#ifdef SEV_HEADER_ONLY
#include <subevent/ssl_socket.inl>
#include <subevent/http.inl>
#include <subevent/http_client.inl>
#include <subevent/http_server.inl>
#include <subevent/http_server_worker.inl>
#include <subevent/ws.inl>
#endif

#endif // SUBEVENT_HTTP_HEADERS_HPP
