#include <xcb/xcb.h>
#include <xcb/xproto.h>
#define main_cpp
#include "include.hpp"

Logger log;
XCB xcb;

std::vector<client *> client_list; // global list of clients
// u_Ptr_Vec<client> U_client_list; // global list of clients
// Vec<u_Ptr<int>> b;
std::vector<desktop *> desktop_list;

desktop * cur_d;
client * focused_client;
win_data data;

static xcb_connection_t * conn;
static xcb_ewmh_connection_t * ewmh; 
static const xcb_setup_t * setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t * screen;



static void 
draw_text(const char * str , const COLOR & text_color, const COLOR & bg_color, const xcb_window_t & win, const int16_t & x, const int16_t & y);

namespace XCBwm 
{
    void
    check_err(xcb_connection_t * connection, xcb_void_cookie_t cookie , const char * sender_function, const char * err_msg)
    {
        xcb_generic_error_t * err = xcb_request_check(connection, cookie);
        if (err)
        {
            log.log(ERROR, sender_function, err_msg, err->error_code);
            free(err);
        }
    }

    xcb_window_t 
    get_parent_window(xcb_window_t window) 
    {
        xcb_query_tree_cookie_t cookie;
        xcb_query_tree_reply_t *reply;

        cookie = xcb_query_tree(conn, window);
        reply = xcb_query_tree_reply(conn, cookie, NULL);

        if (!reply) 
        {
            log.log(ERROR, __func__, "Error: Unable to query the window tree.");
            return (xcb_window_t) 0; // Invalid window ID
        }

        xcb_window_t parent_window = reply->parent;

        free(reply);
        return parent_window;
    }

    xcb_window_t 
    get_root_window(xcb_window_t window) 
    {
        xcb_query_tree_cookie_t cookie;
        xcb_query_tree_reply_t *reply;

        cookie = xcb_query_tree(conn, window);
        reply = xcb_query_tree_reply(conn, cookie, NULL);

        if (!reply) 
        {
            log.log(ERROR, __func__, "Error: Unable to query the window tree.");
            return (xcb_window_t) 0; // Invalid window ID
        }

        xcb_window_t root_window = reply->root;

        free(reply);
        return root_window;
    }

    xcb_window_t *
    get_window_children(xcb_connection_t *conn, xcb_window_t window, uint32_t *child_count) 
    {
        *child_count = 0;
        xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, window);
        xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, NULL);

        if (!reply) 
        {
            log.log(ERROR, __func__, "Error: Unable to query the window tree.");
            return NULL;
        }

        *child_count = xcb_query_tree_children_length(reply);
        xcb_window_t *children = static_cast<xcb_window_t *>(malloc(*child_count * sizeof(xcb_window_t)));

        if (!children) 
        {
            log.log(ERROR, __func__, "Error: Unable to allocate memory for children.");
            free(reply);
            return NULL;
        }

        memcpy(children, xcb_query_tree_children(reply), *child_count * sizeof(xcb_window_t));
        
        free(reply);
        return children;
    }
};

namespace get 
{
    client * 
    client_from_win(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->win) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_all_win(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->win || * w == c->frame || * w == c->titlebar || * w == c->close_button) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_frame(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->frame) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_close_button(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->close_button) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_titlebar(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->titlebar) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    xcb_atom_t
    atom(const char * atom_name) 
    {
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom
        (
            conn, 
            0, 
            strlen(atom_name), 
            atom_name
        );
        
        xcb_intern_atom_reply_t * reply = xcb_intern_atom_reply(conn, cookie, NULL);
        
        if (!reply) 
        {
            return XCB_ATOM_NONE;
        } 

        xcb_atom_t atom = reply->atom;
        free(reply);
        return atom;
    }

    std::string
    name(client * & c)
    {
        log.log(FUNC, __func__);
        xcb_get_property_reply_t * reply;
        unsigned int reply_len;
        char * name;

        reply = xcb_get_property_reply
        (
            conn, 
            xcb_get_property
            (
                conn, 
                false,
                c->win, 
                XCB_ATOM_WM_NAME,
                XCB_GET_PROPERTY_TYPE_ANY, 
                0,
                60
            ), 
            NULL
        );

        if (!reply || xcb_get_property_value_length(reply) == 0)
        {
            if (reply != nullptr)
            {
                log.log(ERROR, __func__, "reply length is = 0");
                free(reply);
                return "";
            }

            log.log(ERROR, __func__, "reply == nullptr");
            return "";
        }

        reply_len = xcb_get_property_value_length(reply);
        name = static_cast<char *>(malloc(sizeof(char) * (reply_len+1)));
        memcpy(name, xcb_get_property_value(reply), reply_len);
        name[reply_len] = '\0';

        if (reply)
        {
            free(reply);
        }

        log.log(INFO, __func__, "name = " + std::string(name)); 
        std::string sname = std::string(name);
        free(name);

        return sname;
    }

    std::string
    propertyAtom_enum_to_string(xcb_atom_enum_t property)
    {
        switch (property) 
        {
            case XCB_ATOM_ANY:                  return "XCB_ATOM_ANY";
            case XCB_ATOM_PRIMARY:              return "XCB_ATOM_PRIMARY";
            case XCB_ATOM_SECONDARY:            return "XCB_ATOM_SECONDARY";
            case XCB_ATOM_ARC:                  return "XCB_ATOM_ARC";
            case XCB_ATOM_ATOM:                 return "XCB_ATOM_ATOM";
            case XCB_ATOM_BITMAP:               return "XCB_ATOM_BITMAP";
            case XCB_ATOM_CARDINAL:             return "XCB_ATOM_CARDINAL";
            case XCB_ATOM_COLORMAP:             return "XCB_ATOM_COLORMAP";
            case XCB_ATOM_CURSOR:               return "XCB_ATOM_CURSOR";
            case XCB_ATOM_CUT_BUFFER0:          return "XCB_ATOM_CUT_BUFFER0";
            case XCB_ATOM_CUT_BUFFER1:          return "XCB_ATOM_CUT_BUFFER1";
            case XCB_ATOM_CUT_BUFFER2:          return "XCB_ATOM_CUT_BUFFER2";
            case XCB_ATOM_CUT_BUFFER3:          return "XCB_ATOM_CUT_BUFFER3";
            case XCB_ATOM_CUT_BUFFER4:          return "XCB_ATOM_CUT_BUFFER4";
            case XCB_ATOM_CUT_BUFFER5:          return "XCB_ATOM_CUT_BUFFER5";
            case XCB_ATOM_CUT_BUFFER6:          return "XCB_ATOM_CUT_BUFFER6";
            case XCB_ATOM_CUT_BUFFER7:          return "XCB_ATOM_CUT_BUFFER7";
            case XCB_ATOM_DRAWABLE:             return "XCB_ATOM_DRAWABLE";
            case XCB_ATOM_FONT:                 return "XCB_ATOM_FONT";
            case XCB_ATOM_INTEGER:              return "XCB_ATOM_INTEGER";
            case XCB_ATOM_PIXMAP:               return "XCB_ATOM_PIXMAP";
            case XCB_ATOM_POINT:                return "XCB_ATOM_POINT";
            case XCB_ATOM_RECTANGLE:            return "XCB_ATOM_RECTANGLE";
            case XCB_ATOM_RESOURCE_MANAGER:     return "XCB_ATOM_RESOURCE_MANAGER";
            case XCB_ATOM_RGB_COLOR_MAP:        return "XCB_ATOM_RGB_COLOR_MAP";
            case XCB_ATOM_RGB_BEST_MAP:         return "XCB_ATOM_RGB_BEST_MAP";
            case XCB_ATOM_RGB_BLUE_MAP:         return "XCB_ATOM_RGB_BLUE_MAP";
            case XCB_ATOM_RGB_DEFAULT_MAP:      return "XCB_ATOM_RGB_DEFAULT_MAP";
            case XCB_ATOM_RGB_GRAY_MAP:         return "XCB_ATOM_RGB_GRAY_MAP";
            case XCB_ATOM_RGB_GREEN_MAP:        return "XCB_ATOM_RGB_GREEN_MAP";
            case XCB_ATOM_RGB_RED_MAP:          return "XCB_ATOM_RGB_RED_MAP";
            case XCB_ATOM_STRING:               return "XCB_ATOM_STRING";
            case XCB_ATOM_VISUALID:             return "XCB_ATOM_VISUALID";
            case XCB_ATOM_WINDOW:               return "XCB_ATOM_WINDOW";
            case XCB_ATOM_WM_COMMAND:           return "XCB_ATOM_WM_COMMAND";
            case XCB_ATOM_WM_HINTS:             return "XCB_ATOM_WM_HINTS";
            case XCB_ATOM_WM_CLIENT_MACHINE:    return "XCB_ATOM_WM_CLIENT_MACHINE";
            case XCB_ATOM_WM_ICON_NAME:         return "XCB_ATOM_WM_ICON_NAME";
            case XCB_ATOM_WM_ICON_SIZE:         return "XCB_ATOM_WM_ICON_SIZE";
            case XCB_ATOM_WM_NAME:              return "XCB_ATOM_WM_NAME";
            case XCB_ATOM_WM_NORMAL_HINTS:      return "XCB_ATOM_WM_NORMAL_HINTS";
            case XCB_ATOM_WM_SIZE_HINTS:        return "XCB_ATOM_WM_SIZE_HINTS";
            case XCB_ATOM_WM_ZOOM_HINTS:        return "XCB_ATOM_WM_ZOOM_HINTS";
            case XCB_ATOM_MIN_SPACE:            return "XCB_ATOM_MIN_SPACE";
            case XCB_ATOM_NORM_SPACE:           return "XCB_ATOM_NORM_SPACE";
            case XCB_ATOM_MAX_SPACE:            return "XCB_ATOM_MAX_SPACE";
            case XCB_ATOM_END_SPACE:            return "XCB_ATOM_END_SPACE";
            case XCB_ATOM_SUPERSCRIPT_X:        return "XCB_ATOM_SUPERSCRIPT_X";
            case XCB_ATOM_SUPERSCRIPT_Y:        return "XCB_ATOM_SUPERSCRIPT_Y";
            case XCB_ATOM_SUBSCRIPT_X:          return "XCB_ATOM_SUBSCRIPT_X";
            case XCB_ATOM_SUBSCRIPT_Y:          return "XCB_ATOM_SUBSCRIPT_Y";
            case XCB_ATOM_UNDERLINE_POSITION:   return "XCB_ATOM_UNDERLINE_POSITION";
            case XCB_ATOM_UNDERLINE_THICKNESS:  return "XCB_ATOM_UNDERLINE_THICKNESS";
            case XCB_ATOM_STRIKEOUT_ASCENT:     return "XCB_ATOM_STRIKEOUT_ASCENT";
            case XCB_ATOM_STRIKEOUT_DESCENT:    return "XCB_ATOM_STRIKEOUT_DESCENT";
            case XCB_ATOM_ITALIC_ANGLE:         return "XCB_ATOM_ITALIC_ANGLE";
            case XCB_ATOM_X_HEIGHT:             return "XCB_ATOM_X_HEIGHT";
            case XCB_ATOM_QUAD_WIDTH:           return "XCB_ATOM_QUAD_WIDTH";
            case XCB_ATOM_WEIGHT:               return "XCB_ATOM_WEIGHT";
            case XCB_ATOM_POINT_SIZE:           return "XCB_ATOM_POINT_SIZE";
            case XCB_ATOM_RESOLUTION:           return "XCB_ATOM_RESOLUTION";
            case XCB_ATOM_COPYRIGHT:            return "XCB_ATOM_COPYRIGHT";
            case XCB_ATOM_NOTICE:               return "XCB_ATOM_NOTICE";
            case XCB_ATOM_FONT_NAME:            return "XCB_ATOM_FONT_NAME";
            case XCB_ATOM_FAMILY_NAME:          return "XCB_ATOM_FAMILY_NAME";
            case XCB_ATOM_FULL_NAME:            return "XCB_ATOM_FULL_NAME";
            case XCB_ATOM_CAP_HEIGHT:           return "XCB_ATOM_CAP_HEIGHT";
            case XCB_ATOM_WM_CLASS:             return "XCB_ATOM_WM_CLASS";
            case XCB_ATOM_WM_TRANSIENT_FOR:     return "XCB_ATOM_WM_TRANSIENT_FOR";
        }
    }

    std::string 
    getNetAtomAsString(xcb_atom_t atom) 
    {
        static const std::unordered_map<xcb_atom_t, std::string> atomMap = {
            {XCB_ATOM_NONE, "XCB_ATOM_NONE"},
            {ewmh->_NET_SUPPORTED, "_NET_SUPPORTED"},
            {ewmh->_NET_CLIENT_LIST, "_NET_CLIENT_LIST"},
            {ewmh->_NET_CLIENT_LIST_STACKING, "_NET_CLIENT_LIST_STACKING"},
            {ewmh->_NET_NUMBER_OF_DESKTOPS, "_NET_NUMBER_OF_DESKTOPS"},
            {ewmh->_NET_DESKTOP_GEOMETRY, "_NET_DESKTOP_GEOMETRY"},
            {ewmh->_NET_DESKTOP_VIEWPORT, "_NET_DESKTOP_VIEWPORT"},
            {ewmh->_NET_CURRENT_DESKTOP, "_NET_CURRENT_DESKTOP"},
            {ewmh->_NET_DESKTOP_NAMES, "_NET_DESKTOP_NAMES"},
            {ewmh->_NET_ACTIVE_WINDOW, "_NET_ACTIVE_WINDOW"},
            {ewmh->_NET_WORKAREA, "_NET_WORKAREA"},
            {ewmh->_NET_SUPPORTING_WM_CHECK, "_NET_SUPPORTING_WM_CHECK"},
            {ewmh->_NET_VIRTUAL_ROOTS, "_NET_VIRTUAL_ROOTS"},
            {ewmh->_NET_DESKTOP_LAYOUT, "_NET_DESKTOP_LAYOUT"},
            {ewmh->_NET_SHOWING_DESKTOP, "_NET_SHOWING_DESKTOP"},
            {ewmh->_NET_CLOSE_WINDOW, "_NET_CLOSE_WINDOW"},
            {ewmh->_NET_MOVERESIZE_WINDOW, "_NET_MOVERESIZE_WINDOW"},
            {ewmh->_NET_WM_MOVERESIZE, "_NET_WM_MOVERESIZE"},
            {ewmh->_NET_RESTACK_WINDOW, "_NET_RESTACK_WINDOW"},
            {ewmh->_NET_REQUEST_FRAME_EXTENTS, "_NET_REQUEST_FRAME_EXTENTS"},
            {ewmh->_NET_WM_NAME, "_NET_WM_NAME"},
            {ewmh->_NET_WM_VISIBLE_NAME, "_NET_WM_VISIBLE_NAME"},
            {ewmh->_NET_WM_ICON_NAME, "_NET_WM_ICON_NAME"},
            {ewmh->_NET_WM_VISIBLE_ICON_NAME, "_NET_WM_VISIBLE_ICON_NAME"},
            {ewmh->_NET_WM_DESKTOP, "_NET_WM_DESKTOP"},
            {ewmh->_NET_WM_WINDOW_TYPE, "_NET_WM_WINDOW_TYPE"},
            {ewmh->_NET_WM_STATE, "_NET_WM_STATE"},
            {ewmh->_NET_WM_ALLOWED_ACTIONS, "_NET_WM_ALLOWED_ACTIONS"},
            {ewmh->_NET_WM_STRUT, "_NET_WM_STRUT"},
            {ewmh->_NET_WM_STRUT_PARTIAL, "_NET_WM_STRUT_PARTIAL"},
            {ewmh->_NET_WM_ICON_GEOMETRY, "_NET_WM_ICON_GEOMETRY"},
            {ewmh->_NET_WM_ICON, "_NET_WM_ICON"},
            {ewmh->_NET_WM_PID, "_NET_WM_PID"},
            {ewmh->_NET_WM_HANDLED_ICONS, "_NET_WM_HANDLED_ICONS"},
            {ewmh->_NET_WM_USER_TIME, "_NET_WM_USER_TIME"},
            {ewmh->_NET_WM_USER_TIME_WINDOW, "_NET_WM_USER_TIME_WINDOW"},
            {ewmh->_NET_FRAME_EXTENTS, "_NET_FRAME_EXTENTS"},
            {ewmh->_NET_WM_PING, "_NET_WM_PING"},
            {ewmh->_NET_WM_SYNC_REQUEST, "_NET_WM_SYNC_REQUEST"},
            {ewmh->_NET_WM_SYNC_REQUEST_COUNTER, "_NET_WM_SYNC_REQUEST_COUNTER"},
            {ewmh->_NET_WM_FULLSCREEN_MONITORS, "_NET_WM_FULLSCREEN_MONITORS"},
            {ewmh->_NET_WM_FULL_PLACEMENT, "_NET_WM_FULL_PLACEMENT"},
            {ewmh->UTF8_STRING, "UTF8_STRING"},
            {ewmh->WM_PROTOCOLS, "WM_PROTOCOLS"},
            {ewmh->MANAGER, "MANAGER"},
            {ewmh->_NET_WM_WINDOW_TYPE_DESKTOP, "_NET_WM_WINDOW_TYPE_DESKTOP"},
            {ewmh->_NET_WM_WINDOW_TYPE_DOCK, "_NET_WM_WINDOW_TYPE_DOCK"},
            {ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR, "_NET_WM_WINDOW_TYPE_TOOLBAR"},
            {ewmh->_NET_WM_WINDOW_TYPE_MENU, "_NET_WM_WINDOW_TYPE_MENU"},
            {ewmh->_NET_WM_WINDOW_TYPE_UTILITY, "_NET_WM_WINDOW_TYPE_UTILITY"},
            {ewmh->_NET_WM_WINDOW_TYPE_SPLASH, "_NET_WM_WINDOW_TYPE_SPLASH"},
            {ewmh->_NET_WM_WINDOW_TYPE_DIALOG, "_NET_WM_WINDOW_TYPE_DIALOG"},
            {ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU"},
            {ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU, "_NET_WM_WINDOW_TYPE_POPUP_MENU"},
            {ewmh->_NET_WM_WINDOW_TYPE_TOOLTIP, "_NET_WM_WINDOW_TYPE_TOOLTIP"},
            {ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION, "_NET_WM_WINDOW_TYPE_NOTIFICATION"},
            {ewmh->_NET_WM_WINDOW_TYPE_COMBO, "_NET_WM_WINDOW_TYPE_COMBO"},
            {ewmh->_NET_WM_WINDOW_TYPE_DND, "_NET_WM_WINDOW_TYPE_DND"},
            {ewmh->_NET_WM_WINDOW_TYPE_NORMAL, "_NET_WM_WINDOW_TYPE_NORMAL"},
            {ewmh->_NET_WM_STATE_MODAL, "_NET_WM_STATE_MODAL"},
            {ewmh->_NET_WM_STATE_STICKY, "_NET_WM_STATE_STICKY"},
            {ewmh->_NET_WM_STATE_MAXIMIZED_VERT, "_NET_WM_STATE_MAXIMIZED_VERT"},
            {ewmh->_NET_WM_STATE_MAXIMIZED_HORZ, "_NET_WM_STATE_MAXIMIZED_HORZ"},
            {ewmh->_NET_WM_STATE_SHADED, "_NET_WM_STATE_SHADED"},
            {ewmh->_NET_WM_STATE_SKIP_TASKBAR, "_NET_WM_STATE_SKIP_TASKBAR"},
            {ewmh->_NET_WM_STATE_SKIP_PAGER, "_NET_WM_STATE_SKIP_PAGER"},
            {ewmh->_NET_WM_STATE_HIDDEN, "_NET_WM_STATE_HIDDEN"},
            {ewmh->_NET_WM_STATE_FULLSCREEN, "_NET_WM_STATE_FULLSCREEN"},
            {ewmh->_NET_WM_STATE_ABOVE, "_NET_WM_STATE_ABOVE"},
            {ewmh->_NET_WM_STATE_BELOW, "_NET_WM_STATE_BELOW"},
            {ewmh->_NET_WM_STATE_DEMANDS_ATTENTION, "_NET_WM_STATE_DEMANDS_ATTENTION"},
            {ewmh->_NET_WM_ACTION_MOVE, "_NET_WM_ACTION_MOVE"},
            {ewmh->_NET_WM_ACTION_RESIZE, "_NET_WM_ACTION_RESIZE"},
            {ewmh->_NET_WM_ACTION_MINIMIZE, "_NET_WM_ACTION_MINIMIZE"},
            {ewmh->_NET_WM_ACTION_SHADE, "_NET_WM_ACTION_SHADE"},
            {ewmh->_NET_WM_ACTION_STICK, "_NET_WM_ACTION_STICK"},
            {ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ, "_NET_WM_ACTION_MAXIMIZE_HORZ"},
            {ewmh->_NET_WM_ACTION_MAXIMIZE_VERT, "_NET_WM_ACTION_MAXIMIZE_VERT"},
            {ewmh->_NET_WM_ACTION_FULLSCREEN, "_NET_WM_ACTION_FULLSCREEN"},
            {ewmh->_NET_WM_ACTION_CHANGE_DESKTOP, "_NET_WM_ACTION_CHANGE_DESKTOP"},
            {ewmh->_NET_WM_ACTION_CLOSE, "_NET_WM_ACTION_CLOSE"},
            {ewmh->_NET_WM_ACTION_ABOVE, "_NET_WM_ACTION_ABOVE"},
            {ewmh->_NET_WM_ACTION_BELOW, "_NET_WM_ACTION_BELOW"}
        };

        auto it = atomMap.find(atom);
        return (it != atomMap.end()) ? it->second : "Unknown Atom";
    }

    std::string 
    WindowProperty(client * c, const char * atom_name) 
    {
        xcb_get_property_reply_t *reply;
        unsigned int reply_len;
        char *propertyValue;

        reply = xcb_get_property_reply
        (
            conn,
            xcb_get_property
            (
                conn,
                false,
                c->win,
                atom
                (
                    atom_name
                ),
                XCB_GET_PROPERTY_TYPE_ANY,
                0,
                60
            ),
            NULL
        );

        if (!reply || xcb_get_property_value_length(reply) == 0) 
        {
            if (reply != nullptr) 
            {
                // log.log(ERROR, __func__, "reply length for property(" + std::string(atom_name) + ") = 0");
                free(reply);
                return "";
            }

            // log.log(ERROR, __func__, "reply == nullptr");
            return "";
        }

        reply_len = xcb_get_property_value_length(reply);
        propertyValue = static_cast<char *>(malloc(sizeof(char) * (reply_len + 1)));
        memcpy(propertyValue, xcb_get_property_value(reply), reply_len);
        propertyValue[reply_len] = '\0';

        if (reply) 
        {
            free(reply);
        }

        log.log(INFO, __func__, "property value(" + std::string(atom_name) + ") = " + std::string(propertyValue));
        std::string spropertyValue = std::string(propertyValue);
        free(propertyValue);

        return spropertyValue;
    }
}

class focus 
{
    public:
        static void
        client(client * c)
        {
            // LOG_func
            if (c == nullptr)
            {
                LOG_warning("client was nullptr");
                return;
            }
            raise_client(c);
            focus_input(c);
            focused_client = c;
        }

    private:
        static void  
        raise_client(struct client * c) 
        {
            xcb_configure_window
            (
                conn,
                c->frame,
                XCB_CONFIG_WINDOW_STACK_MODE, 
                (const uint32_t[1])
                {
                    XCB_STACK_MODE_ABOVE
                }
            );
            XCB_flush();
        }
        
        static void 
        focus_input(struct client * c)
        {
            if (!c)
            {
                LOG_warning("client was nullptr");
                return;
            }
            xcb_set_input_focus
            (
                conn, 
                XCB_INPUT_FOCUS_POINTER_ROOT, 
                c->win, 
                XCB_CURRENT_TIME
            );
            XCB_flush();
        }
};

class wm 
{
    public:
        static void 
        setWindowSize(client * c) 
        {
            xcb_configure_window
            (
                conn, 
                c->win, 
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, 
                (const uint32_t[2])
                {
                    static_cast<const uint32_t>(c->width), 
                    static_cast<const uint32_t>(c->height)
                }
            );
            xcb_flush(conn);
        }

        static void 
        setWindowPosition(client * c) 
        {
            xcb_configure_window
            (
                conn, 
                c->win, 
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, 
                (const uint32_t[2])
                {
                    static_cast<uint32_t>(c->x), 
                    static_cast<uint32_t>(c->y)           
                }
            );
            xcb_flush(conn);
        }

        static void
        launchTerminal() 
        {
            if (fork()) 
            {
                return;
            }
            setsid();
            execlp("/usr/bin/konsole", "/usr/bin/konsole");
        }

        static void
        kill_session()
        {
            if (fork() == 0) 
            {
                setsid();
                xcb_disconnect(conn);
                execlp("/bin/mwm-KILL", "mwm-KILL", (char *)nullptr);
                exit(1); // Exit if exec fails
            }
        }

        static void 
        flush_server(const char * function_name)
        {
            const uint8_t & status = xcb_flush(conn);
            if (status == 0) 
            {
                Log::ERROR(function_name, "]:[Failed to flush server");
                return;
            }
        }

        static void 
        raise_client(client * c) 
        {
            uint32_t values[1] = 
            {
                XCB_STACK_MODE_ABOVE
            };
            xcb_configure_window
            (
                conn,
                c->frame,
                XCB_CONFIG_WINDOW_STACK_MODE, 
                values
            );
            flush_server(__func__);
        }

        static void
        save_ogsize(client * c)
        {
            c->ogsize.x         = c->x; 
            c->ogsize.y         = c->y;
            c->ogsize.width     = c->width;
            c->ogsize.height    = c->height;
        }

        /* TODO */
        // MUST CHECK VALUES SOMETHING IS OFF
        static void 
        update_client(client * c) 
        {
            if (c == nullptr)
            {
                return;
            }

            uint16_t x, y, width, height;
            getWindowSize(c->frame, x, y, width, height);
            c->x        = x;
            c->y        = y;
            c->width    = width;
            c->height   = height;
        }

        static void 
        getWindowSize(xcb_window_t window, uint16_t & x, uint16_t & y, uint16_t & width, uint16_t & height) 
        {
            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
            xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

            if (geometry) 
            {
                x = geometry->x;
                y = geometry->y;
                width = geometry->width;
                height = geometry->height;
                free(geometry);
            } 
            else 
            {
                width = height = x = y = 200;
            }
        }

        static void
        ungrab_button(client * c, const uint8_t & mouse_button, const uint16_t & modifiers)
        {
            xcb_ungrab_button(
                conn,
                mouse_button, // Button to ungrab (e.g., left mouse button)
                c->win,
                modifiers // Modifiers to ungrab (e.g., Shift, Control, Alt, Super)
            );
            xcb_flush(conn); // Flush the request to the X server
        }

        static void
        grab_button(client * c, const uint8_t & mouse_button, const uint16_t & modifiers) 
        {
            LOG_func
            xcb_grab_button(
                conn, 
                1, // 'owner_events'. Set to 0 for no event propagation
                c->win, 
                XCB_EVENT_MASK_BUTTON_PRESS, // Event mask
                XCB_GRAB_MODE_ASYNC, // Pointer mode
                XCB_GRAB_MODE_ASYNC, // Keyboard mode
                XCB_NONE, // Confine to window: none
                XCB_NONE, // Cursor: none
                mouse_button, 
                modifiers
            );
            xcb_flush(conn); // Flush the request to the X server
        }

        static void
        cycle_focus()
        {
            bool focus = false;
            for (auto & c : client_list)
            {
                if (c)
                {
                    if (c == focused_client)
                    {
                        focus = true;
                        continue;
                    }
                    
                    if (focus)
                    {
                        focus::client(c);
                        return;  
                    }
                }
            }
        }
};

void /**
 *
 * @brief Function to remove a client based on its window ID 
 *
 */
removeClient(const xcb_window_t & win) 
{
    auto it = std::remove_if
    (
        client_list.begin(),
        client_list.end(),
        [win](const client * c) { return c->win == win; }
    );

    // Delete the removed clients to avoid memory leaks
    for (auto iter = it; iter != client_list.end(); ++iter) 
    {
        delete *iter;
    }

    // Erase the removed clients from the vector
    client_list.erase(it, client_list.end());
}

void 
show_hide_client(client * c, const show_hide & mode) 
{
    switch (mode) 
    {
        case HIDE:
        {
            xcb_unmap_window(conn, c->win);
            break;
        }

        case SHOW:
        {
            xcb_map_window(conn, c->win);
            break;
        }
    }
    xcb_flush(conn);    
}

class mv_client 
{
    public:
        mv_client(client * & c, const uint16_t & start_x, const uint16_t & start_y) 
        : c(c), start_x(start_x), start_y(start_y)
        {
            log.log(FUNC, __func__);
            grab_pointer();

            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }

    private:
        client * & c;
        const uint16_t & start_x;
        const uint16_t & start_y;
        bool shouldContinue = true;
        xcb_generic_event_t * ev;

        void 
        grab_pointer()
        {
            xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer
            (
                conn,
                false, // owner_events: false to not propagate events to other clients
                c->frame,
                XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC,
                XCB_NONE,
                XCB_NONE, 
                XCB_CURRENT_TIME
            );

            xcb_grab_pointer_reply_t * reply = xcb_grab_pointer_reply(conn, cookie, NULL);
            if (!reply || reply->status != XCB_GRAB_STATUS_SUCCESS) 
            {
                log.log(ERROR, __func__, "Could not grab pointer");
                free(reply);
                return;
            }
            free(reply); 
        }
    
        void 
        move_client(const uint16_t & x, const uint16_t & y)
        {
            xcb_configure_window
            (
                conn,
                c->frame,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t>(x), 
                    static_cast<const uint32_t>(y)
                }
            );
        }

        /* DEFENITIONS TO REDUCE REDUNDENT CODE IN 'snap' FUNCTION */
        #define RIGHT_  screen->width_in_pixels  - c->width
        #define BOTTOM  screen->height_in_pixels - c->height

        void 
        snap(const int16_t & x, const int16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : cur_d->current_clients)
            {
                // SNAP WINDOW TO 'RIGHT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x + cli->width - N && x < cli->x + cli->width + N) 
                    && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    // SNAP WINDOW TO 'RIGHT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC)
                    {  
                        move_client(cli->x + cli->width, cli->y);
                        return;
                    }

                    // SNAP WINDOW TO 'RIGHT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC)
                    {
                        move_client(cli->x + cli->width, (cli->y + cli->height) - c->height);
                        return;
                    }

                    move_client(cli->x + cli->width, y);
                    return;
                }

                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x + c->width > cli->x - N && x + c->width < cli->x + N) 
                    && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    // SNAP WINDOW TO 'LEFT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC)
                    {  
                        move_client(cli->x - c->width, cli->y);
                        return;
                    }

                    // SNAP WINDOW TO 'LEFT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC)
                    {
                        move_client(cli->x - c->width, (cli->y + cli->height) - c->height);
                        return;
                    }                

                    move_client(cli->x - c->width, y);
                    return;
                }

                // SNAP WINDOW TO 'BOTTOM' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y + cli->height - N && y < cli->y + cli->height + N) 
                    && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    // SNAP WINDOW TO 'BOTTOM_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC)
                    {  
                        move_client(cli->x, cli->y + cli->height);
                        return;
                    }

                    // SNAP WINDOW TO 'BOTTOM_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC)
                    {
                        move_client((cli->x + cli->width) - c->width, cli->y + cli->height);
                        return;
                    }

                    move_client(x ,cli->y + cli->height);
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y + c->height > cli->y - N && y + c->height < cli->y + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    // SNAP WINDOW TO 'TOP_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC)
                    {  
                        move_client(cli->x, cli->y - c->height);
                        return;
                    }

                    // SNAP WINDOW TO 'TOP_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC)
                    {
                        move_client((cli->x + cli->width) - c->width, cli->y - c->height);
                        return;
                    }

                    move_client(x ,cli->y - c->height);
                    return;
                }
            }

            // WINDOW TO EDGE OF SCREEN SNAPPING
            if (((x < N) && (x > -N)) && ((y < N) && (y > -N)))
            {
                move_client(0, 0);
            }
            else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < N && y > -N))
            {
                move_client(RIGHT_, 0);
            }
            else if ((y < BOTTOM + N && y > BOTTOM - N) && (x < N && x > -N))
            {
                move_client(0, BOTTOM);
            }
            else if ((x < N) && (x > -N))
            { 
                move_client(0, y);
            }
            else if (y < N && y > -N)
            {
                move_client(x, 0);
            }
            else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < BOTTOM + N && y > BOTTOM - N))
            {
                move_client(RIGHT_, BOTTOM);
            }
            else if ((x < RIGHT_ + N) && (x > RIGHT_ - N))
            { 
                move_client(RIGHT_, y);
            }
            else if (y < BOTTOM + N && y > BOTTOM - N)
            {
                move_client(x, BOTTOM);
            }
            else 
            {
                move_client(x, y);
            }
        }

        void
        run() /*
            THIS IS THE MAIN EVENT LOOP FOR 'mv_client'
         */ 
        {
            while (shouldContinue) 
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) 
                {
                    continue;
                }

                switch (ev->response_type & ~0x80) 
                {
                    case XCB_MOTION_NOTIFY: 
                    {
                        const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                        const int16_t & new_x = e->root_x - start_x;
                        const int16_t & new_y = e->root_y - start_y;
                        
                        if (isTimeToRender())
                        {
                            snap(new_x, new_y);
                            xcb_flush(conn);
                        }
                        break;
                    }
                    case XCB_BUTTON_RELEASE:
                    {
                        shouldContinue = false;
                        wm::update_client(c);
                        break;
                    }
                }
                free(ev);
            }
        }
        
        const double frameRate = 120.0; /* 
            FRAMERATE 
         */
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now(); /*
            HIGH_PRECISION_CLOCK AND TIME_POINT 
         */
        const double frameDuration = 1000.0 / frameRate; /* 
            DURATION IN MILLISECONDS THAT EACH FRAME SHOULD LAST 
        */

        bool 
        isTimeToRender() 
        {
            auto currentTime = std::chrono::high_resolution_clock::now(); /*
                CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
             */ 
            const std::chrono::duration<double, std::milli> elapsedTime = currentTime - lastUpdateTime;

            if (elapsedTime.count() >= frameDuration) /*
                CHECK IF THE ELAPSED TIME EXCEEDS THE FRAME DURATION
             */ 
            {
                
                lastUpdateTime = currentTime; /*
                    UPDATE THE LAST_UPDATE_TIME TO THE 
                    CURRENT TIME FOR THE NEXT CHECK
                 */ 
                return true; /*
                    RETURN TRUE IF IT'S TIME TO RENDER
                 */ 
            }
            return false; /*
                RETURN FALSE IF NOT ENOUGH TIME HAS PASSED
             */ 
        }
};

namespace XCBAnimator {
    class Move {
        public:
            Move(xcb_connection_t* connection, xcb_window_t window)
                : connection(connection), window(window) {}

            // Public method to start the animation
            void move(int startX, int startY, int endX, int endY, int duration) {
                // Ensure any existing animation is stopped
                stopAnimation();

                // Set initial coordinates
                currentX = startX;
                currentY = startY;

                // Calculate step size based on time
                int steps = duration / animationInterval;
                stepX = (endX - startX) / steps;
                stepY = (endY - startY) / steps;

                // Start a new thread for animation
                animationThread = std::thread(&Move::animateThread, this, endX, endY);

                // Wait for the animation to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(duration));

                // Stop the animation
                stopAnimation();
            }

            // Destructor to ensure the animation thread is stopped when the object is destroyed
            ~Move() {
                stopAnimation();
            }

        private:
            xcb_connection_t* connection;
            xcb_window_t window;
            std::thread animationThread;
            int currentX;
            int currentY;
            int stepX;
            int stepY;
            const int animationInterval = 5; // milliseconds
            std::atomic<bool> stopFlag{false};

            // Static method for the animation thread
            void animateThread(int endX, int endY) 
            {
                while (true) 
                {
                    // Perform animation step
                    moveStep();

                    // Sleep for the animation interval
                    std::this_thread::sleep_for(std::chrono::milliseconds(animationInterval));

                    // Check if animation should stop
                    if (currentX >= endX && currentY >= endY) 
                    {
                        break;
                    }
                }
                move(endX, endY);
            }

            void move(const int & x, const int & y) 
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                    (const uint32_t[2])
                    {
                        static_cast<const uint32_t &>(x), 
                        static_cast<const uint32_t &>(y)
                    }
                );
                xcb_flush(connection);
            }

            void moveStep() 
            {
                currentX += stepX;
                currentY += stepY;

                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                    (const uint32_t[2])
                    {
                        static_cast<const uint32_t &>(currentX), 
                        static_cast<const uint32_t &>(currentY)
                    }
                );
                xcb_flush(connection);
            }

            // Static method to stop the animation
            void stopAnimation() 
            {
                if (animationThread.joinable()) 
                {
                    // Signal the thread to exit
                    stopFlag.store(true);
                    // Wait for the thread to finish
                    animationThread.join();
                    // Reset the stop flag
                    stopFlag.store(false);
                }
            }
    };

}

/**
 *
 * @class XCPPBAnimator
 * @brief Class for animating the position and size of an XCB window.
 *
 */
class XCPPBAnimator
{
    public:
        XCPPBAnimator(xcb_connection_t* connection, const xcb_window_t & window)
        : connection(connection), window(window) {}

        XCPPBAnimator(xcb_connection_t* connection, client * c)
        : connection(connection), c(c) {}

        XCPPBAnimator(xcb_connection_t* connection, const xcb_window_t & window, client * c)
        : connection(connection), window(window), c(c) {}

        void /**
         * @brief Animates the position and size of an object from a starting point to an ending point.
         * 
         * @param startX The starting X coordinate.
         * @param startY The starting Y coordinate.
         * @param startWidth The starting width.
         * @param startHeight The starting height.
         * @param endX The ending X coordinate.
         * @param endY The ending Y coordinate.
         * @param endWidth The ending width.
         * @param endHeight The ending height.
         * @param duration The duration of the animation in milliseconds.
         */
        animate(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration) 
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX      = startX;
            currentY      = startY;
            currentWidth  = startWidth;
            currentHeight = startHeight;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX      = std::abs(endX - startX)           / (endX - startX);
            stepY      = std::abs(endY - startY)           / (endY - startY);
            stepWidth  = std::abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = std::abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - startHeight)); 

            /* START ANIMATION THREADS */
            XAnimationThread = std::thread(&XCPPBAnimator::XAnimation, this, endX);
            YAnimationThread = std::thread(&XCPPBAnimator::YAnimation, this, endY);
            WAnimationThread = std::thread(&XCPPBAnimator::WAnimation, this, endWidth);
            HAnimationThread = std::thread(&XCPPBAnimator::HAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }

        void 
        animate_client(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration)
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX      = startX;
            currentY      = startY;
            currentWidth  = startWidth;
            currentHeight = startHeight;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX      = std::abs(endX - startX)           / (endX - startX);
            stepY      = std::abs(endY - startY)           / (endY - startY);
            stepWidth  = std::abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = std::abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - startHeight)); 
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = std::thread(&XCPPBAnimator::GFrameAnimation, this, endX, endY, endWidth, endHeight);
            XAnimationThread = std::thread(&XCPPBAnimator::CliXAnimation, this, endX);
            YAnimationThread = std::thread(&XCPPBAnimator::CliYAnimation, this, endY);
            WAnimationThread = std::thread(&XCPPBAnimator::CliWAnimation, this, endWidth);
            HAnimationThread = std::thread(&XCPPBAnimator::CliHAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
        
        /**
         * @brief Destructor to ensure the animation threads are stopped when the object is destroyed.
         */
        ~XCPPBAnimator()
        {
            stopAnimations();
        }

    private:
        xcb_connection_t* connection;
        xcb_window_t window;
        client * c;
        std::thread GAnimationThread;
        std::thread XAnimationThread;
        std::thread YAnimationThread;
        std::thread WAnimationThread;
        std::thread HAnimationThread;
        int currentX;
        int currentY;
        int currentWidth;
        int currentHeight;
        int stepX;
        int stepY;
        int stepWidth;
        int stepHeight;
        double GAnimDuration;
        double XAnimDuration;
        double YAnimDuration;
        double WAnimDuration;
        double HAnimDuration;
        std::atomic<bool> stopGFlag{false};
        std::atomic<bool> stopXFlag{false};
        std::atomic<bool> stopYFlag{false};
        std::atomic<bool> stopWFlag{false};
        std::atomic<bool> stopHFlag{false};
        std::chrono::high_resolution_clock::time_point XlastUpdateTime;
        std::chrono::high_resolution_clock::time_point YlastUpdateTime;
        std::chrono::high_resolution_clock::time_point WlastUpdateTime;
        std::chrono::high_resolution_clock::time_point HlastUpdateTime;
        
        /* FRAMERATE */
        const double frameRate = 400;
        
        /* DURATION IN MILLISECONDS THAT EACH FRAME SHOULD LAST */
        const double frameDuration = 1000.0 / frameRate; 
        
        void /**
         *
         * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
         * 
         * This function updates the 'x' of a window in a loop until the current 'x'
         * matches the specified end 'x'. It uses the "XStep()" function to incrementally
         * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endX The desired 'x' position of the window.
         *
         */
        XAnimation(const int & endX) 
        {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentX == endX) 
                {
                    config_window(XCB_CONFIG_WINDOW_X, endX);
                    break;
                }
                XStep();
                thread_sleep(XAnimDuration);
            }
        }

        void /**
         * @brief Performs a step in the X direction.
         * 
         * This function increments the currentX variable by the stepX value.
         * If it is time to render, it configures the window's X position using the currentX value.
         * 
         * @note This function assumes that the connection and window variables are properly initialized.
         */
        XStep() 
        {
            currentX += stepX;
            
            if (XisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_X,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentX)
                    }
                );
                xcb_flush(connection);
            }
        }

        void /**
         *
         * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
         * 
         * This function updates the 'y' of a window in a loop until the current 'y'
         * matches the specified end 'y'. It uses the "YStep()" function to incrementally
         * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endY The desired 'y' positon of the window.
         *
         */
        YAnimation(const int & endY) 
        {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentY == endY) 
                {
                    config_window(XCB_CONFIG_WINDOW_Y, endY);
                    break;
                }
                YStep();
                thread_sleep(YAnimDuration);
            }
        }

        void /**
         * @brief Performs a step in the Y direction.
         * 
         * This function increments the currentY variable by the stepY value.
         * If it is time to render, it configures the window's Y position using xcb_configure_window
         * and flushes the connection using xcb_flush.
         */
        YStep() 
        {
            currentY += stepY;
            
            if (YisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_Y,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentY)
                    }
                );
                xcb_flush(connection);
            }
        }

        void /**
         *
         * @brief Performs a 'width' animation until the specified end 'width' is reached.
         * 
         * This function updates the 'width' of a window in a loop until the current 'width'
         * matches the specified end 'width'. It uses the "WStep()" function to incrementally
         * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'width' of the window.
         *
         */
        WAnimation(const int & endWidth) 
        {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentWidth == endWidth) 
                {
                    config_window(XCB_CONFIG_WINDOW_WIDTH, endWidth);
                    break;
                }
                WStep();
                thread_sleep(WAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs a step in the width calculation and updates the window width if it is time to render.
         * 
         * This function increments the current width by the step width. If it is time to render, it configures the window width
         * using the XCB library and flushes the connection.
         *
         */
        WStep() 
        {
            currentWidth += stepWidth;

            if (WisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_WIDTH,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentWidth) 
                    }
                );
                xcb_flush(connection);
            }
        }

        void /**
         *
         * @brief Performs a 'height' animation until the specified end 'height' is reached.
         * 
         * This function updates the 'height' of a window in a loop until the current 'height'
         * matches the specified end 'height'. It uses the "HStep()" function to incrementally
         * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'height' of the window.
         *
         */
        HAnimation(const int & endHeight) 
        {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentHeight == endHeight) 
                {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, endHeight);
                    break;
                }
                HStep();
                thread_sleep(HAnimDuration);
            }
        }

        void /**
         *
         * @brief Increases the current height by the step height and updates the window height if it's time to render.
         * 
         * This function is responsible for incrementing the current height by the step height and updating the window height
         * if it's time to render. It uses the xcb_configure_window function to configure the window height and xcb_flush to
         * flush the changes to the X server.
         *
         */
        HStep() 
        {
            currentHeight += stepHeight;
            
            if (HisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_HEIGHT,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentHeight)
                    }
                );
                xcb_flush(connection);
            }
        }

        void
        GFrameAnimation(const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            while (true)
            {
                if (currentX == endX && currentY == endY && currentWidth == endWidth && currentHeight == endHeight)
                {
                    conf_client_test();
                    break;
                }
                conf_client_test();
                thread_sleep(GAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
         * 
         * This function updates the 'x' of a window in a loop until the current 'x'
         * matches the specified end 'x'. It uses the "XStep()" function to incrementally
         * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endX The desired 'x' position of the window.
         *
         */
        CliXAnimation(const int & endX) 
        {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentX == endX) 
                {
                    break;
                }
                currentX += stepX;
                thread_sleep(XAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
         * 
         * This function updates the 'y' of a window in a loop until the current 'y'
         * matches the specified end 'y'. It uses the "YStep()" function to incrementally
         * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endY The desired 'y' positon of the window.
         *
         */
        CliYAnimation(const int & endY) 
        {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentY == endY) 
                {
                    break;
                }
                currentY += stepY;
                thread_sleep(YAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs a 'width' animation until the specified end 'width' is reached.
         * 
         * This function updates the 'width' of a window in a loop until the current 'width'
         * matches the specified end 'width'. It uses the "WStep()" function to incrementally
         * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'width' of the window.
         *
         */
        CliWAnimation(const int & endWidth) 
        {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentWidth == endWidth) 
                {
                    break;
                }
                currentWidth += stepWidth;
                thread_sleep(WAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs a 'height' animation until the specified end 'height' is reached.
         * 
         * This function updates the 'height' of a window in a loop until the current 'height'
         * matches the specified end 'height'. It uses the "HStep()" function to incrementally
         * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'height' of the window.
         *
         */
        CliHAnimation(const int & endHeight) 
        {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentHeight == endHeight) 
                {
                    break;
                }
                currentHeight += stepHeight;
                thread_sleep(HAnimDuration);
            }
        }

        void /**
         *
         * @brief Stops all animations by setting the corresponding flags to true and joining the animation threads.
         *        After joining the threads, the flags are set back to false.
         *
         */
        stopAnimations() 
        {
            stopHFlag.store(true);
            stopXFlag.store(true);
            stopYFlag.store(true);
            stopWFlag.store(true);
            stopHFlag.store(true);

            if (GAnimationThread.joinable()) 
            {
                GAnimationThread.join();
                stopGFlag.store(false);
            }

            if (XAnimationThread.joinable()) 
            {
                XAnimationThread.join();
                stopXFlag.store(false);
            }

            if (YAnimationThread.joinable()) 
            {
                YAnimationThread.join();
                stopYFlag.store(false);
            }

            if (WAnimationThread.joinable()) 
            {
                WAnimationThread.join();
                stopWFlag.store(false);
            }

            if (HAnimationThread.joinable()) 
            {
                HAnimationThread.join();
                stopHFlag.store(false);
            }
        }

        void /**
         *
         * @brief Sleeps the current thread for the specified number of milliseconds.
         *
         * @param milliseconds The number of milliseconds to sleep. A double is used to allow for
         *                     fractional milliseconds, providing finer control over animation timing.
         *
         * @note This is needed as the time for each thread to sleep is the main thing to be calculated, 
         *       as this class is designed to iterate every pixel and then only update that to the X-server at the given framerate.
         *
         */
        thread_sleep(const double & milliseconds) 
        {
            // Creating a duration with double milliseconds
            auto duration = std::chrono::duration<double, std::milli>(milliseconds);

            // Sleeping for the duration
            std::this_thread::sleep_for(duration);
        }

        bool /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        XisTimeToRender() 
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - XlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                XlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }

        bool /**
         *
         * Checks if it is time to render a frame based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        YisTimeToRender() 
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - YlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                YlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        
        bool /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        WisTimeToRender()
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - WlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                WlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }

        bool /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        HisTimeToRender()
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - HlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                HlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }

        void /**
         *
         * @brief Configures the window with the specified mask and value.
         * 
         * This function configures the window using the XCB library. It takes in a mask and a value
         * as parameters and applies the configuration to the window.
         * 
         * @param mask The mask specifying which attributes to configure.
         * @param value The value to set for the specified attributes.
         * 
         */
        config_window(const uint32_t & mask, const uint32_t & value)
        {
            xcb_configure_window
            (
                connection,
                window,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(connection);
        }

        void /**
         *
         * @brief Configures the window with the specified mask and value.
         * 
         * This function configures the window using the XCB library. It takes in a mask and a value
         * as parameters and applies the configuration to the window.
         * 
         * @param mask The mask specifying which attributes to configure.
         * @param value The value to set for the specified attributes.
         * 
         */
        config_window(const xcb_window_t & win, const uint32_t & mask, const uint32_t & value)
        {
            xcb_configure_window
            (
                connection,
                win,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(connection);
        }

        void /**
         *
         * @brief Configures the window with the specified mask and value.
         * 
         * This function configures the window using the XCB library. It takes in a mask and a value
         * as parameters and applies the configuration to the window.
         * 
         * @param mask The mask specifying which attributes to configure.
         * @param value The value to set for the specified attributes.
         * 
         */
        config_client(const uint32_t & mask, const uint32_t & value)
        {
            xcb_configure_window
            (
                connection,
                c->win,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );

            xcb_configure_window
            (
                connection,
                c->frame,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(connection);
        }

        void 
        conf_client_test()
        {
            const uint32_t x = currentX, y = currentY, w = currentWidth, h = currentHeight;

            xcb_configure_window
            (
                connection,
                c->win,
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(w),
                    static_cast<const uint32_t &>(h - 20)
                }
            );
            xcb_flush(connection);

            xcb_configure_window
            (
                connection,
                c->frame,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[4])
                {
                    static_cast<const uint32_t &>(x),
                    static_cast<const uint32_t &>(y),
                    static_cast<const uint32_t &>(w),
                    static_cast<const uint32_t &>(h)
                }
            );
            xcb_flush(connection);

            xcb_configure_window
            (
                connection,
                c->titlebar,
                XCB_CONFIG_WINDOW_WIDTH,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w)
                }
            );
            xcb_flush(connection);

            xcb_configure_window
            (
                connection,
                c->close_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - 20)
                }
            );
            xcb_flush(connection);

            xcb_configure_window
            (
                connection,
                c->max_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - 40)
                }
            );
            xcb_flush(connection);

            xcb_configure_window
            (
                connection,
                c->min_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - 60)
                }
            );
            xcb_flush(connection);
        }
};

void
animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
    XCPPBAnimator anim(conn, c->frame);
    anim.animate
    (
        c->x,
        c->y, 
        c->width, 
        c->height, 
        endX,
        endY, 
        endWidth, 
        endHeight, 
        duration
    );
    wm::update_client(c);
}

void
animate_client(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
    XCPPBAnimator client_anim(conn, c);
    client_anim.animate_client
    (
        c->x,
        c->y,
        c->width,
        c->height,
        endX,
        endY,
        endWidth,
        endHeight,
        duration
    );
    wm::update_client(c);
}

class color 
{
    public:
        static uint32_t
        get(rgb_color_code color)
        {
            xcb_colormap_t colormap = screen->default_colormap;
            xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply
            (
                conn, 
                xcb_alloc_color
                (
                    conn,
                    colormap,
                    scale::from_8_to_16_bit(color.r), 
                    scale::from_8_to_16_bit(color.g),
                    scale::from_8_to_16_bit(color.b)
                ), 
                NULL
            );
            return reply->pixel;
        }

        static uint32_t
        get(COLOR color)
        {
            xcb_colormap_t colormap = screen->default_colormap;
            rgb_color_code color_code = code(color);
            xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply
            (
                conn, 
                xcb_alloc_color
                (
                    conn,
                    colormap,
                    scale::from_8_to_16_bit(color_code.r), 
                    scale::from_8_to_16_bit(color_code.g),
                    scale::from_8_to_16_bit(color_code.b)
                ), 
                NULL
            );
            return reply->pixel;
        }

        static rgb_color_code
        code(const uint8_t & r, const uint8_t & g, const uint8_t & b)
        {
            rgb_color_code color;
            color.r = r;
            color.g = g;
            color.b = b;
            return color;
        }

        static rgb_color_code
        code(COLOR rgb_code)
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            
            switch (rgb_code) 
            {
                case COLOR::WHITE:
                {
                    r = 255;
                    g = 255;
                    b = 255;
                    break;
                }
                case COLOR::BLACK:
                {
                    r = 0;
                    g = 0;
                    b = 0;
                    break;
                }
                case COLOR::RED:
                {
                    r = 255;
                    g = 0;
                    b = 0;
                    break;
                }
                case COLOR::GREEN:
                {
                    r = 0;
                    g = 255;
                    b = 0;
                    break;
                }
                case COLOR::BLUE:
                {
                    r = 0;
                    g = 0;
                    b = 255;
                    break;
                }
                case COLOR::YELLOW:
                {
                    r = 255;
                    g = 255;
                    b = 0;
                    break;
                }
                case COLOR::CYAN:
                {
                    r = 0;
                    g = 255;
                    b = 255;
                    break;
                }
                case COLOR::MAGENTA:
                {
                    r = 255;
                    g = 0;
                    b = 255;
                    break;
                }
                case COLOR::GREY:
                {
                    r = 128;
                    g = 128;
                    b = 128;
                    break;
                }
                case COLOR::LIGHT_GREY:
                {
                    r = 192;
                    g = 192;
                    b = 192;
                    break;
                }
                case COLOR::DARK_GREY:
                {
                    r = 64;
                    g = 64;
                    b = 64;
                    break;
                }
                case COLOR::ORANGE:
                {
                    r = 255;
                    g = 165;
                    b = 0;
                    break;
                }
                case COLOR::PURPLE:
                {
                    r = 128;
                    g = 0;
                    b = 128;
                    break;
                }
                case COLOR::BROWN:
                {
                    r = 165;
                    g = 42;
                    b = 42;
                    break;
                }
                case COLOR::PINK:
                {
                    r = 255;
                    g = 192;
                    b = 203;
                    break;
                }
                default:
                {
                    r = 0;
                    g = 0;
                    b = 0;
                    break;
                }
            }

            rgb_color_code color;
            color.r = r;
            color.g = g;
            color.b = b;
            return color;
        }
};

class set_win_color 
{
    public:
        set_win_color(SET_COLOR mode, const xcb_window_t & win, const uint16_t & r, const uint16_t & g, const uint16_t & b)
        {
            xcb_colormap_t colormap = make_colormap(win);
            apply_color(colormap, win, r, g, b);
        }

        set_win_color(const xcb_window_t & win, const uint8_t & r, const uint8_t & g, const uint8_t & b)
        {
            xcb_colormap_t colormap = make_colormap(win);
            apply_color
            (
                colormap, 
                win, 
                scale::from_8_to_16_bit(r), 
                scale::from_8_to_16_bit(g), 
                scale::from_8_to_16_bit(b)
            );
        }

        set_win_color(const xcb_window_t & win, rgb_color_code COLOR)
        {
            xcb_colormap_t colormap = make_colormap(win);
            apply_color
            (
                colormap, 
                win, 
                COLOR
            );
        }

        set_win_color(const xcb_window_t & win, COLOR COLOR)
        {
            xcb_colormap_t colormap = make_colormap(win);
            apply_color
            (
                colormap, 
                win, 
                color::code(COLOR)
            );
        }

    private:
        xcb_alloc_color_reply_t *reply;
        
        void
        apply_color(const xcb_colormap_t & colormap, const xcb_window_t & win, const uint16_t & r, const uint16_t & g, const uint16_t & b)
        {
            rgb_color_code COLOR = color::code(r, g, b);
            change_win_color(win, COLOR);
            xcb_flush(conn);
            free(reply);
        }

        void
        apply_color(const xcb_colormap_t & colormap, const xcb_window_t & win, rgb_color_code COLOR)
        {
            change_win_color(win, COLOR);
            xcb_flush(conn);
            free(reply);
        }

        xcb_colormap_t 
        make_colormap(const xcb_window_t & win)
        {
            xcb_colormap_t colormap = xcb_generate_id(conn);
            xcb_create_colormap
            (
                conn, 
                XCB_COLORMAP_ALLOC_NONE, 
                colormap, 
                win, 
                screen->root_visual
            );
            return colormap;
        }

        void
        change_win_color(const xcb_window_t & win, rgb_color_code COLOR)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_BACK_PIXEL,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(color::get(COLOR))
                }
            );
            xcb_flush(conn);
        }
};

class set_png_as_backround 
{
    public:
        set_png_as_backround(const std::string & pngFilePath) : pngFilePath(pngFilePath) {}
        // ~set_png_as_backround();
        
        void 
        setAsBackground(xcb_connection_t * connection, xcb_window_t win)
        {
            apply_background(connection, win, screen, pngFilePath);
        }

    private:
        std::string pngFilePath;

        struct ImageData 
        {
            std::vector<unsigned char> data;
            int width;
            int height;
        };

                
        ImageData 
        loadPNG(const std::string& filePath) 
        {
            std::ifstream fileStream(filePath, std::ios::binary);
            if (!fileStream) 
            {
                throw std::runtime_error("File " + filePath + " could not be opened for reading");
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) 
            {
                throw std::runtime_error("Failed to create PNG read structure");
            }

            png_infop info = png_create_info_struct(png);
            if (!info) 
            {
                png_destroy_read_struct(&png, nullptr, nullptr);
                throw std::runtime_error("Failed to create PNG info structure");
            }

            if (setjmp(png_jmpbuf(png))) 
            {
                png_destroy_read_struct(&png, &info, nullptr);
                throw std::runtime_error("Error during PNG read");
            }

            // Using custom read function to handle C++ streams
            png_set_read_fn(png, static_cast<png_voidp>(&fileStream), [](png_structp pngPtr, png_bytep data, png_size_t length) 
            {
                std::ifstream* stream = static_cast<std::ifstream*>(png_get_io_ptr(pngPtr));
                stream->read(reinterpret_cast<char*>(data), length);
            });

            png_read_info(png, info);

            int width = png_get_image_width(png, info);
            int height = png_get_image_height(png, info);
            png_byte color_type = png_get_color_type(png, info);
            png_byte bit_depth = png_get_bit_depth(png, info);
            
            if(bit_depth == 16) 
            {
                png_set_strip_16(png);
            }

            if(color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_palette_to_rgb(png);
            }

            if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
            {
                png_set_expand_gray_1_2_4_to_8(png);
            }

            if(png_get_valid(png, info, PNG_INFO_tRNS)) 
            {
                png_set_tRNS_to_alpha(png);
            }
                        
            if (color_type == PNG_COLOR_TYPE_RGB 
             || color_type == PNG_COLOR_TYPE_GRAY 
             || color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            }

            if (color_type == PNG_COLOR_TYPE_GRAY 
             || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
            {
                png_set_gray_to_rgb(png);
            }

            png_read_update_info(png, info);

            std::vector<unsigned char> imageData;
            imageData.reserve(width * height * 4); // 4 bytes per pixel (RGBA)

            std::unique_ptr<png_bytep[]> row_pointers(new png_bytep[height]);
            for(int y = 0; y < height; y++) {
                row_pointers[y] = new png_byte[png_get_rowbytes(png, info)];
            }

            png_read_image(png, row_pointers.get());

            for(int y = 0; y < height; y++) 
            {
                png_bytep row = row_pointers[y];
                for(int x = 0; x < width; x++) 
                {
                    png_bytep px = &(row[x * 4]);
                    imageData.push_back(px[0]); // Red
                    imageData.push_back(px[1]); // Green
                    imageData.push_back(px[2]); // Blue
                    imageData.push_back(px[3]); // Alpha
                }
                delete[] row_pointers[y];
            }

            png_destroy_read_struct(&png, &info, nullptr);

            return {imageData, width, height};
        }

        ImageData 
        loadPNG_non_cpp_iso(const std::string& filePath) 
        {
            FILE *fp = fopen(filePath.c_str(), "rb");
            if (!fp) 
            {
                throw std::runtime_error("File " + filePath + " could not be opened for reading");
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) 
            {
                fclose(fp);
                throw std::runtime_error("Failed to create PNG read structure");
            }

            png_infop info = png_create_info_struct(png);
            if (!info) 
            {
                fclose(fp);
                png_destroy_read_struct(&png, nullptr, nullptr);
                throw std::runtime_error("Failed to create PNG info structure");
            }

            if (setjmp(png_jmpbuf(png))) 
            {
                fclose(fp);
                png_destroy_read_struct(&png, &info, nullptr);
                throw std::runtime_error("Error during PNG read");
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            int width = png_get_image_width(png, info);
            int height = png_get_image_height(png, info);
            png_byte color_type = png_get_color_type(png, info);
            png_byte bit_depth = png_get_bit_depth(png, info);

            if(bit_depth == 16) 
            {
                png_set_strip_16(png);
            }

            if(color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_palette_to_rgb(png);
            }

            if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
            {
                png_set_expand_gray_1_2_4_to_8(png);
            }

            if(png_get_valid(png, info, PNG_INFO_tRNS)) 
            {
                png_set_tRNS_to_alpha(png);
            }
                        
            if (color_type == PNG_COLOR_TYPE_RGB 
            || color_type == PNG_COLOR_TYPE_GRAY 
            || color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            }

            if (color_type == PNG_COLOR_TYPE_GRAY 
            || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
            {
                png_set_gray_to_rgb(png);
            }

            png_read_update_info(png, info);

            std::vector<unsigned char> imageData;
            imageData.reserve(width * height * 4); // 4 bytes per pixel (RGBA)

            png_bytep row_pointers[height];
            for(int y = 0; y < height; y++) 
            {
                row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
            }

            png_read_image(png, row_pointers);

            fclose(fp);

            for(int y = 0; y < height; y++) 
            {
                png_bytep row = row_pointers[y];
                for(int x = 0; x < width; x++) 
                {
                    png_bytep px = &(row[x * 4]);
                    imageData.push_back(px[0]); // Red
                    imageData.push_back(px[1]); // Green
                    imageData.push_back(px[2]); // Blue
                    imageData.push_back(px[3]); // Alpha
                }
                free(row_pointers[y]);
            }

            png_destroy_read_struct(&png, &info, nullptr);

            return {imageData, width, height};
        }

        xcb_image_t* 
        create_xcb_image(const ImageData& imgData, xcb_connection_t *connection, xcb_screen_t *screen) 
        {
            xcb_image_t *image = xcb_image_create
            (
                imgData.width, imgData.height, 
                XCB_IMAGE_FORMAT_Z_PIXMAP, 
                screen->root_depth, 
                0, 
                32, 
                0, 
                XCB_IMAGE_ORDER_LSB_FIRST, 
                XCB_IMAGE_ORDER_LSB_FIRST, 
                nullptr, 
                ~0, 
                nullptr
            );

            if (!image) 
            {
                return nullptr;
            }

            for (int y = 0; y < imgData.height; ++y) 
            {
                for (int x = 0; x < imgData.width; ++x) 
                {
                    int idx = (y * imgData.width + x) * 4;
                    uint32_t pixel = (imgData.data[idx + 3] << 24) |
                                     (imgData.data[idx + 2] << 16) | 
                                     (imgData.data[idx + 1] << 8)  | 
                                     (imgData.data[idx]);
                    xcb_image_put_pixel(image, x, y, pixel);
                }
            }

            return image;
        }

        void 
        apply_background(xcb_connection_t *connection, xcb_window_t window, xcb_screen_t *screen, const std::string& image_path) 
        {
            ImageData imgData = loadPNG_non_cpp_iso(image_path);
            xcb_image_t *image = create_xcb_image(imgData, connection, screen);
            if (!image) 
            {
                return;
            }

            xcb_pixmap_t pixmap = xcb_generate_id(connection);
            xcb_create_pixmap(connection, screen->root_depth, pixmap, window, imgData.width, imgData.height);
            xcb_gcontext_t gc = xcb_generate_id(connection);
            uint32_t values[2] = {0, 0};
            xcb_create_gc(connection, gc, pixmap, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, values);

            xcb_image_put(connection, pixmap, gc, image, 0, 0, 0);
            xcb_change_window_attributes(connection, window, XCB_CW_BACK_PIXMAP, &pixmap);
            xcb_clear_area(connection, 0, window, 0, 0, imgData.width, imgData.height);
            xcb_flush(connection);

            xcb_free_pixmap(connection, pixmap);
            xcb_free_gc(connection, gc);
            xcb_image_destroy(image);
        }
};

void 
move_desktop(const uint8_t & n)
{
    if (cur_d == desktop_list[n - 1] || n == 0 || n == desktop_list.size())
    {
        return;
    }
    
    for (const auto & c : cur_d->current_clients)
    {
        if (c)
        {
            if (c->desktop == cur_d->desktop)
            {
                show_hide_client(c, HIDE);
            }
        }
    }

    cur_d = desktop_list[n - 1];
    for (const auto & c : cur_d->current_clients)
    {
        if (c)
        {
            show_hide_client(c, SHOW);           
        }
    }
}

void 
next_hide(client * c) 
{
	XCBAnimator::Move anim(conn, c->win);
    anim.move(c->x, c->y, c->x - screen->width_in_pixels, c->y, 1000);
	wm::update_client(c);
	show_hide_client(c, HIDE);
}

void 
next_show(client * c) 
{
	show_hide_client(c, SHOW);
	XCBAnimator::Move anim(conn, c->win);
    anim.move(c->x, c->y, c->x - screen->width_in_pixels, c->y, 1000);
	wm::update_client(c);
    focus::client(c);
}

void
Next_Desktop() 
{
	if (cur_d->desktop == desktop_list.size()) 
	{
		return;
	}

	// HIDE CLIENTS ON CURRENT_DESKTOP
	for (const auto & c : cur_d->current_clients) 
	{
		if (c) 
		{
            show_hide_client(c, HIDE);
		}
	}

	cur_d = desktop_list[cur_d->desktop];

	// SHOW CLIENTS ON NEXT_DESKTOP
	for (const auto & c : cur_d->current_clients)
	{
		if (c) 
		{
			show_hide_client(c, SHOW);
		}
	}
}

void
prev_hide(client * c)
{
    XCBAnimator::Move anim(conn, c->win);
    anim.move(c->x, c->y, c->x + screen->width_in_pixels, c->y, 1000);
    wm::update_client(c);
    show_hide_client(c, HIDE);
}

void 
prev_show(client * c)
{
    show_hide_client(c, SHOW);
    XCBAnimator::Move anim(conn, c->win);
    anim.move(c->x, c->y, c->x + screen->width_in_pixels, c->y, 1000);
    wm::update_client(c);
    focus::client(c);
}

void
Prev_Desktop()
{
	LOG_func
	if (cur_d->desktop == 1)
	{
		return;
	}

	// HIDE CLIENTS ON CURRENT_DESKTOP
	for (const auto & c : cur_d->current_clients)
	{
		if (c)
		{
            show_hide_client(c, HIDE);
		}
	}

	cur_d = desktop_list[cur_d->desktop - 2];
	// SHOW CLIENTS ON NEXT_DESKTOP
	for (const auto & c : cur_d->current_clients)
	{
		if (c)
		{
            show_hide_client(c, SHOW);
		}
	}
}

void
move_to_next_desktop_w_app()
{
    LOG_func
    if (cur_d->desktop == desktop_list.size())
    {
        return;
    }

    if (focused_client)
    {
        focused_client->desktop = cur_d->desktop + 1;
    }

    move_desktop(cur_d->desktop + 1);
}

void
move_to_previus_desktop_w_app()
{
    LOG_func
    if (cur_d->desktop == 1)
    {
        return;
    }

    if (focused_client)
    {
        focused_client->desktop = cur_d->desktop - 1;
    }

    move_desktop(cur_d->desktop - 1);
    wm::raise_client(focused_client);
}

class resize_client 
{
    public:
        /* 
            THE REASON FOR THE 'retard_int' IS BECUSE WITHOUT IT 
            I CANNOT CALL THIS CLASS LIKE THIS 'resize_client(c)' 
            INSTEAD I WOULD HAVE TO CALL IT LIKE THIS 'resize_client rc(c)'
            AND NOW WITH THE 'retard_int' I CAN CALL IT LIKE THIS 'resize_client(c, 0)'
         */
        resize_client(client * & c , int retard_int) 
        : c(c) 
        {
            grab_pointer();
            
            teleport_mouse(c->x + c->width, c->y + c->height); /* 
                MOVE THE MOUSE TO THE BOTTOM-RIGHT CORNER OF THE WINDOW
             */
            run();
            
            // Release the pointer grab and flush the connection
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }

    private:
        client * & c;

        void
        grab_pointer()
        {
            // Grab the pointer to track mouse events for window resizing
            xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer
            (
                conn,
                false, // owner_events: false to not propagate events to other clients
                c->win,
                XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC,
                XCB_NONE,
                XCB_NONE,
                XCB_CURRENT_TIME
            );

            // CHECK IF THE POINTER GRAB WAS SUCCESSFULL
            xcb_grab_pointer_reply_t* reply = xcb_grab_pointer_reply(conn, cookie, NULL);
            if (!reply || reply->status != XCB_GRAB_STATUS_SUCCESS) 
            {
                LOG_error("Could not grab pointer");
                free(reply);
                return;
            }
            free(reply);
        }

        void 
        teleport_mouse(const uint16_t & x, const uint16_t & y) 
        {
            xcb_warp_pointer
            (
                conn, 
                XCB_NONE, 
                screen->root, 
                0, 
                0, 
                0, 
                0, 
                x, 
                y
            );
            xcb_flush(conn);
        }

        void
        resize_win_old(const uint16_t & width, const uint16_t & height)
        {
            // CONFIGURE THE WINDOW WITH THE NEW WIDTH AND HEIGHT
            xcb_configure_window 
            (
                conn,
                c->win,
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width), 
                    static_cast<const uint32_t &>(height)
                }
            );
        }

        void
        resize_win(const uint16_t & width, const uint16_t & height)
        {
            // CONFIGURE THE WINDOW WITH THE NEW WIDTH AND HEIGHT
            xcb_configure_window 
            (
                conn,
                c->win,
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width), 
                    static_cast<const uint32_t &>(height)
                }
            );

            xcb_configure_window 
            (
                conn,
                c->frame,
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width), 
                    static_cast<const uint32_t &>(height)
                }
            );

            xcb_configure_window 
            (
                conn,
                c->titlebar,
                XCB_CONFIG_WINDOW_WIDTH,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width)
                }
            );

            xcb_configure_window 
            (
                conn,
                c->close_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width - 20)
                }
            );

            xcb_configure_window 
            (
                conn,
                c->max_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width - 40)
                }
            );

            xcb_configure_window 
            (
                conn,
                c->min_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(width - 60)
                }
            );
        }

        void
        snap(const uint16_t & x, const uint16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : cur_d->current_clients)
            {
                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x - N && x < cli->x + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    resize_win(cli->x - c->x, y - c->y);
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y - N && y < cli->y + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    resize_win(x - c->x ,cli->y - c->y);
                    return;
                }
            }
            resize_win(x - c->x, y - c->y);
        }

        void /* 
            THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
         */
        run()
        {
            xcb_generic_event_t * ev;
            bool shouldContinue = true;

            // Wait for motion events and handle window resizing
            while (shouldContinue) 
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) 
                {
                    continue;
                }

                switch (ev->response_type & ~0x80) 
                {
                    case XCB_MOTION_NOTIFY: 
                    {
                        const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                        if (isTimeToRender())
                        {
                            snap(e->root_x, e->root_y);
                            xcb_flush(conn); 
                        }
                        break;
                    }
                    case XCB_BUTTON_RELEASE: 
                    {
                        shouldContinue = false;                        
                        wm::update_client(c); 
                        break;
                    }
                }
                // Free the event memory after processing
                free(ev); 
            }
        }

        /* FRAMERATE */
        const double frameRate = 120.0;

        /* HIGH_PRECISION_CLOCK AND TIME_POINT */
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
        
        /* DURATION IN MILLISECONDS THAT EACH FRAME SHOULD LAST */
        const double frameDuration = 1000.0 / frameRate; 
        
        bool 
        isTimeToRender() 
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - lastUpdateTime;

            /*
                CHECK IF THE ELAPSED TIME EXCEEDS THE FRAME DURATION
             */ 
            if (elapsedTime.count() >= frameDuration) 
            {
                // UPDATE THE LAST_UPDATE_TIME TO THE 
                // CURRENT TIME FOR THE NEXT CHECK
                lastUpdateTime = currentTime; 
                
                // RETURN TRUE IF IT'S TIME TO RENDER
                return true; 
            }
            // RETURN FALSE IF NOT ENOUGH TIME HAS PASSED
            return false; 
        }
};

namespace borrowed 
{
    bool
    getgeom(const xcb_drawable_t * win, int16_t * x, int16_t * y, uint16_t * width, uint16_t * height, uint8_t * depth)
    {
        xcb_get_geometry_reply_t * geom = xcb_get_geometry_reply
        (
            conn,
            xcb_get_geometry
            (
                conn, 
                * win
            ), 
            NULL
        );

        if (NULL == geom)
        {
            return false;
        }

        * x      = geom->x;
        * y      = geom->y;
        * width  = geom->width;
        * height = geom->height;
        * depth  = geom->depth;

        free(geom);
        return true;
    }
    
    void
    moveresize(xcb_drawable_t win, const uint16_t & x, const uint16_t & y, const uint16_t & width, const uint16_t & height)
    {
        if (win == screen->root || win == 0)
        {
            return;
        }
            
        xcb_configure_window
        (
            conn,
            win,
            XCB_CONFIG_WINDOW_WIDTH | 
            XCB_CONFIG_WINDOW_HEIGHT,
            (const uint32_t[2])
            {
                static_cast<const uint32_t &>(width),
                static_cast<const uint32_t &>(height)
            }
        );

        xcb_configure_window
        (
            conn,
            win,
            XCB_CONFIG_WINDOW_X |
            XCB_CONFIG_WINDOW_Y,
            (const uint32_t[2])
            {
                static_cast<const uint32_t &>(x),
                static_cast<const uint32_t &>(y)
            }
        );

        xcb_flush(conn);
    }

    void
    maxwin_animate(client * c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
    {
        if (!c)
        {
            return;
        }
        
        animate_client
        (
            c, 
            endX, 
            endY, 
            endWidth, 
            endHeight, 
            MAXWIN_ANIMATION_DURATION
        );
    }
    
    void
    unmax(client * c)
    {
        if (c == nullptr)
        {
            return;
        }
    
        maxwin_animate
        (
            c, 
            c->ogsize.x, 
            c->ogsize.y, 
            c->ogsize.width, 
            c->ogsize.height
        );
        c->ismax = false;
    }

    void
    unmaxwin(client * c)
    {
        unmax(c);
        xcb_change_property
        (
            conn,
            XCB_PROP_MODE_REPLACE,
            c->win,
            ewmh->_NET_WM_STATE, 
            XCB_ATOM_ATOM,
            32,
            0,
            0
        );
    }

    void 
    maxwin(client * c, const uint8_t & with_offsets) 
    {
        if (c == nullptr)
        {
            LOG_error("client is nullptr");
            return;
        }

        if (c->ismax)
        {
            unmaxwin(c);
            return;
        }

        const uint16_t & mon_x      = 0; 
        const uint16_t & mon_y      = 0;
        const uint16_t & mon_width  = screen->width_in_pixels;
        const uint16_t & mon_height = screen->height_in_pixels;
        wm::save_ogsize(c);

        maxwin_animate
        (
            c, 
            mon_x, 
            mon_y, 
            mon_width, 
            mon_height
        );

        if (!with_offsets) 
        {
            xcb_change_property
            (
                conn,
                XCB_PROP_MODE_REPLACE,
                c->win,
                ewmh->_NET_WM_STATE,
                XCB_ATOM_ATOM,
                32,
                1,
                &ewmh->_NET_WM_STATE_FULLSCREEN
            );
        }
        c->ismax = true;
        xcb_flush(conn);
        focus::client(c);
    }
}

namespace error
{
    int
    conn_error(xcb_connection_t * conn, const char * sender_function)
    {
        if (xcb_connection_has_error(conn))
        {
            log_error("XCB connection is null.");
            return CONN_ERR;
        }
        return 0;
    }

    int
    cookie_error(xcb_void_cookie_t cookie , const char * sender_function)
    {
        if (check_conn != 0)
        {
            return CONN_ERR;
        }

        xcb_generic_error_t * err = xcb_request_check(conn, cookie);
        if (err)
        {
            log_error(err->error_code);
            free(err);
            return err->error_code;
        }
        return 0;
    }
} 

namespace win_tools 
{
    void 
    kill_client(xcb_connection_t *conn, xcb_window_t window) 
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
    apply_event_mask(const xcb_event_mask_t ev_mask, const xcb_window_t & win)
    {
        xcb_change_window_attributes
        (
            conn,
            win,
            XCB_CW_EVENT_MASK,
            (const uint32_t[1])
            {
                ev_mask
            }
        );
    }

    void
    apply_event_mask(const uint32_t * ev_mask, const xcb_window_t & win)
    {
        xcb_change_window_attributes
        (
            conn,
            win,
            XCB_CW_EVENT_MASK,
            ev_mask
        );
    }

    void
    grab_buttons(const xcb_window_t & win, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
    {
        for (const auto & binding : bindings)
        {
            const uint8_t & button = binding.first;
            const uint16_t & modifier = binding.second;
            xcb_grab_button
            (
                conn, 
                
                // 'OWNER_EVENTS : SET TO 0 FOR NO EVENT PROPAGATION'
                1, 
                win, 
                
                // EVENT MASK
                XCB_EVENT_MASK_BUTTON_PRESS, 
                
                // POINTER MODE
                XCB_GRAB_MODE_ASYNC, 
                
                // KEYBOARD MODE
                XCB_GRAB_MODE_ASYNC, 
                
                // CONFINE TO WINDOW
                XCB_NONE, 
                
                // CURSOR
                XCB_NONE, 
                button, 
                modifier    
            );
            // FLUSH THE REQUEST TO THE X SERVER
            xcb_flush(conn); 
        }
    }

    xcb_visualtype_t * /**
     *
     * @brief Function to find an ARGB visual 
     *
     */
    find_argb_visual(xcb_connection_t *conn, xcb_screen_t *screen) 
    {
        xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

        for (; depth_iter.rem; xcb_depth_next(&depth_iter)) 
        {
            xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) 
            {
                if (depth_iter.data->depth == 32) 
                {
                    return visual_iter.data;
                }
            }
        }
        return NULL;
    }

    void 
    get_win_w_h(xcb_window_t window, uint16_t & width, uint16_t & height) 
    {
        xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
        xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

        if (geometry) 
        {
            width = geometry->width;
            height = geometry->height;
            free(geometry);
        } 
        else 
        {
            width = height = 200;
        }
    }

    uint16_t 
    get_win_w(xcb_window_t window) 
    {
        xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
        xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

        uint16_t width;
        if (geometry) 
        {
            width = geometry->width;
            free(geometry);
        } 
        else 
        {
            width = 200;
        }
        return width;
    }
    
    uint16_t 
    get_win_h(xcb_window_t window) 
    {
        xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
        xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

        uint16_t height;
        if (geometry) 
        {
            height = geometry->height;
            free(geometry);
        } 
        else 
        {
            height = 200;
        }
        return height;
    }

    bool 
    getPointerPosition(xcb_window_t window, int& posX, int& posY) 
    {
        if (check_conn == CONN_ERR) 
        {
            return false;
        }

        // Query pointer
        xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, window);
        xcb_query_pointer_reply_t* reply = xcb_query_pointer_reply(conn, cookie, nullptr);

        if (!reply) 
        {
            log_error("Unable to query pointer position.");
            return false;
        }

        // Get pointer coordinates
        posX = reply->root_x;
        posY = reply->root_y;

        free(reply);
        return true;
    }

    int
    send_sigterm_to_client(client * c)
    {
        if (!c)
        {
            return - 1;
        }

        xcb_unmap_window(conn, c->titlebar);
        xcb_unmap_window(conn, c->frame);
        xcb_flush(conn);
        kill_client(conn, c->titlebar);
        kill_client(conn, c->frame);
        xcb_flush(conn);
        delete c;

        return 0;
    }

    void
    close_button_kill(client * c)
    {
        int result = send_sigterm_to_client(c);

        if (result == -1)
        {
            log_error("client is nullptr");
        }
    }
}

class Compositor 
{
    public:
        Compositor(xcb_connection_t* connection, xcb_screen_t* screen) 
        : connection(connection), root(screen->root) 
        {
            // Initialize XRender for ARGB visuals and pictures
            initRenderExtension();
        }

        void 
        addWindow(xcb_window_t window) 
        {
            // Check for a valid connection
            if (xcb_connection_has_error(connection)) 
            {
                log_error("XCB connection is null.");
                return;
            }

            // Get window dimensions
            uint16_t width = win_tools::get_win_w(window);
            uint16_t height = win_tools::get_win_h(window);
            if (width <= 0 || height <= 0) 
            {
                log_error("Invalid window dimensions.");
                return;
            }

            // Create an off-screen pixmap
            xcb_pixmap_t pixmap = xcb_generate_id(connection);
            xcb_void_cookie_t pixmap_cookie = xcb_create_pixmap_checked(connection, 32, pixmap, root, width, height);
            
            // Check for errors in pixmap creation
            xcb_generic_error_t* err = xcb_request_check(connection, pixmap_cookie);
            if (err) 
            {
                log_error("Failed to create pixmap. ERROR_CODE: " + std::to_string(err->error_code));
                free(err);
                return;
            }

            // Create a render picture
            xcb_render_picture_t picture = xcb_generate_id(connection);
            xcb_void_cookie_t picture_cookie = xcb_render_create_picture_checked(connection, picture, pixmap, renderFormat, 0, nullptr);

            // Check for errors in picture creation
            err = xcb_request_check(connection, picture_cookie);
            if (err) 
            {
                log_error("Failed to create render picture. ERROR_CODE: " + std::to_string(err->error_code));
                free(err);

                // Clean up the pixmap since picture creation failed
                xcb_free_pixmap(connection, pixmap);
                return;
            }

            // Store the picture associated with the window
            windowPictures[window] = picture;
        }

        void 
        removeWindow(xcb_window_t window) 
        {
            // Clean up resources for the window
            auto it = windowPictures.find(window);
            if (it != windowPictures.end()) {
                xcb_render_free_picture(connection, it->second);
                windowPictures.erase(it);
            }
        }

        void 
        composite() 
        {
            // Composite all windows onto the screen
            for (const auto& pair : windowPictures) {
                xcb_render_picture_t srcPic = pair.second;
                xcb_render_composite
                (
                    connection, 
                    XCB_RENDER_PICT_OP_OVER, 
                    srcPic, 
                    XCB_NONE, 
                    rootPicture,
                    0, 
                    0, 
                    0, 
                    0, 
                    0, 
                    0, 
                    win_tools::get_win_w(pair.first), 
                    win_tools::get_win_h(pair.first)
                );
            }

            xcb_flush(connection);
        }

        void 
        setTransparency(xcb_window_t window, float alpha) 
        {
            auto it = windowPictures.find(window);
            if (it != windowPictures.end()) 
            {
                xcb_render_picture_t picture = it->second;

                uint16_t w, h;
                win_tools::get_win_w_h(window, w, h);

                // Create a semi-transparent rectangle
                xcb_rectangle_t rect = {0, 0, w, h};
                uint32_t color = (static_cast<uint32_t>(alpha * 255) << 24); // Adjust alpha value

                // Create a graphic context for alpha blending
                xcb_gcontext_t gc = xcb_generate_id(connection);
                xcb_create_gc(connection, gc, root, 0, nullptr);

                // Fill the rectangle with the semi-transparent color
                xcb_change_gc(connection, gc, XCB_GC_FOREGROUND, &color);
                xcb_poly_fill_rectangle(connection, picture, gc, 1, &rect);

                // Free the graphic context
                xcb_free_gc(connection, gc);
            }
        }

    private:
        xcb_connection_t* connection;
        xcb_window_t root;
        xcb_render_pictformat_t renderFormat;
        std::unordered_map<xcb_window_t, xcb_render_picture_t> windowPictures;
        xcb_render_picture_t rootPicture;

        void 
        initRenderExtension() 
        {
            // Find ARGB visual and create a root picture
            xcb_render_query_pict_formats_cookie_t cookie = xcb_render_query_pict_formats(connection);
            xcb_render_query_pict_formats_reply_t* reply = xcb_render_query_pict_formats_reply(connection, cookie, nullptr);

            renderFormat = findArgbFormat(reply);
            rootPicture = xcb_generate_id(connection);
            xcb_render_create_picture(connection, rootPicture, root, renderFormat, 0, nullptr);

            free(reply);
        }

        xcb_render_pictformat_t 
        findArgbFormat(xcb_render_query_pict_formats_reply_t* reply) 
        {
            if (!reply) 
            {
                log_error("XRender query pict formats reply is null.");
                return 0;
            }

            xcb_render_pictformat_t format = 0;

            // Iterate over screens
            xcb_render_pictscreen_iterator_t screen_iter = xcb_render_query_pict_formats_screens_iterator(reply);
            for (; screen_iter.rem; xcb_render_pictscreen_next(&screen_iter)) 
            {
                if (!screen_iter.data) 
                {
                    log_error("Invalid pictscreen data.");
                    continue;
                }

                // Iterate over depths
                xcb_render_pictdepth_iterator_t depth_iter = xcb_render_pictscreen_depths_iterator(screen_iter.data);
                for (; depth_iter.rem; xcb_render_pictdepth_next(&depth_iter)) 
                {
                    if (!depth_iter.data) 
                    {
                        log_error("Invalid pictdepth data.");
                        continue;
                    }

                    // Check if the depth is 32
                    if (depth_iter.data->depth == 32) 
                    {
                        // Iterate over visuals at this depth
                        xcb_render_pictvisual_iterator_t visual_iter = xcb_render_pictdepth_visuals_iterator(depth_iter.data);
                        for (; visual_iter.rem; xcb_render_pictvisual_next(&visual_iter)) 
                        {
                            if (!visual_iter.data) 
                            {
                                log_error("Invalid pictvisual data.");
                                continue;
                            }

                            format = visual_iter.data->format;
                            if (format)
                            {
                                return format; /** 
                                 *
                                 * If a visual with 32-bit depth is found, return its format 
                                 *
                                 */
                            } 
                        }
                    }
                }
            }

            if (format == 0) 
            {
                log_error("No suitable ARGB format found.");
            }

            return format;
        }  
};

Compositor * gCompositor = nullptr;

class WinDecoretor 
{
    public:
        WinDecoretor(xcb_connection_t * connection, client * c) 
        : c(c)
        {
            if (xcb_connection_has_error(connection)) 
            {
                log_error("XCB connection is null.");
                return;
            }

            make_frame(c);
            log_win( "c->frame: ", c->frame);
            make_titlebar(c);
            log_win( "c->titlebar: ", c->titlebar);
            make_close_button(c);
            log_win( "c->close_button: ", c->close_button);
            make_max_button(c);
            log_win( "c->max_button: ", c->max_button);
            make_min_button(c);
            log_win( "c->min_button: ", c->min_button);
        }
        
    private:
        client * c;

        void
        apply_event_mask(const xcb_event_mask_t ev_mask, const xcb_window_t & win)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_EVENT_MASK,
                (const uint32_t[1])
                {
                    ev_mask
                }
            );
        }

        void
        apply_event_mask(const uint32_t * values, const xcb_window_t & win)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_EVENT_MASK,
                values
            );
        }

        void 
        make_frame(client * & c)
        {
            // CREATE A FRAME WINDOW
            c->frame = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->frame, 
                screen->root, 
                c->x, 
                c->y, 
                c->width, 
                c->height + 20, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );

            xcb_change_window_attributes
            (
                conn, 
                c->frame, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(DARK_GREY)
                }
            );

            // REPARENT THE PROGRAM_WINDOW TO THE FRAME_WINDOW
            xcb_reparent_window
            (
                conn, 
                c->win, 
                c->frame, 
                0, 
                20
            );

            xcb_map_window(conn, c->frame);
            xcb_flush(conn);
        
            apply_event_mask
            (
                (const uint32_t[2])
                {
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                    XCB_EVENT_MASK_ENTER_WINDOW
                },
                c->frame
            );
            xcb_flush(conn);
        }

        void
        make_titlebar(client * & c)
        {
            c->titlebar = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->titlebar, 
                c->frame, 
                0, 
                0, 
                c->width, 
                20, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );

            xcb_change_window_attributes
            (
                conn, 
                c->titlebar, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(BLACK)
                }
            );

            win_tools::grab_buttons(c->titlebar, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->titlebar);
            xcb_flush(conn);

            // draw_text("sug", WHITE, BLACK, c->titlebar, 2, 14);
        }

        void
        make_close_button(client * & c)
        {
            c->close_button = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->close_button,
                c->frame,
                c->width - 20,
                0,
                20,
                20,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );

            xcb_change_window_attributes
            (
                conn, 
                c->close_button, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(BLUE)
                }
            );

            win_tools::grab_buttons(c->close_button, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->close_button);
            xcb_flush(conn);

            apply_event_mask
            (
                (const uint32_t[3]) 
                {
                    XCB_EVENT_MASK_BUTTON_PRESS,
                    XCB_EVENT_MASK_ENTER_WINDOW,
                    XCB_EVENT_MASK_LEAVE_WINDOW
                }, 
                c->close_button
            );
            xcb_flush(conn);
        }

        void
        make_max_button(client * & c)
        {
            c->max_button = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->max_button,
                c->frame,
                c->width - 40,
                0,
                20,
                20,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );

            xcb_change_window_attributes
            (
                conn, 
                c->max_button, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(RED)
                }
            );

            apply_event_mask
            (
                (const uint32_t[3]) 
                {
                    XCB_EVENT_MASK_BUTTON_PRESS,
                    XCB_EVENT_MASK_ENTER_WINDOW,
                    XCB_EVENT_MASK_LEAVE_WINDOW
                }, 
                c->max_button
            );

            win_tools::grab_buttons(c->max_button, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->max_button);
            xcb_flush(conn);
        }

        void
        make_min_button(client * & c)
        {
            c->min_button = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->min_button,
                c->frame,
                c->width - 60,
                0,
                20,
                20,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );

            xcb_change_window_attributes
            (
                conn, 
                c->min_button, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(GREEN)
                }
            );

            apply_event_mask
            (
                (const uint32_t[3]) 
                {
                    XCB_EVENT_MASK_BUTTON_PRESS,
                    XCB_EVENT_MASK_ENTER_WINDOW,
                    XCB_EVENT_MASK_LEAVE_WINDOW
                }, 
                c->min_button
            );

            win_tools::grab_buttons(c->min_button, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->min_button);
            xcb_flush(conn);
        }
};

class WinManager 
{
    public:
        static void 
        manage_new_window(const xcb_drawable_t & w) 
        {
            client * c = make_client(w);
            if (!c)
            {
                return;
            }
            
            log.log(INFO, __func__, "c->win: " + std::to_string(c->win));
            wm::setWindowPosition(c);
            wm::setWindowSize(c);

            // MAP THE WINDOW
            XCB_map_window(w);
            // xcb_map_window(conn, c->win);  
            xcb_flush(conn);

            // GRAB MOUSE BUTTONS FOR THE WINDOW 
            // SO WINDOW CAN BE INTERACTED WITH
            grab_buttons(c, 
            {
                {   L_MOUSE_BUTTON,     ALT },
                {   R_MOUSE_BUTTON,     ALT },
                {   L_MOUSE_BUTTON,     0   }
            });
            
            // GRAB KEYS FOR THE WINDOW SO THAT KEYBINDINGS WORK
            grab_keys(c, 
            {
                {   T,          ALT | CTRL              },
                {   Q,          ALT | SHIFT             },
                {   F11,        NULL                    },
                {   N_1,        ALT                     },
                {   N_2,        ALT                     },
                {   N_3,        ALT                     },
                {   N_4,        ALT                     },
                {   N_5,        ALT                     },
                {   R_ARROW,    CTRL | SUPER            },
                {   L_ARROW,    CTRL | SUPER            },
                {   R_ARROW,    CTRL | SUPER | SHIFT    },
                {   L_ARROW,    CTRL | SUPER | SHIFT    },
                {   R_ARROW,    SUPER                   },
                {   L_ARROW,    SUPER                   },
                {   U_ARROW,    SUPER                   },
                {   D_ARROW,    SUPER                   },
                {   TAB,        ALT                     },
                {   K,          SUPER                   }
            });

            WinDecoretor(conn ,c);
            
            win_tools::apply_event_mask
            (
                (const uint32_t[2]) 
                {
                    XCB_EVENT_MASK_FOCUS_CHANGE,
                    XCB_EVENT_MASK_ENTER_WINDOW
                }, 
                c->win
            );

            get::name(c);
            wm::update_client(c);
            focus::client(c);
        }
 
        static void 
        kill_client(xcb_connection_t *conn, xcb_window_t window) 
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

    private:
        static void
        grab_buttons(client * & c, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
        {
            for (const auto & binding : bindings)
            {
                const uint8_t & button = binding.first;
                const uint16_t & modifier = binding.second;
                xcb_grab_button
                (
                    conn, 
                    
                    // 'OWNER_EVENTS : SET TO 0 FOR NO EVENT PROPAGATION'
                    1, 
                    c->win, 
                    
                    // EVENT MASK
                    XCB_EVENT_MASK_BUTTON_PRESS, 
                    
                    // POINTER MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // KEYBOARD MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // CONFINE TO WINDOW
                    XCB_NONE, 
                    
                    // CURSOR
                    XCB_NONE, 
                    button, 
                    modifier    
                );
                // FLUSH THE REQUEST TO THE X SERVER
                xcb_flush(conn); 
            }
        }

        static void
        grab_buttons(const xcb_window_t & win, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
        {
            for (const auto & binding : bindings)
            {
                const uint8_t & button = binding.first;
                const uint16_t & modifier = binding.second;
                xcb_grab_button
                (
                    conn, 
                    
                    // 'OWNER_EVENTS : SET TO 0 FOR NO EVENT PROPAGATION'
                    1, 
                    win, 
                    
                    // EVENT MASK
                    XCB_EVENT_MASK_BUTTON_PRESS, 
                    
                    // POINTER MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // KEYBOARD MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // CONFINE TO WINDOW
                    XCB_NONE, 
                    
                    // CURSOR
                    XCB_NONE, 
                    button, 
                    modifier    
                );
                // FLUSH THE REQUEST TO THE X SERVER
                xcb_flush(conn); 
            }
        }

        static void 
        grab_keys(client * & c, std::initializer_list<std::pair<const uint32_t, const uint16_t>> bindings) 
        {
            xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(conn);
        
            if (!keysyms) 
            {
                LOG_error("keysyms could not get initialized");
                return;
            }

            for (const auto & binding : bindings) 
            {
                xcb_keycode_t * keycodes = xcb_key_symbols_get_keycode(keysyms, binding.first);

                if (keycodes)
                {
                    for (auto * kc = keycodes; * kc; kc++) 
                    {
                        xcb_grab_key
                        (
                            // CONNECTION TO THE X SERVER
                            conn,
                            
                            // 'OWNER_EVENTS' SET TO 1 TO ALLOW EVENT PROPAGATION 
                            1,

                            // KEYS WILL BE GRABBED ON THIS WINDOW          
                            c->win,
                            
                            // MODIFIER MASK
                            binding.second, 
                            
                            // KEYCODE
                            *kc,        

                            // POINTER MODE
                            XCB_GRAB_MODE_ASYNC, 
                            
                            // KEYBOARD MODE
                            XCB_GRAB_MODE_ASYNC  
                        );
                    }
                    // FREE THE MEMORY THAT WAS ALLOCATED FOR THE KEYCODES
                    free(keycodes);
                }
            }
            xcb_key_symbols_free(keysyms);
            
            // FLUSH THE REQUEST TO THE X SERVER
            // SO THAT THE X SERVER HANDELS THIS REQUEST NOW
            xcb_flush(conn); 
        }

        static void
        apply_event_mask(client * & c)
        {
            xcb_change_window_attributes
            (
                conn,
                c->win,
                XCB_CW_EVENT_MASK,
                (const uint32_t[1])
                {
                    XCB_EVENT_MASK_FOCUS_CHANGE
                }
            );
        }

        static void
        apply_event_mask(const xcb_window_t & win)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_EVENT_MASK,
                (const uint32_t[1])
                {
                    XCB_EVENT_MASK_FOCUS_CHANGE
                }
            );
        }

        static bool 
        is_exclusive_fullscreen(client * & c) 
        {
            xcb_ewmh_get_atoms_reply_t wm_state;
            if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, c->win), & wm_state, NULL)) 
            {
                for (unsigned int i = 0; i < wm_state.atoms_len; i++) 
                {
                    if ((wm_state.atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN) 
                    && (i + 1 < wm_state.atoms_len) 
                    && (wm_state.atoms[i + 1] == XCB_NONE)) 
                    {
                        // EXCLUSIVE FULLSCREEN STATE DETECTED
                        xcb_ewmh_get_atoms_reply_wipe(& wm_state);
                        return true;
                    }
                }
                xcb_ewmh_get_atoms_reply_wipe(& wm_state);
            }
            return false;
        }

        static client * 
        make_client(const xcb_window_t & win) 
        {
            // client * c = static_cast<client *>(malloc(sizeof(client)));
            client * c = new client;
            
            if (!c) 
            {
                log.log(ERROR, __func__, "Could not allocate memory for client");
                return nullptr;
            }

            c->win      = win;
            c->height   = (data.height < 300) ? 300 : data.height;
            c->width    = (data.width < 400)  ? 400 : data.width;
            c->x        = (data.x <= 0)       ? (screen->width_in_pixels / 2)  - (c->width / 2)  : data.x;
            c->y        = (data.y <= 0)       ? (screen->height_in_pixels / 2) - (c->height / 2) : data.y;
            
            // DEFAULT DEPTH IS 24 BIT
            // THIS CAN BE ADJUSTED IF NESSESARY
            c->depth    = 24; 
            c->desktop  = cur_d->desktop;

            if (is_exclusive_fullscreen(c)) 
            {
                c->x = 0;
                c->y = 0;
                c->width = screen->width_in_pixels;
                c->height = screen->height_in_pixels;
                c->ismax = true;
            }
            else 
            {
                c->ismax = false;
            }
        
            client_list.push_back(c);
            cur_d->current_clients.push_back(c);
            return c;
        }

        static void 
        make_frame(client * & c)
        {
            // CREATE A FRAME WINDOW
            c->frame = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->frame, 
                screen->root, 
                c->x, 
                c->y, 
                c->width, 
                c->height + 20, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );
            log.log(INFO, __func__, "c->frame: " + std::to_string(c->frame));

            // SET THE BACKGROUND COLOR TO WHITE
            xcb_change_window_attributes
            (
                conn, 
                c->frame, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(BLUE)
                }
            );
            
            // REPARENT THE PROGRAM_WINDOW TO THE FRAME_WINDOW
            xcb_reparent_window
            (
                conn, 
                c->win, 
                c->frame, 
                0, 
                20
            );
            apply_event_mask(c->frame); 

            xcb_map_window(conn, c->frame);
            xcb_flush(conn); 

            // draw_text("sug", WHITE, BLUE, c->frame, 2, 17);
            make_titlebar(c);
        }

        static void
        make_titlebar(client * & c)
        {
            c->titlebar = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->titlebar, 
                c->frame, 
                0, 
                0, 
                screen->width_in_pixels, 
                20, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );

            xcb_change_window_attributes
            (
                conn, 
                c->titlebar, 
                XCB_CW_BACK_PIXEL, 
                (const uint32_t[1])
                {
                    color::get(BLACK)
                }
            );

            apply_event_mask(c->titlebar);
            grab_buttons(c->titlebar, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->titlebar);
            xcb_flush(conn);

            draw_text("sug", WHITE, BLACK, c->titlebar, 2, 14);
        }

        static void
        get_win_info(client * & c)
        {
            get::WindowProperty(c, "WINDOW");
            get::WindowProperty(c, "WM_CLASS");
            get::WindowProperty(c, "FULL_NAME");
            get::WindowProperty(c, "ATOM");
            get::WindowProperty(c, "DRAWABLE");
            get::WindowProperty(c, "FONT");
            get::WindowProperty(c, "INTEGER");
            get::WindowProperty(c, "PIXMAP");
            get::WindowProperty(c, "VISUALID");
            get::WindowProperty(c, "WM_COMMAND");
            get::WindowProperty(c, "WM_HINTS");
            get::WindowProperty(c, "WM_NORMAL_HINTS");
            get::WindowProperty(c, "MIN_SPACE");
            get::WindowProperty(c, "NORM_SPACE");
            get::WindowProperty(c, "WM_SIZE_HINTS");
            get::WindowProperty(c, "NOTICE");
            get::WindowProperty(c, "_NET_WM_NAME");
            get::WindowProperty(c, "_NET_WM_STATE");
            get::WindowProperty(c, "_NET_WM_VISIBLE_NAME");
            get::WindowProperty(c, "_NET_WM_ICON_NAME");
            get::WindowProperty(c, "_NET_WM_VISIBLE_ICON_NAME");
            get::WindowProperty(c, "_NET_WM_DESKTOP");
            get::WindowProperty(c, "_NET_WM_WINDOW_TYPE");
            get::WindowProperty(c, "_NET_WM_STATE");
            get::WindowProperty(c, "_NET_WM_ALLOWED_ACTIONS");
            get::WindowProperty(c, "_NET_WM_STRUT");
            get::WindowProperty(c, "_NET_WM_STRUT_PARTIAL");
            get::WindowProperty(c, "_NET_WM_ICON_GEOMETRY");
            get::WindowProperty(c, "_NET_WM_ICON");
            get::WindowProperty(c, "_NET_WM_PID");
            get::WindowProperty(c, "_NET_WM_HANDLED_ICONS");
            get::WindowProperty(c, "_NET_WM_USER_TIME");
            get::WindowProperty(c, "_NET_WM_USER_TIME_WINDOW");
            get::WindowProperty(c, "_NET_FRAME_EXTENTS");
            get::WindowProperty(c, "_NET_SUPPORTED");
        }
};

/**
 * @class tile
 * @brief Represents a tile obj.
 * 
 * The `tile` class is responsible for managing the tiling behavior of windows in the window manager.
 * It provides methods to tile windows to the left, right, *up, or *down positions on the screen.
 * The class also includes helper methods to check the current tile position of a window and set the size and position of a window.
 */
class tile 
{
    public:
        tile(client * & c, TILE tile)
        {
            switch (tile) 
            {
                case TILE::LEFT:
                {
                    // IF 'CURRENTLT_TILED' TO 'LEFT'
                    if (current_tile_pos(c, TILEPOS::LEFT))
                    {
                        set_tile_ogsize(c);
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' TO 'RIGHT', 'LEFT_DOWN' OR 'LEFT_UP'
                    if (current_tile_pos(c, TILEPOS::RIGHT)
                     || current_tile_pos(c, TILEPOS::LEFT_DOWN)
                     || current_tile_pos(c, TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' TO 'RIGHT_DOWN'
                    if (current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' TO 'RIGHT_UP'
                    if (current_tile_pos(c, TILEPOS::RIGHT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_UP);
                        return;
                    }

                    save_tile_ogsize(c);
                    set_tile_sizepos(c, TILEPOS::LEFT);
                    break;
                }
                case TILE::RIGHT:
                {
                    // IF 'CURRENTLY_TILED' TO 'RIGHT'
                    if (current_tile_pos(c, TILEPOS::RIGHT))
                    {
                        set_tile_ogsize(c);
                        return;
                    }

                    // IF 'CURRENTLT_TILED' TO 'LEFT', 'RIGHT_DOWN' OR 'RIGHT_UP' 
                    if (current_tile_pos(c, TILEPOS::LEFT)
                     || current_tile_pos(c, TILEPOS::RIGHT_UP)
                     || current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT);
                        return;
                    }

                    // IF 'CURRENTLT_TILED' 'LEFT_DOWN'
                    if (current_tile_pos(c, TILEPOS::LEFT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'LEFT_UP'
                    if (current_tile_pos(c, TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_UP);
                        return;
                    }

                    save_tile_ogsize(c);
                    set_tile_sizepos(c, TILEPOS::RIGHT);
                    break;
                }
                case TILE::DOWN:
                {
                    // IF 'CURRENTLY_TILED' 'LEFT' OR 'LEFT_UP'
                    if (current_tile_pos(c, TILEPOS::LEFT)
                     || current_tile_pos(c, TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR 'RIGHT_UP'
                    if (current_tile_pos(c, TILEPOS::RIGHT)
                     || current_tile_pos(c, TILEPOS::RIGHT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'LEFT_DOWN' OR 'RIGHT_DOWN'
                    if (current_tile_pos(c, TILEPOS::LEFT_DOWN)
                     || current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_ogsize(c);
                        return;
                    }
                }
                case TILE::UP:
                {
                    // IF 'CURRENTLY_TILED' 'LEFT'
                    if (current_tile_pos(c, TILEPOS::LEFT)
                     || current_tile_pos(c, TILEPOS::LEFT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_UP);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR RIGHT_DOWN
                    if (current_tile_pos(c, TILEPOS::RIGHT)
                     || current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_UP);
                        return;
                    }
                }
            }
        } 

    private:
        void 
        save_tile_ogsize(client * & c)
        {
            c->tile_ogsize.x        = c->x;
            c->tile_ogsize.y        = c->y;
            c->tile_ogsize.width    = c->width;
            c->tile_ogsize.height   = c->height;
        }
        
        void
        moveresize(client * & c)
        {
            xcb_configure_window
            (
                conn,
                c->win,
                XCB_CONFIG_WINDOW_WIDTH | 
                XCB_CONFIG_WINDOW_HEIGHT| 
                XCB_CONFIG_WINDOW_X |
                XCB_CONFIG_WINDOW_Y,
                (const uint32_t[4])
                {
                    static_cast<const uint32_t &>(c->width),
                    static_cast<const uint32_t &>(c->height),
                    static_cast<const uint32_t &>(c->x),
                    static_cast<const uint32_t &>(c->y)
                }
            );
            xcb_flush(conn);
        }

        bool
        current_tile_pos(client * & c, TILEPOS mode)
        {
            switch (mode) 
            {
                case TILEPOS::LEFT:
                {
                    if (c->x        == 0 
                     && c->y        == 0 
                     && c->width    == screen->width_in_pixels / 2 
                     && c->height   == screen->height_in_pixels)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::RIGHT:
                {
                    if (c->x        == screen->width_in_pixels / 2 
                     && c->y        == 0 
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::LEFT_DOWN:
                {
                    if (c->x        == 0
                     && c->y        == screen->height_in_pixels / 2
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::RIGHT_DOWN:
                {
                    if (c->x        == screen->width_in_pixels / 2
                     && c->y        == screen->height_in_pixels / 2
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::LEFT_UP:
                {
                    if (c->x        == 0
                     && c->y        == 0
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::RIGHT_UP:
                {
                    if (c->x        == screen->width_in_pixels / 2
                     && c->y        == 0
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
            }
            return false;
        }

        void
        set_tile_sizepos(client * & c, TILEPOS sizepos)
        {
            switch (sizepos) 
            {
                case TILEPOS::LEFT:
                {
                    animate
                    (
                        c, 
                        0, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels
                    );
                    return;
                }
                case TILEPOS::RIGHT:
                {
                    animate
                    (
                        c, 
                        screen->width_in_pixels / 2, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels
                    );
                    return;
                }
                case TILEPOS::LEFT_DOWN:
                {
                    animate
                    (
                        c, 
                        0, 
                        screen->height_in_pixels / 2, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                }
                case TILEPOS::RIGHT_DOWN:
                {
                    animate
                    (
                        c, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                }
                case TILEPOS::LEFT_UP:
                {
                    animate
                    (
                        c, 
                        0, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                } 
                case TILEPOS::RIGHT_UP:
                {
                    animate
                    (
                        c, 
                        screen->width_in_pixels / 2, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                }
            }
        }

        void 
        set_tile_ogsize(client * & c)
        {
            animate
            (
                c, 
                c->tile_ogsize.x, 
                c->tile_ogsize.y, 
                c->tile_ogsize.width, 
                c->tile_ogsize.height
            );
        }

        void
        animate_old(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            XCPPBAnimator anim(conn, c->frame);
            anim.animate
            (
                c->x,
                c->y, 
                c->width, 
                c->height, 
                endX,
                endY, 
                endWidth, 
                endHeight, 
                TILE_ANIMATION_DURATION
            );
            wm::update_client(c);
        }

        void
        animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            XCPPBAnimator anim(conn,  c);
            anim.animate_client
            (
                c->x,
                c->y, 
                c->width, 
                c->height, 
                endX,
                endY, 
                endWidth, 
                endHeight, 
                TILE_ANIMATION_DURATION
            );
            wm::update_client(c);
        }
};

class Event 
{
    public:
        /**
         *
         * @brief Constructor for the Event class.
         *        Initializes the key symbols and keycodes.
         *
         */
        Event()
        {
            initialize_keysyms();
        }

        /**
         *
         * @brief Destructor for the Event class.
         *        Frees the allocated key symbols.
         *
         */
        ~Event() 
        {
            if (keysyms) 
            {
                xcb_key_symbols_free(keysyms);
            }
        }
        
        void /**
         *
         * @brief Event handler function that processes the incoming XCB events.
         * 
         * @param ev The XCB event to be handled.
         *
         */
        handler(const xcb_generic_event_t * ev)
        {
            switch (ev->response_type & ~0x80) 
            {
                case XCB_KEY_PRESS: 
                {
                    key_press_handler(ev);
                    break;
                }
                case XCB_MAP_NOTIFY: 
                {
                    map_notify_handler(ev);
                    break;
                }
                case XCB_MAP_REQUEST: 
                {
                    map_req_handler(ev);
                    break;
                }
                case XCB_BUTTON_PRESS: 
                {
                    button_press_handler(ev);
                    break;
                }
                case XCB_CONFIGURE_REQUEST:
                {
                    configure_request_handler(ev);
                    break;
                }
                case XCB_FOCUS_IN:
                {
                    focus_in_handler(ev);
                    break;
                }
                case XCB_FOCUS_OUT:
                {
                    focus_out_handler(ev);
                    break;
                }
                /** TODO ** 
                 *
                 * @brief Handle the 'XCB_CLIENT_MESSAGE' event.
                 *        This event is used to handle the 'close' button on the window.
                 *        When the 'close' button is pressed, the window is destroyed.
                 *        
                 * @param ev The XCB event to be handled.
                 *
                 * @note This event is not currently handled.
                 *       This needs to be checked and implemented.
                 *
                 */
                case XCB_CLIENT_MESSAGE: 
                {
                    break;
                }
                case XCB_DESTROY_NOTIFY:
                {
                    destroy_notify_handler(ev);
                    break;
                }
                case XCB_UNMAP_NOTIFY:
                {
                    unmap_notify_handler(ev);
                    break;
                }
                case XCB_REPARENT_NOTIFY:
                {
                    reparent_notify_handler(ev);
                    break;
                }
                case XCB_ENTER_NOTIFY:
                {
                    enter_notify_handler(ev);
                    break;
                }
            }
        }

    private:
        xcb_key_symbols_t * keysyms;
        /*
            VARIABELS TO STORE KEYCODES
         */ 
        xcb_keycode_t t{}, q{}, f{}, f11{}, n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{}, l_arrow{}, u_arrow{}, d_arrow{}, tab{}, k{}; 
        
        void 
        key_press_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_key_press_event_t * &>(ev);
            
            /*
                CHECK IF 'ALT+CTRL+T' WAS PRESSED
                AND IF SO LAUNCH TERMINAL   
             */         
            if (e->detail == t)  
            {
                switch (e->state) 
                {
                    case CTRL + ALT:
                    {
                        log.log(INFO, __func__, "ALT+CTRL+T");
                        wm::launchTerminal();
                        break;
                    }
                }
            }

            /*
                CHECK IF 'ALT+SHIFT+Q' WAS PRESSED
                AND IF SO LAUNCH KILL_SESSION
             */ 
            if (e->detail == q) 
            {
                switch (e->state) 
                {
                    case SHIFT + ALT:
                    {
                        log.log(INFO, __func__, "ALT+SHIFT+Q");
                        wm::kill_session();
                    }
                }
            }

            /*
                CHECK IF 'F11' WAS PRESSED
                AND IF SO TOGGLE FULLSCREEN
             */ 
            if (e->detail == f11)
            {
                log.log(INFO, __func__, "F11");
                client *c = get::client_from_win(&e->event);
                borrowed::maxwin(c, 0);
            }

            /*
                CHECK IF 'ALT+1' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 1
             */
            if (e->detail == n_1) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log.log(INFO, __func__, "ALT+1");
                        move_desktop(1);
                    }
                }
            }

            /*
                CHECK IF 'ALT+2' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 2
             */ 
            if (e->detail == n_2) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log.log(INFO, __func__, "ALT+2");
                        move_desktop(2);
                    }
                }
            }

            /*
                CHECK IF 'ALT+3' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 3
             */ 
            if (e->detail == n_3) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log.log(INFO, __func__, "ALT+3");
                        move_desktop(3);
                    }
                }
            }

            /*
                CHECK IF 'ALT+4' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 4
             */
            if (e->detail == n_4) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log.log(INFO, __func__, "ALT+4");
                        move_desktop(4);
                    }
                }
            }

            /*
                CHECK IF 'ALT+5' WAS PRESSED
                IF SO MOVE TO DESKTOP 5
             */
            if (e->detail == n_5) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log.log(INFO, __func__, "ALT+5");
                        move_desktop(5);
                    }
                }
            }

            /*
                IF R_ARROW IS PRESSED THEN CHECK WHAT MOD MASK IS APPLIED

                IF 'SHIFT+CTRL+SUPER' THEN MOVE TO NEXT DESKTOP WITH CURRENTLY FOCUSED APP
                IF 'CTRL+SUPER' THEN MOVE TO THE NEXT DESKTOP
                IF 'SUPER' THEN TILE WINDOW TO THE RIGHT
             */
            if (e->detail == r_arrow)
            {
                switch (e->state) 
                {
                    case SHIFT + CTRL + SUPER:
                    {
                        move_to_next_desktop_w_app();
                        break;
                    }
                    case CTRL + SUPER:
                    {
				        Next_Desktop();
                        break;
                    }
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::RIGHT);
                        break;
                    }
                    return;
                }
            }

            /*
                IF L_ARROW IS PRESSED THEN CHECK WHAT MOD MASK IS APPLIED

                IF 'SHIFT+CTRL+SUPER' THEN MOVE TO PREV DESKTOP WITH CURRENTLY FOCUSED APP
                IF 'CTRL+SUPER' THEN MOVE TO THE PREV DESKTOP
                IF 'SUPER' THEN TILE WINDOW TO THE LEFT
             */
            if (e->detail == l_arrow)
            {
                switch (e->state) 
                {
                    case SHIFT + CTRL + SUPER:
                    {
                        move_to_previus_desktop_w_app();
                        break;
                    }
                    case CTRL + SUPER:
                    {
				        Prev_Desktop();
                        break;
                    }
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        log_win("e->event", e->event);
                        tile(c, TILE::LEFT);
                        break;
                    }
                }
            }

            /*
                IF 'D_ARROW' IS PRESSED SEND TO TILE
             */
            if (e->detail == d_arrow)
            {
                switch (e->state) 
                {
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::DOWN);
                        return;
                        break;
                    }
                }
            }

            if (e->detail == u_arrow)
            {
                switch (e->state) 
                {
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::UP);
                        break;
                    }
                }
            }

            /*
                CHECK IF 'ALT+TAB' WAS PRESSED
                IF SO CYCLE FOCUS
             */
            if (e->detail == tab) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        wm::cycle_focus();
                        break;
                    }
                }
            }

            /**
             *
             * @brief This is a debug kebinding to test featurtes before they are implemented.  
             * 
             */
            if (e->detail == k)
            {
                switch (e->state) 
                {
                    case SUPER:
                    {
                        log_win("e->event", e->event);
                        log_win("e->event-parent", XCBwm::get_parent_window(e->event));
                        break;
                    }
                }
            }
        }

        void /*
            INITIALIZES KEYBOARD KEY SYMBOLS AND STORES 
            THE KEYCODES UNTIL SESSION IS KILLED 
         */
        initialize_keysyms() 
        {
            keysyms = xcb_key_symbols_alloc(conn);
            if (keysyms) 
            {
                xcb_keycode_t * t_keycodes          = xcb_key_symbols_get_keycode(keysyms, T);
                xcb_keycode_t * q_keycodes          = xcb_key_symbols_get_keycode(keysyms, Q);
                xcb_keycode_t * f_keycodes          = xcb_key_symbols_get_keycode(keysyms, F);
                xcb_keycode_t * f11_keycodes        = xcb_key_symbols_get_keycode(keysyms, F11);
                xcb_keycode_t * n_1_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_1);
                xcb_keycode_t * n_2_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_2);
                xcb_keycode_t * n_3_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_3);
                xcb_keycode_t * n_4_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_4);
                xcb_keycode_t * n_5_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_5);
                xcb_keycode_t * r_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, R_ARROW);
                xcb_keycode_t * l_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, L_ARROW);
                xcb_keycode_t * u_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, U_ARROW);
                xcb_keycode_t * d_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, D_ARROW);
                xcb_keycode_t * tab_keycodes        = xcb_key_symbols_get_keycode(keysyms, TAB);
                xcb_keycode_t * k_keycodes			= xcb_key_symbols_get_keycode(keysyms, K);
                
                if (t_keycodes) 
                {
                    t = * t_keycodes;
                    free(t_keycodes);
                }
                
                if (q_keycodes) 
                {
                    q = * q_keycodes;
                    free(q_keycodes);
                }
                
                if (f_keycodes)
                {
                    f = * f_keycodes;
                    free(f_keycodes);
                }

                if (f11_keycodes)
                {
                    f11 = * f11_keycodes;
                    free(f11_keycodes);
                }

                if (n_1_keycodes)
                {
                    n_1 = * n_1_keycodes;
                    free(n_1_keycodes);
                }

                if (n_2_keycodes)
                {
                    n_2 = * n_2_keycodes;
                    free(n_2_keycodes);
                }

                if (n_3_keycodes)
                {
                    n_3 = * n_3_keycodes;
                    free(n_3_keycodes);
                }

                if (n_4_keycodes)
                {
                    n_4 = * n_4_keycodes;
                    free(n_4_keycodes);
                }

                if (n_5_keycodes)
                {
                    n_5 = * n_5_keycodes;
                    free(n_5_keycodes);
                }

                if (r_arrow_keycodes)
                {
                    r_arrow = * r_arrow_keycodes;
                    free(r_arrow_keycodes);
                }

                if (l_arrow_keycodes)
                {
                    l_arrow = * l_arrow_keycodes;
                    free(l_arrow_keycodes);
                }

                if (u_arrow_keycodes)
                {
                    u_arrow = * u_arrow_keycodes;
                    free(u_arrow_keycodes);
                }

                if (d_arrow_keycodes)
                {
                    d_arrow = * d_arrow_keycodes;
                    free(d_arrow_keycodes);
                }

                if (tab_keycodes)
                {
                    tab = * tab_keycodes;
                    free(tab_keycodes);
                }

                if (k_keycodes)
				{
					k = * k_keycodes;
					free(k_keycodes);
				}
            }
        }
        
        void 
        map_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_map_notify_event_t *>(ev);
            client * c = get::client_from_win(& e->window);
            if (c)
            {
                wm::update_client(c);
            }
        }
        
        void 
        map_req_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_map_request_event_t *>(ev);
            WinManager::manage_new_window(e->window);
        }
        
        void 
        button_press_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
            log_win("e->event: ", e->event);
            client * c = get::client_from_all_win(& e->event);
            if (!c)
            {
                log_error("c == null");
                return;
            }
            
            if (e->detail == L_MOUSE_BUTTON)
            {
                if (e->event == c->close_button)
                {
                    log_error("L_MOUSE_BUTTON + close_button");
                    win_tools::close_button_kill(c);
                    return;
                }

                if (e->event == c->titlebar)
                {
                    log_error("L_MOUSE_BUTTON + titlebar");
                    wm::raise_client(c);
                    mv_client(c, e->event_x, e->event_y);
                    focus::client(c);
                    return;
                }

                if (e->event == c->win)
                {
                    switch (e->state) 
                    {
                        case ALT:
                        {
                            log_error("ALT + L_MOUSE_BUTTON + win");
                            wm::raise_client(c);
                            mv_client(c, e->event_x, e->event_y + 20);
                            focus::client(c);
                            break;
                        }
                    }
                    log_info("L_MOUSE_BUTTON + win");
                    wm::raise_client(c);
                    focus::client(c);
                    return;
                }
            }

            if (e->detail == R_MOUSE_BUTTON) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log_error("ALT + R_MOUSE_BUTTON");
                        wm::raise_client(c);
                        resize_client(c, 0);
                        focus::client(c);
                        return;
                    }
                }
            }
        }

        void
        configure_request_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_configure_request_event_t *>(ev);
            data.width     = e->width;
            data.height    = e->height;
            data.x         = e->x;
            data.y         = e->y;
        }

        void
        focus_in_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_in_event_t *>(ev);
            log_win("e->event: ", e->event);
            
            client * c = get::client_from_win(& e->event);
            if (c)
            {
                wm::ungrab_button(c, L_MOUSE_BUTTON, 0);
                wm::raise_client(c);
                focused_client = c;
            }
        }

        void
        focus_out_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_out_event_t *>(ev);
            log_win("e->event: ", e->event);
            
            client * c = get::client_from_win(& e->event);
            if (!c)
            {
                return;
            }
            
            if (c == focused_client)
            {
                return;
            }
            
            wm::grab_button(c, L_MOUSE_BUTTON, 0);
        }

        void
        destroy_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_destroy_notify_event_t *>(ev);

            log_win("e->window: ", e->window);
            log_win("e->event: ", e->event);
            client * c = get::client_from_win(& e->window);
            if (!c)
            {
                return;
            }

            xcb_unmap_window(conn, c->titlebar);
            xcb_unmap_window(conn, c->frame);
            xcb_flush(conn);
            WinManager::kill_client(conn, c->titlebar);
            WinManager::kill_client(conn, c->frame);
            xcb_flush(conn);
            delete c;
        }

        void
        unmap_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_unmap_notify_event_t *>(ev);
            
            client * c = get::client_from_win(& e->window);
            if (!c)
            {
                return;
            }

            // if (c->isKilleble)
            // {
            //     xcb_unmap_window(conn, c->titlebar);
            //     xcb_unmap_window(conn, c->frame);
            //     delete c;
            //     XCB_flush();
            // }
        }

        void 
        reparent_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_reparent_notify_event_t *>(ev);
        }
        
        void
        enter_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
            log_win("e->event: ", e->event);
            log_win("e->child: ", e->child);
            log_win("e->root: ", e->root);         
        }
};

void /**
 * THIS IS THE MAIN EVENT LOOP
 * FOR THE 'WINDOW_MANAGER' 
 */
run() 
{
    // INITIALIZES THE EVENT CLASS AS EVENT
    Event event;
    while (true)
    {
        xcb_generic_event_t * ev = xcb_wait_for_event(conn);
        if (!ev) 
        {
            continue;
        }
        // SEND THE EVENT TO THE EVENT HANDLER
        event.handler(ev);
        
        // DEALLOCATE THE MEMORY THE EVENT
        // USED AFTER IT HAS BEEN HANDLED
        free(ev);

        // xcb_generic_event_t * ev;
        // while ((ev = xcb_poll_for_event(conn)))
        // {
        //     if (!ev) 
        //     {
        //         continue;
        //     }
        //     event.handler(ev);
        //     free(ev);
        // }   
    }
}

void /**
 *INITIALIZES AN EWMH CONNECTION 
 */
ewmh_init()
{
    if (!(ewmh = static_cast<xcb_ewmh_connection_t *>(calloc(1, sizeof(xcb_ewmh_connection_t)))))
    {
        LOG_error("ewmh faild to initialize");
        wm::kill_session();
    }    
    
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, ewmh);
    
    if (!(xcb_ewmh_init_atoms_replies(ewmh, cookie, 0)))
    {
        LOG_error("xcb_ewmh_init_atoms_replies:faild")
        exit(1);
    }

    const char * str = "XCBWM";
    XCBwm::check_err
    (
        conn, 
        xcb_ewmh_set_wm_name
        (
            ewmh, 
            screen->root, 
            strlen(str), 
            str
        ), 
        __func__, 
        "xcb_ewmh_set_wm_name"
    );
}

void
make_desktop(const uint16_t & n)
{
    desktop * d = new desktop;
    d->desktop  = n;
    d->width    = screen->width_in_pixels;
    d->height   = screen->height_in_pixels;
    cur_d       = d;
    desktop_list.push_back(d);
}

void
draw_text(const char * str , const COLOR & text_color, const COLOR & bg_color, const xcb_window_t & win, const int16_t & x, const int16_t & y)
{
    xcb_gcontext_t gc = xcb_generate_id(conn);

    // Load font
    const char * font_name = "7x14";
    xcb_font_t font = xcb_generate_id(conn);
    xcb_open_font
    (
        conn, 
        font, 
        strlen(font_name), 
        font_name
    );

    xcb_create_gc
    (
        conn, 
        gc, 
        win, 
        XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT, 
        (const uint32_t[3])
        {
            color::get(text_color), 
            color::get(bg_color),
            font
        }
    );

    // Draw text
    xcb_image_text_8
    (
        conn, 
        strlen(str), 
        win, 
        gc, 
        x, 
        y, 
        str
    );

    // Flush the connection
    xcb_flush(conn);
}

void 
configureRootWindow()
{
    set_win_color
    (
        screen->root, 
        DARK_GREY
    );

    // APPLY THE EVENT MASKS TO THE ROOT WINDOW
    xcb_change_window_attributes
    (
        conn,
        screen->root,
        XCB_CW_EVENT_MASK,
        (const uint32_t[1])
        {
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT    | 
            XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY          | 
            XCB_EVENT_MASK_ENTER_WINDOW                 | 
            XCB_EVENT_MASK_LEAVE_WINDOW                 | 
            XCB_EVENT_MASK_STRUCTURE_NOTIFY             |
            XCB_EVENT_MASK_BUTTON_PRESS                 |
            XCB_EVENT_MASK_BUTTON_RELEASE               |
            XCB_EVENT_MASK_POINTER_MOTION               |
            XCB_EVENT_MASK_KEY_PRESS                    |
            XCB_EVENT_MASK_FOCUS_CHANGE                 |
            XCB_EVENT_MASK_KEY_RELEASE
        }
    );

    // CLEAR THE ROOT WINDOW TO APPLY THE CHANGES
    xcb_clear_area
    (
        conn, 
        0,
        screen->root,
        0, 
        0,
        screen->width_in_pixels,
        screen->height_in_pixels
    );

    // FLUSH TO MAKE X SERVER HANDEL REQUEST NOW
    xcb_flush(conn);
}

bool 
setSubstructureRedirectMask() 
{
    // ATTEMPT TO SET THE SUBSTRUCTURE REDIRECT MASK
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked
    (
        conn,
        screen->root,
        XCB_CW_EVENT_MASK,
        (const uint32_t[1])
        {
            XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        }
    );

    // CHECK IF ANOTHER WINDOW MANAGER IS RUNNING
    xcb_generic_error_t * error = xcb_request_check(conn, cookie);
    if (error) 
    {
        LOG_error("Error: Another window manager is already running or failed to set SubstructureRedirect mask.") 
        free(error);
        return false;
    }
    return true;
}

void 
setDefaultCursor() 
{
    xcb_cursor_context_t *ctx;
    if (xcb_cursor_context_new(conn, screen, &ctx) < 0) 
    {
        LOG_error("Failed to create cursor context")
        return;
    }

    xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, "left_ptr");
    if (!cursor) 
    {
        LOG_error("Failed to load cursor")
        xcb_cursor_context_free(ctx);
        return;
    }

    xcb_change_window_attributes
    (
        conn,
        screen->root, 
        XCB_CW_CURSOR, 
        (const uint32_t[1])
        {
            cursor
        }
    );
    xcb_cursor_context_free(ctx);
}

const int8_t 
setup_wm()
{
    conn = xcb_connect(nullptr, nullptr);
    
    if (xcb_connection_has_error(conn)) 
    {
        LOG_error("Error opening XCB connection")
        return -1;
    }

    setup = xcb_get_setup(conn);
    iter = xcb_setup_roots_iterator(setup);
    screen = iter.data;

    setSubstructureRedirectMask(); 
    configureRootWindow();
    setDefaultCursor();

    ewmh_init();

    /* 
        MAKE ('5') DESKTOPS 
        
        THIS IS JUST FOR 
        DEBUGING AND TESTING 
        IN THE FUTURE I WILL IMPLEMENT
        A WAY TO ADD AND REMOVE 
        DESKTOPS DURING RUNTIME
     */
    make_desktop(1);
    make_desktop(2);
    make_desktop(3);
    make_desktop(4);
    make_desktop(5);

    move_desktop(1);
    Compositor compositor(conn, screen);
    gCompositor = &compositor;
    return 0;
}

int
main() 
{
    LOG_start()
    const int8_t & err = setup_wm();
    if (err != 0)
    {
        return err;
    }
    run();
    xcb_disconnect(conn);
    return 0;
}