#ifndef GLOBALS_H
#define GLOBALS_H

#include <xcb/xcb.h>

#define U32_MAX 0xFFFFFFFF

extern xcb_connection_t *conn;

typedef xcb_void_cookie_t VoidC;

#endif/* GLOBALS_H */