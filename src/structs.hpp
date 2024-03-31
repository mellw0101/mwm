#ifndef STRUCTS_HPP
#define STRUCTS_HPP
#include "include.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>
// #include <type_traits>

using namespace std;

typedef enum __array_type_flag__ {
    
    _uint32_t      = 1,
    _int16_t       = 2,
    _uint16_t      = 3,
    _float         = 4,
    _double        = 5,
    _ptr           = 6,
    _string        = 7,
    _client        = 8,
    _window        = 9,
    _unknown       = 0

} array_type_flag_t;
typedef array_type_flag_t __ArrayTypeFlag;
using ArrayType = __ArrayTypeFlag;
#define IF_TYPE(__T2) if constexpr (is_same<T1, __T2>::value)
template<typename T1>
static ArrayType __make__type_flag__();
template<> inline ArrayType __make__type_flag__<uint32_t>() { return _uint32_t; }
template<> inline ArrayType __make__type_flag__<int16_t>()  { return _int16_t;  }
template<> inline ArrayType __make__type_flag__<uint16_t>() { return _uint16_t; }
template<> inline ArrayType __make__type_flag__<float>()    { return _float;    }
template<> inline ArrayType __make__type_flag__<double>()   { return _double;   }
template<> inline ArrayType __make__type_flag__<void *>()   { return _ptr;      }
template<> inline ArrayType __make__type_flag__<string>()   { return _string;   }
class client; template<> inline ArrayType __make__type_flag__<client *>() { return _client; }
class window; template<> inline ArrayType __make__type_flag__<window *>() { return _window; }

#define make_type_flag(__type) __make__type_flag__<__type>()
template<typename T1>
constexpr ArrayType type_flag() { return make_type_flag(T1); }
#define __MAKE_TYPE_FLAG__(__name, __type) static constexpr ArrayType __name = type_flag<__type>()
#define __data_array__TF __MAKE_TYPE_FLAG__(_array_type_flag, T1)

template<typename T1, size_t Size>
struct __data_array_t__ {

    static_assert(Size > 0, "Size must be greater than 0.");
    T1 _data[Size];
    size_t _index;

    constexpr T1 &operator[](size_t index)
    {
        assert(index < Size);
        return _data[index];
    }

    constexpr T1 &operator[](size_t index) const
    {
        assert(index < Size);
        return _data[index];
    }

    constexpr size_t size()
    {
        return Size;
    }

    constexpr size_t size() const
    {
        return Size;
    }

    constexpr void add(T1 &__input)
    {
        this->_data[_index] = __input;
        this->_index++;
    }

    constexpr __data_array_t__<T1, Size> &operator=(const __data_array_t__<T1, Size> &__other)
    {
        if (this != &__other)
        {
            for (size_t i = 0; i < Size; ++i)
            {
                _data[i] = __other._data[i];
            }
        }

        return *this;
    }

    constexpr __data_array_t__<T1, Size> (const __data_array_t__<T1, Size> &__other)
    {
        if (this != &__other)
        {
            for (size_t i = 0; i < Size; ++i)
            {
                this->_data[i] = __other._data[i];
            }
        }

        return *this;
    }

    constexpr T1 &operator=(T1 others[Size])
    {
        _data = others;
    }

    constexpr T1 &operator=(T1 &other)
    {

    }

    /* Methods */
        constexpr void clear_arr()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                if
                constexpr (is_pointer<T1>::value)
                {
                   _data[i] = nullptr;
                }
                else if
                constexpr (is_arithmetic<T1>::value)
                {
                    _data[i] = 0;
                }
                else
                {
                    _data[i] = '\0';
                }
            }
        }

        constexpr void clear()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                _data[i] = T1{};
            }
        }

        constexpr void init_arr()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                if
                constexpr (is_pointer<T1>::value)
                {
                   _data[i] = nullptr;
                }
                else if
                constexpr (is_arithmetic<T1>::value)
                {
                    _data[i] = 0;
                }
                else
                {
                    _data[i] = 0;
                }
            }
        }

    /* Constructor */
        __data_array_t__<T1, Size> () { clear(); }
};
template<typename T1, size_t Size>
using Array = __data_array_t__<T1, Size>;

#define FIRST_T(__name)  __MAKE_TYPE_FLAG__(__name, T1)
#define SECOND_T(__name) __MAKE_TYPE_FLAG__(__name, T2)
template<typename T1, typename T2, size_t Size>
struct __pair_array_t__ {

    Array<T1, Size> first;
    FIRST_T(_first_t);
    Array<T2, Size> second;
    SECOND_T(_second_t);

    typedef struct __proxy_accessor_t__ {
        /* Variabels    */
            T1 &_first;
            T2 &_second;

        /* Constructors */
            __proxy_accessor_t__(T1 &__first, T2 &__second)
            : _first(__first), _second(__second) {}

        /* Implicit conversion operators */
            operator T1&() { return _first; }
            operator T2&() { return _second; }

    } proxy_accessor_t;

    proxy_accessor_t operator[](size_t index)
    {
        return first[index], second[index];
    }

};
template<typename T1, typename T2, size_t Size>
using PairArray = __pair_array_t__<T1, T2, Size>;

template<typename T1, typename T2, typename T3, typename T4, size_t Size>
struct __quad_array_t__ {

    Array<T1, Size> first;
    Array<T2, Size> second;
    Array<T3, Size> third;
    Array<T4, Size> fourth;

    typedef struct __proxy_accessor_t__ {
        /* Variabels    */
            T1 &_first;
            T2 &_second;
            T3 &_third;
            T4 &_fourth;

        /* Constructors */
            __proxy_accessor_t__(T1 &__first, T2 &__second, T3 &__third, T4 &__fourth)
            : _first(__first), _second(__second), _third(__third), _fourth(__fourth) {}

        /* Implicit conversion operators */
            operator T1&() { return _first;  }
            operator T2&() { return _second; }
            operator T3&() { return _third;  }
            operator T4&() { return _fourth; }

    } proxy_accessor_t;

    proxy_accessor_t operator[](size_t index)
    {
        return first[index], second[index], third[index], fourth[index];
    }

};
template<typename T1, typename T2, typename T3, typename T4, size_t Size>
using QuadArray = __quad_array_t__<T1, T2, T3, T4, Size>;

enum
{
    N = 12, /* 
        THIS IS HOW FAR AWAY SNAPING WILL HAPPEN 
     */ 
    NC = 8 /* 
        THIS IS HOW FAR AWAY WINDOW TO WINDOW CORNER SNAPPING WILL HAPPEN 
     */
};

enum EV : uint8_t {
    REMOVE_ALL                        = 0,
    EXPOSE                            = XCB_EXPOSE,
    ENTER_NOTIFY                      = XCB_ENTER_NOTIFY,
    LEAVE_NOTIFY                      = XCB_LEAVE_NOTIFY,
    L_MOUSE_BUTTON_EVENT              = 36,
    CLIENT_RESIZE                     = 42,
    KILL_SIGNAL                       = 39,
    L_MOUSE_BUTTON_EVENT__ALT         = 43,
    PROPERTY_NOTIFY                   = XCB_PROPERTY_NOTIFY
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