#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <cstdint>
#include <functional>
#include <xcb/xproto.h>

#include "Log.hpp"
#include "window.hpp"
#include "file.hpp"
#include "event_handler.hpp"

class button
{
    public: // constructor 
        button() {}
    ;
    public: // public variables 
        window window;
        const char * name;
    ;
    public: // public methods 
        void create(const uint32_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height, COLOR color)
        {
            window.create_default(parent_window, x, y, width, height);
            window.set_backround_color(color);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.map();
            xcb_flush(conn);
        }
        void action(std::function<void()> action)
        {
            button_action = action;
        }
        void add_event(std::function<void(Ev ev)> action)
        {
            ev_a = action;
            event_handler->setEventCallback(XCB_BUTTON_PRESS, ev_a);
        }
        void activate() const 
        {
            button_action();
        }
        void put_icon_on_button()
        {
            std::string icon_path = file.find_png_icon
            (
                {
                    "/usr/share/icons/gnome/256x256/apps/",
                    "/usr/share/icons/hicolor/256x256/apps/",
                    "/usr/share/icons/gnome/48x48/apps/",
                    "/usr/share/icons/gnome/32x32/apps/"
                },
                name
            );

            if (icon_path == "")
            {
                log_info("could not find icon for button: " + std::string(name));
                return;
            }
            window.set_backround_png(icon_path.c_str());
        }
    ;
    private: // private variables 
        std::function<void()> button_action;
        std::function<void(Ev ev)> ev_a;
        File file;
        Logger log;
    ;
};

#endif // BUTTON_HPP