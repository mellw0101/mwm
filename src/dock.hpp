#ifndef DOCK_HPP
#define DOCK_HPP

#include <X11/X.h>
#include <cstddef>
#include <vector>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <xcb/xcb.h>
#include <functional>
#include <xcb/xproto.h>

#include "Log.hpp"
#include "buttons.hpp"
#include "client.hpp"
#include "contex_meny.hpp"
#include "defenitions.hpp"
#include "launcher.hpp"
#include "file.hpp"
#include "pointer.hpp"
#include "structs.hpp"
#include "window.hpp"
#include "event_handler.hpp"
#include "window_manager.hpp"

class add_app_dialog_window
{
    public: // variabels
        window main_window;
        window search_window;
        client * c;
        buttons buttons;
        pointer pointer;
        Event_Handler ev_handler;
        Logger log;
    ;
    public: // methods
        void init()
        {
            create();
            configure_events();
        }
        void show()
        {
            create_client();
            create_search_window();
        }
    ;
    private: // functions
        void hide()
        {
            wm->send_sigterm_to_client(c);
        }
        void create()
        {
            main_window.create_default(screen->root, pointer.x(), pointer.y(), 300, 200);
            uint32_t mask =  XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            main_window.grab_keys({ { Q, SHIFT | ALT } });
            main_window.set_backround_color(DARK_GREY);
        }
        void create_client()
        {
            main_window.x_y(pointer.x() - (main_window.width() / 2), pointer.y() - (main_window.height() / 2));
            c = wm->make_internal_client(main_window);
            c->x_y((pointer.x() - (c->width / 2)), (pointer.y() - (c->height / 2)));
            main_window.map();
            c->map();
        }
        void create_search_window()
        {
            search_window.create_default(main_window, DOCK_BORDER, DOCK_BORDER, (main_window.width() - (DOCK_BORDER * 2)), 20);
            search_window.set_backround_color(BLACK);
            uint32_t mask =  XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE;
            main_window.apply_event_mask(& mask);
            search_window.map();
            search_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            search_window.grab_keys_for_typing();
            search_window.grab_keys
            ({
                { SPACE_BAR,    NULL        },
                { ENTER,        NULL        },
                { DELETE,       NULL        },

                { Q,            SHIFT | ALT }
            });
            
            search_window.raise();
            search_window.focus_input();
        }
        void configure_events()
        {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == search_window)
                {
                    c->focus();
                    search_window.raise();
                    search_window.focus_input();
                }
            });

            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) 
            {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->event == search_window)
                {
                    if (e->detail == wm->key_codes.a) 
                    { 
                        if (e->state == SHIFT) { search_string += "A"; }
                        else { search_string += "a"; }
                    }
                    if (e->detail == wm->key_codes.b) { search_string += "b"; }
                    if (e->detail == wm->key_codes.c) { search_string += "c"; }
                    if (e->detail == wm->key_codes.d) { search_string += "d"; }
                    if (e->detail == wm->key_codes.e) { search_string += "e"; }
                    if (e->detail == wm->key_codes.f) { search_string += "f"; }
                    if (e->detail == wm->key_codes.g) { search_string += "g"; }
                    if (e->detail == wm->key_codes.h) { search_string += "h"; }
                    if (e->detail == wm->key_codes.i) { search_string += "i"; }
                    if (e->detail == wm->key_codes.j) { search_string += "j"; }
                    if (e->detail == wm->key_codes.k) { search_string += "k"; }
                    if (e->detail == wm->key_codes.l) { search_string += "l"; }
                    if (e->detail == wm->key_codes.m) { search_string += "m"; }
                    if (e->detail == wm->key_codes.n) { search_string += "n"; }
                    if (e->detail == wm->key_codes.o) { search_string += "o"; }
                    if (e->detail == wm->key_codes.p) { search_string += "p"; }
                    if (e->detail == wm->key_codes.q) { search_string += "q"; }
                    if (e->detail == wm->key_codes.r) { search_string += "r"; }
                    if (e->detail == wm->key_codes.s) { search_string += "s"; }
                    if (e->detail == wm->key_codes.t) { search_string += "t"; }
                    if (e->detail == wm->key_codes.u) { search_string += "u"; }
                    if (e->detail == wm->key_codes.v) { search_string += "v"; }
                    if (e->detail == wm->key_codes.w) { search_string += "w"; }
                    if (e->detail == wm->key_codes.x) { search_string += "x"; }
                    if (e->detail == wm->key_codes.y) { search_string += "y"; }
                    if (e->detail == wm->key_codes.z) { search_string += "z"; }

                    if (e->detail == wm->key_codes.space_bar) { search_string += " "; }

                    if (e->detail == wm->key_codes._delete)
                    {
                        if (search_string.length() > 0)
                        {
                            search_string.erase(search_string.length() - 1);
                            search_window.clear();
                        }
                    }

                    if (e->detail == wm->key_codes.enter)
                    {
                        search_string = "";
                        search_window.clear();
                    }

                    search_window.draw_text(search_string.c_str(), WHITE, BLACK, "7x14", 2, 14);
                }
            });
        }
    ;
    private: // variable
        std::string search_string = "";
    ;
};
class Dock
{
    public: // constructor
        Dock() {}
    ;
    public: // public variables 
        context_menu context_menu;
        window main_window;
        buttons buttons;
        uint32_t x = 0, y = 0, width = 48, height = 48;

        add_app_dialog_window add_app_dialog_window;
    ;
    public: // public methods 
        void init()
        {
            main_window.create_default(screen->root, 0, 0, width, height);
            setup_dock();
            configure_context_menu();
            make_apps();
            add_app_dialog_window.init();
            context_menu.init();
        }
        void add_app(const char * app_name)
        {
            if (!file.check_if_binary_exists(app_name))
            {
                return;
            }
            apps.push_back(app_name);
        }
    ;
    private: // private variables
        std::vector<const char *> apps;
        Launcher launcher;
        Logger log;
        File file;
    ;
    private: // private methods
        void calc_size_pos()
        {
            int num_of_buttons = buttons.size();

            if (num_of_buttons == 0)
            {
                num_of_buttons = 1;
            }

            uint32_t calc_width = width * num_of_buttons;

            x = ((screen->width_in_pixels / 2) - (calc_width / 2));
            y = (screen->height_in_pixels - height);

            main_window.x_y_width_height(x, y, calc_width, height);
            xcb_flush(conn);
        }
        void setup_dock()
        {
            main_window.grab_button({{R_MOUSE_BUTTON, NULL}});
            main_window.set_backround_color(DARK_GREY);
            calc_size_pos();
            main_window.map();
        }
        void configure_context_menu()
        {
            context_menu.add_entry("Add app", [this] () { add_app_dialog_window.show(); });
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
        }
        void make_apps()
        {
            for (const auto & app : apps)
            {
                buttons.add
                (app,
                [app, this] () 
                {
                    {
                        launcher.program((char *) app);
                    }
                });
                buttons.list[buttons.index()].create
                (
                    main_window,
                    ((buttons.index() * width) + DOCK_BORDER),
                    DOCK_BORDER,
                    (width - (DOCK_BORDER * 2)),
                    (height - (DOCK_BORDER * 2)),
                    BLACK
                );
                buttons.list[buttons.index()].put_icon_on_button();
            }
            calc_size_pos();
        }
    ;
};

#endif // DOCK_HPP