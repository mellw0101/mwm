/* DEFENITIONS_HPP */
#ifndef DEFENITIONS_HPP
#define DEFENITIONS_HPP

/* main.cpp */
#include "tools.hpp"
#include <cstdarg>
#define conn_err()  error::conn_error(conn ,__func__)
#define check_cookie(cookie)  error::cookie_error(cookie, __func__)

/* // Example function that processes a fixed number of arguments
std::string processStrings(int count, ...)
{
    va_list args;
    va_start(args, count);

    std::string result;
    for (int i = 0; i < count; ++i)
    {
        char* str = va_arg(args, char*);
        result += str;
    }
    va_end(args);

    return result;
} */

// Function that takes a count and a variable number of char* arguments
inline const char *
processStrings(int count, ...)
{
    va_list args;
    va_start(args, count);
    char buf[200];

    int length = 0;
    for (int i = 0; i < count; ++i)
    {
        int start = length;
        length += slen(va_arg(args, char *));
        for (int j = start; j < length; ++j)
        {
            buf[j] = va_arg(args, char *)[j - start];
        }
        
        if (i == count - 1)
        {
            buf[length] = '\0';
        }
    }
    va_end(args);

    const char *s = buf;
    return s;
}

// Macro that forwards its arguments to the processStrings function
#define sctr(...) processStrings(sizeof((const char*[]){__VA_ARGS__})/sizeof(char*), __VA_ARGS__)

#define CheckVoidC(__cookie, __s) \
do { \
    xcb_generic_error_t *err = xcb_request_check(conn, __cookie); \
    if ( err ) \
    { \
        loutE << ERRNO_MSG(__s) << loutEND; \
        free(err); \
    } \
 \
} while (0)

/* #define sctr(...) \
do { \
     \
} while (0) */

/* LOG DEFENITIONS */
#define LOG_ev_response_type(ev)    	    Log::xcb_event_response_type(__func__, ev);
#define LOG_func                    	    Log::FUNC(__func__);
#define LOG_error(message)          	    Log::ERROR(__FUNCTION__, "]:[", message);
#define LOG_info(message)           	    Log::INFO(__FUNCTION__, "]:[", message);
#define LOG_f_info(message)         	    Log::FUNC_INFO(__FUNCTION__, "]:[", message);
#define LOG_warning(message)        	    Log::WARNING(__FUNCTION__, "]:[", message);
#define LOG_start()                 	    Log::Start("mwm");
#define log_event_response_typ()		    Log::xcb_event_response_type(__func__, e->response_type)
#define log_info(message)                   logger.log(INFO, __func__, message)
#define log_error(message)                  logger.log(ERROR, __FUNCTION__, message)
#define log_error_long(message, message_2)                  log.log(ERROR, __FUNCTION__, message, message_2)
#define log_error_code(message, err_code)   logger.log(ERROR, __FUNCTION__, message, err_code)
#define log_win(win_name ,window)           logger.log(INFO, __func__, win_name + std::to_string(window))
#define log_window(window)                  logger.log(INFO, __func__, std::to_string(window))
#define log_func                            logger.log(FUNC, __func__)
#define log_num(__num_var_name, __num_var) logger.log(INFO, __func__, __num_var_name, __num_var)

/* MOD_MASK DEFENITIONS */
#define SHIFT   XCB_MOD_MASK_SHIFT
#define ALT     XCB_MOD_MASK_1
#define CTRL    XCB_MOD_MASK_CONTROL
#define SUPER   XCB_MOD_MASK_4

/* KEY DEFENITIONS */
#define A       0x61
#define B       0x62
#define C       0x63
#define D       0x64
#define E       0x65
#define F       0x66
#define G       0x67
#define H       0x68
#define I       0x69
#define J       0x6a
#define K       0x6b
#define L       0x6c
#define M       0x6d
#define _N      0x6e
#define O       0x6f
#define P       0x70
#define Q       0x71
#define R       0x72
#define S       0x73
#define T       0x74
#define U       0x75
#define V       0x76
#define W       0x77
#define _X      0x78
#define _Y      0x79
#define Z       0x7a

#define SPACE_BAR   0x20
#define ENTER   0xff0d
#define DELETE  0xff08

#define F11     0xffc8
#define F12     0xffc9
#define N_1     0x31
#define N_2     0x32
#define N_3     0x33
#define N_4     0x34
#define N_5     0x35
#define L_ARROW 0xff51
#define U_ARROW 0xff52
#define R_ARROW 0xff53
#define D_ARROW 0xff54
#define TAB     0xff09
#define SEMI    0x3b
#define QUOTE   0x22
#define COMMA   0x2c
#define DOT     0x2e
#define SLASH   0x2f
#define ESC     0xff1b
#define SUPER_L 0xffeb
#define MINUS   0x2d
#define UNDERSCORE 0x5f

/* BUTTON DEFENITIONS */
#define L_MOUSE_BUTTON XCB_BUTTON_INDEX_1
#define R_MOUSE_BUTTON XCB_BUTTON_INDEX_3

#endif/*DEFENITIONS_HPP*/