#ifndef TOOLS_HPP
#define TOOLS_HPP
#include <cstdint>
#include "structs.hpp"
#include <xcb/xproto.h>

namespace scale 
{
    uint16_t 
    from_8_to_16_bit(uint8_t color);
}

namespace get 
{
    namespace color 
    {
        namespace 
        {
            xcb_connection_t *conn = nullptr;
            xcb_screen_t *screen = nullptr;
            xcb_colormap_t colormap = screen->default_colormap;
            xcb_alloc_color_reply_t *reply;
        }

        uint32_t
        rgb(COLOR_RGB color);
    }
}

#endif // TOOLS_HPP