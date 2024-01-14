#ifndef DOCK_HPP
#define DOCK_HPP

#include <filesystem>
#include <string>
#include <vector>

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
            if (!check_if_app_exists(app_name))
            {
                return;
            }
            apps.push_back(app_name);
        }
    ;

    private: // private variables
        std::vector<const char *> apps;
        Launcher launcher;
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
                buttons.list[buttons.size() - 1].create(main_window, ((buttons.size() - 1) * width) + 2, 2, width - 4, height - 4, BLACK);
                std::string icon_path = findIconFilePath(app);
                buttons.list[buttons.size() - 1].window.set_backround_png(icon_path.c_str());
            }
            calc_size_pos();
        }

        bool
        check_if_app_exists(const char * app_name)
        {
            std::string name = app_name;

            if (File::search("/usr/bin/" + name))
            {
                return true;
            }

            if (File::search("/usr/local/bin/" + name))
            {
                return true;
            }

            if (File::search("/usr/local/sbin/" + name))
            {
                return true;
            }

            if (File::search("/usr/sbin/" + name))
            {
                return true;
            }

            if (File::search("/sbin/" + name))
            {
                return true;
            }

            if (File::search("/bin/" + name))
            {
                return true;
            }

            return false;
        }

        std::string 
        findIconFilePath(const std::string& iconName) 
        {
            std::vector<std::string> iconSizes = {"256x256", "128x128", "64x64", "48x48", "32x32"};
            std::vector<std::string> imageExtensions = {"png"};
            std::string shortIconName = iconName.substr(iconName.find_last_of('/') + 1);

            std::string scalableDir = "/usr/share/icons/hicolor/scalable/apps/";
            std::string sizeDirBase = "/usr/share/icons/hicolor/";
            std::string breezeDarkDirBase = "/usr/share/icons/breeze-dark/actions/24/";

            auto checkExistsWildcard = [](const std::string& dir, const std::string& namePart, const std::vector<std::string>& extensions) -> std::string 
            {
                for (const auto& ext : extensions) 
                {
                    for (const auto& entry : std::filesystem::directory_iterator(dir)) 
                    {
                        if (!std::filesystem::is_regular_file(entry.status())) 
                        {
                            continue;
                        }
                        std::string filename = entry.path().filename().string();
                        if (filename.find(namePart) != std::string::npos && filename.find('.' + ext) != std::string::npos) 
                        {
                            return entry.path().string();
                        }
                    }
                }
                return "";
            };

            std::string filePath = checkExistsWildcard(scalableDir, shortIconName, imageExtensions);
            if (!filePath.empty()) return filePath;

            for (const auto& size : iconSizes) 
            {
                filePath = checkExistsWildcard(sizeDirBase + size + "/apps/", shortIconName, imageExtensions);
                if (!filePath.empty()) 
                {
                    return filePath;
                }    
            }

            filePath = checkExistsWildcard(breezeDarkDirBase, shortIconName, imageExtensions);
            if (!filePath.empty()) return filePath;

            std::vector<std::string> nameParts;
            size_t start = 0;
            size_t end = shortIconName.find('-');
            while (end != std::string::npos) {
                nameParts.push_back(shortIconName.substr(start, end - start));
                start = end + 1;
                end = shortIconName.find('-', start);
            }
            nameParts.push_back(shortIconName.substr(start));

            for (const auto& part : nameParts) {
                filePath = checkExistsWildcard(scalableDir, part, imageExtensions);
                if (!filePath.empty()) return filePath;

                for (const auto& size : iconSizes) {
                    filePath = checkExistsWildcard(sizeDirBase + size + "/apps/", part, imageExtensions);
                    if (!filePath.empty()) return filePath;
                }

                filePath = checkExistsWildcard(breezeDarkDirBase, part, imageExtensions);
                if (!filePath.empty()) return filePath;
            }

            return "";
        }
    ;
};

#endif // DOCK_HPP