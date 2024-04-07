#ifndef EVENT__HPP
#define EVENT__HPP

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
// #include "thread.hpp"
// #include "xcb.hpp"

#define lambdA(__Ref, __Param, ...) do { \
    [__Ref](__Param) __VA_ARGS__ \
} while(0)

#define MAKE_MAP_LAMBDA(__Map, __Event, ...) \
    do { \
        __Map[__Event] = lambdA(this, xcb_generic_event_t *, __VA_ARGS__) \
    } \
    while(0)

#define reCast(__Type, ...) \
    do { \
        auto e = reinterpret_cast<__Type *>(ev); \
        __VA_ARGS__ \
    } while(0)

/* typedef struct loop_data_t {
    uint8_t state = 1, event = 0;
    operator uint8_t&() { return this->state; }

    loop_data_t(uint8_t event) {
        
    }

} loop_data_t; */

class loop_data_t {
    private:
        uint8_t state = 1;
        uint8_t event = 0;
        static loop_data_t* instance;

        // Private constructor
        loop_data_t(uint8_t eventValue) : event(eventValue) {}

    public:
        // Delete copy constructor and assignment operator
        loop_data_t(const loop_data_t&) = delete;
        loop_data_t& operator=(const loop_data_t&) = delete;

        // Public method to access the instance
        static loop_data_t* getInstance(uint8_t eventValue)
        {
            if (!instance)
            {
                instance = new loop_data_t(eventValue);
            }
            return instance;
        }

        // Operator to access the state as uint8_t&
        operator uint8_t&() { return this->state; }

        // Optionally, methods to manipulate state and event
};

template<typename Type>
Type get_type( const xcb_generic_event_t *ev ) {};

// Initialize the static instance pointer to nullptr
inline loop_data_t* loop_data_t::instance = nullptr;

class __event__handler {
    private:
        std::unordered_map<int, std::function<void(xcb_generic_event_t *)>> Map;
        xcb_generic_event_t *ev;
        xcb_connection_t *conn;

        void makeMap_() {
            Map[XCB_EXPOSE] = [this](xcb_generic_event_t *ev) -> void {
                reCast(xcb_expose_event_t,
                    
                    
                );
                
            };
            
        }

    public:
        template<typename Func>
        void addCb(int __s, Func &&func) {
            Map[__s] = std::forward<Func>(func);
            
        }
        void run() {
            while ((ev = xcb_wait_for_event(conn)) != nullptr) {
                uint8_t responseType = ev->response_type & ~0x80;
                if (Map.find(responseType) != Map.end()) {
                    Map[responseType](ev);

                } /* free(ev); // Remember to free the event */
            
            }
            
        }

        __event__handler(xcb_connection_t *conn) : ev(new xcb_generic_event_t) {}

};

#endif