#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <xcb/xcb.h>
#include <functional>
#include <unordered_map>
#include <xcb/xproto.h>

#include "structs.hpp"

using Ev = const xcb_generic_event_t *;

class Event_Handler
{
    public: // methods
        using EventCallback = std::function<void(const xcb_generic_event_t*)>;

        void run() 
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
                auto it = eventCallbacks.find(responseType);
                if (it != eventCallbacks.end()) 
                {
                    for (const auto& callback : it->second) 
                    {
                        callback(ev);
                    }
                }

                free(ev);
            }
        }

        void end()
        {
            shouldContinue = false;
        }

        void setEventCallback(uint8_t eventType, EventCallback callback) 
        {
            eventCallbacks[eventType].push_back(std::move(callback));
        }
    ;
    private: // variables
        std::unordered_map<uint8_t, std::vector<EventCallback>> eventCallbacks;
        bool shouldContinue = false;
    ;
};

class Event_Handler_test
{
    public:
        // Function template for event callbacks
        template <typename EventType>
        using EventCallback = std::function<void(const EventType*)>;

        // Method to set an event callback for a specific event type
        template <typename EventType>
        void setEventCallback(uint8_t eventType, EventCallback<EventType> callback)
        {
            // Store the callback after wrapping it in a lambda to fit the generic event callback type
            eventCallbacks[eventType] = [callback](const xcb_generic_event_t* ev) {
                callback(reinterpret_cast<const EventType*>(ev));
            };
        }

        void run()
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

        void end()
        {
            shouldContinue = false;
        }
    ;

    private:
        // Generic event callback type
        using GenericEventCallback = std::function<void(const xcb_generic_event_t*)>;

        // Map to store event callbacks
        std::unordered_map<uint8_t, GenericEventCallback> eventCallbacks;
        bool shouldContinue = false;
    ;
};

#endif // EVENT_HANDLER_HPP