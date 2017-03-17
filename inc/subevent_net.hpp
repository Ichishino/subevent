#ifndef SUBEVENT_NET_HEADERS_HPP
#define SUBEVENT_NET_HEADERS_HPP

//----------------------------------------------------------------------------//
// Headers
//----------------------------------------------------------------------------//

#include <subevent/socket.hpp>
#include <subevent/network.hpp>
#include <subevent/tcp.hpp>
#include <subevent/udp.hpp>

#ifdef SEV_HEADER_ONLY
#include <subevent_impl/network.inl>
#include <subevent_impl/tcp.inl>
#include <subevent_impl/udp.inl>
#include <subevent_impl/socket.inl>
#include <subevent_impl/socket_controller.inl>
#include <subevent_impl/socket_selector_linux.inl>
#include <subevent_impl/socket_selector_win.inl>
#endif

#endif // SUBEVENT_NET_HEADERS_HPP
