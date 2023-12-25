#include <cstddef>
#include <cstdint>
#include <cstring>
#include <xcb/xcb.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>     // For fork() and exec()
#include <sys/wait.h>   // For waitpid()
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xcb/xcb_cursor.h> /* For cursor */

//#include "Log.hpp"

//Log log("/home/mellw/log");

static xcb_connection_t *connection;
static xcb_generic_event_t *ev;
xcb_window_t root;
/*

void 
launchTerminal() 
{
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (execlp("alacritty", "alacritty", (char *) NULL) == -1) {
            std::cerr << "Failed to launch terminal" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, nullptr, 0); // Optionally wait for the terminal to close
    } else {
        // Fork failed
        std::cerr << "Failed to fork" << std::endl;
    }
}
*/
void 
launchTerminal() 
{
    if (fork() == 0) {
        setsid();
        execlp("xterm", "xterm", (char *)__null);
        exit(1); // Exit if exec fails
    }
}
void 
setDefaultCursor(xcb_connection_t* connection, xcb_screen_t* screen, xcb_window_t window) 
{
    xcb_cursor_context_t *ctx;
    if (xcb_cursor_context_new(connection, screen, &ctx) < 0) {
        std::cerr << "Failed to create cursor context" << std::endl;
        return;
    }

    xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, "left_ptr");
    if (!cursor) {
        std::cerr << "Failed to load cursor" << std::endl;
        xcb_cursor_context_free(ctx);
        return;
    }

    uint32_t values[] = { cursor };
    xcb_change_window_attributes(connection, window, XCB_CW_CURSOR, values);
    xcb_cursor_context_free(ctx);
}
xcb_atom_t 
get_atom(xcb_connection_t* connection, const char* atom_name) 
{
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 0, strlen(atom_name), atom_name);
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, NULL);
    if (!reply) return XCB_ATOM_NONE;

    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
void
setRootWindowBackgroundWhite(xcb_connection_t* connection, xcb_screen_t* screen) 
{
    // Get the white pixel value
    uint32_t white_pixel = screen->white_pixel;

    // Change the root window's background to white
    uint32_t values[1] = {white_pixel};
    xcb_change_window_attributes(connection, screen->root, XCB_CW_BACK_PIXEL, values);

    // Clear the root window to update its appearance
    xcb_clear_area(connection, 0, screen->root, 0, 0, screen->width_in_pixels, screen->height_in_pixels);

    // Flush the request to the X server
    xcb_flush(connection);
}

class Event {
public:
    Event()
    {
        initialize_keysyms();
    }
    ~Event() 
    {
        if (keysyms) 
        {
            xcb_key_symbols_free(keysyms);
        }
    }
    void 
    handler(xcb_generic_event_t *ev)
    {   
        switch (ev->response_type) 
        {
            case XCB_KEY_PRESS: 
            {
                key_press_handler(ev);
                break;
            }
            case XCB_MAP_REQUEST: 
            {
                //map_req_handler(ev);
                break;
            }
            case XCB_BUTTON_PRESS: 
            {
                //button_press_handler(ev);
                break;
            }
            case XCB_MOTION_NOTIFY:
            {
                //motion_notify_handler(ev);
                break;
            }
            case XCB_BUTTON_RELEASE:
            {
                //button_release_handler(ev);
                break;
            }
        }
    }
private:
    xcb_key_symbols_t *keysyms;
    xcb_keycode_t t{}; // Member variable to store the keycode for 'T'
    bool isMoving = false;
    xcb_window_t movingWindow{};
    int16_t startPosX{}, startPosY{};
    xcb_connection_t *connection{};

    void 
    key_press_handler(xcb_generic_event_t *ev)
    {   
        xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)ev;
        /* Check if ALT+CTRL+T was pressed */
        if (key_event->detail == t && 
            (key_event->state & XCB_MOD_MASK_CONTROL) &&
            (key_event->state & XCB_MOD_MASK_1)) 
        {
            launchTerminal();
        }
    }
    void 
    initialize_keysyms() 
    {
        keysyms = xcb_key_symbols_alloc(connection);
        if (keysyms) 
        {
            xcb_keycode_t* t_keycodes = xcb_key_symbols_get_keycode(keysyms, 0x74);
            if (t_keycodes) 
            {
                t = *t_keycodes;
                free(t_keycodes);
            }
        }
    }
    void
    map_req_handler(xcb_generic_event_t *ev) 
    {
        auto* map_req = reinterpret_cast<xcb_map_request_event_t*>(ev);
        // Set event masks for the window
        uint32_t values[1] = 
        {    
            XCB_EVENT_MASK_ENTER_WINDOW |
            XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_FOCUS_CHANGE |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_PROPERTY_CHANGE |
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_KEY_PRESS
        };
        xcb_change_window_attributes(connection, map_req->window, XCB_CW_EVENT_MASK, values);
        manageNewWindow(connection, map_req->window);
        xcb_map_window(connection, map_req->window); // Map the window
        xcb_flush(connection); // Ensure the request is sent to the server
    }
    void 
    manageNewWindow(xcb_connection_t* connection, xcb_window_t new_window) 
    {
        xcb_atom_t XA_WINDOW = get_atom(connection, "WINDOW");

        // Get the atom for _NET_CLIENT_LIST
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 0, strlen("_NET_CLIENT_LIST"), "_NET_CLIENT_LIST");
        xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, NULL);
        xcb_atom_t net_client_list_atom = reply->atom;
        free(reply);

        // Get the current _NET_CLIENT_LIST property
        xcb_get_property_cookie_t prop_cookie = xcb_get_property(connection, 0, root, net_client_list_atom, XA_WINDOW, 0, 1024);
        xcb_get_property_reply_t* prop_reply = xcb_get_property_reply(connection, prop_cookie, NULL);

        // Prepare the new client list
        xcb_window_t *client_list = NULL;
        int num_clients = 0;
        if (prop_reply && xcb_get_property_value_length(prop_reply) > 0) 
        {
            client_list = (xcb_window_t*)xcb_get_property_value(prop_reply);
            num_clients = xcb_get_property_value_length(prop_reply) / sizeof(xcb_window_t);
        }
        
        // Append the new window
        xcb_window_t* new_client_list = (xcb_window_t*)malloc((num_clients + 1) * sizeof(xcb_window_t));
        memcpy(new_client_list, client_list, num_clients * sizeof(xcb_window_t));
        new_client_list[num_clients] = new_window;

        // Set the updated client list
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, root, net_client_list_atom, XA_WINDOW, 32, num_clients + 1, new_client_list);

        free(new_client_list);
        if (prop_reply) 
        {
            free(prop_reply);
        }

        // ... [rest of your management code] ...
    }
    void 
    button_press_handler(xcb_generic_event_t *ev) 
    {
        xcb_button_press_event_t* button_ev = (xcb_button_press_event_t*)ev;
        xcb_key_press_event_t *key_ev = (xcb_key_press_event_t *)ev;
        
        if ((button_ev->detail == XCB_BUTTON_INDEX_1) &&
            (key_ev->state & XCB_MOD_MASK_1) && 
            (button_ev->child != root)
        ) 
        {
            //log.log(__func__);
            isMoving = true;
            movingWindow = button_ev->child; // The window to be moved
            startPosX = button_ev->event_x;
            startPosY = button_ev->event_y;

            xcb_grab_pointer
            (
                connection,
                0, 
                button_ev->child, 
                XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC,
                XCB_NONE,
                XCB_NONE,
                XCB_CURRENT_TIME
            );
        }
    }
    void
    motion_notify_handler(xcb_generic_event_t *ev)
    {
        auto *motion_notify_ev = (xcb_motion_notify_event_t*)ev;

        if (isMoving) 
        {
            int16_t dx = motion_notify_ev->root_x - startPosX;
            int16_t dy = motion_notify_ev->root_y - startPosY;

            uint32_t values[2] = 
            {
                static_cast<uint32_t>(motion_notify_ev->root_x - dx),
                static_cast<uint32_t>(motion_notify_ev->root_y - dy)
            };
            xcb_configure_window
            (
                connection,
                movingWindow,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                values
            );
        }
    }
    void
    button_release_handler(xcb_generic_event_t *ev)
    {
        auto *button_release_ev = (xcb_button_release_event_t*)ev;
        if (button_release_ev->detail == XCB_BUTTON_INDEX_1 && isMoving) 
        {
            isMoving = false;
            xcb_ungrab_pointer(connection, XCB_CURRENT_TIME);
        }
    }
};

void 
run() 
{
    Event event;
    while ((ev = xcb_wait_for_event(connection))) {
        event.handler(ev);
        free(ev);
    }
}

int 
main() 
{
    connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection)) 
    {
        std::cerr << "Error opening XCB connection" << std::endl;
        return -1;
    }

    const xcb_setup_t *setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;
    
    root = xcb_generate_id(connection);
    
    uint32_t values[2] = 
    {
        screen->white_pixel,
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_FOCUS_CHANGE |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION
    };
    xcb_create_window
    (
        connection, XCB_COPY_FROM_PARENT,
        root, screen->root,
        0, 0, screen->width_in_pixels, screen->height_in_pixels, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, 
        values
    );
    setDefaultCursor(connection, screen, root);
    xcb_map_window(connection, root);
    xcb_flush(connection);

    
    run();

    xcb_disconnect(connection);
    return 0;
}