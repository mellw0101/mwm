#ifndef MXB_HPP
#define MXB_HPP
#include "include.hpp"

int
check(xcb_void_cookie_t cookie);

int
mxb_create_win(xcb_window_t win, const xcb_window_t & parent, const uint16_t & x, const uint16_t & y, const uint16_t & width, const uint16_t & height);

#endif /* MXB_HPP */