#ifndef TOOLS_HPP
#define TOOLS_HPP
#include <cstdint>
#include <xcb/xproto.h>

namespace scale 
{
    uint16_t 
    from_8_to_16_bit(uint8_t color);
}


#endif // TOOLS_HPP