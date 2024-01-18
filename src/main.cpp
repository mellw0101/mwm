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
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex> // Added for thread safety
#include <iostream>
#include <png.h>
#include <xcb/xcb_image.h>
#include <Imlib2.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <X11/Xauth.h>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include "Log.hpp"
#include "contex_meny.hpp"
Logger log;

#include "defenitions.hpp"
#include "structs.hpp"

#include "window.hpp"
#include "client.hpp"
#include "dock.hpp"
#include "desktop.hpp"
#include "event_handler.hpp"
#include "window_manager.hpp"
#include "mwm_animator.hpp"

static pointer * pointer;
static Dock * dock;

class mxb 
{
    public: 
        class XConnection 
        {
            public:
                struct mxb_auth_info_t 
                {
                    int namelen;
                    char* name;
                    int datalen;
                    char* data;
                };

                XConnection(const char * display) 
                {
                    std::string socketPath = getSocketPath(display);

                    // Initialize address
                    memset(&addr, 0, sizeof(addr));
                    addr.sun_family = AF_UNIX;
                    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

                    // Create socket
                    fd = socket(AF_UNIX, SOCK_STREAM, 0);
                    if (fd == -1) 
                    {
                        throw std::runtime_error("Failed to create socket");
                    }

                    // Connect to the X server's socket
                    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) 
                    {
                        ::close(fd);
                        throw std::runtime_error("Failed to connect to X server");
                    }

                    // Perform authentication
                    int displayNumber = parseDisplayNumber(display);
                    if (!authenticate_x11_connection(displayNumber, auth_info)) 
                    {
                        ::close(fd);
                        throw std::runtime_error("Failed to authenticate with X server");
                    }
                }

                ~XConnection() 
                {
                    if (fd != -1) 
                    {
                        ::close(fd);
                    }
                    delete[] auth_info.name;
                    delete[] auth_info.data;
                }

                int 
                getFd() const 
                {
                    return fd;
                }

                void 
                confirmConnection() 
                {
                    const std::string extensionName = "BIG-REQUESTS";  // Example extension
                    uint16_t nameLength = static_cast<uint16_t>(extensionName.length());

                    // Calculate the total length of the request in 4-byte units, including padding
                    uint16_t requestLength = htons((8 + nameLength + 3) / 4); // Length in 4-byte units
                    char request[32] = {0};  // 32 bytes is enough for most requests
                    request[0] = 98;         // Opcode for QueryExtension
                    request[1] = 0;          // Unused
                    request[2] = (requestLength >> 8) & 0xFF;  // Length (high byte)
                    request[3] = requestLength & 0xFF;         // Length (low byte)
                    request[4] = nameLength & 0xFF;            // Length of the extension name (low byte)
                    request[5] = (nameLength >> 8) & 0xFF;     // Length of the extension name (high byte)
                    std::memcpy(&request[8], extensionName.c_str(), nameLength); // Copy the extension name

                    // Send the request
                    if (send(fd, request, 8 + nameLength, 0) == -1) 
                    {
                        throw std::runtime_error("Failed to send QueryExtension request");
                    }

                    // Prepare to receive the response
                    char reply[32] = {0}; // Buffer to hold the response
                    int received = 0;     // Number of bytes received so far

                    // Read the response from the server
                    while (received < sizeof(reply)) 
                    {
                        int n = recv(fd, reply + received, sizeof(reply) - received, 0);
                        if (n == -1) 
                        {
                            throw std::runtime_error("Failed to receive QueryExtension reply");
                        }
                        else if (n == 0) 
                        {
                            throw std::runtime_error("Connection closed by X server");
                        }
                        received += n;
                    }

                    // Process the reply
                    // Check if the first byte is 1 (indicating a reply)
                    if (reply[0] != 1) 
                    {
                        throw std::runtime_error("Invalid response received from X server");
                    }

                    // Check if the extension is present
                    bool extensionPresent = reply[1];
                    if (extensionPresent) 
                    {
                        log_info("BIG-REQUESTS extension is supported by the X server.");
                    } 
                    else 
                    {
                        log_info("BIG-REQUESTS extension is not supported by the X server.");
                    }
                }

                std::string 
                sendMessage(const std::string & extensionName) 
                {
                    uint16_t nameLength = static_cast<uint16_t>(extensionName.length());

                    // Calculate the total length of the request in 4-byte units, including padding
                    uint16_t requestLength = htons((8 + nameLength + 3) / 4); // Length in 4-byte units
                    char request[32] = {0};  // 32 bytes is enough for most requests
                    request[0] = 98;         // Opcode for QueryExtension
                    request[1] = 0;          // Unused
                    request[2] = (requestLength >> 8) & 0xFF;  // Length (high byte)
                    request[3] = requestLength & 0xFF;         // Length (low byte)
                    request[4] = nameLength & 0xFF;            // Length of the extension name (low byte)
                    request[5] = (nameLength >> 8) & 0xFF;     // Length of the extension name (high byte)
                    std::memcpy(&request[8], extensionName.c_str(), nameLength); // Copy the extension name

                    // Send the request
                    if (send(fd, request, 8 + nameLength, 0) == -1) 
                    {
                        throw std::runtime_error("Failed to send QueryExtension request");
                    }

                    // Prepare to receive the response
                    char reply[32] = {0}; // Buffer to hold the response
                    int received = 0;     // Number of bytes received so far

                    // Read the response from the server
                    while (received < sizeof(reply)) 
                    {
                        int n = recv(fd, reply + received, sizeof(reply) - received, 0);
                        if (n == -1) 
                        {
                            throw std::runtime_error("Failed to receive QueryExtension reply");
                        }
                        else if (n == 0) 
                        {
                            throw std::runtime_error("Connection closed by X server");
                        }
                        received += n;
                    }

                    // Process the reply
                    // Check if the first byte is 1 (indicating a reply)
                    if (reply[0] != 1) 
                    {
                        throw std::runtime_error("Invalid response received from X server");
                    }

                    // Check if the extension is present
                    bool extensionPresent = reply[1];
                    if (extensionPresent) 
                    {
                        return "Extension is supported by the X server.";
                    } 
                    else 
                    {
                        return "Extension is not supported by the X server.";
                    }
                }
            ;

            private:
                int fd;
                struct sockaddr_un addr;
                mxb_auth_info_t auth_info;
                Logger log;

                // Function to authenticate an X11 connection
                bool 
                authenticate_x11_connection(int display_number, mxb_auth_info_t & auth_info) 
                {
                    // Try to get the XAUTHORITY environment variable; fall back to default
                    const char* xauthority_env = std::getenv("XAUTHORITY");
                    std::string xauthority_file = xauthority_env ? xauthority_env : "~/.Xauthority";

                    // Open the Xauthority file
                    FILE* auth_file = fopen(xauthority_file.c_str(), "rb");
                    if (!auth_file) 
                    {
                        // Handle error: Failed to open .Xauthority file
                        return false;
                    }

                    Xauth* xauth_entry;
                    bool found = false;
                    while ((xauth_entry = XauReadAuth(auth_file)) != nullptr) 
                    {
                        // Check if the entry matches your display number
                        // Assuming display_number is the display you're interested in
                        if (std::to_string(display_number) == std::string(xauth_entry->number, xauth_entry->number_length)) 
                        {
                            // Fill the auth_info structure
                            auth_info.namelen = xauth_entry->name_length;
                            auth_info.name = new char[xauth_entry->name_length];
                            std::memcpy(auth_info.name, xauth_entry->name, xauth_entry->name_length);

                            auth_info.datalen = xauth_entry->data_length;
                            auth_info.data = new char[xauth_entry->data_length];
                            std::memcpy(auth_info.data, xauth_entry->data, xauth_entry->data_length);

                            found = true;
                            XauDisposeAuth(xauth_entry);
                            break;
                        }
                        XauDisposeAuth(xauth_entry);
                    }

                    fclose(auth_file);

                    return found;
                }

                std::string 
                getSocketPath(const char * display) 
                {
                    std::string displayStr;

                    if (display == nullptr) 
                    {
                        char* envDisplay = std::getenv("DISPLAY");
                        
                        if (envDisplay != nullptr) 
                        {
                            displayStr = envDisplay;
                        } 
                        else 
                        {
                            displayStr = ":0";
                        }
                    } 
                    else 
                    {
                        displayStr = display;
                    }

                    int displayNumber = 0;

                    // Extract the display number from the display string
                    size_t colonPos = displayStr.find(':');
                    if (colonPos != std::string::npos) 
                    {
                        displayNumber = std::stoi(displayStr.substr(colonPos + 1));
                    }

                    return "/tmp/.X11-unix/X" + std::to_string(displayNumber);
                }

                int 
                parseDisplayNumber(const char * display) 
                {
                    if (!display) 
                    {
                        display = std::getenv("DISPLAY");
                    }

                    if (!display) 
                    {
                        return 0;  // default to display 0
                    }

                    const std::string displayStr = display;
                    size_t colonPos = displayStr.find(':');
                    if (colonPos != std::string::npos) 
                    {
                        return std::stoi(displayStr.substr(colonPos + 1));
                    }

                    return 0;  // default to display 0
                }
            ;
        };
        static XConnection * mxb_connect(const char* display) 
        {
            try 
            {
                return new XConnection(display);
            } 
            catch (const std::exception & e) 
            {
                // Handle exceptions or errors here
                std::cerr << "Connection error: " << e.what() << std::endl;
                return nullptr;
            }
        }
        static int mxb_connection_has_error(XConnection * conn)
        {
            try 
            {
                // conn->confirmConnection();
                std::string response = conn->sendMessage("BIG-REQUESTS");
                log_info(response);
            }
            catch (const std::exception & e)
            {
                log_error(e.what());
                return 1;
            }
            return 0;
        }
        class File
        {
            public: // subclasses
                class search
                {
                    public: // construcers and operators
                        search(const std::string& filename)
                        : file(filename) {}

                        operator bool() const
                        {
                            return file.good();
                        }
                    ;

                    private: // private variables
                        std::ifstream file;
                    ;
                };
            ;
        };
    ;
};
class mv_client 
{
    public: // constructor 
        mv_client(client * & c, const uint16_t & start_x, const uint16_t & start_y) 
        : c(c), start_x(start_x), start_y(start_y)
        {
            if (c->win.is_EWMH_fullscreen())
            {
                return;
            }

            pointer->grab(c->frame);
            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }
    ;
    private: // variabels 
        client * & c;
        const uint16_t & start_x;
        const uint16_t & start_y;
        bool shouldContinue = true;
        xcb_generic_event_t * ev;
        /* DEFENITIONS TO REDUCE REDUNDENT CODE IN 'snap' FUNCTION */
        #define RIGHT_  screen->width_in_pixels  - c->width
        #define BOTTOM_ screen->height_in_pixels - c->height
        const double frameRate = 120.0; /* 
            FRAMERATE 
         */
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now(); /*
            HIGH_PRECISION_CLOCK AND TIME_POINT 
         */
        const double frameDuration = 1000.0 / frameRate; /* 
            DURATION IN MILLISECONDS THAT EACH FRAME SHOULD LAST 
        */
    ;
    private: // functions 
        void snap(const int16_t & x, const int16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : wm->cur_d->current_clients)
            {
                // SNAP WINDOW TO 'RIGHT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x + cli->width - N && x < cli->x + cli->width + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    // SNAP WINDOW TO 'RIGHT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC)
                    {  
                        c->frame.x_y((cli->x + cli->width), cli->y);
                        return;
                    }

                    // SNAP WINDOW TO 'RIGHT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC)
                    {
                        c->frame.x_y((cli->x + cli->width), (cli->y + cli->height) - c->height);
                        return;
                    }

                    c->frame.x_y((cli->x + cli->width), y);
                    return;
                }

                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x + c->width > cli->x - N && x + c->width < cli->x + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    // SNAP WINDOW TO 'LEFT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC)
                    {  
                        c->frame.x_y((cli->x - c->width), cli->y);
                        return;
                    }

                    // SNAP WINDOW TO 'LEFT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC)
                    {
                        c->frame.x_y((cli->x - c->width), (cli->y + cli->height) - c->height);
                        return;
                    }                

                    c->frame.x_y((cli->x - c->width), y);
                    return;
                }

                // SNAP WINDOW TO 'BOTTOM' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y + cli->height - N && y < cli->y + cli->height + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    // SNAP WINDOW TO 'BOTTOM_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC)
                    {  
                        c->frame.x_y(cli->x, (cli->y + cli->height));
                        return;
                    }

                    // SNAP WINDOW TO 'BOTTOM_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC)
                    {
                        c->frame.x_y(((cli->x + cli->width) - c->width), (cli->y + cli->height));
                        return;
                    }

                    c->frame.x_y(x, (cli->y + cli->height));
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y + c->height > cli->y - N && y + c->height < cli->y + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    // SNAP WINDOW TO 'TOP_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC)
                    {  
                        c->frame.x_y(cli->x, (cli->y - c->height));
                        return;
                    }

                    // SNAP WINDOW TO 'TOP_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC)
                    {
                        c->frame.x_y(((cli->x + cli->width) - c->width), (cli->y - c->height));
                        return;
                    }

                    c->frame.x_y(x, (cli->y - c->height));
                    return;
                }
            }

            // WINDOW TO EDGE OF SCREEN SNAPPING
            if (((x < N) && (x > -N)) && ((y < N) && (y > -N)))
            {
                c->frame.x_y(0, 0);
            }
            else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < N && y > -N))
            {
                c->frame.x_y(RIGHT_, 0);
            }
            else if ((y < BOTTOM_ + N && y > BOTTOM_ - N) && (x < N && x > -N))
            {
                c->frame.x_y(0, BOTTOM_);
            }
            else if ((x < N) && (x > -N))
            { 
                c->frame.x_y(0, y);
            }
            else if (y < N && y > -N)
            {
                c->frame.x_y(x, 0);
            }
            else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < BOTTOM_ + N && y > BOTTOM_ - N))
            {
                c->frame.x_y(RIGHT_, BOTTOM_);
            }
            else if ((x < RIGHT_ + N) && (x > RIGHT_ - N))
            { 
                c->frame.x_y(RIGHT_, y);
            }
            else if (y < BOTTOM_ + N && y > BOTTOM_ - N)
            {
                c->frame.x_y(x, BOTTOM_);
            }
            else 
            {
                c->frame.x_y(x, y);
            }
        }
        /*
         * THIS IS THE MAIN EVENT LOOP FOR 'mv_client'
         */ 
        void run() 
        {
            while (shouldContinue) 
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) 
                {
                    continue;
                }

                switch (ev->response_type & ~0x80) 
                {
                    case XCB_MOTION_NOTIFY: 
                    {
                        const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                        const int16_t & new_x = e->root_x - start_x;
                        const int16_t & new_y = e->root_y - start_y;
                        
                        if (isTimeToRender())
                        {
                            snap(new_x, new_y);
                            xcb_flush(conn);
                        }
                        break;
                    }
                    case XCB_BUTTON_RELEASE:
                    {
                        shouldContinue = false;
                        c->update();
                        break;
                    }
                }
                free(ev);
            }
        }
        bool isTimeToRender() 
        {
            auto currentTime = std::chrono::high_resolution_clock::now(); /*
                CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
             */ 
            const std::chrono::duration<double, std::milli> elapsedTime = currentTime - lastUpdateTime;

            if (elapsedTime.count() >= frameDuration) /*
                CHECK IF THE ELAPSED TIME EXCEEDS THE FRAME DURATION
             */ 
            {
                lastUpdateTime = currentTime; /*
                    UPDATE THE LAST_UPDATE_TIME TO THE 
                    CURRENT TIME FOR THE NEXT CHECK
                 */ 
                return true; /*
                    RETURN TRUE IF IT'S TIME TO RENDER
                 */ 
            }
            return false; /*
                RETURN FALSE IF NOT ENOUGH TIME HAS PASSED
             */ 
        }
    ;
};
void animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
    Mwm_Animator anim(c->frame);
    anim.animate
    (
        c->x,
        c->y, 
        c->width, 
        c->height, 
        endX,
        endY, 
        endWidth, 
        endHeight, 
        duration
    );
    c->update();
}
void animate_client(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
    Mwm_Animator client_anim(c);
    client_anim.animate_client
    (
        c->x,
        c->y,
        c->width,
        c->height,
        endX,
        endY,
        endWidth,
        endHeight,
        duration
    );
    c->update();
}
std::mutex mtx;
class change_desktop
{
    public: // constructor
        change_desktop(xcb_connection_t * connection) 
        : connection(connection) {}
    ;
    public: // methods
        enum DIRECTION
        {
            NEXT,
            PREV
        };
        enum DURATION
        {
            DURATION = 100
        };
        void change_to(const DIRECTION & direction)
        {
            switch (direction)
            {
                case NEXT:
                {
                    if (wm->cur_d->desktop == wm->desktop_list.size())
                    {
                        return;
                    }

                    hide = get_clients_on_desktop(wm->cur_d->desktop);
                    show = get_clients_on_desktop(wm->cur_d->desktop + 1);
                    animate(show, NEXT);
                    animate(hide, NEXT);
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop];
                    break;
                }
                case PREV:
                {
                    if (wm->cur_d->desktop == 1)
                    {
                        return;
                    }

                    hide = get_clients_on_desktop(wm->cur_d->desktop);
                    show = get_clients_on_desktop(wm->cur_d->desktop - 1);
                    animate(show, PREV);
                    animate(hide, PREV);
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop - 2];
                    break;
                }
            }
            mtx.lock();
            thread_sleep(DURATION + 20);
            joinAndClearThreads();
            mtx.unlock();
        }
        static void teleport_to(const uint8_t & n)
        {
            if (wm->cur_d == wm->desktop_list[n - 1] || n == 0 || n == wm->desktop_list.size())
            {
                return;
            }
            
            for (const auto & c : wm->cur_d->current_clients)
            {
                if (c)
                {
                    if (c->desktop == wm->cur_d->desktop)
                    {
                        c->unmap();
                    }
                }
            }

            wm->cur_d = wm->desktop_list[n - 1];
            for (const auto & c : wm->cur_d->current_clients)
            {
                if (c)
                {
                    c->map();
                }
            }
        }
    ;
    private: // variables
        xcb_connection_t * connection;
        std::vector<client *> show;
        std::vector<client *> hide;
        std::thread show_thread;
        std::thread hide_thread;
        std::atomic<bool> stop_show_flag{false};
        std::atomic<bool> stop_hide_flag{false};
        std::atomic<bool> reverse_animation_flag{false};
        std::vector<std::thread> animation_threads;
    ;
    private: // functions
        std::vector<client *> get_clients_on_desktop(const uint8_t & desktop)
        {
            std::vector<client *> clients;
            for (const auto & c : wm->client_list)
            {
                if (c->desktop == desktop)
                {
                    clients.push_back(c);
                }
            }
            return clients;
        }
        void animate(std::vector<client *> clients, const DIRECTION & direction)
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
        void anim_cli(client * c, const int & endx)
        {
            Mwm_Animator anim(c);
            anim.animate_client_x(c->x, endx, DURATION);
            c->update();
        }
        void thread_sleep(const double & milliseconds) 
        {
            // Creating a duration with double milliseconds
            auto duration = std::chrono::duration<double, std::milli>(milliseconds);

            // Sleeping for the duration
            std::this_thread::sleep_for(duration);
        }
        void stopAnimations() 
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
        void joinAndClearThreads()
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
void move_to_next_desktop_w_app()
{
    LOG_func
    if (wm->cur_d->desktop == wm->desktop_list.size())
    {
        return;
    }

    if (wm->focused_client)
    {
        wm->focused_client->desktop = wm->cur_d->desktop + 1;
    }

    change_desktop::teleport_to(wm->cur_d->desktop + 1);
}
void move_to_previus_desktop_w_app()
{
    LOG_func
    if (wm->cur_d->desktop == 1)
    {
        return;
    }

    if (wm->focused_client)
    {
        wm->focused_client->desktop = wm->cur_d->desktop - 1;
    }

    change_desktop::teleport_to(wm->cur_d->desktop - 1);
    wm->focused_client->raise();
}
class resize_client
{
    public: // constructor
        /* 
            THE REASON FOR THE 'retard_int' IS BECUSE WITHOUT IT 
            I CANNOT CALL THIS CLASS LIKE THIS 'resize_client(c)' 
            INSTEAD I WOULD HAVE TO CALL IT LIKE THIS 'resize_client rc(c)'
            AND NOW WITH THE 'retard_int' I CAN CALL IT LIKE THIS 'resize_client(c, 0)'
         */
        resize_client(client * & c , int retard_int) 
        : c(c) 
        {
            if (c->win.is_EWMH_fullscreen())
            {
                return;
            }

            pointer->grab(c->frame);
            pointer->teleport(c->x + c->width, c->y + c->height);
            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }
    ;
    public: // subclasses
        class no_border
        {
            public: // constructor
                no_border(client * & c, const uint32_t & x, const uint32_t & y)
                : c(c)
                {
                    if (c->win.is_EWMH_fullscreen())
                    {
                        return;
                    }
                    
                    pointer->grab(c->frame);
                    edge edge = wm->get_client_edge_from_pointer(c, 10);
                    teleport_mouse(edge);
                    run(edge);
                    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                    xcb_flush(conn);
                }
            ;
            private: // variables
                client * & c;
                uint32_t x;
                uint32_t y;
                const double frameRate = 120.0;
                std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
            ;
            private: // functions
                void teleport_mouse(edge edge) 
                {
                    switch (edge) 
                    {
                        case edge::TOP:
                            pointer->teleport(pointer->x(), c->y);
                            break;
                        ;
                        case edge::BOTTOM_edge:
                            pointer->teleport(pointer->x(), (c->y + c->height));
                            break;
                        ;
                        case edge::LEFT:
                            pointer->teleport(c->x, pointer->y());
                            break;
                        ;
                        case edge::RIGHT:
                            pointer->teleport((c->x + c->width), pointer->y());
                            break;
                        ;
                        case edge::NONE:
                            break;
                        ;
                        case edge::TOP_LEFT:
                            pointer->teleport(c->x, c->y);
                            break;
                        ;
                        case edge::TOP_RIGHT:
                            pointer->teleport((c->x + c->width), c->y);
                            break;
                        ;
                        case edge::BOTTOM_LEFT:
                            pointer->teleport(c->x, (c->y + c->height));
                            break;
                        ;
                        case edge::BOTTOM_RIGHT:
                            pointer->teleport((c->x + c->width), (c->y + c->height));
                            break;
                        ;
                    }
                }
                void resize_client(const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge) 
                    {
                        case edge::LEFT:
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        ;
                        case edge::RIGHT:
                            c->_width((x - c->x));
                            break;
                        ;
                        case edge::TOP:
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        ;
                        case edge::BOTTOM_edge:
                            c->_height((y - c->y));
                            break;
                        ;
                        case edge::NONE:
                            return;
                        ;
                        case edge::TOP_LEFT:
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        ;
                        case edge::TOP_RIGHT:
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        ;
                        case edge::BOTTOM_LEFT:
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        ;
                        case edge::BOTTOM_RIGHT:
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        ;
                    }
                }
                void run(edge edge) /**
                 * THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
                 */
                {
                    if (edge == edge::NONE)
                    {
                        log_info("edge == edge::NONE");
                        return;
                    }

                    xcb_generic_event_t * ev;
                    bool shouldContinue = true;

                    // Wait for motion events and handle window resizing
                    while (shouldContinue) 
                    {
                        ev = xcb_wait_for_event(conn);
                        if (!ev) 
                        {
                            continue;
                        }

                        switch (ev->response_type & ~0x80) 
                        {
                            case XCB_MOTION_NOTIFY: 
                            {
                                const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                                if (isTimeToRender())
                                {
                                    resize_client(e->root_x, e->root_y, edge);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: 
                            {
                                shouldContinue = false;                        
                                c->update();
                                break;
                            }
                        }
                        // Free the event memory after processing
                        free(ev); 
                    }
                }
                bool isTimeToRender() 
                {
                    // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
                    const auto & currentTime = std::chrono::high_resolution_clock::now();
                    const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - lastUpdateTime;

                    /*
                        CHECK IF THE ELAPSED TIME EXCEEDS THE FRAME DURATION
                    */ 
                    if (elapsedTime.count() >= frameDuration) 
                    {
                        // UPDATE THE LAST_UPDATE_TIME TO THE 
                        // CURRENT TIME FOR THE NEXT CHECK
                        lastUpdateTime = currentTime; 
                        
                        // RETURN TRUE IF IT'S TIME TO RENDER
                        return true; 
                    }
                    // RETURN FALSE IF NOT ENOUGH TIME HAS PASSED
                    return false; 
                }
            ;
        };
        class border
        {
            public: // constructor 
                border(client * & c, edge _edge)
                : c(c)
                {
                    if (c->win.is_EWMH_fullscreen())
                    {
                        return;
                    }

                    std::map<client *, edge> map = wm->get_client_next_to_client(c, _edge);
                    for (const auto & pair : map)
                    {
                        if (pair.first != nullptr)
                        {
                            c2 = pair.first;
                            c2_edge = pair.second;
                            pointer->grab(c->frame);
                            teleport_mouse(_edge);
                            run_double(_edge);
                            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                            xcb_flush(conn);
                            return;
                        }
                    }
                    
                    pointer->grab(c->frame);
                    teleport_mouse(_edge);
                    run(_edge);
                    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                    xcb_flush(conn);
                }
            ;
            private: // variables
                client * & c;
                client * c2;
                edge c2_edge; 
                const double frameRate = 120.0;
                std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
            ;
            private: // methods
                void teleport_mouse(edge edge) 
                {
                    switch (edge) 
                    {
                        case edge::TOP:
                        {
                            pointer->teleport(pointer->x(), c->y);
                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            pointer->teleport(pointer->x(), (c->y + c->height));
                            break;
                        } 
                        case edge::LEFT:
                        {
                            pointer->teleport(c->x, pointer->y());
                            break;
                        }
                        case edge::RIGHT:
                        {
                            pointer->teleport((c->x + c->width), pointer->y());
                            break;
                        }
                        case edge::NONE:
                        {
                            break;
                        }
                        case edge::TOP_LEFT:
                        {
                            pointer->teleport(c->x, c->y);
                            break;
                        }
                        case edge::TOP_RIGHT:
                        {
                            pointer->teleport((c->x + c->width), c->y);
                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            pointer->teleport(c->x, (c->y + c->height));
                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {
                            pointer->teleport((c->x + c->width), (c->y + c->height));
                            break;
                        }
                    }
                }
                void resize_client(const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge) 
                    {
                        case edge::LEFT:
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        ;
                        case edge::RIGHT:
                            c->_width((x - c->x));
                            break;
                        ;
                        case edge::TOP:
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        ;
                        case edge::BOTTOM_edge:
                            c->_height((y - c->y));
                            break;
                        ;
                        case edge::NONE:
                            return;
                        ;
                        case edge::TOP_LEFT:
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        ;
                        case edge::TOP_RIGHT:
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        ;
                        case edge::BOTTOM_LEFT:
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        ;
                        case edge::BOTTOM_RIGHT:
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        ;
                    }
                }
                void resize_client(client * c, const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge) 
                    {
                        case edge::LEFT:
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        ;
                        case edge::RIGHT:
                            c->_width((x - c->x));
                            break;
                        ;
                        case edge::TOP:
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        ;
                        case edge::BOTTOM_edge:
                            c->_height((y - c->y));
                            break;
                        ;
                        case edge::NONE:
                            return;
                        ;
                        case edge::TOP_LEFT:
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        ;
                        case edge::TOP_RIGHT:
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        ;
                        case edge::BOTTOM_LEFT:
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        ;
                        case edge::BOTTOM_RIGHT:
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        ;
                    }
                }
                void snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox)
                {
                    uint16_t left_border   = 0;
                    uint16_t right_border  = 0;
                    uint16_t top_border    = 0;
                    uint16_t bottom_border = 0;

                    for (const auto & c : wm->cur_d->current_clients)
                    {
                        if (c == this->c)
                        {
                            continue;
                        }

                        left_border = c->x;
                        right_border = (c->x + c->width);
                        top_border = c->y;
                        bottom_border = (c->y + c->height);

                        if (edge != edge::RIGHT
                         && edge != edge::BOTTOM_RIGHT
                         && edge != edge::TOP_RIGHT)
                        {
                            if ((x > right_border - prox && x < right_border + prox)
                            && (y > top_border && y < bottom_border))
                            {
                                resize_client(right_border, y, edge);
                                return;
                            }
                        }

                        if (edge != edge::LEFT
                         && edge != edge::TOP_LEFT
                         && edge != edge::BOTTOM_LEFT)
                        {
                            if ((x > left_border - prox && x < left_border + prox)
                            && (y > top_border && y < bottom_border))
                            {
                                resize_client(left_border, y, edge);
                                return;
                            }
                        }

                        if (edge != edge::BOTTOM_edge 
                         && edge != edge::BOTTOM_LEFT 
                         && edge != edge::BOTTOM_RIGHT)
                        {
                            if ((y > bottom_border - prox && y < bottom_border + prox)
                            && (x > left_border && x < right_border))
                            {
                                resize_client(x, bottom_border, edge);
                                return;
                            }
                        }

                        if (edge != edge::TOP
                         && edge != edge::TOP_LEFT
                         && edge != edge::TOP_RIGHT)
                        {
                            if ((y > top_border - prox && y < top_border + prox)
                            && (x > left_border && x < right_border))
                            {
                                resize_client(x, top_border, edge);
                                return;
                            }
                        }
                    }
                    resize_client(x, y, edge);
                }
                void run(edge edge) /**
                 * THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
                 */
                {
                    xcb_generic_event_t * ev;
                    bool shouldContinue = true;

                    // Wait for motion events and handle window resizing
                    while (shouldContinue) 
                    {
                        ev = xcb_wait_for_event(conn);
                        if (!ev) 
                        {
                            continue;
                        }

                        switch (ev->response_type & ~0x80) 
                        {
                            case XCB_MOTION_NOTIFY: 
                            {
                                const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                                if (isTimeToRender())
                                {
                                    snap(e->root_x, e->root_y, edge, 12);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: 
                            {
                                shouldContinue = false;                        
                                c->update();
                                break;
                            }
                        }
                        // Free the event memory after processing
                        free(ev); 
                    }
                }
                void run_double(edge edge) /**
                 * THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
                 */
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
                            case XCB_MOTION_NOTIFY: 
                            {
                                const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                                if (isTimeToRender())
                                {
                                    resize_client(c, e->root_x, e->root_y, edge);
                                    resize_client(c2, e->root_x, e->root_y, c2_edge);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: 
                            {
                                shouldContinue = false;                        
                                c->update();
                                c2->update();
                                break;
                            }
                        }
                        free(ev); 
                    }
                }
                bool isTimeToRender() 
                {
                    const auto & currentTime = std::chrono::high_resolution_clock::now();
                    const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - lastUpdateTime;

                    if (elapsedTime.count() >= frameDuration) 
                    {
                        lastUpdateTime = currentTime; 
                        
                        return true; 
                    }
                    return false; 
                }
            ;
        };
    ;
    private: // variabels
        client * & c;
        uint32_t x;
        uint32_t y;
        const double frameRate = 120.0;
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
        const double frameDuration = 1000.0 / frameRate; 
    ;
    private: // functions
        void snap(const uint16_t & x, const uint16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : wm->cur_d->current_clients)
            {
                if (cli == this->c)
                {
                    continue;
                }

                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x - N && x < cli->x + N) 
                 && (y + this->c->height > cli->y && y < cli->y + cli->height))
                {
                    c->width_height((cli->x - this->c->x), (y - this->c->y));
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y - N && y < cli->y + N) 
                 && (x + this->c->width > cli->x && x < cli->x + cli->width))
                {
                    c->width_height((x - this->c->x), (cli->y - this->c->y));
                    return;
                }
            }
            c->width_height((x - this->c->x), (y - this->c->y));
        }
        void run() /**
         * THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
         */
        {
            xcb_generic_event_t * ev;
            bool shouldContinue = true;

            // Wait for motion events and handle window resizing
            while (shouldContinue) 
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) 
                {
                    continue;
                }

                switch (ev->response_type & ~0x80) 
                {
                    case XCB_MOTION_NOTIFY: 
                    {
                        const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                        if (isTimeToRender())
                        {
                            snap(e->root_x, e->root_y);
                            xcb_flush(conn); 
                        }
                        break;
                    }
                    case XCB_BUTTON_RELEASE: 
                    {
                        shouldContinue = false;                        
                        c->update();
                        break;
                    }
                }
                // Free the event memory after processing
                free(ev); 
            }
        }
        bool isTimeToRender() 
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - lastUpdateTime;

            /*
                CHECK IF THE ELAPSED TIME EXCEEDS THE FRAME DURATION
             */ 
            if (elapsedTime.count() >= frameDuration) 
            {
                // UPDATE THE LAST_UPDATE_TIME TO THE 
                // CURRENT TIME FOR THE NEXT CHECK
                lastUpdateTime = currentTime; 
                
                // RETURN TRUE IF IT'S TIME TO RENDER
                return true; 
            }
            // RETURN FALSE IF NOT ENOUGH TIME HAS PASSED
            return false; 
        }
    ;
};
class max_win
{
    public: // constructor
        enum max_win_type
        {
            BUTTON_MAXWIN,
            EWMH_MAXWIN,
        };
        max_win(client * c, max_win_type type)
        {
            switch (type)
            {
                case EWMH_MAXWIN:
                {
                    if (c->win.is_EWMH_fullscreen())
                    {
                        ewmh_unmax_win(c);
                    }
                    else
                    {
                        ewmh_max_win(c);
                    }

                    break;
                }
                case BUTTON_MAXWIN:
                {
                    if (is_max_win(c))
                    {
                        button_unmax_win(c);
                    }
                    else
                    {
                        button_max_win(c);
                    }

                    break;
                }
            }
        }
    ;
    private: // functions
        void max_win_animate(client * c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            animate_client
            (
                c, 
                endX, 
                endY, 
                endWidth, 
                endHeight, 
                MAXWIN_ANIMATION_DURATION
            );
        }
        void save_max_ewmh_ogsize(client * c)
        {
            c->max_ewmh_ogsize.x      = c->x;
            c->max_ewmh_ogsize.y      = c->y;
            c->max_ewmh_ogsize.width  = c->width;
            c->max_ewmh_ogsize.height = c->height;
        }
        void ewmh_max_win(client * c)
        {
            save_max_ewmh_ogsize(c);
            max_win_animate
            (
                c, 
                - BORDER_SIZE, 
                - TITLE_BAR_HEIGHT - BORDER_SIZE, 
                screen->width_in_pixels + (BORDER_SIZE * 2), 
                screen->height_in_pixels + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)
            );
            c->win.set_EWMH_fullscreen_state();
            xcb_flush(conn);
        }
        void ewmh_unmax_win(client * c)
        {
            if (c->max_ewmh_ogsize.width > screen->width_in_pixels)
            {
                c->max_ewmh_ogsize.width = screen->width_in_pixels / 2;
            }

            if (c->max_ewmh_ogsize.height > screen->height_in_pixels)
            {
                c->max_ewmh_ogsize.height = screen->height_in_pixels / 2;
            }

            if (c->max_ewmh_ogsize.x >= screen->width_in_pixels - 1)
            {
                c->max_ewmh_ogsize.x = (screen->width_in_pixels / 2) - (c->max_ewmh_ogsize.width / 2) - BORDER_SIZE;
            }

            if (c->max_ewmh_ogsize.y >= screen->height_in_pixels - 1)
            {
                c->max_ewmh_ogsize.y = (screen->height_in_pixels / 2) - (c->max_ewmh_ogsize.height / 2) - TITLE_BAR_HEIGHT - BORDER_SIZE;
            }

            max_win_animate
            (
                c, 
                c->max_ewmh_ogsize.x, 
                c->max_ewmh_ogsize.y, 
                c->max_ewmh_ogsize.width, 
                c->max_ewmh_ogsize.height
            );
            c->win.unset_EWMH_fullscreen_state();
            xcb_flush(conn);
        }
        void save_max_button_ogsize(client * c)
        {
            c->max_button_ogsize.x      = c->x;
            c->max_button_ogsize.y      = c->y;
            c->max_button_ogsize.width  = c->width;
            c->max_button_ogsize.height = c->height;
        }
        void button_max_win(client * c)
        {
            save_max_button_ogsize(c);
            max_win_animate
            (
                c,
                0,
                0,
                screen->width_in_pixels,
                screen->height_in_pixels
            );
            xcb_flush(conn);
        }
        void button_unmax_win(client * c)
        {
            max_win_animate
            (
                c, 
                c->max_button_ogsize.x,
                c->max_button_ogsize.y,
                c->max_button_ogsize.width,
                c->max_button_ogsize.height
            );
            xcb_flush(conn);
        }
        bool is_max_win(client * c)
        {
            if (c->x == 0
             && c->y == 0
             && c->width == screen->width_in_pixels
             && c->height == screen->height_in_pixels)
            {
                return true;
            }

            return false;
        }
    ;
};
namespace win_tools
{
    xcb_visualtype_t * find_argb_visual(xcb_connection_t *conn, xcb_screen_t *screen) /**
     *
     * @brief Function to find an ARGB visual 
     *
     */
    {
        xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

        for (; depth_iter.rem; xcb_depth_next(&depth_iter)) 
        {
            xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) 
            {
                if (depth_iter.data->depth == 32) 
                {
                    return visual_iter.data;
                }
            }
        }
        return NULL;
    }
    void close_button_kill(client * c)
    {
        int result = wm->send_sigterm_to_client(c);

        if (result == -1)
        {
            log_error("client is nullptr");
        }
    }
}
class tile /**
 * @class tile
 * @brief Represents a tile obj.
 * 
 * The `tile` class is responsible for managing the tiling behavior of windows in the window manager.
 * It provides methods to tile windows to the left, right, *up, or *down positions on the screen.
 * The class also includes helper methods to check the current tile position of a window and set the size and position of a window.
 */
{
    public: // constructors
        tile(client * & c, TILE tile)
        {
            if (c->win.is_EWMH_fullscreen())
            {
                return;
            }

            switch (tile) 
            {
                case TILE::LEFT:
                {
                    // IF 'CURRENTLT_TILED' TO 'LEFT'
                    if (current_tile_pos(c, TILEPOS::LEFT))
                    {
                        set_tile_ogsize(c);
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' TO 'RIGHT', 'LEFT_DOWN' OR 'LEFT_UP'
                    if (current_tile_pos(c, TILEPOS::RIGHT)
                     || current_tile_pos(c, TILEPOS::LEFT_DOWN)
                     || current_tile_pos(c, TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' TO 'RIGHT_DOWN'
                    if (current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' TO 'RIGHT_UP'
                    if (current_tile_pos(c, TILEPOS::RIGHT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_UP);
                        return;
                    }

                    save_tile_ogsize(c);
                    set_tile_sizepos(c, TILEPOS::LEFT);
                    break;
                }
                case TILE::RIGHT:
                {
                    // IF 'CURRENTLY_TILED' TO 'RIGHT'
                    if (current_tile_pos(c, TILEPOS::RIGHT))
                    {
                        set_tile_ogsize(c);
                        return;
                    }

                    // IF 'CURRENTLT_TILED' TO 'LEFT', 'RIGHT_DOWN' OR 'RIGHT_UP' 
                    if (current_tile_pos(c, TILEPOS::LEFT)
                     || current_tile_pos(c, TILEPOS::RIGHT_UP)
                     || current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT);
                        return;
                    }

                    // IF 'CURRENTLT_TILED' 'LEFT_DOWN'
                    if (current_tile_pos(c, TILEPOS::LEFT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'LEFT_UP'
                    if (current_tile_pos(c, TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_UP);
                        return;
                    }

                    save_tile_ogsize(c);
                    set_tile_sizepos(c, TILEPOS::RIGHT);
                    break;
                }
                case TILE::DOWN:
                {
                    // IF 'CURRENTLY_TILED' 'LEFT' OR 'LEFT_UP'
                    if (current_tile_pos(c, TILEPOS::LEFT)
                     || current_tile_pos(c, TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR 'RIGHT_UP'
                    if (current_tile_pos(c, TILEPOS::RIGHT)
                     || current_tile_pos(c, TILEPOS::RIGHT_UP))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'LEFT_DOWN' OR 'RIGHT_DOWN'
                    if (current_tile_pos(c, TILEPOS::LEFT_DOWN)
                     || current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_ogsize(c);
                        return;
                    }
                }
                case TILE::UP:
                {
                    // IF 'CURRENTLY_TILED' 'LEFT'
                    if (current_tile_pos(c, TILEPOS::LEFT)
                     || current_tile_pos(c, TILEPOS::LEFT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::LEFT_UP);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR RIGHT_DOWN
                    if (current_tile_pos(c, TILEPOS::RIGHT)
                     || current_tile_pos(c, TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(c, TILEPOS::RIGHT_UP);
                        return;
                    }
                }
            }
        }
    ;
    private: // functions
        void save_tile_ogsize(client * & c)
        {
            c->tile_ogsize.x      = c->x;
            c->tile_ogsize.y      = c->y;
            c->tile_ogsize.width  = c->width;
            c->tile_ogsize.height = c->height;
        }
        void moveresize(client * & c)
        {
            xcb_configure_window
            (
                conn,
                c->win,
                XCB_CONFIG_WINDOW_WIDTH | 
                XCB_CONFIG_WINDOW_HEIGHT| 
                XCB_CONFIG_WINDOW_X |
                XCB_CONFIG_WINDOW_Y,
                (const uint32_t[4])
                {
                    static_cast<const uint32_t &>(c->width),
                    static_cast<const uint32_t &>(c->height),
                    static_cast<const uint32_t &>(c->x),
                    static_cast<const uint32_t &>(c->y)
                }
            );
            xcb_flush(conn);
        }
        bool current_tile_pos(client * & c, TILEPOS mode)
        {
            switch (mode) 
            {
                case TILEPOS::LEFT:
                {
                    if (c->x        == 0 
                     && c->y        == 0 
                     && c->width    == screen->width_in_pixels / 2 
                     && c->height   == screen->height_in_pixels)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::RIGHT:
                {
                    if (c->x        == screen->width_in_pixels / 2 
                     && c->y        == 0 
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::LEFT_DOWN:
                {
                    if (c->x        == 0
                     && c->y        == screen->height_in_pixels / 2
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::RIGHT_DOWN:
                {
                    if (c->x        == screen->width_in_pixels / 2
                     && c->y        == screen->height_in_pixels / 2
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::LEFT_UP:
                {
                    if (c->x        == 0
                     && c->y        == 0
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
                case TILEPOS::RIGHT_UP:
                {
                    if (c->x        == screen->width_in_pixels / 2
                     && c->y        == 0
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    break;
                }
            }
            return false;
        }
        void set_tile_sizepos(client * & c, TILEPOS sizepos)
        {
            switch (sizepos) 
            {
                case TILEPOS::LEFT:
                {
                    animate
                    (
                        c, 
                        0, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels
                    );
                    return;
                }
                case TILEPOS::RIGHT:
                {
                    animate
                    (
                        c, 
                        screen->width_in_pixels / 2, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels
                    );
                    return;
                }
                case TILEPOS::LEFT_DOWN:
                {
                    animate
                    (
                        c, 
                        0, 
                        screen->height_in_pixels / 2, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                }
                case TILEPOS::RIGHT_DOWN:
                {
                    animate
                    (
                        c, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                }
                case TILEPOS::LEFT_UP:
                {
                    animate
                    (
                        c, 
                        0, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                } 
                case TILEPOS::RIGHT_UP:
                {
                    animate
                    (
                        c, 
                        screen->width_in_pixels / 2, 
                        0, 
                        screen->width_in_pixels / 2, 
                        screen->height_in_pixels / 2
                    );
                    return;
                }
            }
        }
        void set_tile_ogsize(client * & c)
        {
            animate
            (
                c, 
                c->tile_ogsize.x, 
                c->tile_ogsize.y, 
                c->tile_ogsize.width, 
                c->tile_ogsize.height
            );
        }
        void animate_old(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            Mwm_Animator anim(c->frame);
            anim.animate
            (
                c->x,
                c->y, 
                c->width, 
                c->height, 
                endX,
                endY, 
                endWidth, 
                endHeight, 
                TILE_ANIMATION_DURATION
            );
            c->update();
        }
        void animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            Mwm_Animator anim(c);
            anim.animate_client
            (
                c->x,
                c->y, 
                c->width, 
                c->height, 
                endX,
                endY, 
                endWidth, 
                endHeight, 
                TILE_ANIMATION_DURATION
            );
            c->update();
        }
    ;
};
class Events
{
    public: // constructor and destructor  
        Events() /**
         *
         * @brief Constructor for the Event class.
         *        Initializes the key symbols and keycodes.
         *
         */
        {}
    ;
    public: // methods
        void setup()
        {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev){ key_press_handler(ev); });
            event_handler->setEventCallback(XCB_MAP_NOTIFY, [&](Ev ev){ map_notify_handler(ev); });
            event_handler->setEventCallback(XCB_MAP_REQUEST, [&](Ev ev){ map_req_handler(ev); });
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev){ button_press_handler(ev); });
            event_handler->setEventCallback(XCB_CONFIGURE_REQUEST, [&](Ev ev){ configure_request_handler(ev); });
            event_handler->setEventCallback(XCB_FOCUS_IN, [&](Ev ev){ focus_in_handler(ev); });
            event_handler->setEventCallback(XCB_FOCUS_OUT, [&](Ev ev){ focus_out_handler(ev); });
            event_handler->setEventCallback(XCB_DESTROY_NOTIFY, [&](Ev ev){ destroy_notify_handler(ev); });
            event_handler->setEventCallback(XCB_UNMAP_NOTIFY, [&](Ev ev){ unmap_notify_handler(ev); });
            event_handler->setEventCallback(XCB_REPARENT_NOTIFY, [&](Ev ev){ reparent_notify_handler(ev); });
            event_handler->setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev){ enter_notify_handler(ev); });
            event_handler->setEventCallback(XCB_LEAVE_NOTIFY, [&](Ev ev){ leave_notify_handler(ev); });
            event_handler->setEventCallback(XCB_MOTION_NOTIFY, [&](Ev ev){ motion_notify_handler(ev); });
        }
    ;
    private: // event handling functions
        void key_press_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                
            if (e->detail == wm->key_codes.t) /**
             * CHECK IF 'ALT+CTRL+T' WAS PRESSED
             * AND IF SO LAUNCH TERMINAL   
             */  
            {
                switch (e->state) 
                {
                    case CTRL + ALT:
                    {
                        wm->launcher.program((char *) "/usr/bin/konsole");
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.q) /**
             * CHECK IF 'ALT+SHIFT+Q' WAS PRESSED
             * AND IF SO LAUNCH KILL_SESSION
             */
            {
                switch (e->state) 
                {
                    case SHIFT + ALT:
                    {
                        wm->quit(0);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.f11) /**
             * CHECK IF 'F11' WAS PRESSED
             * AND IF SO TOGGLE FULLSCREEN
             */ 
            {
                client * c = wm->client_from_window(& e->event);
                max_win(c, max_win::EWMH_MAXWIN);
            }
            if (e->detail == wm->key_codes.n_1) /**
             * CHECK IF 'ALT+1' WAS PRESSED
             * AND IF SO MOVE TO DESKTOP 1
             */
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(1);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_2) /**
             * CHECK IF 'ALT+2' WAS PRESSED
             * AND IF SO MOVE TO DESKTOP 2
             */ 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(2);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_3) /**
             * CHECK IF 'ALT+3' WAS PRESSED
             * AND IF SO MOVE TO DESKTOP 3
             */ 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(3);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_4) /**
             * CHECK IF 'ALT+4' WAS PRESSED
             * AND IF SO MOVE TO DESKTOP 4
             */
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(4);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_5) /**
             * CHECK IF 'ALT+5' WAS PRESSED
             * IF SO MOVE TO DESKTOP 5
             */
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(5);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.r_arrow) /**
             * IF R_ARROW IS PRESSED THEN CHECK WHAT MOD MASK IS APPLIED
             * IF 'SHIFT+CTRL+SUPER' THEN MOVE TO NEXT DESKTOP WITH CURRENTLY FOCUSED APP
             * IF 'CTRL+SUPER' THEN MOVE TO THE NEXT DESKTOP
             * IF 'SUPER' THEN TILE WINDOW TO THE RIGHT
             */
            {
                switch (e->state) 
                {
                    case SHIFT + CTRL + SUPER:
                    {
                        move_to_next_desktop_w_app();
                        break;
                    }
                    case CTRL + SUPER:
                    {
				        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::NEXT);
                        break;
                    }
                    case SUPER:
                    {
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::RIGHT);
                        break;
                    }
                    return;
                }
            }
            if (e->detail == wm->key_codes.l_arrow) /**
             * IF L_ARROW IS PRESSED THEN CHECK WHAT MOD MASK IS APPLIED
             * IF 'SHIFT+CTRL+SUPER' THEN MOVE TO PREV DESKTOP WITH CURRENTLY FOCUSED APP
             * IF 'CTRL+SUPER' THEN MOVE TO THE PREV DESKTOP
             * IF 'SUPER' THEN TILE WINDOW TO THE LEFT
             */
            {
                switch (e->state) 
                {
                    case SHIFT + CTRL + SUPER:
                    {
                        move_to_previus_desktop_w_app();
                        break;
                    }
                    case CTRL + SUPER:
                    {
				        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::PREV);
                        break;
                    }
                    case SUPER:
                    {
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::LEFT);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.d_arrow) /**
             * IF 'D_ARROW' IS PRESSED SEND TO TILE
             */
            {
                switch (e->state) 
                {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::DOWN);
                        return;
                        break;
                    ;
                }
            }
            if (e->detail == wm->key_codes.u_arrow)
            {
                switch (e->state) 
                {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::UP);
                        break;
                    ;
                }
            }
            if (e->detail == wm->key_codes.tab) /**
             * CHECK IF 'ALT+TAB' WAS PRESSED
             * IF SO CYCLE FOCUS
             */
            {
                switch (e->state) 
                {
                    case ALT:
                        wm->cycle_focus();
                        break;
                    ;
                }
            }
            if (e->detail == wm->key_codes.k) /**
             *
             * @brief This is a debug kebinding to test featurtes before they are implemented.  
             * 
             */
            {
                switch (e->state) 
                {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        if (!c)
                        {
                            return;
                        }
                        
                        if (c->win.is_mask_active(XCB_EVENT_MASK_ENTER_WINDOW))
                        {
                            log_info("event_mask is active");
                        }
                        else 
                        {
                            log_info("event_mask is NOT active");
                        }
                        break;
                    ;
                }
            }
        }
        void map_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_map_notify_event_t *>(ev);
            client * c = wm->client_from_window(& e->window);
            if (c)
            {
                c->update();
            }
        }
        void map_req_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_map_request_event_t *>(ev);
            wm->manage_new_client(e->window);
        }
        void button_press_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
            client * c;

            if (BORDER_SIZE == 0)
            {
                c = wm->client_from_pointer(10);
                if (c)
                {
                    if (e->detail == L_MOUSE_BUTTON)
                    {
                        c->raise();
                        resize_client::no_border(c, 0, 0);
                        wm->focus_client(c);
                    }
                    return;
                }
            }

            if (e->event == wm->root)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    wm->context_menu->show();
                    return;
                }
            }

            c = wm->client_from_any_window(& e->event);
            if (!c)
            {
                return;
            }
            
            if (e->detail == L_MOUSE_BUTTON)
            {
                if (e->event == c->win)
                {
                    switch (e->state) 
                    {
                        case ALT:
                            c->raise();
                            mv_client(c, e->event_x, e->event_y + 20);
                            wm->focus_client(c);
                            break;
                        ;
                    }
                    c->raise();
                    wm->focus_client(c);
                    return;
                }

                if (e->event == c->titlebar)
                {
                    c->raise();
                    mv_client(c, e->event_x, e->event_y);
                    wm->focus_client(c);
                    return;
                }

                if (e->event == c->close_button)
                {
                    win_tools::close_button_kill(c);
                    return;
                }

                if (e->event == c->max_button)
                {
                    client * c = wm->client_from_any_window(& e->event);
                    max_win(c, max_win::BUTTON_MAXWIN);
                    return;
                }

                if (e->event == c->border.left)
                {
                    resize_client::border(c, edge::LEFT);
                    return;
                }

                if (e->event == c->border.right)
                {
                    resize_client::border(c, edge::RIGHT);
                    return;
                }

                if (e->event == c->border.top)
                {
                    resize_client::border(c, edge::TOP);
                    return;
                }

                if (e->event == c->border.bottom)
                {
                    resize_client::border(c, edge::BOTTOM_edge);
                }

                if (e->event == c->border.top_left)
                {
                    resize_client::border(c, edge::TOP_LEFT);
                    return;
                }

                if (e->event == c->border.top_right)
                {
                    resize_client::border(c, edge::TOP_RIGHT);
                    return;
                }

                if (e->event == c->border.bottom_left)
                {
                    resize_client::border(c, edge::BOTTOM_LEFT);
                    return;
                }

                if (e->event == c->border.bottom_right)
                {
                    resize_client::border(c, edge::BOTTOM_RIGHT);
                    return;
                }
            }

            if (e->detail == R_MOUSE_BUTTON) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        log_error("ALT + R_MOUSE_BUTTON");
                        c->raise();
                        resize_client(c, 0);
                        wm->focus_client(c);
                        return;
                    }
                }
            }
        }
        void configure_request_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_configure_request_event_t *>(ev);
            wm->data.width     = e->width;
            wm->data.height    = e->height;
            wm->data.x         = e->x;
            wm->data.y         = e->y;
        }
        void focus_in_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_in_event_t *>(ev);
            client * c = wm->client_from_window( & e->event);
            if (c)
            {
                c->win.ungrab_button({ { L_MOUSE_BUTTON, NULL } });
                c->raise();
                c->win.set_active_EWMH_window();
                wm->focused_client = c;
            }
        }
        void focus_out_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_out_event_t *>(ev);
            
            client * c = wm->client_from_window(& e->event);
            if (!c)
            {
                return;
            }
            
            c->win.grab_button({ { L_MOUSE_BUTTON, NULL } });
        }
        void destroy_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_destroy_notify_event_t *>(ev);
            client * c = wm->client_from_any_window(& e->window);
            int result = wm->send_sigterm_to_client(c);
            if (result == -1)
            {
                log_error("send_sigterm_to_client: failed");
            }
        }
        void unmap_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_unmap_notify_event_t *>(ev);
            
            client * c = wm->client_from_window(& e->window);
            if (!c)
            {
                return;
            }

            // if (c->isKilleble)
            // {
            //     xcb_unmap_window(conn, c->titlebar);
            //     xcb_unmap_window(conn, c->frame);
            //     delete c;
            //     XCB_flush();
            // }
        }
        void reparent_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_reparent_notify_event_t *>(ev);
        }
        void enter_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
            log_win("e->event: ", e->event);
        }
        void leave_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_leave_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
        }
        void motion_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
            // log_info(e->event_x);
        }
    ;
};
void setup_wm()
{
    wm = new Window_Manager;
    wm->init();
    
    change_desktop::teleport_to(1);
    dock = new Dock;
    dock->add_app("konsole");
    dock->add_app("alacritty");
    dock->add_app("falkon");
    dock->init();

    mwm_runner = new Mwm_Runner;
    mwm_runner->init();

    pointer = new class pointer;
    Events events;
    events.setup();
}
int main() 
{
    LOG_start()
    setup_wm();
    event_handler->run();
    xcb_disconnect(conn);
    return 0;
}