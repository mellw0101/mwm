#ifndef CONTEX_MENY_HPP
#define CONTEX_MENY_HPP

#include "structs.hpp"
#include "window.hpp"
#include <functional>
#include "pointer.hpp"

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
        void
        show()
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

            window.x_y_height((size_pos.x - BORDER_SIZE), (size_pos.y - BORDER_SIZE), height);
            window.map();
            xcb_flush(conn);
            window.raise();
            make_entries();
            run();
        }

        void 
        addEntry(const char * name, std::function<void()> action) 
        {
            DialogEntry entry;
            entry.add_name(name);
            entry.add_action(action);
            entries.push_back(entry);
        }
    ;

    private: // private subclasses 
        class DialogEntry 
        {
            public: // constructor 
                DialogEntry() {}
            ;

            public: // variabels 
                window window;
                int16_t border_size = 1;
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

            private:
                const char * entryName;
                std::function<void()> entryAction;
            ;
        };
    ;

    private: // private variables 
        window window;
        size_pos size_pos;
        window_borders border;
        int border_size = 1;
        std::vector<DialogEntry> entries;
        pointer pointer;
    ;
    
    private: // private methods 
        void
        create_dialog_win()
        {
            window.create_default(screen->root, 0, 0, size_pos.width, size_pos.height);
            uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION;
            window.apply_event_mask(& mask);
            window.set_backround_color(DARK_GREY);
            window.raise();
        }
        
        void
        hide()
        {
            window.unmap();
            window.kill();
        }
        
        void
        run()
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

        int
        get_num_of_entries()
        {
            int n = 0;
            for (const auto & entry : entries)
            {
                ++n;
            }
            return n;
        }

        void
        run_action(const xcb_window_t * w) 
        {
            for (const auto & entry : entries) 
            {
                if (* w == entry.window)
                {
                    entry.activate();
                }
            }
        }

        void
        make_entries()
        {
            int y = 0;
            for (auto & entry : entries)
            {
                entry.make_window(window, (0 + BORDER_SIZE), (y + BORDER_SIZE), (size_pos.width - (BORDER_SIZE * 2)), (size_pos.height - (BORDER_SIZE * 2)));
                entry.window.draw_text(entry.getName(), WHITE, BLACK, "7x14", 2, 14);
                y += size_pos.height;
            }
        }
    ;
};

#endif