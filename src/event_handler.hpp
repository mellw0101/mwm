#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <cstdint>
#include <xcb/xcb.h>
#include <functional>
#include <unordered_map>
#include <xcb/xproto.h>

#include "structs.hpp"

using Ev = const xcb_generic_event_t *;

class Event_Handler {
    public: // methods
        using EventCallback = std::function<void(Ev)>;
        
        void run() {
            xcb_generic_event_t *ev;
            shouldContinue = true;

            while (shouldContinue) {
                ev = xcb_wait_for_event(conn);
                if (!ev) {
                    continue;
                }

                uint8_t responseType = ev->response_type & ~0x80;
                auto it = eventCallbacks.find(responseType);
                if (it != eventCallbacks.end()) {
                    for (const auto& callback : it->second) {
                        callback(ev);
                    }
                }

                free(ev);
            }
        }
        void end() {
            shouldContinue = false;
        }
        void setEventCallback(uint8_t eventType, EventCallback callback) {
            eventCallbacks[eventType].push_back(std::move(callback));
        }
    ;
    private: // variables
        std::unordered_map<uint8_t, std::vector<EventCallback>> eventCallbacks;
        bool shouldContinue = false;
    ;
};
static Event_Handler * event_handler;

#endif // EVENT_HANDLER_HPP