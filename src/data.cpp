#include <cstdint>
#include <array>
#include "functional"
using namespace std;

constexpr uint8_t transformValue(uint8_t input) {
    return (input % 2 == 0) ? input / 2 : input * 3 + 1;

}

constexpr size_t NUM_EVENTS = 100;
array<function<void(uint32_t)>, NUM_EVENTS> eventHandlers{};
#define EXPOSE 12
#define ENTER_NOTIFY 7
#define LEAVE_NOTIFY 8

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

constexpr uint8_t MapEventToCode(uint32_t eventCode) {
    switch (eventCode) {
        case EXPOSE:       return MWM_EXPOSE;
        case ENTER_NOTIFY: return MWM_ENTER_NOTIFY;
        case LEAVE_NOTIFY: return MWM_LEAVE_NOTIFY;

        default: {
            return 255;

        }

    }

}
template<uint8_t eventCode> struct EvI;
template<> struct EvI<EXPOSE> {
    static constexpr size_t index = 1; // Corresponding index in the eventHandlers array
    
};
template<> struct EvI<ENTER_NOTIFY> {
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