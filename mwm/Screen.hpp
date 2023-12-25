#ifndef _SCREEN_HPP_
#define _SCREEN_HPP_

#include <cstdint>
#include <xcb/xproto.h>

namespace XCPPB 
{
    namespace get 
    {
        namespace screen 
        {
            namespace 
            {
                xcb_screen_t * screen;
                xcb_connection_t * conn;
            }

            xcb_screen_t * 
            get_screen();

            uint16_t
            W();

            uint16_t
            H();

            uint16_t
            W_Half();

            uint16_t
            H_Half();
        }
    }
}

#endif