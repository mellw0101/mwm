#include "structs.hpp"
#include <X11/Xlib.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <map>
#include <thread>
#include <vector>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#define main_cpp
#include "include.hpp"
// #include "mxb.hpp"

Logger log;

std::vector<client *> client_list; // global list of clients
std::vector<desktop *> desktop_list;

desktop * cur_d;
client * focused_client;
win_data data;

static xcb_connection_t * conn;
static xcb_ewmh_connection_t * ewmh; 
static const xcb_setup_t * setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t * screen;
static xcb_gcontext_t gc;
static xcb_window_t start_win;

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

        static XConnection * 
        mxb_connect(const char* display) 
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

        static int
        mxb_connection_has_error(XConnection * conn)
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

        class str 
        {
            public:
                str(const char* str = "") 
                {
                    length = manualStrlen(str);
                    data = new char[length + 1];
                    manualStrcpy(data, str);
                }

                // Move Constructor
                str(str&& other) noexcept
                : data(other.data), length(other.length) 
                {
                    other.data = nullptr;
                    other.length = 0;
                }

                // Move Assignment Operator
                str& operator=(str&& other) noexcept 
                {
                    if (this != &other) 
                    {
                        delete[] data;
                        data = other.data;
                        length = other.length;

                        other.data = nullptr;
                        other.length = 0;
                    }
                    return *this;
                }

                ~str() 
                {
                    delete[] data;
                }

                str(const str& other) 
                {
                    length = other.length;
                    data = new char[length + 1];
                    manualStrcpy(data, other.data);
                }

                str& operator=(const str& other) 
                {
                    if (this != &other) 
                    {
                        delete[] data;
                        length = other.length;
                        data = new char[length + 1];
                        manualStrcpy(data, other.data);
                    }
                    return *this;
                }

                operator const char*()
                {
                    return data;
                }
            ;

            public: // public methods 
                const char* c_str() const 
                {
                    return data;
                }

                void 
                append(const char* str) 
                {
                    size_t newLength = length + manualStrlen(str);
                    char* newData = new char[newLength + 1];

                    manualStrcpy(newData, data);
                    manualStrcpy(newData + length, str);

                    delete[] data;
                    data = newData;
                    length = newLength;
                }
            ;

            private: // private variables
                char* data;
                size_t length;
            ;

            private: // private methods
                static 
                size_t manualStrlen(const char* str) 
                {
                    size_t len = 0;
                    while (str && str[len] != '\0') 
                    {
                        ++len;
                    }
                    return len;
                }

                static void 
                manualStrcpy(char* dest, const char* src) 
                {
                    while (*src) 
                    {
                        *dest++ = *src++;
                    }
                    *dest = '\0';
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
            ;

            public: // public methods 
                public: // main public methods 
                    void
                    create( 
                        const uint8_t        & depth,
                        const xcb_window_t   & parent_window,
                        const int16_t        & x,
                        const int16_t        & y,
                        const uint16_t       & width,
                        const uint16_t       & height,
                        const uint16_t       & border_width,
                        const uint16_t       & _class,
                        const xcb_visualid_t & visual,
                        const uint32_t       & value_mask,
                        const void           * value_list)
                    {
                        _depth = depth;
                        _parent = parent_window;
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
                ;

                public: // configuration public methods
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
                        xcb_flush(conn);
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
                    set_backround_color(COLOR color)
                    {
                        xcb_change_window_attributes
                        (
                            conn,
                            _window,
                            XCB_CW_BACK_PIXEL,
                            (const uint32_t[1])
                            {
                                mxb::get::color(color)
                            }
                        );
                        xcb_flush(conn);
                    }
                ;
            ;

            private: // private variables 
                uint8_t        _depth;
                uint32_t       _window;
                uint32_t       _parent;
                int16_t        _x;
                int16_t        _y;
                uint16_t       _width;
                uint16_t       _height;
                uint16_t       _border_width;
                uint16_t       __class;
                xcb_visualid_t _visual;
                uint32_t       _value_mask;
                const void     * _value_list;
            ;

            private: // private methods 
                void
                make_window()
                {
                    log_func;
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
            ;
        };

        class Dialog_win
        {
            public:
                class context_menu
                {
                    public:
                        context_menu()
                        {
                            size_pos.x      = mxb::pointer::get::x();
                            size_pos.y      = mxb::pointer::get::y();
                            size_pos.width  = 120;
                            size_pos.height = 20;

                            border.left = size_pos.x;
                            border.right = (size_pos.x + size_pos.width);
                            border.top = size_pos.y;
                            border.bottom = (size_pos.y + size_pos.height);

                            create_dialog_win();
                        }

                        void
                        show()
                        {
                            size_pos.x = mxb::pointer::get::x();
                            size_pos.y = mxb::pointer::get::y();
                            uint32_t height = get_num_of_entries() * size_pos.height;
                            xcb_configure_window
                            (
                                conn,
                                dialog_window,
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, 
                                (const uint32_t[3])
                                {
                                    static_cast<const uint32_t &>(size_pos.x - BORDER_SIZE),
                                    static_cast<const uint32_t &>(size_pos.y - BORDER_SIZE),
                                    height
                                }
                            );
                            xcb_map_window(conn, dialog_window);
                            xcb_flush(conn);

                            mxb::win::raise(dialog_window);

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

                    private:
                        xcb_window_t dialog_window;
                        size_pos size_pos;
                        window_borders border;
                        int border_size = 1;
                        
                        class DialogEntry 
                        {
                            public:
                                DialogEntry() {}
                                xcb_window_t window;
                                int16_t border_size = 1;

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
                                    window = xcb_generate_id(conn);
                                    xcb_create_window
                                    (
                                        conn,
                                        XCB_COPY_FROM_PARENT,
                                        window,
                                        parent_window,
                                        x,
                                        y,
                                        width,
                                        height,
                                        0,
                                        XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                        XCB_COPY_FROM_PARENT,
                                        0,
                                        nullptr
                                    );
                                    mxb::set::win::backround::as_color(window, BLACK);
                                    uint32_t mask = XCB_EVENT_MASK_POINTER_MOTION;
                                    mxb::set::event_mask(& mask, window);
                                    mxb::win::grab::button(window, {{L_MOUSE_BUTTON, NULL}});
                                    xcb_map_window(conn, window);
                                    xcb_flush(conn);
                                }
                            ;

                            private:
                                const char * entryName;
                                std::function<void()> entryAction;
                            ;
                        };
                        std::vector<DialogEntry> entries;

                        void
                        create_dialog_win()
                        {
                            dialog_window = xcb_generate_id(conn);
                            xcb_create_window
                            (
                                conn,
                                XCB_COPY_FROM_PARENT,
                                dialog_window,
                                screen->root,
                                0,
                                0,
                                size_pos.width,
                                size_pos.height,
                                0,
                                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                XCB_COPY_FROM_PARENT, 
                                0, 
                                nullptr
                            );

                            uint32_t mask = 
                                XCB_EVENT_MASK_FOCUS_CHANGE        | 
                                XCB_EVENT_MASK_ENTER_WINDOW        |
                                XCB_EVENT_MASK_LEAVE_WINDOW        |
                                XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                                XCB_EVENT_MASK_POINTER_MOTION      
                            ;
                            mxb::set::event_mask(& mask, dialog_window);
                            mxb::set::win::backround::as_color(dialog_window, DARK_GREY);
                            xcb_flush(conn);
                            mxb::win::raise(dialog_window);
                        }
                        
                        void
                        hide()
                        {
                            xcb_unmap_window(conn, dialog_window);
                            xcb_flush(conn);
                            mxb::win::kill(dialog_window);
                            xcb_flush(conn);
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
                                    case XCB_LEAVE_NOTIFY: 
                                    {
                                        const auto * e = reinterpret_cast<const xcb_leave_notify_event_t *>(ev);
                                        if (e->event == dialog_window)
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
                                entry.make_window(dialog_window, 0, y, size_pos.width, size_pos.height);
                                mxb::draw::text(entry.window, entry.getName(), WHITE, BLACK, "7x14", 2, 14);
                                y += size_pos.height;
                            }
                        }
                    ;
                };

                class button
                {
                    public: // constructor 
                        button() {}
                    ;

                    public: // public variables 
                        mxb::window window;
                        const char * name;
                    ;

                    public: // public methods 
                        void
                        create(const xcb_window_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height, COLOR color)
                        {
                            window.create
                            (
                                XCB_COPY_FROM_PARENT,
                                parent_window,
                                x,
                                y,
                                width,
                                height,
                                0,
                                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                XCB_COPY_FROM_PARENT,
                                0,
                                NULL
                            );
                            window.set_backround_color(color);
                            window.grab_button({{L_MOUSE_BUTTON, NULL}});
                            window.map();
                            xcb_flush(conn);
                        }

                        void
                        action(std::function<void()> action)
                        {
                            button_action = action;
                        }

                        void 
                        activate() const 
                        {
                            LOG_func
                            button_action();
                        }
                    ;

                    private: // private variables 
                        std::function<void()> button_action;
                    ;
                };

                class buttons
                {
                    public: // public variables
                        std::vector<mxb::Dialog_win::button> list;
                    ;

                    public: // public methods
                        void
                        add(const char * name, std::function<void()> action)
                        {
                            mxb::Dialog_win::button button;
                            button.name = name;
                            button.action(action);
                            list.push_back(button);
                        }

                        int
                        size()
                        {
                            return list.size();
                        }

                        void
                        run_action(const uint32_t & window)
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

                    private: // private variables
                        int button_index = 0;
                    ;
                };

                class Dock
                {
                    public: // constructor
                        Dock() {}
                    ;

                    public: // public variables 
                        mxb::Dialog_win::context_menu context_menu;
                        mxb::window main_window;
                        mxb::Dialog_win::buttons buttons;
                        uint32_t x = 0, y = 0, width = 48, height = 48;
                    ;

                    public: // public methods 
                        void
                        init()
                        {
                            create_dock();
                            setup_dock();
                            configure_context_menu();
                            make_apps();
                        }

                        void 
                        button_press_handler(const uint32_t & window)
                        {
                            buttons.run_action(window);
                        }

                        void 
                        add_app(mxb::str app_name)
                        {
                            app_data app;
                            app.name = app_name;
                            mxb::str file = "/usr/bin/";
                            file.append(app_name);
                            app.file = file;
                            app.index = apps.size();
                            apps.push_back(app);
                        }
                    ;

                    private: // private variables
                        struct app_data
                        {
                            const char * name;
                            const char * file;
                            int index;
                        };
                        std::vector<app_data> apps;
                    ;

                    private: // private methods 
                        void
                        calc_size_pos()
                        {
                            int num_of_buttons = buttons.size();

                            if (num_of_buttons == 0)
                            {
                                num_of_buttons = 1;
                            }

                            uint32_t calc_width = width * num_of_buttons;

                            x = ((screen->width_in_pixels / 2) - (calc_width / 2));
                            y = (screen->height_in_pixels - height);

                            mxb::conf::win::x_y_width_height(main_window, x, y, calc_width, height);
                            xcb_flush(conn);
                        }

                        void
                        create_dock()
                        {
                            main_window.create
                            (
                                XCB_COPY_FROM_PARENT,
                                screen->root,
                                0,
                                0,
                                width,
                                height,
                                0,
                                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                XCB_COPY_FROM_PARENT,
                                0,
                                NULL
                            );   
                        }

                        void
                        setup_dock()
                        {
                            main_window.grab_button({{R_MOUSE_BUTTON, NULL}});
                            main_window.set_backround_color(RED);
                            calc_size_pos();
                            main_window.map();
                        }

                        void
                        configure_context_menu()
                        {
                            context_menu.addEntry("test with nullptr", nullptr);
                        }

                        void
                        make_apps()
                        {
                            for (const auto & app : apps)
                            {
                                buttons.add
                                (
                                    app.name, 
                                    [app] () 
                                    {
                                        {
                                            mxb::launch::program((char *) app.name);
                                        }
                                    }    
                                );
                                buttons.list[app.index].create(main_window, ((buttons.size() - 1) * width) + 2, 2, width - 4, height - 4, GREEN);
                            }
                            calc_size_pos();
                        }
                    ;
                };
            ;   
        };

        class Root
        {
            public:
                xcb_window_t window = screen->root;
                int16_t x = 0;
                int16_t y = 0;
                uint16_t width = screen->width_in_pixels;
                uint16_t height = screen->height_in_pixels;
            ;
        };

        class get 
        {
            public: 
                static xcb_connection_t * 
                connection() 
                {
                    return conn;
                }

                static xcb_ewmh_connection_t * 
                ewmh_connection() 
                {
                    return ewmh;
                }

                static const xcb_setup_t * 
                _setup() 
                {
                    return setup;
                }

                static xcb_screen_iterator_t 
                _iter() 
                {
                    return iter;
                }

                static xcb_screen_t * 
                _screen() 
                {
                    return screen;
                }

                static xcb_gcontext_t 
                _gc() 
                {
                    return gc;
                }

                class win 
                {
                    public:
                        static xcb_window_t 
                        root(xcb_window_t window) 
                        {
                            xcb_query_tree_cookie_t cookie;
                            xcb_query_tree_reply_t *reply;

                            cookie = xcb_query_tree(conn, window);
                            reply = xcb_query_tree_reply(conn, cookie, NULL);

                            if (!reply) 
                            {
                                log.log(ERROR, __func__, "Error: Unable to query the window tree.");
                                return (xcb_window_t) 0; // Invalid window ID
                            }

                            xcb_window_t root_window = reply->root;

                            free(reply);
                            return root_window;
                        }

                        static xcb_window_t
                        parent(xcb_window_t window) 
                        {
                            xcb_query_tree_cookie_t cookie;
                            xcb_query_tree_reply_t *reply;

                            cookie = xcb_query_tree(conn, window);
                            reply = xcb_query_tree_reply(conn, cookie, NULL);

                            if (!reply) 
                            {
                                log.log(ERROR, __func__, "Error: Unable to query the window tree.");
                                return (xcb_window_t) 0; // Invalid window ID
                            }

                            xcb_window_t parent_window = reply->parent;

                            free(reply);
                            return parent_window;
                        }

                        static xcb_window_t *
                        children(xcb_connection_t *conn, xcb_window_t window, uint32_t *child_count) 
                        {
                            *child_count = 0;
                            xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, window);
                            xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, NULL);

                            if (!reply) 
                            {
                                log.log(ERROR, __func__, "Error: Unable to query the window tree.");
                                return NULL;
                            }

                            *child_count = xcb_query_tree_children_length(reply);
                            xcb_window_t *children = static_cast<xcb_window_t *>(malloc(*child_count * sizeof(xcb_window_t)));

                            if (!children) 
                            {
                                log.log(ERROR, __func__, "Error: Unable to allocate memory for children.");
                                free(reply);
                                return NULL;
                            }

                            memcpy(children, xcb_query_tree_children(reply), *child_count * sizeof(xcb_window_t));
                            
                            free(reply);
                            return children;
                        }

                        static char * 
                        property(xcb_window_t window, const char * atom_name) 
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
                                    window,
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

                        static void
                        size_pos(xcb_window_t window, uint16_t & x, uint16_t & y, uint16_t & width, uint16_t & height) 
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                            xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                            if (geometry) 
                            {
                                x = geometry->x;
                                y = geometry->y;
                                width = geometry->width;
                                height = geometry->height;
                                free(geometry);
                            } 
                            else 
                            {
                                x = y = width = height = 200;
                            }
                        }

                        static void 
                        x_y(xcb_window_t window, uint16_t & x, uint16_t & y) 
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                            xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                            if (geometry) 
                            {
                                x = geometry->x;
                                y = geometry->y;
                                free(geometry);
                            } 
                            else 
                            {
                                x = y = 200;
                            }
                        }

                        static void 
                        width_height(xcb_window_t window, uint16_t & width, uint16_t & height) 
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                            xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                            if (geometry) 
                            {
                                width = geometry->width;
                                height = geometry->height;
                                free(geometry);
                            } 
                            else 
                            {
                                width = height = 200;
                            }
                        }

                        static uint16_t 
                        x(xcb_window_t window)
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
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
                        
                        static uint16_t 
                        y(xcb_window_t window)
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
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

                        static uint16_t 
                        width(xcb_window_t window) 
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
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
                        
                        static uint16_t 
                        height(xcb_window_t window) 
                        {
                            xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
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
                    ;
                };

                static xcb_atom_t
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
                        return XCB_ATOM_NONE;
                    } 

                    xcb_atom_t atom = reply->atom;
                    free(reply);
                    return atom;
                }

                static std::string 
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

                static uint32_t
                event_mask_sum(xcb_window_t window) 
                {
                    uint32_t mask = 0;

                    // Get the window attributes
                    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(conn, window);
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

                static std::vector<xcb_event_mask_t>
                event_mask(uint32_t maskSum)
                {
                    std::vector<xcb_event_mask_t> setMasks;
                    
                    // Iterate over all enum values
                    for (int mask = XCB_EVENT_MASK_KEY_PRESS; mask <= XCB_EVENT_MASK_OWNER_GRAB_BUTTON; mask <<= 1) 
                    {
                        if (maskSum & mask) 
                        {
                            setMasks.push_back(static_cast<xcb_event_mask_t>(mask));
                        }
                    }

                    return setMasks;
                }

                class color
                {
                    public:
                        color(COLOR color)
                        {
                            xcb_colormap_t colormap = screen->default_colormap;
                            rgb_color_code color_code = mxb::get::rgb_code(color);
                            xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply
                            (
                                conn, 
                                xcb_alloc_color
                                (
                                    conn,
                                    colormap,
                                    mxb::scale::from_8_to_16_bit(color_code.r), 
                                    mxb::scale::from_8_to_16_bit(color_code.g),
                                    mxb::scale::from_8_to_16_bit(color_code.b)
                                ), 
                                NULL
                            );
                            color_pixel = reply->pixel;
                        }

                        operator uint32_t()
                        {
                            return color_pixel;
                        }
                    ;

                    private:
                        uint32_t color_pixel;
                    ;
                };

                class rgb_code
                {
                    public:
                        rgb_code(COLOR rgb_code)
                        {
                            uint8_t r;
                            uint8_t g;
                            uint8_t b;
                            
                            switch (rgb_code) 
                            {
                                case COLOR::WHITE:
                                {
                                    r = 255;
                                    g = 255;
                                    b = 255;
                                    break;
                                }
                                case COLOR::BLACK:
                                {
                                    r = 0;
                                    g = 0;
                                    b = 0;
                                    break;
                                }
                                case COLOR::RED:
                                {
                                    r = 255;
                                    g = 0;
                                    b = 0;
                                    break;
                                }
                                case COLOR::GREEN:
                                {
                                    r = 0;
                                    g = 255;
                                    b = 0;
                                    break;
                                }
                                case COLOR::BLUE:
                                {
                                    r = 0;
                                    g = 0;
                                    b = 255;
                                    break;
                                }
                                case COLOR::YELLOW:
                                {
                                    r = 255;
                                    g = 255;
                                    b = 0;
                                    break;
                                }
                                case COLOR::CYAN:
                                {
                                    r = 0;
                                    g = 255;
                                    b = 255;
                                    break;
                                }
                                case COLOR::MAGENTA:
                                {
                                    r = 255;
                                    g = 0;
                                    b = 255;
                                    break;
                                }
                                case COLOR::GREY:
                                {
                                    r = 128;
                                    g = 128;
                                    b = 128;
                                    break;
                                }
                                case COLOR::LIGHT_GREY:
                                {
                                    r = 192;
                                    g = 192;
                                    b = 192;
                                    break;
                                }
                                case COLOR::DARK_GREY:
                                {
                                    r = 64;
                                    g = 64;
                                    b = 64;
                                    break;
                                }
                                case COLOR::ORANGE:
                                {
                                    r = 255;
                                    g = 165;
                                    b = 0;
                                    break;
                                }
                                case COLOR::PURPLE:
                                {
                                    r = 128;
                                    g = 0;
                                    b = 128;
                                    break;
                                }
                                case COLOR::BROWN:
                                {
                                    r = 165;
                                    g = 42;
                                    b = 42;
                                    break;
                                }
                                case COLOR::PINK:
                                {
                                    r = 255;
                                    g = 192;
                                    b = 203;
                                    break;
                                }
                                default:
                                {
                                    r = 0;
                                    g = 0;
                                    b = 0;
                                    break;
                                }
                            }

                            color.r = r;
                            color.g = g;
                            color.b = b;
                        }

                        operator rgb_color_code()
                        {
                            return color;
                        }
                    ;

                    private:
                        rgb_color_code color;
                    ;
                };

                class font
                {
                    public:
                        font(const char * font_name)
                        {
                            _font = xcb_generate_id(conn);
                            xcb_open_font
                            (
                                conn, 
                                _font, 
                                strlen(font_name),
                                font_name
                            );
                        }

                        operator xcb_font_t() const 
                        {
                            return _font;
                        }
                    ;

                    private:
                        xcb_font_t _font;
                    ;
                };
            ;
        };

        class set 
        {
            public: 
                static void 
                _conn(const char * displayname, int * screenp) 
                {
                    conn = xcb_connect(displayname, screenp);
                    mxb::check::_conn();
                }

                static void 
                _ewmh() 
                {
                    if (!(ewmh = static_cast<xcb_ewmh_connection_t *>(calloc(1, sizeof(xcb_ewmh_connection_t)))))
                    {
                        log_error("ewmh faild to initialize");
                        mxb::launch::program((char *) "/usr/bin/mwm-KILL");
                    }    
                    
                    xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms(conn, ewmh);
                    
                    if (!(xcb_ewmh_init_atoms_replies(ewmh, cookie, 0)))
                    {
                        log_error("xcb_ewmh_init_atoms_replies:faild");
                        exit(1);
                    }

                    const char * str = "mwm";
                    mxb::check::err
                    (
                        conn, 
                        xcb_ewmh_set_wm_name
                        (
                            ewmh, 
                            screen->root, 
                            strlen(str), 
                            str
                        ), 
                        __func__, 
                        "xcb_ewmh_set_wm_name"
                    );
                }

                static void 
                _setup() 
                {
                    setup = xcb_get_setup(conn);
                }

                static void 
                _iter() 
                {
                    iter = xcb_setup_roots_iterator(setup);
                }

                static void 
                _screen() 
                {
                    screen = iter.data;
                }

                static void 
                _gc(xcb_gcontext_t g) 
                {
                    gc = g;
                }

                static void
                event_mask(const uint32_t * mask, const xcb_window_t & window)
                {
                    xcb_change_window_attributes
                    (
                        conn,
                        window,
                        XCB_CW_EVENT_MASK,
                        mask
                    );
                }

                static void 
                event_mask(xcb_window_t window, const std::vector<xcb_event_mask_t>& masks) 
                {
                    uint32_t cumulativeMask = 0;
                    for (auto mask : masks) 
                    {
                        cumulativeMask |= mask;
                    }

                    uint32_t values[] = { cumulativeMask };
                    xcb_change_window_attributes(conn, window, XCB_CW_EVENT_MASK, values);
                    xcb_flush(conn);
                }

                class win
                {
                    public:
                        class backround
                        {
                            public:
                                class as_png
                                {
                                    public:
                                        as_png(const char * imagePath, const xcb_window_t & window)
                                        {
                                            // Load an image using Imlib2
                                            Imlib_Image image = imlib_load_image(imagePath);
                                            if (!image) 
                                            {
                                                log_error("Failed to load image: " + std::string(imagePath));
                                                return;
                                            }

                                            // Get the original image size
                                            imlib_context_set_image(image);
                                            int originalWidth = imlib_image_get_width();
                                            int originalHeight = imlib_image_get_height();

                                            // Calculate new size maintaining aspect ratio
                                            double aspectRatio = (double)originalWidth / originalHeight;
                                            int newHeight = mxb::get::win::height(window);
                                            int newWidth = (int)(newHeight * aspectRatio);

                                            // Scale the image if it is wider than the screen
                                            if (newWidth > mxb::get::win::width(window)) 
                                            {
                                                newWidth = mxb::get::win::width(window);
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

                                            xcb_pixmap_t pixmap = mxb::create::pixmap(window);
                                            xcb_gcontext_t gc = mxb::create::gc::graphics_exposure(window);
                                            xcb_rectangle_t rect = {0, 0, mxb::get::win::width(window), mxb::get::win::height(window)};
                                            xcb_poly_fill_rectangle
                                            (
                                                conn, 
                                                pixmap, 
                                                gc, 
                                                1, 
                                                &rect
                                            );

                                            // Calculate position to center the image
                                            int x = (mxb::get::win::width(window) - newWidth) / 2;
                                            int y = (mxb::get::win::height(window) - newHeight) / 2;

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
                                                window, 
                                                XCB_CW_BACK_PIXMAP, 
                                                &pixmap
                                            );

                                            // Cleanup
                                            xcb_free_gc(conn, gc); // Free the GC
                                            xcb_image_destroy(xcb_image);
                                            imlib_free_image(); // Free scaled image

                                            mxb::win::clear(window);
                                        }
                                    ;
                                };

                                class as_color
                                {
                                    public:
                                        as_color(const xcb_window_t & window, COLOR color)
                                        {
                                            xcb_change_window_attributes
                                            (
                                                conn,
                                                window,
                                                XCB_CW_BACK_PIXEL,
                                                (const uint32_t[1])
                                                {
                                                    mxb::get::color(color)
                                                }
                                            );
                                            xcb_flush(conn);
                                        }
                                    ;
                                };
                            ;
                        };
                    ;
                };
            ;
        };

        class check
        {
            public: 
                static void
                err(xcb_connection_t * connection, xcb_void_cookie_t cookie , const char * sender_function, const char * err_msg)
                {
                    xcb_generic_error_t * err = xcb_request_check(connection, cookie);
                    if (err)
                    {
                        log.log(ERROR, sender_function, err_msg, err->error_code);
                        free(err);
                    }
                }

                static void
                error(const int & code)
                {
                    switch (code) 
                    {
                        case CONN_ERR:
                        {
                            log_error("Connection error.");
                            quit(CONN_ERR);
                            break;
                        }
                        case EXTENTION_NOT_SUPPORTED_ERR:
                        {
                            log_error("Extension not supported.");
                            quit(EXTENTION_NOT_SUPPORTED_ERR);
                            break;
                        }
                        case MEMORY_INSUFFICIENT_ERR:
                        {
                            log_error("Insufficient memory.");
                            quit(MEMORY_INSUFFICIENT_ERR);
                            break;
                        }
                        case REQUEST_TO_LONG_ERR:
                        {
                            log_error("Request to long.");
                            quit(REQUEST_TO_LONG_ERR);
                            break;
                        }
                        case PARSE_ERR:
                        {
                            log_error("Parse error.");
                            quit(PARSE_ERR);
                            break;
                        }
                        case SCREEN_NOT_FOUND_ERR:
                        {
                            log_error("Screen not found.");
                            quit(SCREEN_NOT_FOUND_ERR);
                            break;
                        }
                        case FD_ERR:
                        {
                            log_error("File descriptor error.");
                            quit(FD_ERR);
                            break;
                        }
                    }
                }

                static void
                _conn()
                {
                    int status = xcb_connection_has_error(conn);
                    mxb::check::error(status);
                }
            ;
        };

        class EWMH 
        {
            public:
                EWMH(xcb_connection_t* connection, xcb_ewmh_connection_t* ewmh_connection)
                : connection(connection), ewmh_conn(ewmh_connection) {}

                // Function to check the type of a window
                std::vector<std::string> 
                WindowType(xcb_window_t window) 
                {
                    std::vector<std::string> types;
                    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_window_type(ewmh_conn, window);
                    xcb_ewmh_get_atoms_reply_t wm_window_type;

                    if (xcb_ewmh_get_wm_window_type_reply(ewmh_conn, cookie, &wm_window_type, nullptr) == 1) 
                    {
                        for (unsigned int i = 0; i < wm_window_type.atoms_len; i++) 
                        {
                            log_info(mxb::get::AtomName(wm_window_type.atoms[i]));
                            types.push_back(mxb::get::AtomName(wm_window_type.atoms[i]));
                        }
                        xcb_ewmh_get_atoms_reply_wipe(&wm_window_type);
                    }
                    return types;
                }

                bool 
                checkWindowDecorations(xcb_window_t window) 
                {
                    xcb_intern_atom_cookie_t* cookie = xcb_ewmh_init_atoms(connection, ewmh_conn);
                    xcb_ewmh_init_atoms_replies(ewmh_conn, cookie, nullptr);

                    xcb_ewmh_get_atoms_reply_t win_type_reply;
                    if (xcb_ewmh_get_wm_window_type_reply(ewmh_conn, xcb_ewmh_get_wm_window_type(ewmh_conn, window), &win_type_reply, nullptr)) 
                    {
                        for (unsigned int i = 0; i < win_type_reply.atoms_len; i++) 
                        {
                            if (win_type_reply.atoms[i] == ewmh_conn->_NET_WM_WINDOW_TYPE_NORMAL) 
                            {
                                xcb_ewmh_get_atoms_reply_wipe(&win_type_reply);
                                return true; // Window is likely to be decorated
                            }
                        }
                        xcb_ewmh_get_atoms_reply_wipe(&win_type_reply);
                    }

                    return false; // Window is likely not decorated
                }

                void
                check_extends(xcb_window_t window)
                {
                    xcb_ewmh_get_extents_reply_t extents;
                    if (xcb_ewmh_get_frame_extents_reply(ewmh_conn, xcb_ewmh_get_frame_extents(ewmh_conn, window), &extents, nullptr)) 
                    {
                        log.log(INFO_PRIORITY, __func__, "Extents: left: %d, right: %d, top: %d, bottom: %d", extents.left, extents.right, extents.top, extents.bottom);
                    }
                }

                bool 
                checkWindowFrameExtents(xcb_window_t window) 
                {
                    xcb_ewmh_get_extents_reply_t extents;
                    xcb_get_property_cookie_t cookie = xcb_ewmh_get_frame_extents(ewmh_conn, window);

                    if (xcb_ewmh_get_frame_extents_reply(ewmh_conn, cookie, &extents, nullptr)) 
                    {
                        bool hasDecorations = extents.left > 0 || extents.right > 0 || extents.top > 0 || extents.bottom > 0;
                        return hasDecorations;
                    }

                    return false; // Unable to determine or no decorations
                }

                class set
                {
                    public:
                        static void
                        active_window(xcb_window_t window)
                        {
                            xcb_ewmh_set_active_window(ewmh, 0, window); // 0 for the first (default) screen
                            xcb_flush(conn);
                        }

                        static void
                        max_win_state(const xcb_window_t & window)
                        {
                            xcb_change_property
                            (
                                conn,
                                XCB_PROP_MODE_REPLACE,
                                window,
                                ewmh->_NET_WM_STATE,
                                XCB_ATOM_ATOM,
                                32,
                                1,
                                &ewmh->_NET_WM_STATE_FULLSCREEN
                            );
                            xcb_flush(conn);
                        }
                    ;
                };

                class unset
                {
                    public:
                        static void
                        max_win_state(const xcb_window_t & window)
                        {
                            xcb_change_property
                            (
                                conn,
                                XCB_PROP_MODE_REPLACE,
                                window,
                                ewmh->_NET_WM_STATE, 
                                XCB_ATOM_ATOM,
                                32,
                                0,
                                0
                            );
                            xcb_flush(conn);
                        }
                    ;
                };

                class get
                {
                    public:
                        static xcb_window_t
                        active_window()
                        {
                            xcb_window_t window = 0;
                            xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, 0), &window, NULL);
                            return window;
                        }
                    ;
                };

                class check
                {
                    public:
                        static bool
                        active_window(xcb_window_t window)
                        {
                            xcb_window_t active_window = 0;
                            xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, 0), &active_window, NULL);
                            return window == active_window;
                        }
                        
                        static bool 
                        isWindowNormalType(xcb_window_t window) 
                        {
                            xcb_ewmh_get_atoms_reply_t type_reply;
                            if (xcb_ewmh_get_wm_window_type_reply(ewmh, xcb_ewmh_get_wm_window_type(ewmh, window), &type_reply, nullptr)) 
                            {
                                for (unsigned int i = 0; i < type_reply.atoms_len; ++i) 
                                {
                                    if (type_reply.atoms[i] == ewmh->_NET_WM_WINDOW_TYPE_NORMAL) 
                                    {
                                        xcb_ewmh_get_atoms_reply_wipe(&type_reply);
                                        return true;
                                    }
                                }
                                xcb_ewmh_get_atoms_reply_wipe(&type_reply);
                            }
                            return false;
                        }

                        static bool 
                        is_window_fullscreen(xcb_window_t window) 
                        {
                            xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(ewmh, window);
                            xcb_ewmh_get_atoms_reply_t wm_state;
                            if (xcb_ewmh_get_wm_state_reply(ewmh, cookie, &wm_state, NULL) == 1) 
                            {
                                for (unsigned int i = 0; i < wm_state.atoms_len; i++) 
                                {
                                    log_info(mxb::get::AtomName(wm_state.atoms[i]));
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
                    ;
                };
            ;

            private:
                xcb_connection_t* connection;
                xcb_ewmh_connection_t* ewmh_conn;
            ;
        };

        class Client 
        {
            public:
                class calc
                {
                    public:
                        static client *
                        client_edge_prox_to_pointer(const int & prox)
                        {
                            const uint32_t & x = mxb::pointer::get::x();
                            const uint32_t & y = mxb::pointer::get::y();
                            for (const auto & c : cur_d->current_clients)
                            {
                                // LEFT EDGE OF CLIENT
                                if (x > c->x - prox && x <= c->x)
                                {
                                    return c;
                                }

                                // RIGHT EDGE OF CLIENT
                                if (x >= c->x + c->width && x < c->x + c->width + prox)
                                {
                                    return c;
                                }

                                // TOP EDGE OF CLIENT
                                if (y > c->y - prox && y <= c->y)
                                {
                                    return c;
                                }

                                // BOTTOM EDGE OF CLIENT
                                if (y >= c->y + c->height && y < c->y + c->height + prox)
                                {
                                    return c;
                                }
                            }
                            return nullptr;
                        }

                        static std::map<client *, edge>
                        if_client_is_next_to_other_client(client * c, edge c_edge)
                        {
                            std::map<client *, edge> map;
                            for (client * c2 : cur_d->current_clients)
                            {
                                if (c == c2)
                                {
                                    continue;
                                }

                                if (c_edge == edge::LEFT)
                                {
                                    if (c->x == c2->x + c2->width)
                                    {
                                        map[c2] = edge::RIGHT;
                                        return map;
                                    }
                                }

                                
                                if (c_edge == edge::RIGHT)
                                {
                                    if (c->x + c->width == c2->x)
                                    {
                                        map[c2] = edge::LEFT;
                                        return map;
                                    }
                                }

                                
                                if (c_edge == edge::TOP)
                                {
                                    if (c->y == c2->y + c2->height)
                                    {
                                        map[c2] = edge::BOTTOM_edge;
                                        return map;
                                    }
                                }

                                
                                if (c_edge == edge::BOTTOM_edge)
                                {
                                    if (c->y + c->height == c2->y)
                                    {
                                        map[c2] = edge::TOP;
                                        return map;
                                    }
                                }
                            }

                            map[nullptr] = edge::NONE;
                            return map;
                        }
                    ;
                };  

                class get
                {
                    public:    
                        static edge
                        client_edge(client * c, const int & prox)
                        {
                            const uint32_t & x = mxb::pointer::get::x();
                            const uint32_t & y = mxb::pointer::get::y();

                            const uint32_t & top_border = c->y;
                            const uint32_t & bottom_border = (c->y + c->height);
                            const uint32_t & left_border = c->x;
                            const uint32_t & right_border = (c->x + c->width);

                            // TOP EDGE OF CLIENT
                            if (((y > top_border - prox) && (y <= top_border))
                             && ((x > left_border + prox) && (x < right_border - prox)))
                            {
                                return edge::TOP;
                            }

                            // BOTTOM EDGE OF CLIENT
                            if (((y >= bottom_border) && (y < bottom_border + prox))
                             && ((x > left_border + prox) && (x < right_border - prox)))
                            {
                                return edge::BOTTOM_edge;
                            }

                            // LEFT EDGE OF CLIENT
                            if (((x > left_border) - prox && (x <= left_border))
                             && ((y > top_border + prox) && (y < bottom_border - prox)))
                            {
                                return edge::LEFT;
                            }

                            // RIGHT EDGE OF CLIENT
                            if (((x >= right_border) && (x < right_border + prox))
                             && ((y > top_border + prox) && (y < bottom_border - prox)))
                            {
                                return edge::RIGHT;
                            }

                            // TOP LEFT CORNER OF CLIENT
                            if (((x > left_border - prox) && x < left_border + prox)
                             && ((y > top_border - prox) && y < top_border + prox))
                            {
                                return edge::TOP_LEFT;
                            }

                            // TOP RIGHT CORNER OF CLIENT
                            if (((x > right_border - prox) && x < right_border + prox)
                             && ((y > top_border - prox) && y < top_border + prox))
                            {
                                return edge::TOP_RIGHT;
                            }

                            // BOTTOM LEFT CORNER OF CLIENT
                            if (((x > left_border - prox) && x < left_border + prox)
                             && ((y > bottom_border - prox) && y < bottom_border + prox))
                            {
                                return edge::BOTTOM_LEFT;
                            }

                            // BOTTOM RIGHT CORNER OF CLIENT
                            if (((x > right_border - prox) && x < right_border + prox)
                             && ((y > bottom_border - prox) && y < bottom_border + prox))
                            {
                                return edge::BOTTOM_RIGHT;
                            }


                            return edge::NONE;
                        }

                        static client * 
                        client_from_all_win(const xcb_window_t * window) 
                        {
                            for (const auto & c : client_list) 
                            {
                                if (* window == c->win 
                                || * window == c->frame 
                                || * window == c->titlebar 
                                || * window == c->close_button 
                                || * window == c->max_button 
                                || * window == c->min_button 
                                || * window == c->border.left 
                                || * window == c->border.right 
                                || * window == c->border.top 
                                || * window == c->border.bottom
                                || * window == c->border.top_left
                                || * window == c->border.top_right
                                || * window == c->border.bottom_left
                                || * window == c->border.bottom_right) 
                                {
                                    return c;
                                }
                            }
                            return nullptr; /*
                             *
                             * THIS WILL
                             * RETURN 'nullptr' BECAUSE THE
                             * WINDOW DOES NOT BELONG TO ANY 
                             * CLIENT IN THE CLIENT LIST
                             *  
                             */ 
                        }
                    ;
                };

                static void
                unmap(client * c)
                {
                    xcb_unmap_window(conn, c->frame);
                    xcb_flush(conn);
                }

                static void
                map(client * c)
                {
                    xcb_map_window(conn, c->frame);
                    xcb_flush(conn);
                }

                static void
                remove(client * c)
                {
                    client_list.erase(std::remove(client_list.begin(), client_list.end(), c), client_list.end());
                    cur_d->current_clients.erase(std::remove(cur_d->current_clients.begin(), cur_d->current_clients.end(), c), cur_d->current_clients.end());
                    delete c;
                }

                static void
                remove(client * c, std::vector<client *> & vec)
                {
                    if (!c)
                    {
                        log_error("client is nullptr.");
                    }
                    vec.erase(std::remove(vec.begin(), vec.end(), c), vec.end());
                    delete c;
                }

                static void
                update(client * c)
                {
                    xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, c->frame);
                    xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                    if (geometry) 
                    {
                        c->x = geometry->x;
                        c->y = geometry->y;
                        c->width = geometry->width;
                        c->height = geometry->height;
                        free(geometry);
                    } 
                    else 
                    {
                        c->x = c->y = c->width = c->height = 200;
                    }
                }

                static void 
                raise(client * c) 
                {
                    uint32_t values[1] = {XCB_STACK_MODE_ABOVE};
                    xcb_configure_window
                    (
                        conn,
                        c->frame,
                        XCB_CONFIG_WINDOW_STACK_MODE, 
                        values
                    );
                    xcb_flush(conn);
                }

                static void
                send_sigterm(client * c)
                {
                    if (!c)
                    {
                        return;
                    }

                    xcb_unmap_window(conn, c->win);
                    xcb_unmap_window(conn, c->close_button);
                    xcb_unmap_window(conn, c->max_button);
                    xcb_unmap_window(conn, c->min_button);
                    xcb_unmap_window(conn, c->titlebar);
                    xcb_unmap_window(conn, c->frame);
                    xcb_unmap_window(conn, c->border.left);
                    xcb_unmap_window(conn, c->border.right);
                    xcb_unmap_window(conn, c->border.top);
                    xcb_unmap_window(conn, c->border.bottom);
                    xcb_unmap_window(conn, c->border.top_right);
                    xcb_unmap_window(conn, c->border.bottom_left);
                    xcb_unmap_window(conn, c->border.bottom_right);
                    xcb_flush(conn);

                    mxb::win::kill(c->win);
                    mxb::win::kill(c->close_button);
                    mxb::win::kill(c->max_button);
                    mxb::win::kill(c->min_button);
                    mxb::win::kill(c->titlebar);
                    mxb::win::kill(c->frame);
                    mxb::win::kill(c->border.left);
                    mxb::win::kill(c->border.right);
                    mxb::win::kill(c->border.top);
                    mxb::win::kill(c->border.bottom);
                    mxb::win::kill(c->border.top_right);
                    mxb::win::kill(c->border.bottom_left);
                    mxb::win::kill(c->border.bottom_right);
                    xcb_flush(conn);
                }
            ;
        };

        class create
        {
            public:
                class gc
                {
                    public:
                        class graphics_exposure
                        {
                            public:
                                graphics_exposure(const xcb_window_t & window)
                                {
                                    gc = xcb_generate_id(conn);
                                    mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
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
                                        window,
                                        mask,
                                        values
                                    );
                                }

                                operator xcb_gcontext_t() const 
                                {
                                    return gc;
                                }
                            ;

                            private:
                                xcb_gcontext_t gc;
                                uint32_t mask;
                            ;
                        };

                        class font
                        {
                            public:
                                font(xcb_window_t window, const COLOR & text_color, const COLOR & backround_color, xcb_font_t font)
                                {
                                    gc = xcb_generate_id(conn);

                                    xcb_create_gc
                                    (
                                        conn, 
                                        gc, 
                                        window, 
                                        XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT, 
                                        (const uint32_t[3])
                                        {
                                            mxb::get::color(text_color),
                                            mxb::get::color(backround_color),
                                            font
                                        }
                                    );
                                }

                                operator xcb_gcontext_t() const 
                                {
                                    return gc;
                                }
                            ;

                            private:
                                xcb_gcontext_t gc;
                            ;
                        };
                    ;
                };

                class new_desktop
                {
                    public:
                        new_desktop(const uint16_t & n)
                        {
                            desktop * d = new desktop;
                            d->desktop  = n;
                            d->width    = screen->width_in_pixels;
                            d->height   = screen->height_in_pixels;
                            cur_d       = d;
                            desktop_list.push_back(d);
                        }
                    ;
                };

                static void
                png(const std::string& file_name, const bool bitmap[20][20]) 
                {
                    FILE *fp = fopen(file_name.c_str(), "wb");
                    if (!fp) 
                    {
                        throw std::runtime_error("Failed to create PNG file");
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (!png_ptr) 
                    {
                        fclose(fp);
                        throw std::runtime_error("Failed to create PNG write struct");
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (!info_ptr) 
                    {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, NULL);
                        throw std::runtime_error("Failed to create PNG info struct");
                    }

                    if (setjmp(png_jmpbuf(png_ptr))) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        throw std::runtime_error("Error during PNG creation");
                    }

                    png_init_io(png_ptr, fp);
                    png_set_IHDR(png_ptr, info_ptr, 20, 20, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                    png_write_info(png_ptr, info_ptr);

                    // Write character bitmap to PNG
                    png_bytep row = new png_byte[20];
                    for (int y = 0; y < 20; y++) 
                    {
                        for (int x = 0; x < 20; x++) 
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

                static void 
                png(const std::string& file_name, bool** bitmap, int width, int height) 
                {
                    FILE *fp = fopen(file_name.c_str(), "wb");
                    if (!fp) 
                    {
                        throw std::runtime_error("Failed to create PNG file");
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (!png_ptr) 
                    {
                        fclose(fp);
                        throw std::runtime_error("Failed to create PNG write struct");
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (!info_ptr) 
                    {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, NULL);
                        throw std::runtime_error("Failed to create PNG info struct");
                    }

                    if (setjmp(png_jmpbuf(png_ptr))) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        throw std::runtime_error("Error during PNG creation");
                    }

                    png_init_io(png_ptr, fp);
                    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                    png_write_info(png_ptr, info_ptr);

                    // Write character bitmap to PNG
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

                static void 
                png(const std::string& file_name, const int bitmap[20][20]) 
                {
                    FILE *fp = fopen(file_name.c_str(), "wb");
                    if (!fp) {
                        throw std::runtime_error("Failed to create PNG file");
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (!png_ptr) {
                        fclose(fp);
                        throw std::runtime_error("Failed to create PNG write struct");
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (!info_ptr) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, NULL);
                        throw std::runtime_error("Failed to create PNG info struct");
                    }

                    if (setjmp(png_jmpbuf(png_ptr))) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        throw std::runtime_error("Error during PNG creation");
                    }

                    png_init_io(png_ptr, fp);
                    png_set_IHDR(png_ptr, info_ptr, 20, 20, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                    png_write_info(png_ptr, info_ptr);

                    // Write character bitmap to PNG
                    png_bytep row = new png_byte[20];
                    for (int y = 0; y < 20; y++) {
                        for (int x = 0; x < 20; x++) {
                            if (bitmap[y][x] < 0 || bitmap[y][x] > 10) {
                                throw std::runtime_error("Invalid bitmap value");
                            }
                            // Scale the value to the range 0x00 to 0xFF
                            row[x] = static_cast<png_byte>((bitmap[y][x] * 255) / 10);
                        }
                        png_write_row(png_ptr, row);
                    }
                    delete[] row;

                    png_write_end(png_ptr, NULL);

                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                }

                static void 
                png(const std::string& file_name, const std::vector<std::vector<bool>>& bitmap) 
                {
                    int width = bitmap[0].size();
                    int height = bitmap.size();

                    FILE *fp = fopen(file_name.c_str(), "wb");
                    if (!fp) {
                        throw std::runtime_error("Failed to create PNG file");
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (!png_ptr) {
                        fclose(fp);
                        throw std::runtime_error("Failed to create PNG write struct");
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (!info_ptr) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, NULL);
                        throw std::runtime_error("Failed to create PNG info struct");
                    }

                    if (setjmp(png_jmpbuf(png_ptr))) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        throw std::runtime_error("Error during PNG creation");
                    }

                    png_init_io(png_ptr, fp);
                    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                    png_write_info(png_ptr, info_ptr);

                    // Write bitmap to PNG
                    png_bytep row = new png_byte[width];
                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            row[x] = bitmap[y][x] ? 0xFF : 0x00;
                        }
                        png_write_row(png_ptr, row);
                    }
                    delete[] row;

                    png_write_end(png_ptr, NULL);

                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                }

                class Bitmap
                {
                    public:
                        Bitmap(int width, int height) 
                        : width(width), height(height), bitmap(height, std::vector<bool>(width, false)) {}

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
                                log_error("Failed to create PNG file");
                                return;
                            }

                            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
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
                                png_destroy_write_struct(&png_ptr, nullptr);
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
                    
                    private:
                        int width, height;
                        std::vector<std::vector<bool>> bitmap;
                    ;
                };

                class icon
                {
                    public:
                        static void
                        close_button()
                        {

                        }
                        
                        static void
                        max_button(const char * file_creation_path)
                        {
                            mxb::create::Bitmap bitmap(20, 20);
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
                            bitmap.exportToPng(file_creation_path);
                        }
                        
                        static void
                        min_button()
                        {

                        }
                    ;
                };

                static xcb_pixmap_t
                pixmap(const xcb_window_t & window)
                {
                    xcb_pixmap_t pixmap = xcb_generate_id(conn);
                    xcb_create_pixmap
                    (
                        conn, 
                        screen->root_depth, 
                        pixmap, 
                        window, 
                        mxb::get::win::width(window), 
                        mxb::get::win::height(window)
                    );
                    return pixmap;
                }
            ;
        };

        class scale
        {
            public:
                static uint16_t
                from_8_to_16_bit(const uint8_t & n)
                {
                    return (n << 8) | n;
                }
            ;
        };

        class conf
        {
            public:
                class win
                {
                    public:
                        static void
                        x(const xcb_window_t & window, const uint32_t & x)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_X,
                                (const uint32_t[1])
                                {
                                    x
                                }
                            );
                        }

                        static void
                        y(const xcb_window_t & window, const uint32_t & y)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_Y,
                                (const uint32_t[1])
                                {
                                    y
                                }
                            );
                        }

                        static void
                        width(const xcb_window_t & window, const uint32_t & width)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_WIDTH,
                                (const uint32_t[1])
                                {
                                    width
                                }
                            );
                        }

                        static void
                        height(const xcb_window_t & window, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[1])
                                {
                                    height
                                }
                            );
                        }

                        static void
                        x_y(const xcb_window_t & window, const uint32_t & x, const uint32_t & y)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                                (const uint32_t[2])
                                {
                                    x,
                                    y
                                }
                            );
                        }

                        static void
                        width_height(const xcb_window_t & window, const uint32_t & width, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[2])
                                {
                                    width,
                                    height
                                }
                            );
                        }

                        static void
                        x_y_width_height(const xcb_window_t & window, const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[4])
                                {
                                    x,
                                    y,
                                    width,
                                    height
                                }
                            );
                        }

                        static void
                        x_width_height(const xcb_window_t & window, const uint32_t & x, const uint32_t & width, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[3])
                                {
                                    x,
                                    width,
                                    height
                                }
                            );
                        }

                        static void
                        y_width_height(const xcb_window_t & window, const uint32_t & y, const uint32_t & width, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[3])
                                {
                                    y,
                                    width,
                                    height
                                }
                            );
                        }

                        static void
                        x_width(const xcb_window_t & window, const uint32_t & x, const uint32_t & width)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH,
                                (const uint32_t[2])
                                {
                                    x,
                                    width
                                }
                            );
                        }

                        static void
                        x_height(const xcb_window_t & window, const uint32_t & x, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[2])
                                {
                                    x,
                                    height
                                }
                            );
                        }
                        
                        static void
                        y_width(const xcb_window_t & window, const uint32_t & y, const uint32_t & width)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH,
                                (const uint32_t[2])
                                {
                                    y,
                                    width
                                }
                            );
                        }
                        
                        static void 
                        y_height(const xcb_window_t & window, const uint32_t & y, const uint32_t & height)
                        {
                            xcb_configure_window
                            (
                                conn, 
                                window, 
                                XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT,
                                (const uint32_t[2])
                                {
                                    y,
                                    height
                                }
                            );
                        } 
                    ;
                };
            ;
        };

        class launch
        {
            public:
                static void
                program(char * program)
                {
                    if (fork() == 0) 
                    {
                        setsid();
                        execvp(program, (char *[]) { program, NULL });
                    }
                }
            ;
        };

        class win
        {
            public:
                class grab
                {
                    public:
                        class button
                        {
                            public:
                                button(const xcb_window_t & window, const uint8_t & button, const uint16_t & modifiers)
                                {
                                    xcb_grab_button
                                    (
                                        conn, 
                                        1, // 'owner_events'. Set to 0 for no event propagation
                                        window, 
                                        XCB_EVENT_MASK_BUTTON_PRESS, // Event mask
                                        XCB_GRAB_MODE_ASYNC, // Pointer mode
                                        XCB_GRAB_MODE_ASYNC, // Keyboard mode
                                        XCB_NONE, // Confine to window: none
                                        XCB_NONE, // Cursor: none
                                        button, 
                                        modifiers
                                    );
                                    xcb_flush(conn); // Flush the request to the X server
                                }

                                button(const xcb_window_t & window, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
                                {
                                    for (const auto & binding : bindings)
                                    {
                                        const uint8_t & button = binding.first;
                                        const uint16_t & modifier = binding.second;
                                        xcb_grab_button
                                        (
                                            conn, 
                                            1, 
                                            window, 
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
                            ;
                        };
                    ;
                };

                class ungrab
                {
                    public:
                        class button
                        {
                            public:
                                button(const xcb_window_t & window, const uint8_t & button, const uint16_t & modifiers)
                                {
                                    xcb_ungrab_button
                                    (
                                        conn, 
                                        button, 
                                        window, 
                                        modifiers
                                    );
                                    xcb_flush(conn); // Flush the request to the X server
                                }
                            ;
                        };
                    ;
                };

                static void
                clear(const xcb_window_t & window)
                {
                    xcb_clear_area
                    (
                        conn, 
                        0,
                        window,
                        0, 
                        0,
                        mxb::get::win::width(window),
                        mxb::get::win::height(window)
                    );
                }

                static void 
                kill(xcb_window_t window)
                {
                    xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, 12, "WM_PROTOCOLS");
                    xcb_intern_atom_reply_t * protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, NULL);

                    xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
                    xcb_intern_atom_reply_t * delete_reply = xcb_intern_atom_reply(conn, delete_cookie, NULL);

                    if (!protocols_reply || !delete_reply) 
                    {
                        log_error("Could not create atoms.");
                        free(protocols_reply);
                        free(delete_reply);
                        return;
                    }

                    xcb_client_message_event_t ev = {0};
                    ev.response_type = XCB_CLIENT_MESSAGE;
                    ev.window = window;
                    ev.format = 32;
                    ev.sequence = 0;
                    ev.type = protocols_reply->atom;
                    ev.data.data32[0] = delete_reply->atom;
                    ev.data.data32[1] = XCB_CURRENT_TIME;

                    xcb_send_event
                    (
                        conn,
                        0,
                        window,
                        XCB_EVENT_MASK_NO_EVENT,
                        (char *) & ev
                    );

                    free(protocols_reply);
                    free(delete_reply);
                }

                static void  
                raise(const xcb_window_t & window) 
                {
                    xcb_configure_window
                    (
                        conn,
                        window,
                        XCB_CONFIG_WINDOW_STACK_MODE, 
                        (const uint32_t[1])
                        {
                            XCB_STACK_MODE_ABOVE
                        }
                    );
                    xcb_flush(conn);
                }
            ;
        };

        class pointer
        {
            public:
                class set
                {
                    public:
                        set(xcb_window_t window, CURSOR cursor_type) 
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
                                window, 
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

                    private:
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
                };

                class get
                {
                    public:
                        static uint32_t
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
                        
                        static uint32_t
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
                    ;
                };

                static void 
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

                static void
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
        };

        class draw
        {
            public:
                static void 
                text(const xcb_window_t & window, const char * str , const COLOR & text_color, const COLOR & backround_color,const char * font_name, const int16_t & x, const int16_t & y)
                {
                    /*
                     *  font names
                     *
                     *  '7x14'
                     *
                     */
                    xcb_image_text_8
                    (
                        conn, 
                        strlen(str), 
                        window, 
                        mxb::create::gc::font(window, text_color, backround_color, mxb::get::font(font_name)),
                        x, 
                        y, 
                        str
                    );

                    xcb_flush(conn);
                }
            ;
        };

        class Delete
        {
            public:
                static void
                client_vec(std::vector<client *> & vec)
                {
                    for (client * c : vec)
                    {
                        if (c)
                        {
                            mxb::Client::send_sigterm(c);
                            xcb_flush(conn);
                        }
                        mxb::Client::remove(c, vec);
                    }

                    vec.clear();

                    std::vector<client *>().swap(vec);
                }

                static void
                desktop_vec(std::vector<desktop *> & vec)
                {
                    for (desktop * d : vec)
                    {
                        mxb::Delete::client_vec(d->current_clients);
                        delete d;
                    }

                    vec.clear();

                    std::vector<desktop *>().swap(vec);
                }

                template <typename Type>
                static void 
                ptr_vector(std::vector<Type *>& vec) 
                {
                    for (Type * ptr : vec) 
                    {
                        delete ptr;
                    }
                    vec.clear();

                    std::vector<Type *>().swap(vec);
                }
            ;
        };

        static void
        quit(const int & status)
        {
            xcb_flush(conn);
            mxb::Delete::client_vec(client_list);
            mxb::Delete::desktop_vec(desktop_list);
            xcb_ewmh_connection_wipe(ewmh);
            xcb_disconnect(conn);
            exit(status);
        }
    ;
};

static mxb::Dialog_win::Dock * dock;

namespace bitmap
{
    const bool CHAR_BITMAP_A[20][20] = 
    {    
        {0,0,0,1,0,0,0},
        {0,0,1,0,1,0,0},
        {0,0,1,0,1,0,0},
        {0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0},
        {1,0,0,0,0,0,1},
        {1,1,1,1,1,1,1},
        {1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1},
    };

    const bool CHAR_BITMAP_B[20][20] = 
    {
        {1, 1, 1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 1, 1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 1, 1, 1, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0},
    };

    const bool CHAR_BITMAP_C[20][20] = 
    {
        {0, 1, 1, 1, 1, 1, 0},
        {1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 1},
        {0, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 0, 0, 0, 0},
    };

    const bool CHAR_BITMAP_CLOSE_BUTTON[20][20] = 
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

    const bool CHAR_BITMAP_MIN_BUTTON[20][20] = 
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };

    namespace letter
    {
        const bool h[11][7] =
        {
            {1,0,0,0,0,0,0},
            {1,0,0,0,0,0,0},
            {1,0,0,0,0,0,0},
            {1,0,0,0,0,0,0},
            {1,1,1,1,1,1,0},
            {1,0,0,0,0,1,0},
            {1,0,0,0,0,1,0},
            {1,0,0,0,0,1,0},
            {1,0,0,0,0,1,0},
            {1,0,0,0,0,1,0},
            {1,0,0,0,0,1,0}
        };
    }

    std::vector<std::vector<bool>> 
    create_bool_bitmap(int width, int height) 
    {
        std::vector<std::vector<bool>> bitmap(height, std::vector<bool>(width, false));
        return bitmap;
    }

    void 
    modify_bitmap(std::vector<std::vector<bool>>& bitmap, int row, int startCol, int endCol, bool value) 
    {
        // Check if the row and column indices are within the bounds of the bitmap
        if (row < 0 || row >= bitmap.size()) 
        {
            log_error("Invalid row");
            return;
        }

        if (startCol < 0 || endCol > bitmap[0].size())
        {
            log_error("Invalid column");
            return;
        }

        for (int i = startCol; i < endCol; ++i) 
        {
            bitmap[row][i] = value;
        }
    }
}

namespace get
{
    client * 
    client_from_win(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->win) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_all_win(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->win 
             || * w == c->frame 
             || * w == c->titlebar 
             || * w == c->close_button 
             || * w == c->max_button 
             || * w == c->min_button 
             || * w == c->border.left 
             || * w == c->border.right 
             || * w == c->border.top 
             || * w == c->border.bottom
             || * w == c->border.top_left
             || * w == c->border.top_right
             || * w == c->border.bottom_left
             || * w == c->border.bottom_right) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_frame(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->frame) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_close_button(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->close_button) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

    client * 
    client_from_titlebar(const xcb_window_t * w) 
    {
        for (const auto & c : client_list) 
        {
            if (* w == c->titlebar) 
            {
                return c;
            }
        }
        return nullptr; /*
         *
         * THIS WILL
         * RETURN 'nullptr' BECAUSE THE
         * WINDOW DOES NOT BELONG TO ANY 
         * CLIENT IN THE CLIENT LIST
         *  
         */ 
    }

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
            return XCB_ATOM_NONE;
        } 

        xcb_atom_t atom = reply->atom;
        free(reply);
        return atom;
    }

    std::string
    name(client * & c)
    {
        log.log(FUNC, __func__);
        xcb_get_property_reply_t * reply;
        unsigned int reply_len;
        char * name;

        reply = xcb_get_property_reply
        (
            conn, 
            xcb_get_property
            (
                conn, 
                false,
                c->win, 
                XCB_ATOM_WM_NAME,
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
                log.log(ERROR, __func__, "reply length is = 0");
                free(reply);
                return "";
            }

            log.log(ERROR, __func__, "reply == nullptr");
            return "";
        }

        reply_len = xcb_get_property_value_length(reply);
        name = static_cast<char *>(malloc(sizeof(char) * (reply_len+1)));
        memcpy(name, xcb_get_property_value(reply), reply_len);
        name[reply_len] = '\0';

        if (reply)
        {
            free(reply);
        }

        log.log(INFO, __func__, "name = " + std::string(name)); 
        std::string sname = std::string(name);
        free(name);

        return sname;
    }

    std::string
    propertyAtom_enum_to_string(xcb_atom_enum_t property)
    {
        switch (property) 
        {
            case XCB_ATOM_ANY:                  return "XCB_ATOM_ANY";
            case XCB_ATOM_PRIMARY:              return "XCB_ATOM_PRIMARY";
            case XCB_ATOM_SECONDARY:            return "XCB_ATOM_SECONDARY";
            case XCB_ATOM_ARC:                  return "XCB_ATOM_ARC";
            case XCB_ATOM_ATOM:                 return "XCB_ATOM_ATOM";
            case XCB_ATOM_BITMAP:               return "XCB_ATOM_BITMAP";
            case XCB_ATOM_CARDINAL:             return "XCB_ATOM_CARDINAL";
            case XCB_ATOM_COLORMAP:             return "XCB_ATOM_COLORMAP";
            case XCB_ATOM_CURSOR:               return "XCB_ATOM_CURSOR";
            case XCB_ATOM_CUT_BUFFER0:          return "XCB_ATOM_CUT_BUFFER0";
            case XCB_ATOM_CUT_BUFFER1:          return "XCB_ATOM_CUT_BUFFER1";
            case XCB_ATOM_CUT_BUFFER2:          return "XCB_ATOM_CUT_BUFFER2";
            case XCB_ATOM_CUT_BUFFER3:          return "XCB_ATOM_CUT_BUFFER3";
            case XCB_ATOM_CUT_BUFFER4:          return "XCB_ATOM_CUT_BUFFER4";
            case XCB_ATOM_CUT_BUFFER5:          return "XCB_ATOM_CUT_BUFFER5";
            case XCB_ATOM_CUT_BUFFER6:          return "XCB_ATOM_CUT_BUFFER6";
            case XCB_ATOM_CUT_BUFFER7:          return "XCB_ATOM_CUT_BUFFER7";
            case XCB_ATOM_DRAWABLE:             return "XCB_ATOM_DRAWABLE";
            case XCB_ATOM_FONT:                 return "XCB_ATOM_FONT";
            case XCB_ATOM_INTEGER:              return "XCB_ATOM_INTEGER";
            case XCB_ATOM_PIXMAP:               return "XCB_ATOM_PIXMAP";
            case XCB_ATOM_POINT:                return "XCB_ATOM_POINT";
            case XCB_ATOM_RECTANGLE:            return "XCB_ATOM_RECTANGLE";
            case XCB_ATOM_RESOURCE_MANAGER:     return "XCB_ATOM_RESOURCE_MANAGER";
            case XCB_ATOM_RGB_COLOR_MAP:        return "XCB_ATOM_RGB_COLOR_MAP";
            case XCB_ATOM_RGB_BEST_MAP:         return "XCB_ATOM_RGB_BEST_MAP";
            case XCB_ATOM_RGB_BLUE_MAP:         return "XCB_ATOM_RGB_BLUE_MAP";
            case XCB_ATOM_RGB_DEFAULT_MAP:      return "XCB_ATOM_RGB_DEFAULT_MAP";
            case XCB_ATOM_RGB_GRAY_MAP:         return "XCB_ATOM_RGB_GRAY_MAP";
            case XCB_ATOM_RGB_GREEN_MAP:        return "XCB_ATOM_RGB_GREEN_MAP";
            case XCB_ATOM_RGB_RED_MAP:          return "XCB_ATOM_RGB_RED_MAP";
            case XCB_ATOM_STRING:               return "XCB_ATOM_STRING";
            case XCB_ATOM_VISUALID:             return "XCB_ATOM_VISUALID";
            case XCB_ATOM_WINDOW:               return "XCB_ATOM_WINDOW";
            case XCB_ATOM_WM_COMMAND:           return "XCB_ATOM_WM_COMMAND";
            case XCB_ATOM_WM_HINTS:             return "XCB_ATOM_WM_HINTS";
            case XCB_ATOM_WM_CLIENT_MACHINE:    return "XCB_ATOM_WM_CLIENT_MACHINE";
            case XCB_ATOM_WM_ICON_NAME:         return "XCB_ATOM_WM_ICON_NAME";
            case XCB_ATOM_WM_ICON_SIZE:         return "XCB_ATOM_WM_ICON_SIZE";
            case XCB_ATOM_WM_NAME:              return "XCB_ATOM_WM_NAME";
            case XCB_ATOM_WM_NORMAL_HINTS:      return "XCB_ATOM_WM_NORMAL_HINTS";
            case XCB_ATOM_WM_SIZE_HINTS:        return "XCB_ATOM_WM_SIZE_HINTS";
            case XCB_ATOM_WM_ZOOM_HINTS:        return "XCB_ATOM_WM_ZOOM_HINTS";
            case XCB_ATOM_MIN_SPACE:            return "XCB_ATOM_MIN_SPACE";
            case XCB_ATOM_NORM_SPACE:           return "XCB_ATOM_NORM_SPACE";
            case XCB_ATOM_MAX_SPACE:            return "XCB_ATOM_MAX_SPACE";
            case XCB_ATOM_END_SPACE:            return "XCB_ATOM_END_SPACE";
            case XCB_ATOM_SUPERSCRIPT_X:        return "XCB_ATOM_SUPERSCRIPT_X";
            case XCB_ATOM_SUPERSCRIPT_Y:        return "XCB_ATOM_SUPERSCRIPT_Y";
            case XCB_ATOM_SUBSCRIPT_X:          return "XCB_ATOM_SUBSCRIPT_X";
            case XCB_ATOM_SUBSCRIPT_Y:          return "XCB_ATOM_SUBSCRIPT_Y";
            case XCB_ATOM_UNDERLINE_POSITION:   return "XCB_ATOM_UNDERLINE_POSITION";
            case XCB_ATOM_UNDERLINE_THICKNESS:  return "XCB_ATOM_UNDERLINE_THICKNESS";
            case XCB_ATOM_STRIKEOUT_ASCENT:     return "XCB_ATOM_STRIKEOUT_ASCENT";
            case XCB_ATOM_STRIKEOUT_DESCENT:    return "XCB_ATOM_STRIKEOUT_DESCENT";
            case XCB_ATOM_ITALIC_ANGLE:         return "XCB_ATOM_ITALIC_ANGLE";
            case XCB_ATOM_X_HEIGHT:             return "XCB_ATOM_X_HEIGHT";
            case XCB_ATOM_QUAD_WIDTH:           return "XCB_ATOM_QUAD_WIDTH";
            case XCB_ATOM_WEIGHT:               return "XCB_ATOM_WEIGHT";
            case XCB_ATOM_POINT_SIZE:           return "XCB_ATOM_POINT_SIZE";
            case XCB_ATOM_RESOLUTION:           return "XCB_ATOM_RESOLUTION";
            case XCB_ATOM_COPYRIGHT:            return "XCB_ATOM_COPYRIGHT";
            case XCB_ATOM_NOTICE:               return "XCB_ATOM_NOTICE";
            case XCB_ATOM_FONT_NAME:            return "XCB_ATOM_FONT_NAME";
            case XCB_ATOM_FAMILY_NAME:          return "XCB_ATOM_FAMILY_NAME";
            case XCB_ATOM_FULL_NAME:            return "XCB_ATOM_FULL_NAME";
            case XCB_ATOM_CAP_HEIGHT:           return "XCB_ATOM_CAP_HEIGHT";
            case XCB_ATOM_WM_CLASS:             return "XCB_ATOM_WM_CLASS";
            case XCB_ATOM_WM_TRANSIENT_FOR:     return "XCB_ATOM_WM_TRANSIENT_FOR";
        }
    }

    std::string 
    getNetAtomAsString(xcb_atom_t atom) 
    {
        static const std::unordered_map<xcb_atom_t, std::string> atomMap = {
            {XCB_ATOM_NONE, "XCB_ATOM_NONE"},
            {ewmh->_NET_SUPPORTED, "_NET_SUPPORTED"},
            {ewmh->_NET_CLIENT_LIST, "_NET_CLIENT_LIST"},
            {ewmh->_NET_CLIENT_LIST_STACKING, "_NET_CLIENT_LIST_STACKING"},
            {ewmh->_NET_NUMBER_OF_DESKTOPS, "_NET_NUMBER_OF_DESKTOPS"},
            {ewmh->_NET_DESKTOP_GEOMETRY, "_NET_DESKTOP_GEOMETRY"},
            {ewmh->_NET_DESKTOP_VIEWPORT, "_NET_DESKTOP_VIEWPORT"},
            {ewmh->_NET_CURRENT_DESKTOP, "_NET_CURRENT_DESKTOP"},
            {ewmh->_NET_DESKTOP_NAMES, "_NET_DESKTOP_NAMES"},
            {ewmh->_NET_ACTIVE_WINDOW, "_NET_ACTIVE_WINDOW"},
            {ewmh->_NET_WORKAREA, "_NET_WORKAREA"},
            {ewmh->_NET_SUPPORTING_WM_CHECK, "_NET_SUPPORTING_WM_CHECK"},
            {ewmh->_NET_VIRTUAL_ROOTS, "_NET_VIRTUAL_ROOTS"},
            {ewmh->_NET_DESKTOP_LAYOUT, "_NET_DESKTOP_LAYOUT"},
            {ewmh->_NET_SHOWING_DESKTOP, "_NET_SHOWING_DESKTOP"},
            {ewmh->_NET_CLOSE_WINDOW, "_NET_CLOSE_WINDOW"},
            {ewmh->_NET_MOVERESIZE_WINDOW, "_NET_MOVERESIZE_WINDOW"},
            {ewmh->_NET_WM_MOVERESIZE, "_NET_WM_MOVERESIZE"},
            {ewmh->_NET_RESTACK_WINDOW, "_NET_RESTACK_WINDOW"},
            {ewmh->_NET_REQUEST_FRAME_EXTENTS, "_NET_REQUEST_FRAME_EXTENTS"},
            {ewmh->_NET_WM_NAME, "_NET_WM_NAME"},
            {ewmh->_NET_WM_VISIBLE_NAME, "_NET_WM_VISIBLE_NAME"},
            {ewmh->_NET_WM_ICON_NAME, "_NET_WM_ICON_NAME"},
            {ewmh->_NET_WM_VISIBLE_ICON_NAME, "_NET_WM_VISIBLE_ICON_NAME"},
            {ewmh->_NET_WM_DESKTOP, "_NET_WM_DESKTOP"},
            {ewmh->_NET_WM_WINDOW_TYPE, "_NET_WM_WINDOW_TYPE"},
            {ewmh->_NET_WM_STATE, "_NET_WM_STATE"},
            {ewmh->_NET_WM_ALLOWED_ACTIONS, "_NET_WM_ALLOWED_ACTIONS"},
            {ewmh->_NET_WM_STRUT, "_NET_WM_STRUT"},
            {ewmh->_NET_WM_STRUT_PARTIAL, "_NET_WM_STRUT_PARTIAL"},
            {ewmh->_NET_WM_ICON_GEOMETRY, "_NET_WM_ICON_GEOMETRY"},
            {ewmh->_NET_WM_ICON, "_NET_WM_ICON"},
            {ewmh->_NET_WM_PID, "_NET_WM_PID"},
            {ewmh->_NET_WM_HANDLED_ICONS, "_NET_WM_HANDLED_ICONS"},
            {ewmh->_NET_WM_USER_TIME, "_NET_WM_USER_TIME"},
            {ewmh->_NET_WM_USER_TIME_WINDOW, "_NET_WM_USER_TIME_WINDOW"},
            {ewmh->_NET_FRAME_EXTENTS, "_NET_FRAME_EXTENTS"},
            {ewmh->_NET_WM_PING, "_NET_WM_PING"},
            {ewmh->_NET_WM_SYNC_REQUEST, "_NET_WM_SYNC_REQUEST"},
            {ewmh->_NET_WM_SYNC_REQUEST_COUNTER, "_NET_WM_SYNC_REQUEST_COUNTER"},
            {ewmh->_NET_WM_FULLSCREEN_MONITORS, "_NET_WM_FULLSCREEN_MONITORS"},
            {ewmh->_NET_WM_FULL_PLACEMENT, "_NET_WM_FULL_PLACEMENT"},
            {ewmh->UTF8_STRING, "UTF8_STRING"},
            {ewmh->WM_PROTOCOLS, "WM_PROTOCOLS"},
            {ewmh->MANAGER, "MANAGER"},
            {ewmh->_NET_WM_WINDOW_TYPE_DESKTOP, "_NET_WM_WINDOW_TYPE_DESKTOP"},
            {ewmh->_NET_WM_WINDOW_TYPE_DOCK, "_NET_WM_WINDOW_TYPE_DOCK"},
            {ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR, "_NET_WM_WINDOW_TYPE_TOOLBAR"},
            {ewmh->_NET_WM_WINDOW_TYPE_MENU, "_NET_WM_WINDOW_TYPE_MENU"},
            {ewmh->_NET_WM_WINDOW_TYPE_UTILITY, "_NET_WM_WINDOW_TYPE_UTILITY"},
            {ewmh->_NET_WM_WINDOW_TYPE_SPLASH, "_NET_WM_WINDOW_TYPE_SPLASH"},
            {ewmh->_NET_WM_WINDOW_TYPE_DIALOG, "_NET_WM_WINDOW_TYPE_DIALOG"},
            {ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU"},
            {ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU, "_NET_WM_WINDOW_TYPE_POPUP_MENU"},
            {ewmh->_NET_WM_WINDOW_TYPE_TOOLTIP, "_NET_WM_WINDOW_TYPE_TOOLTIP"},
            {ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION, "_NET_WM_WINDOW_TYPE_NOTIFICATION"},
            {ewmh->_NET_WM_WINDOW_TYPE_COMBO, "_NET_WM_WINDOW_TYPE_COMBO"},
            {ewmh->_NET_WM_WINDOW_TYPE_DND, "_NET_WM_WINDOW_TYPE_DND"},
            {ewmh->_NET_WM_WINDOW_TYPE_NORMAL, "_NET_WM_WINDOW_TYPE_NORMAL"},
            {ewmh->_NET_WM_STATE_MODAL, "_NET_WM_STATE_MODAL"},
            {ewmh->_NET_WM_STATE_STICKY, "_NET_WM_STATE_STICKY"},
            {ewmh->_NET_WM_STATE_MAXIMIZED_VERT, "_NET_WM_STATE_MAXIMIZED_VERT"},
            {ewmh->_NET_WM_STATE_MAXIMIZED_HORZ, "_NET_WM_STATE_MAXIMIZED_HORZ"},
            {ewmh->_NET_WM_STATE_SHADED, "_NET_WM_STATE_SHADED"},
            {ewmh->_NET_WM_STATE_SKIP_TASKBAR, "_NET_WM_STATE_SKIP_TASKBAR"},
            {ewmh->_NET_WM_STATE_SKIP_PAGER, "_NET_WM_STATE_SKIP_PAGER"},
            {ewmh->_NET_WM_STATE_HIDDEN, "_NET_WM_STATE_HIDDEN"},
            {ewmh->_NET_WM_STATE_FULLSCREEN, "_NET_WM_STATE_FULLSCREEN"},
            {ewmh->_NET_WM_STATE_ABOVE, "_NET_WM_STATE_ABOVE"},
            {ewmh->_NET_WM_STATE_BELOW, "_NET_WM_STATE_BELOW"},
            {ewmh->_NET_WM_STATE_DEMANDS_ATTENTION, "_NET_WM_STATE_DEMANDS_ATTENTION"},
            {ewmh->_NET_WM_ACTION_MOVE, "_NET_WM_ACTION_MOVE"},
            {ewmh->_NET_WM_ACTION_RESIZE, "_NET_WM_ACTION_RESIZE"},
            {ewmh->_NET_WM_ACTION_MINIMIZE, "_NET_WM_ACTION_MINIMIZE"},
            {ewmh->_NET_WM_ACTION_SHADE, "_NET_WM_ACTION_SHADE"},
            {ewmh->_NET_WM_ACTION_STICK, "_NET_WM_ACTION_STICK"},
            {ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ, "_NET_WM_ACTION_MAXIMIZE_HORZ"},
            {ewmh->_NET_WM_ACTION_MAXIMIZE_VERT, "_NET_WM_ACTION_MAXIMIZE_VERT"},
            {ewmh->_NET_WM_ACTION_FULLSCREEN, "_NET_WM_ACTION_FULLSCREEN"},
            {ewmh->_NET_WM_ACTION_CHANGE_DESKTOP, "_NET_WM_ACTION_CHANGE_DESKTOP"},
            {ewmh->_NET_WM_ACTION_CLOSE, "_NET_WM_ACTION_CLOSE"},
            {ewmh->_NET_WM_ACTION_ABOVE, "_NET_WM_ACTION_ABOVE"},
            {ewmh->_NET_WM_ACTION_BELOW, "_NET_WM_ACTION_BELOW"}
        };

        auto it = atomMap.find(atom);
        return (it != atomMap.end()) ? it->second : "Unknown Atom";
    }

    std::string 
    WindowProperty(client * c, const char * atom_name) 
    {
        xcb_get_property_reply_t *reply;
        unsigned int reply_len;
        char *propertyValue;

        reply = xcb_get_property_reply
        (
            conn,
            xcb_get_property
            (
                conn,
                false,
                c->win,
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
                // log.log(ERROR, __func__, "reply length for property(" + std::string(atom_name) + ") = 0");
                free(reply);
                return "";
            }

            // log.log(ERROR, __func__, "reply == nullptr");
            return "";
        }

        reply_len = xcb_get_property_value_length(reply);
        propertyValue = static_cast<char *>(malloc(sizeof(char) * (reply_len + 1)));
        memcpy(propertyValue, xcb_get_property_value(reply), reply_len);
        propertyValue[reply_len] = '\0';

        if (reply) 
        {
            free(reply);
        }

        log.log(INFO, __func__, "property value(" + std::string(atom_name) + ") = " + std::string(propertyValue));
        std::string spropertyValue = std::string(propertyValue);
        free(propertyValue);

        return spropertyValue;
    }
}

class focus 
{
    public:
        static void
        client(client * c)
        {
            // LOG_func
            if (c == nullptr)
            {
                LOG_warning("client was nullptr");
                return;
            }
            raise_client(c);
            focus_input(c);
            focused_client = c;
        }

        static void
        cycle()
        {
            bool focus = false;
            for (auto & c : client_list)
            {
                if (c)
                {
                    if (c == focused_client)
                    {
                        focus = true;
                        continue;
                    }
                    
                    if (focus)
                    {
                        focus::client(c);
                        return;  
                    }
                }
            }
        }
    ;

    private:
        static void  
        raise_client(struct client * c) 
        {
            xcb_configure_window
            (
                conn,
                c->frame,
                XCB_CONFIG_WINDOW_STACK_MODE, 
                (const uint32_t[1])
                {
                    XCB_STACK_MODE_ABOVE
                }
            );
            xcb_flush(conn);
        }
        
        static void 
        focus_input(struct client * c)
        {
            if (!c)
            {
                LOG_warning("client was nullptr");
                return;
            }
            xcb_set_input_focus
            (
                conn, 
                XCB_INPUT_FOCUS_POINTER_ROOT, 
                c->win, 
                XCB_CURRENT_TIME
            );
            xcb_flush(conn);
        }
    ;
};

class mv_client 
{
    public:
        mv_client(client * & c, const uint16_t & start_x, const uint16_t & start_y) 
        : c(c), start_x(start_x), start_y(start_y)
        {
            if (mxb::EWMH::check::is_window_fullscreen(c->win))
            {
                return;
            }

            // log.log(FUNC, __func__);
            grab_pointer();

            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }
    ;

    private:
        client * & c;
        const uint16_t & start_x;
        const uint16_t & start_y;
        bool shouldContinue = true;
        xcb_generic_event_t * ev;

        void 
        grab_pointer()
        {
            xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer
            (
                conn,
                false, // owner_events: false to not propagate events to other clients
                c->frame,
                XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC,
                XCB_GRAB_MODE_ASYNC,
                XCB_NONE,
                XCB_NONE, 
                XCB_CURRENT_TIME
            );

            xcb_grab_pointer_reply_t * reply = xcb_grab_pointer_reply(conn, cookie, NULL);
            if (!reply || reply->status != XCB_GRAB_STATUS_SUCCESS) 
            {
                log.log(ERROR, __func__, "Could not grab pointer");
                free(reply);
                return;
            }
            free(reply); 
        }

        /* DEFENITIONS TO REDUCE REDUNDENT CODE IN 'snap' FUNCTION */
        #define RIGHT_  screen->width_in_pixels  - c->width
        #define BOTTOM_ screen->height_in_pixels - c->height

        void 
        snap(const int16_t & x, const int16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : cur_d->current_clients)
            {
                // SNAP WINDOW TO 'RIGHT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x + cli->width - N && x < cli->x + cli->width + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    // SNAP WINDOW TO 'RIGHT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC)
                    {  
                        mxb::conf::win::x_y(c->frame, (cli->x + cli->width), cli->y);
                        return;
                    }

                    // SNAP WINDOW TO 'RIGHT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC)
                    {
                        mxb::conf::win::x_y(c->frame, (cli->x + cli->width), (cli->y + cli->height) - c->height);
                        return;
                    }

                    mxb::conf::win::x_y(c->frame, (cli->x + cli->width), y);
                    return;
                }

                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x + c->width > cli->x - N && x + c->width < cli->x + N) 
                 && (y + c->height > cli->y && y < cli->y + cli->height))
                {
                    // SNAP WINDOW TO 'LEFT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y > cli->y - NC && y < cli->y + NC)
                    {  
                        mxb::conf::win::x_y(c->frame, (cli->x - c->width), cli->y);
                        return;
                    }

                    // SNAP WINDOW TO 'LEFT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (y + c->height > cli->y + cli->height - NC && y + c->height < cli->y + cli->height + NC)
                    {
                        mxb::conf::win::x_y(c->frame, (cli->x - c->width), (cli->y + cli->height) - c->height);
                        return;
                    }                

                    mxb::conf::win::x_y(c->frame, (cli->x - c->width), y);
                    return;
                }

                // SNAP WINDOW TO 'BOTTOM' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y + cli->height - N && y < cli->y + cli->height + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    // SNAP WINDOW TO 'BOTTOM_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC)
                    {  
                        mxb::conf::win::x_y(c->frame, cli->x, (cli->y + cli->height));
                        return;
                    }

                    // SNAP WINDOW TO 'BOTTOM_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC)
                    {
                        mxb::conf::win::x_y(c->frame, ((cli->x + cli->width) - c->width), (cli->y + cli->height));
                        return;
                    }

                    mxb::conf::win::x_y(c->frame, x, (cli->y + cli->height));
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y + c->height > cli->y - N && y + c->height < cli->y + N) 
                 && (x + c->width > cli->x && x < cli->x + cli->width))
                {
                    // SNAP WINDOW TO 'TOP_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x > cli->x - NC && x < cli->x + NC)
                    {  
                        mxb::conf::win::x_y(c->frame, cli->x, (cli->y - c->height));
                        return;
                    }

                    // SNAP WINDOW TO 'TOP_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET
                    if (x + c->width > cli->x + cli->width - NC && x + c->width < cli->x + cli->width + NC)
                    {
                        mxb::conf::win::x_y(c->frame, ((cli->x + cli->width) - c->width), (cli->y - c->height));
                        return;
                    }

                    mxb::conf::win::x_y(c->frame, x, (cli->y - c->height));
                    return;
                }
            }

            // WINDOW TO EDGE OF SCREEN SNAPPING
            if (((x < N) && (x > -N)) && ((y < N) && (y > -N)))
            {
                mxb::conf::win::x_y(c->frame, 0, 0);
            }
            else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < N && y > -N))
            {
                mxb::conf::win::x_y(c->frame, RIGHT_, 0);
            }
            else if ((y < BOTTOM_ + N && y > BOTTOM_ - N) && (x < N && x > -N))
            {
                mxb::conf::win::x_y(c->frame, 0, BOTTOM_);
            }
            else if ((x < N) && (x > -N))
            { 
                mxb::conf::win::x_y(c->frame, 0, y);
            }
            else if (y < N && y > -N)
            {
                mxb::conf::win::x_y(c->frame, x, 0);
            }
            else if ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < BOTTOM_ + N && y > BOTTOM_ - N))
            {
                mxb::conf::win::x_y(c->frame, RIGHT_, BOTTOM_);
            }
            else if ((x < RIGHT_ + N) && (x > RIGHT_ - N))
            { 
                mxb::conf::win::x_y(c->frame, RIGHT_, y);
            }
            else if (y < BOTTOM_ + N && y > BOTTOM_ - N)
            {
                mxb::conf::win::x_y(c->frame, x, BOTTOM_);
            }
            else 
            {
                mxb::conf::win::x_y(c->frame, x, y);
            }
        }

        void
        run() /*
            THIS IS THE MAIN EVENT LOOP FOR 'mv_client'
         */ 
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
                        mxb::Client::update(c);
                        break;
                    }
                }
                free(ev);
            }
        }
        
        const double frameRate = 120.0; /* 
            FRAMERATE 
         */
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now(); /*
            HIGH_PRECISION_CLOCK AND TIME_POINT 
         */
        const double frameDuration = 1000.0 / frameRate; /* 
            DURATION IN MILLISECONDS THAT EACH FRAME SHOULD LAST 
        */

        bool 
        isTimeToRender() 
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

/**
 *
 * @class XCPPBAnimator
 * @brief Class for animating the position and size of an XCB window.
 *
 */
class XCPPBAnimator
{
    public:
        XCPPBAnimator(xcb_connection_t* connection, const xcb_window_t & window)
        : connection(connection), window(window) {}

        XCPPBAnimator(xcb_connection_t* connection, client * c)
        : connection(connection), c(c) {}

        XCPPBAnimator(xcb_connection_t * connection)
        : connection(connection) {}

        void /**
         * @brief Animates the position and size of an object from a starting point to an ending point.
         * 
         * @param startX The starting X coordinate.
         * @param startY The starting Y coordinate.
         * @param startWidth The starting width.
         * @param startHeight The starting height.
         * @param endX The ending X coordinate.
         * @param endY The ending Y coordinate.
         * @param endWidth The ending width.
         * @param endHeight The ending height.
         * @param duration The duration of the animation in milliseconds.
         */
        animate(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration) 
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX      = startX;
            currentY      = startY;
            currentWidth  = startWidth;
            currentHeight = startHeight;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX      = std::abs(endX - startX)           / (endX - startX);
            stepY      = std::abs(endY - startY)           / (endY - startY);
            stepWidth  = std::abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = std::abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - startHeight)); 

            /* START ANIMATION THREADS */
            XAnimationThread = std::thread(&XCPPBAnimator::XAnimation, this, endX);
            YAnimationThread = std::thread(&XCPPBAnimator::YAnimation, this, endY);
            WAnimationThread = std::thread(&XCPPBAnimator::WAnimation, this, endWidth);
            HAnimationThread = std::thread(&XCPPBAnimator::HAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }

        void 
        animate_client(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration)
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX      = startX;
            currentY      = startY;
            currentWidth  = startWidth;
            currentHeight = startHeight;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX      = std::abs(endX - startX)           / (endX - startX);
            stepY      = std::abs(endY - startY)           / (endY - startY);
            stepWidth  = std::abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = std::abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - startHeight)); 
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = std::thread(&XCPPBAnimator::GFrameAnimation, this, endX, endY, endWidth, endHeight);
            XAnimationThread = std::thread(&XCPPBAnimator::CliXAnimation, this, endX);
            YAnimationThread = std::thread(&XCPPBAnimator::CliYAnimation, this, endY);
            WAnimationThread = std::thread(&XCPPBAnimator::CliWAnimation, this, endWidth);
            HAnimationThread = std::thread(&XCPPBAnimator::CliHAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }

        enum DIRECTION 
        {
            NEXT,
            PREV
        };

        void
        animate_client_x(int startX, int endX, int duration)
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX = startX;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX = std::abs(endX - startX) / (endX - startX);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = std::thread(&XCPPBAnimator::GFrameAnimation_X, this, endX);
            XAnimationThread = std::thread(&XCPPBAnimator::CliXAnimation, this, endX);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
        
        /**
         * @brief Destructor to ensure the animation threads are stopped when the object is destroyed.
         */
        ~XCPPBAnimator()
        {
            stopAnimations();
        }
    ;

    private:
        xcb_connection_t* connection;
        xcb_window_t window;
        client * c;
        std::thread GAnimationThread;
        std::thread XAnimationThread;
        std::thread YAnimationThread;
        std::thread WAnimationThread;
        std::thread HAnimationThread;
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
        std::atomic<bool> stopGFlag{false};
        std::atomic<bool> stopXFlag{false};
        std::atomic<bool> stopYFlag{false};
        std::atomic<bool> stopWFlag{false};
        std::atomic<bool> stopHFlag{false};
        std::chrono::high_resolution_clock::time_point XlastUpdateTime;
        std::chrono::high_resolution_clock::time_point YlastUpdateTime;
        std::chrono::high_resolution_clock::time_point WlastUpdateTime;
        std::chrono::high_resolution_clock::time_point HlastUpdateTime;
        
        /* FRAMERATE */
        const double frameRate = 120;
        
        /* DURATION IN MILLISECONDS THAT EACH FRAME SHOULD LAST */
        const double frameDuration = 1000.0 / frameRate; 
        
        void /**
         *
         * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
         * 
         * This function updates the 'x' of a window in a loop until the current 'x'
         * matches the specified end 'x'. It uses the "XStep()" function to incrementally
         * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endX The desired 'x' position of the window.
         *
         */
        XAnimation(const int & endX) 
        {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentX == endX) 
                {
                    config_window(XCB_CONFIG_WINDOW_X, endX);
                    break;
                }
                XStep();
                thread_sleep(XAnimDuration);
            }
        }

        void /**
         * @brief Performs a step in the X direction.
         * 
         * This function increments the currentX variable by the stepX value.
         * If it is time to render, it configures the window's X position using the currentX value.
         * 
         * @note This function assumes that the connection and window variables are properly initialized.
         */
        XStep() 
        {
            currentX += stepX;
            
            if (XisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_X,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentX)
                    }
                );
                xcb_flush(connection);
            }
        }

        void /**
         *
         * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
         * 
         * This function updates the 'y' of a window in a loop until the current 'y'
         * matches the specified end 'y'. It uses the "YStep()" function to incrementally
         * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endY The desired 'y' positon of the window.
         *
         */
        YAnimation(const int & endY) 
        {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentY == endY) 
                {
                    config_window(XCB_CONFIG_WINDOW_Y, endY);
                    break;
                }
                YStep();
                thread_sleep(YAnimDuration);
            }
        }

        void /**
         * @brief Performs a step in the Y direction.
         * 
         * This function increments the currentY variable by the stepY value.
         * If it is time to render, it configures the window's Y position using xcb_configure_window
         * and flushes the connection using xcb_flush.
         */
        YStep() 
        {
            currentY += stepY;
            
            if (YisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_Y,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentY)
                    }
                );
                xcb_flush(connection);
            }
        }

        void /**
         *
         * @brief Performs a 'width' animation until the specified end 'width' is reached.
         * 
         * This function updates the 'width' of a window in a loop until the current 'width'
         * matches the specified end 'width'. It uses the "WStep()" function to incrementally
         * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'width' of the window.
         *
         */
        WAnimation(const int & endWidth) 
        {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentWidth == endWidth) 
                {
                    config_window(XCB_CONFIG_WINDOW_WIDTH, endWidth);
                    break;
                }
                WStep();
                thread_sleep(WAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs a step in the width calculation and updates the window width if it is time to render.
         * 
         * This function increments the current width by the step width. If it is time to render, it configures the window width
         * using the XCB library and flushes the connection.
         *
         */
        WStep() 
        {
            currentWidth += stepWidth;

            if (WisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_WIDTH,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentWidth) 
                    }
                );
                xcb_flush(connection);
            }
        }

        void /**
         *
         * @brief Performs a 'height' animation until the specified end 'height' is reached.
         * 
         * This function updates the 'height' of a window in a loop until the current 'height'
         * matches the specified end 'height'. It uses the "HStep()" function to incrementally
         * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'height' of the window.
         *
         */
        HAnimation(const int & endHeight) 
        {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentHeight == endHeight) 
                {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, endHeight);
                    break;
                }
                HStep();
                thread_sleep(HAnimDuration);
            }
        }

        void /**
         *
         * @brief Increases the current height by the step height and updates the window height if it's time to render.
         * 
         * This function is responsible for incrementing the current height by the step height and updating the window height
         * if it's time to render. It uses the xcb_configure_window function to configure the window height and xcb_flush to
         * flush the changes to the X server.
         *
         */
        HStep() 
        {
            currentHeight += stepHeight;
            
            if (HisTimeToRender())
            {
                xcb_configure_window
                (
                    connection,
                    window,
                    XCB_CONFIG_WINDOW_HEIGHT,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentHeight)
                    }
                );
                xcb_flush(connection);
            }
        }

        void
        GFrameAnimation(const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            while (true)
            {
                if (currentX == endX && currentY == endY && currentWidth == endWidth && currentHeight == endHeight)
                {
                    conf_client_test();
                    break;
                }
                conf_client_test();
                thread_sleep(GAnimDuration);
            }
        }

        void
        GFrameAnimation_X(const int & endX)
        {
            while (true)
            {
                if (currentX == endX)
                {
                    conf_client_x();
                    break;
                }
                conf_client_x();
                thread_sleep(GAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
         * 
         * This function updates the 'x' of a window in a loop until the current 'x'
         * matches the specified end 'x'. It uses the "XStep()" function to incrementally
         * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endX The desired 'x' position of the window.
         *
         */
        CliXAnimation(const int & endX) 
        {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentX == endX) 
                {
                    break;
                }
                currentX += stepX;
                thread_sleep(XAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
         * 
         * This function updates the 'y' of a window in a loop until the current 'y'
         * matches the specified end 'y'. It uses the "YStep()" function to incrementally
         * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endY The desired 'y' positon of the window.
         *
         */
        CliYAnimation(const int & endY) 
        {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentY == endY) 
                {
                    break;
                }
                currentY += stepY;
                thread_sleep(YAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs a 'width' animation until the specified end 'width' is reached.
         * 
         * This function updates the 'width' of a window in a loop until the current 'width'
         * matches the specified end 'width'. It uses the "WStep()" function to incrementally
         * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'width' of the window.
         *
         */
        CliWAnimation(const int & endWidth) 
        {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentWidth == endWidth) 
                {
                    break;
                }
                currentWidth += stepWidth;
                thread_sleep(WAnimDuration);
            }
        }

        void /**
         *
         * @brief Performs a 'height' animation until the specified end 'height' is reached.
         * 
         * This function updates the 'height' of a window in a loop until the current 'height'
         * matches the specified end 'height'. It uses the "HStep()" function to incrementally
         * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'height' of the window.
         *
         */
        CliHAnimation(const int & endHeight) 
        {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentHeight == endHeight) 
                {
                    break;
                }
                currentHeight += stepHeight;
                thread_sleep(HAnimDuration);
            }
        }

        void /**
         *
         * @brief Stops all animations by setting the corresponding flags to true and joining the animation threads.
         *        After joining the threads, the flags are set back to false.
         *
         */
        stopAnimations() 
        {
            stopHFlag.store(true);
            stopXFlag.store(true);
            stopYFlag.store(true);
            stopWFlag.store(true);
            stopHFlag.store(true);

            if (GAnimationThread.joinable()) 
            {
                GAnimationThread.join();
                stopGFlag.store(false);
            }

            if (XAnimationThread.joinable()) 
            {
                XAnimationThread.join();
                stopXFlag.store(false);
            }

            if (YAnimationThread.joinable()) 
            {
                YAnimationThread.join();
                stopYFlag.store(false);
            }

            if (WAnimationThread.joinable()) 
            {
                WAnimationThread.join();
                stopWFlag.store(false);
            }

            if (HAnimationThread.joinable()) 
            {
                HAnimationThread.join();
                stopHFlag.store(false);
            }
        }

        void /**
         *
         * @brief Sleeps the current thread for the specified number of milliseconds.
         *
         * @param milliseconds The number of milliseconds to sleep. A double is used to allow for
         *                     fractional milliseconds, providing finer control over animation timing.
         *
         * @note This is needed as the time for each thread to sleep is the main thing to be calculated, 
         *       as this class is designed to iterate every pixel and then only update that to the X-server at the given framerate.
         *
         */
        thread_sleep(const double & milliseconds) 
        {
            // Creating a duration with double milliseconds
            auto duration = std::chrono::duration<double, std::milli>(milliseconds);

            // Sleeping for the duration
            std::this_thread::sleep_for(duration);
        }

        bool /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        XisTimeToRender() 
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - XlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                XlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }

        bool /**
         *
         * Checks if it is time to render a frame based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        YisTimeToRender() 
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - YlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                YlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        
        bool /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        WisTimeToRender()
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - WlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                WlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }

        bool /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        HisTimeToRender()
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - HlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                HlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
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
        config_window(const uint32_t & mask, const uint32_t & value)
        {
            xcb_configure_window
            (
                connection,
                window,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(connection);
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
        config_window(const xcb_window_t & win, const uint32_t & mask, const uint32_t & value)
        {
            xcb_configure_window
            (
                connection,
                win,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(connection);
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
        config_client(const uint32_t & mask, const uint32_t & value)
        {
            xcb_configure_window
            (
                connection,
                c->win,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );

            xcb_configure_window
            (
                connection,
                c->frame,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(connection);
        }

        void 
        conf_client_test()
        {
            const uint32_t x = currentX, y = currentY, w = currentWidth, h = currentHeight;

            // c->win
            xcb_configure_window
            (
                connection,
                c->win,
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(w - (BORDER_SIZE * 2)),
                    static_cast<const uint32_t &>(h - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2))
                }
            );
            xcb_flush(connection);

            // c->frame
            xcb_configure_window
            (
                connection,
                c->frame,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[4])
                {
                    static_cast<const uint32_t &>(x),
                    static_cast<const uint32_t &>(y),
                    static_cast<const uint32_t &>(w),
                    static_cast<const uint32_t &>(h)
                }
            );
            xcb_flush(connection);

            // c->titlebar
            xcb_configure_window
            (
                connection,
                c->titlebar,
                XCB_CONFIG_WINDOW_WIDTH,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - (BORDER_SIZE * 2))
                }
            );
            xcb_flush(connection);

            // c->close_button
            xcb_configure_window
            (
                connection,
                c->close_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - 20 - BORDER_SIZE)
                }
            );
            xcb_flush(connection);

            // c->max_button
            xcb_configure_window
            (
                connection,
                c->max_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - 40 - BORDER_SIZE)
                }
            );
            xcb_flush(connection);

            // c->min_button
            xcb_configure_window
            (
                connection,
                c->min_button,
                XCB_CONFIG_WINDOW_X,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(w - 60 - BORDER_SIZE)
                }
            );
            xcb_flush(connection);

            // c->border.left
            xcb_configure_window
            (
                connection,
                c->border.left,
                XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(h - (BORDER_SIZE * 2))
                }
            );
            xcb_flush(connection);

            // c->border.right
            xcb_configure_window
            (
                connection, 
                c->border.right, 
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_HEIGHT,
                (const uint32_t[2])
                {
                    static_cast<const uint32_t &>(w - BORDER_SIZE),
                    static_cast<const uint32_t &>(h - (BORDER_SIZE * 2))
                }
            );
            xcb_flush(connection);

            // c->border.top
            xcb_configure_window
            (
                connection, 
                c->border.top, 
                XCB_CONFIG_WINDOW_WIDTH, 
                (const uint32_t[1]) 
                { 
                    static_cast<const uint32_t &>(w - (BORDER_SIZE * 2)) 
                }
            );
            xcb_flush(connection);

            // c->border.bottom
            xcb_configure_window
            (
                connection, 
                c->border.bottom, 
                XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, 
                (const uint32_t[2]) 
                { 
                    static_cast<const uint32_t &>(h - BORDER_SIZE),
                    static_cast<const uint32_t &>(w - (BORDER_SIZE * 2))
                }
            );
            xcb_flush(connection);

            mxb::conf::win::x(c->border.top_right, (w - BORDER_SIZE));
            xcb_flush(connection);

            mxb::conf::win::x_y(c->border.bottom_right, (w - BORDER_SIZE), (h - BORDER_SIZE));
            xcb_flush(connection);

            mxb::conf::win::y(c->border.bottom_left, (h - BORDER_SIZE));
            xcb_flush(connection);
        }

        void 
        conf_client_x()
        {
            const uint32_t x = currentX;

            xcb_configure_window
            (
                connection, 
                c->frame, 
                XCB_CONFIG_WINDOW_X, 
                (const uint32_t[1]) { x }
            );
            xcb_flush(connection);
        }
    ;
};

void
animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
    XCPPBAnimator anim(conn, c->frame);
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
    mxb::Client::update(c);
}

void
animate_client(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
    XCPPBAnimator client_anim(conn, c);
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
    mxb::Client::update(c);
}

std::mutex mtx;

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
                        mxb::Client::unmap(c);
                    }
                }
            }

            cur_d = desktop_list[n - 1];
            for (const auto & c : cur_d->current_clients)
            {
                if (c)
                {
                    mxb::Client::map(c);
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
            mxb::Client::update(c);
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

void
move_to_next_desktop_w_app()
{
    LOG_func
    if (cur_d->desktop == desktop_list.size())
    {
        return;
    }

    if (focused_client)
    {
        focused_client->desktop = cur_d->desktop + 1;
    }

    change_desktop::teleport_to(cur_d->desktop + 1);
}

void
move_to_previus_desktop_w_app()
{
    LOG_func
    if (cur_d->desktop == 1)
    {
        return;
    }

    if (focused_client)
    {
        focused_client->desktop = cur_d->desktop - 1;
    }

    change_desktop::teleport_to(cur_d->desktop - 1);
    mxb::Client::raise(focused_client);
}

class resize_client
{
    public:
        /* 
            THE REASON FOR THE 'retard_int' IS BECUSE WITHOUT IT 
            I CANNOT CALL THIS CLASS LIKE THIS 'resize_client(c)' 
            INSTEAD I WOULD HAVE TO CALL IT LIKE THIS 'resize_client rc(c)'
            AND NOW WITH THE 'retard_int' I CAN CALL IT LIKE THIS 'resize_client(c, 0)'
         */
        resize_client(client * & c , int retard_int) 
        : c(c) 
        {
            if (mxb::EWMH::check::is_window_fullscreen(c->win))
            {
                return;
            }

            mxb::pointer::grab(c->frame);
            mxb::pointer::teleport(c->x + c->width, c->y + c->height);
            run();
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }

        class no_border
        {
            public:   
                no_border(client * & c, const uint32_t & x, const uint32_t & y)
                : c(c)
                {
                    if (mxb::EWMH::check::is_window_fullscreen(c->win))
                    {
                        return;
                    }
                    
                    mxb::pointer::grab(c->frame);
                    edge edge = mxb::Client::get::client_edge(c, 10);
                    teleport_mouse(edge);
                    run(edge);
                    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                    xcb_flush(conn);
                }
            ;

            private:
                client * & c;
                uint32_t x;
                uint32_t y;
                const double frameRate = 120.0;
                std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate; 

                void 
                teleport_mouse(edge edge) 
                {
                    switch (edge) 
                    {
                        case edge::TOP:
                        {
                            mxb::pointer::teleport(mxb::pointer::get::x(), c->y);
                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            mxb::pointer::teleport(mxb::pointer::get::x(), (c->y + c->height));
                            break;
                        } 
                        case edge::LEFT:
                        {
                            mxb::pointer::teleport(c->x, mxb::pointer::get::y());
                            break;
                        }
                        case edge::RIGHT:
                        {
                            mxb::pointer::teleport((c->x + c->width), mxb::pointer::get::y());
                            break;
                        }
                        case edge::NONE:
                        {
                            break;
                        }
                        case edge::TOP_LEFT:
                        {
                            mxb::pointer::teleport(c->x, c->y);
                            break;
                        }
                        case edge::TOP_RIGHT:
                        {
                            mxb::pointer::teleport((c->x + c->width), c->y);
                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            mxb::pointer::teleport(c->x, (c->y + c->height));
                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {
                            mxb::pointer::teleport((c->x + c->width), (c->y + c->height));
                            break;
                        }
                    }
                }

                void
                resize_client(const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge) 
                    {
                        case edge::LEFT:
                        {
                            const uint32_t width = (c->width + c->x - x);

                            mxb::conf::win::width(c->win, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_width(c->frame, x, (width));
                            mxb::conf::win::width(c->titlebar, (width));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::RIGHT:
                        {
                            const uint32_t width = (x - c->x);

                            mxb::conf::win::width(c->win, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->frame, width);
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::TOP:
                        {
                            const uint32_t height = (c->height + c->y - y);

                            mxb::conf::win::height(c->win, (height - TITLE_BAR_HEIGHT) - (BORDER_SIZE * 2));
                            mxb::conf::win::y_height(c->frame, y, height);

                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::height(c->win, (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->frame, height);
                            
                            break;
                        }
                        case edge::NONE:
                        {
                            return;
                        }
                        case edge::TOP_LEFT:
                        {
                            const uint32_t height = (c->height + c->y - y);
                            const uint32_t width = (c->width + c->x - x);

                            mxb::conf::win::x_y_width_height(c->frame, x, y, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));

                            break;
                        }
                        case edge::TOP_RIGHT:
                        {   
                            const uint32_t width = (x - c->x);
                            const uint32_t height = (c->height + c->y - y);

                            mxb::conf::win::y_width_height(c->frame, y, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));

                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            const uint32_t width = (c->width + c->x - x);
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::x_width_height(c->frame, x, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));

                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {   
                            const uint32_t width = (x - c->x);
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::width_height(c->frame, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));

                            break;
                        }
                    }
                }

                void /* 
                    THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
                 */
                run(edge edge)
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
                                mxb::Client::update(c);
                                break;
                            }
                        }
                        // Free the event memory after processing
                        free(ev); 
                    }
                }
                
                bool 
                isTimeToRender() 
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
            public:   
                border(client * & c, edge _edge)
                : c(c)
                {
                    if (mxb::EWMH::check::is_window_fullscreen(c->win))
                    {
                        return;
                    }

                    std::map<client *, edge> map = mxb::Client::calc::if_client_is_next_to_other_client(c, _edge);
                    for (const auto & pair : map)
                    {
                        if (pair.first != nullptr)
                        {
                            c2 = pair.first;
                            c2_edge = pair.second;
                            mxb::pointer::grab(c->frame);
                            teleport_mouse(_edge);
                            run_double(_edge);
                            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                            xcb_flush(conn);
                            return;
                        }
                    }
                    
                    mxb::pointer::grab(c->frame);
                    teleport_mouse(_edge);
                    run(_edge);
                    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
                    xcb_flush(conn);
                }
            ;

            private:
                client * & c;
                client * c2;
                edge c2_edge; 
                const double frameRate = 120.0;
                std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;

                void 
                teleport_mouse(edge edge) 
                {
                    switch (edge) 
                    {
                        case edge::TOP:
                        {
                            mxb::pointer::teleport(mxb::pointer::get::x(), c->y);
                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            mxb::pointer::teleport(mxb::pointer::get::x(), (c->y + c->height));
                            break;
                        } 
                        case edge::LEFT:
                        {
                            mxb::pointer::teleport(c->x, mxb::pointer::get::y());
                            break;
                        }
                        case edge::RIGHT:
                        {
                            mxb::pointer::teleport((c->x + c->width), mxb::pointer::get::y());
                            break;
                        }
                        case edge::NONE:
                        {
                            break;
                        }
                        case edge::TOP_LEFT:
                        {
                            mxb::pointer::teleport(c->x, c->y);
                            break;
                        }
                        case edge::TOP_RIGHT:
                        {
                            mxb::pointer::teleport((c->x + c->width), c->y);
                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            mxb::pointer::teleport(c->x, (c->y + c->height));
                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {
                            mxb::pointer::teleport((c->x + c->width), (c->y + c->height));
                            break;
                        }
                    }
                }

                void
                resize_client(const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge) 
                    {
                        case edge::LEFT:
                        {
                            const uint32_t width = (c->width + c->x - x);

                            mxb::conf::win::width(c->win, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_width(c->frame, x, (width));
                            mxb::conf::win::width(c->titlebar, (width));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::x(c->border.right, (width - BORDER_SIZE));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.bottom, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->border.bottom_right, (width - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::RIGHT:
                        {
                            const uint32_t width = (x - c->x);

                            mxb::conf::win::width(c->win, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->frame, width);
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::x(c->border.right, (width - BORDER_SIZE));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.bottom, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->border.bottom_right, (width - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::TOP:
                        {
                            const uint32_t height = (c->height + c->y - y);

                            mxb::conf::win::height(c->win, (height - TITLE_BAR_HEIGHT) - (BORDER_SIZE * 2));
                            mxb::conf::win::y_height(c->frame, y, height);
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->border.right, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::y(c->border.bottom, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_right, (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::height(c->win, (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->frame, height);
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->border.right, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::y(c->border.bottom, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_right, (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::NONE:
                        {
                            return;
                        }
                        case edge::TOP_LEFT:
                        {
                            const uint32_t height = (c->height + c->y - y);
                            const uint32_t width = (c->width + c->x - x);

                            mxb::conf::win::x_y_width_height(c->frame, x, y, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));

                            break;
                        }
                        case edge::TOP_RIGHT:
                        {
                            const uint32_t width = (x - c->x);
                            const uint32_t height = (c->height + c->y - y);

                            mxb::conf::win::y_width_height(c->frame, y, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            const uint32_t width = (c->width + c->x - x);
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::x_width_height(c->frame, x, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - (BORDER_SIZE * 2) - TITLE_BAR_HEIGHT));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {
                            const uint32_t width = (x - c->x);
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::width_height(c->frame, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
                            
                            break;
                        }
                    }
                }

                void
                resize_client(client * c, const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge) 
                    {
                        case edge::LEFT:
                        {
                            const uint32_t width = (c->width + c->x - x);

                            mxb::conf::win::width(c->win, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_width(c->frame, x, (width));
                            mxb::conf::win::width(c->titlebar, (width));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::x(c->border.right, (width - BORDER_SIZE));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.bottom, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->border.bottom_right, (width - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::RIGHT:
                        {
                            const uint32_t width = (x - c->x);

                            mxb::conf::win::width(c->win, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->frame, width);
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::x(c->border.right, (width - BORDER_SIZE));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.bottom, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->border.bottom_right, (width - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::TOP:
                        {
                            const uint32_t height = (c->height + c->y - y);

                            mxb::conf::win::height(c->win, (height - TITLE_BAR_HEIGHT) - (BORDER_SIZE * 2));
                            mxb::conf::win::y_height(c->frame, y, height);
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->border.right, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::y(c->border.bottom, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_right, (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::BOTTOM_edge:
                        {
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::height(c->win, (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->frame, height);
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::height(c->border.right, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::y(c->border.bottom, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_right, (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::NONE:
                        {
                            return;
                        }
                        case edge::TOP_LEFT:
                        {
                            const uint32_t height = (c->height + c->y - y);
                            const uint32_t width = (c->width + c->x - x);

                            mxb::conf::win::x_y_width_height(c->frame, x, y, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));

                            break;
                        }
                        case edge::TOP_RIGHT:
                        {
                            const uint32_t width = (x - c->x);
                            const uint32_t height = (c->height + c->y - y);

                            mxb::conf::win::y_width_height(c->frame, y, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::BOTTOM_LEFT:
                        {
                            const uint32_t width = (c->width + c->x - x);
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::x_width_height(c->frame, x, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - (BORDER_SIZE * 2) - TITLE_BAR_HEIGHT));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
                            
                            break;
                        }
                        case edge::BOTTOM_RIGHT:
                        {
                            const uint32_t width = (x - c->x);
                            const uint32_t height = (y - c->y);

                            mxb::conf::win::width_height(c->frame, width, height);
                            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->titlebar, (width - BORDER_SIZE));
                            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
                            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
                            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
                            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
                            
                            break;
                        }
                    }
                }

                void
                snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox)
                {
                    uint16_t left_border   = 0;
                    uint16_t right_border  = 0;
                    uint16_t top_border    = 0;
                    uint16_t bottom_border = 0;

                    for (const auto & c : cur_d->current_clients)
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

                void /* 
                    THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
                 */
                run(edge edge)
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
                                mxb::Client::update(c);
                                break;
                            }
                        }
                        // Free the event memory after processing
                        free(ev); 
                    }
                }

                void /* 
                    THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
                 */
                run_double(edge edge)
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
                                    resize_client(c, e->root_x, e->root_y, edge);
                                    resize_client(c2, e->root_x, e->root_y, c2_edge);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: 
                            {
                                shouldContinue = false;                        
                                mxb::Client::update(c);
                                mxb::Client::update(c2);
                                break;
                            }
                        }
                        // Free the event memory after processing
                        free(ev); 
                    }
                }

                bool 
                isTimeToRender() 
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
    ;

    private:
        client * & c;
        uint32_t x;
        uint32_t y;
        const double frameRate = 120.0;
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
        const double frameDuration = 1000.0 / frameRate; 

        void
        resize_win(const uint16_t & width, const uint16_t & height)
        {
            mxb::conf::win::width_height(c->win, (width - (BORDER_SIZE * 2)), (height - (BORDER_SIZE * 2)));
            mxb::conf::win::width_height(c->frame, width, height);
            mxb::conf::win::width(c->titlebar, (width - (BORDER_SIZE * 2)));
            mxb::conf::win::x(c->close_button, (width - BUTTON_SIZE - BORDER_SIZE));
            mxb::conf::win::x(c->max_button, (width - (BUTTON_SIZE * 2) - BORDER_SIZE));
            mxb::conf::win::x(c->min_button, (width - (BUTTON_SIZE * 3) - BORDER_SIZE));
            mxb::conf::win::height(c->border.left, (height - (BORDER_SIZE * 2)));
            mxb::conf::win::x_height(c->border.right, (width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
            mxb::conf::win::width(c->border.top, (width - (BORDER_SIZE * 2)));
            mxb::conf::win::y_width(c->border.bottom, (height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
            mxb::conf::win::x(c->border.top_right, (width - BORDER_SIZE));
            mxb::conf::win::x_y(c->border.bottom_right, (width - BORDER_SIZE), (height - BORDER_SIZE));
            mxb::conf::win::y(c->border.bottom_left, (height - BORDER_SIZE));
        }

        void
        snap(const uint16_t & x, const uint16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : cur_d->current_clients)
            {
                if (cli == this->c)
                {
                    continue;
                }

                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((x > cli->x - N && x < cli->x + N) 
                 && (y + this->c->height > cli->y && y < cli->y + cli->height))
                {
                    resize_win(cli->x - this->c->x, y - this->c->y);
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if ((y > cli->y - N && y < cli->y + N) 
                 && (x + this->c->width > cli->x && x < cli->x + cli->width))
                {
                    resize_win(x - this->c->x ,cli->y - this->c->y);
                    return;
                }
            }
            resize_win(x - this->c->x, y - this->c->y);
        }

        void /* 
            THIS IS THE MAIN EVENT LOOP FOR 'resize_client'
         */
        run()
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
                        mxb::Client::update(c);
                        break;
                    }
                }
                // Free the event memory after processing
                free(ev); 
            }
        }

        bool 
        isTimeToRender() 
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

namespace error
{
    int
    conn_error(xcb_connection_t * conn, const char * sender_function)
    {
        if (xcb_connection_has_error(conn))
        {
            log_error("XCB connection is null.");
            return CONN_ERR;
        }
        return 0;
    }

    int
    cookie_error(xcb_void_cookie_t cookie , const char * sender_function)
    {
        if (check_conn != 0)
        {
            return CONN_ERR;
        }

        xcb_generic_error_t * err = xcb_request_check(conn, cookie);
        if (err)
        {
            log_error(err->error_code);
            free(err);
            return err->error_code;
        }
        return 0;
    }
} 

class max_win
{    
    public:
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
                    if (mxb::EWMH::check::is_window_fullscreen(c->win))
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

    private:
        void
        max_win_animate(client * c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
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

        void 
        save_max_ewmh_ogsize(client * c)
        {
            c->max_ewmh_ogsize.x      = c->x;
            c->max_ewmh_ogsize.y      = c->y;
            c->max_ewmh_ogsize.width  = c->width;
            c->max_ewmh_ogsize.height = c->height;
        }

        void
        ewmh_max_win(client * c)
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
            mxb::EWMH::set::max_win_state(c->win);
            xcb_flush(conn);
        }

        void
        ewmh_unmax_win(client * c)
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
            mxb::EWMH::unset::max_win_state(c->win);
            xcb_flush(conn);
        }

        void 
        save_max_button_ogsize(client * c)
        {
            c->max_button_ogsize.x      = c->x;
            c->max_button_ogsize.y      = c->y;
            c->max_button_ogsize.width  = c->width;
            c->max_button_ogsize.height = c->height;
        }

        void
        button_max_win(client * c)
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

        void
        button_unmax_win(client * c)
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

        bool
        is_max_win(client * c)
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
    void 
    kill_client(xcb_connection_t *conn, xcb_window_t window) 
    {
        xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, NULL);

        xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
        xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply(conn, delete_cookie, NULL);

        if (!protocols_reply || !delete_reply) 
        {
            log.log(ERROR, __func__, "Could not create atoms.");
            free(protocols_reply);
            free(delete_reply);
            return;
        }

        xcb_client_message_event_t ev = {0};
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = window;
        ev.format = 32;
        ev.sequence = 0;
        ev.type = protocols_reply->atom;
        ev.data.data32[0] = delete_reply->atom;
        ev.data.data32[1] = XCB_CURRENT_TIME;

        xcb_send_event(conn, 0, window, XCB_EVENT_MASK_NO_EVENT, (char *) & ev);

        free(protocols_reply);
        free(delete_reply);
    }

    void
    apply_event_mask(const xcb_event_mask_t ev_mask, const xcb_window_t & win)
    {
        xcb_change_window_attributes
        (
            conn,
            win,
            XCB_CW_EVENT_MASK,
            (const uint32_t[1])
            {
                ev_mask
            }
        );
    }

    void
    apply_event_mask(const uint32_t * ev_mask, const xcb_window_t & win)
    {
        xcb_change_window_attributes
        (
            conn,
            win,
            XCB_CW_EVENT_MASK,
            ev_mask
        );
    }

    void
    grab_buttons(const xcb_window_t & win, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
    {
        for (const auto & binding : bindings)
        {
            const uint8_t & button = binding.first;
            const uint16_t & modifier = binding.second;
            xcb_grab_button
            (
                conn, 
                
                // 'OWNER_EVENTS : SET TO 0 FOR NO EVENT PROPAGATION'
                1, 
                win, 
                
                // EVENT MASK
                XCB_EVENT_MASK_BUTTON_PRESS, 
                
                // POINTER MODE
                XCB_GRAB_MODE_ASYNC, 
                
                // KEYBOARD MODE
                XCB_GRAB_MODE_ASYNC, 
                
                // CONFINE TO WINDOW
                XCB_NONE, 
                
                // CURSOR
                XCB_NONE, 
                button, 
                modifier    
            );
            // FLUSH THE REQUEST TO THE X SERVER
            xcb_flush(conn); 
        }
    }

    xcb_visualtype_t * /**
     *
     * @brief Function to find an ARGB visual 
     *
     */
    find_argb_visual(xcb_connection_t *conn, xcb_screen_t *screen) 
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

    bool 
    getPointerPosition(xcb_window_t window, int& posX, int& posY) 
    {
        if (check_conn == CONN_ERR) 
        {
            return false;
        }

        // Query pointer
        xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, window);
        xcb_query_pointer_reply_t* reply = xcb_query_pointer_reply(conn, cookie, nullptr);

        if (!reply) 
        {
            log_error("Unable to query pointer position.");
            return false;
        }

        // Get pointer coordinates
        posX = reply->root_x;
        posY = reply->root_y;

        free(reply);
        return true;
    }

    int
    send_sigterm_to_client(client * c)
    {
        if (!c)
        {
            return - 1;
        }

        xcb_unmap_window(conn, c->win);
        xcb_unmap_window(conn, c->close_button);
        xcb_unmap_window(conn, c->max_button);
        xcb_unmap_window(conn, c->min_button);
        xcb_unmap_window(conn, c->titlebar);
        xcb_unmap_window(conn, c->frame);
        xcb_unmap_window(conn, c->border.left);
        xcb_unmap_window(conn, c->border.right);
        xcb_unmap_window(conn, c->border.top);
        xcb_unmap_window(conn, c->border.bottom);
        xcb_unmap_window(conn, c->border.top_right);
        xcb_unmap_window(conn, c->border.bottom_left);
        xcb_unmap_window(conn, c->border.bottom_right);
        xcb_flush(conn);

        kill_client(conn, c->win);
        kill_client(conn, c->close_button);
        kill_client(conn, c->max_button);
        kill_client(conn, c->min_button);
        kill_client(conn, c->titlebar);
        kill_client(conn, c->frame);
        kill_client(conn, c->border.left);
        kill_client(conn, c->border.right);
        kill_client(conn, c->border.top);
        kill_client(conn, c->border.bottom);
        kill_client(conn, c->border.top_right);
        kill_client(conn, c->border.bottom_left);
        kill_client(conn, c->border.bottom_right);
        xcb_flush(conn);
        
        mxb::Client::remove(c);

        return 0;
    }

    void
    close_button_kill(client * c)
    {
        int result = send_sigterm_to_client(c);

        if (result == -1)
        {
            log_error("client is nullptr");
        }
    }
}

class Compositor
{
    public:
        Compositor(xcb_connection_t* connection, xcb_screen_t* screen) 
        : connection(connection), root(screen->root) 
        {
            // Initialize XRender for ARGB visuals and pictures
            initRenderExtension();
        }

        void 
        addWindow(xcb_window_t window) 
        {
            // Check for a valid connection
            if (xcb_connection_has_error(connection)) 
            {
                log_error("XCB connection is null.");
                return;
            }

            // Get window dimensions
            uint16_t width = mxb::get::win::width(window);
            uint16_t height = mxb::get::win::height(window);
            if (width <= 0 || height <= 0) 
            {
                log_error("Invalid window dimensions.");
                return;
            }

            // Create an off-screen pixmap
            xcb_pixmap_t pixmap = xcb_generate_id(connection);
            xcb_void_cookie_t pixmap_cookie = xcb_create_pixmap_checked(connection, 32, pixmap, root, width, height);
            
            // Check for errors in pixmap creation
            xcb_generic_error_t* err = xcb_request_check(connection, pixmap_cookie);
            if (err) 
            {
                log_error("Failed to create pixmap. ERROR_CODE: " + std::to_string(err->error_code));
                free(err);
                return;
            }

            // Create a render picture
            xcb_render_picture_t picture = xcb_generate_id(connection);
            xcb_void_cookie_t picture_cookie = xcb_render_create_picture_checked(connection, picture, pixmap, renderFormat, 0, nullptr);

            // Check for errors in picture creation
            err = xcb_request_check(connection, picture_cookie);
            if (err) 
            {
                log_error("Failed to create render picture. ERROR_CODE: " + std::to_string(err->error_code));
                free(err);

                // Clean up the pixmap since picture creation failed
                xcb_free_pixmap(connection, pixmap);
                return;
            }

            // Store the picture associated with the window
            windowPictures[window] = picture;
        }

        void 
        removeWindow(xcb_window_t window) 
        {
            // Clean up resources for the window
            auto it = windowPictures.find(window);
            if (it != windowPictures.end()) {
                xcb_render_free_picture(connection, it->second);
                windowPictures.erase(it);
            }
        }

        void 
        composite() 
        {
            // Composite all windows onto the screen
            for (const auto& pair : windowPictures) {
                xcb_render_picture_t srcPic = pair.second;
                xcb_render_composite
                (
                    connection, 
                    XCB_RENDER_PICT_OP_OVER, 
                    srcPic, 
                    XCB_NONE, 
                    rootPicture,
                    0, 
                    0, 
                    0, 
                    0, 
                    0, 
                    0, 
                    mxb::get::win::width(pair.first), 
                    mxb::get::win::height(pair.first)
                );
            }

            xcb_flush(connection);
        }

        void 
        setTransparency(xcb_window_t window, float alpha) 
        {
            auto it = windowPictures.find(window);
            if (it != windowPictures.end()) 
            {
                xcb_render_picture_t picture = it->second;

                uint16_t w, h;
                mxb::get::win::width_height(window, w, h);

                // Create a semi-transparent rectangle
                xcb_rectangle_t rect = {0, 0, w, h};
                uint32_t color = (static_cast<uint32_t>(alpha * 255) << 24); // Adjust alpha value

                // Create a graphic context for alpha blending
                xcb_gcontext_t gc = xcb_generate_id(connection);
                xcb_create_gc(connection, gc, root, 0, nullptr);

                // Fill the rectangle with the semi-transparent color
                xcb_change_gc(connection, gc, XCB_GC_FOREGROUND, &color);
                xcb_poly_fill_rectangle(connection, picture, gc, 1, &rect);

                // Free the graphic context
                xcb_free_gc(connection, gc);
            }
        }
    ;

    private:
        xcb_connection_t* connection;
        xcb_window_t root;
        xcb_render_pictformat_t renderFormat;
        std::unordered_map<xcb_window_t, xcb_render_picture_t> windowPictures;
        xcb_render_picture_t rootPicture;

        void 
        initRenderExtension() 
        {
            // Find ARGB visual and create a root picture
            xcb_render_query_pict_formats_cookie_t cookie = xcb_render_query_pict_formats(connection);
            xcb_render_query_pict_formats_reply_t* reply = xcb_render_query_pict_formats_reply(connection, cookie, nullptr);

            renderFormat = findArgbFormat(reply);
            rootPicture = xcb_generate_id(connection);
            xcb_render_create_picture(connection, rootPicture, root, renderFormat, 0, nullptr);

            free(reply);
        }

        xcb_render_pictformat_t 
        findArgbFormat(xcb_render_query_pict_formats_reply_t* reply) 
        {
            if (!reply) 
            {
                log_error("XRender query pict formats reply is null.");
                return 0;
            }

            xcb_render_pictformat_t format = 0;

            // Iterate over screens
            xcb_render_pictscreen_iterator_t screen_iter = xcb_render_query_pict_formats_screens_iterator(reply);
            for (; screen_iter.rem; xcb_render_pictscreen_next(&screen_iter)) 
            {
                if (!screen_iter.data) 
                {
                    log_error("Invalid pictscreen data.");
                    continue;
                }

                // Iterate over depths
                xcb_render_pictdepth_iterator_t depth_iter = xcb_render_pictscreen_depths_iterator(screen_iter.data);
                for (; depth_iter.rem; xcb_render_pictdepth_next(&depth_iter)) 
                {
                    if (!depth_iter.data) 
                    {
                        log_error("Invalid pictdepth data.");
                        continue;
                    }

                    // Check if the depth is 32
                    if (depth_iter.data->depth == 32) 
                    {
                        // Iterate over visuals at this depth
                        xcb_render_pictvisual_iterator_t visual_iter = xcb_render_pictdepth_visuals_iterator(depth_iter.data);
                        for (; visual_iter.rem; xcb_render_pictvisual_next(&visual_iter)) 
                        {
                            if (!visual_iter.data) 
                            {
                                log_error("Invalid pictvisual data.");
                                continue;
                            }

                            format = visual_iter.data->format;
                            if (format)
                            {
                                return format; /** 
                                 *
                                 * If a visual with 32-bit depth is found, return its format 
                                 *
                                 */
                            } 
                        }
                    }
                }
            }

            if (format == 0) 
            {
                log_error("No suitable ARGB format found.");
            }

            return format;
        }
    ;
};

class WinDecoretor
{
    public:
        WinDecoretor(xcb_connection_t * connection, client * c)
        : c(c)
        {
            make_frame(c);
            make_titlebar(c);
            make_close_button(c);
            make_max_button(c);
            make_min_button(c);
            
            if (BORDER_SIZE > 0)
            {
                make_borders(c);
            }
        }
    ;
        
    private:
        client * c;

        void
        apply_event_mask(const xcb_event_mask_t ev_mask, const xcb_window_t & win)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_EVENT_MASK,
                (const uint32_t[1])
                {
                    ev_mask
                }
            );
        }

        void
        apply_event_mask(const uint32_t * values, const xcb_window_t & win)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_EVENT_MASK,
                values
            );
        }

        void 
        make_frame(client * & c)
        {
            // CREATE A FRAME WINDOW
            c->frame = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->frame, 
                screen->root, 
                c->x - BORDER_SIZE, 
                c->y - TITLE_BAR_HEIGHT - BORDER_SIZE, 
                c->width + (BORDER_SIZE * 2), 
                c->height + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2), 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );

            mxb::set::win::backround::as_color(c->frame, DARK_GREY);

            // REPARENT THE PROGRAM_WINDOW TO THE FRAME_WINDOW
            xcb_reparent_window
            (
                conn, 
                c->win, 
                c->frame, 
                BORDER_SIZE, 
                TITLE_BAR_HEIGHT + BORDER_SIZE
            );

            xcb_map_window(conn, c->frame);
            xcb_flush(conn);

            uint32_t mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
            apply_event_mask(&mask, c->frame);
            xcb_flush(conn);
        }

        void
        make_titlebar(client * & c)
        {
            c->titlebar = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->titlebar, 
                c->frame, 
                BORDER_SIZE, 
                BORDER_SIZE, 
                c->width, 
                TITLE_BAR_HEIGHT, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );

            mxb::set::win::backround::as_color(c->titlebar, BLACK);

            win_tools::grab_buttons(c->titlebar, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->titlebar);
            xcb_flush(conn);

            mxb::draw::text(c->titlebar, "sug", WHITE, BLACK, "7x14", 2, 14);
        }

        void
        make_close_button(client * & c)
        {
            c->close_button = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->close_button,
                c->frame,
                (c->width - BUTTON_SIZE) + BORDER_SIZE,
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );

            mxb::set::win::backround::as_color(c->close_button, BLUE);

            win_tools::grab_buttons(c->close_button, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->close_button);
            xcb_flush(conn);

            uint mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
            apply_event_mask(& mask, c->close_button);
            xcb_flush(conn);

            mxb::create::png("/home/mellw/close.png", bitmap::CHAR_BITMAP_CLOSE_BUTTON);
            mxb::set::win::backround::as_png("/home/mellw/close.png", c->close_button);

            // mxb::set::win::backround::as_png("/home/mellw/mwm_png/window_decoration_icons/close_button/1_10x10.png", c->close_button);
        }

        void
        make_max_button(client * & c)
        {
            c->max_button = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->max_button,
                c->frame,
                (c->width - (BUTTON_SIZE * 2)) + BORDER_SIZE,
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );

            mxb::set::win::backround::as_color(c->max_button, RED);

            uint32_t mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
            apply_event_mask(& mask, c->max_button);

            win_tools::grab_buttons(c->max_button, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->max_button);
            xcb_flush(conn);

            mxb::create::icon::max_button("/home/mellw/max.png");
            mxb::set::win::backround::as_png("/home/mellw/max.png", c->max_button);
        }

        void
        make_min_button(client * & c)
        {
            c->min_button = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->min_button,
                c->frame,
                (c->width - (BUTTON_SIZE * 3)) + BORDER_SIZE,
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );

            mxb::set::win::backround::as_color(c->min_button, GREEN);

            uint32_t mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
            apply_event_mask(& mask, c->min_button);

            win_tools::grab_buttons(c->min_button, {
               {   L_MOUSE_BUTTON,     NULL }
            });

            xcb_map_window(conn, c->min_button);
            xcb_flush(conn);

            mxb::create::Bitmap bitmap(20, 20);            
            bitmap.modify(9, 4, 16, true);
            bitmap.modify(10, 4, 16, true);
            bitmap.exportToPng("/home/mellw/min.png");

            mxb::set::win::backround::as_png("/home/mellw/min.png", c->min_button);
        }

        void
        make_borders(client * & c)
        {
            c->border.left = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->border.left,
                c->frame,
                0,
                BORDER_SIZE,
                BORDER_SIZE,
                c->height + TITLE_BAR_HEIGHT,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );
            mxb::set::win::backround::as_color(c->border.left, BLACK);
            mxb::pointer::set(c->border.left, CURSOR::left_side);
            win_tools::grab_buttons(c->border.left, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.left);
            xcb_flush(conn);

            c->border.right = xcb_generate_id(conn);
            xcb_create_window
            (
                conn,
                XCB_COPY_FROM_PARENT,
                c->border.right,
                c->frame,
                c->width + BORDER_SIZE,
                BORDER_SIZE,
                BORDER_SIZE,
                c->height + TITLE_BAR_HEIGHT,
                0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                screen->root_visual,
                0,
                NULL
            );
            mxb::set::win::backround::as_color(c->border.right, BLACK);
            mxb::pointer::set(c->border.right, CURSOR::right_side);
            win_tools::grab_buttons(c->border.right, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.right);
            xcb_flush(conn);

            c->border.top = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->border.top, 
                c->frame, 
                BORDER_SIZE, 
                0, 
                c->width, 
                BORDER_SIZE, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT,  
                screen->root_visual,
                0,
                NULL
            );
            mxb::set::win::backround::as_color(c->border.top, BLACK);
            mxb::pointer::set(c->border.top, CURSOR::top_side);
            win_tools::grab_buttons(c->border.top, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.top);
            xcb_flush(conn);

            c->border.bottom = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->border.bottom, 
                c->frame, 
                BORDER_SIZE, 
                c->height + TITLE_BAR_HEIGHT + BORDER_SIZE, 
                c->width, 
                BORDER_SIZE, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );
            mxb::set::win::backround::as_color(c->border.bottom, BLACK);
            mxb::pointer::set(c->border.bottom, CURSOR::bottom_side);
            win_tools::grab_buttons(c->border.bottom, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.bottom);
            xcb_flush(conn);

            c->border.top_left = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->border.top_left, 
                c->frame, 
                0, 
                0, 
                BORDER_SIZE, 
                BORDER_SIZE, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );
            mxb::set::win::backround::as_color(c->border.top_left, BLACK);
            mxb::pointer::set(c->border.top_left, CURSOR::top_left_corner);
            win_tools::grab_buttons(c->border.top_left, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.top_left);
            xcb_flush(conn);

            c->border.top_right = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->border.top_right, 
                c->frame, 
                c->width + BORDER_SIZE, 
                0, 
                BORDER_SIZE, 
                BORDER_SIZE, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );
            mxb::set::win::backround::as_color(c->border.top_right, BLACK);
            mxb::pointer::set(c->border.top_right, CURSOR::top_right_corner);
            win_tools::grab_buttons(c->border.top_right, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.top_right);
            xcb_flush(conn);
            
            c->border.bottom_left = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->border.bottom_left, 
                c->frame, 
                0, 
                c->height + TITLE_BAR_HEIGHT + BORDER_SIZE, 
                BORDER_SIZE, 
                BORDER_SIZE, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );
            mxb::set::win::backround::as_color(c->border.bottom_left, BLACK);
            mxb::pointer::set(c->border.bottom_left, CURSOR::bottom_left_corner);
            win_tools::grab_buttons(c->border.bottom_left, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.bottom_left);
            xcb_flush(conn);

            c->border.bottom_right = xcb_generate_id(conn);
            xcb_create_window
            (
                conn, 
                XCB_COPY_FROM_PARENT, 
                c->border.bottom_right, 
                c->frame, 
                c->width + BORDER_SIZE, 
                c->height + TITLE_BAR_HEIGHT + BORDER_SIZE, 
                BORDER_SIZE, 
                BORDER_SIZE, 
                0, 
                XCB_WINDOW_CLASS_INPUT_OUTPUT, 
                screen->root_visual, 
                0, 
                NULL
            );
            mxb::set::win::backround::as_color(c->border.bottom_right, BLACK);
            mxb::pointer::set(c->border.bottom_right, CURSOR::bottom_right_corner);
            win_tools::grab_buttons(c->border.bottom_right, {{ L_MOUSE_BUTTON, NULL }});
            xcb_map_window(conn, c->border.bottom_right);
            xcb_flush(conn);
        }
    ;
};

class WinManager
{
    public:
        static void 
        manage_new_window(const xcb_window_t & w) 
        {
            client * c = make_client(w);
            if (!c)
            {
                log_error("could not make client");
                return;
            }            
            mxb::conf::win::x_y_width_height(c->win, c->x, c->y, c->width, c->height);
            xcb_map_window(conn, c->win);  
            xcb_flush(conn);

            grab_buttons(c, 
            {
                {   L_MOUSE_BUTTON,     ALT },
                {   R_MOUSE_BUTTON,     ALT },
                {   L_MOUSE_BUTTON,     0   }
            });
            
            grab_keys(c, 
            {
                {   T,          ALT | CTRL              },
                {   Q,          ALT | SHIFT             },
                {   F11,        NULL                    },
                {   N_1,        ALT                     },
                {   N_2,        ALT                     },
                {   N_3,        ALT                     },
                {   N_4,        ALT                     },
                {   N_5,        ALT                     },
                {   R_ARROW,    CTRL | SUPER            },
                {   L_ARROW,    CTRL | SUPER            },
                {   R_ARROW,    CTRL | SUPER | SHIFT    },
                {   L_ARROW,    CTRL | SUPER | SHIFT    },
                {   R_ARROW,    SUPER                   },
                {   L_ARROW,    SUPER                   },
                {   U_ARROW,    SUPER                   },
                {   D_ARROW,    SUPER                   },
                {   TAB,        ALT                     },
                {   K,          SUPER                   }
            });

            WinDecoretor(conn ,c);
            
            uint32_t mask = 
            XCB_EVENT_MASK_FOCUS_CHANGE  | 
            XCB_EVENT_MASK_ENTER_WINDOW  |
            XCB_EVENT_MASK_LEAVE_WINDOW  ;
            mxb::set::event_mask(& mask, c->win);

            mxb::get::win::property(c->win, "_NET_WM_NAME");
            mxb::Client::update(c);
            focus::client(c);
        }
    ;

    private:
        static void
        grab_buttons(client * & c, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
        {
            for (const auto & binding : bindings)
            {
                const uint8_t & button = binding.first;
                const uint16_t & modifier = binding.second;
                xcb_grab_button
                (
                    conn, 
                    
                    // 'OWNER_EVENTS : SET TO 0 FOR NO EVENT PROPAGATION'
                    1, 
                    c->win, 
                    
                    // EVENT MASK
                    XCB_EVENT_MASK_BUTTON_PRESS, 
                    
                    // POINTER MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // KEYBOARD MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // CONFINE TO WINDOW
                    XCB_NONE, 
                    
                    // CURSOR
                    XCB_NONE, 
                    button, 
                    modifier    
                );
                // FLUSH THE REQUEST TO THE X SERVER
                xcb_flush(conn); 
            }
        }

        static void
        grab_buttons(const xcb_window_t & win, std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
        {
            for (const auto & binding : bindings)
            {
                const uint8_t & button = binding.first;
                const uint16_t & modifier = binding.second;
                xcb_grab_button
                (
                    conn, 
                    
                    // 'OWNER_EVENTS : SET TO 0 FOR NO EVENT PROPAGATION'
                    1, 
                    win, 
                    
                    // EVENT MASK
                    XCB_EVENT_MASK_BUTTON_PRESS, 
                    
                    // POINTER MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // KEYBOARD MODE
                    XCB_GRAB_MODE_ASYNC, 
                    
                    // CONFINE TO WINDOW
                    XCB_NONE, 
                    
                    // CURSOR
                    XCB_NONE, 
                    button, 
                    modifier    
                );
                // FLUSH THE REQUEST TO THE X SERVER
                xcb_flush(conn); 
            }
        }

        static void 
        grab_keys(client * & c, std::initializer_list<std::pair<const uint32_t, const uint16_t>> bindings) 
        {
            xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(conn);
        
            if (!keysyms) 
            {
                LOG_error("keysyms could not get initialized");
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
                            // CONNECTION TO THE X SERVER
                            conn,
                            
                            // 'OWNER_EVENTS' SET TO 1 TO ALLOW EVENT PROPAGATION 
                            1,

                            // KEYS WILL BE GRABBED ON THIS WINDOW          
                            c->win,
                            
                            // MODIFIER MASK
                            binding.second, 
                            
                            // KEYCODE
                            *kc,        

                            // POINTER MODE
                            XCB_GRAB_MODE_ASYNC, 
                            
                            // KEYBOARD MODE
                            XCB_GRAB_MODE_ASYNC  
                        );
                    }
                    // FREE THE MEMORY THAT WAS ALLOCATED FOR THE KEYCODES
                    free(keycodes);
                }
            }
            xcb_key_symbols_free(keysyms);
            
            // FLUSH THE REQUEST TO THE X SERVER
            // SO THAT THE X SERVER HANDELS THIS REQUEST NOW
            xcb_flush(conn); 
        }

        static void
        apply_event_mask(client * & c)
        {
            xcb_change_window_attributes
            (
                conn,
                c->win,
                XCB_CW_EVENT_MASK,
                (const uint32_t[1])
                {
                    XCB_EVENT_MASK_FOCUS_CHANGE
                }
            );
        }

        static void
        apply_event_mask(const xcb_window_t & win)
        {
            xcb_change_window_attributes
            (
                conn,
                win,
                XCB_CW_EVENT_MASK,
                (const uint32_t[1])
                {
                    XCB_EVENT_MASK_FOCUS_CHANGE
                }
            );
        }

        static bool 
        is_exclusive_fullscreen(client * & c) 
        {
            xcb_ewmh_get_atoms_reply_t wm_state;
            if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, c->win), & wm_state, NULL)) 
            {
                for (unsigned int i = 0; i < wm_state.atoms_len; i++) 
                {
                    if ((wm_state.atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN) 
                     && (i + 1 < wm_state.atoms_len) 
                     && (wm_state.atoms[i + 1] == XCB_NONE)) 
                    {
                        // EXCLUSIVE FULLSCREEN STATE DETECTED
                        xcb_ewmh_get_atoms_reply_wipe(& wm_state);
                        return true;
                    }
                }
                xcb_ewmh_get_atoms_reply_wipe(& wm_state);
            }
            return false;
        }

        static client * 
        make_client(const xcb_window_t & win) 
        {
            client * c = new client;
            if (!c) 
            {
                log_error("Could not allocate memory for client");
                return nullptr;
            }

            c->win    = win;
            c->height = (data.height < 300) ? 300 : data.height;
            c->width  = (data.width < 400)  ? 400 : data.width;
            c->x      = (data.x <= 0)       ? (screen->width_in_pixels / 2)  - (c->width / 2)  : data.x;
            c->y      = (data.y <= 0)       ? (screen->height_in_pixels / 2) - (c->height / 2) : data.y;
            c->depth   = 24;
            c->desktop = cur_d->desktop;

            for (int i = 0; i < 256; ++i)
            {
                c->name[i] = '\0';
            }

            int i = 0;
            char * name = mxb::get::win::property(c->win, "_NET_WM_NAME");
            while(name[i] != '\0' && i < 255)
            {
                c->name[i] = name[i];
                ++i;
            }
            c->name[i] = '\0';
            free(name);

            if (is_exclusive_fullscreen(c)) 
            {
                c->x      = 0;
                c->y      = 0;
                c->width  = screen->width_in_pixels;
                c->height = screen->height_in_pixels;
                mxb::EWMH::set::max_win_state(c->win);
            }
            
            client_list.push_back(c);
            cur_d->current_clients.push_back(c);
            return c;
        }

        static void
        get_win_info(client * & c)
        {
            get::WindowProperty(c, "WINDOW");
            get::WindowProperty(c, "WM_CLASS");
            get::WindowProperty(c, "FULL_NAME");
            get::WindowProperty(c, "ATOM");
            get::WindowProperty(c, "DRAWABLE");
            get::WindowProperty(c, "FONT");
            get::WindowProperty(c, "INTEGER");
            get::WindowProperty(c, "PIXMAP");
            get::WindowProperty(c, "VISUALID");
            get::WindowProperty(c, "WM_COMMAND");
            get::WindowProperty(c, "WM_HINTS");
            get::WindowProperty(c, "WM_NORMAL_HINTS");
            get::WindowProperty(c, "MIN_SPACE");
            get::WindowProperty(c, "NORM_SPACE");
            get::WindowProperty(c, "WM_SIZE_HINTS");
            get::WindowProperty(c, "NOTICE");
            get::WindowProperty(c, "_NET_WM_NAME");
            get::WindowProperty(c, "_NET_WM_STATE");
            get::WindowProperty(c, "_NET_WM_VISIBLE_NAME");
            get::WindowProperty(c, "_NET_WM_ICON_NAME");
            get::WindowProperty(c, "_NET_WM_VISIBLE_ICON_NAME");
            get::WindowProperty(c, "_NET_WM_DESKTOP");
            get::WindowProperty(c, "_NET_WM_WINDOW_TYPE");
            get::WindowProperty(c, "_NET_WM_STATE");
            get::WindowProperty(c, "_NET_WM_ALLOWED_ACTIONS");
            get::WindowProperty(c, "_NET_WM_STRUT");
            get::WindowProperty(c, "_NET_WM_STRUT_PARTIAL");
            get::WindowProperty(c, "_NET_WM_ICON_GEOMETRY");
            get::WindowProperty(c, "_NET_WM_ICON");
            get::WindowProperty(c, "_NET_WM_PID");
            get::WindowProperty(c, "_NET_WM_HANDLED_ICONS");
            get::WindowProperty(c, "_NET_WM_USER_TIME");
            get::WindowProperty(c, "_NET_WM_USER_TIME_WINDOW");
            get::WindowProperty(c, "_NET_FRAME_EXTENTS");
            get::WindowProperty(c, "_NET_SUPPORTED");
        }
    ;
};

/**
 * @class tile
 * @brief Represents a tile obj.
 * 
 * The `tile` class is responsible for managing the tiling behavior of windows in the window manager.
 * It provides methods to tile windows to the left, right, *up, or *down positions on the screen.
 * The class also includes helper methods to check the current tile position of a window and set the size and position of a window.
 */
class tile
{
    public:
        tile(client * & c, TILE tile)
        {
            if (mxb::EWMH::check::is_window_fullscreen(c->win))
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

    private:
        void 
        save_tile_ogsize(client * & c)
        {
            c->tile_ogsize.x      = c->x;
            c->tile_ogsize.y      = c->y;
            c->tile_ogsize.width  = c->width;
            c->tile_ogsize.height = c->height;
        }
        
        void
        moveresize(client * & c)
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

        bool
        current_tile_pos(client * & c, TILEPOS mode)
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

        void
        set_tile_sizepos(client * & c, TILEPOS sizepos)
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

        void 
        set_tile_ogsize(client * & c)
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

        void
        animate_old(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            XCPPBAnimator anim(conn, c->frame);
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
            mxb::Client::update(c);
        }

        void
        animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            XCPPBAnimator anim(conn,  c);
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
            mxb::Client::update(c);
        }
    ;
};

class Event
{
    public:
        /**
         *
         * @brief Constructor for the Event class.
         *        Initializes the key symbols and keycodes.
         *
         */
        Event()
        {
            initialize_keysyms();
        }

        /**
         *
         * @brief Destructor for the Event class.
         *        Frees the allocated key symbols.
         *
         */
        ~Event() 
        {
            if (keysyms) 
            {
                xcb_key_symbols_free(keysyms);
            }
        }
        
        void /**
         *
         * @brief Event handler function that processes the incoming XCB events.
         * 
         * @param ev The XCB event to be handled.
         *
         */
        handler(const xcb_generic_event_t * ev)
        {
            switch (ev->response_type & ~0x80) 
            {
                case XCB_KEY_PRESS: 
                {
                    key_press_handler(ev);
                    break;
                }
                case XCB_MAP_NOTIFY: 
                {
                    map_notify_handler(ev);
                    break;
                }
                case XCB_MAP_REQUEST: 
                {
                    map_req_handler(ev);
                    break;
                }
                case XCB_BUTTON_PRESS: 
                {
                    button_press_handler(ev);
                    break;
                }
                case XCB_CONFIGURE_REQUEST:
                {
                    configure_request_handler(ev);
                    break;
                }
                case XCB_FOCUS_IN:
                {;
                    focus_in_handler(ev);
                    break;
                }
                case XCB_FOCUS_OUT:
                {
                    focus_out_handler(ev);
                    break;
                }
                /** TODO ** 
                 *
                 * @brief Handle the 'XCB_CLIENT_MESSAGE' event.
                 *        This event is used to handle the 'close' button on the window.
                 *        When the 'close' button is pressed, the window is destroyed.
                 *        
                 * @param ev The XCB event to be handled.
                 *
                 * @note This event is not currently handled.
                 *       This needs to be checked and implemented.
                 *
                 */
                case XCB_CLIENT_MESSAGE: 
                {
                    break;
                }
                case XCB_DESTROY_NOTIFY:
                {
                    destroy_notify_handler(ev);
                    break;
                }
                case XCB_UNMAP_NOTIFY:
                {
                    unmap_notify_handler(ev);
                    break;
                }
                case XCB_REPARENT_NOTIFY:
                {
                    reparent_notify_handler(ev);
                    break;
                }
                case XCB_ENTER_NOTIFY:
                {
                    enter_notify_handler(ev);
                    break;
                }
                case XCB_LEAVE_NOTIFY:
                {
                    leave_notify_handler(ev);
                    break;
                }
                case XCB_MOTION_NOTIFY:
                {
                    motion_notify_handler(ev);
                    break;
                }
            }
        }
    ;

    private:
        xcb_key_symbols_t * keysyms;
        /*
            VARIABELS TO STORE KEYCODES
         */ 
        xcb_keycode_t t{}, q{}, f{}, f11{}, n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{}, l_arrow{}, u_arrow{}, d_arrow{}, tab{}, k{}; 
        
        void 
        key_press_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
            
            /*
                CHECK IF 'ALT+CTRL+T' WAS PRESSED
                AND IF SO LAUNCH TERMINAL   
             */         
            if (e->detail == t)  
            {
                switch (e->state) 
                {
                    case CTRL + ALT:
                    {
                        mxb::launch::program((char *) "/usr/bin/konsole");
                        break;
                    }
                }
            }

            /*
                CHECK IF 'ALT+SHIFT+Q' WAS PRESSED
                AND IF SO LAUNCH KILL_SESSION
             */ 
            if (e->detail == q) 
            {
                switch (e->state) 
                {
                    case SHIFT + ALT:
                    {
                        mxb::quit(0);
                        break;
                    }
                }
            }

            /*
                CHECK IF 'F11' WAS PRESSED
                AND IF SO TOGGLE FULLSCREEN
             */ 
            if (e->detail == f11)
            {
                client * c = get::client_from_win(& e->event);
                max_win(c, max_win::EWMH_MAXWIN);
            }

            /*
                CHECK IF 'ALT+1' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 1
             */
            if (e->detail == n_1) 
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

            /*
                CHECK IF 'ALT+2' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 2
             */ 
            if (e->detail == n_2) 
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

            /*
                CHECK IF 'ALT+3' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 3
             */ 
            if (e->detail == n_3) 
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

            /*
                CHECK IF 'ALT+4' WAS PRESSED
                AND IF SO MOVE TO DESKTOP 4
             */
            if (e->detail == n_4) 
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

            /*
                CHECK IF 'ALT+5' WAS PRESSED
                IF SO MOVE TO DESKTOP 5
             */
            if (e->detail == n_5) 
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

            /*
                IF R_ARROW IS PRESSED THEN CHECK WHAT MOD MASK IS APPLIED

                IF 'SHIFT+CTRL+SUPER' THEN MOVE TO NEXT DESKTOP WITH CURRENTLY FOCUSED APP
                IF 'CTRL+SUPER' THEN MOVE TO THE NEXT DESKTOP
                IF 'SUPER' THEN TILE WINDOW TO THE RIGHT
             */
            if (e->detail == r_arrow)
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
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::RIGHT);
                        break;
                    }
                    return;
                }
            }

            /*
                IF L_ARROW IS PRESSED THEN CHECK WHAT MOD MASK IS APPLIED

                IF 'SHIFT+CTRL+SUPER' THEN MOVE TO PREV DESKTOP WITH CURRENTLY FOCUSED APP
                IF 'CTRL+SUPER' THEN MOVE TO THE PREV DESKTOP
                IF 'SUPER' THEN TILE WINDOW TO THE LEFT
             */
            if (e->detail == l_arrow)
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
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::LEFT);
                        break;
                    }
                }
            }

            /*
                IF 'D_ARROW' IS PRESSED SEND TO TILE
             */
            if (e->detail == d_arrow)
            {
                switch (e->state) 
                {
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::DOWN);
                        return;
                        break;
                    }
                }
            }

            if (e->detail == u_arrow)
            {
                switch (e->state) 
                {
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        tile(c, TILE::UP);
                        break;
                    }
                }
            }

            /*
                CHECK IF 'ALT+TAB' WAS PRESSED
                IF SO CYCLE FOCUS
             */
            if (e->detail == tab) 
            {
                switch (e->state) 
                {
                    case ALT:
                    {
                        focus::cycle();
                        break;
                    }
                }
            }

            /**
             *
             * @brief This is a debug kebinding to test featurtes before they are implemented.  
             * 
             */
            if (e->detail == k)
            {
                switch (e->state) 
                {
                    case SUPER:
                    {
                        client * c = get::client_from_win(& e->event);
                        
                        // mxb::EWMH ewmhChecker(conn, ewmh);
                        // if (ewmhChecker.checkWindowDecorations(c->win))
                        // {
                        //     log_info("has decorations");
                        // }
                        // else
                        // {
                        //     log_info("does not have decorations");
                        // }

                        // if (ewmhChecker.checkWindowFrameExtents(c->win))
                        // {
                        //     log_info("has frame extents");
                        // }
                        // else
                        // {
                        //     log_info("does not have frame extents");
                        // }

                        // mxb::get::win::property(c->win, "WM_CLASS"); // works
                        // mxb::get::win::property(c->win, "WM_NAME"); //works
                        // mxb::get::win::property(c->win, "WM_PROTOCOLS"); // works
                        // mxb::get::win::property(c->win, "WM_HINTS"); // works
                        // mxb::get::win::property(c->win, "WM_NORMAL_HINTS"); // works
                        // mxb::get::win::property(c->win, "_NET_WM_NAME"); // works, gives (WM_NAME WM_CLASS) 
                        // mxb::get::win::property(c->win, "_NET_WM_WINDOW_TYPE"); // works
                        // mxb::get::win::property(c->win, "_NET_WM_PID"); // works, needs conversion to pid_t or other integer type 
                        // mxb::get::win::property(c->win, "_NET_WM_USER_TIME"); // works, needs conversion to int or other integer type

                        // if (mxb::EWMH::check::isWindowNormalType(c->win))
                        // {
                        //     log_info("is normal type");
                        // }
                        // else
                        // {
                        //     log_info("is not normal type");
                        // }

                        // log_info(mxb::get::event_mask(mxb::get::event_mask_sum(c->win)));
                        break;
                    }
                }
            }
        }

        void /*
            INITIALIZES KEYBOARD KEY SYMBOLS AND STORES 
            THE KEYCODES UNTIL SESSION IS KILLED 
         */
        initialize_keysyms() 
        {
            keysyms = xcb_key_symbols_alloc(conn);
            if (keysyms) 
            {
                xcb_keycode_t * t_keycodes          = xcb_key_symbols_get_keycode(keysyms, T);
                xcb_keycode_t * q_keycodes          = xcb_key_symbols_get_keycode(keysyms, Q);
                xcb_keycode_t * f_keycodes          = xcb_key_symbols_get_keycode(keysyms, F);
                xcb_keycode_t * f11_keycodes        = xcb_key_symbols_get_keycode(keysyms, F11);
                xcb_keycode_t * n_1_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_1);
                xcb_keycode_t * n_2_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_2);
                xcb_keycode_t * n_3_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_3);
                xcb_keycode_t * n_4_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_4);
                xcb_keycode_t * n_5_keycodes        = xcb_key_symbols_get_keycode(keysyms, N_5);
                xcb_keycode_t * r_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, R_ARROW);
                xcb_keycode_t * l_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, L_ARROW);
                xcb_keycode_t * u_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, U_ARROW);
                xcb_keycode_t * d_arrow_keycodes    = xcb_key_symbols_get_keycode(keysyms, D_ARROW);
                xcb_keycode_t * tab_keycodes        = xcb_key_symbols_get_keycode(keysyms, TAB);
                xcb_keycode_t * k_keycodes			= xcb_key_symbols_get_keycode(keysyms, K);
                
                if (t_keycodes) 
                {
                    t = * t_keycodes;
                    free(t_keycodes);
                }
                
                if (q_keycodes) 
                {
                    q = * q_keycodes;
                    free(q_keycodes);
                }
                
                if (f_keycodes)
                {
                    f = * f_keycodes;
                    free(f_keycodes);
                }

                if (f11_keycodes)
                {
                    f11 = * f11_keycodes;
                    free(f11_keycodes);
                }

                if (n_1_keycodes)
                {
                    n_1 = * n_1_keycodes;
                    free(n_1_keycodes);
                }

                if (n_2_keycodes)
                {
                    n_2 = * n_2_keycodes;
                    free(n_2_keycodes);
                }

                if (n_3_keycodes)
                {
                    n_3 = * n_3_keycodes;
                    free(n_3_keycodes);
                }

                if (n_4_keycodes)
                {
                    n_4 = * n_4_keycodes;
                    free(n_4_keycodes);
                }

                if (n_5_keycodes)
                {
                    n_5 = * n_5_keycodes;
                    free(n_5_keycodes);
                }

                if (r_arrow_keycodes)
                {
                    r_arrow = * r_arrow_keycodes;
                    free(r_arrow_keycodes);
                }

                if (l_arrow_keycodes)
                {
                    l_arrow = * l_arrow_keycodes;
                    free(l_arrow_keycodes);
                }

                if (u_arrow_keycodes)
                {
                    u_arrow = * u_arrow_keycodes;
                    free(u_arrow_keycodes);
                }

                if (d_arrow_keycodes)
                {
                    d_arrow = * d_arrow_keycodes;
                    free(d_arrow_keycodes);
                }

                if (tab_keycodes)
                {
                    tab = * tab_keycodes;
                    free(tab_keycodes);
                }

                if (k_keycodes)
				{
					k = * k_keycodes;
					free(k_keycodes);
				}
            }
        }
        
        void 
        map_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_map_notify_event_t *>(ev);
            client * c = get::client_from_win(& e->window);
            if (c)
            {
                mxb::Client::update(c);
            }
        }
        
        void 
        map_req_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_map_request_event_t *>(ev);
            WinManager::manage_new_window(e->window);
        }
        
        void 
        button_press_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
            client * c;

            if (BORDER_SIZE == 0)
            {
                c = mxb::Client::calc::client_edge_prox_to_pointer(10);
                if (c)
                {
                    if (e->detail == L_MOUSE_BUTTON)
                    {
                        mxb::Client::raise(c);
                        resize_client::no_border(c, 0, 0);
                        focus::client(c);
                    }
                    return;
                }
            }
            
            if (e->detail == L_MOUSE_BUTTON)
            {
                dock->button_press_handler(e->event);
            }
            
            if (e->event == dock->main_window)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    dock->context_menu.show();
                    return;
                }    
            }

            if (e->event == screen->root)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    mxb::Dialog_win::context_menu main_context_menu;
                    main_context_menu.addEntry
                    ("konsole",
                    []() 
                    {
                        mxb::launch::program((char *) "/usr/bin/konsole");
                    });
                    main_context_menu.show();
                    return;
                }
            }

            c = get::client_from_all_win(& e->event);
            if (!c)
            {
                // log_error("c == null");
                return;
            }
            
            if (e->detail == L_MOUSE_BUTTON)
            {
                if (e->event == c->win)
                {
                    switch (e->state) 
                    {
                        case ALT:
                        {
                            // log_info("ALT + L_MOUSE_BUTTON + win");
                            mxb::Client::raise(c);
                            mv_client(c, e->event_x, e->event_y + 20);
                            focus::client(c);
                            break;
                        }
                    }
                    // log_info("L_MOUSE_BUTTON + win");
                    mxb::Client::raise(c);
                    focus::client(c);
                    return;
                }

                if (e->event == c->titlebar)
                {
                    // log_info("L_MOUSE_BUTTON + titlebar");
                    mxb::Client::raise(c);
                    mv_client(c, e->event_x, e->event_y);
                    focus::client(c);
                    return;
                }

                if (e->event == c->close_button)
                {
                    // log_info("L_MOUSE_BUTTON + close_button");
                    win_tools::close_button_kill(c);
                    return;
                }

                if (e->event == c->max_button)
                {
                    // log_info("L_MOUSE_BUTTON + max_button");
                    client * c = get::client_from_all_win(& e->event);
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
                        mxb::Client::raise(c);
                        resize_client(c, 0);
                        focus::client(c);
                        return;
                    }
                }
            }
        }

        void
        configure_request_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_configure_request_event_t *>(ev);
            data.width     = e->width;
            data.height    = e->height;
            data.x         = e->x;
            data.y         = e->y;
        }

        void
        focus_in_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_in_event_t *>(ev);
            client * c = get::client_from_win( & e->event);
            if (c)
            {
                mxb::win::ungrab::button(c->win, L_MOUSE_BUTTON, 0);
                mxb::Client::raise(c);
                mxb::EWMH::set::active_window(c->win);
                focused_client = c;
            }
        }

        void
        focus_out_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_out_event_t *>(ev);
            
            client * c = get::client_from_win(& e->event);
            if (!c)
            {
                return;
            }
            
            if (c == focused_client)
            {
                return;
            }
            
            mxb::win::grab::button(c->win, L_MOUSE_BUTTON, 0);
        }

        void
        destroy_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_destroy_notify_event_t *>(ev);

            log_win("e->window: ", e->window);
            log_win("e->event: ", e->event);
            client * c = get::client_from_all_win(& e->window);
        
            int result = win_tools::send_sigterm_to_client(c);
            if (result == -1)
            {
                log_error("send_sigterm_to_client: failed");
            }
        }

        void
        unmap_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_unmap_notify_event_t *>(ev);
            
            client * c = get::client_from_win(& e->window);
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

        void 
        reparent_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_reparent_notify_event_t *>(ev);
        }
        
        void
        enter_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
            log_win("e->event: ", e->event);
        }

        void
        leave_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_leave_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
        }

        void 
        motion_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
            // log_win("e->event: ", e->event);
            // log_info(e->event_x);
        }
    ;
};

void /**
 * THIS IS THE MAIN EVENT LOOP
 * FOR THE 'WINDOW_MANAGER' 
 */
run() 
{
    Event event;
    while (true)
    {
        xcb_generic_event_t * ev = xcb_wait_for_event(conn);
        if (!ev) 
        {
            continue;
        }
        event.handler(ev);
        free(ev);
    }
}

void 
configureRootWindow()
{
    // SET THE ROOT WINDOW BACKROUND COLOR TO 'DARK_GREY'(0x222222, THE DEFAULT COLOR) SO THAT IF SETTING THE PNG AS BACKROUND FAILS THE ROOT WINDOW WILL BE THE DEFAULT COLOR
    mxb::set::win::backround::as_color(screen->root, DARK_GREY);

    uint32_t mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   |
                    XCB_EVENT_MASK_ENTER_WINDOW          |
                    XCB_EVENT_MASK_LEAVE_WINDOW          |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY      |
                    XCB_EVENT_MASK_BUTTON_PRESS          |
                    XCB_EVENT_MASK_BUTTON_RELEASE        |
                    XCB_EVENT_MASK_KEY_PRESS             |
                    XCB_EVENT_MASK_FOCUS_CHANGE          |
                    XCB_EVENT_MASK_KEY_RELEASE           |
                    XCB_EVENT_MASK_POINTER_MOTION
    ;
    mxb::set::event_mask(& mask, screen->root);
    mxb::win::clear(screen->root);

    // FLUSH TO MAKE X SERVER HANDEL REQUEST NOW
    xcb_flush(conn);
}

bool 
setSubstructureRedirectMask() 
{
    // ATTEMPT TO SET THE SUBSTRUCTURE REDIRECT MASK
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked
    (
        conn,
        screen->root,
        XCB_CW_EVENT_MASK,
        (const uint32_t[1])
        {
            XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        }
    );

    // CHECK IF ANOTHER WINDOW MANAGER IS RUNNING
    xcb_generic_error_t * error = xcb_request_check(conn, cookie);
    if (error) 
    {
        log_error("Error: Another window manager is already running or failed to set SubstructureRedirect mask."); 
        free(error);
        return false;
    }
    return true;
}

int
start_screen_window()
{
    start_win = xcb_generate_id(conn);
    xcb_create_window
    (
        conn,
        XCB_COPY_FROM_PARENT,
        start_win,
        screen->root,
        0,
        0,
        screen->width_in_pixels,
        screen->height_in_pixels,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        0,
        nullptr
    );

    mxb::set::win::backround::as_color(start_win, DARK_GREY);
    xcb_map_window(conn, start_win);
    xcb_flush(conn);
    return 0;
}

void
setup_wm()
{
    mxb::set::_conn(nullptr, nullptr);
    mxb::set::_setup();
    mxb::set::_iter();
    mxb::set::_screen();
    setSubstructureRedirectMask();
    configureRootWindow();
    mxb::pointer::set(screen->root, CURSOR::arrow);

    mxb::set::_ewmh();

    /* 
        MAKE ('5') DESKTOPS 
        
        THIS IS JUST FOR 
        DEBUGING AND TESTING 
        IN THE FUTURE I WILL IMPLEMENT
        A WAY TO ADD AND REMOVE 
        DESKTOPS DURING RUNTIME
     */
    mxb::create::new_desktop(1);
    mxb::create::new_desktop(2);
    mxb::create::new_desktop(3);
    mxb::create::new_desktop(4);
    mxb::create::new_desktop(5);
    
    change_desktop::teleport_to(1);

    mxb::set::win::backround::as_png("/home/mellw/mwm_png/galaxy17.png", screen->root);
    
    dock = new mxb::Dialog_win::Dock;
    dock->add_app("konsole");
    dock->add_app("falkon");
    dock->init();
}

int
main() 
{
    LOG_start()
    setup_wm();
    run();
    xcb_disconnect(conn);
    return 0;
}