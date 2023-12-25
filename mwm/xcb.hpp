#ifndef XCB_HPP
#define XCB_HPP
#include "include.hpp"

class XCB {
    public:
        void 
        map_window(xcb_connection_t * & conn, const xcb_window_t & win, const std::string & func)
        {
            cookie = xcb_map_window
            (
                conn, 
                win
            );
            
            error = xcb_request_check
            (
                conn, 
                cookie
            );
            
            if (error)
            {
                log.log(ERROR, func, "FAILED TO MAP WINDOW");
                free(error);
            }
        }
        
        void 
        flush(xcb_connection_t * & conn, const std::string & func)
        {
            const uint8_t & status = xcb_flush(conn);
            if (status <= 0) 
            {
                log.log(ERROR, func, "Failed to flush request to server");
                return;
            }
        }

    private:
        Logger log;
        xcb_void_cookie_t cookie;
        xcb_generic_error_t * error ;
};

#endif