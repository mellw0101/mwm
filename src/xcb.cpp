#include <cstdint>
#include "Log.hpp"
#include <xcb/xcb.h>

namespace xcb {
    inline uint32_t gen_Xid(xcb_connection_t *__c) {
        uint32_t w = xcb_generate_id(__c);
        if (w == UINT32_MAX) {
            loutE << "failed to generate Xid" << loutEND;

        } return w;

    }

}