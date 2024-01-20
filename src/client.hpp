#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <vector>
#include <xcb/randr.h>
#include <thread>

#include "Log.hpp"
#include "defenitions.hpp"
#include "structs.hpp"
#include "window.hpp"

class Threads {
    public:
        template <typename Callable>
        void addThread(Callable&& func) {
            threads.emplace_back(std::thread(std::forward<Callable>(func)));
        }
        void joinAll() {
            for (auto& thread : threads) 
            {
                if (thread.joinable()) 
                {
                    thread.join();
                }
            }
        }
        // Destructor to ensure all threads are joined
        ~Threads() {
            joinAll();
        }
    ;
    private:
        std::vector<std::thread> threads;
    ;
};
class client {
    public: // subclasses
        class client_border_decor {
            public:    
                window left;
                window right;
                window top;
                window bottom;

                window top_left;
                window top_right;
                window bottom_left;
                window bottom_right;
            ;
        };
    ;
    public: // variabels
        window win;
        window frame;
        window titlebar;
        window close_button;
        window max_button;
        window min_button;

        client_border_decor border;

        int16_t x, y;     
        uint16_t width,height;
        uint8_t  depth;
        
        size_pos ogsize;
        size_pos tile_ogsize;
        size_pos max_ewmh_ogsize;
        size_pos max_button_ogsize;

        uint16_t desktop;
    ;
    public: // methods
        public: // main methods
            void make_decorations() {
                make_frame();
                Threads t;
                t.addThread([this](){
                    make_titlebar();
                });
                t.addThread([this](){
                    make_close_button();
                });
                t.addThread([this](){
                    make_max_button();
                });
                t.addThread([this](){
                    make_min_button();
                });
                
                if (BORDER_SIZE > 0)
                {
                    t.addThread([this](){
                        make_borders();
                    });
                }
            }
            void raise() {
                frame.raise();
            }
            void focus() {
                win.focus_input();
                frame.raise();
            }
            void update() {
                x = frame.x();
                y = frame.y();
                width = frame.width();
                height = frame.height();
            }
            void map() {
                frame.map();
            }
            void unmap() {
                frame.unmap();
            }
            void kill() {
                win.unmap();
                close_button.unmap();
                max_button.unmap();
                min_button.unmap();
                titlebar.unmap();
                border.left.unmap();
                border.right.unmap();
                border.top.unmap();
                border.bottom.unmap();
                border.top_left.unmap();
                border.top_right.unmap();
                border.bottom_left.unmap();
                border.bottom_right.unmap();
                frame.unmap();

                win.kill();
                close_button.kill();
                max_button.kill();
                min_button.kill();
                titlebar.kill();
                border.left.kill();
                border.right.kill();
                border.top.kill();
                border.bottom.kill();
                border.top_left.kill();
                border.top_right.kill();
                border.bottom_left.kill();
                border.bottom_right.kill();
                frame.kill();
            }
        ;
        public: // config methods
            void x_y(const int32_t & x, const uint32_t & y) {
                frame.x_y(x, y);
            }
            void _width(const uint32_t & width) {
                win.width((width - (BORDER_SIZE * 2)));
                xcb_flush(conn);
                frame.width((width));
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border.right.x((width - BORDER_SIZE));
                border.top.width((width - (BORDER_SIZE * 2)));
                border.bottom.width((width - (BORDER_SIZE * 2)));
                border.top_right.x((width - BORDER_SIZE));
                border.bottom_right.x((width - BORDER_SIZE));
                xcb_flush(conn);
            }
            void _height(const uint32_t & height) {
                win.height((height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                frame.height(height);
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.height((height - (BORDER_SIZE * 2)));
                border.bottom.y((height - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                border.bottom_right.y((height - BORDER_SIZE));
            }
            void x_width(const uint32_t & x, const uint32_t & width) {
                win.width((width - (BORDER_SIZE * 2)));
                xcb_flush(conn);
                frame.x_width(x, (width));
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border.right.x((width - BORDER_SIZE));
                border.top.width((width - (BORDER_SIZE * 2)));
                border.bottom.width((width - (BORDER_SIZE * 2)));
                border.top_right.x((width - BORDER_SIZE));
                border.bottom_right.x((width - BORDER_SIZE));
                xcb_flush(conn);
            }
            void y_height(const uint32_t & y, const uint32_t & height) {
                win.height((height - TITLE_BAR_HEIGHT) - (BORDER_SIZE * 2));
                xcb_flush(conn);
                frame.y_height(y, height);
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.height((height - (BORDER_SIZE * 2)));
                border.bottom.y((height - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                border.bottom_right.y((height - BORDER_SIZE));
                xcb_flush(conn);
            }
            void x_width_height(const uint32_t & x, const uint32_t & width, const uint32_t & height) {
                frame.x_width_height(x, width, height);
                win.width_height((width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                titlebar.width((width - BORDER_SIZE));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border.top.width((width - (BORDER_SIZE * 2)));
                border.bottom.y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border.top_right.x((width - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                border.bottom_right.x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
            }
            void y_width_height(const uint32_t & y, const uint32_t & width, const uint32_t & height) {
                frame.y_width_height(y, width, height);
                win.width_height((width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                titlebar.width((width - BORDER_SIZE));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border.top.width((width - (BORDER_SIZE * 2)));
                border.bottom.y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border.top_right.x((width - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                border.bottom_right.x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
            }
            void x_y_width_height(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height) {
                win.width_height((width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                xcb_flush(conn);
                frame.x_y_width_height(x, y, width, height);
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border.top.width((width - (BORDER_SIZE * 2)));
                border.bottom.y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border.top_right.x((width - BORDER_SIZE));
                border.bottom_right.x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                xcb_flush(conn);
            }
            void width_height(const uint32_t & width, const uint32_t & height) {
                win.width_height((width - (BORDER_SIZE * 2)), (height - (BORDER_SIZE * 2) - TITLE_BAR_HEIGHT));
                xcb_flush(conn);
                frame.width_height(width, height);
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border.top.width((width - (BORDER_SIZE * 2)));
                border.bottom.y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border.top_right.x((width - BORDER_SIZE));
                border.bottom_right.x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                xcb_flush(conn);
            }
        ;
    ;
    private: // functions
        void make_frame() {
            frame.create_default(screen->root, (x - BORDER_SIZE), (y - TITLE_BAR_HEIGHT - BORDER_SIZE), (width + (BORDER_SIZE * 2)), (height + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)));
            frame.set_backround_color(DARK_GREY);
            win.reparent(frame, BORDER_SIZE, (TITLE_BAR_HEIGHT + BORDER_SIZE));
            frame.apply_event_mask({XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY});
            frame.map();
        }
        void make_titlebar() {
            titlebar.create_default(frame, BORDER_SIZE, BORDER_SIZE, width, TITLE_BAR_HEIGHT);
            titlebar.set_backround_color(BLACK);
            titlebar.grab_button({ { L_MOUSE_BUTTON, NULL } });
            titlebar.map();
        }
        void make_close_button() {
            close_button.create_default(frame, (width - BUTTON_SIZE + BORDER_SIZE), BORDER_SIZE, BUTTON_SIZE, BUTTON_SIZE);
            close_button.apply_event_mask({XCB_EVENT_MASK_ENTER_WINDOW, XCB_EVENT_MASK_LEAVE_WINDOW});
            close_button.set_backround_color(BLUE);
            close_button.grab_button({ { L_MOUSE_BUTTON, NULL } });
            close_button.map();
            close_button.make_then_set_png("/home/mellw/close.png", CLOSE_BUTTON_BITMAP);
        }
        void make_max_button() {
            max_button.create_default(frame, (width - (BUTTON_SIZE * 2) + BORDER_SIZE), BORDER_SIZE, BUTTON_SIZE, BUTTON_SIZE);
            max_button.set_backround_color(RED);
            max_button.apply_event_mask({XCB_EVENT_MASK_ENTER_WINDOW, XCB_EVENT_MASK_LEAVE_WINDOW});
            max_button.grab_button({ { L_MOUSE_BUTTON, NULL } });
            max_button.map();

            Bitmap bitmap(20, 20);
            bitmap.modify(4, 4, 16, true);
            bitmap.modify(5, 4, 5, true);
            bitmap.modify(5, 15, 16, true);
            bitmap.modify(6, 4, 5, true);
            bitmap.modify(6, 15, 16, true);
            bitmap.modify(7, 4, 5, true);
            bitmap.modify(7, 15, 16, true);
            bitmap.modify(8, 4, 5, true);
            bitmap.modify(8, 15, 16, true);
            bitmap.modify(9, 4, 5, true);
            bitmap.modify(9, 15, 16, true);
            bitmap.modify(10, 4, 5, true);
            bitmap.modify(10, 15, 16, true);
            bitmap.modify(11, 4, 5, true);
            bitmap.modify(11, 15, 16, true);
            bitmap.modify(12, 4, 5, true);
            bitmap.modify(12, 15, 16, true);
            bitmap.modify(13, 4, 5, true);
            bitmap.modify(13, 15, 16, true);
            bitmap.modify(14, 4, 5, true);
            bitmap.modify(14, 15, 16, true);
            bitmap.modify(15, 4, 16, true);
            bitmap.exportToPng("/home/mellw/max.png");

            max_button.set_backround_png("/home/mellw/max.png");
        }
        void make_min_button() {
            min_button.create_default(frame, (width - (BUTTON_SIZE * 3) + BORDER_SIZE), BORDER_SIZE, BUTTON_SIZE, BUTTON_SIZE);
            min_button.set_backround_color(GREEN);
            min_button.apply_event_mask({XCB_EVENT_MASK_ENTER_WINDOW, XCB_EVENT_MASK_LEAVE_WINDOW});
            min_button.grab_button({ { L_MOUSE_BUTTON, NULL } });
            min_button.map();

            Bitmap bitmap(20, 20);            
            bitmap.modify(9, 4, 16, true);
            bitmap.modify(10, 4, 16, true);
            bitmap.exportToPng("/home/mellw/min.png");

            min_button.set_backround_png("/home/mellw/min.png");
        }
        void make_borders() {
            border.left.create_default(frame, 0, BORDER_SIZE, BORDER_SIZE, (height + TITLE_BAR_HEIGHT));
            border.left.set_backround_color(BLACK);
            border.left.set_pointer(CURSOR::left_side);
            border.left.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.left.map();

            border.right.create_default(frame, (width + BORDER_SIZE), BORDER_SIZE, BORDER_SIZE, (height + TITLE_BAR_HEIGHT));
            border.right.set_backround_color(BLACK);
            border.right.set_pointer(CURSOR::right_side);
            border.right.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.right.map();

            border.top.create_default(frame, BORDER_SIZE, 0, width, BORDER_SIZE);
            border.top.set_backround_color(BLACK);
            border.top.set_pointer(CURSOR::top_side);
            border.top.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.top.map();

            border.bottom.create_default(frame, BORDER_SIZE, (height + TITLE_BAR_HEIGHT + BORDER_SIZE), width, BORDER_SIZE);
            border.bottom.set_backround_color(BLACK);
            border.bottom.set_pointer(CURSOR::bottom_side);
            border.bottom.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.bottom.map();

            border.top_left.create_default(frame, 0, 0, BORDER_SIZE, BORDER_SIZE);
            border.top_left.set_backround_color(BLACK);
            border.top_left.set_pointer(CURSOR::top_left_corner);
            border.top_left.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.top_left.map();

            border.top_right.create_default(frame, (width + BORDER_SIZE), 0, BORDER_SIZE, BORDER_SIZE);
            border.top_right.set_backround_color(BLACK);
            border.top_right.set_pointer(CURSOR::top_right_corner);
            border.top_right.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.top_right.map();
            
            border.bottom_left.create_default(frame, 0, (height + TITLE_BAR_HEIGHT + BORDER_SIZE), BORDER_SIZE, BORDER_SIZE);
            border.bottom_left.set_backround_color(BLACK);
            border.bottom_left.set_pointer(CURSOR::bottom_left_corner);
            border.bottom_left.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.bottom_left.map();

            border.bottom_right.create_default(frame, (width + BORDER_SIZE), (height + TITLE_BAR_HEIGHT + BORDER_SIZE), BORDER_SIZE, BORDER_SIZE);
            border.bottom_right.set_backround_color(BLACK);
            border.bottom_right.set_pointer(CURSOR::bottom_right_corner);
            border.bottom_right.grab_button({ { L_MOUSE_BUTTON, NULL } });
            border.bottom_right.map();
        }
    ;
    private: // variables
        std::vector<std::vector<bool>> CLOSE_BUTTON_BITMAP =
        {
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0},
            {0,0,0,0,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0},
            {0,0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,0,0},
            {0,0,0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,0},
            {0,0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,0,0},
            {0,0,0,0,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0},
            {0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
        };

        Logger log;
    ;
};

#endif