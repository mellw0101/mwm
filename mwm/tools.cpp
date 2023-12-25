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