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

#include "Log.hpp"
#include "defenitions.hpp"
#include "structs.hpp"

class Bitmap
{
    public: // constructor 
        Bitmap(int width, int height) 
        : width(width), height(height), bitmap(height, std::vector<bool>(width, false)) {}
    ;
    public: // methods 
        void modify(int row, int startCol, int endCol, bool value) 
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
        void exportToPng(const char * file_name) const
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
        Logger log;
    ;
};
class _scale
{
    public:
        static uint16_t from_8_to_16_bit(const uint8_t & n)
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
            void create( 
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
            void create_default(const uint32_t & parent, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height)
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
            void create_client_window(const uint32_t & parent, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height)
            {
                _window = xcb_generate_id(conn);
                uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
                uint32_t value_list[2];
                value_list[0] = screen->white_pixel;
                value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

                _depth = 0L;
                _parent = parent;
                _x = x;
                _y = y;
                _width = width;
                _height = height;
                _border_width = 0;
                __class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual = screen->root_visual;
                _value_mask = value_mask;
                _value_list = value_list;
                
                make_window();
            }
            void raise() 
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
            void map()
            {
                xcb_map_window(conn, _window);
                xcb_flush(conn);
            }
            void unmap()
            {
                xcb_unmap_window(conn, _window);
                xcb_flush(conn);
            }
            void reparent(const uint32_t & new_parent, const int16_t & x, const int16_t & y)
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
            void kill() 
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
            void clear()
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
            void focus_input()
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
        ;
        public: // check methods
            bool is_EWMH_fullscreen() 
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
            bool is_active_EWMH_window()
            {
                uint32_t active_window = 0;
                xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, 0), &active_window, NULL);
                return _window == active_window;
            }
            uint32_t check_event_mask_sum() 
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
            std::vector<xcb_event_mask_t> check_event_mask_codes()
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
            bool is_mask_active(const uint32_t & event_mask)
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
            void set_active_EWMH_window()
            {
                xcb_ewmh_set_active_window(ewmh, 0, _window); // 0 for the first (default) screen
                xcb_flush(conn);
            }
            void set_EWMH_fullscreen_state()
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
            void unset_EWMH_fullscreen_state()
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
            char * property(const char * atom_name) 
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
            uint32_t root_window() 
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
            uint32_t parent() 
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
            uint32_t * children(uint32_t * child_count) 
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
            void apply_event_mask(const std::vector<uint32_t> & values) 
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
            void apply_event_mask(const uint32_t * mask)
            {
                xcb_change_window_attributes
                (
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    mask
                );
            }
            void set_pointer(CURSOR cursor_type) 
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
            void draw_text(const char * str , const COLOR & text_color, const COLOR & backround_color, const char * font_name, const int16_t & x, const int16_t & y)
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
                    uint32_t x() 
                    {
                        return _x;
                    }
                    uint32_t y()
                    {
                        return _y;
                    }
                    uint32_t width()
                    {
                        return _width;
                    }
                    uint32_t height()
                    {
                        return _height;
                    }
                ;    
                void x(const uint32_t & x)
                {
                    config_window(MWM_CONFIG_x, x);
                    update(x, _y, _width, _height);
                }
                void y(const uint32_t & y)
                {
                    config_window(XCB_CONFIG_WINDOW_Y, y);
                    update(_x, y, _width, _height);
                }
                void width(const uint32_t & width)
                {
                    config_window(MWM_CONFIG_width, width);
                    update(_x, _y, width, _height);
                }
                void height(const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, height);
                    update(_x, _y, _width, height);
                }
                void x_y(const uint32_t & x, const uint32_t & y)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, {x, y});
                    update(x, y, _width, _height);
                }
                void width_height(const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {width, height});
                    update(_x, _y, width, height);
                }
                void x_y_width_height(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {x, y, width, height});
                    update(x, y, width, height);
                }
                void x_width_height(const uint32_t & x, const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {x, width, height});
                    update(x, _y, width, height);
                }
                void y_width_height(const uint32_t & y, const uint32_t & width, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {y, width, height});
                    update(_x, y, width, height);
                }
                void x_width(const uint32_t & x, const uint32_t & width)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, {x, width});
                    update(x, _y, width, _height);
                }
                void x_height(const uint32_t & x, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_HEIGHT, {x, height});
                    update(x, _y, _width, height);
                }
                void y_width(const uint32_t & y, const uint32_t & width)
                {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, {y, width});
                    update(_x, y, width, _height);
                }
                void y_height(const uint32_t & y, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, {y, height});
                    update(_x, y, _width, height);
                }
                void x_y_width(const uint32_t & x, const uint32_t & y, const uint32_t & width)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, {x, y, width});
                    update(x, y, width, _height);
                }
                void x_y_height(const uint32_t & x, const uint32_t & y, const uint32_t & height)
                {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, {x, y, height});
                    update(x, y, _width, height);
                }
            ;
            public: // backround methods
                void set_backround_color(COLOR color)
                {
                    change_back_pixel(get_color(color));
                }
                void set_backround_color_8_bit(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value)
                {
                    change_back_pixel(get_color(red_value, green_value, blue_value));
                }
                void set_backround_color_16_bit(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value)
                {
                    change_back_pixel(get_color(red_value, green_value, blue_value));
                }
                void set_backround_png(const char * imagePath)
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
                void make_then_set_png(const char * file_name, const std::vector<std::vector<bool>>& bitmap) 
                {
                    create_png_from_vector_bitmap(file_name, bitmap);
                    set_backround_png(file_name);
                }
            ;
        ;
        public: // keys
            void grab_default_keys()
            {
                grab_keys(
                {
                    {   T,          ALT | CTRL              }, // for launching terminal
                    {   Q,          ALT | SHIFT             }, // quiting key_binding for mwm_wm
                    {   F11,        NULL                    }, // key_binding for fullscreen
                    {   N_1,        ALT                     },
                    {   N_2,        ALT                     },
                    {   N_3,        ALT                     },
                    {   N_4,        ALT                     },
                    {   N_5,        ALT                     },
                    {   R_ARROW,    CTRL | SUPER            }, // key_binding for moving to the next desktop
                    {   L_ARROW,    CTRL | SUPER            }, // key_binding for moving to the previous desktop
                    {   R_ARROW,    CTRL | SUPER | SHIFT    },
                    {   L_ARROW,    CTRL | SUPER | SHIFT    },
                    {   R_ARROW,    SUPER                   },
                    {   L_ARROW,    SUPER                   },
                    {   U_ARROW,    SUPER                   },
                    {   D_ARROW,    SUPER                   },
                    {   TAB,        ALT                     },
                    {   K,          SUPER                   }
                });
            }
            void grab_keys(std::initializer_list<std::pair<const uint32_t, const uint16_t>> bindings) 
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
            void grab_keys_for_typing()
            {
                grab_keys(
                {
                    { A,   NULL  },
                    { B,   NULL  },
                    { C,   NULL  },
                    { D,   NULL  },
                    { E,   NULL  },
                    { F,   NULL  },
                    { G,   NULL  },
                    { H,   NULL  },
                    { I,   NULL  },
                    { J,   NULL  },
                    { K,  NULL  },
                    { L,  NULL  },
                    { M,  NULL  },
                    { _N, NULL  },
                    { O,  NULL  },
                    { P,  NULL  },
                    { Q,  NULL  },
                    { R,  NULL  },
                    { S,  NULL  },
                    { T,  NULL  },
                    { U,  NULL  },
                    { V,  NULL  },
                    { W,  NULL  },
                    { _X, NULL  },
                    { _Y, NULL  },
                    { Z,  NULL  },

                    { A,  SHIFT },
                    { B,  SHIFT },
                    { C,  SHIFT },
                    { D,  SHIFT },
                    { E,  SHIFT },
                    { F,  SHIFT },
                    { G,  SHIFT },
                    { H,  SHIFT },
                    { I,  SHIFT },
                    { J,  SHIFT },
                    { K,  SHIFT },
                    { L,  SHIFT },
                    { M,  SHIFT },
                    { _N, SHIFT },
                    { O,  SHIFT },
                    { P,  SHIFT },
                    { Q,  SHIFT },
                    { R,  SHIFT },
                    { S,  SHIFT },
                    { T,  SHIFT },
                    { U,  SHIFT },
                    { V,  SHIFT },
                    { W,  SHIFT },
                    { _X, SHIFT },
                    { _Y, SHIFT },
                    { Z,  SHIFT },
                });
            }
        ;
        public: // buttons
            void grab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
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
            void ungrab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
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
        void send_event(xcb_client_message_event_t ev)
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
        xcb_client_message_event_t make_client_message_event(const uint32_t & format, const uint32_t & type, const uint32_t & data)
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
            const char * pointer_from_enum(CURSOR CURSOR)
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
                void create_graphics_exposure_gc()
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
                void create_font_gc(const COLOR & text_color, const COLOR & backround_color, xcb_font_t font)
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
                void create_pixmap()
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
                void create_png_from_vector_bitmap(const char * file_name, const std::vector<std::vector<bool>> & bitmap)
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
            xcb_atom_t atom(const char * atom_name) 
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
            std::string AtomName(xcb_atom_t atom) 
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
            uint16_t x_from_req()
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
            uint16_t y_from_req()
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
            uint16_t width_from_req() 
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
            uint16_t height_from_req() 
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
            void get_font(const char * font_name)
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
            void change_back_pixel(const uint32_t & pixel)
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
            uint32_t get_color(COLOR color)
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
            uint32_t get_color(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value)
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
            uint32_t get_color(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value)
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
            rgb_color_code rgb_code(COLOR COLOR)
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
    ;
};

#endif