#ifndef __DATA__HPP__
#define __DATA__HPP__
#include <X11/X.h>
#include <cstdint>
#include <array>
#include "functional"
// #include "structs.hpp"

using namespace std;

constexpr uint8_t transformValue(uint8_t input) {
    return (input % 2 == 0) ? input / 2 : input * 3 + 1;

}

constexpr size_t NUM_EVENTS = 100;
static array<function<void(uint32_t)>, NUM_EVENTS> eventHandlers{};

#define Ev1 12
#define Ev2 7
#define Ev3 8
#define Ev4 9
#define Ev5 10

enum class MWM_Ev : const uint8_t {
    NO_Ev = 255,
    EXPOSE = 1,
    ENTER_NOTIFY = 2,
    LEAVE_NOTIFY = 3,
    FOCUS_IN     = 4,
    FOCUS_OUT    = 5

};

#define MWM_EXPOSE 1
#define MWM_ENTER_NOTIFY 2
#define MWM_LEAVE_NOTIFY 3
#define MWM_PROPERTY_NOTIFY
#define MWM_MAP_REQ
#define MWM_MAP_NOTIFY
#define MWM_KEY_PRESS
#define MWM_FOCUS_IN
#define MWM_FOCUS_OUT
#define MWM_KEY_RELESE
#define MWM_BUTTON_RELEASE
#define MWM_MOTION_NOTIFY
#define MWM_DESTROY_NOTIFY
#define MWM_REPARENT_NOTIFY
#define MWM_CONFIGURE_REQUEST

static constexpr uint8_t uint8_t_MAX = 255; 
constexpr uint8_t MapEventToCode(uint8_t eventCode) {
    switch (eventCode) {
        case Ev1: return MWM_EXPOSE;
        case Ev2: return MWM_ENTER_NOTIFY;
        case Ev3: return MWM_LEAVE_NOTIFY;
        
        default: {
            return uint8_t_MAX;

        }

    }

}
constexpr MWM_Ev map_ev_to_enum(uint8_t eventCode) {
    switch (eventCode) {
        case Ev1: return MWM_Ev::EXPOSE;
        case Ev2: return MWM_Ev::ENTER_NOTIFY;
        case Ev3: return MWM_Ev::LEAVE_NOTIFY;

        
        default: {
            return MWM_Ev::NO_Ev;

        }

    }

}
template<uint8_t eventCode> struct EvI;
template<> struct EvI<12> {
    static constexpr size_t index = 1; // Corresponding index in the eventHandlers array
    
};
template<> struct EvI<7> {
    static constexpr size_t index = 2;

};
template<uint8_t eventCode>
void emitEvent(uint32_t windowId) {
    constexpr size_t index = EvI<eventCode>::index;
    if (index < NUM_EVENTS && eventHandlers[index]) {
        eventHandlers[index](windowId);
        
    }

}

#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

class AnyFunction {
    public:
        virtual ~AnyFunction() = default;
        virtual void call() = 0; // A common interface for invocation

};
template<typename Func>
class TypedFunction : public AnyFunction {
    public:
        TypedFunction(Func f) : func(std::move(f)) {}
        void call() override {
            func(); // Assumes a no-argument call for simplicity

        }

    private:
        Func func;

};
class FunctionMap {
    public:
        template<typename Fu>
        void add(uint8_t outerKey, uint32_t innerKey, Fu f) {
            auto& innerMap = functions[outerKey];
            innerMap[innerKey] = std::make_unique<TypedFunction<Fu>>(std::move(f));

        }
        void invoke(uint8_t outerKey, uint32_t innerKey) {
            auto outerIt = functions.find(outerKey);
            if (outerIt != functions.end()) {
                auto& innerMap = outerIt->second;
                auto innerIt = innerMap.find(innerKey);
                if (innerIt != innerMap.end()) {
                    innerIt->second->call(); // Invoke the function

                }

            }

        }

    private:
        std::unordered_map<uint8_t, std::unordered_map<uint32_t, std::unique_ptr<AnyFunction>>> functions;

};

template<typename T>
using Callback = std::function<void(T)>;

template<typename... Args>
auto makeCallback(Args&&... args) -> std::function<void(Args...)> {
    return [...args = std::forward<Args>(args)]() {
        // Use args...
        
    };

} /* Usage -> ' auto myCallback = makeCallback(1, 2.0, "test"); ' */
#define DEFINE_CALLBACK(name, ...) \
    auto name = [__VA_ARGS__](auto&&... args) -> void { \
        // Implementation using args and __VA_ARGS__ \
    };

/* Usage -> ' DEFINE_CALLBACK(myCallback, int a, float b) ' */

template<typename Func, typename... Args>
void registerCallback(Func&& func, Args&&... args) {
    // Assuming a hypothetical register function that takes a std::function
    auto callback = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    // register callback

} /* Usage -> */// registerCallback([](int x){ /* do something with x */ }, 42);

#include <vector>
#include <memory>
#include <functional>

class AnyCallable {
    public:
        virtual ~AnyCallable() = default;
        virtual void call() = 0; // Define a common interface

};

template<typename Func>
class TypedCallable : public AnyCallable {
    public:
        TypedCallable(Func f) : func(std::move(f)) {}
        void call() override {
            func(); // Invoke the stored callable
        }
    private:
        Func func;
    
};

class CallableVector {
    public:
        template<typename Fu>
        void add(Fu f) {
            callables.push_back(std::make_unique<TypedCallable<Fu>>(std::move(f)));

        }
        void invokeAll() {
            for (auto& callable : callables) {
                callable->call(); // Call through the common interface

            }

        }

    private:
        std::vector<std::unique_ptr<AnyCallable>> callables;
        
};

// A simple Signal class that accepts callbacks with varying parameters
template<typename... Args>
class Signal {
    public:
        template<typename Fu>
        void connect(Fu&& f) {
            // Store the callback; for simplicity, we're directly storing it without a container
            callback = std::forward<Fu>(f);

        }/* Register a callback */

        // Emit the signal, invoking the callback with perfect forwarding
        template<typename... CallArgs>
        void emit(CallArgs&&... args) {
            callback(std::forward<CallArgs>(args)...);

        }

    private:
        function<void(Args...)> callback;

};



#endif/*__DATA_HPP__*/

