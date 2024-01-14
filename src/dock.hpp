#ifndef DOCK_HPP
#define DOCK_HPP

#include <vector>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <xcb/xcb.h>
#include <functional>
#include <xcb/xproto.h>

#include "Log.hpp"
#include "buttons.hpp"
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
    public:
        window window;
        buttons buttons;
        pointer pointer;
        Event_Handler ev_handler;
        Logger log;
    ;
    public:
        void
        init()
        {
            window.create_default(screen->root, 0, 0, 300, 200);
            log_win("add_app_win: ", window);
            uint32_t mask =  XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            window.apply_event_mask(& mask);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.set_backround_color(DARK_GREY);
            window.raise();

            ev_handler.setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev) 
            {
                const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
                if (e->event == screen->root)
                {
                    hide();
                }
            });

            wm->event_handler.setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == window)
                {
                    hide();
                }
            });
        }

        void
        show()
        {
            window.x_y((pointer.x() - (window.width() / 2)), (pointer.y() - (window.height() / 2)));
            window.map();
            window.raise();
        }
    ;
    private:
        void
        hide()
        {
            window.unmap();
            window.kill();
        }
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
        void
        init()
        {
            main_window.create_default(screen->root, 0, 0, width, height);
            setup_dock();
            configure_context_menu();
            make_apps();
            add_app_dialog_window.init();
        }

        void 
        add_app(const char * app_name)
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
        void
        calc_size_pos()
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

        void
        setup_dock()
        {
            main_window.grab_button({{R_MOUSE_BUTTON, NULL}});
            main_window.set_backround_color(DARK_GREY);
            calc_size_pos();
            main_window.map();
        }

        void
        configure_context_menu()
        {
            context_menu.add_entry("Add app", [this] () { add_app_dialog_window.show(); });
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
        }

        void
        make_apps()
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
    ;
};

#endif // DOCK_HPP