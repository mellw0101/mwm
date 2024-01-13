#ifndef DESKTOP_HPP
#define DESKTOP_HPP

#include <stdint.h>
#include <vector>
#include "client.hpp"

class desktop
{
    public: // variabels
        std::vector<client *> current_clients;
        uint16_t desktop;
        const uint16_t x = 0;
        const uint16_t y = 0;
        uint16_t width;
        uint16_t height;
    ;
};

#endif