#include "Screen.hpp"

namespace XCPPB 
{
    namespace get 
    {
        namespace screen 
        {
            xcb_screen_t* 
            get_screen() 
            {
                if (screen == nullptr) 
                {
                    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
                }
                return screen;
            }

            uint16_t
            W()
            {
                return get_screen()->width_in_pixels;
            }

            uint16_t
            H()
            {
                return get_screen()->height_in_pixels;
            }

            uint16_t
            W_Half()
            {
                return get_screen()->width_in_pixels / 2;
            }

            uint16_t
            H_Half()
            {
                return get_screen()->height_in_pixels / 2;
            }
        }
    }
}