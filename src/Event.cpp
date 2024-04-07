#include "Event.hpp"

template<>
const xcb_expose_event_t *get_type(const xcb_generic_event_t *ev) {
    return reinterpret_cast<const xcb_expose_event_t *>(ev);
}