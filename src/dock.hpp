#ifndef DOCK_HPP
#define DOCK_HPP

#include <X11/X.h>
#include <cstddef>
#include <cstdint>
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

class search_window {
    public: // variabels
        window main_window;
        std::string search_string = ""
    ;
    public: // mehods
        void create(const uint32_t & parent_window, const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height) {
            main_window.create_default(parent_window, x, y, width, height);
            main_window.set_backround_color(BLACK);
            uint32_t mask =  XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE;
            main_window.apply_event_mask(& mask);
            main_window.map();
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            main_window.grab_keys_for_typing();
            main_window.grab_default_keys();

            for (int i = 0; i < 7; ++i) {
                window entry;
                entry.create_default(main_window, 0, (20 * (i + 1)) , 140, 20);
                entry.set_backround_color(BLACK);
                entry.raise();
                entry.map();
                entry_list.push_back(entry);
            }
            
            main_window.raise();
            main_window.focus_input();
        }
        void add_enter_action(std::function<void()> enter_action) {
            enter_function = enter_action;
        }
        void init() {
            setup_events();
        }
        void clear_search_string() {
            search_string = "";
        }
        std::string string() {
            return search_string;
        }
    ;
    private: // functions
        void setup_events() {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->event == main_window) {
                    if (e->detail == wm->key_codes.a) {
                        if (e->state == SHIFT) {
                            search_string += "A";
                        } else {
                            search_string += "a";
                        } 
                    }
                    if (e->detail == wm->key_codes.b) {
                        if (e->state == SHIFT) {
                            search_string += "B";
                        } else {
                            search_string += "b"; 
                        }
                    }
                    if (e->detail == wm->key_codes.c) {
                        if (e->state == SHIFT) {
                            search_string += "C";
                        } else {
                            search_string += "c"; 
                        }
                    }
                    if (e->detail == wm->key_codes.d) {
                        if (e->state == SHIFT) {
                            search_string += "D";
                        } else {
                            search_string += "d";
                        }
                    }
                    if (e->detail == wm->key_codes.e) {
                        if (e->state == SHIFT) {
                            search_string += "E";
                        } else {
                            search_string += "e";
                        }
                    }
                    if (e->detail == wm->key_codes.f) {
                        if (e->state == SHIFT) {
                            search_string += "F";
                        } else {
                            search_string += "f";
                        }
                    }
                    if (e->detail == wm->key_codes.g) {
                        if (e->state == SHIFT) {
                            search_string += "G";
                        } else {
                            search_string += "g";
                        }
                    }
                    if (e->detail == wm->key_codes.h) {
                        if (e->state == SHIFT) {
                            search_string += "H";
                        } else {
                            search_string += "h";
                        }
                    }
                    if (e->detail == wm->key_codes.i) {
                        if (e->state == SHIFT) {
                            search_string += "I";
                        } else {
                            search_string += "i";
                        }
                    }
                    if (e->detail == wm->key_codes.j) {
                        if (e->state == SHIFT) {
                            search_string += "J";
                        } else {
                            search_string += "j";
                        }
                    }
                    if (e->detail == wm->key_codes.k) {
                        if (e->state == SHIFT) {
                            search_string += "K";
                        } else {
                            search_string += "k";
                        }
                    }
                    if (e->detail == wm->key_codes.l) {
                        if (e->state == SHIFT) {
                            search_string += "L";
                        } else {
                            search_string += "l";
                        }
                    }
                    if (e->detail == wm->key_codes.m) {
                        if (e->state == SHIFT) {
                            search_string += "M";
                        } else {
                            search_string += "m";
                        }
                    }
                    if (e->detail == wm->key_codes.n) {
                        if (e->state == SHIFT) {
                            search_string += "N";
                        } else {
                            search_string += "n";
                        }
                    }
                    if (e->detail == wm->key_codes.o) {
                        if (e->state == SHIFT) {
                            search_string += "O";
                        } else {
                            search_string += "o";
                        }
                    }
                    if (e->detail == wm->key_codes.p) {
                        if (e->state == SHIFT) {
                            search_string += "P";
                        } else {
                            search_string += "p";
                        }
                    }
                    if (e->detail == wm->key_codes.q) {
                        if (e->state == SHIFT) {
                            search_string += "Q";
                        } else {
                            search_string += "q";
                        }
                    }
                    if (e->detail == wm->key_codes.r) {
                        if (e->state == SHIFT) {
                            search_string += "R";
                        } else {
                            search_string += "r";
                        }
                    }
                    if (e->detail == wm->key_codes.s) {
                        if (e->state == SHIFT) {
                            search_string += "S";
                        } else {
                            search_string += "s";
                        }
                    }
                    if (e->detail == wm->key_codes.t) {
                        if (e->state == SHIFT) {
                            search_string += "T";
                        } else {
                            search_string += "t";
                        }
                    }
                    if (e->detail == wm->key_codes.u) {
                        if (e->state == SHIFT) {
                            search_string += "U";
                        } else {
                            search_string += "u";
                        }
                    }
                    if (e->detail == wm->key_codes.v) {
                        if (e->state == SHIFT) {
                            search_string += "V";
                        } else {
                            search_string += "v";
                        }
                    }
                    if (e->detail == wm->key_codes.w) {
                        if (e->state == SHIFT) {
                            search_string += "W";
                        } else {
                            search_string += "w";
                        }
                    }
                    if (e->detail == wm->key_codes.x) {
                        if (e->state == SHIFT) {
                            search_string += "X";
                        } else {
                            search_string += "x";
                        }
                    }
                    if (e->detail == wm->key_codes.y) {
                        if (e->state == SHIFT) {
                            search_string += "Y";
                        } else {
                            search_string += "y";
                        }
                    }
                    if (e->detail == wm->key_codes.z) {
                        if (e->state == SHIFT) {
                            search_string += "Z";
                        } else {
                            search_string += "z";
                        }
                    }

                    if (e->detail == wm->key_codes.space_bar) {
                        search_string += " ";
                    }
                    if (e->detail == wm->key_codes._delete) {
                        if (search_string.length() > 0) {
                            search_string.erase(search_string.length() - 1);
                            main_window.clear();
                        }
                    }
                    if (e->detail == wm->key_codes.enter) {
                        if (enter_function) {
                            enter_function();
                        }
                        search_string = "";
                        main_window.clear();
                    }

                    draw_text();
                }
            });
            
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == main_window)
                {
                    main_window.raise();
                    main_window.focus_input();
                }
            });
        }
        void draw_text() {
            main_window.draw_text(search_string.c_str(), WHITE, BLACK, "7x14", 2, 14);
            if (search_string.length() > 0)
            {
                results = file.search_for_binary(search_string.c_str());
                int entry_list_size = results.size(); 
                if (results.size() > 7)
                {
                    entry_list_size = 7;
                }
                main_window.height(20 * entry_list_size);
                xcb_flush(conn);
                for (int i = 0; i < entry_list_size; ++i)
                {
                    entry_list[i].draw_text(results[i].c_str(), WHITE, BLACK, "7x14", 2, 14);
                }
            }
        }
    ;
    private: // variables
        std::function<void()> enter_function;
        File file;
        std::vector<std::string> results;
        std::vector<window> entry_list;
    ;
};
class Mwm_Runner {
    public: // variabels
        window main_window;
        search_window search_window;
        uint32_t BORDER = 2;
        Launcher launcher;
    ;
    public: // methods
        void init() {
            main_window.create_default(
                screen->root,
                (screen->width_in_pixels / 2) - ((140 + (BORDER * 2)) / 2),
                0,
                140 + (BORDER * 2),
                400 + (BORDER * 2)
            );
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            main_window.set_backround_color(DARK_GREY);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            setup_events();
            search_window.create
            (
                main_window,
                2,
                2,
                main_window.width() - (BORDER * 2),
                main_window.height() - (BORDER * 2)
            );
            search_window.init();
            search_window.add_enter_action([this]()
            {
                launcher.program((char *) search_window.string().c_str());
                hide();
            });
        }
        void show() {
            main_window.raise();
            main_window.map();
            search_window.main_window.focus_input();
        }
    ;
    private: // functions
        void hide() {
            main_window.unmap();
            search_window.clear_search_string();
        }
        void setup_events() {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->detail == wm->key_codes.r) {
                    if (e->state == SUPER) {
                        show();
                    }
                }
            });

            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) 
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (!main_window.is_mapped())
                {
                    return;
                }
                if (e->event != main_window && e->event != search_window.main_window)
                {
                    hide();
                }
            });
        }
    ;
};
static Mwm_Runner * mwm_runner;
class add_app_dialog_window {
    public: // variabels
        window main_window;
        search_window search_window;
        client * c;
        buttons buttons;
        pointer pointer;
        Logger log;
    ;
    public: // methods
        void init() {
            create();
            configure_events();
        }
        void show() {
            create_client();
            search_window.create(main_window, DOCK_BORDER, DOCK_BORDER, (main_window.width() - (DOCK_BORDER * 2)), 20);
            search_window.init();
        }
        void add_enter_action(std::function<void()> enter_action) {
            enter_function = enter_action;
        }
    ;
    private: // functions
        void hide() {
            wm->send_sigterm_to_client(c);
        }
        void create() {
            main_window.create_default(screen->root, pointer.x(), pointer.y(), 300, 200);
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            main_window.grab_keys({ { Q, SHIFT | ALT } });
            main_window.set_backround_color(DARK_GREY);
        }
        void create_client() {
            main_window.x_y(pointer.x() - (main_window.width() / 2), pointer.y() - (main_window.height() / 2));
            c = wm->make_internal_client(main_window);
            c->x_y((pointer.x() - (c->width / 2)), (pointer.y() - (c->height / 2)));
            main_window.map();
            c->map();
        }
        void configure_events() {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == search_window.main_window)
                {
                    c->focus();
                }
            });
        }
    ;
    private: // variables
        std::function<void()> enter_function;
    ;
};
class File_App {
    public: // variabels
        window main_window;
        window left_side_window;

        client * c;
    ;
    public: // methods
        void init() {
            create_main_window();
            create_left_side_window();
            setup_events();
        }
    ;
    private: // functions
        void create_main_window() {
            int width = (screen->width_in_pixels / 2), height = (screen->height_in_pixels / 2);
            int x = ((screen->width_in_pixels / 2) - (width / 2)), y = ((screen->height_in_pixels / 2) - (height / 2));
            main_window.create_default(screen->root, x, y, width, height);

            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            if (!main_window.is_mask_active(XCB_EVENT_MASK_STRUCTURE_NOTIFY)) {
                log_error("could not apply XCB_EVENT_MASK_STRUCTURE_NOTIFY");
            }
            main_window.set_backround_color(BLUE);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            log_win("main_window: ", main_window);
        }
        void create_left_side_window() {
            left_side_window.create_default(main_window, 0, 0, 80, main_window.height());

            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            left_side_window.apply_event_mask(& mask);
            left_side_window.set_backround_color(DARK_GREY);
            left_side_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
        }
        void make_internal_client() {
            c = new client;
            c->win = main_window;
            c->x = main_window.x();
            c->y = main_window.y();
            c->width = main_window.width();
            c->height = main_window.height();
            c->make_decorations();
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            wm->client_list.push_back(c);
        }
        void launch() {
            main_window.raise();
            main_window.map();
            make_internal_client();
            left_side_window.raise();
            left_side_window.map();
        }
        void setup_events() {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->detail == wm->key_codes.f) {
                    if (e->state == SUPER) {
                        launch();
                    }
                }
            });
            event_handler->setEventCallback(XCB_CONFIGURE_NOTIFY, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_configure_notify_event_t *>(ev);
                if (e->window == c->border.left 
                 || e->window == c->border.right
                 || e->window == c->border.top
                 || e->window == c->border.bottom
                 || e->window == c->border.top_left 
                 || e->window == c->border.top_right 
                 || e->window == c->border.bottom_left 
                 || e->window == c->border.bottom_right)
                {
                    log_info("success");
                }
            });
        }
    ;
    private:
        Logger log;
    ;
};
static File_App * file_app;
class Dock {
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
        void init() {
            main_window.create_default(screen->root, 0, 0, width, height);
            setup_dock();
            configure_context_menu();
            make_apps();
            add_app_dialog_window.init();
            add_app_dialog_window.search_window.add_enter_action([this]()
            {
                launcher.program((char *) add_app_dialog_window.search_window.search_string.c_str());
            });
            context_menu.init();
            configure_events();
        }
        void add_app(const char * app_name) {
            if (!file.check_if_binary_exists(app_name)) {
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
        void calc_size_pos() {
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
        void setup_dock() {
            main_window.grab_button({ { R_MOUSE_BUTTON, NULL } });
            main_window.set_backround_color(DARK_GREY);
            calc_size_pos();
            main_window.map();
        }
        void configure_context_menu() {
            context_menu.add_entry("Add app", [this] () { add_app_dialog_window.show(); });
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
        }
        void make_apps() {
            for (const auto & app : apps) {
                buttons.add
                (app,
                [app, this] () 
                {
                    {
                        launcher.program((char *) app);
                    }
                });
                buttons.list[buttons.index()].create(
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
        void configure_events() {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == main_window)
                {
                    if (e->detail == R_MOUSE_BUTTON)
                    {
                        context_menu.show();
                    }
                }
                for (int i = 0; i < buttons.size(); ++i)
                {
                    if (e->event == buttons.list[i].window)
                    {
                        if (e->detail == L_MOUSE_BUTTON) 
                        {
                            buttons.list[i].activate();
                        }
                    }
                }
            });
        }
    ;
};

#endif // DOCK_HPP