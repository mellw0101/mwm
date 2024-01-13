#ifndef STRUCTS_HPP
#define STRUCTS_HPP
#include "include.hpp"
#include <cstdint>
#include <xcb/xproto.h>

enum 
{
    N = 12, /* 
        THIS IS HOW FAR AWAY SNAPING WILL HAPPEN 
     */ 
    NC = 8 /* 
        THIS IS HOW FAR AWAY WINDOW TO WINDOW CORNER SNAPPING WILL HAPPEN 
     */
};

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
    YELLOW,
    MAGENTA,
    CYAN,
    GREY,
    DARK_GREY,
    LIGHT_GREY,
    ORANGE,
    BROWN,
    PINK,
    PURPLE
};

typedef enum Error_codes 
{
    OK                          = 0,
    CONN_ERR                    = 1,
    EXTENTION_NOT_SUPPORTED_ERR = 2,
    MEMORY_INSUFFICIENT_ERR     = 3,
    REQUEST_TO_LONG_ERR         = 4,
    PARSE_ERR                   = 5,
    SCREEN_NOT_FOUND_ERR        = 6,
    FD_ERR                      = 7
} 
Error_codes;

enum class TILEPOS 
{
    LEFT        = 1,
    RIGHT       = 2,
    LEFT_DOWN   = 3,
    RIGHT_DOWN  = 4,
    LEFT_UP     = 5,
    RIGHT_UP    = 6
};

enum class TILE 
{
    LEFT    = 1 ,
    RIGHT   = 2 ,
    DOWN    = 3 ,
    UP      = 4
};

enum TILE_ANIMATION 
{
    TILE_ANIMATION_DURATION = 80
};

typedef enum Decor_data 
{
    BORDER_SIZE      = 0,
    TITLE_BAR_HEIGHT = 20,
    BUTTON_SIZE      = TITLE_BAR_HEIGHT
} 
Decor_data;

enum MAXWIN_ANIMATION 
{
    MAXWIN_ANIMATION_DURATION = 80
};

enum show_hide 
{
    SHOW,
    HIDE
};

enum class edge
{
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

enum Direction 
{
    NEXT,
    PREV
};

enum class MAX 
{
    X       = 1 ,
    Y       = 2 ,
    WIDTH   = 3 ,
    HEIGHT  = 4
};

struct window_borders
{
    int16_t left, right, top, bottom;
};

struct size_pos 
{
    int16_t x, y;
    uint16_t width, height;
};

struct win_data 
{
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

enum class config : uint32_t
{
    x = 1,
    y = 2,
    width = 4,
    height = 8
};

typedef enum mwm_config_t
{
    MWM_CONFIG_x      = 1,
    MWM_CONFIG_y      = 2,
    MWM_CONFIG_width  = 4,
    MWM_CONFIG_height = 8

} mwm_config_t;

#endif