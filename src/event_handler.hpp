#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <xcb/xcb.h>
#include <functional>
#include <unordered_map>
#include <xcb/xproto.h>

#include "structs.hpp"

using Ev = const xcb_generic_event_t;

class Event_Handler
{
    public:
        using EventCallback = std::function<void(const xcb_generic_event_t*)>;
        

        void 
        run() 
        {
            xcb_generic_event_t *ev;
            shouldContinue = true;

            while (shouldContinue) 
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) 
                {
                    continue;
                }

                uint8_t responseType = ev->response_type & ~0x80;
                if (eventCallbacks.find(responseType) != eventCallbacks.end()) 
                {
                    eventCallbacks[responseType](ev);
                }

                free(ev);
            }
        }

        void
        end()
        {
            shouldContinue = false;
        }

        void 
        setEventCallback(uint8_t eventType, EventCallback callback) 
        {
            eventCallbacks[eventType] = std::move(callback);
        }
    ;
    private:
        std::unordered_map<uint8_t, EventCallback> eventCallbacks;
        bool shouldContinue = false;
    ;
};

#endif // EVENT_HANDLER_HPP