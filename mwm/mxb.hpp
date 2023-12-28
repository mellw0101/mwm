#ifndef MXB_HPP
#define MXB_HPP
#include "include.hpp"

int
check(xcb_void_cookie_t cookie);

int
mxb_create_win(xcb_window_t win, const xcb_window_t & parent, const uint16_t & x, const uint16_t & y, const uint16_t & width, const uint16_t & height);

void 
mxb_kill_client(xcb_window_t window);

void
getCurrentEventMask(xcb_connection_t* conn, xcb_window_t window);

void 
mxb_apply_event_mask(uint32_t values, const xcb_window_t &win);

#endif /* MXB_HPP */