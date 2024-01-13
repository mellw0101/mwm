#ifndef POINTER_HPP
#define POINTER_HPP

#include <stdint.h>
#include <xcb/xcb.h>
#include "structs.hpp"
#include "Log.hpp"

class pointer
{
    public: // methods
        uint32_t
        x()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t * reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply) 
            {
                log_error("reply is nullptr.");
                return 0;                            
            } 

            uint32_t x;
            x = reply->root_x;
            free(reply);
            return x;
        }
        
        uint32_t
        y()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t * reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply) 
            {
                log_error("reply is nullptr.");
                return 0;                            
            } 

            uint32_t y;
            y = reply->root_y;
            free(reply);
            return y;
        }

        void 
        teleport(const int16_t & x, const int16_t & y) 
        {
            xcb_warp_pointer
            (
                conn, 
                XCB_NONE, 
                screen->root, 
                0, 
                0, 
                0, 
                0, 
                x, 
                y
            );
            xcb_flush(conn);
        }

        void
        grab(const xcb_window_t & window)
        {
            xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer
            (
                conn,
                false,
                window,
                XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC,
                XCB_NONE,
                XCB_NONE,
                XCB_CURRENT_TIME
            );

            xcb_grab_pointer_reply_t * reply = xcb_grab_pointer_reply(conn, cookie, nullptr);
            if (!reply)
            {
                log_error("reply is nullptr.");
                free(reply);
                return;
            }
            if (reply->status != XCB_GRAB_STATUS_SUCCESS) 
            {
                log_error("Could not grab pointer");
                free(reply);
                return;
            }

            free(reply);
        }
    ; 
    
    private: // functions
        const char *
        pointer_from_enum(CURSOR CURSOR)
        {
            switch (CURSOR) 
            {
                case CURSOR::arrow: return "arrow";
                case CURSOR::hand1: return "hand1";
                case CURSOR::hand2: return "hand2";
                case CURSOR::watch: return "watch";
                case CURSOR::xterm: return "xterm";
                case CURSOR::cross: return "cross";
                case CURSOR::left_ptr: return "left_ptr";
                case CURSOR::right_ptr: return "right_ptr";
                case CURSOR::center_ptr: return "center_ptr";
                case CURSOR::sb_v_double_arrow: return "sb_v_double_arrow";
                case CURSOR::sb_h_double_arrow: return "sb_h_double_arrow";
                case CURSOR::fleur: return "fleur";
                case CURSOR::question_arrow: return "question_arrow";
                case CURSOR::pirate: return "pirate";
                case CURSOR::coffee_mug: return "coffee_mug";
                case CURSOR::umbrella: return "umbrella";
                case CURSOR::circle: return "circle";
                case CURSOR::xsb_left_arrow: return "xsb_left_arrow";
                case CURSOR::xsb_right_arrow: return "xsb_right_arrow";
                case CURSOR::xsb_up_arrow: return "xsb_up_arrow";
                case CURSOR::xsb_down_arrow: return "xsb_down_arrow";
                case CURSOR::top_left_corner: return "top_left_corner";
                case CURSOR::top_right_corner: return "top_right_corner";
                case CURSOR::bottom_left_corner: return "bottom_left_corner";
                case CURSOR::bottom_right_corner: return "bottom_right_corner";
                case CURSOR::sb_left_arrow: return "sb_left_arrow";
                case CURSOR::sb_right_arrow: return "sb_right_arrow";
                case CURSOR::sb_up_arrow: return "sb_up_arrow";
                case CURSOR::sb_down_arrow: return "sb_down_arrow";
                case CURSOR::top_side: return "top_side";
                case CURSOR::bottom_side: return "bottom_side";
                case CURSOR::left_side: return "left_side";
                case CURSOR::right_side: return "right_side";
                case CURSOR::top_tee: return "top_tee";
                case CURSOR::bottom_tee: return "bottom_tee";
                case CURSOR::left_tee: return "left_tee";
                case CURSOR::right_tee: return "right_tee";
                case CURSOR::top_left_arrow: return "top_left_arrow";
                case CURSOR::top_right_arrow: return "top_right_arrow";
                case CURSOR::bottom_left_arrow: return "bottom_left_arrow";
                case CURSOR::bottom_right_arrow: return "bottom_right_arrow";
                default: return "left_ptr";
            }
        }
    ;

    private: // variables
        Logger log;
    ;
};

#endif