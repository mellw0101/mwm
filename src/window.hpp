#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <cstdlib>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <utility>
#include <vector>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_xrm.h>
#include <xcb/render.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <xcb/xcb.h>
#include <unistd.h>     // For fork() and exec()
#include <sys/wait.h>   // For waitpid()
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xcb/xcb_cursor.h> /* For cursor */
#include <xcb/xcb_icccm.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <png.h>
#include <xcb/xcb_image.h>
#include <Imlib2.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <X11/Xauth.h>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>

#include "Log.hpp"
#include "defenitions.hpp"
#include "structs.hpp"

class Bitmap
{
    public: // constructor 
        Bitmap(int width, int height) 
        : width(width), height(height), bitmap(height, std::vector<bool>(width, false)) {}

        Logger log;
    ;

    public: // methods 
        void 
        modify(int row, int startCol, int endCol, bool value) 
        {
            if (row < 0 || row >= height || startCol < 0 || endCol > width) 
            {
                log_error("Invalid row or column indices");
            }
        
            for (int i = startCol; i < endCol; ++i) 
            {
                bitmap[row][i] = value;
            }
        }

        void 
        exportToPng(const char * file_name) const
        {
            FILE * fp = fopen(file_name, "wb");
            if (!fp) 
            {
                // log_error("Failed to create PNG file");
                return;
            }

            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr) 
            {
                fclose(fp);
                // log_error("Failed to create PNG write struct");
                return;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr) 
            {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, nullptr);
                // log_error("Failed to create PNG info struct");
                return;
            }

            if (setjmp(png_jmpbuf(png_ptr))) 
            {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                // log_error("Error during PNG creation");
                return;
            }

            png_init_io(png_ptr, fp);
            png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
            png_write_info(png_ptr, info_ptr);

            png_bytep row = new png_byte[width];
            for (int y = 0; y < height; y++) 
            {
                for (int x = 0; x < width; x++) 
                {
                    row[x] = bitmap[y][x] ? 0xFF : 0x00;
                }
                png_write_row(png_ptr, row);
            }
            delete[] row;

            png_write_end(png_ptr, nullptr);

            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
        }
    ;
    
    private: // variables
        int width, height;
        std::vector<std::vector<bool>> bitmap;
    ;
};

class _scale
{
    public:
        static uint16_t
        from_8_to_16_bit(const uint8_t & n)
        {
            return (n << 8) | n;
        }
    ;
};

class window
{
    public: // construcers and operators 
        window() {}

        operator uint32_t() const 
        {
            return _window;
        }

        // Overload the assignment operator for uint32_t
        window& operator=(uint32_t new_window) 
        {
            _window = new_window;
            return *this;
        }
    ;

    public: // methods 
        public: // main methods 
            void
            create( 
                const uint8_t  & depth,
                const uint32_t & parent,
                const int16_t  & x,
                const int16_t  & y,
                const uint16_t & width,
                const uint16_t & height,
                const uint16_t & border_width,
                const uint16_t & _class,
                const uint32_t & visual,
                const uint32_t & value_mask,
                const void     * value_list)
            { 
                _depth = depth;
                _parent = parent;
                _x = x;
                _y = y;
                _width = width;
                _height = height;
                _border_width = border_width;
                __class = _class;
                _visual = visual;
                _value_mask = value_mask;
                _value_list = value_list;

                make_window();
            }

            void
            create_default(const uint32_t & parent, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height)
            {
                _depth = 0L;
                _parent = parent;
                _x = x;
                _y = y;
                _width = width;
                _height = height;
                _border_width = 0;
                __class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual = screen->root_visual;
                _value_mask = 0;
                _value_list = nullptr;

                make_window();
            }

            void  
            raise() 
            {
                xcb_configure_window
                (
                    conn,
                    _window,
                    XCB_CONFIG_WINDOW_STACK_MODE, 
                    (const uint32_t[1])
                    {
                        XCB_STACK_MODE_ABOVE
                    }
                );
                xcb_flush(conn);
            }

            void
            map()
            {
                xcb_map_window(conn, _window);
                xcb_flush(conn);
            }

            void
            unmap()
            {
                xcb_unmap_window(conn, _window);
                xcb_flush(conn);
            }

            void
            reparent(const uint32_t & new_parent, const int16_t & x, const int16_t & y)
            {
                xcb_reparent_window
                (
                    conn, 
                    _window, 
                    new_parent, 
                    x, 
                    y
                );
                xcb_flush(conn);
            }

            void 
            kill() 
            {
                xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, 12, "WM_PROTOCOLS");
                xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, NULL);

                xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
                xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply(conn, delete_cookie, NULL);

                if (!protocols_reply)
                {
                    log_error("protocols reply is null");
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }
                if (!delete_reply)
                {
                    log_error("delete reply is null");
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }

                send_event
                (
                    make_client_message_event
                    (
                        32,
                        protocols_reply->atom,
                        delete_reply->atom
                    )
                );

                free(protocols_reply);
                free(delete_reply);

                xcb_flush(conn);
            }

            void
            clear()
            {
                xcb_clear_area
                (
                    conn, 
                    0,
                    _window,
                    0, 
                    0,
                    _width,
                    _height
                );
                xcb_flush(conn);
            }

            void
            focus_input()
            {
                xcb_set_input_focus
                (
                    conn, 
                    XCB_INPUT_FOCUS_POINTER_ROOT, 
                    _window, 
                    XCB_CURRENT_TIME
                );
                xcb_flush(conn);
            }

            // void /**
            //  * @brief Animates the position and size of an object from a starting point to an ending point.
            //  * 
            //  * @param startX The starting X coordinate.
            //  * @param startY The starting Y coordinate.
            //  * @param startWidth The starting width.
            //  * @param startHeight The starting height.
            //  * @param endX The ending X coordinate.
            //  * @param endY The ending Y coordinate.
            //  * @param endWidth The ending width.
            //  * @param endHeight The ending height.
            //  * @param duration The duration of the animation in milliseconds.
            //  */
            // animate(int endX, int endY, int endWidth, int endHeight, int duration) 
            // {
            //     /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            //     stopAnimations();
                
            //     /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            //     currentX      = _x;
            //     currentY      = _y;
            //     currentWidth  = _width;
            //     currentHeight = _height;

            //     int steps = duration; 

            //     /**
            //      * @brief Calculate if the step is positive or negative for each property.
            //      *
            //      * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
            //      * This is determined by dividing the absolute value of the difference between the start and end values
            //      * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
            //      * is moving in a positive (increasing) or negative (decreasing) direction for each property.
            //      */
            //     stepX      = std::abs(endX - _x)           / (endX - _x);
            //     stepY      = std::abs(endY - _y)           / (endY - _y);
            //     stepWidth  = std::abs(endWidth - _width)   / (endWidth - _width);
            //     stepHeight = std::abs(endHeight - _height) / (endHeight - _height);

            //     /**
            //      * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
            //      * 
            //      * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
            //      * 
            //      * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
            //      * This ensures that each thread will iterate each pixel from start to end value,
            //      * ensuring that all threads will complete at the same time.
            //      */
            //     XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - _x));
            //     YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - _y)); 
            //     WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - _width));
            //     HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - _height)); 

            //     /* START ANIMATION THREADS */
            //     XAnimationThread = std::thread(&window::XAnimation, this, endX);
            //     YAnimationThread = std::thread(&window::YAnimation, this, endY);
            //     WAnimationThread = std::thread(&window::WAnimation, this, endWidth);
            //     HAnimationThread = std::thread(&window::HAnimation, this, endHeight);

            //     /* WAIT FOR ANIMATION TO COMPLETE */
            //     std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            //     /* STOP THE ANIMATION */
            //     stopAnimations();
            // }
        ;

        public: // check methods 
            bool 
            check_if_EWMH_fullscreen() 
            {
                xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(ewmh, _window);
                xcb_ewmh_get_atoms_reply_t wm_state;
                if (xcb_ewmh_get_wm_state_reply(ewmh, cookie, &wm_state, NULL) == 1) 
                {
                    for (unsigned int i = 0; i < wm_state.atoms_len; i++) 
                    {
                        if (wm_state.atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN) 
                        {
                            xcb_ewmh_get_atoms_reply_wipe(&wm_state);
                            return true;
                        }
                    }
                    xcb_ewmh_get_atoms_reply_wipe(&wm_state);
                }
                return false;
            }

            bool
            check_if_active_EWMH_window()
            {
                uint32_t active_window = 0;
                xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, 0), &active_window, NULL);
                return _window == active_window;
            }
            
            uint32_t
            check_event_mask_sum() 
            {
                uint32_t mask = 0;

                // Get the window attributes
                xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(conn, _window);
                xcb_get_window_attributes_reply_t * attr_reply = xcb_get_window_attributes_reply(conn, attr_cookie, NULL);

                // Check if the reply is valid
                if (attr_reply) 
                {
                    mask = attr_reply->all_event_masks;
                    free(attr_reply);
                }
                else 
                {
                    log_error("Unable to get window attributes.");
                }
                return mask;
            }

            std::vector<xcb_event_mask_t>
            check_event_mask_codes()
            {
                uint32_t maskSum = check_event_mask_sum();
                std::vector<xcb_event_mask_t> setMasks;
                for (int mask = XCB_EVENT_MASK_KEY_PRESS; mask <= XCB_EVENT_MASK_OWNER_GRAB_BUTTON; mask <<= 1) 
                {
                    if (maskSum & mask) 
                    {
                        setMasks.push_back(static_cast<xcb_event_mask_t>(mask));
                    }
                }
                return setMasks;
            }

            bool
            check_if_mask_is_active(const uint32_t & event_mask)
            {
                std::vector<xcb_event_mask_t> masks = check_event_mask_codes();
                for (const auto & ev_mask : masks)
                {
                    if (ev_mask == event_mask)
                    {
                        return true;
                    }
                }
                return false;
            }
        ;

        public: // set methods 
            void
            set_active_EWMH_window()
            {
                xcb_ewmh_set_active_window(ewmh, 0, _window); // 0 for the first (default) screen
                xcb_flush(conn);
            }

            void
            set_EWMH_fullscreen_state()
            {
                xcb_change_property
                (
                    conn,
                    XCB_PROP_MODE_REPLACE,
                    _window,
                    ewmh->_NET_WM_STATE,
                    XCB_ATOM_ATOM,
                    32,
                    1,
                    &ewmh->_NET_WM_STATE_FULLSCREEN
                );
                xcb_flush(conn);
            }
        ;

        public: // unset methods 
            void
            unset_EWMH_fullscreen_state()
            {
                xcb_change_property
                (
                    conn,
                    XCB_PROP_MODE_REPLACE,
                    _window,
                    ewmh->_NET_WM_STATE, 
                    XCB_ATOM_ATOM,
                    32,
                    0,
                    0
                );
                xcb_flush(conn);
            }
        ;

        public: // get methods 
            char *
            property(const char * atom_name) 
            {
                xcb_get_property_reply_t *reply;
                unsigned int reply_len;
                char * propertyValue;

                reply = xcb_get_property_reply
                (
                    conn,
                    xcb_get_property
                    (
                        conn,
                        false,
                        _window,
                        atom
                        (
                            atom_name
                        ),
                        XCB_GET_PROPERTY_TYPE_ANY,
                        0,
                        60
                    ),
                    NULL
                );

                if (!reply || xcb_get_property_value_length(reply) == 0) 
                {
                    if (reply != nullptr) 
                    {
                        log_error("reply length for property(" + std::string(atom_name) + ") = 0");
                        free(reply);
                        return (char *) "";
                    }

                    log_error("reply == nullptr");
                    return (char *) "";
                }

                reply_len = xcb_get_property_value_length(reply);
                propertyValue = static_cast<char *>(malloc(sizeof(char) * (reply_len + 1)));
                memcpy(propertyValue, xcb_get_property_value(reply), reply_len);
                propertyValue[reply_len] = '\0';

                if (reply) 
                {
                    free(reply);
                }

                log_info("property(" + std::string(atom_name) + ") = " + std::string(propertyValue));
                return propertyValue;
            }

            uint32_t 
            root_window() 
            {
                xcb_query_tree_cookie_t cookie;
                xcb_query_tree_reply_t *reply;

                cookie = xcb_query_tree(conn, _window);
                reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (!reply) 
                {
                    log_error("Unable to query the window tree.");
                    return (uint32_t) 0; // Invalid window ID
                }

                uint32_t root_window = reply->root;
                free(reply);
                return root_window;
            }

            uint32_t
            parent() 
            {
                xcb_query_tree_cookie_t cookie;
                xcb_query_tree_reply_t *reply;

                cookie = xcb_query_tree(conn, _window);
                reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (!reply) 
                {
                    log_error("Unable to query the window tree.");
                    return (uint32_t) 0; // Invalid window ID
                }

                uint32_t parent_window = reply->parent;
                free(reply);
                return parent_window;
            }

            uint32_t *
            children(uint32_t * child_count) 
            {
                * child_count = 0;
                xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, _window);
                xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (!reply) 
                {
                    log_error("Unable to query the window tree.");
                    return nullptr;
                }

                * child_count = xcb_query_tree_children_length(reply);
                uint32_t * children = static_cast<uint32_t *>(malloc(* child_count * sizeof(xcb_window_t)));

                if (!children) 
                {
                    log_error("Unable to allocate memory for children.");
                    free(reply);
                    return nullptr;
                }

                memcpy(children, xcb_query_tree_children(reply), * child_count * sizeof(uint32_t));
                free(reply);
                return children;
            }
        ;

        public: // configuration methods 
            void 
            apply_event_mask(const std::vector<uint32_t> & values) 
            {
                if (values.empty()) 
                {
                    log_error("values vector is empty");
                    return;
                }

                xcb_change_window_attributes
                (
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    values.data()
                );

                xcb_flush(conn);
            }

            void
            apply_event_mask(const uint32_t * mask)
            {
                xcb_change_window_attributes
                (
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    mask
                );
            }
            
            void
            grab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
            {
                for (const auto & binding : bindings)
                {
                    const uint8_t & button = binding.first;
                    const uint16_t & modifier = binding.second;
                    xcb_grab_button
                    (
                        conn, 
                        1, 
                        _window, 
                        XCB_EVENT_MASK_BUTTON_PRESS, 
                        XCB_GRAB_MODE_ASYNC, 
                        XCB_GRAB_MODE_ASYNC, 
                        XCB_NONE, 
                        XCB_NONE, 
                        button, 
                        modifier    
                    );
                    xcb_flush(conn); 
                }
            }

            void
            ungrab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
            {
                for (const auto & binding : bindings)
                {
                    const uint8_t & button = binding.first;
                    const uint16_t & modifier = binding.second;
                    xcb_ungrab_button
                    (
                        conn,
                        button,
                        _window,
                        modifier
                    );
                }
                xcb_flush(conn); // Flush the request to the X server
            }

            void 
            grab_keys(std::initializer_list<std::pair<const uint32_t, const uint16_t>> bindings) 
            {
                xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(conn);
            
                if (!keysyms) 
                {
                    log_error("keysyms could not get initialized");
                    return;
                }

                for (const auto & binding : bindings) 
                {
                    xcb_keycode_t * keycodes = xcb_key_symbols_get_keycode(keysyms, binding.first);
                    if (keycodes)
                    {
                        for (auto * kc = keycodes; * kc; kc++) 
                        {
                            xcb_grab_key
                            (
                                conn,
                                1,
                                _window,
                                binding.second, 
                                *kc,        
                                XCB_GRAB_MODE_ASYNC, 
                                XCB_GRAB_MODE_ASYNC  
                            );
                        }
                        free(keycodes);
                    }
                }
                xcb_key_symbols_free(keysyms);

                xcb_flush(conn); 
            }

            void
            set_pointer(CURSOR cursor_type) 
            {
                xcb_cursor_context_t * ctx;

                if (xcb_cursor_context_new(conn, screen, &ctx) < 0) 
                {
                    log_error("Unable to create cursor context.");
                    return;
                }

                xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, pointer_from_enum(cursor_type));
                if (!cursor) 
                {
                    log_error("Unable to load cursor.");
                    return;
                }

                xcb_change_window_attributes
                (
                    conn, 
                    _window, 
                    XCB_CW_CURSOR, 
                    (uint32_t[1])
                    {
                        cursor 
                    }
                );
                xcb_flush(conn);

                xcb_cursor_context_free(ctx);
                xcb_free_cursor(conn, cursor);
            }

            void 
            draw_text(const char * str , const COLOR & text_color, const COLOR & backround_color, const char * font_name, const int16_t & x, const int16_t & y)
            {
                get_font(font_name);
                create_font_gc(text_color, backround_color, font);
                xcb_image_text_8
                (
                    conn, 
                    strlen(str), 
                    _window, 
                    font_gc,
                    x, 
                    y, 
                    str
                );
                xcb_flush(conn);
            }

            public: // size_pos configuration methods 
                public: // fetch methods
                    uint32_t
                    x()
                    {
                        return _x;
                    }
                    
                    uint32_t
                    y()
                    {
                        return _y;
                    }

                    uint32_t
                    width()
                    {
                        return _width;
                    }

                    uint32_t
                    height()
                    {
                        return _height;
                    }
                ;    
                
                void
                x(const uint32_t & x)
                {
                    config_window(MWM_CONFIG_x, x);
                    update(x, _y, _width, _height);
                }

                void
                y(const uint32_t & y)
                {
                    config_window(XCB_CONFIG_WINDOW_Y, y);
                    update(_x, y, _width, _height);
                }                

                void
                width(const uint32_t & width)
                {
                    config_window(MWM_CONFIG_width, width);
                    update(_x, _y, width, _height);
                }

                void
                height(const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, height);
                    update(_x, _y, _width, height);
                }

                void
                x_y(const uint32_t & x, const uint32_t & y)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, {x, y});
                    update(x, y, _width, _height);
                }

                void
                width_height(const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {width, height});
                    update(_x, _y, width, height);
                }

                void
                x_y_width_height(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {x, y, width, height});
                    update(x, y, width, height);
                }

                void
                x_width_height(const uint32_t & x, const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {x, width, height});
                    update(x, _y, width, height);
                }

                void
                y_width_height(const uint32_t & y, const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {y, width, height});
                    update(_x, y, width, height);
                }

                void
                x_width(const uint32_t & x, const uint32_t & width)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, {x, width});
                    update(x, _y, width, _height);
                }

                void
                x_height(const uint32_t & x, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_HEIGHT, {x, height});
                    update(x, _y, _width, height);
                }
                
                void
                y_width(const uint32_t & y, const uint32_t & width)
                {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, {y, width});
                    update(_x, y, width, _height);
                }
                
                void 
                y_height(const uint32_t & y, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, {y, height});
                    update(_x, y, _width, height);
                }

                void
                x_y_width(const uint32_t & x, const uint32_t & y, const uint32_t & width)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, {x, y, width});
                    update(x, y, width, _height);
                }

                void
                x_y_height(const uint32_t & x, const uint32_t & y, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, {x, y, height});
                    update(x, y, _width, height);
                }
            ;

            public: // backround methods 
                void
                set_backround_color(COLOR color)
                {
                    change_back_pixel(get_color(color));
                }
              
                void
                set_backround_color_8_bit(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value)
                {
                    change_back_pixel(get_color(red_value, green_value, blue_value));
                }

                void
                set_backround_color_16_bit(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value)
                {
                    change_back_pixel(get_color(red_value, green_value, blue_value));
                }

                void
                set_backround_png(const char * imagePath)
                {
                    Imlib_Image image = imlib_load_image(imagePath);
                    if (!image) 
                    {
                        log_error("Failed to load image: " + std::string(imagePath));
                        return;
                    }

                    imlib_context_set_image(image);
                    int originalWidth = imlib_image_get_width();
                    int originalHeight = imlib_image_get_height();

                    // Calculate new size maintaining aspect ratio
                    double aspectRatio = (double)originalWidth / originalHeight;
                    int newHeight = _height;
                    int newWidth = (int)(newHeight * aspectRatio);

                    if (newWidth > _width) 
                    {
                        newWidth = _width;
                        newHeight = (int)(newWidth / aspectRatio);
                    }

                    Imlib_Image scaledImage = imlib_create_cropped_scaled_image
                    (
                        0, 
                        0, 
                        originalWidth, 
                        originalHeight, 
                        newWidth, 
                        newHeight
                    );
                    imlib_free_image(); // Free original image
                    imlib_context_set_image(scaledImage);

                    // Get the scaled image data
                    DATA32 * data = imlib_image_get_data();

                    // Create an XCB image from the scaled data
                    xcb_image_t * xcb_image = xcb_image_create_native
                    (
                        conn, 
                        newWidth, 
                        newHeight,
                        XCB_IMAGE_FORMAT_Z_PIXMAP, 
                        screen->root_depth, 
                        NULL, 
                        ~0, (uint8_t*)data
                    );

                    create_pixmap();
                    create_graphics_exposure_gc();
                    xcb_rectangle_t rect = {0, 0, _width, _height};
                    xcb_poly_fill_rectangle
                    (
                        conn, 
                        pixmap, 
                        gc, 
                        1, 
                        &rect
                    );

                    // Calculate position to center the image
                    int x = (_width - newWidth) / 2;
                    int y = (_height - newHeight) / 2;

                    // Put the scaled image onto the pixmap at the calculated position
                    xcb_image_put
                    (
                        conn, 
                        pixmap, 
                        gc, 
                        xcb_image, 
                        x,
                        y, 
                        0
                    );

                    // Set the pixmap as the background of the window
                    xcb_change_window_attributes
                    (
                        conn,
                        _window,
                        XCB_CW_BACK_PIXMAP,
                        &pixmap
                    );

                    // Cleanup
                    xcb_free_gc(conn, gc); // Free the GC
                    xcb_image_destroy(xcb_image);
                    imlib_free_image(); // Free scaled image

                    clear_window();
                }

                void 
                make_then_set_png(const char * file_name, const std::vector<std::vector<bool>>& bitmap) 
                {
                    create_png_from_vector_bitmap(file_name, bitmap);
                    set_backround_png(file_name);
                }
            ;
        ;
    ;

    private: // variables 
        private: // main variables 
            uint8_t        _depth;
            uint32_t       _window;
            uint32_t       _parent;
            int16_t        _x;
            int16_t        _y;
            uint16_t       _width;
            uint16_t       _height;
            uint16_t       _border_width;
            uint16_t       __class;
            uint32_t       _visual;
            uint32_t       _value_mask;
            const void     * _value_list;
        ;

        xcb_gcontext_t gc;
        xcb_gcontext_t font_gc;
        xcb_font_t     font;
        xcb_pixmap_t   pixmap;

        Logger log;

        private: // animation variables 
            // std::thread XAnimationThread{};
            // std::thread YAnimationThread{};
            // std::thread WAnimationThread{};
            // std::thread HAnimationThread{};
            int currentX;
            int currentY;
            int currentWidth;
            int currentHeight;
            int stepX;
            int stepY;
            int stepWidth;
            int stepHeight;
            double GAnimDuration;
            double XAnimDuration;
            double YAnimDuration;
            double WAnimDuration;
            double HAnimDuration;
            // std::atomic<bool> stopXFlag{false};
            // std::atomic<bool> stopYFlag{false};
            // std::atomic<bool> stopWFlag{false};
            // std::atomic<bool> stopHFlag{false};
            std::chrono::high_resolution_clock::time_point XlastUpdateTime;
            std::chrono::high_resolution_clock::time_point YlastUpdateTime;
            std::chrono::high_resolution_clock::time_point WlastUpdateTime;
            std::chrono::high_resolution_clock::time_point HlastUpdateTime;
            const double frameRate = 120;
            const double frameDuration = 1000.0 / frameRate;
        ;
    ;

    private: // functions 
        private: // main functions 
            void
            make_window()
            {
                _window = xcb_generate_id(conn);
                xcb_create_window
                (
                    conn,
                    _depth,
                    _window,
                    _parent,
                    _x,
                    _y,
                    _width,
                    _height,
                    _border_width,
                    __class,
                    _visual,
                    _value_mask,
                    _value_list
                );
                xcb_flush(conn);
            }

            void
            clear_window()
            {
                xcb_clear_area
                (
                    conn, 
                    0,
                    _window,
                    0, 
                    0,
                    _width,
                    _height
                );
                xcb_flush(conn);
            }

            void
            update(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height)
            {
                _x = x;
                _y = y;
                _width = width;
                _height = height;
            }

            void /**
             *
             * @brief Configures the window with the specified mask and value.
             * 
             * This function configures the window using the XCB library. It takes in a mask and a value
             * as parameters and applies the configuration to the window.
             * 
             * @param mask The mask specifying which attributes to configure.
             * @param value The value to set for the specified attributes.
             * 
             */
            config_window(const uint16_t & mask, const uint16_t & value)
            {
                xcb_configure_window
                (
                    conn,
                    _window,
                    mask,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(value)
                    }
                );
            }

            void 
            config_window(uint32_t mask, const std::vector<uint32_t> & values) 
            {
                if (values.empty()) 
                {
                    log_error("values vector is empty");
                    return;
                }

                xcb_configure_window
                (
                    conn,
                    _window,
                    mask,
                    values.data()
                );
            }
        ;

        void
        send_event(xcb_client_message_event_t ev)
        {
            xcb_send_event
            (
                conn,
                0,
                _window,
                XCB_EVENT_MASK_NO_EVENT,
                (char *) & ev
            );
        }

        xcb_client_message_event_t
        make_client_message_event(const uint32_t & format, const uint32_t & type, const uint32_t & data)
        {
            xcb_client_message_event_t ev = {0};
            ev.response_type = XCB_CLIENT_MESSAGE;
            ev.window = _window;
            ev.format = format;
            ev.sequence = 0;
            ev.type = type;
            ev.data.data32[0] = data;
            ev.data.data32[1] = XCB_CURRENT_TIME;

            return ev;
        }

        private: // pointer functions 
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

        private: // create functions 
            private: // gc functions 
                void
                create_graphics_exposure_gc()
                {
                    gc = xcb_generate_id(conn);
                    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
                    uint32_t values[3] =
                    {
                        screen->black_pixel,
                        screen->white_pixel,
                        0
                    };

                    xcb_create_gc
                    (
                        conn,
                        gc,
                        _window,
                        mask,
                        values
                    );
                    xcb_flush(conn);
                }

                void
                create_font_gc(const COLOR & text_color, const COLOR & backround_color, xcb_font_t font)
                {
                    font_gc = xcb_generate_id(conn);

                    xcb_create_gc
                    (
                        conn, 
                        font_gc, 
                        _window, 
                        XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT, 
                        (const uint32_t[3])
                        {
                            get_color(text_color),
                            get_color(backround_color),
                            font
                        }
                    );
                }
            ;

            private: // pixmap functions 
                void
                create_pixmap()
                {
                    pixmap = xcb_generate_id(conn);
                    xcb_create_pixmap
                    (
                        conn, 
                        screen->root_depth, 
                        pixmap, 
                        _window, 
                        _width, 
                        _height
                    );
                    xcb_flush(conn);
                }
            ;

            private: // png functions
                void
                create_png_from_vector_bitmap(const char * file_name, const std::vector<std::vector<bool>> & bitmap)
                {
                    int width = bitmap[0].size();
                    int height = bitmap.size();

                    FILE *fp = fopen(file_name, "wb");
                    if (!fp)
                    {
                        log_error("Failed to open file: " + std::string(file_name));
                        return;
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (!png_ptr)
                    {
                        fclose(fp);
                        log_error("Failed to create PNG write struct");
                        return;
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (!info_ptr)
                    {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, NULL);
                        log_error("Failed to create PNG info struct");
                        return;
                    }

                    if (setjmp(png_jmpbuf(png_ptr))) 
                    {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        log_error("Error during PNG creation");
                        return;
                    }

                    png_init_io(png_ptr, fp);
                    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                    png_write_info(png_ptr, info_ptr);

                    // Write bitmap to PNG
                    png_bytep row = new png_byte[width];
                    for (int y = 0; y < height; y++) 
                    {
                        for (int x = 0; x < width; x++) 
                        {
                            row[x] = bitmap[y][x] ? 0xFF : 0x00;
                        }
                        png_write_row(png_ptr, row);
                    }
                    delete[] row;

                    png_write_end(png_ptr, NULL);

                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                }
            ;
        ;

        private: // get functions 
            xcb_atom_t
            atom(const char * atom_name) 
            {
                xcb_intern_atom_cookie_t cookie = xcb_intern_atom
                (
                    conn, 
                    0, 
                    strlen(atom_name), 
                    atom_name
                );
                
                xcb_intern_atom_reply_t * reply = xcb_intern_atom_reply(conn, cookie, NULL);
                
                if (!reply) 
                {
                    log_error("could not get atom");
                    return XCB_ATOM_NONE;
                } 

                xcb_atom_t atom = reply->atom;
                free(reply);
                return atom;
            }

            std::string 
            AtomName(xcb_atom_t atom) 
            {
                xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
                xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(conn, cookie, nullptr);

                if (!reply) 
                {
                    log_error("reply is nullptr.");
                    return "";
                }

                int name_len = xcb_get_atom_name_name_length(reply);
                char* name = xcb_get_atom_name_name(reply);

                std::string atomName(name, name + name_len);

                free(reply);
                return atomName;
            }

            uint16_t 
            x_from_req()
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                uint16_t x;
                if (geometry) 
                {
                    x = geometry->x;
                    free(geometry);
                } 
                else 
                {
                    x = 200;
                }
                return x;
            }
            
            uint16_t 
            y_from_req()
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                uint16_t y;
                if (geometry) 
                {
                    y = geometry->y;
                    free(geometry);
                } 
                else 
                {
                    y = 200;
                }
                return y;
            }

            uint16_t 
            width_from_req() 
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                uint16_t width;
                if (geometry) 
                {
                    width = geometry->width;
                    free(geometry);
                } 
                else 
                {
                    width = 200;
                }
                return width;
            }
            
            uint16_t 
            height_from_req() 
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                uint16_t height;
                if (geometry) 
                {
                    height = geometry->height;
                    free(geometry);
                } 
                else 
                {
                    height = 200;
                }
                return height;
            }

            void
            get_font(const char * font_name)
            {
                font = xcb_generate_id(conn);
                xcb_open_font
                (
                    conn, 
                    font, 
                    strlen(font_name),
                    font_name
                );
                xcb_flush(conn);
            }
        ;

        private: // backround functions 
            void
            change_back_pixel(const uint32_t & pixel)
            {
                xcb_change_window_attributes
                (
                    conn,
                    _window,
                    XCB_CW_BACK_PIXEL,
                    (const uint32_t[1])
                    {
                        pixel
                    }
                );
                xcb_flush(conn);
            }

            uint32_t
            get_color(COLOR color)
            {
                uint32_t pixel = 0;
                xcb_colormap_t colormap = screen->default_colormap;
                rgb_color_code color_code = rgb_code(color);
                xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply
                (
                    conn, 
                    xcb_alloc_color
                    (
                        conn,
                        colormap,
                        _scale::from_8_to_16_bit(color_code.r), 
                        _scale::from_8_to_16_bit(color_code.g),
                        _scale::from_8_to_16_bit(color_code.b)
                    ), 
                    NULL
                );
                pixel = reply->pixel;
                free(reply);
                return pixel;
            }

            uint32_t
            get_color(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value)
            {
                uint32_t pixel = 0;
                xcb_colormap_t colormap = screen->default_colormap;
                xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply
                (
                    conn, 
                    xcb_alloc_color
                    (
                        conn,
                        colormap,
                        red_value, 
                        green_value,
                        blue_value
                    ), 
                    NULL
                );
                pixel = reply->pixel;
                free(reply);
                return pixel;
            }

            uint32_t
            get_color(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value)
            {
                uint32_t pixel = 0;
                xcb_colormap_t colormap = screen->default_colormap;
                xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply
                (
                    conn, 
                    xcb_alloc_color
                    (
                        conn,
                        colormap,
                        _scale::from_8_to_16_bit(red_value), 
                        _scale::from_8_to_16_bit(green_value),
                        _scale::from_8_to_16_bit(blue_value)
                    ), 
                    NULL
                );
                pixel = reply->pixel;
                free(reply);
                return pixel;
            }

            rgb_color_code
            rgb_code(COLOR COLOR)
            {
                rgb_color_code color;
                uint8_t r;
                uint8_t g;
                uint8_t b;
                
                switch (COLOR) 
                {
                    case COLOR::WHITE:
                        r = 255; g = 255; b = 255;
                        break;
                    ;
                    case COLOR::BLACK:
                        r = 0; g = 0; b = 0;
                        break;
                    ;
                    case COLOR::RED:
                        r = 255; g = 0; b = 0;
                        break;
                    ;
                    case COLOR::GREEN:
                        r = 0; g = 255; b = 0;
                        break;
                    ;
                    case COLOR::BLUE:
                        r = 0; g = 0; b = 255;
                        break;
                    ;
                    case COLOR::YELLOW:
                        r = 255; g = 255; b = 0;
                        break;
                    ;
                    case COLOR::CYAN:
                        r = 0; g = 255; b = 255;
                        break;
                    ;
                    case COLOR::MAGENTA:
                        r = 255; g = 0; b = 255;
                        break;
                    ;
                    case COLOR::GREY:
                        r = 128; g = 128; b = 128;
                        break;
                    ;
                    case COLOR::LIGHT_GREY:
                        r = 192; g = 192; b = 192;
                        break;
                    ;
                    case COLOR::DARK_GREY:
                        r = 64; g = 64; b = 64;
                        break;
                    ;
                    case COLOR::ORANGE:
                        r = 255; g = 165; b = 0;
                        break;
                    ;
                    case COLOR::PURPLE:
                        r = 128; g = 0; b = 128;
                        break;
                    ;
                    case COLOR::BROWN:
                        r = 165; g = 42; b = 42;
                        break;
                    ;
                    case COLOR::PINK:
                        r = 255; g = 192; b = 203;
                        break;
                    ;
                    default:
                        r = 0; g = 0; b = 0; 
                        break;
                    ;
                }

                color.r = r;
                color.g = g;
                color.b = b;
                return color;
            }
        ;

        // private: // animation funcrions 
        //     void /**
        //      *
        //      * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
        //      * 
        //      * This function updates the 'x' of a window in a loop until the current 'x'
        //      * matches the specified end 'x'. It uses the "XStep()" function to incrementally
        //      * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
        //      * 
        //      * @param endX The desired 'x' position of the window.
        //      *
        //      */
        //     XAnimation(const int & endX) 
        //     {
        //         XlastUpdateTime = std::chrono::high_resolution_clock::now();
        //         while (true) 
        //         {
        //             if (currentX == endX) 
        //             {
        //                 x(currentX);
        //                 break;
        //             }
        //             XStep();
        //             thread_sleep(XAnimDuration);
        //         }
        //     }

        //     void /**
        //      * @brief Performs a step in the X direction.
        //      * 
        //      * This function increments the currentX variable by the stepX value.
        //      * If it is time to render, it configures the window's X position using the currentX value.
        //      * 
        //      * @note This function assumes that the connection and window variables are properly initialized.
        //      */
        //     XStep() 
        //     {
        //         currentX += stepX;
                
        //         if (XisTimeToRender())
        //         {
        //             x(currentX);
        //             xcb_flush(conn);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
        //      * 
        //      * This function updates the 'y' of a window in a loop until the current 'y'
        //      * matches the specified end 'y'. It uses the "YStep()" function to incrementally
        //      * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
        //      * 
        //      * @param endY The desired 'y' positon of the window.
        //      *
        //      */
        //     YAnimation(const int & endY) 
        //     {
        //         YlastUpdateTime = std::chrono::high_resolution_clock::now();
        //         while (true) 
        //         {
        //             if (currentY == endY) 
        //             {
        //                 y(endY);
        //                 break;
        //             }
        //             YStep();
        //             thread_sleep(YAnimDuration);
        //         }
        //     }

        //     void /**
        //      * @brief Performs a step in the Y direction.
        //      * 
        //      * This function increments the currentY variable by the stepY value.
        //      * If it is time to render, it configures the window's Y position using xcb_configure_window
        //      * and flushes the connection using xcb_flush.
        //      */
        //     YStep() 
        //     {
        //         currentY += stepY;
                
        //         if (YisTimeToRender())
        //         {
        //             y(currentY);
        //             xcb_flush(conn);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Performs a 'width' animation until the specified end 'width' is reached.
        //      * 
        //      * This function updates the 'width' of a window in a loop until the current 'width'
        //      * matches the specified end 'width'. It uses the "WStep()" function to incrementally
        //      * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
        //      * 
        //      * @param endWidth The desired 'width' of the window.
        //      *
        //      */
        //     WAnimation(const int & endWidth) 
        //     {
        //         WlastUpdateTime = std::chrono::high_resolution_clock::now();
        //         while (true) 
        //         {
        //             if (currentWidth == endWidth) 
        //             {
        //                 width(endWidth);
        //                 break;
        //             }
        //             WStep();
        //             thread_sleep(WAnimDuration);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Performs a step in the width calculation and updates the window width if it is time to render.
        //      * 
        //      * This function increments the current width by the step width. If it is time to render, it configures the window width
        //      * using the XCB library and flushes the connection.
        //      *
        //      */
        //     WStep() 
        //     {
        //         currentWidth += stepWidth;

        //         if (WisTimeToRender())
        //         {
        //             width(currentWidth);
        //             xcb_flush(conn);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Performs a 'height' animation until the specified end 'height' is reached.
        //      * 
        //      * This function updates the 'height' of a window in a loop until the current 'height'
        //      * matches the specified end 'height'. It uses the "HStep()" function to incrementally
        //      * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
        //      * 
        //      * @param endWidth The desired 'height' of the window.
        //      *
        //      */
        //     HAnimation(const int & endHeight) 
        //     {
        //         HlastUpdateTime = std::chrono::high_resolution_clock::now();
        //         while (true) 
        //         {
        //             if (currentHeight == endHeight) 
        //             {
        //                 height(endHeight);
        //                 break;
        //             }
        //             HStep();
        //             thread_sleep(HAnimDuration);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Increases the current height by the step height and updates the window height if it's time to render.
        //      * 
        //      * This function is responsible for incrementing the current height by the step height and updating the window height
        //      * if it's time to render. It uses the xcb_configure_window function to configure the window height and xcb_flush to
        //      * flush the changes to the X server.
        //      *
        //      */
        //     HStep() 
        //     {
        //         currentHeight += stepHeight;
                
        //         if (HisTimeToRender())
        //         {
        //             height(currentHeight);
        //             xcb_flush(conn);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Stops all animations by setting the corresponding flags to true and joining the animation threads.
        //      *        After joining the threads, the flags are set back to false.
        //      *
        //      */
        //     stopAnimations() 
        //     {
        //         stopHFlag.store(true);
        //         stopXFlag.store(true);
        //         stopYFlag.store(true);
        //         stopWFlag.store(true);
        //         stopHFlag.store(true);

                

        //         if (XAnimationThread.joinable()) 
        //         {
        //             XAnimationThread.join();
        //             stopXFlag.store(false);
        //         }

        //         if (YAnimationThread.joinable()) 
        //         {
        //             YAnimationThread.join();
        //             stopYFlag.store(false);
        //         }

        //         if (WAnimationThread.joinable()) 
        //         {
        //             WAnimationThread.join();
        //             stopWFlag.store(false);
        //         }

        //         if (HAnimationThread.joinable()) 
        //         {
        //             HAnimationThread.join();
        //             stopHFlag.store(false);
        //         }
        //     }

        //     void /**
        //      *
        //      * @brief Sleeps the current thread for the specified number of milliseconds.
        //      *
        //      * @param milliseconds The number of milliseconds to sleep. A double is used to allow for
        //      *                     fractional milliseconds, providing finer control over animation timing.
        //      *
        //      * @note This is needed as the time for each thread to sleep is the main thing to be calculated, 
        //      *       as this class is designed to iterate every pixel and then only update that to the X-server at the given framerate.
        //      *
        //      */
        //     thread_sleep(const double & milliseconds) 
        //     {
        //         // Creating a duration with double milliseconds
        //         auto duration = std::chrono::duration<double, std::milli>(milliseconds);

        //         // Sleeping for the duration
        //         std::this_thread::sleep_for(duration);
        //     }

        //     bool /**
        //      *
        //      * Checks if it is time to render based on the elapsed time since the last update.
        //      * @return true if it is time to render, @return false otherwise.
        //      *
        //      */
        //     XisTimeToRender() 
        //     {
        //         // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
        //         const auto & currentTime = std::chrono::high_resolution_clock::now();
        //         const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - XlastUpdateTime;

        //         if (elapsedTime.count() >= frameDuration) 
        //         {
        //             XlastUpdateTime = currentTime; 
        //             return true; 
        //         }
        //         return false; 
        //     }

        //     bool /**
        //      *
        //      * Checks if it is time to render a frame based on the elapsed time since the last update.
        //      * @return true if it is time to render, @return false otherwise.
        //      *
        //      */
        //     YisTimeToRender() 
        //     {
        //         // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
        //         const auto & currentTime = std::chrono::high_resolution_clock::now();
        //         const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - YlastUpdateTime;

        //         if (elapsedTime.count() >= frameDuration) 
        //         {
        //             YlastUpdateTime = currentTime; 
        //             return true; 
        //         }
        //         return false; 
        //     }
            
        //     bool /**
        //      *
        //      * Checks if it is time to render based on the elapsed time since the last update.
        //      * @return true if it is time to render, @return false otherwise.
        //      *
        //      */
        //     WisTimeToRender()
        //     {
        //         // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
        //         const auto & currentTime = std::chrono::high_resolution_clock::now();
        //         const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - WlastUpdateTime;

        //         if (elapsedTime.count() >= frameDuration) 
        //         {
        //             WlastUpdateTime = currentTime; 
        //             return true; 
        //         }
        //         return false; 
        //     }

        //     bool /**
        //      *
        //      * Checks if it is time to render based on the elapsed time since the last update.
        //      * @return true if it is time to render, @return false otherwise.
        //      *
        //      */
        //     HisTimeToRender()
        //     {
        //         // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
        //         const auto & currentTime = std::chrono::high_resolution_clock::now();
        //         const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - HlastUpdateTime;

        //         if (elapsedTime.count() >= frameDuration) 
        //         {
        //             HlastUpdateTime = currentTime; 
        //             return true; 
        //         }
        //         return false; 
        //     }
        // ;
    ;
};

#endif