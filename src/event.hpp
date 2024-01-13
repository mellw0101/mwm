#ifndef EVENT_HPP
#define EVENT_HPP
#include <cstdlib>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
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
#include "Log.hpp"
#include "defenitions.hpp"
#include "structs.hpp"
#include "window.hpp"
#include "client.hpp"

class Event
{
    public: // constructor and destructor 
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
    ;

    public: // methods 
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
                {;
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
                case XCB_LEAVE_NOTIFY:
                {
                    leave_notify_handler(ev);
                    break;
                }
                case XCB_MOTION_NOTIFY:
                {
                    motion_notify_handler(ev);
                    break;
                }
            }
        }
    ;

    private: // variabels 
        xcb_key_symbols_t * keysyms;
        /*
            VARIABELS TO STORE KEYCODES
         */ 
        xcb_keycode_t t{}, q{}, f{}, f11{}, n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{}, l_arrow{}, u_arrow{}, d_arrow{}, tab{}, k{}; 
    ;

    private: // helper functions 
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
    ;

    private: // event handling functions 
        void 
        key_press_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
            
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
                        launcher->program((char *) "/usr/bin/konsole");
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
                        wm->quit(0);
                        break;
                    }
                }
            }

            /*
                CHECK IF 'F11' WAS PRESSED
                AND IF SO TOGGLE FULLSCREEN
             */ 
            if (e->detail == f11)
            {
                client * c = wm->client_from_window(& e->event);
                max_win(c, max_win::EWMH_MAXWIN);
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
                        change_desktop::teleport_to(1);
                        break;
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
                        change_desktop::teleport_to(2);
                        break;
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
                        change_desktop::teleport_to(3);
                        break;
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
                        change_desktop::teleport_to(4);
                        break;
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
                        change_desktop::teleport_to(5);
                        break;
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
				        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::NEXT);
                        break;
                    }
                    case SUPER:
                    {
                        client * c = wm->client_from_window(& e->event);
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
				        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::PREV);
                        break;
                    }
                    case SUPER:
                    {
                        client * c = wm->client_from_window(& e->event);
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
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::DOWN);
                        return;
                        break;
                    ;
                }
            }

            if (e->detail == u_arrow)
            {
                switch (e->state) 
                {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::UP);
                        break;
                    ;
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
                        wm->cycle_focus();
                        break;
                    ;
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
                        client * c = wm->client_from_window(& e->event);
                        break;
                    ;
                }
            }
        }

        void 
        map_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_map_notify_event_t *>(ev);
            client * c = wm->client_from_window(& e->window);
            if (c)
            {
                c->update();
            }
        }
        
        void 
        map_req_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_map_request_event_t *>(ev);
            wm->manage_new_client(e->window);
        }
        
        void 
        button_press_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
            client * c;

            if (BORDER_SIZE == 0)
            {
                c = wm->client_from_pointer(10);
                if (c)
                {
                    if (e->detail == L_MOUSE_BUTTON)
                    {
                        c->raise();
                        resize_client::no_border(c, 0, 0);
                        wm->focus_client(c);
                    }
                    return;
                }
            }
            
            // dock
            if (e->detail == L_MOUSE_BUTTON)
            {
                dock->buttons.run_action(e->event);
            }
            
            if (e->event == dock->main_window)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    dock->context_menu.show();
                    return;
                }    
            }

            if (e->event == wm->root)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    wm->context_menu->show();
                    return;
                }
            }

            c = wm->client_from_any_window(& e->event);
            if (!c)
            {
                // log_error("c == null");
                return;
            }
            
            if (e->detail == L_MOUSE_BUTTON)
            {
                if (e->event == c->win)
                {
                    switch (e->state) 
                    {
                        case ALT:
                            c->raise();
                            mv_client(c, e->event_x, e->event_y + 20);
                            wm->focus_client(c);
                            break;
                        ;
                    }
                    c->raise();
                    wm->focus_client(c);
                    return;
                }

                if (e->event == c->titlebar)
                {
                    c->raise();
                    mv_client(c, e->event_x, e->event_y);
                    wm->focus_client(c);
                    return;
                }

                if (e->event == c->close_button)
                {
                    win_tools::close_button_kill(c);
                    return;
                }

                if (e->event == c->max_button)
                {
                    client * c = wm->client_from_any_window(& e->event);
                    max_win(c, max_win::BUTTON_MAXWIN);
                    return;
                }

                if (e->event == c->border.left)
                {
                    resize_client::border(c, edge::LEFT);
                    return;
                }

                if (e->event == c->border.right)
                {
                    resize_client::border(c, edge::RIGHT);
                    return;
                }

                if (e->event == c->border.top)
                {
                    resize_client::border(c, edge::TOP);
                    return;
                }

                if (e->event == c->border.bottom)
                {
                    resize_client::border(c, edge::BOTTOM_edge);
                }

                if (e->event == c->border.top_left)
                {
                    resize_client::border(c, edge::TOP_LEFT);
                    return;
                }

                if (e->event == c->border.top_right)
                {
                    resize_client::border(c, edge::TOP_RIGHT);
                    return;
                }

                if (e->event == c->border.bottom_left)
                {
                    resize_client::border(c, edge::BOTTOM_LEFT);
                    return;
                }

                if (e->event == c->border.bottom_right)
                {
                    resize_client::border(c, edge::BOTTOM_RIGHT);
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
                        c->raise();
                        resize_client(c, 0);
                        wm->focus_client(c);
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
            client * c = wm->client_from_window( & e->event);
            if (c)
            {
                c->win.ungrab_button({ { L_MOUSE_BUTTON, NULL } });
                c->raise();
                c->win.set_active_EWMH_window();
                focused_client = c;
            }
        }

        void
        focus_out_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_out_event_t *>(ev);
            
            client * c = wm->client_from_window(& e->event);
            if (!c)
            {
                return;
            }
            
            c->win.grab_button({ { L_MOUSE_BUTTON, NULL } });
        }

        void
        destroy_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_destroy_notify_event_t *>(ev);
            client * c = wm->client_from_any_window(& e->window);
            int result = wm->send_sigterm_to_client(c);
            if (result == -1)
            {
                log_error("send_sigterm_to_client: failed");
            }
        }

        void
        unmap_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_unmap_notify_event_t *>(ev);
            
            client * c = wm->client_from_window(& e->window);
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
        }

        void
        leave_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_leave_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
        }

        void 
        motion_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
            // log_info(e->event_x);
        }
    ;
};

#endif