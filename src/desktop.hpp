#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <cstdlib>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
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
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Log.hpp"
#include "defenitions.hpp"
#include "structs.hpp"
#include "window.hpp"
#include "client.hpp"
#include <vector>

class desktop
{
    public:
        std::vector<client *> current_clients;
        uint16_t desktop;
        const uint16_t x = 0;
        const uint16_t y = 0;
        uint16_t width;
        uint16_t height;
    ;
};

class change_desktop
{
    public:
        enum DIRECTION
        {
            NEXT,
            PREV
        };

        enum DURATION
        {
            DURATION = 100
        };

        change_desktop(xcb_connection_t * connection) 
        : connection(connection) {}

        void 
        change_to(const DIRECTION & direction)
        {
            switch (direction)
            {
                case NEXT:
                {
                    if (cur_d->desktop == desktop_list.size())
                    {
                        return;
                    }

                    hide = get_clients_on_desktop(cur_d->desktop);
                    show = get_clients_on_desktop(cur_d->desktop + 1);
                    animate(show, NEXT);
                    animate(hide, NEXT);
                    cur_d = desktop_list[cur_d->desktop];
                    break;
                }
                case PREV:
                {
                    if (cur_d->desktop == 1)
                    {
                        return;
                    }

                    hide = get_clients_on_desktop(cur_d->desktop);
                    show = get_clients_on_desktop(cur_d->desktop - 1);
                    animate(show, PREV);
                    animate(hide, PREV);
                    cur_d = desktop_list[cur_d->desktop - 2];
                    break;
                }
            }
            mtx.lock();
            thread_sleep(DURATION + 20);
            joinAndClearThreads();
            mtx.unlock();
        }

        static void
        teleport_to(const uint8_t & n)
        {
            if (cur_d == desktop_list[n - 1] || n == 0 || n == desktop_list.size())
            {
                return;
            }
            
            for (const auto & c : cur_d->current_clients)
            {
                if (c)
                {
                    if (c->desktop == cur_d->desktop)
                    {
                        c->unmap();
                    }
                }
            }

            cur_d = desktop_list[n - 1];
            for (const auto & c : cur_d->current_clients)
            {
                if (c)
                {
                    c->map();
                }
            }
        }
    ;

    private:
        xcb_connection_t * connection;

        std::vector<client *> show;
        std::vector<client *> hide;

        std::thread show_thread;
        std::thread hide_thread;

        std::atomic<bool> stop_show_flag{false};
        std::atomic<bool> stop_hide_flag{false};
        std::atomic<bool> reverse_animation_flag{false};

        std::vector<std::thread> animation_threads;

        std::vector<client *> 
        get_clients_on_desktop(const uint8_t & desktop)
        {
            std::vector<client *> clients;
            for (const auto & c : client_list)
            {
                if (c->desktop == desktop)
                {
                    clients.push_back(c);
                }
            }
            return clients;
        }

        void
        animate(std::vector<client *> clients, const DIRECTION & direction)
        {
            switch (direction) 
            {
                case NEXT:
                {
                    for (const auto c : clients)
                    {
                        if (c)
                        {
                            // std::thread t(&change_desktop::anim_cli, this, c, c->x - screen->width_in_pixels);
                            // t.detach();
                            // animation_threads.emplace_back(&change_desktop::anim_cli, this, c, calculateEndX(c, direction));
                            animation_threads.emplace_back(&change_desktop::anim_cli, this, c, c->x - screen->width_in_pixels);
                        }
                    }
                    break;
                }
                case PREV:
                {
                    for (const auto & c : clients)
                    {
                        if (c)
                        {
                            // std::thread t(&change_desktop::anim_cli, this, c, c->x + screen->width_in_pixels);
                            // t.detach();
                            animation_threads.emplace_back(&change_desktop::anim_cli, this, c, c->x + screen->width_in_pixels);
                        }
                    }
                    break;
                }
            }
        }

        void
        anim_cli(client * c, const int & endx)
        {
            XCPPBAnimator anim(conn, c);
            anim.animate_client_x(c->x, endx, DURATION);
            c->update();
        }

        void
        thread_sleep(const double & milliseconds) 
        {
            // Creating a duration with double milliseconds
            auto duration = std::chrono::duration<double, std::milli>(milliseconds);

            // Sleeping for the duration
            std::this_thread::sleep_for(duration);
        }

        void
        stopAnimations() 
        {
            stop_show_flag.store(true);
            stop_hide_flag.store(true);
            
            if (show_thread.joinable()) 
            {
                show_thread.join();
                stop_show_flag.store(false);
            }

            if (hide_thread.joinable()) 
            {
                hide_thread.join();
                stop_hide_flag.store(false);
            }
        }

        void 
        joinAndClearThreads()
        {
            for (std::thread & t : animation_threads)
            {
                if (t.joinable()) 
                {
                    t.join();
                }
            }
            animation_threads.clear();
            show.clear();
            hide.clear();

            std::vector<std::thread>().swap(animation_threads);
            std::vector<client *>().swap(show);
            std::vector<client *>().swap(hide);
        }
    ;
};


#endif