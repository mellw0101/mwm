#ifndef CONTEX_MENY_HPP
#define CONTEX_MENY_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "launcher.hpp"
#include "structs.hpp"
#include "window.hpp"
#include "pointer.hpp"

class Entry 
{
    public: // constructor
        Entry() {}
    ;
    public: // variabels
        window window;
        bool menu = false;
    ;
    public: // public methods
        void
        add_name(const char * name)
        {
            entryName = name;
        }

        void
        add_action(std::function<void()> action)
        {
            entryAction = action;
        }

        void 
        activate() const 
        {
            LOG_func
            entryAction();
        }

        const char * 
        getName() const 
        {
            return entryName;
        }

        void 
        make_window(const xcb_window_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height)
        {
            window.create_default(parent_window, x, y, width, height);
            window.set_backround_color(BLACK);
            uint32_t mask = XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
            window.apply_event_mask(& mask);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.map();
        }
    ;
    private: // vatiabels
        const char * entryName;
        std::function<void()> entryAction;
    ;
};

class context_menu
{
    public: // consructor
        context_menu()
        {
            size_pos.x      = pointer.x();
            size_pos.y      = pointer.y();
            size_pos.width  = 120;
            size_pos.height = 20;

            border.left = size_pos.x;
            border.right = (size_pos.x + size_pos.width);
            border.top = size_pos.y;
            border.bottom = (size_pos.y + size_pos.height);

            create_dialog_win();
        }
    ;
    public: // public methods
        void show()
        {
            size_pos.x = pointer.x();
            size_pos.y = pointer.y();
            
            uint32_t height = entries.size() * size_pos.height;
            if (size_pos.y + height > screen->height_in_pixels)
            {
                size_pos.y = (screen->height_in_pixels - height);
            }

            if (size_pos.x + size_pos.width > screen->width_in_pixels)
            {
                size_pos.x = (screen->width_in_pixels - size_pos.width);
            }

            context_window.x_y_height((size_pos.x - BORDER_SIZE), (size_pos.y - BORDER_SIZE), height);
            context_window.map();
            context_window.raise();
            make_entries();
            run();
        }
        void add_entry(const char * name, std::function<void()> action)
        {
            Entry entry;
            entry.add_name(name);
            entry.add_action(action);
            entries.push_back(entry);
        }
    ;
    private: // private variables
        window context_window;
        size_pos size_pos;
        window_borders border;
        int border_size = 1;
        
        std::vector<Entry> entries;
        pointer pointer;
        Launcher launcher;
    ;
    private: // private methods
        void create_dialog_win()
        {
            context_window.create_default(screen->root, 0, 0, size_pos.width, size_pos.height);
            uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION;
            context_window.apply_event_mask(& mask);
            context_window.set_backround_color(DARK_GREY);
            context_window.raise();
        }
        void hide()
        {
            context_window.unmap();
            context_window.kill();
        }
        void run()
        {
            xcb_generic_event_t * ev;
            bool shouldContinue = true;

            while (shouldContinue) 
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) 
                {
                    continue;
                }

                switch (ev->response_type & ~0x80)
                {
                    case XCB_BUTTON_PRESS:
                    {
                        const auto & e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                        if (e->detail == L_MOUSE_BUTTON)
                        {
                            run_action(& e->event);
                            shouldContinue = false;
                            hide();
                        }
                        break;
                    }
                    case XCB_ENTER_NOTIFY: 
                    {
                        const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
                        if (e->event == screen->root)
                        {
                            shouldContinue = false;
                            hide();
                        } 
                        break;
                    }
                }
                free(ev); 
            }
        }
        void configure_events()
        {
            
        }
        void run_action(const xcb_window_t * w) 
        {
            for (const auto & entry : entries)
            {
                if (* w == entry.window)
                {
                    entry.activate();
                }
            }
        }
        void make_entries()
        {
            int y = 0;
            for (auto & entry : entries)
            {
                entry.make_window(context_window, (0 + (BORDER_SIZE / 2)), (y + (BORDER_SIZE / 2)), (size_pos.width - BORDER_SIZE), (size_pos.height - BORDER_SIZE));
                entry.window.draw_text(entry.getName(), WHITE, BLACK, "7x14", 2, 14);
                y += size_pos.height;
            }
        }
    ;
};

#endif