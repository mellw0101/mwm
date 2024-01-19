#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <cstdlib>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_xrm.h>
#include <xcb/render.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <xcb/xcb.h>
#include <unistd.h>     // For fork() and exec()
#include <sys/wait.h>   // For waitpid()
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xcb/xcb_cursor.h> /* For cursor */
#include <xcb/xcb_icccm.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <functional>
#include <algorithm>
#include <png.h>
#include <xcb/xcb_image.h>
#include <Imlib2.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <X11/Xauth.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <map>

#include "Log.hpp"
#include "contex_meny.hpp"
#include "defenitions.hpp"
#include "launcher.hpp"
#include "event_handler.hpp"
#include "client.hpp"
#include "desktop.hpp"
#include "pointer.hpp"
#include "structs.hpp"



class Key_Codes
{
    public: // constructor and destructor
        Key_Codes() 
        : keysyms(nullptr) {}

        ~Key_Codes()
        {
            free(keysyms);
        }
    ;
    public: // methods
        void init()
        {
            keysyms = xcb_key_symbols_alloc(conn);
            if (keysyms)
            {
                std::map<uint32_t, xcb_keycode_t *> key_map = 
                {
                    { A,            &a         },
                    { B,            &b         },
                    { C,            &c         },
                    { D,            &d         },
                    { E,            &e         },
                    { F,            &f         },
                    { G,            &g         },
                    { H,            &h         },
                    { I,            &i         },
                    { J,            &j         },
                    { K,            &k         },
                    { L,            &l         },
                    { M,            &m         },
                    { _N,           &n         },
                    { O,            &o         },
                    { P,            &p         },
                    { Q,            &q         },
                    { R,            &r         },
                    { S,            &s         },
                    { T,            &t         },
                    { U,            &u         },
                    { V,            &v         },
                    { W,            &w         },
                    { _X,           &x         },
                    { _Y,           &y         },
                    { Z,            &z         },

                    { SPACE_BAR,    &space_bar },
                    { ENTER,        &enter     },
                    { DELETE,       &_delete   },

                    { F11,          &f11       },
                    { N_1,          &n_1       },
                    { N_2,          &n_2       },
                    { N_3,          &n_3       },
                    { N_4,          &n_4       },
                    { N_5,          &n_5       },
                    { R_ARROW,      &r_arrow   },
                    { L_ARROW,      &l_arrow   },
                    { U_ARROW,      &u_arrow   },
                    { D_ARROW,      &d_arrow   },
                    { TAB,          &tab       },
                    { K,            &k         },

                };
                
                for (auto &pair : key_map) 
                {
                    xcb_keycode_t * keycode = xcb_key_symbols_get_keycode(keysyms, pair.first);
                    if (keycode) 
                    {
                        * (pair.second) = * keycode;
                        free(keycode);
                    }
                }
            }
        }
    ;
    public: // variabels
        xcb_keycode_t 
            a{},
            b{},
            c{},
            d{},
            e{},
            f{},
            g{},
            h{},
            i{},
            j{},
            k{},
            l{},
            m{},
            n{},
            o{},
            p{},
            q{},
            r{},
            s{},
            t{},
            u{},
            v{},
            w{},
            x{},
            y{},
            z{},

            space_bar{},
            enter{},

            f11{},
            n_1{},
            n_2{},
            n_3{},
            n_4{},
            n_5{},
            r_arrow{},
            l_arrow{},
            u_arrow{},
            d_arrow{},
            tab{},
            _delete{}
        ;
    ;
    private: // variabels
        xcb_key_symbols_t * keysyms;
    ;
};
class Window_Manager
{
    public: // constructor
        Window_Manager() {}
    ;
    public: // variabels 
        context_menu * context_menu = nullptr;
        window root;
        Launcher launcher;
        std::vector<client *> client_list;
        std::vector<desktop *> desktop_list;
        client * focused_client = nullptr;
        desktop * cur_d = nullptr;
        Logger log;
        pointer pointer;
        win_data data;
        Key_Codes key_codes;
    ;
    public: // methods 
        public: // main methods 
            void init()
            {
                _conn(nullptr, nullptr);
                _setup();
                _iter();
                _screen();
                root = screen->root;
                root.width(screen->width_in_pixels);
                root.height(screen->height_in_pixels);
                setSubstructureRedirectMask();
                configure_root();
                _ewmh();

                key_codes.init();

                event_handler = new Event_Handler();

                create_new_desktop(1);
                create_new_desktop(2);
                create_new_desktop(3);
                create_new_desktop(4);
                create_new_desktop(5);

                context_menu = new class context_menu();
                context_menu->add_entry
                (   
                    "konsole",
                    [this]() 
                    {
                        launcher.program((char *) "konsole");
                    }
                );
                context_menu->init();
            }
            void launch_program(char * program)
            {
                if (fork() == 0) 
                {
                    setsid();
                    execvp(program, (char *[]) { program, NULL });
                }
            }
            void quit(const int & status)
            {
                xcb_flush(conn);
                delete_client_vec(client_list);
                delete_desktop_vec(desktop_list);
                xcb_ewmh_connection_wipe(ewmh);
                xcb_disconnect(conn);
                exit(status);
            }
        ;
        public: // client methods 
            public: // focus methods 
                void focus_client(client * c)
                {
                    if (!c)
                    {
                        log_error("c is null");
                        return;
                    }

                    focused_client = c;
                    c->focus();
                }
                void cycle_focus()
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
                                focus_client(c);
                                return;  
                            }
                        }
                    }
                }
            ;
            public: // client fetch methods
                client * client_from_window(const xcb_window_t * window) 
                {
                    for (const auto & c : client_list) 
                    {
                        if (* window == c->win) 
                        {
                            return c;
                        }
                    }
                    return nullptr;
                }
                client * client_from_any_window(const xcb_window_t * window) 
                {
                    for (const auto & c : client_list) 
                    {
                        if (* window == c->win 
                         || * window == c->frame 
                         || * window == c->titlebar 
                         || * window == c->close_button 
                         || * window == c->max_button 
                         || * window == c->min_button 
                         || * window == c->border.left 
                         || * window == c->border.right 
                         || * window == c->border.top 
                         || * window == c->border.bottom
                         || * window == c->border.top_left
                         || * window == c->border.top_right
                         || * window == c->border.bottom_left
                         || * window == c->border.bottom_right) 
                        {
                            return c;
                        }
                    }
                    return nullptr;
                }
                client * client_from_pointer(const int & prox)
                {
                    const uint32_t & x = pointer.x();
                    const uint32_t & y = pointer.y();
                    for (const auto & c : cur_d->current_clients)
                    {
                        // LEFT EDGE OF CLIENT
                        if (x > c->x - prox && x <= c->x)
                        {
                            return c;
                        }

                        // RIGHT EDGE OF CLIENT
                        if (x >= c->x + c->width && x < c->x + c->width + prox)
                        {
                            return c;
                        }

                        // TOP EDGE OF CLIENT
                        if (y > c->y - prox && y <= c->y)
                        {
                            return c;
                        }

                        // BOTTOM EDGE OF CLIENT
                        if (y >= c->y + c->height && y < c->y + c->height + prox)
                        {
                            return c;
                        }
                    }
                    return nullptr;
                }
                std::map<client *, edge> get_client_next_to_client(client * c, edge c_edge)
                {
                    std::map<client *, edge> map;
                    for (client * c2 : cur_d->current_clients)
                    {
                        if (c == c2)
                        {
                            continue;
                        }

                        if (c_edge == edge::LEFT)
                        {
                            if (c->x == c2->x + c2->width)
                            {
                                map[c2] = edge::RIGHT;
                                return map;
                            }
                        }
                        
                        if (c_edge == edge::RIGHT)
                        {
                            if (c->x + c->width == c2->x)
                            {
                                map[c2] = edge::LEFT;
                                return map;
                            }
                        }
                        
                        if (c_edge == edge::TOP)
                        {
                            if (c->y == c2->y + c2->height)
                            {
                                map[c2] = edge::BOTTOM_edge;
                                return map;
                            }
                        }

                        if (c_edge == edge::BOTTOM_edge)
                        {
                            if (c->y + c->height == c2->y)
                            {
                                map[c2] = edge::TOP;
                                return map;
                            }
                        }
                    }

                    map[nullptr] = edge::NONE;
                    return map;
                }
                edge get_client_edge_from_pointer(client * c, const int & prox)
                {
                    const uint32_t & x = pointer.x();
                    const uint32_t & y = pointer.y();

                    const uint32_t & top_border = c->y;
                    const uint32_t & bottom_border = (c->y + c->height);
                    const uint32_t & left_border = c->x;
                    const uint32_t & right_border = (c->x + c->width);

                    // TOP EDGE OF CLIENT
                    if (((y > top_border - prox) && (y <= top_border))
                     && ((x > left_border + prox) && (x < right_border - prox)))
                    {
                        return edge::TOP;
                    }

                    // BOTTOM EDGE OF CLIENT
                    if (((y >= bottom_border) && (y < bottom_border + prox))
                     && ((x > left_border + prox) && (x < right_border - prox)))
                    {
                        return edge::BOTTOM_edge;
                    }

                    // LEFT EDGE OF CLIENT
                    if (((x > left_border) - prox && (x <= left_border))
                     && ((y > top_border + prox) && (y < bottom_border - prox)))
                    {
                        return edge::LEFT;
                    }

                    // RIGHT EDGE OF CLIENT
                    if (((x >= right_border) && (x < right_border + prox))
                     && ((y > top_border + prox) && (y < bottom_border - prox)))
                    {
                        return edge::RIGHT;
                    }

                    // TOP LEFT CORNER OF CLIENT
                    if (((x > left_border - prox) && x < left_border + prox)
                     && ((y > top_border - prox) && y < top_border + prox))
                    {
                        return edge::TOP_LEFT;
                    }

                    // TOP RIGHT CORNER OF CLIENT
                    if (((x > right_border - prox) && x < right_border + prox)
                     && ((y > top_border - prox) && y < top_border + prox))
                    {
                        return edge::TOP_RIGHT;
                    }

                    // BOTTOM LEFT CORNER OF CLIENT
                    if (((x > left_border - prox) && x < left_border + prox)
                     && ((y > bottom_border - prox) && y < bottom_border + prox))
                    {
                        return edge::BOTTOM_LEFT;
                    }

                    // BOTTOM RIGHT CORNER OF CLIENT
                    if (((x > right_border - prox) && x < right_border + prox)
                     && ((y > bottom_border - prox) && y < bottom_border + prox))
                    {
                        return edge::BOTTOM_RIGHT;
                    }

                    return edge::NONE;
                }
            ;
            void manage_new_client(const uint32_t & window)
            {
                client * c = make_client(window);
                if (!c)
                {
                    log_error("could not make client");
                    return;
                }

                c->win.x_y_width_height(c->x, c->y, c->width, c->height);
                c->win.map();
                c->win.grab_button(
                {
                    {   L_MOUSE_BUTTON,     ALT },
                    {   R_MOUSE_BUTTON,     ALT },
                    {   L_MOUSE_BUTTON,     0   }
                });
                c->win.grab_default_keys();
                c->update();
                c->make_decorations();
                uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
                c->win.apply_event_mask(& mask);

                c->update();
                focus_client(c);
            }
            client * make_internal_client(window window)
            {
                client * c = new client;
                c->win = window;
                c->x = window.x();
                c->y = window.y();
                c->width = window.width();
                c->height = window.height();
                
                c->make_decorations();
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                c->focus();

                return c;
            }
            int send_sigterm_to_client(client * c)
            {
                if (!c)
                {
                    return - 1;
                }

                c->win.unmap();
                c->close_button.unmap();
                c->max_button.unmap();
                c->min_button.unmap();
                c->titlebar.unmap();
                c->border.left.unmap();
                c->border.right.unmap();
                c->border.top.unmap();
                c->border.bottom.unmap();
                c->border.top_left.unmap();
                c->border.top_right.unmap();
                c->border.bottom_left.unmap();
                c->border.bottom_right.unmap();
                c->frame.unmap();

                c->win.kill();
                c->close_button.kill();
                c->max_button.kill();
                c->min_button.kill();
                c->titlebar.kill();
                c->border.left.kill();
                c->border.right.kill();
                c->border.top.kill();
                c->border.bottom.kill();
                c->border.top_left.kill();
                c->border.top_right.kill();
                c->border.bottom_left.kill();
                c->border.bottom_right.kill();
                c->frame.kill();
                
                remove_client(c);

                return 0;
            }
        ;
        public: // desktop methods 
            void
            create_new_desktop(const uint16_t & n)
            {
                desktop * d = new desktop;
                d->desktop  = n;
                d->width    = screen->width_in_pixels;
                d->height   = screen->height_in_pixels;
                cur_d       = d;
                desktop_list.push_back(d);
            }
        ;
    ;
    private: // variables 
        window start_window;
    ;
    private: // functions 
        private: // init functions 
            void 
            _conn(const char * displayname, int * screenp) 
            {
                conn = xcb_connect(displayname, screenp);
                check_conn();
            }

            void 
            _ewmh() 
            {
                if (!(ewmh = static_cast<xcb_ewmh_connection_t *>(calloc(1, sizeof(xcb_ewmh_connection_t)))))
                {
                    log_error("ewmh faild to initialize");
                    quit(1);
                }    
                
                xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms(conn, ewmh);
                
                if (!(xcb_ewmh_init_atoms_replies(ewmh, cookie, 0)))
                {
                    log_error("xcb_ewmh_init_atoms_replies:faild");
                    quit(1);
                }

                const char * str = "mwm";
                check_error
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
            _setup() 
            {
                setup = xcb_get_setup(conn);
            }

            void 
            _iter() 
            {
                iter = xcb_setup_roots_iterator(setup);
            }

            void 
            _screen() 
            {
                screen = iter.data;
            }

            bool
            setSubstructureRedirectMask() 
            {
                // ATTEMPT TO SET THE SUBSTRUCTURE REDIRECT MASK
                xcb_void_cookie_t cookie = xcb_change_window_attributes_checked
                (
                    conn,
                    root,
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
                    log_error("Error: Another window manager is already running or failed to set SubstructureRedirect mask."); 
                    free(error);
                    return false;
                }
                return true;
            }

            void 
            configure_root()
            {
                root.set_backround_color(DARK_GREY);
                uint32_t mask = 
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_ENTER_WINDOW |
                    XCB_EVENT_MASK_LEAVE_WINDOW |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_BUTTON_RELEASE |
                    XCB_EVENT_MASK_KEY_PRESS |
                    XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_FOCUS_CHANGE |
                    XCB_EVENT_MASK_POINTER_MOTION
                ; 
                root.apply_event_mask(& mask);
                root.clear();

                root.set_backround_png("/home/mellw/mwm_png/galaxy17.png");
                root.set_pointer(CURSOR::arrow);
            }
        ;
        private: // check functions 
            void
            check_error(const int & code)
            {
                switch (code) 
                {
                    case CONN_ERR:
                        log_error("Connection error.");
                        quit(CONN_ERR);
                        break;
                    ;
                    case EXTENTION_NOT_SUPPORTED_ERR:
                        log_error("Extension not supported.");
                        quit(EXTENTION_NOT_SUPPORTED_ERR);
                        break;
                    ;
                    case MEMORY_INSUFFICIENT_ERR:
                        log_error("Insufficient memory.");
                        quit(MEMORY_INSUFFICIENT_ERR);
                        break;
                    ;
                    case REQUEST_TO_LONG_ERR:
                        log_error("Request to long.");
                        quit(REQUEST_TO_LONG_ERR);
                        break;
                    ;
                    case PARSE_ERR:
                        log_error("Parse error.");
                        quit(PARSE_ERR);
                        break;
                    ;
                    case SCREEN_NOT_FOUND_ERR:
                        log_error("Screen not found.");
                        quit(SCREEN_NOT_FOUND_ERR);
                        break;
                    ;
                    case FD_ERR:
                        log_error("File descriptor error.");
                        quit(FD_ERR);
                        break;
                    ;
                }
            }

            void
            check_conn()
            {
                int status = xcb_connection_has_error(conn);
                check_error(status);
            }

            int
            cookie_error(xcb_void_cookie_t cookie , const char * sender_function)
            {
                xcb_generic_error_t * err = xcb_request_check(conn, cookie);
                if (err)
                {
                    log_error(err->error_code);
                    free(err);
                    return err->error_code;
                }
                return 0;
            }

            void
            check_error(xcb_connection_t * connection, xcb_void_cookie_t cookie , const char * sender_function, const char * err_msg)
            {
                xcb_generic_error_t * err = xcb_request_check(connection, cookie);
                if (err)
                {
                    log_error_code(err_msg, err->error_code);
                    free(err);
                }
            }
        ;
        int start_screen_window()
        {
            start_window.create_default(root, 0, 0, 0, 0);
            start_window.set_backround_color(DARK_GREY);
            start_window.map();
            return 0;
        }
        private: // delete functions 
            void delete_client_vec(std::vector<client *> & vec)
            {
                for (client * c : vec)
                {
                    send_sigterm_to_client(c);
                    xcb_flush(conn);                    
                }

                vec.clear();

                std::vector<client *>().swap(vec);
            }
            void delete_desktop_vec(std::vector<desktop *> & vec)
            {
                for (desktop * d : vec)
                {
                    delete_client_vec(d->current_clients);
                    delete d;
                }

                vec.clear();

                std::vector<desktop *>().swap(vec);
            }
            template <typename Type> 
            static void delete_ptr_vector(std::vector<Type *>& vec) 
            {
                for (Type * ptr : vec) 
                {
                    delete ptr;
                }
                vec.clear();

                std::vector<Type *>().swap(vec);
            }
            void remove_client(client * c)
            {
                client_list.erase(std::remove(client_list.begin(), client_list.end(), c), client_list.end());
                cur_d->current_clients.erase(std::remove(cur_d->current_clients.begin(), cur_d->current_clients.end(), c), cur_d->current_clients.end());
                delete c;
            }
            void remove_client_from_vector(client * c, std::vector<client *> & vec)
            {
                if (!c)
                {
                    log_error("client is nullptr.");
                }
                vec.erase(std::remove(vec.begin(), vec.end(), c), vec.end());
                delete c;
            }
        ;
        private: // client functions
            client * make_client(const uint32_t & window) 
            {
                client * c = new client;
                if (!c) 
                {
                    log_error("Could not allocate memory for client");
                    return nullptr;
                }

                c->win    = window;
                get_window_parameters(window, &c->x, &c->y, &c->width, &c->height, &c->depth);
                c->desktop = cur_d->desktop;

                for (int i = 0; i < 256; ++i)
                {
                    c->name[i] = '\0';
                }

                int i = 0;
                char * name = c->win.property("_NET_WM_NAME");
                while(name[i] != '\0' && i < 255)
                {
                    c->name[i] = name[i];
                    ++i;
                }
                c->name[i] = '\0';
                free(name);
                
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                return c;
            }
            
        ;
        private: // window functions
            void getWindowParameters(const uint32_t & window) 
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);

                if (geometry_reply != NULL) 
                {
                    log_info("Window Parameters");
                    log_info(std::to_string(geometry_reply->x));
                    log_info(std::to_string(geometry_reply->y));
                    log_info(std::to_string(geometry_reply->width));
                    log_info(std::to_string(geometry_reply->height));

                    free(geometry_reply);
                } 
                else 
                {
                    std::cerr << "Unable to get window geometry." << std::endl;
                }
            }
            void get_window_parameters(const uint32_t &window, int16_t *x, int16_t *y, uint16_t *width, uint16_t *height, uint8_t *depth)
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);
                if (!geometry_reply) 
                {
                    log_error("unable to get window parameters for window: " + std::to_string(window));
                }

                *x = geometry_reply->x;
                *y = geometry_reply->y;
                *width = geometry_reply->width;
                *height = geometry_reply->height;
                *depth = geometry_reply->depth;

                free(geometry_reply);
            }
            int16_t get_window_x(const uint32_t & window) 
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);

                if (!geometry_reply) 
                {
                    log_error("Unable to get window geometry.");
                    return (screen->width_in_pixels / 2);
                } 
            
                int16_t x = geometry_reply->x;
                free(geometry_reply);
                return x;
            }
            int16_t get_window_y(const uint32_t & window) 
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);

                if (!geometry_reply) 
                {
                    log_error("Unable to get window geometry.");
                    return (screen->height_in_pixels / 2);
                } 
            
                int16_t y = geometry_reply->y;
                free(geometry_reply);
                return y;
            }
        ;
    ;
};
static Window_Manager * wm;

#endif // WINDOW_MANAGER_HPP