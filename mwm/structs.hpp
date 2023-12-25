#ifndef STRUCTS_HPP
#define STRUCTS_HPP
#include "include.hpp"

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

enum MAXWIN_ANIMATION 
{
    MAXWIN_ANIMATION_DURATION = 80
};

enum show_hide 
{
    SHOW,
    HIDE
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

struct client 
{
	xcb_window_t win;
    xcb_window_t frame;            
	int16_t x, y;                   
	uint16_t width,height;          
	uint8_t  depth;  
    size_pos ogsize;
    size_pos tile_ogsize;
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