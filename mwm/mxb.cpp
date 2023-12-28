#include "mxb.hpp"

namespace 
{
    static xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_generic_error_t * err;
    Logger log;
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

void 
mxb_kill_client(xcb_window_t window) 
{
    xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, NULL);

    xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply(conn, delete_cookie, NULL);

    if (!protocols_reply || !delete_reply) 
    {
        log.log(ERROR, __func__, "Could not create atoms.");
        free(protocols_reply);
        free(delete_reply);
        return;
    }

    xcb_client_message_event_t ev = {0};
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = window;
    ev.format = 32;
    ev.sequence = 0;
    ev.type = protocols_reply->atom;
    ev.data.data32[0] = delete_reply->atom;
    ev.data.data32[1] = XCB_CURRENT_TIME;

    xcb_send_event(conn, 0, window, XCB_EVENT_MASK_NO_EVENT, (char *) & ev);

    free(protocols_reply);
    free(delete_reply);
}

void
getCurrentEventMask(xcb_connection_t* conn, xcb_window_t window) 
{
    // Get the window attributes
    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(conn, window);
    xcb_get_window_attributes_reply_t* attr_reply = xcb_get_window_attributes_reply(conn, attr_cookie, NULL);

    // Check if the reply is valid
    if (attr_reply) 
    {
        log_info("event_mask: " + std::to_string(attr_reply->your_event_mask));
        log_info("do_not_propagate_mask: " + std::to_string(attr_reply->do_not_propagate_mask));
        log_info("override_redirect: " + std::to_string(attr_reply->override_redirect));
        log_info("map_state: " + std::to_string(attr_reply->map_state));
        log_info("all_event_masks: " + std::to_string(attr_reply->all_event_masks));
        free(attr_reply);
    }
    else 
    {
        log_error("Unable to get window attributes.");
    }
}