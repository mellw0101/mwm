#ifndef STRUCTS_HPP
#define STRUCTS_HPP
#include "include.hpp"
#include <X11/Xlib.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>
#include <initializer_list>
// #include "data.hpp"

using namespace std;

struct SimplifiedEvent {
    uint32_t windowID; // Essential: Identifies the target window
    uint8_t  eventType; // Optional: Identifies the type of event

};

template<typename Type>
inline Type* AllocArr(size_t __num_elements, size_t alignment = alignof(Type))
{
    size_t size = 0;

    // Calculate the size of the allocation
    if constexpr (is_pointer_v<Type>)
    {
        size = __num_elements * sizeof(void*);
    }
    else 
    {
        size = __num_elements * sizeof(Type);
    }

    // Ensure the size is a multiple of the alignment
    if (size % alignment != 0)
    {
        // Adjust size to be a multiple of alignment
        size += alignment - (size % alignment);
    }

    // Allocate the aligned memory
    void* ptr = std::aligned_alloc(alignment, size);
    
    if (!ptr)
    {
        // If allocation failed, throw std::bad_alloc
        throw std::bad_alloc();
    }

    // Return the allocated memory cast to the correct type
    return static_cast<Type*>(ptr);
}

template<typename T0, size_t Size>
class __fixed_array_t__ {
    public:
    /* Variabels */
        T0 data[Size];

    /* Methods */
        T0 &operator[](size_t index)
        {
            return data[index];
        }

        const T0 &operator[](size_t index) const
        {
            return this->data[index];
        }

        __fixed_array_t__<T0, Size>()
        : data{} {}

        __fixed_array_t__<T0, Size>(T0 __data[Size])
        : data{}
        {

        }

        // __fixed_array_t__<T0, Size>(const T0 __data)
        // : data{*__data}
        // {}



        // __fixed_array_t__(const T0 &__data_ptr)
        // :  data{} {}


        // __fixed_array_t__(const T0 &__data_ptr)
        // :  data{__data_ptr} {}


        // __fixed_array_t__(T0 &__default)
        // :  data{}
        // {
        //     for (size_t i = 0; i < Size; ++i)
        //     {
        //         data[i] = __default[i];
        //     }
        // }

        // __fixed_array_t__(const T0 &__data_ptr)
        // :  data{__data_ptr} {}

        // __fixed_array_t__(const T0 * const &__test)
        // :  data{__test}
        // {
        //     // // if (this->data != &__data)

        //     // for (size_t i = 0; i < Size; ++i)
        //     // {
        //     //     this->data[i] = __test[i];
        //     // }
        // }

};
template<typename T0, size_t n0 = 20>
using FixedArray = __fixed_array_t__<T0, n0>;

constexpr size_t size_t_MAX = numeric_limits<size_t>::max(); 

template<typename T1>
static constexpr T1 make_T_MAX() { return numeric_limits<T1>::max(); };

template<typename T0, size_t Size>
class __fixed_dynamic_array_t {
    public:
    /* Constructor */
        __fixed_dynamic_array_t()
        : data(new T0[Size]) {}

        __fixed_dynamic_array_t(initializer_list<T0> init)
        {
            for (auto &value : init)
            {
                push_back(value);
            }
        }

    /* Destructor  */
        ~__fixed_dynamic_array_t() { delete[] data; }

    /* operator    */
        T0& operator[](size_t index) { return data[index]; }

    /* Methods     */
        size_t getSize() const { return Size; }
        
        const auto iter(T0 &__value)
        {
            return std::find(begin(), end(), __value);
        }

        template<typename Type = T0>
        static size_t find(Type __value);

        template<>
        size_t find(T0 __value)
        {
            size_t i = 0;
            while (i < Size && data[i] != __value) ++i;
            if (i < Size) return i;
            return size_t_MAX;
        }

        bool is_valid(size_t __index)
        {
            if constexpr (is_pointer_v<T0>)
            {
                if (data[__index] == nullptr) return false;
            }
            else
            {
                if (data[__index] == size_t_MAX) return false;
            }
        }

        void removeAt(size_t __index)
        {
            if (__index >= Size) return;

            if constexpr (is_pointer_v<T0>) 
            {
                data[__index] = nullptr;
                return;
            }
            else
            {
                data[__index] = numeric_limits<T0>::max();
            }
        }

        T0* begin() const { return &data[0];    }/* Return pointer to the first element */
        T0* end()   const { return &data[Size]; }/* Return pointer past the last element */

    private:
    /* Variabels */
        T0* data;

};
template<typename T1, size_t Size = 20>
using FixedDynamicArray = __fixed_dynamic_array_t<T1, Size>;
class window;
class client;

template<typename T0>
class __dynamic_array_t {
    public:
    /* Constructor */
        __dynamic_array_t()
        : capacity(10), size(0), data(new T0[capacity]) {}

        __dynamic_array_t(initializer_list<T0> init)
        : capacity(init.size()), size(0)
        {
            for (auto &value : init)
            {
                push_back(value);
            }
        }

    /* Destructor  */
        ~__dynamic_array_t() { delete[] data; }

    /* operator    */
        T0& operator[](size_t index) { return data[index]; }

    /* Methods     */
        size_t getSize() const { return size; }
        
        const auto iter(T0 &__value)
        {
            return std::find(begin(), end(), __value);
        }
        
        void push_back(T0 __value)
        {
            if (size >= capacity) resize(capacity * 2);
            data[size++] = __value;
        }

        template<typename Type = T0>
        static size_t find(Type __value);

        template<>
        size_t find(T0 __value) {
            size_t i = 0;
            while (i < size && data[i] != __value) ++i;
            if (i < size) return i;
            return size_t_MAX;

        }

        bool is_valid_at(size_t __index) const {
            if constexpr (is_pointer_v<T0>) {
                return is_valid(data[__index]);
            } return is_valid(data[__index]);
        
        } /** @return @p 'true' if element at @p '__index' is valid */

        bool is_roten_at(size_t __index) const {
            if constexpr (is_pointer_v<T0>) {
                return is_roten(data[__index]);
            } return is_roten(data[__index]);
        
        } /** @return @p 'true' if element at @p '__index' is roten */

        void removeAt(size_t __index) {
            if (__index >= size) return;

            if constexpr (is_pointer_v<T0>) {
                data[__index] = nullptr;
                return;
            
            } else {
                data[__index] = numeric_limits<T0>::max();
            }

        } /** @b remove element at @p '__index' */

        T0* begin() const {
            return &data[0];
        
        } /** @return pointer to the first element  */
        
        T0* end()   const {
            return &data[size];
            
        } /** @return pointer past the last element */
        
        bool is_valid(T0 __input) const {
            if constexpr (is_pointer_v<T0>) return (__input != nullptr);
            return (__input != size_t_MAX);
            
        } /** @return true if a element is valid */

        bool is_roten(T0 __input) const {
            if constexpr (is_pointer_v<T0>) return (__input == nullptr);
            return (__input == size_t_MAX);
        
        } /** @return true if a element is roten */

    private:
    /* Variabels */
        size_t capacity;
        size_t size;
        T0* data;

    /* Methods   */
        void resize(size_t __new_capacity)
        {
            T0* new_data = new T0[__new_capacity];
            for (size_t i = 0; i < size; ++i) new_data[i] = data[i];
            delete[]   data;
            data     = new_data;
            capacity = __new_capacity;
        }

        void sort()
        {
            bool swapped;

            do
            {
                swapped = false;
                for (size_t i = 1; i < size; ++i)
                {
                    if (data[i - 1] > data[i]) // Assuming T1 supports comparison
                    {
                        T0 temp = data[i - 1];
                        data[i - 1] = data[i];
                        data[i] = temp;
                        swapped = true;
                    }
                }
            }
            while (swapped);
        }

        void remove(T0 __value)
        {
            size_t i = 0;
            while (i < size && data[i] != __value) ++i; // Find the element

            if (i < size) // If found
            {
                for (size_t j = i; j < size - 1; ++j)
                {
                    data[j] = data[j + 1]; // Shift elements
                }

                --size; // Decrease size
            }
        }

};
template<typename T1>
using DynamicArray = __dynamic_array_t<T1>;

class client;
class window;
class client_border_decor;
template<typename T1, typename T2>
class __linked_dynamic_array_t;

// template<>
// class __linked_dynamic_array_t<client *, uint32_t *> {
//     public:
//     /* Variabels */
//         DynamicArray<uint32_t *> _window;
//         DynamicArray<client *> _client;

//     /* Methods */
//         size_t add_client(client *__c, uint32_t *__w)
//         {
//             _client.push_back(__c);
//             _window.push_back(__w);
//             size_t c_size = size;
//             size++;
//             return c_size;
//         }

//         void remove_at(size_t __index)
//         {
//             _window.removeAt(__index);
//             _client.removeAt(__index);
//         }

//         // client *retrive_client(uint32_t __window)
//         // {
//         // }

//     /* Constructor */
//         size_t size;

//         __linked_dynamic_array_t() {}
// };
// using ldClientWindowArray = __linked_dynamic_array_t<client *, uint32_t *>;

template<
    typename T0,
    typename T1,
    typename T2 = T1,
    typename T3 = T2,
    typename T4 = T3,
    typename T5 = T4,
    typename T6 = T5,
    typename T7 = T6,
    typename T8 = T7,
    typename T9 = T8,
    typename T10 = T9,
    typename T11 = T10,
    typename T12 = T11,
    typename T13 = T12,
    typename T14 = T13,
    typename T15 = T14>
class __linked_super_array_t {
    public:
    /* Variabels */
        DynamicArray<T0> data_0;
        DynamicArray<T1> data_1;
        DynamicArray<T2> data_2;
        DynamicArray<T3> data_3;
        DynamicArray<T4> data_4;
        DynamicArray<T5> data_5;
        DynamicArray<T6> data_6;
        DynamicArray<T7> data_7;
        DynamicArray<T8> data_8;
        DynamicArray<T9> data_9;
        DynamicArray<T10> data_10;
        DynamicArray<T11> data_11;
        DynamicArray<T12> data_12;
        DynamicArray<T13> data_13;
        DynamicArray<T14> data_14;
        DynamicArray<T15> data_15;
};
template<
    typename T0,
    typename T1,
    typename T2 = T1,
    typename T3 = T2,
    typename T4 = T3,
    typename T5 = T4,
    typename T6 = T5,
    typename T7 = T6,
    typename T8 = T7,
    typename T9 = T8,
    typename T10 = T9,
    typename T11 = T10,
    typename T12 = T11,
    typename T13 = T12,
    typename T14 = T13,
    typename T15 = T14>
using LinkedSuperArray = __linked_super_array_t<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15>;

template<typename T0, typename T1>
class __linked_super_array_t<T0, T1, T1, T1, T1, T1, T1, T1, T1, T1, T1, T1, T1, T1, T1, T1>;

template<>
class __linked_super_array_t<client *, window> {
    private:
    /* Variabels */
        size_t _size;
        size_t _index;

    public:
    /* Variabels */
        DynamicArray<client *> c;
        DynamicArray<window *> win;
        DynamicArray<window *> frame;
        DynamicArray<window *> titlebar;
        DynamicArray<window *> close_button;
        DynamicArray<window *> max_button;
        DynamicArray<window *> min_button;
        DynamicArray<window *> icon;
        DynamicArray<client_border_decor *> border;
        DynamicArray<function<void()>*> function;

    /* Methods   */
        void push_back(

            client *__c,
            
            window &__win,
            window &__frame,
            window &__titlebar,
            window &__close_button,
            window &__max_button,
            window &__min_button,
            window &__icon,
            
            client_border_decor &__b)

        {
            c.push_back(__c);
            win.push_back(&__win);
            frame.push_back(&__frame);
            titlebar.push_back(&__titlebar);
            close_button.push_back(&__close_button);
            max_button.push_back(&__max_button);
            min_button.push_back(&__min_button);
            icon.push_back(&__icon);
            border.push_back(&__b);

            _size = c.getSize();
        }

        void remove(client* __c)
        {
            auto it = std::find(c.begin(), c.end(), __c);
            if (it != c.end())
            {
                size_t index = std::distance(c.begin(), it);
                // Assume dynamic arrays manage deletion if they own the objects
                c.removeAt(index);
                win.removeAt(index);
                frame.removeAt(index);
                titlebar.removeAt(index);
                close_button.removeAt(index);
                max_button.removeAt(index);
                min_button.removeAt(index);
                icon.removeAt(index);
                border.removeAt(index);
            }
        }

        typedef enum {
            _WIN          = 1 << 0,
            _FRAME        = 1 << 1,
            _TITLEBAR     = 1 << 2,
            _CLOSE_BUTTON = 1 << 3,
            _MAX_BUTTON   = 1 << 4,
            _MIN_BUTTON   = 1 << 5,
            _ICON         = 1 << 6,
            _ALL =
                _WIN | _FRAME | _TITLEBAR | _CLOSE_BUTTON | _MAX_BUTTON | _MIN_BUTTON | _ICON
        } client_window_t;
        template<client_window_t c_window_t = _ALL>
        client* get_client_from_window(window *__window)
        {
            auto find_client = [&](DynamicArray<window*>& arr) -> client *
            {
                auto it = arr.iter(__window);
                if (it != arr.end())
                {
                    return c[arr.find(it)];
                }

                return static_cast<client*>(nullptr);
            };

            client* found_client = nullptr;
            if constexpr (c_window_t & _WIN         ) if ((found_client = find_client(win))          != nullptr) return found_client;
            if constexpr (c_window_t & _FRAME       ) if ((found_client = find_client(frame))        != nullptr) return found_client;
            if constexpr (c_window_t & _TITLEBAR    ) if ((found_client = find_client(titlebar))     != nullptr) return found_client;
            if constexpr (c_window_t & _CLOSE_BUTTON) if ((found_client = find_client(close_button)) != nullptr) return found_client;
            if constexpr (c_window_t & _MAX_BUTTON  ) if ((found_client = find_client(max_button))   != nullptr) return found_client;
            if constexpr (c_window_t & _MIN_BUTTON  ) if ((found_client = find_client(min_button))   != nullptr) return found_client;
            if constexpr (c_window_t & _ICON        ) if ((found_client = find_client(icon))         != nullptr) return found_client;

            return nullptr; // Window not found in any array
        }

        client *get_client_from_window_test(window *__window)
        {
            auto it = win.iter(__window);
            if (it != win.end()) return c[win.find(it)];

            it = frame.iter(__window);
            if (it != frame.end()) return c[frame.find(it)];

            it = titlebar.iter(__window);
            if (it != titlebar.end()) return c[titlebar.find(it)];

            it = close_button.iter(__window);
            if (it != close_button.end()) return c[close_button.find(it)];

            it = max_button.iter(__window);
            if (it != max_button.end()) return c[max_button.find(it)];

            it = min_button.iter(__window);
            if (it != min_button.end()) return c[min_button.find(it)];

            it = icon.iter(__window);
            if (it != icon.end()) return c[icon.find(it)];

            return nullptr;
        }

};
template<typename T0, typename T1>
using LinkedSuperArrayMap = __linked_super_array_t<T0, T1>;

// template<typename T1, size_t Size>
// class __data_array_t__ {
//     public:
//     /* Variabels */
//         static_assert(Size > 0, "Size must be greater than 0.");
//         T1 _data[Size];
//         size_t _index;

//     /* Proxy */
//         typedef struct __proxy_data_t__ {
//             /* Vatiabels */
//                 __data_array_t__& _arr;
//                 size_t _index;

//             /* Constructor */
//                 constexpr __proxy_data_t__(__data_array_t__ &__arr, size_t __index)
//                 : _arr(__arr), _index(__index) {}

//             /* Operators */
//                 constexpr T1 &operator=(const T1 &__value)
//                 {
//                     /* Assign the value to the actual array element */
//                     this->_arr._data[this->_index] = __value;

//                     /* Update the current index in the main array */
//                     this->_arr._index = this->_index;
                    
//                     /* Return the assigned value */
//                     return this->_arr._data[this->_index];
//                 }

//                 constexpr operator T1&() const
//                 {
//                     return this->_arr._data[this->_index];
//                 }

//         } Proxy;

//         constexpr T1 &operator[](size_t index)
//         {
//             assert(index < Size);
//             return Proxy(*this, index);
//         }

//         constexpr const T1 &operator[](size_t index) const
//         {
//             assert(index < Size);
//             return this->_data[index];
//         }

//     /* Methods */
//         /* Size */
//             constexpr size_t size()
//             {
//                 return _index;
//             }

//             constexpr size_t size() const
//             {
//                 return _index;
//             }

//         /* Max Size */
//             constexpr size_t max_size()
//             {
//                 return Size;
//             }

//             constexpr size_t max_size() const
//             {
//                 return Size;
//             }

//         constexpr void add(const T1 &__input)
//         {
//             if (_index < Size)
//             {
//                 _data[_index++] = __input;
//             }
//         }

//     /* Methods */
//         constexpr void clear()
//         {
//             for (size_t i = 0; i < Size; ++i)
//             {
//                 _data[i] = T1{};
//             }

//             _index = 0;
//         }

//     /* Constructor */
//         constexpr __data_array_t__ ()
//         : _index(0)
//         { clear(); }

//         constexpr __data_array_t__(const T1 (&__data)[Size])
//         : _index(Size)
//         {
//             this->clear();
//             for (size_t i = 0; i < Size; ++i)
//             {
//                 this->_data[i] = __data[i];
//             }
//         }

// };
// // template<typename T1, size_t Size>
// // using Array = __data_array_t__<T1, Size>;

// #define FIRST_T(__name)  __MAKE_TYPE_FLAG__(__name, T1)
// #define SECOND_T(__name) __MAKE_TYPE_FLAG__(__name, T2)
// template<typename T1, typename T2, size_t Size>
// struct __pair_array_t__ {

//     Array<T1, Size> first;
//     Array<T2, Size> second;

//     typedef struct __proxy_accessor_t__ {
//         /* Variabels    */
//             T1 &_first;
//             T2 &_second;

//         /* Constructors */
//             __proxy_accessor_t__(T1 &__first, T2 &__second)
//             : _first(__first), _second(__second) {}

//         /* Implicit conversion operators */
//             operator T1&() { return _first; }
//             operator T2&() { return _second; }

//     } proxy_accessor_t;

//     proxy_accessor_t operator[](size_t index)
//     {
//         return first[index], second[index];
//     }

// };
// template<typename T1, typename T2, size_t Size>
// using PairArray = __pair_array_t__<T1, T2, Size>;

// template<typename T1, typename T2, typename T3, typename T4, size_t Size>
// struct __quad_array_t__ {

//     Array<T1, Size> first;
//     Array<T2, Size> second;
//     Array<T3, Size> third;
//     Array<T4, Size> fourth;

//     typedef struct __proxy_accessor_t__ {
//         /* Variabels    */
//             T1 &_first;
//             T2 &_second;
//             T3 &_third;
//             T4 &_fourth;

//         /* Constructors */
//             __proxy_accessor_t__(T1 &__first, T2 &__second, T3 &__third, T4 &__fourth)
//             : _first(__first), _second(__second), _third(__third), _fourth(__fourth) {}

//         /* Implicit conversion operators */
//             operator T1&() { return _first;  }
//             operator T2&() { return _second; }
//             operator T3&() { return _third;  }
//             operator T4&() { return _fourth; }

//     } proxy_accessor_t;

//     proxy_accessor_t operator[](size_t index)
//     {
//         return first[index], second[index], third[index], fourth[index];
//     }

// };
// template<typename T1, typename T2, typename T3, typename T4, size_t Size>
// using QuadArray = __quad_array_t__<T1, T2, T3, T4, Size>;
enum
{
    N = 12, /* 
        THIS IS HOW FAR AWAY SNAPING WILL HAPPEN 
     */ 
    NC = 8 /* 
        THIS IS HOW FAR AWAY WINDOW TO WINDOW CORNER SNAPPING WILL HAPPEN 
     */
};

// inline const function<void(uint32_t)> arr[2] = {
//     {[](uint32_t __w) -> void {
        
        
//     }},
//     {}

// };

#define L_MOUSE_BUTTON_EVENT              36
#define L_MOUSE_BUTTON_EVENT__ALT         37
#define QUIT_KEY_PRESS                    45
#define TERM_KEY_PRESS                    44
#define TILE_RIGHT                        55
#define TILE_LEFT                         56
#define TILE_UP                           57
#define TILE_DOWN                         58
#define DESTROY_NOTIF_EV                  82
#define DESTROY_NOTIF_W                   83
#define DEBUG_KEY_PRESS                   62
#define MOVE_TO_DESKTOP_1                 48
#define MOVE_TO_DESKTOP_2                 49
#define MOVE_TO_DESKTOP_3                 50
#define MOVE_TO_DESKTOP_4                 51
#define MOVE_TO_DESKTOP_5                 52
#define MOVE_TO_PREV_DESKTOP              53
#define MOVE_TO_NEXT_DESKTOP              54
#define CYCLE_FOCUS_KEY_PRESS             59
#define MOVE_TO_PREV_DESKTOP_WAPP         60
#define MOVE_TO_NEXT_DESKTOP_WAPP         61
#define EWMH_MAXWIN_SIGNAL                46
#define R_MOUSE_BUTTON_EVENT              38
#define R_MOUSE_BUTTON_EVENT__ALT         39
#define KILL_SIGNAL                       40
#define CLIENT_RESIZE                     41
#define CLIENT_RESIZE_ALT                 42
#define BUTTON_MAXWIN_PRESS               63
#define FOCUS_CLIENT                      64
#define MOVE_CLIENT_MOUSE                 65
#define MOVE_CLIENT_ALT                   66
#define CONF_REQ_WIDTH                    67
#define CONF_REQ_HEIGHT                   68
#define CONF_REQ_X                        69
#define CONF_REQ_Y                        70
#define FOCUS_CLIENT_FROM_POINTER         71
#define RESIZE_CLIENT_BORDER_LEFT         72
#define RESIZE_CLIENT_BORDER_RIGHT        73
#define RESIZE_CLIENT_BORDER_TOP          74
#define RESIZE_CLIENT_BORDER_BOTTOM       75
#define RESIZE_CLIENT_BORDER_TOP_LEFT     76
#define RESIZE_CLIENT_BORDER_TOP_RIGHT    77
#define RESIZE_CLIENT_BORDER_BOTTOM_RIGHT 78
#define RESIZE_CLIENT_BORDER_BOTTOM_LEFT  79
#define HIDE_CONTEXT_MENU                 80
#define CLIENT_SIGNAL                     98
#define ROOT_SIGNAL                       99

enum EV : uint8_t {
    REMOVE_ALL                        = 0,
    EXPOSE                            = XCB_EXPOSE,
    ENTER_NOTIFY                      = XCB_ENTER_NOTIFY,
    LEAVE_NOTIFY                      = XCB_LEAVE_NOTIFY,
    PROPERTY_NOTIFY                   = XCB_PROPERTY_NOTIFY,
    MAP_REQ                           = XCB_MAP_REQUEST,
    MAP_NOTIFY                        = XCB_MAP_NOTIFY,
    KEY_PRESS                         = XCB_KEY_PRESS,
    FOCUS_IN                          = XCB_FOCUS_IN,
    FOCUS_OUT                         = XCB_FOCUS_OUT,
    KEY_RELESE                        = XCB_KEY_RELEASE,
    BUTTON_RELEASE                    = XCB_BUTTON_RELEASE,
    MOTION_NOTIFY                     = XCB_MOTION_NOTIFY,
    DESTROY_NOTIFY                    = XCB_DESTROY_NOTIFY,
    REPARENT_NOTIFY                   = XCB_REPARENT_NOTIFY,
    CONFIGURE_REQUEST                 = XCB_CONFIGURE_REQUEST,

};

typedef struct __client__data__t__{
    client *c = nullptr;
    __client__data__t__&operator=(client *const &__c) { this->c = __c; return *this; }

    __client__data__t__(client *__c) : c(__c) {}
    
} client_data_t;

class __c_func_arr__ {
    /* Defines   */
        #define C_SIGNAL_WDATA(__cb, __sig) \
            signal_manager->client_arr.add_func_to_sig_wdata([this](client *__c, void *__data){__cb}, __sig)

        #define C_SIGNAL(__cb, __sig) \
            signal_manager->client_arr.add_func_to_sig([this](client *__c){__cb}, __sig)

        #define C_EMIT(__c, __sig) \
            signal_manager->client_arr.send_c_sig(__c, __sig)

        #define C_EMIT_DATA(__c, __sig, __data) \
            signal_manager->client_arr.send_c_sig_w_data(__c, __sig, __data)

    /* Variabels */
        static constexpr uint8_t ERROR_STATE = make_T_MAX<uint8_t>();

    /* Methods */
        constexpr uint8_t sig_to_index__(uint8_t __sig) {
            switch (__sig) {
                case BUTTON_MAXWIN_PRESS:               return 0;
                case KILL_SIGNAL:                       return 1;
                case FOCUS_CLIENT:                      return 2;
                case MOVE_CLIENT_MOUSE:                 return 3;
                case TILE_LEFT:                         return 4;
                case TILE_RIGHT:                        return 5;
                case TILE_DOWN:                         return 6;
                case TILE_UP:                           return 7;
                case EWMH_MAXWIN_SIGNAL:                return 8;
                case RESIZE_CLIENT_BORDER_LEFT:         return 9;
                case RESIZE_CLIENT_BORDER_RIGHT:        return 10;
                case RESIZE_CLIENT_BORDER_TOP:          return 11;
                case RESIZE_CLIENT_BORDER_BOTTOM:       return 12;
                case RESIZE_CLIENT_BORDER_TOP_LEFT:     return 13;
                case RESIZE_CLIENT_BORDER_TOP_RIGHT:    return 14;
                case RESIZE_CLIENT_BORDER_BOTTOM_RIGHT: return 15;
                case RESIZE_CLIENT_BORDER_BOTTOM_LEFT:  return 16;

                default: return make_T_MAX<uint8_t>();

            }

        }

    public:
    /* Variabels */
        FixedArray<function<void(client *c)>, 17> func;

    /* Methods   */
        constexpr void send_c_sig(client *__c, int __sig) {
            uint8_t index = sig_to_index__(__sig);
            if (index == ERROR_STATE) return;
            func[index](__c);

        }

        template<typename Callback>
        constexpr void add_func_to_sig(Callback &&__callback, uint8_t __sig) {
            uint8_t index = sig_to_index__(__sig);
            if (index == ERROR_STATE) return;
            func[index] = std::forward<function<void(client *)>>(__callback);

        }

};

class __root_func_arr__ {
    /* Defines   */
        #define ROOT_SIG(__cb, __sig) \
            signal_manager->client_arr.add_func_to_sig([this](client *__c){__cb}, __sig)

        #define ROOT_EMIT(__c, __sig) \
            signal_manager->client_arr.send_c_sig(__c, __sig)

    /* Variabels */
        static constexpr uint8_t ERROR_STATE = make_T_MAX<uint8_t>();

    /* Methods */
        uint8_t sig_to_index__(uint8_t __sig) {
            switch (__sig) {
                case BUTTON_MAXWIN_PRESS: return 0;
                case KILL_SIGNAL:         return 1;
                case FOCUS_CLIENT:        return 2;
                case MOVE_CLIENT_ALT:     return 3;
                case MOVE_CLIENT_MOUSE:   return 4;
                case TILE_LEFT:           return 5;
                case TILE_RIGHT:          return 6;
                case TILE_DOWN:           return 7;
                case TILE_UP:             return 8;
                case EWMH_MAXWIN_SIGNAL:  return 9;

                default: return make_T_MAX<uint8_t>();

            }

        }

    public:
    /* Variabels */
        FixedArray<function<void(window &w)>, 10> func;

    /* Methods   */
        void send_root_sig(window &__w, int __sig) {
            uint8_t index = sig_to_index__(__sig);
            if (index == ERROR_STATE) return;
            func[index](__w);

        }

        template<typename Callback>
        void add_func_to_sig(Callback &&__callback, uint8_t __sig) {
            uint8_t index = sig_to_index__(__sig);
            if (index == ERROR_STATE) return;
            func[index] = std::forward<function<void(client *)>>(__callback);

        }

};

typedef enum {
    SET_EV_CALLBACK__RESIZE_NO_BORDER = 1,
    HIDE_DOCK                         = 2

} enum_signal_t;

enum SET_COLOR
{
    RAW
};

struct rgb_color_code
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

enum COLOR
{
    BLACK,
    WHITE,
    RED,
    GREEN,
    BLUE,
    BLUE_2,
    BLUE_3,
    BLUE_4,
    BLUE_5,
    BLUE_6,
    BLUE_7,
    BLUE_8,
    BLUE_9,
    BLUE_10,
    YELLOW,
    MAGENTA,
    CYAN,
    GREY,
    DARK_GREY,
    DARK_GREY_2,
    DARK_GREY_3,
    DARK_GREY_4,
    LIGHT_GREY,
    ORANGE,
    BROWN,
    PINK,
    PURPLE,
    NO_COLOR,
    DEFAULT_COLOR = DARK_GREY
};

typedef enum Error_codes {
    OK                          = 0,
    CONN_ERR                    = 1,
    EXTENTION_NOT_SUPPORTED_ERR = 2,
    MEMORY_INSUFFICIENT_ERR     = 3,
    REQUEST_TO_LONG_ERR         = 4,
    PARSE_ERR                   = 5,
    SCREEN_NOT_FOUND_ERR        = 6,
    FD_ERR                      = 7
} Error_codes;

enum class TILEPOS {
    LEFT        = 1,
    RIGHT       = 2,
    LEFT_DOWN   = 3,
    RIGHT_DOWN  = 4,
    LEFT_UP     = 5,
    RIGHT_UP    = 6
};

enum class TILE {
    LEFT    = 1 ,
    RIGHT   = 2 ,
    DOWN    = 3 ,
    UP      = 4
};

enum TILE_ANIMATION
{
    TILE_ANIMATION_DURATION = 80
};

typedef enum Decor_data {
    BORDER_SIZE      = 4,
    DOCK_BORDER      = 2,
    TITLE_BAR_HEIGHT = 20,
    BUTTON_SIZE      = TITLE_BAR_HEIGHT
} Decor_data;

enum MAXWIN_ANIMATION {
    MAXWIN_ANIMATION_DURATION = 80
};
enum show_hide {
    SHOW,
    HIDE
};
enum class edge {
    NONE,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM_edge,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};
enum Direction {
    NEXT,
    PREV
};
enum class MAX {
    X       = 1 ,
    Y       = 2 ,
    WIDTH   = 3 ,
    HEIGHT  = 4
};
struct window_borders {
    int16_t left, right, top, bottom;
};
struct win_data {
    xcb_window_t win;
    uint16_t x, y, width, height;
};
enum class CURSOR
{
    arrow,
    hand1,
    hand2,
    watch,
    xterm,
    cross,
    left_ptr,
    right_ptr,
    center_ptr,
    sb_v_double_arrow,
    sb_h_double_arrow,
    fleur,
    question_arrow,
    pirate,
    coffee_mug,
    umbrella,
    circle,
    xsb_left_arrow,
    xsb_right_arrow,
    xsb_up_arrow,
    xsb_down_arrow,
    top_left_corner,
    top_right_corner,
    bottom_left_corner,
    bottom_right_corner,
    sb_left_arrow,
    sb_right_arrow,
    sb_up_arrow,
    sb_down_arrow,
    top_side,
    bottom_side,
    left_side,
    right_side,
    top_tee,
    bottom_tee,
    left_tee,
    right_tee,
    top_left_arrow,
    top_right_arrow,
    bottom_left_arrow,
    bottom_right_arrow
};
enum class config : uint32_t {
    x = 1,
    y = 2,
    width = 4,
    height = 8
};
typedef enum mwm_config_t {
    MWM_CONFIG_x      = 1,
    MWM_CONFIG_y      = 2,
    MWM_CONFIG_width  = 4,
    MWM_CONFIG_height = 8

} mwm_config_t;

#endif