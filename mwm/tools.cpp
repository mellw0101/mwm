#include "tools.hpp"

namespace scale 
{
    uint16_t 
    from_8_to_16_bit(uint8_t color) 
    {
        // Scale 8-bit color to 16-bit color
        return static_cast<uint16_t>(color) << 8 | color;
    }
}

namespace get 
{
    namespace color 
    {
        uint32_t
        rgb(COLOR_RGB color)
        {
            reply = xcb_alloc_color_reply
            (
                conn, 
                xcb_alloc_color
                (
                    conn,
                    colormap,
                    scale::from_8_to_16_bit(color.r), 
                    scale::from_8_to_16_bit(color.g),
                    scale::from_8_to_16_bit(color.b)
                ), 
                NULL
            );
            return reply->pixel;
        }
    }
}