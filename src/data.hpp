#ifndef __DATA__HPP__
#define __DATA__HPP__
#include <X11/X.h>
#include <cstdint>
#include <array>
#include <utility>
#include <xcb/xcb.h>
#include "functional"
// #include "structs.hpp"
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
#define Ev6 17
#define Ev7 20
#define Ev8 6
#define Ev9 2
#define Ev10 4
#define Ev11 19
#define Ev12 28 /* <- XCB_PROPERTY_NOTIFY */

enum class MWM_Ev : const uint8_t {
    NO_Ev          = 0,
    EXPOSE         = 1,
    ENTER_NOTIFY   = 2,
    LEAVE_NOTIFY   = 3,
    FOCUS_IN       = 4,
    FOCUS_OUT      = 5,
    DESTROY_NOTIF  = 6,
    MAP_REQ        = 7,
    MOTION_NOTIFY  = 8,
    KEY_PRESS      = 9,
    BUTTON_PRESS   = 10,
    MAP_NOTIF      = 11,
    PROPERTY_NOTIF = 12

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
#define MWM_Ev_NO_Ev          0
#define MWM_Ev_EXPOSE         1
#define MWM_Ev_ENTER_NOTIFY   2
#define MWM_Ev_LEAVE_NOTIFY   3
#define MWM_Ev_FOCUS_IN       4
#define MWM_Ev_FOCUS_OUT      5
#define MWM_Ev_DESTROY_NOTIF  6
#define MWM_Ev_MAP_REQ        7
#define MWM_Ev_MOTION_NOTIFY  8
#define MWM_Ev_KEY_PRESS      9
#define MWM_Ev_BUTTON_PRESS   10
#define MWM_Ev_MAP_NOTIF      11
#define MWM_Ev_PROPERTY_NOTIF 12
constexpr uint8_t get_ev(uint8_t eventCode) {
    switch (eventCode) {
        case Ev1:  return MWM_Ev_EXPOSE;
        case Ev2:  return MWM_Ev_ENTER_NOTIFY;
        case Ev3:  return MWM_Ev_LEAVE_NOTIFY;
        case Ev4:  return MWM_Ev_FOCUS_IN;
        case Ev5:  return MWM_Ev_FOCUS_OUT;
        case Ev6:  return MWM_Ev_DESTROY_NOTIF;
        case Ev7:  return MWM_Ev_MAP_REQ;
        case Ev8:  return MWM_Ev_MOTION_NOTIFY;
        case Ev9:  return MWM_Ev_KEY_PRESS;
        case Ev10: return MWM_Ev_BUTTON_PRESS;
        case Ev11: return MWM_Ev_MAP_NOTIF;
        case Ev12: return MWM_Ev_PROPERTY_NOTIF;
        
        default: {
            return MWM_Ev_NO_Ev;

        }

    }

}
constexpr MWM_Ev map_ev_to_enum(uint8_t eventCode) {
    switch (eventCode) {
        case Ev1:  return MWM_Ev::EXPOSE;
        case Ev2:  return MWM_Ev::ENTER_NOTIFY;
        case Ev3:  return MWM_Ev::LEAVE_NOTIFY;
        case Ev4:  return MWM_Ev::FOCUS_IN;
        case Ev5:  return MWM_Ev::FOCUS_OUT;
        case Ev6:  return MWM_Ev::DESTROY_NOTIF;
        case Ev7:  return MWM_Ev::MAP_REQ;
        case Ev8:  return MWM_Ev::MOTION_NOTIFY;
        case Ev9:  return MWM_Ev::KEY_PRESS;
        case Ev10: return MWM_Ev::BUTTON_PRESS;
        case Ev11: return MWM_Ev::MAP_NOTIF;
        case Ev12: return MWM_Ev::PROPERTY_NOTIF;

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

    };

} /* Usage -> ' auto myCallback = makeCallback(1, 2.0, "test"); ' */
#define DEFINE_CALLBACK(name, ...) \
    auto name = [__VA_ARGS__](auto&&... args) -> void /* { \
        // Implementation using args and __VA_ARGS__ \
    }; */

/* Usage -> ' DEFINE_CALLBACK(myCallback, int a, float b) ' */

template<typename Func, typename... Args>
void registerCallback(Func&& func, Args&&... args) {
    // Assuming a hypothetical register function that takes a std::function
    auto callback = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    // register callback

} /* Usage -> */// registerCallback([](int x){ /* do something with x */ }, 42);

template<typename Func, typename... Args>
auto reg_static_callB(Func&& func, Args&&... args) {
    // Assuming a hypothetical register function that takes a std::function
    return std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    // register callback

} /* Usage -> */// registerCallback([](int x){ /* do something with x */ }, 42);

template<typename Func, typename... BoundArgs>
auto reg_callB(Func&& func, BoundArgs&&... boundArgs) {
    // Return a lambda that captures bound arguments and accepts additional arguments at call time.
    return [func = std::forward<Func>(func), ...boundArgs = std::forward<BoundArgs>(boundArgs)]
           (auto&&... callArgs) mutable -> decltype(auto) {
        return func(std::forward<BoundArgs>(boundArgs)..., std::forward<decltype(callArgs)>(callArgs)...);
    };
}

#include <cstdlib> // For malloc and free
#include <new>     // For placement new

template<typename T>
class MallocAllocator {
public:
    // Allocates memory for one object of type T using malloc
    static T* allocate() {
        void* ptr = std::malloc(sizeof(T)); // Allocate raw memory
        if (!ptr) throw std::bad_alloc();   // Check for allocation failure
        return new(ptr) T();                // Use placement new to construct the object

    }
    // Deallocates memory for one object of type T
    static void deallocate(T* ptr) {
        if (!ptr) return;
        ptr->~T();    // Call the destructor explicitly
        std::free(ptr); // Free the memory

    }

};

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
    private:
        function<void(Args...)> cb;

    public:
        template<typename Cb>
        Signal(Cb &&__cb) : cb(std::forward<Cb>(__cb)) {}
        Signal() {}

        template<typename Fu>
        void connect(Fu&& f) {
            cb = std::forward<Fu>(f);

        }/* Register a callback */
        
        template<typename... CbArgs>/**
         *
         * Emit the signal, invoking the callback with perfect forwarding
         *
         */
        void emit(CbArgs&&... args) {
            cb(std::forward<CbArgs>(args)...);

        }

        template<typename ...A>
        void operator()(A &&...a) {
            cb(std::forward<A>(a)...);
            
        }
        
};

template<typename ReturnType, typename... Args>
class Sig {
    private:
        function<ReturnType(Args...)> cb;

    public:
        template<typename Cb>
        Sig(Cb &&__cb) : cb(std::forward<Cb>(__cb)) {}
        Sig() {}

        template<typename Fu>
        void connect(Fu&& f) {
            cb = std::forward<Fu>(f);

        }/* Register a callback */

        template<typename... CbArgs>/**
         *
         * Emit the signal, invoking the callback with perfect forwarding
         *
         */
        void emit(CbArgs&&... args) {
            cb(std::forward<CbArgs>(args)...);

        }

        template<typename ...A>
        void operator()(A &&...a) {
            cb(std::forward<A>(a)...);
            
        }
        
};

#define FUNC_TIMER_SIGNAL 145
#include <chrono>
#include "Log.hpp"

class ScopeTimer {
    private:
        uint8_t ev_type;
        string scopeName;
        chrono::high_resolution_clock::time_point startTime;
        chrono::microseconds &executionTime;

    public:
        ScopeTimer(const string& name, chrono::microseconds &executionTimeRef, uint8_t ev_type)
            : scopeName(name), executionTime(executionTimeRef), ev_type(ev_type) {
            startTime = chrono::high_resolution_clock::now();

        }
        ~ScopeTimer() {
            auto endTime = chrono::high_resolution_clock::now();
            executionTime = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
            loutI << scopeName << " executed in " << executionTime.count() << " microseconds" << loutEND;

        }

};
/* 
class AtomicSignal {
    public:
        static void encodeInstruction(std::atomic<uint64_t>& var, uint64_t instruction) {
            var.store(instruction);
        }

        static uint64_t decodeInstruction(const std::atomic<uint64_t>& var) {
            return var.load();
        }

        // Example of a more complex encoding/decoding that uses two atomic variables
        static void encodeComplexInstruction(std::atomic<uint64_t>& var1, std::atomic<uint64_t>& var2, uint64_t instructionPart1, uint64_t instructionPart2) {
            var1.store(instructionPart1);
            var2.store(instructionPart2);
        }

        // Imagine this being called by worker threads to perform actions based on the encoded instructions
        static void executeBasedOnInstruction(const std::atomic<uint64_t>& instructionVar) {
            uint64_t instruction = decodeInstruction(instructionVar);

            // Decode and execute the instruction
            // This is a simplistic representation; your actual decoding logic can be much more complex
            if (instruction & 0x1) { // Example condition
                // Perform action based on the instruction
            }
        }
};
 */
#endif/*__DATA_HPP__*/

