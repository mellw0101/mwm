#ifndef DOCK_HPP
#define DOCK_HPP

#include <X11/X.h>
#include <vector>
#include <vector>
#include <dirent.h>
#include <sys/types.h>

#include "Log.hpp"
#include "buttons.hpp"
#include "contex_meny.hpp"
#include "launcher.hpp"
#include "file.hpp"

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
    ;

    public: // public methods 
        void
        init()
        {
            main_window.create_default(screen->root, 0, 0, width, height);
            setup_dock();
            configure_context_menu();
            make_apps();
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
            context_menu.addEntry("test with nullptr", nullptr);
            context_menu.addEntry("test with nullptr", nullptr);
            context_menu.addEntry("test with nullptr", nullptr);
            context_menu.addEntry("test with nullptr", nullptr);
            context_menu.addEntry("test with nullptr", nullptr);
            context_menu.addEntry("test with nullptr", nullptr);
        }

        void
        make_apps()
        {
            for (const auto & app : apps)
            {
                buttons.add
                (
                    app,
                    [app, this] () 
                    {
                        {
                            launcher.program((char *) app);
                        }
                    }    
                );
                buttons.list[buttons.index()].create(main_window, ((buttons.index() * width) + 2), 2, (width - 4), (height - 4), BLACK);
                buttons.list[buttons.index()].put_icon_on_button();
            }
            calc_size_pos();
        }
    ;
};

#endif // DOCK_HPP