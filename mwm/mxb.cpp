#include "mxb.hpp"

namespace 
{
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_generic_error_t * err;
}

int
check(xcb_void_cookie_t cookie)
{
    err = xcb_request_check(conn, cookie);
    if (err)
    {
        const int & error_code = err->error_code;
        free(err);
        return error_code;
    }
    return 0;
}

int
mxb_create_win(xcb_window_t win, const xcb_window_t & parent, const uint16_t & x, const uint16_t & y, const uint16_t & width, const uint16_t & height)
{
    win = xcb_generate_id(conn);
    xcb_void_cookie_t cookie = xcb_create_window
    (
        conn, 
        XCB_COPY_FROM_PARENT, 
        win, 
        screen->root, 
        x,
        y, 
        width, 
        height, 
        0, 
        XCB_WINDOW_CLASS_INPUT_OUTPUT, 
        screen->root_visual, 
        0, 
        NULL
    );
    return check(cookie);
}