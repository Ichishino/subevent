#ifndef SUBEVENT_NET_HEADERS_HPP
#define SUBEVENT_NET_HEADERS_HPP

//----------------------------------------------------------------------------//
// Headers
//----------------------------------------------------------------------------//

#include <subevent/network.hpp>
#include <subevent/socket.hpp>
#include <subevent/tcp.hpp>
#include <subevent/udp.hpp>

#ifdef SEV_HEADER_ONLY
#include <subevent/network.inl>
#include <subevent/socket.inl>
#include <subevent/socket_controller.inl>
#include <subevent/socket_selector_linux.inl>
#include <subevent/socket_selector_win.inl>
#include <subevent/tcp.inl>
#include <subevent/udp.inl>
#endif

#endif // SUBEVENT_NET_HEADERS_HPP
