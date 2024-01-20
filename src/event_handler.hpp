#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <cstdint>
#include <algorithm> // For std::remove_if
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
                        callback.second(ev);
                    }
                }

                free(ev);
            }
        }
        void end() {
            shouldContinue = false;
        }
        using CallbackId = int;

        CallbackId setEventCallback(uint8_t eventType, EventCallback callback) {
            CallbackId id = nextCallbackId++;
            eventCallbacks[eventType].emplace_back(id, std::move(callback));
            return id;
        }

        void removeEventCallback(uint8_t eventType, CallbackId id) {
            auto& callbacks = eventCallbacks[eventType];
            callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                        [id](const auto& pair) { return pair.first == id; }),
                            callbacks.end());
        }
    ;
    private: // variables
        std::unordered_map<uint8_t, std::vector<std::pair<CallbackId, EventCallback>>> eventCallbacks;
        bool shouldContinue = false;
        CallbackId nextCallbackId = 0;
    ;
};
static Event_Handler * event_handler;

#endif // EVENT_HANDLER_HPP