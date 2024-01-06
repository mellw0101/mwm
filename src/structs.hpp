#ifndef STRUCTS_HPP
#define STRUCTS_HPP
#include "include.hpp"
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
    CONN_ERR = -28,
    OK  = 0
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
    BORDER_SIZE = 2,
    TITLE_BAR_HEIGHT = 20,
    BUTTON_SIZE = TITLE_BAR_HEIGHT
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
    BOTTOM_edge
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

struct size_pos 
{
    uint16_t x, y, width, height;
};

struct client_border_decor
{
    xcb_window_t left;
    xcb_window_t right;
    xcb_window_t top;
    xcb_window_t bottom;
};

struct client 
{
    char name[256];

	xcb_window_t win;
    xcb_window_t frame;
    xcb_window_t titlebar;
    xcb_window_t close_button;
    xcb_window_t max_button;
    xcb_window_t min_button;

    client_border_decor border;

	int16_t x, y;     
	uint16_t width,height;
	uint8_t  depth;
    
    size_pos ogsize;
    size_pos tile_ogsize;
    size_pos max_ewmh_ogsize;
    size_pos max_button_ogsize;
    
    bool ismax;
    uint16_t desktop;
};

struct win_data 
{
    xcb_window_t win;
    uint16_t x, y, width, height;
};

struct desktop 
{
    std::vector<client *> current_clients;
    uint16_t desktop;
    const uint16_t x = 0;
    const uint16_t y = 0;
    uint16_t width;
    uint16_t height;
};

// template <typename T>
// using u_Ptr = std::unique_ptr<T>;
// template <typename T>
// using Vec = std::vector<T>;

// template <typename T>
// using u_Ptr_Vec = std::vector<std::unique_ptr<client>>;

// template <typename T>
// using SPtr = std::shared_ptr<T>;
// template <typename T>
// using WPtr = std::weak_ptr<T>;



#endif