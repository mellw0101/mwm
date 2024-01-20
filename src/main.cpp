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
#include "pointer.hpp"
Logger log;

#include "defenitions.hpp"
#include "structs.hpp"

#include "window.hpp"
#include "client.hpp"
#include "desktop.hpp"
#include "event_handler.hpp"
#include "window_manager.hpp"
#include "mwm_animator.hpp"

class mxb {
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
class button {
    public: // constructor 
        button() {}
    ;
    public: // public variables 
        window window;
        const char * name;
    ;
    public: // public methods 
        void create(const uint32_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height, COLOR color)
        {
            window.create_default(parent_window, x, y, width, height);
            window.set_backround_color(color);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.map();
            xcb_flush(conn);
        }
        void action(std::function<void()> action)
        {
            button_action = action;
        }
        void add_event(std::function<void(Ev ev)> action)
        {
            ev_a = action;
            event_handler->setEventCallback(XCB_BUTTON_PRESS, ev_a);
        }
        void activate() const 
        {
            button_action();
        }
        void put_icon_on_button()
        {
            std::string icon_path = file.find_png_icon
            (
                {
                    "/usr/share/icons/gnome/256x256/apps/",
                    "/usr/share/icons/hicolor/256x256/apps/",
                    "/usr/share/icons/gnome/48x48/apps/",
                    "/usr/share/icons/gnome/32x32/apps/"
                },
                name
            );

            if (icon_path == "")
            {
                log_info("could not find icon for button: " + std::string(name));
                return;
            }
            window.set_backround_png(icon_path.c_str());
        }
    ;
    private: // private variables 
        std::function<void()> button_action;
        std::function<void(Ev ev)> ev_a;
        File file;
        Logger log;
    ;
};
class buttons {
    public: // public constructors
        buttons() {}
    ;
    public: // public variables
        std::vector<button> list;
    ;
    public: // public methods
        void add(const char * name, std::function<void()> action)
        {
            button button;
            button.name = name;
            button.action(action);
            list.push_back(button);
        }
        int size()
        {
            return list.size();
        }
        int index()
        {
            return list.size() - 1;
        }
        void run_action(const uint32_t & window)
        {
            for (const auto & button : list) 
            {
                if (window == button.window)
                {
                    button.activate();
                    return;
                }
            }
        }
    ;
};
class search_window {
    public: // variabels
        window main_window;
        std::string search_string = ""
    ;
    public: // mehods
        void create(const uint32_t & parent_window, const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height) {
            main_window.create_default(parent_window, x, y, width, height);
            main_window.set_backround_color(BLACK);
            uint32_t mask =  XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE;
            main_window.apply_event_mask(& mask);
            main_window.map();
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            main_window.grab_keys_for_typing();
            main_window.grab_default_keys();

            for (int i = 0; i < 7; ++i) {
                window entry;
                entry.create_default(main_window, 0, (20 * (i + 1)) , 140, 20);
                entry.set_backround_color(BLACK);
                entry.raise();
                entry.map();
                entry_list.push_back(entry);
            }
            
            main_window.raise();
            main_window.focus_input();
        }
        void add_enter_action(std::function<void()> enter_action) {
            enter_function = enter_action;
        }
        void init() {
            setup_events();
        }
        void clear_search_string() {
            search_string = "";
        }
        std::string string() {
            return search_string;
        }
    ;
    private: // functions
        void setup_events() {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->event == main_window) {
                    if (e->detail == wm->key_codes.a) {
                        if (e->state == SHIFT) {
                            search_string += "A";
                        } else {
                            search_string += "a";
                        } 
                    }
                    if (e->detail == wm->key_codes.b) {
                        if (e->state == SHIFT) {
                            search_string += "B";
                        } else {
                            search_string += "b"; 
                        }
                    }
                    if (e->detail == wm->key_codes.c) {
                        if (e->state == SHIFT) {
                            search_string += "C";
                        } else {
                            search_string += "c"; 
                        }
                    }
                    if (e->detail == wm->key_codes.d) {
                        if (e->state == SHIFT) {
                            search_string += "D";
                        } else {
                            search_string += "d";
                        }
                    }
                    if (e->detail == wm->key_codes.e) {
                        if (e->state == SHIFT) {
                            search_string += "E";
                        } else {
                            search_string += "e";
                        }
                    }
                    if (e->detail == wm->key_codes.f) {
                        if (e->state == SHIFT) {
                            search_string += "F";
                        } else {
                            search_string += "f";
                        }
                    }
                    if (e->detail == wm->key_codes.g) {
                        if (e->state == SHIFT) {
                            search_string += "G";
                        } else {
                            search_string += "g";
                        }
                    }
                    if (e->detail == wm->key_codes.h) {
                        if (e->state == SHIFT) {
                            search_string += "H";
                        } else {
                            search_string += "h";
                        }
                    }
                    if (e->detail == wm->key_codes.i) {
                        if (e->state == SHIFT) {
                            search_string += "I";
                        } else {
                            search_string += "i";
                        }
                    }
                    if (e->detail == wm->key_codes.j) {
                        if (e->state == SHIFT) {
                            search_string += "J";
                        } else {
                            search_string += "j";
                        }
                    }
                    if (e->detail == wm->key_codes.k) {
                        if (e->state == SHIFT) {
                            search_string += "K";
                        } else {
                            search_string += "k";
                        }
                    }
                    if (e->detail == wm->key_codes.l) {
                        if (e->state == SHIFT) {
                            search_string += "L";
                        } else {
                            search_string += "l";
                        }
                    }
                    if (e->detail == wm->key_codes.m) {
                        if (e->state == SHIFT) {
                            search_string += "M";
                        } else {
                            search_string += "m";
                        }
                    }
                    if (e->detail == wm->key_codes.n) {
                        if (e->state == SHIFT) {
                            search_string += "N";
                        } else {
                            search_string += "n";
                        }
                    }
                    if (e->detail == wm->key_codes.o) {
                        if (e->state == SHIFT) {
                            search_string += "O";
                        } else {
                            search_string += "o";
                        }
                    }
                    if (e->detail == wm->key_codes.p) {
                        if (e->state == SHIFT) {
                            search_string += "P";
                        } else {
                            search_string += "p";
                        }
                    }
                    if (e->detail == wm->key_codes.q) {
                        if (e->state == SHIFT) {
                            search_string += "Q";
                        } else {
                            search_string += "q";
                        }
                    }
                    if (e->detail == wm->key_codes.r) {
                        if (e->state == SHIFT) {
                            search_string += "R";
                        } else {
                            search_string += "r";
                        }
                    }
                    if (e->detail == wm->key_codes.s) {
                        if (e->state == SHIFT) {
                            search_string += "S";
                        } else {
                            search_string += "s";
                        }
                    }
                    if (e->detail == wm->key_codes.t) {
                        if (e->state == SHIFT) {
                            search_string += "T";
                        } else {
                            search_string += "t";
                        }
                    }
                    if (e->detail == wm->key_codes.u) {
                        if (e->state == SHIFT) {
                            search_string += "U";
                        } else {
                            search_string += "u";
                        }
                    }
                    if (e->detail == wm->key_codes.v) {
                        if (e->state == SHIFT) {
                            search_string += "V";
                        } else {
                            search_string += "v";
                        }
                    }
                    if (e->detail == wm->key_codes.w) {
                        if (e->state == SHIFT) {
                            search_string += "W";
                        } else {
                            search_string += "w";
                        }
                    }
                    if (e->detail == wm->key_codes.x) {
                        if (e->state == SHIFT) {
                            search_string += "X";
                        } else {
                            search_string += "x";
                        }
                    }
                    if (e->detail == wm->key_codes.y) {
                        if (e->state == SHIFT) {
                            search_string += "Y";
                        } else {
                            search_string += "y";
                        }
                    }
                    if (e->detail == wm->key_codes.z) {
                        if (e->state == SHIFT) {
                            search_string += "Z";
                        } else {
                            search_string += "z";
                        }
                    }

                    if (e->detail == wm->key_codes.space_bar) {
                        search_string += " ";
                    }
                    if (e->detail == wm->key_codes._delete) {
                        if (search_string.length() > 0) {
                            search_string.erase(search_string.length() - 1);
                            main_window.clear();
                        }
                    }
                    if (e->detail == wm->key_codes.enter) {
                        if (enter_function) {
                            enter_function();
                        }
                        search_string = "";
                        main_window.clear();
                    }

                    draw_text();
                }
            });
            
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == main_window)
                {
                    main_window.raise();
                    main_window.focus_input();
                }
            });
        }
        void draw_text() {
            main_window.draw_text(search_string.c_str(), WHITE, BLACK, "7x14", 2, 14);
            if (search_string.length() > 0)
            {
                results = file.search_for_binary(search_string.c_str());
                int entry_list_size = results.size(); 
                if (results.size() > 7)
                {
                    entry_list_size = 7;
                }
                main_window.height(20 * entry_list_size);
                xcb_flush(conn);
                for (int i = 0; i < entry_list_size; ++i)
                {
                    entry_list[i].draw_text(results[i].c_str(), WHITE, BLACK, "7x14", 2, 14);
                }
            }
        }
    ;
    private: // variables
        std::function<void()> enter_function;
        File file;
        std::vector<std::string> results;
        std::vector<window> entry_list;
    ;
};
class Mwm_Runner {
    public: // variabels
        window main_window;
        search_window search_window;
        uint32_t BORDER = 2;
        Launcher launcher;
    ;
    public: // methods
        void init() {
            main_window.create_default(
                screen->root,
                (screen->width_in_pixels / 2) - ((140 + (BORDER * 2)) / 2),
                0,
                140 + (BORDER * 2),
                400 + (BORDER * 2)
            );
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            main_window.set_backround_color(DARK_GREY);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            setup_events();
            search_window.create
            (
                main_window,
                2,
                2,
                main_window.width() - (BORDER * 2),
                main_window.height() - (BORDER * 2)
            );
            search_window.init();
            search_window.add_enter_action([this]()
            {
                launcher.program((char *) search_window.string().c_str());
                hide();
            });
        }
        void show() {
            main_window.raise();
            main_window.map();
            search_window.main_window.focus_input();
        }
    ;
    private: // functions
        void hide() {
            main_window.unmap();
            search_window.clear_search_string();
        }
        void setup_events() {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->detail == wm->key_codes.r) {
                    if (e->state == SUPER) {
                        show();
                    }
                }
            });

            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) 
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (!main_window.is_mapped())
                {
                    return;
                }
                if (e->event != main_window && e->event != search_window.main_window)
                {
                    hide();
                }
            });
        }
    ;
}; static Mwm_Runner * mwm_runner;
class add_app_dialog_window {
    public: // variabels
        window main_window;
        search_window search_window;
        client * c;
        buttons buttons;
        pointer pointer;
        Logger log;
    ;
    public: // methods
        void init() {
            create();
            configure_events();
        }
        void show() {
            create_client();
            search_window.create(main_window, DOCK_BORDER, DOCK_BORDER, (main_window.width() - (DOCK_BORDER * 2)), 20);
            search_window.init();
        }
        void add_enter_action(std::function<void()> enter_action) {
            enter_function = enter_action;
        }
    ;
    private: // functions
        void hide() {
            wm->send_sigterm_to_client(c);
        }
        void create() {
            main_window.create_default(screen->root, pointer.x(), pointer.y(), 300, 200);
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            main_window.grab_keys({ { Q, SHIFT | ALT } });
            main_window.set_backround_color(DARK_GREY);
        }
        void create_client() {
            main_window.x_y(pointer.x() - (main_window.width() / 2), pointer.y() - (main_window.height() / 2));
            c = wm->make_internal_client(main_window);
            c->x_y((pointer.x() - (c->width / 2)), (pointer.y() - (c->height / 2)));
            main_window.map();
            c->map();
        }
        void configure_events() {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == search_window.main_window)
                {
                    c->focus();
                }
            });
        }
    ;
    private: // variables
        std::function<void()> enter_function;
    ;
}; 
class File_App {
    public: // variabels
        window main_window;
        window left_side_window;

        client * c;
    ;
    public: // methods
        void init() {
            create_main_window();
            create_left_side_window();
            setup_events();
        }
    ;
    private: // functions
        void create_main_window() {
            int width = (screen->width_in_pixels / 2), height = (screen->height_in_pixels / 2);
            int x = ((screen->width_in_pixels / 2) - (width / 2)), y = ((screen->height_in_pixels / 2) - (height / 2));
            main_window.create_default(screen->root, x, y, width, height);

            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            if (!main_window.is_mask_active(XCB_EVENT_MASK_STRUCTURE_NOTIFY)) {
                log_error("could not apply XCB_EVENT_MASK_STRUCTURE_NOTIFY");
            }
            main_window.set_backround_color(BLUE);
            main_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            log_win("main_window: ", main_window);
        }
        void create_left_side_window() {
            left_side_window.create_default(main_window, 0, 0, 80, main_window.height());

            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            left_side_window.apply_event_mask(& mask);
            left_side_window.set_backround_color(DARK_GREY);
            left_side_window.grab_button({ { L_MOUSE_BUTTON, NULL } });
        }
        void make_internal_client() {
            c = new client;
            c->win = main_window;
            c->x = main_window.x();
            c->y = main_window.y();
            c->width = main_window.width();
            c->height = main_window.height();
            c->make_decorations();
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            wm->client_list.push_back(c);
        }
        void launch() {
            main_window.raise();
            main_window.map();
            make_internal_client();
            left_side_window.raise();
            left_side_window.map();
        }
        void setup_events() {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->detail == wm->key_codes.f) {
                    if (e->state == SUPER) {
                        launch();
                    }
                }
            });
            event_handler->setEventCallback(XCB_CONFIGURE_NOTIFY, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_configure_notify_event_t *>(ev);
            });
        }
    ;
    private:
        Logger log;
    ;
}; static File_App * file_app;
class Dock {
    public: // constructor
        Dock() {}
    ;
    public: // public variables 
        context_menu context_menu;
        window main_window;
        buttons buttons;
        uint32_t x = 0, y = 0, width = 48, height = 48;

        add_app_dialog_window add_app_dialog_window;
    ;
    public: // public methods 
        void init() {
            main_window.create_default(screen->root, 0, 0, width, height);
            setup_dock();
            configure_context_menu();
            make_apps();
            add_app_dialog_window.init();
            add_app_dialog_window.search_window.add_enter_action([this]()
            {
                launcher.program((char *) add_app_dialog_window.search_window.search_string.c_str());
            });
            context_menu.init();
            configure_events();
        }
        void add_app(const char * app_name) {
            if (!file.check_if_binary_exists(app_name)) {
                return;
            }
            apps.push_back(app_name);
        }
    ;
    private: // private variables
        std::vector<const char *> apps;
        Launcher launcher;
        Logger log;
        File file;
    ;
    private: // private methods
        void calc_size_pos() {
            int num_of_buttons = buttons.size();

            if (num_of_buttons == 0)
            {
                num_of_buttons = 1;
            }

            uint32_t calc_width = width * num_of_buttons;

            x = ((screen->width_in_pixels / 2) - (calc_width / 2));
            y = (screen->height_in_pixels - height);

            main_window.x_y_width_height(x, y, calc_width, height);
            xcb_flush(conn);
        }
        void setup_dock() {
            main_window.grab_button({ { R_MOUSE_BUTTON, NULL } });
            main_window.set_backround_color(DARK_GREY);
            calc_size_pos();
            main_window.map();
        }
        void configure_context_menu() {
            context_menu.add_entry("Add app", [this] () { add_app_dialog_window.show(); });
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
        }
        void make_apps() {
            for (const auto & app : apps) {
                buttons.add
                (app,
                [app, this] () 
                {
                    {
                        launcher.program((char *) app);
                    }
                });
                buttons.list[buttons.index()].create(
                    main_window,
                    ((buttons.index() * width) + DOCK_BORDER),
                    DOCK_BORDER,
                    (width - (DOCK_BORDER * 2)),
                    (height - (DOCK_BORDER * 2)),
                    BLACK
                );
                buttons.list[buttons.index()].put_icon_on_button();
            }
            calc_size_pos();
        }
        void configure_events() {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)
            {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == main_window)
                {
                    if (e->detail == R_MOUSE_BUTTON)
                    {
                        context_menu.show();
                    }
                }
                for (int i = 0; i < buttons.size(); ++i)
                {
                    if (e->event == buttons.list[i].window)
                    {
                        if (e->detail == L_MOUSE_BUTTON) 
                        {
                            buttons.list[i].activate();
                        }
                    }
                }
            });
        }
    ;
}; static Dock * dock;
class mv_client {
    public: // constructor
        mv_client(client * c, int start_x, int start_y) 
        : c(c), start_x(start_x), start_y(start_y) {
            if (c->win.is_EWMH_fullscreen()) {
                return;
            }

            pointer.grab(c->frame);
            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }
    ;
    private: // variabels
        client * c;
        pointer pointer;
        int start_x;
        int start_y;
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
        void snap(int x, int y) {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : wm->cur_d->current_clients) {
                // SNAP WINDOW TO 'RIGHT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x + cli->width - N && x < cli->x + cli->width + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height)) {
                    // SNAP WINDOW TO 'RIGHT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC) {  
                        c->frame.x_y((cli->x + cli->width), cli->y);
                        return;
                    }
                    // SNAP WINDOW TO 'RIGHT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC) {
                        c->frame.x_y((cli->x + cli->width), (cli->y + cli->height) - c->height);
                        return;
                    }
                    c->frame.x_y((cli->x + cli->width), y);
                    return;
                }
                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x + c->width > cli->x - N && x + c->width < cli->x + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height)) {
                    // SNAP WINDOW TO 'LEFT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC) {  
                        c->frame.x_y((cli->x - c->width), cli->y);
                        return;
                    }
                    // SNAP WINDOW TO 'LEFT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC) {
                        c->frame.x_y((cli->x - c->width), (cli->y + cli->height) - c->height);
                        return;
                    }                
                    c->frame.x_y((cli->x - c->width), y);
                    return;
                }
                // SNAP WINDOW TO 'BOTTOM' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y + cli->height - N && y < cli->y + cli->height + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width)) {
                    // SNAP WINDOW TO 'BOTTOM_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC) {  
                        c->frame.x_y(cli->x, (cli->y + cli->height));
                        return;
                    }
                    // SNAP WINDOW TO 'BOTTOM_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC) {
                        c->frame.x_y(((cli->x + cli->width) - c->width), (cli->y + cli->height));
                        return;
                    }
                    c->frame.x_y(x, (cli->y + cli->height));
                    return;
                }
                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y + c->height > cli->y - N && y + c->height < cli->y + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width)) {
                    // SNAP WINDOW TO 'TOP_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC) {  
                        c->frame.x_y(cli->x, (cli->y - c->height));
                        return;
                    }
                    // SNAP WINDOW TO 'TOP_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC) {
                        c->frame.x_y(((cli->x + cli->width) - c->width), (cli->y - c->height));
                        return;
                    }
                    c->frame.x_y(x, (cli->y - c->height));
                    return;
                }
            }

            // WINDOW TO EDGE OF SCREEN SNAPPING
            if (((x < N) && (x > -N)) && ((y < N) && (y > -N))) {
                c->frame.x_y(0, 0);
            } else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < N && y > -N)) {
                c->frame.x_y(RIGHT_, 0);
            } else if ((y < BOTTOM_ + N && y > BOTTOM_ - N) && (x < N && x > -N)) {
                c->frame.x_y(0, BOTTOM_);
            } else if ((x < N) && (x > -N)) { 
                c->frame.x_y(0, y);
            } else if (y < N && y > -N) {
                c->frame.x_y(x, 0);
            } else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < BOTTOM_ + N && y > BOTTOM_ - N)) {
                c->frame.x_y(RIGHT_, BOTTOM_);
            } else if ((x < RIGHT_ + N) && (x > RIGHT_ - N)) { 
                c->frame.x_y(RIGHT_, y);
            } else if (y < BOTTOM_ + N && y > BOTTOM_ - N) {
                c->frame.x_y(x, BOTTOM_);
            } else {
                c->frame.x_y(x, y);
            }
        }
        void run() { 
            while (shouldContinue) {
                ev = xcb_wait_for_event(conn);
                if (!ev) {
                    continue;
                }

                switch (ev->response_type & ~0x80) {
                    case XCB_MOTION_NOTIFY: {
                        const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                        int new_x = e->root_x - start_x - BORDER_SIZE;
                        int new_y = e->root_y - start_y - BORDER_SIZE;
                        
                        if (isTimeToRender()) {
                            snap(new_x, new_y);
                            xcb_flush(conn);
                        }
                        break;
                    }
                    case XCB_BUTTON_RELEASE: {
                        shouldContinue = false;
                        c->update();
                        break;
                    }
                }
                free(ev);
            }
        }
        bool isTimeToRender() {
            auto currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> elapsedTime = currentTime - lastUpdateTime;

            if (elapsedTime.count() >= frameDuration) {
                lastUpdateTime = currentTime;
                return true;
            }
            return false;
        }
    ;
};
void animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration) {
    Mwm_Animator anim(c->frame);
    anim.animate(
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
void animate_client(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration) {
    Mwm_Animator client_anim(c);
    client_anim.animate_client(
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
class change_desktop {
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
void move_to_next_desktop_w_app() {
    if (wm->cur_d->desktop == wm->desktop_list.size()) {
        return;
    }

    if (wm->focused_client) {
        wm->focused_client->desktop = wm->cur_d->desktop + 1;
    }

    change_desktop::teleport_to(wm->cur_d->desktop + 1);
}
void move_to_previus_desktop_w_app() {
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
class resize_client {
    public: // constructor
        /**
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

            pointer.grab(c->frame);
            pointer.teleport(c->x + c->width, c->y + c->height);
            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }
    ;
    public: // subclasses
        class no_border {
            public: // constructor
                no_border(client * & c, const uint32_t & x, const uint32_t & y)
                : c(c) {
                    if (c->win.is_EWMH_fullscreen()) {
                        return;
                    }
                    
                    pointer.grab(c->frame);
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
                pointer pointer;
                uint32_t y;
                const double frameRate = 120.0;
                std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
            ;
            private: // functions
                void teleport_mouse(edge edge) {
                    switch (edge) {
                        case edge::TOP:
                            pointer.teleport(pointer.x(), c->y);
                            break;
                        case edge::BOTTOM_edge:
                            pointer.teleport(pointer.x(), (c->y + c->height));
                            break;
                        case edge::LEFT:
                            pointer.teleport(c->x, pointer.y());
                            break;
                        case edge::RIGHT:
                            pointer.teleport((c->x + c->width), pointer.y());
                            break;
                        case edge::NONE:
                            break;
                        case edge::TOP_LEFT:
                            pointer.teleport(c->x, c->y);
                            break;
                        case edge::TOP_RIGHT:
                            pointer.teleport((c->x + c->width), c->y);
                            break;
                        case edge::BOTTOM_LEFT:
                            pointer.teleport(c->x, (c->y + c->height));
                            break;
                        case edge::BOTTOM_RIGHT:
                            pointer.teleport((c->x + c->width), (c->y + c->height));
                            break;
                    }
                }
                void resize_client(const uint32_t x, const uint32_t y, edge edge) {
                    switch (edge) {
                        case edge::LEFT:
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        case edge::RIGHT:
                            c->_width((x - c->x));
                            break;
                        case edge::TOP:
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        case edge::BOTTOM_edge:
                            c->_height((y - c->y));
                            break;
                        case edge::NONE:
                            return;
                        case edge::TOP_LEFT:
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        case edge::TOP_RIGHT:
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        case edge::BOTTOM_LEFT:
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        case edge::BOTTOM_RIGHT:
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                    }
                }
                void run(edge edge) {
                    xcb_generic_event_t * ev;
                    bool shouldContinue = true;
                    while (shouldContinue) {
                        ev = xcb_wait_for_event(conn);
                        if (!ev) {
                            continue;
                        }

                        switch (ev->response_type & ~0x80) {
                            case XCB_MOTION_NOTIFY: {
                                const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                                if (isTimeToRender()) {
                                    resize_client(e->root_x, e->root_y, edge);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: {
                                shouldContinue = false;                        
                                c->update();
                                break;
                            }
                        }
                        free(ev); 
                    }
                }
                bool isTimeToRender() {
                    const auto & currentTime = std::chrono::high_resolution_clock::now();
                    const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - lastUpdateTime;

                    if (elapsedTime.count() >= frameDuration) {
                        lastUpdateTime = currentTime;                         
                        return true; 
                    }
                    return false; 
                }
            ;
        };
        class border {
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
                            pointer.grab(c->frame);
                            teleport_mouse(_edge);
                            run_double(_edge);
                            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                            xcb_flush(conn);
                            return;
                        }
                    }
                    
                    pointer.grab(c->frame);
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
                pointer pointer; 
                const double frameRate = 120.0;
                std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
            ;
            private: // methods
                void teleport_mouse(edge edge) {
                    switch (edge) 
                    {
                        case edge::TOP:
                        {
                            pointer.teleport(pointer.x(), c->y);
                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            pointer.teleport(pointer.x(), (c->y + c->height));
                            break;
                        } 
                        case edge::LEFT:
                        {
                            pointer.teleport(c->x, pointer.y());
                            break;
                        }
                        case edge::RIGHT:
                        {
                            pointer.teleport((c->x + c->width), pointer.y());
                            break;
                        }
                        case edge::NONE:
                        {
                            break;
                        }
                        case edge::TOP_LEFT:
                        {
                            pointer.teleport(c->x, c->y);
                            break;
                        }
                        case edge::TOP_RIGHT:
                        {
                            pointer.teleport((c->x + c->width), c->y);
                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            pointer.teleport(c->x, (c->y + c->height));
                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {
                            pointer.teleport((c->x + c->width), (c->y + c->height));
                            break;
                        }
                    }
                }
                void resize_client(const uint32_t x, const uint32_t y, edge edge) {
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
                void resize_client(client * c, const uint32_t x, const uint32_t y, edge edge) {
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
                void snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox) {
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
                    while (shouldContinue) {
                        ev = xcb_wait_for_event(conn);
                        if (!ev) {
                            continue;
                        }

                        switch (ev->response_type & ~0x80) {
                            case XCB_MOTION_NOTIFY: {
                                const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                                if (isTimeToRender()) {
                                    snap(e->root_x, e->root_y, edge, 12);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: {
                                shouldContinue = false;                        
                                c->update();
                                break;
                            }
                        }
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
                bool isTimeToRender() {
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
        pointer pointer;
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
class max_win {
    public: // constructor
        enum max_win_type {
            BUTTON_MAXWIN,
            EWMH_MAXWIN
        };
        max_win(client * c, max_win_type type) {
            switch (type) {
                case EWMH_MAXWIN:
                    if (c->win.is_EWMH_fullscreen()) {
                        ewmh_unmax_win(c);
                    } else {
                        ewmh_max_win(c);
                    }
                    break;
                case BUTTON_MAXWIN:
                    if (is_max_win(c)) {
                        button_unmax_win(c);
                    } else {
                        button_max_win(c);
                    }
                    break;
            }
        }
    ;
    private: // functions
        void max_win_animate(client * c, const int & endX, const int & endY, const int & endWidth, const int & endHeight) {
            animate_client(
                c, 
                endX, 
                endY, 
                endWidth, 
                endHeight, 
                MAXWIN_ANIMATION_DURATION
            );
        }
        void save_max_ewmh_ogsize(client * c) {
            c->max_ewmh_ogsize.x      = c->x;
            c->max_ewmh_ogsize.y      = c->y;
            c->max_ewmh_ogsize.width  = c->width;
            c->max_ewmh_ogsize.height = c->height;
        }
        void ewmh_max_win(client * c) {
            save_max_ewmh_ogsize(c);
            max_win_animate(
                c, 
                - BORDER_SIZE, 
                - TITLE_BAR_HEIGHT - BORDER_SIZE, 
                screen->width_in_pixels + (BORDER_SIZE * 2), 
                screen->height_in_pixels + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)
            );
            c->win.set_EWMH_fullscreen_state();
            xcb_flush(conn);
        }
        void ewmh_unmax_win(client * c) {
            if (c->max_ewmh_ogsize.width > screen->width_in_pixels) {
                (c->max_ewmh_ogsize.width = screen->width_in_pixels / 2);
            }
            if (c->max_ewmh_ogsize.height > screen->height_in_pixels) {
                (c->max_ewmh_ogsize.height = screen->height_in_pixels / 2);
            }
            if (c->max_ewmh_ogsize.x >= screen->width_in_pixels - 1) {
                c->max_ewmh_ogsize.x = ((screen->width_in_pixels / 2) - (c->max_ewmh_ogsize.width / 2) - BORDER_SIZE);
            }
            if (c->max_ewmh_ogsize.y >= screen->height_in_pixels - 1) {
                c->max_ewmh_ogsize.y = ((screen->height_in_pixels / 2) - (c->max_ewmh_ogsize.height / 2) - TITLE_BAR_HEIGHT - BORDER_SIZE);
            }

            max_win_animate(
                c, 
                c->max_ewmh_ogsize.x, 
                c->max_ewmh_ogsize.y, 
                c->max_ewmh_ogsize.width, 
                c->max_ewmh_ogsize.height
            );
            c->win.unset_EWMH_fullscreen_state();
            xcb_flush(conn);
        }
        void save_max_button_ogsize(client * c) {
            c->max_button_ogsize.x      = c->x;
            c->max_button_ogsize.y      = c->y;
            c->max_button_ogsize.width  = c->width;
            c->max_button_ogsize.height = c->height;
        }
        void button_max_win(client * c) {
            save_max_button_ogsize(c);
            max_win_animate(
                c,
                0,
                0,
                screen->width_in_pixels,
                screen->height_in_pixels
            );
            xcb_flush(conn);
        }
        void button_unmax_win(client * c) {
            max_win_animate(
                c, 
                c->max_button_ogsize.x,
                c->max_button_ogsize.y,
                c->max_button_ogsize.width,
                c->max_button_ogsize.height
            );
            xcb_flush(conn);
        }
        bool is_max_win(client * c) {
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
namespace win_tools {
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
        wm->send_sigterm_to_client(c);
    }
}
/**
 * @class tile
 * @brief Represents a tile obj.
 * 
 * The `tile` class is responsible for managing the tiling behavior of windows in the window manager.
 * It provides methods to tile windows to the left, right, *up, or *down positions on the screen.
 * The class also includes helper methods to check the current tile position of a window and set the size and position of a window.
 */
class tile {
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
class Events {
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
        void setup() {
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
        void key_press_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                
            if (e->detail == wm->key_codes.t) {
                switch (e->state) 
                {
                    case CTRL + ALT:
                    {
                        wm->launcher.program((char *) "konsole");
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.q) {
                switch (e->state) 
                {
                    case SHIFT + ALT:
                    {
                        wm->quit(0);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.f11) {
                client * c = wm->client_from_window(& e->event);
                max_win(c, max_win::EWMH_MAXWIN);
            }
            if (e->detail == wm->key_codes.n_1) {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(1);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_2) {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(2);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_3) {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(3);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_4) {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(4);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.n_5) {
                switch (e->state) 
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(5);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.r_arrow) {
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
            if (e->detail == wm->key_codes.l_arrow) {
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
            if (e->detail == wm->key_codes.d_arrow) {
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
            if (e->detail == wm->key_codes.u_arrow) {
                switch (e->state) 
                {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::UP);
                        break;
                    ;
                }
            }
            if (e->detail == wm->key_codes.tab) {
                switch (e->state) 
                {
                    case ALT:
                        wm->cycle_focus();
                        break;
                    ;
                }
            }
            if (e->detail == wm->key_codes.k) {
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
        void map_notify_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_map_notify_event_t *>(ev);
            client * c = wm->client_from_window(& e->window);
            if (c) {
                c->update();
            }
        }
        void map_req_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_map_request_event_t *>(ev);
            client * c = wm->client_from_window(& e->window);
            if (c) {
                return;
            }
            wm->manage_new_client(e->window);
        }
        void button_press_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
            client * c;
            if (BORDER_SIZE == 0) {
                c = wm->client_from_pointer(10);
                if (c)
                {
                    if (e->detail == L_MOUSE_BUTTON)
                    {
                        c->raise();
                        resize_client::no_border border(c, 0, 0);
                        wm->focus_client(c);
                    }
                    return;
                }
            }
            if (e->event == wm->root) {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    wm->context_menu->show();
                    return;
                }
            }
            c = wm->client_from_any_window(& e->event);
            if (!c) {
                return;
            }
            if (e->detail == L_MOUSE_BUTTON) {
                if (e->event == c->win) {
                    switch (e->state) 
                    {
                        case ALT:
                            c->raise();
                            mv_client mv(c, e->event_x, e->event_y + 20);
                            wm->focus_client(c);
                            break;
                        ;
                    }
                    c->raise();
                    wm->focus_client(c);
                    return;
                }
                if (e->event == c->titlebar) {
                    c->raise();
                    mv_client mv(c, e->event_x, e->event_y);
                    wm->focus_client(c);
                    return;
                }
                if (e->event == c->close_button) {
                    win_tools::close_button_kill(c);
                    return;
                }
                if (e->event == c->max_button) {
                    client * c = wm->client_from_any_window(& e->event);
                    max_win(c, max_win::BUTTON_MAXWIN);
                    return;
                }
                if (e->event == c->border.left) {
                    resize_client::border border(c, edge::LEFT);
                    return;
                }
                if (e->event == c->border.right) {
                    resize_client::border border(c, edge::RIGHT);
                    return;
                }
                if (e->event == c->border.top) {
                    resize_client::border border(c, edge::TOP);
                    return;
                }
                if (e->event == c->border.bottom) {
                    resize_client::border(c, edge::BOTTOM_edge);
                }
                if (e->event == c->border.top_left) {
                    resize_client::border border(c, edge::TOP_LEFT);
                    return;
                }
                if (e->event == c->border.top_right) {
                    resize_client::border border(c, edge::TOP_RIGHT);
                    return;
                }
                if (e->event == c->border.bottom_left) {
                    resize_client::border border(c, edge::BOTTOM_LEFT);
                    return;
                }
                if (e->event == c->border.bottom_right) {
                    resize_client::border border(c, edge::BOTTOM_RIGHT);
                    return;
                }
            }
            if (e->detail == R_MOUSE_BUTTON) {
                switch (e->state) {
                    case ALT:
                        log_error("ALT + R_MOUSE_BUTTON");
                        c->raise();
                        resize_client resize(c, 0);
                        wm->focus_client(c);
                        return;
                }
            }
        }
        void configure_request_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_configure_request_event_t *>(ev);
            wm->data.width     = e->width;
            wm->data.height    = e->height;
            wm->data.x         = e->x;
            wm->data.y         = e->y;
        }
        void focus_in_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_focus_in_event_t *>(ev);
            client * c = wm->client_from_window( & e->event);
            if (c) {
                c->win.ungrab_button({ { L_MOUSE_BUTTON, NULL } });
                c->raise();
                c->win.set_active_EWMH_window();
                wm->focused_client = c;
            }
        }
        void focus_out_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_focus_out_event_t *>(ev);
            client * c = wm->client_from_window(& e->event);
            if (c) {
                c->win.grab_button({ { L_MOUSE_BUTTON, NULL } });
            }
        }
        void destroy_notify_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_destroy_notify_event_t *>(ev);
            client * c = wm->client_from_window(& e->event);
            if (!c) {
                return;
            }
            wm->send_sigterm_to_client(c);
        }
        void unmap_notify_handler(const xcb_generic_event_t * & ev) {
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
        void reparent_notify_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_reparent_notify_event_t *>(ev);
        }
        void enter_notify_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
        }
        void leave_notify_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_leave_notify_event_t *>(ev);
        }
        void motion_notify_handler(const xcb_generic_event_t * & ev) {
            const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
            // log_info(e->event_x);
        }
    ;
};
void setup_wm() {
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

    Events events;
    events.setup();

    file_app = new File_App;
    file_app->init();
}
int main() {
    LOG_start()
    setup_wm();
    event_handler->run();
    xcb_disconnect(conn);
    return 0;
}