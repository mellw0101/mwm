#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <cstdint>
#include "window.hpp"
#include <functional>

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
        void
        create(const uint32_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height, COLOR color)
        {
            window.create_default(parent_window, x, y, width, height);
            window.set_backround_color(color);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.map();
            xcb_flush(conn);
        }

        void
        action(std::function<void()> action)
        {
            button_action = action;
        }

        void 
        activate() const 
        {
            button_action();
        }
    ;

    private: // private variables 
        std::function<void()> button_action;
    ;
};

#endif // BUTTON_HPP