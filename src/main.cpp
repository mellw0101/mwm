#include <cstdlib>
#include <features.h>
#include <iterator>
#include <regex>
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
#include <xcb/xinput.h>
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
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <cstdint>
#include <algorithm> // For std::remove_if
#include <xcb/xcb.h>
#include <functional>
#include <unordered_map>
#include <xcb/xproto.h>
#include <filesystem>
#include <iwlib.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pulse/pulseaudio.h>
#include <type_traits>


#include "Log.hpp"
Logger logger;
#include "defenitions.hpp"
#include "structs.hpp"

static xcb_connection_t * conn;
static xcb_ewmh_connection_t * ewmh; 
static const xcb_setup_t * setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t * screen;

#define DEFAULT_FONT "7x14"
#define DEFAULT_FONT_WIDTH 7
#define DEFAULT_FONT_HEIGHT 14

using namespace std;
using Uint = unsigned int;
using SUint = unsigned short int;

#define NET_DEBUG false

#define FRAME_EVENT_MASK \
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   | \
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | \
    XCB_EVENT_MASK_POINTER_MOTION        | \
    XCB_EVENT_MASK_FOCUS_CHANGE          | \
    XCB_EVENT_MASK_ENTER_WINDOW          | \
    XCB_EVENT_MASK_PROPERTY_CHANGE

#define CLIENT_EVENT_MASK \
    XCB_EVENT_MASK_STRUCTURE_NOTIFY | \
    XCB_EVENT_MASK_FOCUS_CHANGE     | \
    XCB_EVENT_MASK_PROPERTY_CHANGE

/*
#define BUTTON_EVENT_MASK \
    EnterWindowMask | \
    LeaveWindowMask

#define ROOT_EVENT_MASK \
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | \
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   | \
    XCB_EVENT_MASK_STRUCTURE_NOTIFY      | \
    XCB_EVENT_MASK_BUTTON_PRESS          | \
    XCB_EVENT_MASK_FOCUS_CHANGE 
*/

/**
 * @brief General purpose definition to reinterpret cast.
 */
#define RE_CAST(__type, __from) \
    reinterpret_cast<__type>(__from)

/**
 * @brief Specialized definition to reinterpret a const xcb_generic_event_t *
 * into a const __type * (the event that 'e' should represent) with the name 'e' for use in event handling.
 */
#define RE_CAST_EV(__type) \
    auto e = RE_CAST(const __type *, ev)

/*
 * @breif Defenition to set an event callback to the global event handler
 * from within the class that holds the internal client
 * as this way it can be properly cleaned when killed
 */
#define SET_INTR_CLI_KILL_CALLBACK() \
    event_handler->setEventCallback(XCB_CLIENT_MESSAGE, [this](Ev ev)-> void \
    {                                                                        \
        RE_CAST_EV(xcb_client_message_event_t);                              \
        if (c == nullptr) return;                                            \
        if (e->window == c->win)                                             \
        {                                                                    \
            if (e->format == 32)                                             \
            {                                                                \
                xcb_atom_t atom;                                             \
                wm->get_atom((char *)"WM_DELETE_WINDOW", &atom);             \
                if (e->data.data32[0] == atom)                               \
                {                                                            \
                    c->kill();                                               \
                    delete c;                                                \
                    c = nullptr;                                             \
                    wm->remove_client(c);                                    \
                }                                                            \
            }                                                                \
        }                                                                    \
    })

#define RETURN_IF(__statement) \
    if (__statement)           \
    {                          \
        return;                \
    }

#define CONTINUE_IF(__statement) \
    if (__statement)             \
    {                            \
        continue;                \
    }

#define GET_CLIENT_FROM_WINDOW(__window) \
    client *c = wm->client_from_any_window(&__window); \
    RETURN_IF(c == nullptr)

constexpr const char * pointer_from_enum(CURSOR CURSOR)
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

class __net_logger__
{
    // Defines.
        #define ESP_SERVER "192.168.0.29"
        #define ESP_PORT 23

        #define NET_LOG_MSG(message)                "\033[33m" + message + "\033[0m"
        #define FUNC_NAME_STR                       string(__func__)
        #define NET_LOG_FUNCTION                    "\033[34m(FUNCTION) " + FUNC_NAME_STR + "\033[0m"
        #define NET_LOG_WINDOW(window_name, window) "\033[35m(FUNCTION) " + FUNC_NAME_STR + ":\033[34m (WINDOW_NAME) " + window_name + ":\033[32m (uint32_t) " + to_string(window) + "\033[0m"
        #define WINDOW(window)                      to_string(window)
        #define CLASS_NAME_RAW                      typeid(*this).name()
        #define CLASS_NAME_STR                      string(CLASS_NAME_RAW)
        /*
        *
        * @breif fetches the calling class's name and extracts a substr containing only the class name
        *
        * NOTE: this only works for class's with prefix '__'
        *
        */
        #define CLASS_NAME                          CLASS_NAME_STR.substr(CLASS_NAME_STR.find("__"))
        #define NET_LOG_CLASS                       "\033[35m(CLASS) " + CLASS_NAME + "\033[0m"
        #define NET_LOG(__type)                     net_logger->send_to_server(__type)
        #define NET_LOG_CLASS_FUNCTION_START()      NET_LOG(NET_LOG_CLASS + " -> " + NET_LOG_FUNCTION + " -> " + NET_LOG_MSG("Starting"))
        #define NET_LOG_CLASS_FUNCTION_DONE()       NET_LOG(NET_LOG_CLASS + " -> " + NET_LOG_FUNCTION + " -> " + NET_LOG_MSG("Done!!!"))

    private:
    // Variabels.
        long _socket;
        int _connected;
        struct sockaddr_in(_sock_addr);

    public:
    // Methods.
        void init(const char *__address, const int &__port = 0)
        {
            if (NET_DEBUG == false) return;

            if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            {
                perror("socket");
                return;
            }

            _sock_addr.sin_family = AF_INET;
            if (__address == string(ESP_SERVER))
            {
                _sock_addr.sin_port = htons(ESP_PORT);
            }
            else
            {
                _sock_addr.sin_port = htons(__port);
            }

            if (inet_pton(AF_INET, __address, &_sock_addr.sin_addr) < 0)
            {
                perror("inet_pton");
                return;
            }

            if (connect(_socket, (struct sockaddr*)&_sock_addr, sizeof(_sock_addr)) < 0)
            {
                perror("connect");
                return;
            }

            _connected = true;
        }

        void send_to_server(const string &__input)
        {
            if (NET_DEBUG == false) return;

            if (!_connected) return;

            if (send(_socket, __input.c_str(), __input.length(), 0) < 0)
            {
                _connected = false;
                return;
            }

            char s_char('\0');
            if (send(_socket, &s_char, 1, 0) < 0)
            {
                _connected = false;
                return;
            }
        }

    // Constructor.
        __net_logger__()
        : _connected(false) {}
};
static __net_logger__ *net_logger(nullptr);

struct size_pos
{
    int16_t x, y;
    uint16_t width, height;

    void save(const int & x, const int & y, const int & width, const int & height)
    {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
    }
};

class mxb
{
    class XConnection
    {
        public: // constructor and destructor
            
            struct mxb_auth_info_t {
                int namelen;
                char* name;
                int datalen;
                char* data;
            };

            XConnection(const char * display) {
                string socketPath = getSocketPath(display);
                memset(&addr, 0, sizeof(addr)); // Initialize address
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
                fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create socket 
                if(fd == -1) {
                    throw runtime_error("Failed to create socket");
                }

                if(connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) { // Connect to the X server's socket
                    ::close(fd);
                    throw
                    std::runtime_error("Failed to connect to X server");
                }
                int displayNumber = parseDisplayNumber(display); // Perform authentication 
                if(!authenticate_x11_connection(displayNumber, auth_info)) {
                    ::close(fd);
                    throw runtime_error("Failed to authenticate with X server");
                }
            }
            
            ~XConnection() {
                if(fd != -1) {
                    ::close(fd);
                }

                delete[] auth_info.name;
                delete[] auth_info.data;
            }

        // Methods
            int getFd() const
            {
                return fd;
            }

            void confirmConnection()
            {
                const string extensionName = "BIG-REQUESTS";  // Example extension
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
                memcpy(&request[8], extensionName.c_str(), nameLength); // Copy the extension name

                /* Send the request */
                if (send(fd, request, 8 + nameLength, 0) == -1)
                {
                    throw std::runtime_error("Failed to send QueryExtension request");
                }

                /* Prepare to receive the response */ 
                char reply[32] = {0}; int received = 0;

                // Read the response from the server
                while (received < sizeof(reply))
                {
                    int n = recv(fd, reply + received, sizeof(reply) - received, 0);
                    if (n == -1)
                    {
                        throw runtime_error("Failed to receive QueryExtension reply");
                    }
                    else if (n == 0)
                    {
                        throw runtime_error("Connection closed by X server");
                    }

                    received += n;
                }

                if (reply[0] != 1)
                {
                    throw std::runtime_error("Invalid response received from X server");
                }
                
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
            
            string sendMessage(const string &extensionName)
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

                /* Send the request */ if (send(fd, request, 8 + nameLength, 0) == -1)
                throw runtime_error("Failed to send QueryExtension request");
                /* Prepare to receive the response */ char reply[32] = {0}; int received = 0;
                /* Read the response from the server */ while(received < sizeof(reply)) {
                int n = recv(fd, reply + received, sizeof(reply) - received, 0);
                if(n == -1)
                    throw runtime_error("Failed to receive QueryExtension reply");
                else if(n == 0)
                    throw runtime_error("Connection closed by X server");
                received += n; }
                if (reply[0] != 1) {
                    throw runtime_error("Invalid response received from X server");
                }
                
                bool extensionPresent = reply[1]; 
                // Check if the extension is present
                if(extensionPresent) {
                    return("Extension is supported by the X server."); 
                } else {
                    return "Extension is not supported by the X server.";
                }
            }

        private:
            int(fd);
            struct sockaddr_un(addr);
            mxb_auth_info_t(auth_info);
            Logger(log);
            
            bool authenticate_x11_connection(int display_number, mxb_auth_info_t & auth_info) {
                const char* xauthority_env = std::getenv("XAUTHORITY"); 
                string xauthority_file = xauthority_env ? xauthority_env : "~/.Xauthority";

                FILE* auth_file = fopen(xauthority_file.c_str(), "rb");
                if(!auth_file) {
                    return false;
                }
                    
                Xauth* xauth_entry;
                bool found = false;
                while((xauth_entry = XauReadAuth(auth_file)) != nullptr) {
                    // Check if the entry matches your display number
                    // Assuming display_number is the display you're interested in
                    if(to_string(display_number) == string(xauth_entry->number, xauth_entry->number_length)) {
                        auth_info.namelen = xauth_entry->name_length;
                        auth_info.name = new char[xauth_entry->name_length];
                        memcpy(auth_info.name, xauth_entry->name, xauth_entry->name_length);

                        auth_info.datalen = xauth_entry->data_length;
                        auth_info.data = new char[xauth_entry->data_length];
                        memcpy(auth_info.data, xauth_entry->data, xauth_entry->data_length);

                        found = true;
                        XauDisposeAuth(xauth_entry);
                        break;
                    }

                    XauDisposeAuth(xauth_entry);
                }

                fclose(auth_file);
                return found;
            }

            string getSocketPath(const char * display) {
                string displayStr;
                if(display == nullptr) {
                    char* envDisplay = getenv("DISPLAY");
                    
                    if(envDisplay != nullptr) {
                        displayStr = envDisplay;
                    } else {
                        displayStr = ":0";
                    }

                } else {
                    displayStr = display;
                }
                
                int displayNumber = 0;
                size_t colonPos = displayStr.find(':'); // Extract the display number from the display string
                if(colonPos != string::npos) {
                    displayNumber = std::stoi(displayStr.substr(colonPos + 1));
                }
                
                return ("/tmp/.X11-unix/X" + std::to_string(displayNumber));
            }

            int parseDisplayNumber(const char * display) {
                if(!display) {
                    display = getenv("DISPLAY");
                }
                
                if(!display) {
                    return 0;  // default to display 0
                }
                
                const string displayStr = display;
                size_t colonPos = displayStr.find(':');
                if(colonPos != string::npos) {
                    return std::stoi(displayStr.substr(colonPos + 1));
                }

                return 0;
            }
    };

    static XConnection * mxb_connect(const char* display)
    {
        try
        {
            return (new XConnection(display));
        }
        catch (const exception & e)
        {
            cerr << "Connection error: " << e.what() << endl;
            return nullptr; 
        }
    }

    static int mxb_connection_has_error(XConnection* conn)
    {
        try
        {
            string response = conn->sendMessage("BIG-REQUESTS");
            log_info(response);
        }
        catch (const exception &e)
        {
            log_error(e.what());
            return 1;
        }

        return 0;
    };
};

class pointer
{
    public:
    // Methods.
        uint32_t x()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply)
            {
                log_error("reply is nullptr.");
                return 0;
            }

            uint32_t x = reply->root_x;
            free(reply);
            return x;
        }

        uint32_t y()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply)
            {
                log_error("reply is nullptr.");
                return 0;
            }

            uint32_t y = reply->root_y;
            free(reply);
            return y;
        }

        void teleport(const int16_t &x, const int16_t &y)
        {
            xcb_warp_pointer(conn, XCB_NONE, screen->root, 0, 0, 0, 0, x, y);
            xcb_flush(conn);
        }

        void grab()
        {
            xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(
                conn,
                false,
                screen->root,
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
                free(reply); return;
            }

            if (reply->status != XCB_GRAB_STATUS_SUCCESS)
            {
                log_error("Could not grab pointer"); 
                free(reply); return;
            }

            free(reply);
        }

        void ungrab()
        {
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            xcb_flush(conn);
        }

        const char * pointer_from_enum(CURSOR CURSOR)
        {
            switch (CURSOR)
            {
                case CURSOR::arrow               : return("arrow");
                case CURSOR::hand1               : return("hand1");
                case CURSOR::hand2               : return("hand2");
                case CURSOR::watch               : return("watch");
                case CURSOR::xterm               : return("xterm");
                case CURSOR::cross               : return("cross");
                case CURSOR::left_ptr            : return("left_ptr");
                case CURSOR::right_ptr           : return("right_ptr");
                case CURSOR::center_ptr          : return("center_ptr");
                case CURSOR::sb_v_double_arrow   : return("sb_v_double_arrow");
                case CURSOR::sb_h_double_arrow   : return("sb_h_double_arrow");
                case CURSOR::fleur               : return("fleur");
                case CURSOR::question_arrow      : return("question_arrow");
                case CURSOR::pirate              : return("pirate");
                case CURSOR::coffee_mug          : return("coffee_mug");
                case CURSOR::umbrella            : return("umbrella");
                case CURSOR::circle              : return("circle");
                case CURSOR::xsb_left_arrow      : return("xsb_left_arrow");
                case CURSOR::xsb_right_arrow     : return("xsb_right_arrow");
                case CURSOR::xsb_up_arrow        : return("xsb_up_arrow");
                case CURSOR::xsb_down_arrow      : return("xsb_down_arrow");
                case CURSOR::top_left_corner     : return("top_left_corner");
                case CURSOR::top_right_corner    : return("top_right_corner");
                case CURSOR::bottom_left_corner  : return("bottom_left_corner");
                case CURSOR::bottom_right_corner : return("bottom_right_corner");
                case CURSOR::sb_left_arrow       : return("sb_left_arrow");
                case CURSOR::sb_right_arrow      : return("sb_right_arrow");
                case CURSOR::sb_up_arrow         : return("sb_up_arrow");
                case CURSOR::sb_down_arrow       : return("sb_down_arrow");
                case CURSOR::top_side            : return("top_side");
                case CURSOR::bottom_side         : return("bottom_side");
                case CURSOR::left_side           : return("left_side");
                case CURSOR::right_side          : return("right_side");
                case CURSOR::top_tee             : return("top_tee");
                case CURSOR::bottom_tee          : return("bottom_tee");
                case CURSOR::left_tee            : return("left_tee");
                case CURSOR::right_tee           : return("right_tee");
                case CURSOR::top_left_arrow      : return("top_left_arrow");
                case CURSOR::top_right_arrow     : return("top_right_arrow");
                case CURSOR::bottom_left_arrow   : return("bottom_left_arrow");
                case CURSOR::bottom_right_arrow  : return("bottom_right_arrow");
                default                          : return("left_ptr");
            }
        }

        void center_pointer()
        {
            grab();
            int16_t x = (screen->width_in_pixels / 2);
            int16_t y = (screen->height_in_pixels / 2);
            teleport(x, y);
            // xcb_warp_pointer(conn, XCB_NONE, screen->root, 0, 0, 0, 0, x, y);
            ungrab();
        }

    private:
    // Variabels.
        Logger(log);
};

class fast_vector
{
    public:
    // Operators.
        operator vector<const char*>() const
        {
            return data;
        }

        const char* operator[](size_t index) const
        {
            return data[index];
        }

    // Destuctor.
        ~fast_vector()
        {
            for (auto str:data) 
            delete[] str;
        }

    // Methods.
        void push_back(const char* str)
        {
            char* copy = new char[strlen(str) + 1];
            strcpy(copy, str);
            data.push_back(copy);
        }

        void append(const char* str)
        {
            char* copy = new char[strlen(str) + 1];
            strcpy(copy, str);
            data.push_back(copy);
        }

        size_t size() const
        {
            return data.size();
        }

        size_t index_size() const
        {
            if(data.size() == 0) {
                return(0);
            }
            return(data.size() - 1);
        }

        void clear()
        {
            data.clear();
        }

    private:
    // Variabels.
        vector<const char*>(data);
};

class string_tokenizer
{
    public:
    // Constructors and Destructor.
        string_tokenizer() {}

        string_tokenizer(const char* input, const char* delimiter)
        {
            // Copy the input string
            str = new char[strlen(input) + 1];
            strcpy(str, input);

            // Tokenize the string using strtok() and push tokens to the vector
            char* token = strtok(str, delimiter);
            while (token != nullptr) {
                tokens.push_back(token);
                token = strtok(nullptr, delimiter);
            }
        }
        ~string_tokenizer()
        {
            delete[] str;
        }

    // Methods.
        const fast_vector & tokenize(const char* input, const char* delimiter)
        {
            tokens.clear();

            str = new char[strlen(input) + 1];
            strcpy(str, input); // Copy the input string

            char* token = strtok(str, delimiter); // Tokenize the string using strtok() and push tokens to the vector
            while (token != nullptr) {
                tokens.append(token);
                token = strtok(nullptr, delimiter);
            }
            return tokens;
        }

        const fast_vector & get_tokens() const
        {
            return tokens;
        }

        void clear()
        {
            tokens.clear();
        }

    private:
    // Variables.
        char* str;
        fast_vector tokens;
};

class str
{
    public:
    // Constructors and Destructor.
        str(const char* str = "")
        {
            length = strlen(str);
            data = new char[length + 1];
            strcpy(data, str);
        }

        str(const str& other)
        {
            length = other.length;
            data = new char[length + 1];
            strcpy(data, other.data);
        }

        str(str&& other) noexcept 
        : data(other.data), length(other.length)
        {
            other.data = nullptr;
            other.length = 0;
        }

        ~str()
        {
            delete[] data;
        }

    // Operators.
        str& operator=(const str& other)
        {
            if (this != &other)
            {
                delete[] data;
                length = other.length;
                data = new char[length + 1];
                strcpy(data, other.data);
            }

            return *this;
        }

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

        str operator+(const str& other) const
        {
            str result;
            result.length = length + other.length;
            result.data = new char[result.length + 1];
            strcpy(result.data, data);
            strcat(result.data, other.data);
            return result;
        }

    // methods
        const char * c_str() const // Access to underlying C-string
        {
            return data;
        }        

        size_t size() const // Get the length of the string
        {
            return length;
        }

        bool isEmpty() const // Method to check if the string is empty
        {
            return length == 0;
        }

        bool is_nullptr() const
        {
            if (data == nullptr)
            {
                delete [] data;
                return true;
            }

            return false;
        }

    private:
    // Variables
        char* data;
        size_t length;
        bool is_null = false;
};

class fast_str_vector
{
    public:
    // operators.
        operator vector<str>() const
        {
            return data;
        }

        str operator[](size_t index) const // [] operator Access an element in the vector
        {
            return data[index];
        }

    // methods.
        void push_back(str str) // Add a string to the vector
        {
            data.push_back(str);
        }

        void append(str str) // Add a string to the vector
        {
            data.push_back(str);
        }

        size_t size() const // Get the size of the vector
        {
            return data.size();
        }

        size_t index_size() const // get the index of the last element in the vector
        {
            if (data.size() == 0)
            {
                return 0;
            }

            return data.size() - 1;
        }

        void clear() // Clear the vector
        {
            data.clear();
        }
    
    private:
    // variabels.
        vector<str>(data); // Internal vector to store const char* strings
};

class Directory_Searcher
{
    public:
    // Construtor.
        Directory_Searcher() {}
    
    // Methods.
        void search(const vector<const char *> &directories, const string &searchString)
        {
            results.clear();
            searchDirectories = directories;

            for (const auto &dir : searchDirectories)
            {
                DIR *d = opendir(dir);
                if (d == nullptr)
                {
                    log_error("opendir() failed for directory: " + std::string(dir));
                    continue;
                }

                struct dirent *entry;
                while ((entry = readdir(d)) != nullptr)
                {
                    string fileName = entry->d_name;
                    if (fileName.find(searchString) != std::string::npos)
                    {
                        bool already_found = false;
                        for (const auto &result : results)
                        {
                            if (result == fileName)
                            {
                                already_found = true;
                                break;
                            }
                        }
                        
                        if (!already_found)
                        {
                            results.push_back(fileName);
                        }
                    }
                }

                closedir(d);
            }
        }

        const vector<string> &getResults() const
        {
            return results;
        }
    
    private:
    // Variabels.
        vector<const char *>(searchDirectories);
        vector<string>(results);
        Logger log;
};

class Directory_Lister
{
    public:
    // Constructor.
        Directory_Lister() {}
    
    // Methods.
        vector<string> list(const string &Directory)
        {
            vector<string> results;
            DIR *d = opendir(Directory.c_str());
            if (d == nullptr)
            {
                log_error("Failed to open Directory: " + Directory);
                return results;
            }

            struct dirent *entry;
            while ((entry = readdir(d)) != nullptr)
            {
                std::string fileName = entry->d_name;
                results.push_back(fileName);
            }

            closedir(d);
            return results;   
        }

    private:
    // Variabels.
        Logger log;
};

class File
{
    public:
    // Sub Classes.
        class search
        {
            public: // construcers and operators
                search(const string &filename)
                : file(filename) {}

                operator bool() const
                {
                    return file.good();
                }
            
            private: // private variables
                ifstream file;
        };
    
    // Constructor.
        File() {}
    
    // Variabels.
        Directory_Lister directory_lister;
    
    // Methods.
        string find_png_icon(vector<const char *> dirs, const char *app)
        {
            std::string name = app;
            name += ".png";

            for (const auto &dir : dirs)
            {
                vector<string> files = list_dir_to_vector(dir);
                for (const auto &file : files)
                {
                    if (file == name)
                    {
                        return dir + file;
                    }
                }
            }

            return "";
        }
        
        string findPngFile(const vector<const char *> &__dirs, const char *__name)
        {
            for (const auto &dir : __dirs)
            {
                if (filesystem::is_directory(dir))
                {
                    for (const auto &entry : filesystem::directory_iterator(dir))
                    {
                        if (entry.is_regular_file())
                        {
                            string filename = entry.path().filename().string();
                            if (filename.find(__name) != string::npos && filename.find(".png") != string::npos)
                            {
                                return entry.path().string();
                            }                
                        }
                    }
                }
            }

            return "";
        }
        
        bool check_if_binary_exists(const char *name)
        {
            vector<const char *> dirs = split_$PATH_into_vector();
            return check_if_file_exists_in_DIRS(dirs, name);
        }
        
        vector<string> search_for_binary(const char *name)
        {
            ds.search({ "/usr/bin" }, name);
            return ds.getResults();
        }
        
        string get_current_directory()
        {
            return get_env_var("PWD");
        }
    
    private:
    // Variables.
        Logger log;
        string_tokenizer st;
        Directory_Searcher ds;
    
    // Methods.
        bool check_if_file_exists_in_DIRS(std::vector<const char *> dirs, const char *app)
        {
            string name = app;
            for (const auto &dir : dirs)
            {
                vector<string> files = list_dir_to_vector(dir);
                for (const auto &file : files)
                {
                    if (file == name)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        vector<string> list_dir_to_vector(const char *directoryPath)
        {
            vector<string> files;
            DIR* dirp = opendir(directoryPath);
            if (dirp)
            {
                struct dirent* dp;
                while ((dp = readdir(dirp)) != nullptr)
                {
                    files.push_back(dp->d_name);
                }
                closedir(dirp);
            }

            return files;
        }
        
        vector<const char *> split_$PATH_into_vector()
        {
            str $PATH(get_env_var("PATH"));
            return st.tokenize($PATH.c_str(), ":");
        }

        const char * get_env_var(const char *var)
        {
            const char *_var = getenv(var);
            if (_var == nullptr)
            {
                return nullptr;
            }

            return _var;
        }  
};

class Launcher
{
    public:
    // Methods.
        void program(char *program)
        {
            if (!file.check_if_binary_exists(program))
            {
                return;
            }

            if (fork() == 0)
            {
                setsid();
                execvp(program, (char *[]) { program, NULL });
            }
        }
    
    private:
    // Variabels.
        File file;
};

using Ev = const xcb_generic_event_t *;
class Event_Handler
{
    public: // methods
        using EventCallback = function<void(Ev)>;

        void run()
        {
            xcb_generic_event_t *ev;
            shouldContinue = true;

            while (shouldContinue)
            {
                ev = xcb_wait_for_event(conn);
                if (!ev) continue;

                uint8_t responseType = ev->response_type & ~0x80;
                auto it = eventCallbacks.find(responseType);
                if (it != eventCallbacks.end())
                {
                    for (const pair<CallbackId, EventCallback> &callback : it->second)
                    {
                        callback.second(ev);
                    }
                }

                free(ev);
            }
        }

        void end()
        {
            shouldContinue = false;
        }
        using CallbackId = int;

        CallbackId setEventCallback(uint8_t eventType, EventCallback callback)
        {
            CallbackId id = nextCallbackId++;
            eventCallbacks[eventType].emplace_back(id, std::move(callback));
            return id;
        }

        void removeEventCallback(uint8_t eventType, CallbackId id)
        {
            auto& callbacks = eventCallbacks[eventType];
            callbacks.erase(
                std::remove_if(
                    callbacks.begin(),
                    callbacks.end(),
                    [id](const auto& pair)
                    {
                        return pair.first == id;
                    }
                ),
                callbacks.end()
            );
        }

        template<typename Callback>
        void set_key_press_callback(const uint16_t &__mask, const xcb_keycode_t &__key, Callback &&callback)
        {
            setEventCallback(XCB_KEY_PRESS, [this, callback, __key, __mask](Ev ev)
            {
                RE_CAST_EV(xcb_key_press_event_t);
                if (e->detail == __key)
                {
                    if (e->state == __mask)
                    {
                        callback();
                    }
                }
            });
        }

    private: // variables
        unordered_map<uint8_t, vector<pair<CallbackId, EventCallback>>>(eventCallbacks);
        bool shouldContinue = false;
        CallbackId nextCallbackId = 0;
};
static Event_Handler *event_handler(nullptr);

class Bitmap
{
    public: // constructor
        Bitmap(int width, int height) 
        : width(width), height(height), bitmap(height, std::vector<bool>(width, false)) {}
    
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
            FILE *fp = fopen(file_name, "wb");
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
    
    private: // variables
        int width, height;
        vector<vector<bool>>(bitmap);
        Logger log;
    
};

class _scale
{
    public:
        static uint16_t from_8_to_16_bit(const uint8_t & n)
        {
            return (n << 8) | n;
        }
};

namespace // window namespace
{
    enum BORDER
    {
        NONE  = 0,
        LEFT  = 1 << 0, // 1
        RIGHT = 1 << 1, // 2
        UP    = 1 << 2, // 4
        DOWN  = 1 << 3, // 8
        ALL   = 1 << 4  // 16
    };

    enum window_flags
    {
        MAP          = 1 << 0, // 1
        DEFAULT_KEYS = 1 << 1, // 2
        FOCUS_INPUT  = 1 << 2  // 4
    };

    enum window_event_mask : uint32_t
    {
        KILL_WINDOW = 1 << 25
    };

    void get_atom(char *__name, xcb_atom_t *__atom)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, xcb_intern_atom(conn, 0, strlen(__name), __name), NULL);
        if (reply == nullptr)
        {
            *__atom = XCB_NONE;
            free(reply);
            log_error("reply is nullptr.");
            return;
        }

        *__atom = reply->atom;
        free(reply);
    }

    // Struct representing the _MOTIF_WM_HINTS property format
    typedef struct {
        uint32_t flags;
        uint32_t functions;
        uint32_t decorations;
        int32_t input_mode;
        uint32_t status;
    } motif_wm_hints;

    // Define macros to create the enum class and declare the bitwise operators
    #define DEFINE_BITWISE_ENUM(EnumName, UnderlyingType) \
        enum class EnumName : UnderlyingType; \
        inline EnumName operator|(EnumName a, EnumName b) { \
            return static_cast<EnumName>(static_cast<UnderlyingType>(a) | static_cast<UnderlyingType>(b)); \
        } \
        inline EnumName operator&(EnumName a, EnumName b) { \
            return static_cast<EnumName>(static_cast<UnderlyingType>(a) & static_cast<UnderlyingType>(b)); \
        } \
        inline EnumName operator^(EnumName a, EnumName b) { \
            return static_cast<EnumName>(static_cast<UnderlyingType>(a) ^ static_cast<UnderlyingType>(b)); \
        } \
        inline EnumName operator~(EnumName a) { \
            return static_cast<EnumName>(~static_cast<UnderlyingType>(a)); \
        } \
        enum class EnumName : UnderlyingType

    namespace // WINDOW_CONFIG.
    {
        enum class WINDOW_CONF : uint32_t
        {
            X      = 1 << 0,
            Y      = 1 << 1,
            WIDTH  = 1 << 2,
            HEIGHT = 1 << 3
        };

        inline WINDOW_CONF operator|(WINDOW_CONF a, WINDOW_CONF b)
        {
            return static_cast<WINDOW_CONF>(
                static_cast<std::underlying_type<WINDOW_CONF>::type>(a) |
                static_cast<std::underlying_type<WINDOW_CONF>::type>(b));
        }

        inline WINDOW_CONF operator&(WINDOW_CONF a, WINDOW_CONF b)
        {
            return static_cast<WINDOW_CONF>(
                static_cast<std::underlying_type<WINDOW_CONF>::type>(a) &
                static_cast<std::underlying_type<WINDOW_CONF>::type>(b));
        }

        inline WINDOW_CONF operator^(WINDOW_CONF a, WINDOW_CONF b)
        {
            return static_cast<WINDOW_CONF>(
                static_cast<std::underlying_type<WINDOW_CONF>::type>(a) ^
                static_cast<std::underlying_type<WINDOW_CONF>::type>(b));
        }

        inline WINDOW_CONF operator~(WINDOW_CONF a)
        {
            return static_cast<WINDOW_CONF>(
                ~static_cast<std::underlying_type<WINDOW_CONF>::type>(a));
        }

        DEFINE_BITWISE_ENUM(WINDOW_CONFIG, uint16_t)
        {
            X      = 1 << 0,
            Y      = 1 << 1,
            WIDTH  = 1 << 2,
            HEIGHT = 1 << 3
        };
    }
}

class window
{
    public:  
    // Constructor.
        window() {}

    // Operators.
        operator uint32_t() const
        {
            return _window;
        }
    
        window& operator=(uint32_t new_window) // Overload the assignment operator for uint32_t
        { 
            _window = new_window;
            return *this;
        }
    
    // Methods.
        // Create.
            void create(const uint8_t  &depth,
                        const uint32_t &parent,
                        const int16_t  &x,
                        const int16_t  &y,
                        const uint16_t &width,
                        const uint16_t &height,
                        const uint16_t &border_width,
                        const uint16_t &_class,
                        const uint32_t &visual,
                        const uint32_t &value_mask,
                        const void     *value_list)
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

            void create_default(const uint32_t &parent, const int16_t &x, const int16_t &y, const uint16_t &width, const uint16_t &height)
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

            void create_def(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask)
            {
                _depth        = 0L;
                _parent       = __parent;
                _x            = __x;
                _y            = __y;
                _width        = __width;
                _height       = __height;
                _border_width = 0;
                __class       = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual       = screen->root_visual;
                _value_mask   = 0;
                _value_list   = nullptr;

                make_window();
                set_backround_color(__color);
                grab_default_keys();

                if (__mask > 0)
                {
                    apply_event_mask(&__mask);
                }
            }

            void create_def_no_keys(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask)
            {
                _depth        = 0L;
                _parent       = __parent;
                _x            = __x;
                _y            = __y;
                _width        = __width;
                _height       = __height;
                _border_width = 0;
                __class       = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual       = screen->root_visual;
                _value_mask   = 0;
                _value_list   = nullptr;

                make_window();
                set_backround_color(__color);
                if (__mask > 0) apply_event_mask(&__mask);
            }

            void create_def_and_map(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask)
            {
                _depth        = 0L;
                _parent       = __parent;
                _x            = __x;
                _y            = __y;
                _width        = __width;
                _height       = __height;
                _border_width = 0;
                __class       = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual       = screen->root_visual;
                _value_mask   = 0;
                _value_list   = nullptr;

                make_window();
                set_backround_color(__color);
                grab_default_keys();
                if (__mask > 0) apply_event_mask(&__mask);
                map();
                raise();
            }

            void create_def_and_map_no_keys(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask)
            {
                _depth        = 0L;
                _parent       = __parent;
                _x            = __x;
                _y            = __y;
                _width        = __width;
                _height       = __height;
                _border_width = 0;
                __class       = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual       = screen->root_visual;
                _value_mask   = 0;
                _value_list   = nullptr;

                make_window();
                set_backround_color(__color);
                if (__mask > 0) apply_event_mask(&__mask);
                map();
                raise();
            }

            void create_def_and_map_no_keys_with_borders(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask, int __border_info[3])
            {
                _depth        = 0L;
                _parent       = __parent;
                _x            = __x;
                _y            = __y;
                _width        = __width;
                _height       = __height;
                _border_width = 0;
                __class       = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual       = screen->root_visual;
                _value_mask   = 0;
                _value_list   = nullptr;

                make_window();
                set_backround_color(__color);
                if (__mask > 0) apply_event_mask(&__mask);
                map();
                raise();
                make_borders(__border_info[0], __border_info[1], __border_info[2]);
            }

            void create_window( uint32_t __parent,
                                int16_t  __x,
                                int16_t  __y,
                                uint16_t __width,
                                uint16_t __height,
                                COLOR    __color = DEFAULT_COLOR,
                                uint32_t __event_mask = 0,
                                int      __flags = NONE,
                                void    *__border_data = nullptr)
            {
                _depth        = 0L;
                _parent       = __parent;
                _x            = __x;
                _y            = __y;
                _width        = __width;
                _height       = __height;
                _border_width = 0;
                __class       = XCB_WINDOW_CLASS_INPUT_OUTPUT;
                _visual       = screen->root_visual;
                _value_mask   = 0;
                _value_list   = nullptr;

                make_window();
                set_backround_color(__color);
                if (__flags & DEFAULT_KEYS) grab_default_keys();
                if (__flags & FOCUS_INPUT ) focus_input();
                if (__flags & MAP)
                {
                    map();
                    raise();
                }

                if (__event_mask > 0) apply_event_mask(&__event_mask);

                if (__border_data != nullptr)
                {
                    int *border_data = static_cast<int *>(__border_data);
                    make_borders(border_data[0], border_data[1], border_data[2]);
                }
            }
            
            void create_client_window(const uint32_t &parent, const int16_t &x, const int16_t &y, const uint16_t &width, const uint16_t &height)
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

        // Borders.
            void make_borders(int __border_mask, const uint32_t __size, const int __color)
            {
                if (__border_mask & UP   ) make_border_window(UP,    __size, __color);
                if (__border_mask & DOWN ) make_border_window(DOWN,  __size, __color);
                if (__border_mask & LEFT ) make_border_window(LEFT,  __size, __color);
                if (__border_mask & RIGHT) make_border_window(RIGHT, __size, __color);
                if (__border_mask & ALL  ) make_border_window(ALL,   __size, __color);
            }

            void make_xcb_borders(const int &__color)
            {
                xcb_change_window_attributes(conn, _window, XCB_CW_BORDER_PIXEL, (uint32_t[1]){get_color(__color)});
                xcb_flush(conn);
            }
        
        // Main.
            void raise()
            {
                xcb_configure_window(
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
            
            void reparent(uint32_t __new_parent, int16_t __x, int16_t __y)
            {
                xcb_reparent_window(
                    conn,
                    _window,
                    __new_parent,
                    __x,
                    __y
                );
                xcb_flush(conn);
            }
            
            void make_child(uint32_t __window_to_make_child, uint32_t __child_x, uint32_t __child_y)
            {
                xcb_reparent_window(
                    conn,
                    __window_to_make_child,
                    _window,
                    __child_x,
                    __child_y
                );
                xcb_flush(conn);
            }
            
            void kill()
            {
                xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, 12, "WM_PROTOCOLS");
                xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, nullptr);

                xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
                xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply(conn, delete_cookie, nullptr);

                if (protocols_reply == nullptr)
                {
                    log_error("protocols reply is null");
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }

                if (delete_reply == nullptr)
                {
                    log_error("delete reply is null");
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }

                send_event(KILL_WINDOW, (uint32_t[]){32, protocols_reply->atom, delete_reply->atom});
                free(protocols_reply);
                free(delete_reply);
            }
            
            void clear()
            {
                xcb_clear_area(
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
                xcb_set_input_focus(
                    conn, 
                    XCB_INPUT_FOCUS_POINTER_ROOT, 
                    _window, 
                    XCB_CURRENT_TIME
                );
                xcb_flush(conn);
            }

            void send_event(uint32_t __event_mask, void *__value_list = nullptr)
            {
                if (__event_mask & XCB_EVENT_MASK_EXPOSURE)
                {
                    xcb_expose_event_t expose_event = {
                        .response_type = XCB_EXPOSE,
                        .window = _window,
                        .x      = 0,                              /* < Top-left x coordinate of the area to be redrawn                 */
                        .y      = 0,                              /* < Top-left y coordinate of the area to be redrawn                 */
                        .width  = static_cast<uint16_t>(_width),  /* < Width of the area to be redrawn                                 */
                        .height = static_cast<uint16_t>(_height), /* < Height of the area to be redrawn                                */
                        .count  = 0                               /* < Number of expose events to follow if this is part of a sequence */
                    };

                    xcb_send_event(conn, false, _window, XCB_EVENT_MASK_EXPOSURE, (char *)&expose_event);
                    xcb_flush(conn);
                }

                if (__event_mask & XCB_EVENT_MASK_STRUCTURE_NOTIFY)
                {
                    uint32_t *value_list =  reinterpret_cast<uint32_t *>(__value_list);

                    xcb_configure_notify_event_t event;
                    event.response_type     = XCB_CONFIGURE_NOTIFY;
                    event.event             = _window;
                    event.window            = _window;
                    event.above_sibling     = XCB_NONE;
                    event.x                 = value_list[0];
                    event.y                 = value_list[1];
                    event.width             = value_list[2];
                    event.height            = value_list[3];
                    event.border_width      = 0;
                    event.override_redirect = false;
                    event.pad0              = 0;
                    event.sequence          = 0;

                    xcb_send_event(conn, false, _window, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *)&event);
                    xcb_flush(conn);
                }

                if (__event_mask & KILL_WINDOW)
                {
                    uint32_t *value_list = reinterpret_cast<uint32_t *>(__value_list);

                    xcb_client_message_event_t ev;
                    ev.response_type  = XCB_CLIENT_MESSAGE;
                    ev.format         = value_list[0];
                    ev.sequence       = 0;
                    ev.window         = _window;
                    ev.type           = value_list[1];
                    ev.data.data32[0] = value_list[2];
                    ev.data.data32[1] = XCB_CURRENT_TIME;

                    xcb_send_event(conn, 0, _window, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
                    xcb_flush(conn);
                }
            }

        // Event.
            template<typename Callback>
            void on_expose_event(Callback&& callback)
            {
                event_handler->setEventCallback(XCB_EXPOSE, [this, callback](Ev ev)
                {
                    auto e = reinterpret_cast<const xcb_expose_event_t *>(ev);
                    if (e->window == _window)
                    {
                        callback();
                    }
                });
            }
            
            template<typename Callback>
            void on_button_press_event(Callback&& callback)
            {
                event_handler->setEventCallback(XCB_BUTTON_PRESS, [this, callback](Ev ev)
                {
                    auto e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                    if (e->event == _window)
                    {
                        callback();
                    }
                });
            }

            template<typename Callback>
            void on_L_MOUSE_BUTTON_PRESS_event(Callback&& callback)
            {
                event_handler->setEventCallback(XCB_BUTTON_PRESS, [this, callback](Ev ev)
                {
                    auto e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                    if (e->detail != L_MOUSE_BUTTON) return;
                    
                    if (e->event == _window)
                    {
                        callback();
                    }
                });
            }

        // Check.
            bool check_atom(xcb_atom_t __atom)
            {
                xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(ewmh, _window);
                xcb_ewmh_get_atoms_reply_t reply;

                if (xcb_ewmh_get_wm_state_reply(ewmh, cookie, &reply, NULL)) // Try to get the property
                {
                    for (unsigned int i = 0; i < reply.atoms_len; ++i) // Loop through the atoms in the reply to find if the specified atom is present
                    {
                        if (reply.atoms[i] == __atom) // 'true' if the specified atom is found
                        {
                            xcb_ewmh_get_atoms_reply_wipe(&reply);
                            return true; // Indicate that the atom was found
                        }
                    }
                    
                    xcb_ewmh_get_atoms_reply_wipe(&reply); // Clean up
                }

                return false; // The atom was not found or the property could not be retrieved
            }

            // Function to fetch and check the _MOTIF_WM_HINTS property
            bool check_frameless_window_hint() 
            {
                bool is_frameless = false;
                xcb_atom_t property;
                get_atom((char *)"_MOTIF_WM_HINTS", &property);
                xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, _window, property, XCB_ATOM_ANY, 0, sizeof(motif_wm_hints) / 4);
                xcb_get_property_reply_t* reply = xcb_get_property_reply(conn, cookie, NULL);

                if (reply)
                {
                    if (reply->type != XCB_NONE && reply->format == 32 && reply->length >= 5)
                    {
                        motif_wm_hints* hints = (motif_wm_hints*)xcb_get_property_value(reply);

                        if (hints->decorations == 0)
                        {
                            is_frameless = true;
                        }
                    }
                    else
                    {
                        log_error("No _MOTIF_WM_HINTS property found.");
                    }

                    free(reply);
                }

                return is_frameless;
            }

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
                xcb_ewmh_get_active_window_reply(
                    ewmh,
                    xcb_ewmh_get_active_window(ewmh, 0),
                    &active_window,
                    nullptr
                );
                return _window == active_window;
            }
            
            uint32_t check_event_mask_sum()
            {
                uint32_t mask = 0;
                xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(conn, _window);
                xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, cookie, nullptr);

                if (reply == nullptr) // Check if the reply is valid
                { 
                    log_error("Unable to get window attributes.");
                    return 0;
                }

                mask = reply->all_event_masks;
                free(reply);
                return mask;
            }
            
            vector<xcb_event_mask_t> check_event_mask_codes()
            {
                uint32_t maskSum = check_event_mask_sum();
                vector<xcb_event_mask_t>(setMasks);
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
                vector<xcb_event_mask_t>(masks) = check_event_mask_codes();
                for (const auto &ev_mask : masks)
                {
                    if (ev_mask == event_mask)
                    {
                        return true;
                    }
                }

                return false;
            }
            
            bool is_mapped()
            {
                xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(conn, _window);
                xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, cookie, nullptr);
                if (reply == nullptr) 
                {
                    log_error("Unable to get window attributes.");
                    return false;
                }

                bool isMapped = (reply->map_state == XCB_MAP_STATE_VIEWABLE);
                free(reply);
                return isMapped;    
            }

            bool should_be_decorated()
            {
                // Atom for the _MOTIF_WM_HINTS property, used to control window decorations
                const char* MOTIF_WM_HINTS = "_MOTIF_WM_HINTS";
                
                // Get the atom for the _MOTIF_WM_HINTS property
                xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(MOTIF_WM_HINTS), MOTIF_WM_HINTS);
                xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(conn, cookie, NULL);
                if (!reply)
                {
                    log_error("reply = nullptr");
                    return true; // Default to decorating if we can't check
                }
                
                xcb_atom_t motif_wm_hints_atom = reply->atom;
                free(reply);

                // Try to get the _MOTIF_WM_HINTS property from the window
                xcb_get_property_cookie_t prop_cookie = xcb_get_property(conn, 0, _window, motif_wm_hints_atom, XCB_ATOM_ANY, 0, sizeof(uint32_t) * 5);
                xcb_get_property_reply_t* prop_reply = xcb_get_property_reply(conn, prop_cookie, NULL);

                if (prop_reply && xcb_get_property_value_length(prop_reply) >= sizeof(uint32_t) * 5)
                {
                    uint32_t* hints = (uint32_t*)xcb_get_property_value(prop_reply);
                    
                    // The second uint32_t in the hints indicates decoration status
                    // flags are in the second uint32_t of the property
                    bool decorate = !(hints[1] & (1 << 1)); // Check if decorations are disabled

                    free(prop_reply);
                    return decorate;
                }

                if (prop_reply) free(prop_reply);

                return true; // Default to decorating if we can't find or interpret the hints
            }
        
        // Set.
            void set_active_EWMH_window()
            {
                // 0 for the first (default) screen
                xcb_ewmh_set_active_window(
                    ewmh,
                    0,
                    _window
                );
                xcb_flush(conn);
            }
        
            void set_EWMH_fullscreen_state()
            {
                xcb_change_property(
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
        
        // Unset.
            void unset_EWMH_fullscreen_state()
            {
                xcb_change_property(
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
        
        // Get.
            void get_override_redirect()
            {
	            xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(conn, xcb_get_window_attributes(conn, _window), NULL);
                RETURN_IF(wa == nullptr);

                _override_redirect = wa->override_redirect;
                free(wa);

                if (_override_redirect == true ) log_info("overide_redirect = true");
                if (_override_redirect == false) log_info("overide_redirect = false");
            }

            void get_min_window_size_hints()
            {
                xcb_size_hints_t hints;
                xcb_icccm_get_wm_normal_hints_reply(conn, xcb_icccm_get_wm_normal_hints(conn, _window), &hints, NULL);

                if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
                {
                    _min_width = hints.min_width;
                    _min_height = hints.min_height;
                }
            }

            uint32_t get_transient_for_window()
            {
                uint32_t transient_for = 0; // Default to 0 (no parent)
                xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, _window, XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 0, sizeof(uint32_t));
                xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);

                if (reply && xcb_get_property_value_length(reply) == sizeof(uint32_t))
                {
                    transient_for = *(uint32_t *)xcb_get_property_value(reply);
                }

                free(reply); // Remember to free the reply
                return transient_for;
            }

            string get_window_property(xcb_atom_t __atom)
            {
                xcb_get_property_cookie_t cookie = xcb_get_property(ewmh->connection, 0, _window, __atom, XCB_GET_PROPERTY_TYPE_ANY, 0, 1024);
                xcb_get_property_reply_t* reply = xcb_get_property_reply(ewmh->connection, cookie, NULL);
                string propertyValue = "";

                if (reply && (reply->type == XCB_ATOM_STRING || reply->type == ewmh->UTF8_STRING))
                {
                    // Assuming the property is a UTF-8 string, but you might check and convert depending on the actual type
                    char* value = (char*)xcb_get_property_value(reply);
                    int len = xcb_get_property_value_length(reply);

                    if (value && len > 0)
                    {
                        propertyValue.assign(value, len);
                    }
                }

                free(reply);
                return propertyValue;
            }

            string get_net_wm_name_by_req()
            {
                xcb_ewmh_get_utf8_strings_reply_t wm_name;
                string windowName = "";

                if (xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, _window), &wm_name, NULL))
                {
                    windowName.assign(wm_name.strings, wm_name.strings_len);
                    xcb_ewmh_get_utf8_strings_reply_wipe(&wm_name);
                }

                _name = windowName;
                return windowName;
            }

            string get_net_wm_name() const
            {
                return _name;
            }

            void print_wm_class(xcb_connection_t* conn, xcb_window_t window)
            {
                xcb_get_property_cookie_t cookie;
                xcb_get_property_reply_t* reply;

                // Get the WM_CLASS property
                cookie = xcb_get_property(conn, 0, window, XCB_ATOM_WM_CLASS, XCB_GET_PROPERTY_TYPE_ANY, 0, 1024);
                reply = xcb_get_property_reply(conn, cookie, NULL);

                if (reply && xcb_get_property_value_length(reply) > 0)
                {
                    // WM_CLASS property value is a null-separated string "instance\0class\0"
                    char* value = (char *)xcb_get_property_value(reply);
                    printf("WM_CLASS: %s\n", value); // Prints the instance name
                    printf("WM_CLASS: %s\n", value + strlen(value) + 1); // Prints the class name
                }

                free(reply);
            }

            void print_icccm_wm_class()
            {
                xcb_get_property_cookie_t cookie;
                xcb_icccm_get_wm_class_reply_t wm_class_reply;

                cookie = xcb_icccm_get_wm_class(conn, _window); // Request WM_CLASS property
                if (xcb_icccm_get_wm_class_reply(conn, cookie, &wm_class_reply, NULL)) // Retrieve the WM_CLASS property
                {
                    logger.log(INFO, __func__, "Instance Name", wm_class_reply.instance_name);
                    logger.log(INFO, __func__, "Class Name", wm_class_reply.class_name);

                    xcb_icccm_get_wm_class_reply_wipe(&wm_class_reply);
                }
                else
                {
                    logger.log(ERROR, __func__, "Failed to retrieve WM_CLASS for window");
                }
            }

            void print_icccm_wm_name()
            {
                xcb_get_property_cookie_t cookie;
                xcb_icccm_get_text_property_reply_t wm_name_reply;

                // Request WM_NAME property
                cookie = xcb_icccm_get_wm_name(conn, _window);

                // Retrieve the WM_NAME property
                if (xcb_icccm_get_wm_name_reply(conn, cookie, &wm_name_reply, NULL))
                {
                    logger.log(INFO, __func__, "Window Name", wm_name_reply.name);

                    xcb_icccm_get_text_property_reply_wipe(&wm_name_reply);
                }
                else
                {
                    logger.log(ERROR, __func__, "Failed to retrieve WM_NAME for window");
                }
            }

            char * property(const char *atom_name)
            {
                xcb_get_property_reply_t *reply;
                unsigned int reply_len;
                char * propertyValue;

                reply = xcb_get_property_reply(
                    conn,
                    xcb_get_property(
                        conn,
                        false,
                        _window,
                        atom(
                            atom_name
                        ),
                        XCB_GET_PROPERTY_TYPE_ANY,
                        0,
                        60
                    ),
                    nullptr
                );

                if (reply == nullptr || xcb_get_property_value_length(reply) == 0)
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

            xcb_rectangle_t get_xcb_rectangle_t_by_req()
            {
                xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, _window), nullptr);
                if (g == nullptr) return (xcb_rectangle_t) {0, 0, 0, 0};
                
                xcb_rectangle_t rect = (xcb_rectangle_t) {g->x, g->y, g->width, g->height};
                free(g);
                return rect;
            }

            xcb_rectangle_t get_xcb_rectangle_t() const
            {
                return (xcb_rectangle_t) {_x, _y, _width, _height};
            }
        
            uint32_t root_window()
            {
                xcb_query_tree_cookie_t cookie;
                xcb_query_tree_reply_t *reply;
                cookie = xcb_query_tree(conn, _window);
                reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (reply == nullptr)
                {
                    log_error("Unable to query the window tree.");
                    return (uint32_t) 0; // Invalid window ID
                }

                uint32_t root_window = reply->root;
                free(reply);
                return root_window;
            }
        
            uint32_t parent_by_req()
            {
                xcb_query_tree_cookie_t cookie;
                xcb_query_tree_reply_t *reply;
                cookie = xcb_query_tree(conn, _window);
                reply = xcb_query_tree_reply(conn, cookie, nullptr);

                if (reply == nullptr)
                {
                    log_error("Unable to query the window tree.");
                    return (uint32_t) 0; // return an invalid window ID.
                }

                uint32_t parent_window = reply->parent;
                if (_parent != parent_window)
                {
                    logger.log(ERROR, __func__,  "internal parent: ", _parent, " does not match 'retrived_parent': ", parent_window);
                    _parent = parent_window;
                }

                free(reply);
                return parent_window;
            }

            uint32_t parent() const
            {
                return _parent;
            }
        
            uint32_t *children(uint32_t *child_count)
            {
                *child_count = 0;
                xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, _window);
                xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, nullptr);

                if (reply == nullptr)
                {
                    log_error("Unable to query the window tree.");
                    return nullptr;
                }

                *child_count = xcb_query_tree_children_length(reply);
                uint32_t *children = static_cast<uint32_t *>(malloc(*child_count *sizeof(xcb_window_t)));

                if (children == nullptr)
                {
                    log_error("Unable to allocate memory for children.");
                    free(reply);
                    return nullptr;
                }

                memcpy(children, xcb_query_tree_children(reply), * child_count * sizeof(uint32_t));
                free(reply);
                return children;
            }

            uint32_t get_min_width() const
            {
                return _min_width;
            }

            uint32_t get_min_height() const
            {
                return _min_height;
            }
        
            int16_t x_from_req()
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t x;
                if (geometry == nullptr)
                {
                    x = 200;
                    return x;
                }
                else
                {
                    x = geometry->x;
                    free(geometry);
                }

                return x;
            }
        
            int16_t y_from_req()
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t y;
                if (geometry == nullptr)
                {
                    y = 200;
                    return y;
                }

                y = geometry->y;
                free(geometry);
                return y;
            }
        
            int16_t width_from_req()
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t width;
                if (geometry == nullptr)
                {
                    width = 200;
                    return width;
                }

                width = geometry->width;
                free(geometry);
                return width;
            }
        
            int16_t height_from_req()
            {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t height;
                if (geometry == nullptr)
                {
                    height = 200;
                    return height;
                }

                height = geometry->height;
                free(geometry);
                return height;
            }
        
        // Configuration.
            void apply_event_mask(const vector<uint32_t> &values)
            {
                if (values.empty())
                {
                    log_error("values vector is empty");
                    return;
                }

                xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    values.data()
                );
                xcb_flush(conn);
            }

            void set_event_mask(uint32_t __mask)
            {
                apply_event_mask(&__mask);
            }

            void apply_event_mask(const uint32_t *__mask)
            {
                xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    __mask
                );
                xcb_flush(conn);
            }

            void set_pointer(CURSOR cursor_type)
            {
                xcb_cursor_context_t *ctx;
                if (xcb_cursor_context_new(conn, screen, &ctx) < 0)
                {
                    log_error("Unable to create cursor context.");
                    return;
                }

                xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, pointer_from_enum(cursor_type));
                if (!cursor)
                {
                    log_error("Unable to load cursor.");
                    xcb_cursor_context_free(ctx);
                    xcb_free_cursor(conn, cursor);
                    return;
                }

                xcb_change_window_attributes(
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

            // Size_pos.
                // Fetch.
                    int16_t x() const
                    {
                        return _x;
                    }
                
                    int16_t y() const
                    {
                        return _y;
                    }
                
                    uint16_t width() const
                    {
                        return _width;
                    }
                
                    uint16_t height() const
                    {
                        return _height;
                    }

                void configure(uint16_t __mask, const void *__value_list)
                {
                    xcb_configure_window(conn, _window, __mask, __value_list);
                    xcb_flush(conn);
                }
                  
                void x(const uint32_t &x)
                {
                    config_window(MWM_CONFIG_x, x);
                    update(x, _y, _width, _height);
                }

                void y(const uint32_t &y)
                {
                    config_window(XCB_CONFIG_WINDOW_Y, y);
                    update(_x, y, _width, _height);
                }

                void width(const uint32_t &width)
                {
                    config_window(MWM_CONFIG_width, width);
                    update(_x, _y, width, _height);
                }

                void height(const uint32_t &height)
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

            // Backround.
                void set_backround_color(COLOR color)
                {
                    _color = color;
                    change_back_pixel(get_color(color));
                }

                void set_backround_color_8_bit(const uint8_t &red_value, const uint8_t &green_value, const uint8_t &blue_value)
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

                    Imlib_Image scaledImage = imlib_create_cropped_scaled_image(
                        0, 
                        0, 
                        originalWidth, 
                        originalHeight, 
                        newWidth, 
                        newHeight
                    );
                    imlib_free_image(); // Free original image
                    imlib_context_set_image(scaledImage);
                    DATA32 *data = imlib_image_get_data(); // Get the scaled image data
                    
                    // Create an XCB image from the scaled data
                    xcb_image_t *xcb_image = xcb_image_create_native( 
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
                    xcb_poly_fill_rectangle(
                        conn, 
                        pixmap, 
                        gc, 
                        1, 
                        &rect
                    );

                    // Calculate position to center the image
                    int x(0), y(0);
                    if (newWidth != _width)
                    {
                        x = (_width - newWidth) / 2;
                    }
                    if (newHeight != _height)
                    {
                        y = (_height - newHeight) / 2;
                    }
                    
                    xcb_image_put( // Put the scaled image onto the pixmap at the calculated position
                        conn, 
                        pixmap, 
                        gc, 
                        xcb_image, 
                        x,
                        y, 
                        0
                    );

                    xcb_change_window_attributes( // Set the pixmap as the background of the window
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

                void make_then_set_png(const char * file_name, const std::vector<std::vector<bool>> &bitmap)
                {
                    create_png_from_vector_bitmap(file_name, bitmap);
                    set_backround_png(file_name);
                }

        // Draw.
            void draw_text(const char *str , const int &text_color, const int &backround_color, const char *font_name, const int16_t &x, const int16_t &y)
            {
                get_font(font_name);
                create_font_gc(text_color, backround_color, font);
                xcb_image_text_8(
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

            void draw_text_auto_color(const char *__str, const int16_t &__x, const int16_t &__y, const int &__text_color = WHITE, const int &__backround_color = 0, const char *__font_name = DEFAULT_FONT)
            {
                get_font(__font_name);
                int backround_color = __backround_color;
                if (backround_color == 0) backround_color = _color;
                create_font_gc(__text_color, backround_color, font);
                xcb_image_text_8(
                    conn,
                    strlen(__str),
                    _window,
                    font_gc,
                    __x,
                    __y,
                    __str
                );
                xcb_flush(conn);
            }

            void draw_text_16(const char *str, const int &text_color, const int &background_color, const char *font_name, const int16_t &x, const int16_t &y)
            {
                get_font(font_name); // Your existing function to set the font
                create_font_gc(text_color, background_color, font); // Your existing function to create a GC with the font

                int len;
                xcb_char2b_t *char2b_str = convert_to_char2b(str, &len);

                xcb_image_text_16(
                    conn,
                    len,
                    _window,
                    font_gc,
                    x,
                    y,
                    char2b_str
                );

                xcb_flush(conn);
                free(char2b_str);
            }

            void draw_text_16_auto_color(const char *__str, const int16_t &__x, const int16_t &__y, const int &__text_color = WHITE, const int &__background_color = 0, const char *__font_name = DEFAULT_FONT)
            {
                get_font(__font_name); // Your existing function to set the font
                int bg_color;
                if (__background_color == 0) bg_color = _color;
                create_font_gc(__text_color, bg_color, font); // Your existing function to create a GC with the font

                int len;
                xcb_char2b_t *char2b_str = convert_to_char2b(__str, &len);

                xcb_image_text_16(
                    conn,
                    len,
                    _window,
                    font_gc,
                    __x,
                    __y,
                    char2b_str
                );

                xcb_flush(conn);
                free(char2b_str);
            }

            void draw_on_expose_event(const char *str , const int &text_color, const int &backround_color, const char *font_name, const int16_t &x, const int16_t &y)
            {
                on_expose_event([this, str, text_color, backround_color, font_name, x, y]()-> void
                {
                    draw_text(str, text_color, backround_color, font_name, x, y);
                });
            }

        // Keys.
            void grab_default_keys()
            {
                grab_keys({
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
                    {   K,          SUPER                   },
                    {   R,          SUPER                   }, // key_binding for 'runner_window'
                    {   F,          SUPER                   }, // key_binding for 'file_app'
                    {   S,          SUPER                   }, // key_binding for 'system_settings'
                    {   D,          SUPER                   }  // key_binding for 'debub menu'
                });
            }
        
            void grab_keys(initializer_list<pair<const uint32_t, const uint16_t>> bindings)
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
                            xcb_grab_key(
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
                grab_keys({
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

                    { SPACE_BAR,    NULL        },
                    { SPACE_BAR,    SHIFT       },
                    { ENTER,        NULL        },
                    { ENTER,        SHIFT       },
                    { DELETE,       NULL        },
                    { DELETE,       SHIFT       },
                });
            }
        
        // Buttons.
            void grab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings)
            {
                for (const auto & binding : bindings) {
                    const uint8_t & button = binding.first;
                    const uint16_t & modifier = binding.second;
                    xcb_grab_button(
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
                for (const auto & binding : bindings) {
                    const uint8_t & button = binding.first;
                    const uint16_t & modifier = binding.second;
                    xcb_ungrab_button(
                        conn,
                        button,
                        _window,
                        modifier
                    );
                }
                xcb_flush(conn); // Flush the request to the X server
            }
        
    private:
    // Variables.
        // main variables.
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
            const void    *_value_list;
        
        xcb_gcontext_t gc;
        xcb_gcontext_t font_gc;
        xcb_font_t     font;
        xcb_pixmap_t   pixmap;
        string _name{};
        int _color = 0;

        uint32_t _min_width  = 200;
        uint32_t _min_height = 100;
        uint8_t  _override_redirect = 0;

        Logger log;
    
    // Methods.
        // Main.
            void make_window()
            {
                _window = xcb_generate_id(conn);
                xcb_create_window(
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
            
            void clear_window()
            {
                xcb_clear_area(
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
            
            void update(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height)
            {
                _x = x;
                _y = y;
                _width = width;
                _height = height;
            }

            void config_window(const uint16_t & mask, const uint16_t & value)/**
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
            {
                xcb_configure_window(
                    conn,
                    _window,
                    mask,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(value)
                    }
                );
            }
            
            void config_window(uint32_t mask, const std::vector<uint32_t> & values)
            {
                if (values.empty())
                {
                    log_error("values vector is empty");
                    return;
                }

                xcb_configure_window(
                    conn,
                    _window,
                    mask,
                    values.data()
                );
            }

        // Create.
            // Gc.
                void create_graphics_exposure_gc()
                {
                    gc = xcb_generate_id(conn);
                    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
                    uint32_t values[3] = {
                        screen->black_pixel,
                        screen->white_pixel,
                        0
                    };

                    xcb_create_gc(
                        conn,
                        gc,
                        _window,
                        mask,
                        values
                    );
                    xcb_flush(conn);
                }
            
                void create_font_gc(const int & text_color, const int & backround_color, xcb_font_t font)
                {
                    font_gc = xcb_generate_id(conn);
                    xcb_create_gc(
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
            
            // Pixmap.
                void create_pixmap()
                {
                    pixmap = xcb_generate_id(conn);
                    xcb_create_pixmap(
                        conn, 
                        screen->root_depth, 
                        pixmap, 
                        _window, 
                        _width, 
                        _height
                    );
                    xcb_flush(conn);
                }
            
            // Png.
                void create_png_from_vector_bitmap(const char *file_name, const vector<vector<bool>> &bitmap)
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
        
        // Get.
            xcb_atom_t atom(const char *atom_name)
            {
                xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
                    conn, 
                    0, 
                    strlen(atom_name), 
                    atom_name
                );
                
                xcb_intern_atom_reply_t * reply = xcb_intern_atom_reply(conn, cookie, NULL);
                if (!reply) {
                    log_error("could not get atom");
                    return XCB_ATOM_NONE;
                } 

                xcb_atom_t atom = reply->atom;
                free(reply);
                return atom;
            }
        
            string AtomName(xcb_atom_t atom)
            {
                xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
                xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(conn, cookie, nullptr);

                if (!reply) {
                    log_error("reply is nullptr.");
                    return "";
                }

                int name_len = xcb_get_atom_name_name_length(reply);
                char* name = xcb_get_atom_name_name(reply);

                std::string atomName(name, name + name_len);

                free(reply);
                return atomName;
            }
        
            void get_font(const char *font_name)
            {
                font = xcb_generate_id(conn);
                xcb_open_font(
                    conn, 
                    font, 
                    strlen(font_name),
                    font_name
                );
                xcb_flush(conn);
            }
        
        // Background.
            void change_back_pixel(const uint32_t &pixel)
            {
                xcb_change_window_attributes(
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

            void change_back_pixel(const uint32_t &pixel, const uint32_t &__window)
            {
                xcb_change_window_attributes(
                    conn,
                    __window,
                    XCB_CW_BACK_PIXEL,
                    (const uint32_t[1])
                    {
                        pixel
                    }
                );
                xcb_flush(conn);
            }

            uint32_t get_color(const int &__color)
            {
                uint32_t pixel = 0;
                xcb_colormap_t colormap = screen->default_colormap;
                rgb_color_code color_code = rgb_code(__color);
                xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply(
                    conn, 
                    xcb_alloc_color(
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
            
            uint32_t get_color(const uint16_t &red_value, const uint16_t &green_value, const uint16_t &blue_value)
            {
                uint32_t pixel = 0;
                xcb_colormap_t colormap = screen->default_colormap;
                xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply(
                    conn, 
                    xcb_alloc_color(
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
                xcb_alloc_color_reply_t * reply = xcb_alloc_color_reply(
                    conn, 
                    xcb_alloc_color(
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
            
            rgb_color_code rgb_code(const int &__color)
            {
                rgb_color_code color;
                uint8_t r;
                uint8_t g;
                uint8_t b;
                
                switch (__color)
                {
                    case COLOR::WHITE:
                        r = 255; g = 255; b = 255;
                        break;
                    case COLOR::BLACK:
                        r = 0; g = 0; b = 0;
                        break;
                    case COLOR::RED:
                        r = 255; g = 0; b = 0;
                        break;
                    case COLOR::GREEN:
                        r = 0; g = 255; b = 0;
                        break;
                    case COLOR::BLUE:
                        r = 0; g = 0; b = 255;
                        break;
                    case COLOR::BLUE_2:
                        r = 0; g = 0; b = 230;
                        break;
                    case COLOR::BLUE_3:
                        r = 0; g = 0; b = 204;
                        break;
                    case COLOR::BLUE_4:
                        r = 0; g = 0; b = 178;
                        break;
                    case COLOR::BLUE_5:
                        r = 0; g = 0; b = 153;
                        break;
                    case COLOR::BLUE_6:
                        r = 0; g = 0; b = 128;
                        break;
                    case COLOR::BLUE_7:
                        r = 0; g = 0; b = 102;
                        break;
                    case COLOR::BLUE_8:
                        r = 0; g = 0; b = 76;
                        break;
                    case COLOR::BLUE_9:
                        r = 0; g = 0; b = 51;
                        break;
                    case COLOR::BLUE_10:
                        r = 0; g = 0; b = 26;
                        break;
                    case COLOR::YELLOW:
                        r = 255; g = 255; b = 0;
                        break;
                    case COLOR::CYAN:
                        r = 0; g = 255; b = 255;
                        break;
                    case COLOR::MAGENTA:
                        r = 255; g = 0; b = 255;
                        break;
                    case COLOR::GREY:
                        r = 128; g = 128; b = 128;
                        break;
                    case COLOR::LIGHT_GREY:
                        r = 192; g = 192; b = 192;
                        break;
                    case COLOR::DARK_GREY:
                        r = 64; g = 64; b = 64;
                        break;
                    case COLOR::DARK_GREY_2:
                        r = 70; g = 70; b = 70;
                        break;
                    case COLOR::DARK_GREY_3:
                        r = 76; g = 76; b = 76;
                        break;
                    case COLOR::DARK_GREY_4:
                        r = 82; g = 82; b = 82;
                        break;
                    case COLOR::ORANGE:
                        r = 255; g = 165; b = 0;
                        break;
                    case COLOR::PURPLE:
                        r = 128; g = 0; b = 128;
                        break;
                    case COLOR::BROWN:
                        r = 165; g = 42; b = 42;
                        break;
                    case COLOR::PINK:
                        r = 255; g = 192; b = 203;
                        break;
                    default:
                        r = 0; g = 0; b = 0; 
                        break;
                }

                color.r = r;
                color.g = g;
                color.b = b;
                return color;
            }
        
        // Borders.
            void create_border_window(const int __color, const uint32_t __x, const uint32_t __y, const uint32_t __width, const uint32_t __height)
            {
                uint32_t window = xcb_generate_id(conn);
                xcb_create_window(
                    conn,
                    _depth,
                    window,
                    _window,
                    __x,
                    __y,
                    __width,
                    __height,
                    0,
                    __class,
                    _visual,
                    _value_mask,
                    _value_list
                );
                xcb_flush(conn);
                change_back_pixel(get_color(__color), window);
                xcb_map_window(conn, window);
                xcb_flush(conn);
            }

            #define CREATE_UP_BORDER(__size, __color)    create_border_window(__color, 0, 0, _width, __size)
            #define CREATE_DOWN_BORDER(__size, __color)  create_border_window(__color, 0, (_height - __size), _width, __size)
            #define CREATE_LEFT_BORDER(__size, __color)  create_border_window(__color, 0, 0, __size, _height)
            #define CREATE_RIGHT_BORDER(__size, __color) create_border_window(__color, (_width - __size), 0, __size, _height)

            void make_border_window(BORDER __border, const uint32_t &__size, const int &__color)
            {
                switch (__border)
                {
                    case UP:
                    {
                        CREATE_UP_BORDER(__size, __color);
                        break;
                    }
                    case DOWN:
                    {
                        CREATE_DOWN_BORDER(__size, __color);
                        break;
                    }
                    case LEFT:
                    {
                        CREATE_LEFT_BORDER(__size, __color);
                        break;
                    }
                    case RIGHT:
                    {
                        CREATE_RIGHT_BORDER(__size, __color);
                        break;
                    }
                    case ALL:
                    {
                        CREATE_UP_BORDER(__size, __color);
                        CREATE_DOWN_BORDER(__size, __color);
                        CREATE_LEFT_BORDER(__size, __color);
                        CREATE_RIGHT_BORDER(__size, __color);
                        break;
                    }
                    case NONE:
                    {
                        break;
                    }
                }
            }

        // Font.
            /*
             * Decodes a single UTF-8 encoded character from the input string
             * and returns the Unicode code point.
             * Also advances the input string by the number of bytes used for
             * the decoded character. 
             */
            uint32_t decode_utf8_char(const char **input)
            {
                const unsigned char *str = (const unsigned char *)*input;
                uint32_t codepoint = 0;
                if (str[0] <= 0x7F)
                {
                    // 1-byte character
                    codepoint = str[0];
                    *input += 1;
                }
                else if ((str[0] & 0xE0) == 0xC0)
                {
                    // 2-byte character
                    codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
                    *input += 2;
                }
                else if ((str[0] & 0xF0) == 0xE0)
                {
                    // 3-byte character
                    codepoint = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
                    *input += 3;
                }
                else if ((str[0] & 0xF8) == 0xF0)
                {
                    // 4-byte character (will not be fully represented in UCS-2)
                    codepoint = 0xFFFD; // Replacement character, as UCS-2 cannot represent this
                    *input += 4;
                }
                else
                {
                    // Invalid UTF-8, return replacement character
                    codepoint = 0xFFFD;
                    *input += 1; // Advance past the invalid byte
                }

                return codepoint;
            }

            // Converts a UTF-8 string to an array of xcb_char2b_t for xcb_image_text_16
            xcb_char2b_t *convert_to_char2b(const char *input, int *len)
            {
                size_t utf8_len = strlen(input);
                size_t max_chars = utf8_len; // Maximum possible number of characters (all 1-byte)

                xcb_char2b_t *char2b = (xcb_char2b_t *)malloc(max_chars * sizeof(xcb_char2b_t));
                int count = 0;

                while (*input != '\0' && count < max_chars)
                {
                    uint32_t codepoint = decode_utf8_char(&input);

                    // Convert Unicode codepoint to xcb_char2b_t
                    char2b[count].byte1 = (codepoint >> 8) & 0xFF;
                    char2b[count].byte2 = codepoint & 0xFF;
                    count++;
                }

                *len = count; // Actual number of characters converted
                return char2b;
            }
};

class client
{
    public:
    // Sub Classes.
        class client_border_decor
        {
            public:
                window left;
                window right;
                window top;
                window bottom;

                window top_left;
                window top_right;
                window bottom_left;
                window bottom_right;
        };

        struct __atoms__
        {
            bool is_modal = false;
            bool has_modal = false;
        };

        struct __modal_data__
        {
            uint32_t transient_for = 0;
            uint32_t modal_window = 0;
        };

    // Variabels.
        window win;
        window frame;
        window titlebar;
        window close_button;
        window max_button;
        window min_button;
        window(icon);

        client_border_decor border;
        __atoms__ atoms;
        __modal_data__ modal_data;

        int16_t x, y;     
        uint16_t width,height;
        uint8_t  depth;
        
        size_pos ogsize;
        size_pos tile_ogsize;
        size_pos max_ewmh_ogsize;
        size_pos max_button_ogsize;

        uint16_t desktop;
        pid_t pid;

    // Methods.
        // Main.
            void make_decorations()
            {
                make_frame();
                make_titlebar();
                make_close_button();
                make_max_button();
                make_min_button();
                
                if (BORDER_SIZE > 0)
                {
                    make_borders();
                }
            }
        
            void raise()
            {
                frame.raise();
            }
        
            void focus()
            {
                win.focus_input();
                win.set_active_EWMH_window();
                frame.raise();
            }
        
            void update()
            {
                x = frame.x();
                y = frame.y();
                width = frame.width();
                height = frame.height();
                win.send_event(XCB_EVENT_MASK_STRUCTURE_NOTIFY, (uint32_t[]){(static_cast<uint32_t>(x) + BORDER_SIZE), (static_cast<uint32_t>(y) + (TITLE_BAR_HEIGHT + BORDER_SIZE)), win.width(), win.height()});
            }
        
            void map()
            {
                frame.map();
            }
        
            void unmap()
            {
                frame.unmap();
            }
        
            void kill()
            {
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

            void align()
            {
                win.x(BORDER_SIZE);
                win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
                xcb_flush(conn);
            }

            #define TITLE_REQ_DRAW  (uint32_t)1 << 0
            #define TITLE_INTR_DRAW (uint32_t)1 << 1

            void draw_title(const uint32_t &__mode)
            {
                titlebar.clear();

                if (__mode & TITLE_REQ_DRAW ) titlebar.draw_text_16_auto_color(win.get_net_wm_name_by_req().c_str(), 4, 15);
                if (__mode & TITLE_INTR_DRAW) titlebar.draw_text_16_auto_color(win.get_net_wm_name().c_str(), 4, 15);
            }
        
        // Config.
            void x_y(const uint32_t &x, const uint32_t &y)
            {
                frame.x_y(x, y);
                // frame.configure(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, (uint32_t[2]){x, y});
            }
        
            void _x(const int &x)
            {
                frame.x(x);
            }
        
            void _y(const int &y)
            {
                frame.y(y);
            }
        
            void _width(const uint32_t &width)
            {
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
        
            void _height(const uint32_t &height)
            {
                win.height((height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                frame.height(height);
                border.left.height((height - (BORDER_SIZE * 2)));
                border.right.height((height - (BORDER_SIZE * 2)));
                border.bottom.y((height - BORDER_SIZE));
                border.bottom_left.y((height - BORDER_SIZE));
                border.bottom_right.y((height - BORDER_SIZE));
            }
        
            void x_width(const uint32_t &x, const uint32_t &width)
            {
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
        
            void y_height(const uint32_t & y, const uint32_t & height)
            {
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
        
            void x_width_height(const uint32_t &x, const uint32_t &width, const uint32_t &height)
            {
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
        
            void y_width_height(const uint32_t &y, const uint32_t &width, const uint32_t &height)
            {
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
        
            void x_y_width_height(const uint32_t &x, const uint32_t &y, const uint32_t &width, const uint32_t &height)
            {
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
        
            void width_height(const uint32_t &width, const uint32_t &height)
            {
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
        
        // Size_pos.
            void save_ogsize()
            {
                ogsize.save(x, y, width, height);
            }
        
            void save_tile_ogsize()
            {
                tile_ogsize.save(x, y, width, height);
            }
        
            void save_max_ewmh_ogsize()
            {
                max_ewmh_ogsize.save(x, y, width, height);
            }
        
            void save_max_button_ogsize()
            {
                max_button_ogsize.save(x, y, width, height);
            }
        
        // Check.
            bool is_active_EWMH_window()
            {
                return win.is_active_EWMH_window();
            }
        
            bool is_EWMH_fullscreen()
            {
                return win.is_EWMH_fullscreen();
            }
        
            bool is_button_max_win()
            {
                if (x      == 0
                 && y      == 0
                 && width  == screen->width_in_pixels
                 && height == screen->height_in_pixels) {
                    return true;
                }
                return false;
            }
        
        // Set.
            void set_active_EWMH_window()
            {
                win.set_active_EWMH_window();
            }
    
            void set_EWMH_fullscreen_state()
            {
                win.set_EWMH_fullscreen_state();
            }

        // Get.
            void get_window_parameters()
            {
                xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, win);
                xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, nullptr);
                if (reply == nullptr)
                {
                    log_error("Unable to get window geometry.");
                    return;
                }
            
                x      = reply->x;
                y      = reply->y;
                width  = reply->width;
                height = reply->height;

                free(reply);
            }

        // Unset.
            void unset_EWMH_fullscreen_state()
            {
                win.unset_EWMH_fullscreen_state();
            }
        
    private:
    // Methods.
        void make_frame()
        {
            frame.create_default(screen->root, (x - BORDER_SIZE), (y - TITLE_BAR_HEIGHT - BORDER_SIZE), (width + (BORDER_SIZE * 2)), (height + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)));
            frame.set_backround_color(DARK_GREY);
            win.reparent(frame, BORDER_SIZE, (TITLE_BAR_HEIGHT + BORDER_SIZE));
            frame.apply_event_mask({XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY});
            frame.map();
        }
    
        void make_titlebar()
        {
            titlebar.create_window(
                frame,
                BORDER_SIZE,
                BORDER_SIZE,
                (width - (BUTTON_SIZE * 3)),
                TITLE_BAR_HEIGHT,
                BLACK,
                XCB_EVENT_MASK_EXPOSURE,
                MAP
            );
            titlebar.grab_button({ { L_MOUSE_BUTTON, NULL } });
            draw_title(TITLE_REQ_DRAW);
            event_handler->setEventCallback(XCB_EXPOSE,          [this](Ev ev)-> void
            {
                RE_CAST_EV(xcb_expose_event_t);
                if (e->window == titlebar)
                {
                    draw_title(TITLE_INTR_DRAW);
                }
            });
            event_handler->setEventCallback(XCB_PROPERTY_NOTIFY, [this](Ev ev)-> void
            {
                RE_CAST_EV(xcb_property_notify_event_t);
                if (e->window == win)
                {
                    if (e->atom == ewmh->_NET_WM_NAME)
                    {
                        draw_title(TITLE_REQ_DRAW);
                    }
                }
            });
        }
    
        void make_close_button()
        {
            close_button.create_default(frame, (width - BUTTON_SIZE + BORDER_SIZE), BORDER_SIZE, BUTTON_SIZE, BUTTON_SIZE);
            close_button.apply_event_mask({XCB_EVENT_MASK_ENTER_WINDOW, XCB_EVENT_MASK_LEAVE_WINDOW});
            close_button.set_backround_color(BLUE);
            close_button.grab_button({ { L_MOUSE_BUTTON, NULL } });
            close_button.map();
            close_button.make_then_set_png("/home/mellw/close.png", CLOSE_BUTTON_BITMAP);
        }
    
        void make_max_button()
        {
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
    
        void make_min_button()
        {
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
    
        void make_borders()
        {
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
    
    // Variables.
        vector<vector<bool>> CLOSE_BUTTON_BITMAP = {
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
};

class desktop
{
    public:
    // Variabels
        vector<client *>(current_clients);
        client *focused_client = nullptr;
        uint16_t desktop;
        const uint16_t x = 0;
        const uint16_t y = 0;
        uint16_t width;
        uint16_t height;
};

class Key_Codes
{
    public:
    // constructor and destructor.
        Key_Codes() 
        : keysyms(nullptr) {}

        ~Key_Codes()
        {
            free(keysyms);
        }

    // methods.
        void init()
        {
            keysyms = xcb_key_symbols_alloc(conn);
            if (keysyms)
            {
                map<uint32_t, xcb_keycode_t *> key_map = {
                    { A,            &a         },
                    { B,            &b         },
                    { C,            &c         },
                    { D,            &d         },
                    { E,            &e         },
                    { F,            &f         },
                    { G,            &g         },
                    { H,            &h         },
                    { I,            &i         },
                    { J,            &j         },
                    { K,            &k         },
                    { L,            &l         },
                    { M,            &m         },
                    { _N,           &n         },
                    { O,            &o         },
                    { P,            &p         },
                    { Q,            &q         },
                    { R,            &r         },
                    { S,            &s         },
                    { T,            &t         },
                    { U,            &u         },
                    { V,            &v         },
                    { W,            &w         },
                    { _X,           &x         },
                    { _Y,           &y         },
                    { Z,            &z         },

                    { SPACE_BAR,    &space_bar },
                    { ENTER,        &enter     },
                    { DELETE,       &_delete   },

                    { F11,          &f11       },
                    { N_1,          &n_1       },
                    { N_2,          &n_2       },
                    { N_3,          &n_3       },
                    { N_4,          &n_4       },
                    { N_5,          &n_5       },
                    { R_ARROW,      &r_arrow   },
                    { L_ARROW,      &l_arrow   },
                    { U_ARROW,      &u_arrow   },
                    { D_ARROW,      &d_arrow   },
                    { TAB,          &tab       }
                };
                
                for (auto &pair : key_map) {
                    xcb_keycode_t * keycode = xcb_key_symbols_get_keycode(keysyms, pair.first);
                    if (keycode) {
                        * (pair.second) = * keycode;
                        free(keycode);
                    }
                }
            }
        }

    // variabels.
        xcb_keycode_t
            a{}, b{}, c{}, d{}, e{}, f{}, g{}, h{}, i{}, j{}, k{}, l{}, m{},
            n{}, o{}, p{}, q{}, r{}, s{}, t{}, u{}, v{}, w{}, x{}, y{}, z{},
            
            space_bar{}, enter{},

            f11{}, n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{},
            l_arrow{}, u_arrow{}, d_arrow{}, tab{}, _delete{};
    
    private:
    // variabels.
        xcb_key_symbols_t * keysyms;
};

class Entry
{
    public:
    // constructor.
        Entry() {}

    // variabels.
        window window;
        bool menu = false;

    // methods.
        void add_name(const char *name)
        {
            entryName = name;
        }

        void add_action(function<void()> action)
        {
            entryAction = action;
        }

        void activate() const
        {
            entryAction();
        }

        const char * getName() const
        {
            return entryName;
        }

        void make_window(const xcb_window_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height)
        {
            window.create_default(
                parent_window,
                x,
                y,
                width,
                height
            );
            window.set_backround_color(BLACK);
            uint32_t mask = XCB_EVENT_MASK_POINTER_MOTION |
                            XCB_EVENT_MASK_ENTER_WINDOW   |
                            XCB_EVENT_MASK_LEAVE_WINDOW;
            window.apply_event_mask(& mask);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.map();
        }

    private:
    // vatiabels.
        const char * entryName;
        function<void()> entryAction;
};

class context_menu
{
    private:
    // Variabels.
        window context_window;
        size_pos size_pos;
        window_borders border;
        int border_size = 1;
        
        vector<Entry>(entries);
        pointer pointer;
        Launcher launcher;

    // Methods.
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
        
        void configure_events()
        {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)-> void
            {
                const auto & e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->detail == L_MOUSE_BUTTON)
                {
                    run_action(&e->event);
                    hide();
                }
            });

            event_handler->setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev)->void
            {
                const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
                if (e->event == screen->root)
                {
                    hide();
                } 
            });
        }
        
        void run_action(const xcb_window_t *w)
        {
            for (const auto & entry : entries)
            {
                if (*w == entry.window)
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
                entry.make_window(
                    context_window,
                    (0 + (BORDER_SIZE / 2)),
                    (y + (BORDER_SIZE / 2)),
                    (size_pos.width - BORDER_SIZE),
                    (size_pos.height - BORDER_SIZE)
                );
                entry.window.draw_text(
                    entry.getName(),
                    WHITE,
                    BLACK,
                    "7x14",
                    2,
                    14
                );
                y += size_pos.height;
            }
        }
    
    public:
    // Methods.
        void init()
        {
            configure_events();
        }
        
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
        }
        
        void add_entry(const char * name, std::function<void()> action)
        {
            Entry entry;
            entry.add_name(name);
            entry.add_action(action);
            entries.push_back(entry);
        }
    
    // consructor.
        context_menu()
        {
            size_pos.x      = pointer.x();
            size_pos.y      = pointer.y();
            size_pos.width  = 120;
            size_pos.height = 20;

            border.left   = size_pos.x;
            border.right  = (size_pos.x + size_pos.width);
            border.top    = size_pos.y;
            border.bottom = (size_pos.y + size_pos.height);

            create_dialog_win();
        }
};

class Window_Manager
{
    public:
    // Constructor.
        Window_Manager() {}
    
    // Variabels.
        window(root);
        Launcher(launcher);
        Logger(log);
        pointer(pointer);
        win_data(data);
        Key_Codes(key_codes);
        
        context_menu *context_menu = nullptr;
        vector<client *>(client_list);
        vector<desktop *>(desktop_list);
        client *focused_client = nullptr;
        desktop *cur_d = nullptr;
    
    // Methods.
        // Main.
            void init()
            {
                _conn(nullptr, nullptr);
                _setup();
                _iter();
                _screen();
                
                root = screen->root;
                root.width(screen->width_in_pixels);
                root.height(screen->height_in_pixels);

                setSubstructureRedirectMask();
                configure_root();
                _ewmh();
                
                key_codes.init();
                event_handler = new Event_Handler();
                
                create_new_desktop(1);
                create_new_desktop(2);
                create_new_desktop(3);
                create_new_desktop(4);
                create_new_desktop(5);

                context_menu = new class context_menu();
                context_menu->add_entry("konsole", [this]()-> void
                {
                    launcher.program((char *) "konsole");
                });

                context_menu->init();

                // std::thread(check_volt()); // dosent work 
            }

            void launch_program(char *program)
            {
                if (fork() == 0)
                {
                    setsid();
                    execvp(program, (char *[]) { program, nullptr });
                }
            }

            void quit(const int &__status)
            {
                xcb_flush(conn);
                delete_client_vec(client_list);
                delete_desktop_vec(desktop_list);
                xcb_ewmh_connection_wipe(ewmh);
                xcb_disconnect(conn);
                exit(__status);
            }

            void change_refresh_rate(xcb_connection_t *conn, int desired_width, int desired_height, int desired_refresh)
            {
                // Initialize RandR and get screen resources
                // This is highly simplified and assumes you have the output and CRTC IDs

                xcb_randr_get_screen_resources_current_reply_t *res_reply = xcb_randr_get_screen_resources_current_reply(
                    conn,
                    xcb_randr_get_screen_resources_current(conn, screen->root),
                    nullptr
                );

                // Iterate through modes to find matching resolution and refresh rate
                xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_current_modes(res_reply);
                int mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);
                for (int i = 0; i < mode_count; ++i)
                {
                    // log_info(modes[i]);
                }

                free(res_reply);
            }
        
            vector<xcb_randr_mode_t> find_mode()
            {
                vector<xcb_randr_mode_t>(mode_vec);

                xcb_randr_get_screen_resources_current_cookie_t res_cookie;
                xcb_randr_get_screen_resources_current_reply_t *res_reply;

                // Get screen resources
                res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
                res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

                if (!res_reply)
                {
                    log_error("Could not get screen resources");
                    return{};
                }

                xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_current_modes(res_reply);
                int mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);

                // Iterate through modes to find matching resolution and refresh rate
                for (int i = 0; i < mode_count; ++i)
                {
                    mode_vec.push_back(modes[i].id);
                }

                return mode_vec;
            }

            // Function to calculate the refresh rate from mode info
            static float calculate_refresh_rate(const xcb_randr_mode_info_t *mode_info)
            {
                if (mode_info->htotal && mode_info->vtotal)
                {
                    return ((float)mode_info->dot_clock / (float)(mode_info->htotal * mode_info->vtotal));
                }

                return 0.0f;
            }

            void list_screen_res_and_refresh_rates()
            {
                xcb_randr_get_screen_resources_current_cookie_t res_cookie;
                xcb_randr_get_screen_resources_current_reply_t *res_reply;
                xcb_randr_mode_info_t *mode_info;
                int mode_count;

                // Get screen resources
                res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
                res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

                if (!res_reply)
                {
                    log_error("Could not get screen resources");
                    return;
                }

                mode_info = xcb_randr_get_screen_resources_current_modes(res_reply);
                mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);

                // Iterate through all modes
                for (int i = 0; i < mode_count; i++)
                {
                    float refresh_rate = calculate_refresh_rate(&mode_info[i]);
                    log_info("Resolution: " + to_string(mode_info[i].width) + ":" + to_string(mode_info[i].height) + ", Refresh Rate: " + to_string(refresh_rate) + " Hz");
                }

                free(res_reply);
            }

            void change_resolution()
            {
                xcb_randr_get_screen_resources_current_cookie_t res_cookie;
                xcb_randr_get_screen_resources_current_reply_t *res_reply;
                xcb_randr_output_t *outputs;
                int outputs_len;

                // Get current screen resources
                res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
                res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, NULL);

                if (!res_reply)
                {
                    log_error("Could not get screen resources");
                    return;
                }

                // Get outputs; assuming the first output is the one we want to change
                outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
                outputs_len = xcb_randr_get_screen_resources_current_outputs_length(res_reply);

                if (!outputs_len)
                {
                    log_error("No outputs found");
                    free(res_reply);
                    return;
                }

                xcb_randr_output_t output = outputs[0]; // Assuming we change the first output

                // Get the current configuration for the output
                xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
                xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, NULL);

                if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
                {
                    log_error("Output is not connected to any CRTC");
                    free(output_info_reply);
                    free(res_reply);
                    return;
                }

                xcb_randr_mode_t mode_id;
                vector<xcb_randr_mode_t>(mode_vector)(find_mode());
                for (int i(0); i < mode_vector.size(); ++i)
                {
                    if (mode_vector[i] == get_current_resolution_and_refresh())
                    {
                        if (i == (mode_vector.size() - 1))
                        {
                            mode_id = mode_vector[0];
                            break;
                        }

                        mode_id = mode_vector[i + 1];
                        break;
                    }
                }

                // Set the mode
                xcb_randr_set_crtc_config_cookie_t set_crtc_config_cookie = xcb_randr_set_crtc_config(
                    conn,
                    output_info_reply->crtc,
                    XCB_CURRENT_TIME,
                    XCB_CURRENT_TIME,
                    0, // x
                    0, // y
                    mode_id,
                    XCB_RANDR_ROTATION_ROTATE_0,
                    1, &output
                );

                xcb_randr_set_crtc_config_reply_t *set_crtc_config_reply = xcb_randr_set_crtc_config_reply(conn, set_crtc_config_cookie, NULL);

                if (!set_crtc_config_reply)
                {
                    log_error("Failed to set mode");
                }
                else
                {
                    log_info("Mode set successfully");
                }

                free(set_crtc_config_reply);
                free(output_info_reply);
                free(res_reply);
            }
            
            xcb_randr_mode_t get_current_resolution_and_refresh()
            {
                xcb_randr_get_screen_resources_current_cookie_t res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
                xcb_randr_get_screen_resources_current_reply_t *res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

                if (!res_reply)
                {
                    log_error("Could not get screen resources");
                    return{};
                }

                xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
                if (!res_reply->num_outputs)
                {
                    log_error("No outputs found");
                    free(res_reply);
                    return{};
                }

                // Assuming the first output is the primary one
                xcb_randr_output_t output = outputs[0];
                xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
                xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

                if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
                {
                    log_error("Output is not currently connected to a CRTC");
                    free(output_info_reply);
                    free(res_reply);
                    return{};
                }

                xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
                xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

                if (!crtc_info_reply)
                {
                    log_error("Could not get CRTC info");
                    free(output_info_reply);
                    free(res_reply);
                    return{};
                }

                xcb_randr_mode_t mode_id;
                xcb_randr_mode_info_t *mode_info = nullptr;
                xcb_randr_mode_info_iterator_t mode_iter = xcb_randr_get_screen_resources_current_modes_iterator(res_reply);
                for (; mode_iter.rem; xcb_randr_mode_info_next(&mode_iter))
                {
                    if (mode_iter.data->id == crtc_info_reply->mode)
                    {
                        mode_id = mode_iter.data->id;
                        break;
                    }
                }

                free(crtc_info_reply);
                free(output_info_reply);
                free(res_reply);
                return mode_id;
            }

            void get_atom(char *name, xcb_atom_t *atom)
            {
                xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, xcb_intern_atom(conn, 0, strlen(name), name), NULL);
                if (reply != NULL)
                {
                    *atom = reply->atom;
                }
                else
                {
                    *atom = XCB_NONE;
                }
                free(reply);
            }

        // Window.
            bool window_exists(const uint32_t &__window)
            {
                xcb_generic_error_t *err;
                free(xcb_query_tree_reply(conn, xcb_query_tree(conn, __window), &err));

                if (err != NULL)
                {
                    free(err);
                    return false;
                }

                return true;
            }

            void window_stack(const uint32_t &__window1, const uint32_t &__window2, const uint32_t &__mode)
            {
                if (__window2 == XCB_NONE) return;
                
                uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
                uint32_t values[] = {__window2, __mode};
                xcb_configure_window(conn, __window1, mask, values);
            }

            // stack '__window1' above '__window2'
            void window_above(const uint32_t &__window1, const uint32_t &__window2)
            {
                window_stack(__window1, __window2, XCB_STACK_MODE_ABOVE);
            }

            // stack '__window1' below '__window2'
            void window_below(const uint32_t &__window1, const uint32_t &__window2)
            {
                window_stack(__window1, __window2, XCB_STACK_MODE_BELOW);
            }

        // Client.
            // Focus.
                void cycle_focus()
                {
                    if (focused_client == nullptr)
                    {
                        if (cur_d->current_clients.size() == 0) return;

                        for (long i(0); i < cur_d->current_clients.size(); ++i)
                        {
                            if (cur_d->current_clients[i] == nullptr) continue;
                            
                            cur_d->current_clients[i]->focus();
                            focused_client = cur_d->current_clients[i];
                            
                            return;
                        }
                    }

                    for (int i(0); i < cur_d->current_clients.size(); ++i)
                    {
                        if (cur_d->current_clients[i] == nullptr) continue;

                        if (cur_d->current_clients[i] == focused_client)
                        {
                            if (i == (cur_d->current_clients.size() - 1))
                            {
                                cur_d->current_clients[0]->focus();
                                focused_client = cur_d->current_clients[0];
                            
                                return;
                            }

                            cur_d->current_clients[i + 1]->focus();
                            focused_client = cur_d->current_clients[i + 1];

                            return;
                        }

                        if (i == (cur_d->current_clients.size() - 1)) i = 0;
                    }
                }

                void unfocus()
                {
                    window(tmp);
                    tmp.create_default(
                        screen->root,
                        -20,
                        -20,
                        20,
                        20
                    );
                    tmp.map();
                    tmp.focus_input();
                    tmp.unmap();
                    tmp.kill();
                }

            // client fetch.
                client *client_from_window(const xcb_window_t *window)
                {
                    for (const auto &c:client_list)
                    {
                        if (*window == c->win)
                        {
                            return c;
                        }
                    }

                    return nullptr;
                }

                client *client_from_any_window(const xcb_window_t *window)
                {
                    for (const auto &c:client_list)
                    {
                        if (*window == c->win 
                        ||  *window == c->frame 
                        ||  *window == c->titlebar 
                        ||  *window == c->close_button 
                        ||  *window == c->max_button 
                        ||  *window == c->min_button 
                        ||  *window == c->border.left 
                        ||  *window == c->border.right 
                        ||  *window == c->border.top 
                        ||  *window == c->border.bottom
                        ||  *window == c->border.top_left
                        ||  *window == c->border.top_right
                        ||  *window == c->border.bottom_left
                        ||  *window == c->border.bottom_right)
                        {
                            return c;
                        }
                    }

                    return nullptr;
                }

                client *client_from_pointer(const int &prox)
                {
                    const uint32_t &x = pointer.x();
                    const uint32_t &y = pointer.y();
                    for (const auto &c : cur_d->current_clients)
                    {
                        // LEFT EDGE OF CLIENT
                        if (x > c->x - prox && x <= c->x) return c;
                        
                        // RIGHT EDGE OF CLIENT
                        if (x >= c->x + c->width && x < c->x + c->width + prox) return c;
                        
                        // TOP EDGE OF CLIENT
                        if (y > c->y - prox && y <= c->y) return c;
                        
                        // BOTTOM EDGE OF CLIENT
                        if (y >= c->y + c->height && y < c->y + c->height + prox) return c;
                    }
                    
                    return nullptr;
                }

                map<client *, edge> get_client_next_to_client(client *c, edge c_edge)
                {
                    map<client *, edge> map;
                    for (client *c2:cur_d->current_clients)
                    {
                        if (c == c2) continue;
 
                        if (c_edge == edge::LEFT)
                        {
                            if(c->x == c2->x + c2->width)
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

                edge get_client_edge_from_pointer(client *c, const int &prox)
                {
                    const uint32_t &x = pointer.x();
                    const uint32_t &y = pointer.y();

                    const uint32_t &top_border    = c->y;
                    const uint32_t &bottom_border = (c->y + c->height);
                    const uint32_t &left_border   = c->x;
                    const uint32_t &right_border  = (c->x + c->width);

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
            
            void manage_new_client(const uint32_t &__window)
            {
                client *c = make_client(__window);
                if (c == nullptr)
                {
                    log_error("could not make client");
                    return;
                }
                c->win.get_override_redirect();

                c->win.x_y_width_height(c->x, c->y, c->width, c->height);
                xcb_flush(conn);
                
                c->win.map();
                c->win.grab_button({
                    { L_MOUSE_BUTTON, ALT },
                    { R_MOUSE_BUTTON, ALT },
                    { L_MOUSE_BUTTON, 0   }
                });

                c->win.grab_default_keys();
                if (!c->win.check_frameless_window_hint())
                {
                    c->make_decorations();
                    c->frame.set_event_mask(FRAME_EVENT_MASK);
                    c->win.set_event_mask(CLIENT_EVENT_MASK);
                    c->update();
                    c->focus();
                    focused_client = c;
                    check_client(c);
                }
            }
            
            client *make_internal_client(window window)
            {
                client *c = new client;
                
                c->win    = window;
                c->x      = window.x();
                c->y      = window.y();
                c->width  = window.width();
                c->height = window.height();
                
                c->make_decorations();
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                c->focus();
                
                return c;
            }
            
            void send_sigterm_to_client(client *c)
            {
                c->kill();
                remove_client(c);
            }
     
            void remove_client(client *c)
            {
                client_list.erase(
                    remove(
                        client_list.begin(),
                        client_list.end(),
                        c
                    ), 
                    client_list.end()
                );

                cur_d->current_clients.erase(
                    remove(
                        cur_d->current_clients.begin(),
                        cur_d->current_clients.end(),
                        c
                    ),
                    cur_d->current_clients.end()
                );

                delete c;
            }

        // desktop.
            void create_new_desktop(const uint16_t &n)
            {
                desktop *d = new desktop;
                
                d->desktop = n;
                d->width   = screen->width_in_pixels;
                d->height  = screen->height_in_pixels;
                cur_d      = d;

                desktop_list.push_back(d);
            }
        
        // experimental.
            xcb_visualtype_t *find_argb_visual(xcb_connection_t *conn, xcb_screen_t *screen)
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

                return nullptr;
            }

            void synchronize_xcb()
            {
                free(xcb_get_input_focus_reply(conn, xcb_get_input_focus(conn), NULL));
            }
        
        // xcb.
            void send_expose_event(window &__window)
            {
                xcb_expose_event_t expose_event = {
                    .response_type = XCB_EXPOSE,
                    .window = __window,
                    .x      = 0,                                        /* < Top-left x coordinate of the area to be redrawn                 */
                    .y      = 0,                                        /* < Top-left y coordinate of the area to be redrawn                 */
                    .width  = static_cast<uint16_t>(__window.width()),  /* < Width of the area to be redrawn                                 */
                    .height = static_cast<uint16_t>(__window.height()), /* < Height of the area to be redrawn                                */
                    .count  = 0                                         /* < Number of expose events to follow if this is part of a sequence */
                };

                xcb_send_event(conn, false, __window, XCB_EVENT_MASK_EXPOSURE, (char *)&expose_event);
                xcb_flush(conn);
            }

    private:
    // Variables.
        window(start_window);
    
    // Functions.
        // Init.
            void _conn(const char *displayname, int *screenp)
            {
                conn = xcb_connect(displayname, screenp);
                check_conn();
            }
            
            void _ewmh()
            {
                if (!(ewmh = static_cast<xcb_ewmh_connection_t *>(calloc(1, sizeof(xcb_ewmh_connection_t)))))
                {
                    log_error("ewmh faild to initialize");
                    quit(1);
                }    
                
                xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms(conn, ewmh);
                if (!(xcb_ewmh_init_atoms_replies(ewmh, cookie, 0)))
                {
                    log_error("xcb_ewmh_init_atoms_replies:faild");
                    quit(1);
                }

                const char * str = "mwm";
                check_error(
                    xcb_ewmh_set_wm_name(
                        ewmh,
                        screen->root,
                        strlen(str),
                        str
                    ), 
                    __func__,
                    "xcb_ewmh_set_wm_name"
                );
            }
            
            void _setup()
            {
                setup = xcb_get_setup(conn);
            }
            
            void _iter()
            {
                iter = xcb_setup_roots_iterator(setup);
            }
            
            void _screen()
            {
                screen = iter.data;
            }
            
            bool setSubstructureRedirectMask()
            {
                xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(
                    conn,
                    root,
                    XCB_CW_EVENT_MASK,
                    (const uint32_t[1])
                    {
                        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                    }
                );

                xcb_generic_error_t * error = xcb_request_check(conn, cookie);
                if (error)
                {
                    log_error("Error: Another window manager is already running or failed to set SubstructureRedirect mask."); 
                    free(error);
                    return false;
                }

                return true;
            }
            
            void configure_root()
            {
                root.set_backround_color(DARK_GREY);
                uint32_t mask =
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   |
                    XCB_EVENT_MASK_ENTER_WINDOW          |
                    XCB_EVENT_MASK_LEAVE_WINDOW          |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY      |
                    XCB_EVENT_MASK_BUTTON_PRESS          |
                    XCB_EVENT_MASK_BUTTON_RELEASE        |
                    XCB_EVENT_MASK_KEY_PRESS             |
                    XCB_EVENT_MASK_KEY_RELEASE           |
                    XCB_EVENT_MASK_FOCUS_CHANGE          |
                    XCB_EVENT_MASK_POINTER_MOTION
                ; 
                root.apply_event_mask(&mask);
                root.clear();
                root.set_backround_png("/home/mellw/mwm_png/galaxy21.png");
                root.set_pointer(CURSOR::arrow);
            }
        
        // Check.
            void check_error(const int &code)
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
        
            void check_conn()
            {
                int status = xcb_connection_has_error(conn);
                check_error(status);
            }
        
            int cookie_error(xcb_void_cookie_t cookie , const char *sender_function)
            {
                xcb_generic_error_t *err = xcb_request_check(conn, cookie);
                if (err)
                {
                    log_error(err->error_code);
                    free(err);
                    return err->error_code;
                }

                return 0;
            }
        
            void check_error(xcb_void_cookie_t cookie , const char *sender_function, const char *err_msg)
            {
                xcb_generic_error_t * err = xcb_request_check(conn, cookie);
                if (err)
                {
                    log_error_code(err_msg, err->error_code);
                    free(err);
                }
            }
        
        int start_screen_window()
        {
            start_window.create_default(
                root,
                0,
                0,
                0,
                0
            );
            start_window.set_backround_color(DARK_GREY);
            start_window.map();
            return 0;
        }

        // Delete.
            void delete_client_vec(vector<client *> &vec)
            {
                for (client *c : vec)
                {
                    send_sigterm_to_client(c);
                    xcb_flush(conn);                    
                }

                vec.clear();
                vector<client *>().swap(vec);
            }
            
            void delete_desktop_vec(vector<desktop *> &vec)
            {
                for (desktop *d : vec)
                {
                    delete_client_vec(d->current_clients);
                    delete d;
                }

                vec.clear();
                vector<desktop *>().swap(vec);
            }
            
            template <typename Type> 
            static void delete_ptr_vector(vector<Type *>& vec)
            {
                for (Type *ptr : vec)
                {
                    delete ptr;
                }

                vec.clear();
                vector<Type *>().swap(vec);
            }
       
            void remove_client_from_vector(client * c, vector<client *> &vec)
            {
                if (c == nullptr) log_error("client is nullptr.");

                vec.erase(
                    std::remove(
                        vec.begin(),
                        vec.end(),
                        c
                    ),
                    vec.end()
                );

                delete c;
            }
        
        // Client.
            client *make_client(const uint32_t &window)
            {
                client * c = new client;
                if (c == nullptr)
                {
                    log_error("Could not allocate memory for client");
                    return nullptr;
                }

                c->win     = window;
                // get_window_parameters(c->win, &c->x, &c->y, &c->width, &c->height);
                c->get_window_parameters();
                if (c->width < 20) c->width = 200;
                if (c->height < 10) c->height = 100;
                c->depth   = 24;
                c->desktop = cur_d->desktop;

                if (c->x <= 0 && c->y <= 0 && c->width != screen->width_in_pixels && c->height != screen->height_in_pixels)
                {
                    c->x = ((screen->width_in_pixels - c->width) / 2);
                    c->y = (((screen->height_in_pixels - c->height) / 2) + (BORDER_SIZE * 2));
                }

                if (c->height > screen->height_in_pixels) c->height = screen->height_in_pixels;
                if (c->width  > screen->width_in_pixels ) c->width  = screen->width_in_pixels;

                if (c->win.is_EWMH_fullscreen())
                {
                    c->x      = 0;
                    c->y      = 0;
                    c->width  = screen->width_in_pixels;
                    c->height = screen->height_in_pixels;
                    c->win.set_EWMH_fullscreen_state();
                }

                if (c->win.check_atom(ewmh->_NET_WM_STATE_MODAL))
                {
                    c->atoms.is_modal = true;
                    c->modal_data.transient_for = c->win.get_transient_for_window();
                }

                c->win.get_min_window_size_hints();
                
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                return c;
            }
        
            void check_client(client * c)
            {
                c->win.x(BORDER_SIZE);
                xcb_flush(conn);

                c->win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
                xcb_flush(conn);

                // if client if full_screen but 'y' is offset for some reason, make 'y' (0)
                if (c->x == 0
                &&  c->y != 0
                &&  c->width == screen->width_in_pixels
                &&  c->height == screen->height_in_pixels)
                {
                    c->_y(0);
                    xcb_flush(conn);
                    return;
                }

                // if client is full_screen 'width' and 'height' but position is offset from (0, 0) then make pos (0, 0)
                if (c->x != 0
                &&  c->y != 0
                &&  c->width == screen->width_in_pixels
                &&  c->height == screen->height_in_pixels)
                {
                    c->x_y(0,0);
                    xcb_flush(conn);
                    return;
                }

                if (c->x < 0)
                {
                    c->_x(0);
                    xcb_flush(conn);
                }

                if (c->y < 0)
                {
                    c->_y(0);
                    xcb_flush(conn);
                }

                if (c->width > screen->width_in_pixels)
                {
                    c->_width(screen->width_in_pixels);
                    xcb_flush(conn);
                }

                if (c->height > screen->height_in_pixels)
                {
                    c->_height(screen->height_in_pixels);
                    xcb_flush(conn);
                }
                
                if ((c->x + c->width) > screen->width_in_pixels)
                {
                    c->_width(screen->width_in_pixels - c->x);
                    xcb_flush(conn);
                }
                
                if ((c->y + c->height) > screen->height_in_pixels)
                {
                    c->_height(screen->height_in_pixels - c->y);
                    xcb_flush(conn);
                }
            }

        // Window.
            void get_window_parameters(const uint32_t &__window, int16_t *__x, int16_t *__y, uint16_t *__width,  uint16_t *__height)
            {
                xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, __window);
                xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, nullptr);
                if (reply == nullptr)
                {
                    log_error("Unable to get window geometry.");
                    return;
                }
            
                *__x      = reply->x;
                *__y      = reply->y;
                *__width  = reply->width;
                *__height = reply->height;

                free(reply);
            }
        
        // Status.
            void check_volt()
            {
                log_info("running");
                this_thread::sleep_for(chrono::minutes(1));
                check_volt();
            }
};
static Window_Manager *wm;

class __network__
{
    public:
    // Methods.
        enum
        {
            LOCAL_IP = 0,
            INTERFACE_FOR_LOCAL_IP = 1
        };

        string get_local_ip_info(int type)
        {
            struct ifaddrs* ifAddrStruct = nullptr;
            struct ifaddrs* ifa = nullptr;
            void* tmpAddrPtr = nullptr;
            string result;

            getifaddrs(&ifAddrStruct);
            for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
            {
                CONTINUE_IF(ifa->ifa_addr == nullptr);
                if (ifa->ifa_addr->sa_family == AF_INET) // check it is IP4
                { 
                    // is a valid IP4 Address
                    tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                    char addressBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                    if (addressBuffer[0] == '1'
                    &&  addressBuffer[1] == '9'
                    &&  addressBuffer[2] == '2')
                    {
                        if (type == LOCAL_IP)
                        {
                            result = addressBuffer;
                            break;
                        }

                        if (type == INTERFACE_FOR_LOCAL_IP)
                        {
                            result = ifa->ifa_name;
                            break;
                        }
                    }
                }
            }

            if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
            return result;
        }

    public:
    // Constructor.
        __network__() {}
};
static __network__ *network(nullptr);

class __wifi__
{
    private:
    // Methods.
        void scan__(const char* interface)
        {
            wireless_scan_head head;
            wireless_scan* result;
            iwrange range;
            int sock;

            // Open a socket to the wireless driver
            sock = iw_sockets_open();
            if (sock < 0)
            {
                log_error("could not open socket");
                return;
            }

            // Get the range of settings
            if (iw_get_range_info(sock, interface, &range) < 0)
            {
                log_error("could not get range info");
                iw_sockets_close(sock);
                return;
            }

            // Perform the scan
            if (iw_scan(sock, const_cast<char*>(interface), range.we_version_compiled, &head) < 0)
            {
                log_error("could not scan");
                iw_sockets_close(sock);
                return;
            }

            // Iterate through the scan results
            result = head.result;
            while (nullptr != result)
            {
                if (result->b.has_essid)
                {
                    string ssid(result->b.essid);
                    log_info(
                        "\nSSID: "    + ssid +
                        "\nSTATUS: "  + to_string(result->stats.status) +
                        "\nQUAL: "    + to_string(result->stats.qual.qual) +
                        "\nLEVEL: "   + to_string(result->stats.qual.level) +
                        "\nNOICE: "   + to_string(result->stats.qual.noise) +
                        "\nBITRATE: " + to_string(result->maxbitrate.value)
                    );
                }

                result = result->next;
            }

            // Close the socket to the wireless driver
            iw_sockets_close(sock);
        }

        void check_network_interfaces__()
        {
            struct ifaddrs* ifAddrStruct = nullptr;
            struct ifaddrs* ifa = nullptr;
            void* tmpAddrPtr = nullptr;

            getifaddrs(&ifAddrStruct);

            for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
            {
                CONTINUE_IF(!ifa->ifa_addr);
                if (ifa->ifa_addr->sa_family == AF_INET) // check it is IP4
                { 
                    // is a valid IP4 Address
                    tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                    char addressBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                    if (addressBuffer[0] == '1'
                    &&  addressBuffer[1] == '9'
                    &&  addressBuffer[2] == '2')
                    {
                        log_info("Interface: " + string(ifa->ifa_name) + " Address: " + addressBuffer);
                    }
                }
                else if (ifa->ifa_addr->sa_family == AF_INET6) // check it is IP6
                {
                    // is a valid IP6 Address
                    tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
                    char addressBuffer[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                    log_info("Interface: " + string(ifa->ifa_name) + " Address: " + addressBuffer);
                } 
                // Here, you could attempt to deduce the interface type (e.g., wlan for WiFi) by name or other means
            }

            if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
            return;
        }
    
    public:
    // Methods.
        void init()
        {
            event_handler->set_key_press_callback((ALT + SUPER), wm->key_codes.f, [this]()-> void { scan__("wlo1"); });
        }

    // Constructor.
        __wifi__() {}
};
static __wifi__ *wifi(nullptr);

class __audio__
{
    private:
    // Variabels
        pa_mainloop* mainloop = nullptr;
        pa_mainloop_api* mainloop_api = nullptr;
        pa_context* context = nullptr;

    public:
    // Methods.
        static void sink_list_cb(pa_context* c, const pa_sink_info* l, int eol, void* userdata)
        {
            // End of list
            if (eol > 0)
            {
                log_error("end of list.");
                return;
            }

            log_info("Sink name: " + string(l->name) + " Description: " + string(l->description));
        }

        static void success_cb(pa_context* c, int success, void* userdata)
        {
            if (success)
            {
                std::cout << "Default sink changed successfully." << std::endl;
            }
            else
            {
                std::cout << "Failed to change the default sink." << std::endl;
            }
            // Signal the main loop to quit after attempting to change the default sink
            pa_mainloop_quit(reinterpret_cast<pa_mainloop*>(userdata), 0);
        }

        static void context_state_cb(pa_context* c, void* userdata)
        {
            switch (pa_context_get_state(c))
            {
                case PA_CONTEXT_READY:
                {
                    const char* sink_name = reinterpret_cast<const char*>(userdata);
                    // Set the default sink
                    pa_operation* o = pa_context_set_default_sink(c, sink_name, success_cb, userdata);
                    if (o) pa_operation_unref(o);
                    break;
                }
                case PA_CONTEXT_FAILED:
                case PA_CONTEXT_TERMINATED:
                    pa_mainloop_quit(reinterpret_cast<pa_mainloop*>(userdata), 1);
                    break;
                default:
                    break;
            }
        }

        static bool change_default_sink(const std::string& sink_name)
        {
            pa_mainloop* m = pa_mainloop_new();
            pa_mainloop_api* mainloop_api = pa_mainloop_get_api(m);
            pa_context* context = pa_context_new(mainloop_api, "Change Default Sink Example");

            if (!context)
            {
                std::cerr << "Failed to create PA context." << std::endl;
                pa_mainloop_free(m);
                return false;
            }

            pa_context_set_state_callback(context, context_state_cb, m);
            pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

            int ret = 0;
            pa_mainloop_run(m, &ret);

            pa_context_disconnect(context);
            pa_context_unref(context);
            pa_mainloop_free(m);

            return ret == 0;
        }

        // // Callback for context state
        // void context_state_cb(pa_context* c, void* userdata)
        // {
        //     pa_context_state_t state = pa_context_get_state(c);
        //     switch (state)
        //     {
        //         // This is the state where we can start doing operations
        //         case PA_CONTEXT_READY:
        //         {
        //             pa_operation* o = pa_context_get_sink_info_list(c, sink_list_cb, nullptr);
        //             if (o) pa_operation_unref(o);
        //             break;
        //         }
        //         default:
        //             break;
        //     }
        // }

        void init()
        {
            mainloop = pa_mainloop_new();
            mainloop_api = pa_mainloop_get_api(mainloop);
            context = pa_context_new(mainloop_api, "mwm");

            pa_context_set_state_callback(context, context_state_cb, this);
            pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
        }

        void cleanup()
        {
            if (context)
            {
                pa_context_disconnect(context);
                pa_context_unref(context);
            }
            if (mainloop)
            {
                pa_mainloop_free(mainloop);
            }
        }

        void run()
        {
            int ret;
            if (pa_mainloop_run(mainloop, &ret) < 0)
            {
                log_error("Failed to run mainloop.");
            }
        }

        void list_sinks()
        {
            pa_operation* op = pa_context_get_sink_info_list(context, sink_list_cb, nullptr);
            if (op) pa_operation_unref(op);
        }

        void set_default_sink(const std::string& sink_name)
        {
            pa_operation* op = pa_context_set_default_sink(context, sink_name.c_str(), success_cb, nullptr);
            if (op) pa_operation_unref(op);
        }
        
    // Constructor.
        __audio__() {}
};
__audio__ audio;

namespace
{
    #define BAR_WINDOW_X      0
    #define BAR_WINDOW_Y      0
    #define BAR_WINDOW_WIDTH  screen->width_in_pixels
    #define BAR_WINDOW_HEIGHT 20 

    #define TIME_DATE_WINDOW_X      screen->width_in_pixels - 140
    #define TIME_DATE_WINDOW_Y      0
    #define TIME_DATE_WINDOW_WIDTH  140
    #define TIME_DATE_WINDOW_HEIGHT 20

    #define WIFI_WINDOW_X      screen->width_in_pixels - 160
    #define WIFI_WINDOW_Y      0
    #define WIFI_WINDOW_WIDTH  20
    #define WIFI_WINDOW_HEIGHT 20

    #define WIFI_DROPDOWN_BORDER 2
    #define WIFI_DROPDOWN_X      ((screen->width_in_pixels - 150) - 110)
    #define WIFI_DROPDOWN_Y      20
    #define WIFI_DROPDOWN_WIDTH  220
    #define WIFI_DROPDOWN_HEIGHT 240

    #define WIFI_CLOSE_WINDOW_X      20
    #define WIFI_CLOSE_WINDOW_Y      WIFI_DROPDOWN_HEIGHT - 40
    #define WIFI_CLOSE_WINDOW_WIDTH  80
    #define WIFI_CLOSE_WINDOW_HEIGHT 20

    #define WIFI_INFO_WINDOW_X      20
    #define WIFI_INFO_WINDOW_Y      20
    #define WIFI_INFO_WINDOW_WIDTH  WIFI_DROPDOWN_WIDTH - 40
    #define WIFI_INFO_WINDOW_HEIGHT WIFI_DROPDOWN_HEIGHT - 120
}
class __status_bar__
{
    private:
    // Methods.
        string get_time_and_date__()
        {
            long now(time({}));
            char buf[80];
            strftime(
                buf,
                size(buf),
                "%Y-%m-%d %H:%M:%S",
                localtime(&now)
            );

            return string(buf);
        }

        void create_windows__()
        {
            _bar_window.create_window(
                screen->root,
                BAR_WINDOW_X,
                BAR_WINDOW_Y,
                BAR_WINDOW_WIDTH,
                BAR_WINDOW_HEIGHT,
                DARK_GREY,
                NONE,
                MAP
            );
            _time_date_window.create_window(
                _bar_window,
                TIME_DATE_WINDOW_X,
                TIME_DATE_WINDOW_Y,
                TIME_DATE_WINDOW_WIDTH,
                TIME_DATE_WINDOW_HEIGHT,
                DARK_GREY,
                XCB_EVENT_MASK_EXPOSURE,
                MAP
            );
            _wifi_window.create_window(
                _bar_window,
                WIFI_WINDOW_X,
                WIFI_WINDOW_Y,
                WIFI_WINDOW_WIDTH,
                WIFI_WINDOW_HEIGHT,
                DARK_GREY,
                XCB_EVENT_MASK_BUTTON_PRESS,
                MAP
            );

            Bitmap bitmap(20, 20);
            
            bitmap.modify(1, 6, 13, 1);
            bitmap.modify(2, 4, 15, 1);
            bitmap.modify(3, 3, 7, 1); bitmap.modify(3, 12, 16, 1);
            bitmap.modify(4, 2, 5, 1); bitmap.modify(4, 14, 17, 1);
            bitmap.modify(5, 1, 4, 1); bitmap.modify(5, 15, 18, 1);
            
            bitmap.modify(5, 7, 12, 1);
            bitmap.modify(6, 6, 13, 1);
            bitmap.modify(7, 5, 9, 1); bitmap.modify(7, 10, 14, 1);
            bitmap.modify(8, 4, 7, 1); bitmap.modify(8, 12, 15, 1);
            bitmap.modify(9, 3, 6, 1); bitmap.modify(9, 13, 16, 1);
            
            bitmap.modify(10, 9, 10, 1);
            bitmap.modify(11, 8, 11, 1);
            bitmap.modify(12, 7, 12, 1);
            bitmap.modify(13, 8, 11, 1);
            bitmap.modify(14, 9, 10, 1);

            bitmap.exportToPng("/home/mellw/wifi.png");
            _wifi_window.set_backround_png("/home/mellw/wifi.png");
            _wifi_window.set_pointer(CURSOR::hand2);

            _audio_window.create_window(
                _bar_window,
                (WIFI_WINDOW_X - 50),
                0,
                50,
                20,
                DARK_GREY,
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                MAP,
                (int[]){ALL, 2, BLACK}
            );
            _audio_window.set_pointer(CURSOR::hand2);
            expose(_audio_window);
        }

        void show__(const uint32_t &__window)
        {
            if (__window == _wifi_dropdown_window)
            {
                _wifi_dropdown_window.create_window(
                    screen->root,
                    WIFI_DROPDOWN_X,
                    WIFI_DROPDOWN_Y,
                    WIFI_DROPDOWN_WIDTH,
                    WIFI_DROPDOWN_HEIGHT,
                    DARK_GREY,
                    NONE,
                    MAP,
                    (int[3]){ALL, WIFI_DROPDOWN_BORDER, BLACK}
                );
                _wifi_close_window.create_window(
                    _wifi_dropdown_window,
                    WIFI_CLOSE_WINDOW_X,
                    WIFI_CLOSE_WINDOW_Y,
                    WIFI_CLOSE_WINDOW_WIDTH,
                    WIFI_CLOSE_WINDOW_HEIGHT,
                    WHITE,
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                    MAP,
                    (int[3]){ALL, WIFI_DROPDOWN_BORDER, BLACK}
                );
                _wifi_close_window.set_pointer(CURSOR::hand2);
                expose(_wifi_close_window);  
                _wifi_info_window.create_window(
                    _wifi_dropdown_window,
                    WIFI_INFO_WINDOW_X,
                    WIFI_INFO_WINDOW_Y,
                    WIFI_INFO_WINDOW_WIDTH,
                    WIFI_INFO_WINDOW_HEIGHT,
                    WHITE,
                    XCB_EVENT_MASK_EXPOSURE,
                    MAP,
                    (int[3]){ALL, WIFI_DROPDOWN_BORDER, BLACK}
                );
                expose(_wifi_info_window);
            }
            
            if (__window == _audio_dropdown_window)
            {
                _audio_dropdown_window.create_window(
                    screen->root,
                    ((WIFI_WINDOW_X - (50 / 2) - (200 / 2))),
                    20,
                    200,
                    100,
                    DARK_GREY,
                    NONE,
                    MAP,
                    (int[3]){ALL, 2, BLACK}
                );
            }
        }

        void hide__(const uint32_t &__window)
        {
            if (__window == _wifi_dropdown_window)
            {
                _wifi_close_window.unmap();
                _wifi_close_window.kill();
                _wifi_info_window.unmap();
                _wifi_info_window.kill();
                _wifi_dropdown_window.unmap();
                _wifi_dropdown_window.kill();
            }

            if (__window == _audio_dropdown_window)
            {
                _audio_dropdown_window.unmap();
                _audio_dropdown_window.kill();
            }
        }

        void setup_events__()
        {
            event_handler->setEventCallback(XCB_EXPOSE,  [&](Ev ev)-> void
            {
                RE_CAST_EV(xcb_expose_event_t);
                expose(e->window);
            });
            
            _wifi_close_window.on_button_press_event([&]()-> void
            {
                hide__(_wifi_dropdown_window);
            });

            _wifi_window.on_button_press_event([&]()-> void
            {
                if (_wifi_dropdown_window.is_mapped())
                {
                    hide__(_wifi_dropdown_window);
                }
                else
                {
                    if (_audio_dropdown_window.is_mapped())
                    {
                        hide__(_audio_dropdown_window);
                    }

                    show__(_wifi_dropdown_window);
                }
            });

            _audio_window.on_button_press_event([&]()-> void
            {
                if (_audio_dropdown_window.is_mapped())
                {
                    hide__(_audio_dropdown_window);
                }
                else
                {
                    if (_wifi_dropdown_window.is_mapped())
                    {
                        hide__(_wifi_dropdown_window);
                    }

                    show__(_audio_dropdown_window);
                }
            });
        }

        void setup_thread__(const uint32_t &__window)
        {
            if (__window == _time_date_window)
            {
                function<void()> __time__ = [&]()-> void
                {
                    while (true)
                    {
                        this->_time_date_window.send_event(XCB_EVENT_MASK_EXPOSURE);
                        this_thread::sleep_for(chrono::seconds(1));
                    }
                };

                thread(__time__).detach();
            }
        }

    public:
    // Variabels.
        window(_bar_window),
            (_time_date_window), 
            (_wifi_window), (_wifi_dropdown_window), (_wifi_close_window), (_wifi_info_window),
            (_audio_window), (_audio_dropdown_window);

    // Methods.
        void init()
        {
            create_windows__();
            setup_events__();
            setup_thread__(_time_date_window);
        }

        void expose(const uint32_t &__window)
        {
            if (__window == _time_date_window ) _time_date_window.draw_text_auto_color(get_time_and_date__().c_str(), 4, 15);
            if (__window == _audio_window     ) _audio_window.draw_text_auto_color("Audio", 4, 15);
            if (__window == _wifi_close_window) _wifi_close_window.draw_text_auto_color("Close", 22, 15,BLACK);
            if (__window == _wifi_info_window )
            {
                string local_ip("Local ip: " + network->get_local_ip_info(__network__::LOCAL_IP));
                _wifi_info_window.draw_text_auto_color(local_ip.c_str(), 4, 16, BLACK);

                string local_interface("interface: " + network->get_local_ip_info(__network__::INTERFACE_FOR_LOCAL_IP));
                _wifi_info_window.draw_text_auto_color(local_interface.c_str(), 4, 30, BLACK);
            }
        }

    // Constructor.
        __status_bar__() {}
};
static __status_bar__ *status_bar(nullptr);

/**
 *
 * @class XCPPBAnimator
 * @brief Class for animating the position and size of an XCB window.
 *
 */
class Mwm_Animator
{
    public:
    // Constructors And Destructor.
        Mwm_Animator(const uint32_t &window)
        : window(window) {}

        Mwm_Animator(client *c)
        : c(c) {}

        ~Mwm_Animator()/**
         * @brief Destructor to ensure the animation threads are stopped when the object is destroyed.
         */
        {
            stopAnimations();
        }
    
    // Methods.
        void animate(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration) /**
         *
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
         *
         */
        {
            stopAnimations(); // ENSURE ANY EXISTING ANIMATION IS STOPPED
            
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
            stepX      = abs(endX - startX)           / (endX - startX);
            stepY      = abs(endY - startY)           / (endY - startY);
            stepWidth  = abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endHeight - startHeight)); 

            /* START ANIMATION THREADS */
            XAnimationThread = thread(&Mwm_Animator::XAnimation, this, endX);
            YAnimationThread = thread(&Mwm_Animator::YAnimation, this, endY);
            WAnimationThread = thread(&Mwm_Animator::WAnimation, this, endWidth);
            HAnimationThread = thread(&Mwm_Animator::HAnimation, this, endHeight);

            this_thread::sleep_for(chrono::milliseconds(duration)); // WAIT FOR ANIMATION TO COMPLETE
            stopAnimations(); // STOP THE ANIMATION
        }

        void animate_client(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration)
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
            stepX      = abs(endX - startX)           / (endX - startX);
            stepY      = abs(endY - startY)           / (endY - startY);
            stepWidth  = abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endHeight - startHeight)); 
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = thread(&Mwm_Animator::GFrameAnimation, this, endX, endY, endWidth, endHeight);
            XAnimationThread = thread(&Mwm_Animator::CliXAnimation, this, endX);
            YAnimationThread = thread(&Mwm_Animator::CliYAnimation, this, endY);
            WAnimationThread = thread(&Mwm_Animator::CliWAnimation, this, endWidth);
            HAnimationThread = thread(&Mwm_Animator::CliHAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            this_thread::sleep_for(chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }

        enum DIRECTION
        {
            NEXT,
            PREV
        };

        void animate_client_x(int startX, int endX, int duration)
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
            stepX = abs(endX - startX) / (endX - startX);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(abs(endX - startX));
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = thread(&Mwm_Animator::GFrameAnimation_X, this, endX);
            XAnimationThread = thread(&Mwm_Animator::CliXAnimation, this, endX);

            /* WAIT FOR ANIMATION TO COMPLETE */
            this_thread::sleep_for(chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
    
    private:
    // variabels.
        xcb_window_t window;
        client * c;
        thread(GAnimationThread);
        thread(XAnimationThread);
        thread(YAnimationThread);
        thread(WAnimationThread);
        thread(HAnimationThread);
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
        atomic<bool> stopGFlag{false};
        atomic<bool> stopXFlag{false};
        atomic<bool> stopYFlag{false};
        atomic<bool> stopWFlag{false};
        atomic<bool> stopHFlag{false};
        chrono::high_resolution_clock::time_point(XlastUpdateTime);
        chrono::high_resolution_clock::time_point(YlastUpdateTime);
        chrono::high_resolution_clock::time_point(WlastUpdateTime);
        chrono::high_resolution_clock::time_point(HlastUpdateTime);
        const double frameRate = 120;
        const double frameDuration = 1000.0 / frameRate; 
    
    // Methods.
        void XAnimation(const int & endX)/**
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
        {
            XlastUpdateTime = chrono::high_resolution_clock::now();
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

        void XStep()/**
         * @brief Performs a step in the X direction.
         * 
         * This function increments the currentX variable by the stepX value.
         * If it is time to render, it configures the window's X position using the currentX value.
         * 
         * @note This function assumes that the connection and window variables are properly initialized.
         */
        {
            currentX += stepX;
            
            if (XisTimeToRender())
            {
                xcb_configure_window(
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_X,
                    (const uint32_t[1]) {
                        static_cast<const uint32_t &>(currentX)
                    }
                );
                xcb_flush(conn);
            }
        }
    
        void YAnimation(const int & endY)/**
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
    
        void YStep()/**
         * @brief Performs a step in the Y direction.
         * 
         * This function increments the currentY variable by the stepY value.
         * If it is time to render, it configures the window's Y position using xcb_configure_window
         * and flushes the connection using xcb_flush.
         */
        {
            currentY += stepY;
            
            if (YisTimeToRender())
            {
                xcb_configure_window(
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_Y,
                    (const uint32_t[1]) {
                        static_cast<const uint32_t &>(currentY)
                    }
                );
                xcb_flush(conn);
            }
        }
        
        void WAnimation(const int & endWidth)/**
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
        {
            WlastUpdateTime = chrono::high_resolution_clock::now();
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
    
        void WStep()/**
         *
         * @brief Performs a step in the width calculation and updates the window width if it is time to render.
         * 
         * This function increments the current width by the step width. If it is time to render, it configures the window width
         * using the XCB library and flushes the connection.
         *
         */
        {
            currentWidth += stepWidth;

            if (WisTimeToRender())
            {
                xcb_configure_window(
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_WIDTH,
                    (const uint32_t[1]) {
                        static_cast<const uint32_t &>(currentWidth) 
                    }
                );
                xcb_flush(conn);
            }
        }

        void HAnimation(const int & endHeight)/**
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
        {
            HlastUpdateTime = chrono::high_resolution_clock::now();
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
    
        void HStep()/**
         *
         * @brief Increases the current height by the step height and updates the window height if it's time to render.
         * 
         * This function is responsible for incrementing the current height by the step height and updating the window height
         * if it's time to render. It uses the xcb_configure_window function to configure the window height and xcb_flush to
         * flush the changes to the X server.
         *
         */
        {
            currentHeight += stepHeight;
            
            if (HisTimeToRender())
            {
                xcb_configure_window(
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_HEIGHT,
                    (const uint32_t[1]) {
                        static_cast<const uint32_t &>(currentHeight)
                    }
                );
                xcb_flush(conn);
            }
        }
    
        void GFrameAnimation(const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            while (true)
            {
                if (currentX == endX && currentY == endY && currentWidth == endWidth && currentHeight == endHeight)
                {
                    c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                    break;
                }

                c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                thread_sleep(GAnimDuration);
            }
        }
    
        void GFrameAnimation_X(const int & endX)
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
    
        void CliXAnimation(const int & endX)/**
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
        
        void CliYAnimation(const int & endY)/**
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
        {
            YlastUpdateTime = chrono::high_resolution_clock::now();
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
        
        void CliWAnimation(const int & endWidth)/**
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
    
        void CliHAnimation(const int & endHeight)/**
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
        {
            HlastUpdateTime = chrono::high_resolution_clock::now();
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

        void stopAnimations()/**
         *
         * @brief Stops all animations by setting the corresponding flags to true and joining the animation threads.
         *        After joining the threads, the flags are set back to false.
         *
         */
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
    
        void thread_sleep(const double & milliseconds)/**
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
        {
            auto duration = chrono::duration<double, milli>(milliseconds); // Creating a duration with a 'double' in milliseconds
            this_thread::sleep_for(duration); // Sleeping for the duration
        }
    
        bool XisTimeToRender()/**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            const auto & currentTime = chrono::high_resolution_clock::now();
            const chrono::duration<double, milli> & elapsedTime = currentTime - XlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration)
            {
                XlastUpdateTime = currentTime; 
                return true; 
            }

            return false; 
        }

        bool YisTimeToRender()/**
         *
         * Checks if it is time to render a frame based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            const auto & currentTime = chrono::high_resolution_clock::now();
            const chrono::duration<double, milli> & elapsedTime = currentTime - YlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration)
            {
                YlastUpdateTime = currentTime; 
                return true; 
            }

            return false; 
        }
        
        bool WisTimeToRender()/**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - WlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration) 
            {
                WlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        
        bool HisTimeToRender()/**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            const auto &currentTime = chrono::high_resolution_clock::now();
            const chrono::duration<double, milli> & elapsedTime = currentTime - HlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration)
            {
                HlastUpdateTime = currentTime; 
                return true; 
            }

            return false; 
        }
        
        void config_window(const uint32_t & mask, const uint32_t & value)/**
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
        {
            xcb_configure_window(
                conn,
                window,
                mask,
                (const uint32_t[1]) {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(conn);
        }
     
        void config_window(const xcb_window_t & win, const uint32_t & mask, const uint32_t & value)/**
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
        {
            xcb_configure_window(
                conn,
                win,
                mask,
                (const uint32_t[1]) {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(conn);
        }
        
        void config_client(const uint32_t & mask, const uint32_t & value)/**
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
        {
            xcb_configure_window(
                conn,
                c->win,
                mask,
                (const uint32_t[1]) {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_configure_window(
                conn,
                c->frame,
                mask,
                (const uint32_t[1]) {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(conn);
        }

        void conf_client_x()
        {
            const uint32_t x = currentX;
            c->frame.x(x);
            xcb_flush(conn);
        }
};

void animate(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
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

void animate_client(client * & c, const int & endX, const int & endY, const int & endWidth, const int & endHeight, const int & duration)
{
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

class button
{
    public:
    // Constructor.
        button() {}
    
    // Variables.
        window(window);
        const char *name;
    
    // Methods.
        void create(const uint32_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height, COLOR color)
        {
            window.create_default(parent_window, x, y, width, height);
            window.set_backround_color(color);
            window.grab_button({
                { L_MOUSE_BUTTON, NULL }
            });

            window.map();
        }
        
        void action(std::function<void()> action)
        {
            button_action = action;
        }

        void add_event(std::function<void(Ev ev)> action)
        {
            ev_a = action;
            event_id = event_handler->setEventCallback(XCB_BUTTON_PRESS, ev_a);
        }
        
        void activate() const
        {
            button_action();
        }
        
        void put_icon_on_button()
        {
            string icon_path = file.findPngFile({
                "/usr/share/icons/gnome/256x256/apps/",
                "/usr/share/icons/hicolor/256x256/apps/",
                "/usr/share/icons/gnome/48x48/apps/",
                "/usr/share/icons/gnome/32x32/apps/",
                "/usr/share/pixmaps"
            }, name );

            if (icon_path.empty())
            {
                log_info("could not find icon for button: " + string(name));
                return;
            }

            window.set_backround_png(icon_path.c_str());
        }
    
    private: 
    // Variables.
        function<void()> button_action;
        function<void(Ev ev)> ev_a;
        File file;
        Logger log;
        int event_id = 0;
};

class buttons
{
    public:
    // Constructors.
        buttons() {}

    // Variables.
        vector<button>(list);
    
    // Methods.
        void add(const char *name, function<void()>(action))
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
            for (const auto &button : list)
            {
                if (window == button.window)
                {
                    button.activate();
                    return;
                }
            }
        }
};

class search_window
{
    public:
    // Variabels.
        window(main_window);
        string search_string = "";
    
    // Methods.
        void create(const uint32_t & parent_window, const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height)
        {
            main_window.create_default(parent_window, x, y, width, height);
            main_window.set_backround_color(BLACK);
            uint32_t mask =  XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE;
            main_window.apply_event_mask(& mask);
            main_window.map();
            main_window.grab_button({
                { L_MOUSE_BUTTON, NULL }
            });

            main_window.grab_keys_for_typing();
            main_window.grab_default_keys();

            for (int i = 0; i < 7; ++i)
            {
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

        void add_enter_action(std::function<void()> enter_action)
        {
            enter_function = enter_action;
        }

        void init()
        {
            setup_events();
        }

        void clear_search_string()
        {
            search_string = "";
        }

        string string()
        {
            return(search_string);
        }
    
    private:
    // Methods.
        void setup_events()
        {
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) -> void
            {
                const xcb_key_press_event_t * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->event == main_window)
                {
                    if (e->detail == wm->key_codes.a)
                    {
                        if (e->state == SHIFT)
                        {
                            search_string += "A";
                        }
                        else
                        {
                            search_string += "a";
                        }
                    }

                    if (e->detail == wm->key_codes.b)
                    {
                        if (e->state == SHIFT)
                        {
                            search_string += "B";
                        }
                        else
                        {
                            search_string += "b";
                        }
                    }
                    
                    if (e->detail == wm->key_codes.c)
                    {
                        if (e->state == SHIFT)
                        {
                            search_string += "C";
                        }
                        else
                        {
                            search_string += "c";
                        }
                    }
                    
                    if (e->detail == wm->key_codes.d)
                    {
                        if (e->state == SHIFT)
                        {
                            search_string += "D";
                        }
                        else
                        {
                            search_string += "d";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.e) {
                        if(e->state == SHIFT) {
                            search_string += "E";
                        } else {
                            search_string += "e";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.f) {
                        if(e->state == SHIFT) {
                            search_string += "F";
                        } else {
                            search_string += "f";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.g) {
                        if(e->state == SHIFT) {
                            search_string += "G";
                        } else {
                            search_string += "g";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.h) {
                        if(e->state == SHIFT) {
                            search_string += "H";
                        } else {
                            search_string += "h";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.i) {
                        if(e->state == SHIFT) {
                            search_string += "I";
                        } else {
                            search_string += "i";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.j) {
                        if(e->state == SHIFT) {
                            search_string += "J";
                        } else {
                            search_string += "j";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.k) {
                        if(e->state == SHIFT) {
                            search_string += "K";
                        } else {
                            search_string += "k";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.l) {
                        if(e->state == SHIFT) {
                            search_string += "L";
                        } else {
                            search_string += "l";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.m) {
                        if(e->state == SHIFT) {
                            search_string += "M";
                        } else {
                            search_string += "m";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.n) {
                        if(e->state == SHIFT) {
                            search_string += "N";
                        } else {
                            search_string += "n";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.o) {
                        if(e->state == SHIFT) {
                            search_string += "O";
                        } else {
                            search_string += "o"; 
                        }
                    }
                    
                    if(e->detail == wm->key_codes.p) {
                        if(e->state == SHIFT) {
                            search_string += "P";
                        } else {
                            search_string += "p";
                        }
                    }

                    if(e->detail == wm->key_codes.q) {
                        if(e->state == SHIFT) {
                            search_string += "Q";
                        } else { 
                            search_string += "q";
                        }
                    }

                    if(e->detail == wm->key_codes.r) {
                        if(e->state == SHIFT) {
                            search_string += "R";
                        } else {
                            search_string += "r";
                        }
                    }

                    if(e->detail == wm->key_codes.s) {
                        if(e->state == SHIFT) {
                            search_string += "S";
                        } else {
                            search_string += "s";
                        }
                    }

                    if(e->detail == wm->key_codes.t) {
                        if(e->state == SHIFT) {
                            search_string += "T";
                        } else {
                            search_string += "t";
                        }
                    }

                    if(e->detail == wm->key_codes.u) {
                        if(e->state == SHIFT) {
                            search_string += "U";
                        } else {
                            search_string += "u";
                        }
                    }

                    if(e->detail == wm->key_codes.v) {
                        if(e->state == SHIFT) {
                            search_string += "V";
                        } else {
                            search_string += "v";
                        }
                    }

                    if(e->detail == wm->key_codes.w) {
                        if(e->state == SHIFT) {
                            search_string += "W";
                        } else {
                            search_string += "w";
                        }
                    }

                    if(e->detail == wm->key_codes.x) {
                        if(e->state == SHIFT) {
                            search_string += "X";
                        } else {
                            search_string += "x";
                        }
                    }
                    
                    if(e->detail == wm->key_codes.y) {
                        if(e->state == SHIFT) {
                            search_string += "Y";
                        } else {
                            search_string += "y";
                        }
                    }

                    if(e->detail == wm->key_codes.z) {
                        if(e->state == SHIFT) {
                            search_string += "Z";
                        } else {
                            search_string += "z";
                        }
                    }

                    if(e->detail == wm->key_codes.space_bar) {
                        search_string += " ";
                    }

                    if(e->detail == wm->key_codes._delete) {
                        if(search_string.length() > 0) {
                            search_string.erase(search_string.length() - 1);
                            main_window.clear();
                        }
                    }

                    if(e->detail == wm->key_codes.enter) {
                        if(enter_function) {
                            enter_function();
                        }
                    
                        search_string = "";
                        main_window.clear();
                    }

                    draw_text();
                }
            });

            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) -> void {
                const xcb_button_press_event_t * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if(e->event == main_window) {
                    main_window.raise();
                    main_window.focus_input();
                }
            });
        }

        void draw_text()
        {
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
        
    // Variables.
        function<void()> enter_function;
        File file;
        vector<std::string> results;
        vector<window> entry_list;
};

class Mwm_Runner
{
    public:
    // Variabels.
        window main_window;
        search_window search_window;
        uint32_t BORDER = 2;
        Launcher launcher;
        
    // Methods.
        void init()
        {
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
            main_window.grab_button({
                { L_MOUSE_BUTTON, NULL }
            });

            setup_events();
            search_window.create(
                main_window,
                2,
                2,
                main_window.width() - (BORDER * 2),
                main_window.height() - (BORDER * 2)
            );

            search_window.init();
            search_window.add_enter_action([this]()-> void
            {
                launcher.program((char *) search_window.string().c_str());
                hide();
            });
        }

        void show()
        {
            main_window.raise();
            main_window.map();
            search_window.main_window.focus_input();
        }

    private:
    // Methods.
        void hide()
        {
            main_window.unmap();
            search_window.clear_search_string();
        }

        void setup_events()
        {
            event_handler->setEventCallback(XCB_KEY_PRESS,    [&](Ev ev)-> void
            {
                RE_CAST_EV(xcb_button_press_event_t);
                if (e->detail == wm->key_codes.r)
                {
                    if (e->state == SUPER)
                    {
                        show();
                    }
                }
            });

            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)-> void
            {
                RE_CAST_EV(xcb_button_press_event_t);
                if (!main_window.is_mapped()) return;

                if (e->event != main_window && e->event != search_window.main_window)
                {
                    hide();
                }
            });
        }
};
static Mwm_Runner * mwm_runner;

class add_app_dialog_window
{
    public:
    // Variables.
        window(main_window);
        search_window(search_window);
        client(*c);
        buttons(buttons);
        pointer(pointer);
        Logger(log);
    
    // Methods.
        void init()
        {
            create();
            configure_events();
        }

        void show()
        {
            create_client();
            search_window.create(
                main_window,
                DOCK_BORDER,
                DOCK_BORDER,
                (main_window.width() - (DOCK_BORDER * 2)),
                20
            );
            search_window.init();
        }

        void add_enter_action(function<void()> enter_action)
        {
            enter_function = enter_action;
        }
    
    private:
    // Methods.
        void hide()
        {
            wm->send_sigterm_to_client(c);
        }

        void create()
        {
            main_window.create_default(screen->root, pointer.x(), pointer.y(), 300, 200);
            uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            main_window.apply_event_mask(& mask);
            main_window.grab_button({
                { L_MOUSE_BUTTON, NULL }
            });

            main_window.grab_keys({ { Q, SHIFT | ALT } });
            main_window.set_backround_color(DARK_GREY);
        }

        void create_client()
        {
            main_window.x_y(
                pointer.x() - (main_window.width() / 2),
                pointer.y() - (main_window.height() / 2)
            );
            c = wm->make_internal_client(main_window);
            c->x_y(
                (pointer.x() - (c->width / 2)),
                (pointer.y() - (c->height / 2))
            );
            main_window.map();
            c->map();
        }

        void configure_events()
        {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)-> void
            {
                const xcb_button_press_event_t * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == search_window.main_window)
                {
                    c->focus();
                }
            });
        }

    // Variabels.
        function<void()> enter_function;
};

class __file_app__
{
    // Defines.
        #define FILE_APP_LEFT_MENU_WIDTH 120
        #define FILE_APP_LEFT_MENU_ENTRY_HEIGHT 20
        #define FILE_APP_BORDER_SIZE 2

    private:
    // Subclasses.
        class __left_menu__
        {
            private:
            // Variabels.
                struct __menu_entry__
                {
                    window(_window);
                    string(_string);

                    void create(const uint32_t &__parent_window, const uint32_t &__x, const uint32_t &__y, const uint32_t &__width, const uint32_t &__height)
                    {
                        _window.create_window(
                            __parent_window,
                            __x,
                            __y,
                            __width,
                            __height,
                            DARK_GREY,
                            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                            MAP,
                            (int[]){DOWN, FILE_APP_BORDER_SIZE, BLACK}
                        );
                        draw();
                    }

                    void draw()
                    {
                        _window.draw_text(
                            _string.c_str(),
                            WHITE,
                            DARK_GREY,
                            DEFAULT_FONT,
                            4,
                            14
                        );
                    }

                    void configure(const uint32_t &__width)
                    {
                        _window.width(__width);
                        xcb_flush(conn);
                    }
                };
                vector<__menu_entry__>(_menu_entry_vector);

            // Methods.
                void create_menu_entry__(const string &__name)
                {
                    __menu_entry__(menu_entry);
                    menu_entry._string = __name;
                    menu_entry.create(
                        _window,
                        0,
                        (FILE_APP_LEFT_MENU_ENTRY_HEIGHT * _menu_entry_vector.size()),
                        FILE_APP_LEFT_MENU_WIDTH,
                        FILE_APP_LEFT_MENU_ENTRY_HEIGHT
                    );
                    _menu_entry_vector.push_back(menu_entry);
                }

            public:
            // Variabels.
                window(_window), (_border);

            // Methods.
                void create(window &__parent_window)
                {
                    uint32_t mask;

                    _window.create_default(
                        __parent_window,
                        0,
                        0,
                        120,
                        __parent_window.height()
                    );
                    mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
                    _window.apply_event_mask(&mask);
                    _window.set_backround_color(DARK_GREY);
                    _window.grab_button({
                        { L_MOUSE_BUTTON, NULL }
                    });

                    _border.create_default(
                        __parent_window,
                        FILE_APP_LEFT_MENU_WIDTH,
                        0,
                        FILE_APP_BORDER_SIZE,
                        __parent_window.height()
                    );
                    mask = XCB_EVENT_MASK_BUTTON_PRESS;
                    _border.apply_event_mask(&mask);
                    _border.set_backround_color(BLACK);
                }

                void show()
                {
                    _window.map();
                    _window.raise();
                    _border.map();
                    _border.raise();
                    create_menu_entry__("hello");
                    create_menu_entry__("hello");
                    create_menu_entry__("hello");
                    create_menu_entry__("hello");
                }

                void configure(const uint32_t &__width, const uint32_t &__height)
                {
                    _window.height(__height);
                    _border.height(__height);
                    xcb_flush(conn);
                }

                void expose(const uint32_t &__window)
                {
                    for (int i(0); i < _menu_entry_vector.size(); ++i)
                    {
                        if (__window == _menu_entry_vector[i]._window)
                        {
                            _menu_entry_vector[i].draw();
                        }
                    }
                }
        };
        __left_menu__(_left_menu);

    // Methods.
        void create_main_window()
        {
            int width = (screen->width_in_pixels / 2), height = (screen->height_in_pixels / 2);
            int x = ((screen->width_in_pixels / 2) - (width / 2)), y = ((screen->height_in_pixels / 2) - (height / 2));
            main_window.create_window(
                screen->root,
                x,
                y,
                width,
                height,
                BLUE,
                XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE,
                MAP | DEFAULT_KEYS
            );
            main_window.grab_button({{ L_MOUSE_BUTTON, NULL }});
        }

        void make_internal_client()
        {
            c         = new client;
            c->win    = main_window;
            c->x      = main_window.x();
            c->y      = main_window.y();
            c->width  = main_window.width();
            c->height = main_window.height();
            c->make_decorations();
            main_window.set_event_mask(XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY);
            wm->client_list.push_back(c);
            wm->cur_d->current_clients.push_back(c);
            c->focus();
            wm->focused_client = c;
        }

        void launch__()
        {
            create_main_window();
            _left_menu.create(main_window);
            main_window.raise();
            main_window.map();
            make_internal_client();
            _left_menu.show();
        }

        void setup_events()
        {
            event_handler->set_key_press_callback(SUPER, wm->key_codes.f, [this]()-> void { launch__(); });

            event_handler->setEventCallback(XCB_CONFIGURE_NOTIFY, [&](Ev ev)-> void
            {
                const auto *e = reinterpret_cast<const xcb_configure_notify_event_t *>(ev);
                configure(e->window, e->width, e->height);
            });

            event_handler->setEventCallback(XCB_EXPOSE,           [this](Ev ev)->void
            {
                const auto *e = reinterpret_cast<const xcb_expose_event_t *>(ev);
                expose(e->window);
            });

            SET_INTR_CLI_KILL_CALLBACK();
        }

    public:
    // Variabels.
        window(main_window);
        client *c = nullptr;

    // Methods.
        void configure(const uint32_t &__window, const uint32_t &__width, const uint32_t &__height)
        {
            if (__window == main_window)
            {
                _left_menu.configure(__width, __height);
            }
        }

        void expose(const uint32_t &__window)
        {
            _left_menu.expose(__window);
        }

        void init()
        {
            setup_events();
        }

    // Constructor.
        __file_app__() {}
};
static __file_app__ *file_app;

class __screen_settings__
{
    private:
    // Methods.
        void change_refresh_rate(xcb_connection_t *conn, int desired_width, int desired_height, int desired_refresh)
        {
            // Initialize RandR and get screen resources
            // This is highly simplified and assumes you have the output and CRTC IDs

            xcb_randr_get_screen_resources_current_reply_t *res_reply = xcb_randr_get_screen_resources_current_reply(
                conn,
                xcb_randr_get_screen_resources_current(conn, screen->root),
                nullptr
            );

            // Iterate through modes to find matching resolution and refresh rate
            xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_current_modes(res_reply);
            int mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);
            for (int i = 0; i < mode_count; ++i)
            {
                // log_info(modes[i]);
            }

            free(res_reply);
        }
    
        vector<xcb_randr_mode_t> find_mode()
        {
            vector<xcb_randr_mode_t>(mode_vec);

            xcb_randr_get_screen_resources_current_cookie_t res_cookie;
            xcb_randr_get_screen_resources_current_reply_t *res_reply;

            // Get screen resources
            res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
            res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

            if (!res_reply)
            {
                log_error("Could not get screen resources");
                return{};
            }

            xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_current_modes(res_reply);
            int mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);

            // Iterate through modes to find matching resolution and refresh rate
            for (int i = 0; i < mode_count; ++i)
            {
                mode_vec.push_back(modes[i].id);
            }

            return mode_vec;
        }

        static float calculate_refresh_rate(const xcb_randr_mode_info_t *mode_info) /**
         *
         * Function to calculate the refresh rate from mode info
         *
         */
        {
            if (mode_info->htotal && mode_info->vtotal)
            {
                return ((float)mode_info->dot_clock / (float)(mode_info->htotal * mode_info->vtotal));
            }

            return 0.0f;
        }

        vector<pair<xcb_randr_mode_t, string>> get_avalible_resolutions__()
        {
            vector<pair<xcb_randr_mode_t, string>>(results);
            xcb_randr_get_screen_resources_current_cookie_t res_cookie;
            xcb_randr_get_screen_resources_current_reply_t *res_reply;
            xcb_randr_mode_info_t *mode_info;
            int mode_count;

            // Get screen resources
            res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
            res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

            if (!res_reply)
            {
                log_error("Could not get screen resources");
                return {};
            }

            xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            if (!res_reply->num_outputs)
            {
                log_error("No outputs found");
                free(res_reply);
                return {};
            }

            // Assuming the first output is the primary one
            xcb_randr_output_t output = outputs[0];
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                log_error("Output is not currently connected to a CRTC");
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

            if (!crtc_info_reply)
            {
                log_error("Could not get CRTC info");
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            mode_info = xcb_randr_get_screen_resources_current_modes(res_reply);
            mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);

            for (int i = 0; i < mode_count; i++) // Iterate through all modes
            {
                string s = to_string(mode_info[i].width) + "x" + to_string(mode_info[i].height) + " " + to_string(calculate_refresh_rate(&mode_info[i])) + " Hz";
                pair<xcb_randr_mode_t, string> pair{mode_info[i].id, s};
                results.push_back(pair);
            }

            free(crtc_info_reply);
            free(output_info_reply);
            free(res_reply);

            return results;
        }

        void change_resolution()
        {
            xcb_randr_get_screen_resources_current_cookie_t res_cookie;
            xcb_randr_get_screen_resources_current_reply_t *res_reply;
            xcb_randr_output_t *outputs;
            int outputs_len;

            // Get current screen resources
            res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
            res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, NULL);

            if (!res_reply)
            {
                log_error("Could not get screen resources");
                return;
            }

            // Get outputs; assuming the first output is the one we want to change
            outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            outputs_len = xcb_randr_get_screen_resources_current_outputs_length(res_reply);

            if (!outputs_len)
            {
                log_error("No outputs found");
                free(res_reply);
                return;
            }

            xcb_randr_output_t output = outputs[0]; // Assuming we change the first output

            // Get the current configuration for the output
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, NULL);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                log_error("Output is not connected to any CRTC");
                free(output_info_reply);
                free(res_reply);
                return;
            }

            xcb_randr_mode_t mode_id;
            vector<xcb_randr_mode_t>(mode_vector)(find_mode());
            for (int i(0); i < mode_vector.size(); ++i)
            {
                if (mode_vector[i] == get_current_resolution__())
                {
                    if (i == (mode_vector.size() - 1))
                    {
                        mode_id = mode_vector[0];
                        break;
                    }

                    mode_id = mode_vector[i + 1];
                    break;
                }
            }

            // Set the mode
            xcb_randr_set_crtc_config_cookie_t set_crtc_config_cookie = xcb_randr_set_crtc_config(
                conn,
                output_info_reply->crtc,
                XCB_CURRENT_TIME,
                XCB_CURRENT_TIME,
                0, // x
                0, // y
                mode_id,
                XCB_RANDR_ROTATION_ROTATE_0,
                1, &output
            );

            xcb_randr_set_crtc_config_reply_t *set_crtc_config_reply = xcb_randr_set_crtc_config_reply(conn, set_crtc_config_cookie, NULL);

            if (!set_crtc_config_reply)
            {
                log_error("Failed to set mode");
            }
            else
            {
                log_info("Mode set successfully");
            }

            free(set_crtc_config_reply);
            free(output_info_reply);
            free(res_reply);
        }
        
        xcb_randr_mode_t get_current_resolution__()
        {
            xcb_randr_mode_t mode_id;
            xcb_randr_get_screen_resources_current_cookie_t res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
            xcb_randr_get_screen_resources_current_reply_t *res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

            if (!res_reply)
            {
                log_error("Could not get screen resources");
                return {};
            }

            xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            if (!res_reply->num_outputs)
            {
                log_error("No outputs found");
                free(res_reply);
                return {};
            }

            // Assuming the first output is the primary one
            xcb_randr_output_t output = outputs[0];
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                log_error("Output is not currently connected to a CRTC");
                free(output_info_reply);
                free(res_reply);
                return {};
            }

            xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

            if (!crtc_info_reply)
            {
                log_error("Could not get CRTC info");
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            xcb_randr_mode_info_t *mode_info = nullptr;
            xcb_randr_mode_info_iterator_t mode_iter = xcb_randr_get_screen_resources_current_modes_iterator(res_reply);
            for (; mode_iter.rem; xcb_randr_mode_info_next(&mode_iter))
            {
                if (mode_iter.data->id == crtc_info_reply->mode)
                {
                    mode_id = mode_iter.data->id;
                    break;
                }
            }

            free(crtc_info_reply);
            free(output_info_reply);
            free(res_reply);

            return mode_id;
        }

        string get_current_resolution_string__()
        {
            string result;
            xcb_randr_get_screen_resources_current_cookie_t res_cookie;
            xcb_randr_get_screen_resources_current_reply_t *res_reply;
            xcb_randr_mode_info_t *mode_info;
            int mode_count;

            // Get screen resources
            res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
            res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, nullptr);

            if (!res_reply)
            {
                log_error("Could not get screen resources");
                return {};
            }

            xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            if (!res_reply->num_outputs)
            {
                log_error("No outputs found");
                free(res_reply);
                return {};
            }

            // Assuming the first output is the primary one
            xcb_randr_output_t output = outputs[0];
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                log_error("Output is not currently connected to a CRTC");
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

            if (!crtc_info_reply)
            {
                log_error("Could not get CRTC info");
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            mode_info = xcb_randr_get_screen_resources_current_modes(res_reply);
            mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);

            for (int i = 0; i < mode_count; i++) // Iterate through all modes
            {
                if (mode_info[i].id == crtc_info_reply->mode)
                {
                    result = to_string(mode_info[i].width) + "x" + to_string(mode_info[i].height) + " " + to_string(calculate_refresh_rate(&mode_info[i])) + " Hz";
                    break;
                }
            }

            free(crtc_info_reply);
            free(output_info_reply);
            free(res_reply);

            return result;
        }

    public:
    // Variabels.
        xcb_randr_mode_t(_current_resolution);
        string(_current_resoluton_string);
        vector<pair<xcb_randr_mode_t, string>>(_avalible_resolutions);

    // Methods.
        void set_resolution(xcb_randr_mode_t __mode_id)
        {
            xcb_randr_get_screen_resources_current_cookie_t res_cookie;
            xcb_randr_get_screen_resources_current_reply_t *res_reply;
            xcb_randr_output_t *outputs;
            int outputs_len;

            // Get current screen resources
            res_cookie = xcb_randr_get_screen_resources_current(conn, screen->root);
            res_reply = xcb_randr_get_screen_resources_current_reply(conn, res_cookie, NULL);

            if (!res_reply)
            {
                log_error("Could not get screen resources");
                return;
            }

            // Get outputs; assuming the first output is the one we want to change
            outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            outputs_len = xcb_randr_get_screen_resources_current_outputs_length(res_reply);

            if (!outputs_len)
            {
                log_error("No outputs found");
                free(res_reply);
                return;
            }

            xcb_randr_output_t output = outputs[0]; // Assuming we change the first output

            // Get the current configuration for the output
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, NULL);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                log_error("Output is not connected to any CRTC");
                free(output_info_reply);
                free(res_reply);
                return;
            }

            // Set the mode
            xcb_randr_set_crtc_config_cookie_t set_crtc_config_cookie = xcb_randr_set_crtc_config(
                conn,
                output_info_reply->crtc,
                XCB_CURRENT_TIME,
                XCB_CURRENT_TIME,
                0, // x
                0, // y
                __mode_id,
                XCB_RANDR_ROTATION_ROTATE_0,
                1, &output
            );

            xcb_randr_set_crtc_config_reply_t *set_crtc_config_reply = xcb_randr_set_crtc_config_reply(conn, set_crtc_config_cookie, NULL);

            if (!set_crtc_config_reply)
            {
                log_error("Failed to set mode");
            }

            free(set_crtc_config_reply);
            free(output_info_reply);
            free(res_reply);
        }

        void init()
        {
            _avalible_resolutions     = get_avalible_resolutions__();
            _current_resolution       = get_current_resolution__();
            _current_resoluton_string = get_current_resolution_string__();
        }

    // Constructor.
        __screen_settings__() {}
};
static __screen_settings__ *screen_settings(nullptr);

typedef struct dropdown_menu_t
{
    window         _window;
    window         _button_window;
    window         _dropdown_window;
    vector<window> _options_vec;
    vector<string> _options_str_vec;
    string         _name;

    void create(uint32_t __parent, uint32_t __x, uint32_t __y)
    {
        _window.create_window(__parent, __x, __y, 100, 20, RED);
    }
}
dropdown_menu_t;

class __system_settings__
{
    // Defines.
        #define MENU_WINDOW_WIDTH 120
        #define MENU_ENTRY_HEIGHT 20
        #define MENU_ENTRY_TEXT_Y 14
        #define MENU_ENTRY_TEXT_X 4
    
    private:
    // Structs.
        typedef struct {
            window   _window;
            string   _device_name;
            uint16_t _device_id;
        } pointer_device_info_t;

    // Variabels.
        vector<pointer_device_info_t> pointer_vec;

    // Methods.
        // Input Devices.
            void query_input_devices__()
            {
                xcb_input_xi_query_device_cookie_t cookie = xcb_input_xi_query_device(conn, XCB_INPUT_DEVICE_ALL);
                xcb_input_xi_query_device_reply_t* reply = xcb_input_xi_query_device_reply(conn, cookie, NULL);

                if (reply == nullptr)
                {
                    log_error("xcb_input_xi_query_device_reply_t == nullptr");
                    return;
                }

                xcb_input_xi_device_info_iterator_t iter;
                for (iter = xcb_input_xi_query_device_infos_iterator(reply); iter.rem; xcb_input_xi_device_info_next(&iter))
                {
                    xcb_input_xi_device_info_t* device = iter.data;

                    char* device_name = (char*)(device + 1); // Device name is stored immediately after the device info structure.
                    if (device->type == XCB_INPUT_DEVICE_TYPE_SLAVE_POINTER || device->type == XCB_INPUT_DEVICE_TYPE_FLOATING_SLAVE)
                    {
                        pointer_device_info_t pointer_device;
                        pointer_device._device_name = device_name;
                        pointer_device._device_id   = device->deviceid;
                        pointer_vec.push_back(pointer_device);

                        log_info("Found pointing device:" + string(device_name));
                        log_info("Device ID:" + to_string(device->deviceid));
                    }
                }

                free(reply);
            }

        void make_windows__()
        {
            uint32_t mask;
            int width = (screen->width_in_pixels / 2), height = (screen->height_in_pixels / 2);
            int x = ((screen->width_in_pixels / 2) - (width / 2)), y = ((screen->height_in_pixels / 2) - (height / 2));

            _main_window.create_window(
                screen->root,
                x,
                y,
                width,
                height,
                DARK_GREY,
                NONE,
                MAP | DEFAULT_KEYS
            );
            
            _menu_window.create_window(
                _main_window,
                0,
                0,
                MENU_WINDOW_WIDTH,
                _main_window.height(),
                RED,
                XCB_EVENT_MASK_BUTTON_PRESS,
                MAP
            );
            
            _default_settings_window.create_window(
                _main_window,
                MENU_WINDOW_WIDTH,
                0,
                (_main_window.width() - MENU_WINDOW_WIDTH),
                _main_window.height(),
                ORANGE,
                XCB_EVENT_MASK_BUTTON_PRESS,
                MAP
            );

            _screen_menu_entry_window.create_window(
                _menu_window,
                0,
                0,
                MENU_WINDOW_WIDTH,
                MENU_ENTRY_HEIGHT,
                BLUE,
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                MAP,
                (int[]){DOWN | RIGHT, 2, BLACK}
            );

            _screen_settings_window.create_window(
                _main_window,
                MENU_WINDOW_WIDTH,
                0,
                (_main_window.width() - MENU_WINDOW_WIDTH),
                _main_window.height(),
                WHITE,
                XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS
            );

            _screen_resolution_window.create_window(
                _screen_settings_window,
                100,
                20,
                160,
                20,
                DARK_GREY,
                XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS,
                NONE,
                (int[]){ALL, 2, BLACK}
            );

            _screen_resolution_button_window.create_window(
                _screen_settings_window,
                260,
                20,
                20,
                20,
                DARK_GREY,
                XCB_EVENT_MASK_BUTTON_PRESS,
                NONE,
                (int[]){RIGHT | DOWN | UP, 2, BLACK}
            );

            _audio_menu_entry_window.create_window(
                _menu_window,
                0,
                20,
                MENU_WINDOW_WIDTH,
                MENU_ENTRY_HEIGHT,
                BLUE,
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                MAP,
                (int[]){DOWN | RIGHT, 2, BLACK}
            );

            _audio_settings_window.create_window(
                _main_window,
                MENU_WINDOW_WIDTH,
                0,
                (_main_window.width() - MENU_WINDOW_WIDTH),
                _main_window.height(),
                PURPLE,
                XCB_EVENT_MASK_BUTTON_PRESS
            );

            _network_menu_entry_window.create_window(
                _menu_window,
                0,
                40,
                MENU_WINDOW_WIDTH,
                MENU_ENTRY_HEIGHT,
                BLUE,
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                MAP,
                (int[]){DOWN | RIGHT, 2, BLACK}
            );

            _network_settings_window.create_window(
                _main_window,
                MENU_WINDOW_WIDTH,
                0,
                (_main_window.width() - MENU_WINDOW_WIDTH),
                _main_window.height(),
                MAGENTA
            );

            _input_menu_entry_window.create_window(
                _menu_window,
                0,
                60,
                MENU_WINDOW_WIDTH,
                MENU_ENTRY_HEIGHT,
                BLUE,
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                MAP,
                (int[]){DOWN | RIGHT, 2, BLACK}
            );

            _input_settings_window.create_window(
                _main_window,
                MENU_WINDOW_WIDTH,
                0,
                _main_window.width() - MENU_WINDOW_WIDTH,
                _main_window.height()
            );

            _input_device_window.create_window(
                _input_settings_window,
                100,
                20,
                160,
                20,
                WHITE,
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE
            );

            _input_device_button_window.create_window(
                _input_settings_window,
                (_input_device_window.x() + _input_device_window.width()),
                _input_device_window.y(),
                20,
                20,
                WHITE,
                XCB_EVENT_MASK_BUTTON_PRESS
            );
        }

        void make_internal_client__()
        {
            c         = new client;
            c->win    = _main_window;
            c->x      = _main_window.x();
            c->y      = _main_window.y();
            c->width  = _main_window.width();
            c->height = _main_window.height();
            c->make_decorations();
            c->frame.set_event_mask(FRAME_EVENT_MASK);
            c->win.set_event_mask(CLIENT_EVENT_MASK);
            wm->client_list.push_back(c);
            wm->cur_d->current_clients.push_back(c);
            c->focus();
            wm->focused_client = c;
        }

        void adjust_and_map_subwindow__(window &__window)
        {
            if (__window.is_mapped()) return;

            if (_default_settings_window.is_mapped()) _default_settings_window.unmap();
            if (_screen_settings_window.is_mapped() ) _screen_settings_window.unmap();
            if (_audio_settings_window.is_mapped()  ) _audio_settings_window.unmap();
            if (_network_settings_window.is_mapped()) _network_settings_window.unmap();
            if (_input_settings_window.is_mapped()  ) _input_settings_window.unmap();

            __window.width_height((c->win.width() - MENU_WINDOW_WIDTH), c->win.height());
            xcb_flush(conn);
            __window.map();
            __window.raise();

            if (__window == _screen_settings_window)
            {
                expose(__window);
                _screen_resolution_window.map();
                expose(_screen_resolution_window);
                _screen_resolution_button_window.map();
            }

            if (__window == _input_settings_window)
            {
                expose(__window);
                query_input_devices__();
                
                uint32_t window_width = 0;
                for (int i = 0; i < pointer_vec.size(); ++i)
                {
                    uint32_t len = pointer_vec[i]._device_name.length() * DEFAULT_FONT_WIDTH;
                    if (len > window_width) window_width = len;
                }

                _input_device_window.width(window_width);
                _input_device_button_window.x(_input_device_window.x() + _input_device_window.width());

                _input_device_window.make_borders(ALL, 2, BLACK);
                _input_device_window.map();
                _input_device_button_window.make_borders(RIGHT | DOWN | UP, 2, BLACK);
                _input_device_button_window.map();
            }
        }

        void show__(uint32_t __window)
        {
            if (__window == _screen_resolution_dropdown_window)
            {
                _screen_resolution_dropdown_window.create_window(
                    _screen_settings_window,
                    100,
                    40,
                    180,
                    (screen_settings->_avalible_resolutions.size() * MENU_ENTRY_HEIGHT),
                    DARK_GREY,
                    NONE,
                    MAP
                );
                _screen_resolution_options_vector.clear();

                for (int i = 0; i < screen_settings->_avalible_resolutions.size(); ++i)
                {
                    window option;
                    option.create_window(
                        _screen_resolution_dropdown_window,
                        0,
                        (_screen_resolution_options_vector.size() * MENU_ENTRY_HEIGHT),
                        _screen_resolution_dropdown_window.width(),
                        MENU_ENTRY_HEIGHT,
                        DARK_GREY,
                        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
                        MAP,
                        (int[]){RIGHT | LEFT | DOWN, 2, BLACK}
                    );
                    option.draw_text_auto_color(
                        screen_settings->_avalible_resolutions[i].second.c_str(),
                        4,
                        14
                    );
                    _screen_resolution_options_vector.push_back(option);
                }
            }

            if (__window == _input_device_dropdown_window)
            {
                _input_device_dropdown_window.create_window(
                    _input_settings_window,
                    _input_device_window.x(),
                    (_input_device_window.y() + _input_device_window.height()),
                    (_input_device_window.width() + _input_device_button_window.width()),
                    pointer_vec.size() * 20,
                    WHITE,
                    NONE,
                    MAP
                );

                for (int i = 0; i < pointer_vec.size(); ++i)
                {
                    pointer_vec[i]._window.create_window(
                        _input_device_dropdown_window,
                        0,
                        (i * 20),
                        _input_device_dropdown_window.width(),
                        20,
                        WHITE,
                        NONE,
                        NONE,
                        (int[]){LEFT | RIGHT | DOWN, 2, BLACK}
                    );
                    pointer_vec[i]._window.map();
                    pointer_vec[i]._window.draw_text_auto_color(pointer_vec[i]._device_name.c_str(), 4, 14, BLACK);
                }
            }
        }

        void hide__(uint32_t __window)
        {
            if (__window == _screen_resolution_dropdown_window)
            {
                _screen_resolution_dropdown_window.unmap();
                _screen_resolution_dropdown_window.kill();
            }

            if (__window == _input_device_dropdown_window)
            {
                _input_device_dropdown_window.unmap();
                _input_device_dropdown_window.kill();
            }
        }

        void setup_events__()
        {
            event_handler->setEventCallback(XCB_BUTTON_PRESS,     [this](Ev ev)-> void
            {
                RE_CAST_EV(xcb_button_press_event_t);
                if (e->detail == L_MOUSE_BUTTON)
                {
                    if (wm->focused_client != this->c)
                    {
                        if (e->event == _main_window
                        ||  e->event == _menu_window
                        ||  e->event == _default_settings_window
                        ||  e->event == _screen_menu_entry_window
                        ||  e->event == _screen_settings_window
                        ||  e->event == _audio_menu_entry_window
                        ||  e->event == _audio_settings_window
                        ||  e->event == _network_menu_entry_window
                        ||  e->event == _network_settings_window)
                        {
                            this->c->focus();
                            wm->focused_client = this->c;
                        }
                    }

                    if (e->event == _menu_window)
                    {
                        adjust_and_map_subwindow__(_default_settings_window);
                        return;
                    }

                    if (e->event == _screen_menu_entry_window)
                    {
                        adjust_and_map_subwindow__(_screen_settings_window);
                        return;
                    }

                    if (e->event == _screen_resolution_button_window)
                    {
                        if (!_screen_resolution_dropdown_window.is_mapped())
                        {
                            show__(_screen_resolution_dropdown_window);
                            return;
                        }

                        hide__(_screen_resolution_dropdown_window);
                        return;
                    }

                    if (e->event == _audio_menu_entry_window)
                    {
                        adjust_and_map_subwindow__(_audio_settings_window);
                        return;
                    }

                    if (e->event == _network_menu_entry_window)
                    {
                        adjust_and_map_subwindow__(_network_settings_window);
                        return;
                    }

                    if (e->event == _input_menu_entry_window)
                    {
                        adjust_and_map_subwindow__(_input_settings_window);
                    }

                    if (e->event == _input_device_button_window)
                    {
                        if (!_input_device_dropdown_window.is_mapped())
                        {
                            show__(_input_device_dropdown_window);
                            return;
                        }

                        hide__(_input_device_dropdown_window);
                        return;
                    }

                    for (int i(0); i < _screen_resolution_options_vector.size(); ++i)
                    {
                        if (e->event == _screen_resolution_options_vector[i])
                        {
                            screen_settings->set_resolution(screen_settings->_avalible_resolutions[i].first);
                            log_info("screen->width_in_pixels: " + to_string(screen->width_in_pixels));
                            log_info("screen->height_in_pixels: " + to_string(screen->height_in_pixels));
                        }
                    }
                }
            });

            event_handler->set_key_press_callback(SUPER, wm->key_codes.s, [this]()-> void { launch(); });

            event_handler->setEventCallback(XCB_CONFIGURE_NOTIFY, [this](Ev ev)->void
            {
                RE_CAST_EV(xcb_configure_notify_event_t); 
                configure(e->window, e->width, e->height);
            });

            event_handler->setEventCallback(XCB_EXPOSE,           [this](Ev ev)->void
            {
                RE_CAST_EV(xcb_expose_event_t);
                expose(e->window);
            });

            SET_INTR_CLI_KILL_CALLBACK();
        }

    public:
    // Variabels.
        window(_main_window),
            (_menu_window), (_default_settings_window),
            (_screen_menu_entry_window), (_screen_settings_window), (_screen_resolution_window), (_screen_resolution_button_window), (_screen_resolution_dropdown_window),
            (_audio_menu_entry_window), (_audio_settings_window),
            (_network_menu_entry_window), (_network_settings_window),
            (_input_menu_entry_window), (_input_settings_window), (_input_device_window), (_input_device_button_window), (_input_device_dropdown_window);
        
        vector<window>(_screen_resolution_options_vector);
        client *c = nullptr;

    // Methods.
        void launch()
        {
            make_windows__();
            make_internal_client__();
        }

        void expose(uint32_t __window)
        {
            if (__window == _screen_menu_entry_window ) _screen_menu_entry_window.draw_text_auto_color("Screen", 4, 14);
            if (__window == _screen_settings_window   ) _screen_settings_window.draw_text_auto_color("Resolution", (_screen_resolution_window.x() - (size("Resolution") * DEFAULT_FONT_WIDTH)), 35, BLACK);
            if (__window == _audio_menu_entry_window  ) _audio_menu_entry_window.draw_text_auto_color("Audio", 4, 14);
            if (__window == _network_menu_entry_window) _network_menu_entry_window.draw_text_auto_color("Network", 4, 14);
            if (__window == _input_menu_entry_window  ) _input_menu_entry_window.draw_text_auto_color("Input", 4, 14);
            if (__window == _input_settings_window    ) _input_settings_window.draw_text_auto_color("Device", (_input_device_window.x() - (size("Device") * DEFAULT_FONT_WIDTH)), 35);
            if (__window == _input_device_window      ) _input_device_window.draw_text_auto_color("Select Input Device.", 4, 15, BLACK);
            if (__window == _screen_resolution_window )
            {
                string resolution(screen_settings->_current_resoluton_string);
                if (resolution.empty()) return;
                _screen_resolution_window.draw_text_auto_color(resolution.c_str(), 4, 15);
            }

            for (int i(0); i < _screen_resolution_options_vector.size(); ++i)
            {
                if (__window == _screen_resolution_options_vector[i])
                {
                    _screen_resolution_options_vector[i].draw_text(
                        screen_settings->_avalible_resolutions[i].second.c_str(),
                        WHITE,
                        DARK_GREY,
                        DEFAULT_FONT,
                        MENU_ENTRY_TEXT_X,
                        MENU_ENTRY_TEXT_Y
                    );
                }
            }
        }

        void configure(uint32_t __window, uint32_t __width, uint32_t __height)
        {
            if (__window == _main_window)
            {
                _menu_window.height(__height);
                if (_default_settings_window.is_mapped()) _default_settings_window.width_height((__width - MENU_WINDOW_WIDTH), __height);
                if (_screen_settings_window.is_mapped() ) _screen_settings_window.width_height((__width - MENU_WINDOW_WIDTH), __height);
                if (_audio_settings_window.is_mapped()  ) _audio_settings_window.width_height((__width - MENU_WINDOW_WIDTH), __height);
                if (_network_settings_window.is_mapped()) _network_settings_window.width_height((__width - MENU_WINDOW_WIDTH), __height);
                if (_input_settings_window.is_mapped()  ) _input_settings_window.width_height((__width - MENU_WINDOW_WIDTH), __height);
                xcb_flush(conn);
            }
        }

        void init()
        {
            setup_events__();
        }

    // Constructor.
        __system_settings__() {}
};
static __system_settings__ *system_settings(nullptr);

class Dock
{
    public:
    // Constructor.
        Dock() {}
        
    // Variabels.
        context_menu(context_menu);
        window(main_window);
        buttons(buttons);
        uint32_t x = 0, y = 0, width = 48, height = 48;
        add_app_dialog_window(add_app_dialog_window);
    
    // Methods.
        void init()
        {
            main_window.create_default(screen->root, 0, 0, width, height);
            setup_dock();
            configure_context_menu();
            make_apps();
            add_app_dialog_window.init();
            add_app_dialog_window.search_window.add_enter_action([this]()-> void
            {
                launcher.program((char *) add_app_dialog_window.search_window.search_string.c_str());
            });

            context_menu.init();
            configure_events();
        }

        void add_app(const char *app_name)
        {
            if (!file.check_if_binary_exists(app_name)) return;
            apps.push_back(app_name);
        }

    private:
    // Variables.
        vector<const char *>(apps);
        Launcher(launcher);
        Logger(log);
        File(file);
        
    // Methods.
        void calc_size_pos()
        {
            int num_of_buttons(buttons.size());
            if (num_of_buttons == 0) num_of_buttons = 1;

            uint32_t calc_width = width * num_of_buttons;
            x = ((screen->width_in_pixels / 2) - (calc_width / 2));
            y = (screen->height_in_pixels - height);

            main_window.x_y_width_height(x, y, calc_width, height);
            xcb_flush(conn);
        }

        void setup_dock()
        {
            main_window.grab_button({
                { R_MOUSE_BUTTON, NULL }
            });

            main_window.set_backround_color(DARK_GREY);
            calc_size_pos();
            main_window.map();
        }

        void configure_context_menu()
        {
            context_menu.add_entry("Add app", [this]()-> void
            {
                add_app_dialog_window.show();
            });

            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
            context_menu.add_entry("test with nullptr", nullptr);
        }

        void make_apps()
        {
            for (const char *app : apps)
            {
                buttons.add(app, [app, this]()-> void
                {
                    launcher.program((char *) app);
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

        void configure_events()
        {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev)-> void
            {
                const auto *e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
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
};
static Dock * dock;

class mv_client
{
    // Defines.
        #define RIGHT_  screen->width_in_pixels  - c->width
        #define BOTTOM_ screen->height_in_pixels - c->height
   
    public:
    // Constructor.
        mv_client(client * c, int start_x, int start_y)
        : c(c), start_x(start_x), start_y(start_y)
        {
            if (c->win.is_EWMH_fullscreen()) return;

            pointer.grab();
            run();
            pointer.ungrab();
        }

    private:
    // Variabels.
        client(*c);
        pointer(pointer);
        int start_x, start_y;
        bool shouldContinue = true;
        xcb_generic_event_t(*ev);
        const double frameRate = 120.0;
        chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
        const double frameDuration = 1000.0 / frameRate;
    
    // Methods.
        void snap(int x, int y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (client * const &cli : wm->cur_d->current_clients)
            {
                if (cli == c) continue;
                
                // SNAP WINDOW TO 'RIGHT' BORDER OF 'NON_CONTROLLED' WINDOW
                if (x > cli->x + cli->width - N && x < cli->x + cli->width + N
                &&  y + c->height > cli->y && y < cli->y + cli->height)
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

                // SNAP WINDOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if (x + c->width > cli->x - N && x + c->width < cli->x + N       
                &&  y + c->height > cli->y && y < cli->y + cli->height)
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
                if (y > cli->y + cli->height - N && y < cli->y + cli->height + N 
                &&  x + c->width > cli->x && x < cli->x + cli->width)
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
                if (y + c->height > cli->y - N && y + c->height < cli->y + N     
                &&  x + c->width > cli->x && x < cli->x + cli->width)
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
            if      (((x < N) && (x > -N)) && ((y < N) && (y > -N)))                              c->frame.x_y(0, 0);
            else if  ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < N && y > -N))                    c->frame.x_y(RIGHT_, 0);
            else if  ((y < BOTTOM_ + N && y > BOTTOM_ - N) && (x < N && x > -N))                  c->frame.x_y(0, BOTTOM_);
            else if  ((x < N) && (x > -N))                                                        c->frame.x_y(0, y);
            else if   (y < N && y > -N)                                                           c->frame.x_y(x, 0);
            else if  ((x < RIGHT_ + N && x > RIGHT_ - N) && (y < BOTTOM_ + N && y > BOTTOM_ - N)) c->frame.x_y(RIGHT_, BOTTOM_);
            else if  ((x < RIGHT_ + N) && (x > RIGHT_ - N))                                       c->frame.x_y(RIGHT_, y);
            else if   (y < BOTTOM_ + N && y > BOTTOM_ - N)                                        c->frame.x_y(x, BOTTOM_);
            else                                                                                  c->frame.x_y(x, y);
        }

        void run()
        {
            while (shouldContinue)
            {
                ev = xcb_wait_for_event(conn);
                if (ev == nullptr) continue;

                switch (ev->response_type & ~0x80)
                {
                    case XCB_MOTION_NOTIFY:
                    {
                        const auto *e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                        int new_x = e->root_x - start_x - BORDER_SIZE;
                        int new_y = e->root_y - start_y - BORDER_SIZE;
                        
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

                    case XCB_EXPOSE:
                    {
                        RE_CAST_EV(xcb_expose_event_t);
                        status_bar->expose(e->window);
                        file_app->expose(e->window);
                        system_settings->expose(e->window);
                        
                        client *c = wm->client_from_any_window(&e->window);
                        if (c != nullptr)
                        {
                            if (e->window == c->titlebar)
                            {
                                c->draw_title(TITLE_INTR_DRAW);
                            }
                        }

                        break;
                    }

                    case XCB_PROPERTY_NOTIFY:
                    {
                        RE_CAST_EV(xcb_property_notify_event_t);
                        client *c = wm->client_from_any_window(&e->window);
                        if (c != nullptr)
                        {
                            if (e->atom   == ewmh->_NET_WM_NAME
                            &&  e->window == c->win)
                            {
                                c->draw_title(TITLE_REQ_DRAW);
                            }
                        }

                        break;
                    }
                }

                free(ev);
            }
        }

        bool isTimeToRender()
        {
            auto currentTime = chrono::high_resolution_clock::now();
            const chrono::duration<double, milli> elapsedTime = currentTime - lastUpdateTime;

            if (elapsedTime.count() >= frameDuration)
            {
                lastUpdateTime = currentTime;
                return true;
            }

            return false;
        }
};

mutex mtx;
class change_desktop
{
    public:
    // Constructor.
        change_desktop(xcb_connection_t *connection) {}

    // Variabels.
        int duration = 100;

        enum DIRECTION
        {
            NEXT,
            PREV
        };

    // Methods.
        void change_to(const DIRECTION &direction)
        {
            switch (direction)
            {
                case NEXT:
                {
                    if (wm->cur_d->desktop == wm->desktop_list.size()) return;
                    if (wm->focused_client != nullptr)
                    {
                        wm->cur_d->focused_client = wm->focused_client;
                    }

                    hide = get_clients_on_desktop(wm->cur_d->desktop);
                    show = get_clients_on_desktop(wm->cur_d->desktop + 1);
                    animate(show, NEXT);
                    animate(hide, NEXT);
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop];
                    if (wm->cur_d->focused_client != nullptr)
                    {
                        wm->cur_d->focused_client->focus();
                        wm->focused_client = wm->cur_d->focused_client;
                    }
                    else if (wm->cur_d->current_clients.size() > 0)
                    {
                        wm->cur_d->current_clients[0]->focus();
                        wm->focused_client = wm->cur_d->current_clients[0];
                    }
                    else
                    {
                        wm->focused_client = nullptr;
                        wm->root.focus_input();
                    }

                    break;
                }
            
                case PREV:
                {
                    if (wm->cur_d->desktop == 1) return;
                    if (wm->focused_client != nullptr)
                    {
                        wm->cur_d->focused_client = wm->focused_client;
                    }

                    hide = get_clients_on_desktop(wm->cur_d->desktop);
                    show = get_clients_on_desktop(wm->cur_d->desktop - 1);
                    animate(show, PREV);
                    animate(hide, PREV);
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop - 2];
                    if (wm->cur_d->focused_client != nullptr)
                    {
                        wm->cur_d->focused_client->focus();
                        wm->focused_client = wm->cur_d->focused_client;
                    }
                    else if (wm->cur_d->current_clients.size() > 0)
                    {
                        wm->cur_d->current_clients[0]->focus();
                        wm->focused_client = wm->cur_d->current_clients[0];
                    }
                    else
                    {
                        wm->focused_client = nullptr;
                        wm->root.focus_input();
                    }

                    break;
                }
            }

            mtx.lock();
            thread_sleep(duration + 20);
            joinAndClearThreads();
            mtx.unlock();
        }

        void change_with_app(const DIRECTION &direction)
        {
            if (wm->focused_client == nullptr) return;

            switch (direction)
            {
                case NEXT:
                {
                    if (wm->cur_d->desktop == wm->desktop_list.size()) return;
                    if (wm->focused_client->desktop != wm->cur_d->desktop) return;

                    hide = get_clients_on_desktop_with_app(wm->cur_d->desktop);
                    show = get_clients_on_desktop_with_app(wm->cur_d->desktop + 1);
                    animate(show, NEXT);
                    animate(hide, NEXT);
                    wm->cur_d->current_clients.erase(
                        remove(
                            wm->cur_d->current_clients.begin(),
                            wm->cur_d->current_clients.end(),
                            wm->focused_client
                        ),
                        wm->cur_d->current_clients.end()
                    );
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop];
                    wm->focused_client->desktop = wm->cur_d->desktop;
                    wm->cur_d->current_clients.push_back(wm->focused_client);

                    break;
                }
            
                case PREV:
                {
                    if (wm->cur_d->desktop == 1) return;
                    if (wm->focused_client->desktop != wm->cur_d->desktop) return;

                    hide = get_clients_on_desktop_with_app(wm->cur_d->desktop);
                    show = get_clients_on_desktop_with_app(wm->cur_d->desktop - 1);
                    animate(show, PREV);
                    animate(hide, PREV);
                    wm->cur_d->current_clients.erase(
                        remove(
                            wm->cur_d->current_clients.begin(),
                            wm->cur_d->current_clients.end(),
                            wm->focused_client
                        ),
                        wm->cur_d->current_clients.end()
                    );
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop - 2];
                    wm->focused_client->desktop = wm->cur_d->desktop;
                    wm->cur_d->current_clients.push_back(wm->focused_client);
                    
                    break;
                }
            }

            mtx.lock();
            thread_sleep(duration + 20);
            joinAndClearThreads();
            mtx.unlock();
        }

        static void teleport_to(const uint8_t & n)
        {
            if (wm->cur_d == wm->desktop_list[n - 1] || n == 0 || n == wm->desktop_list.size()) return;

            for (client *const &c:wm->cur_d->current_clients)
            {
                if (c != nullptr)
                {
                    if (c->desktop == wm->cur_d->desktop)
                    {
                        c->unmap();
                    }
                }
            }

            wm->cur_d = wm->desktop_list[n - 1];
            for (client *const &c : wm->cur_d->current_clients) 
            {
                if (c != nullptr)
                {
                    c->map();
                }
            }
        }
    
    private:
    // Variabels.
        // xcb_connection_t(*connection);
        vector<client *>(show);
        vector<client *>(hide);
        thread(show_thread);
        thread(hide_thread);
        atomic<bool>(stop_show_flag){false};
        atomic<bool>(stop_hide_flag){false};
        atomic<bool>(reverse_animation_flag){false};
        vector<thread>(animation_threads);

    // Methods.
        vector<client *> get_clients_on_desktop(const uint8_t &desktop)
        {
            vector<client *>(clients);
            for (client *const &c : wm->client_list)
            {
                if (c->desktop == desktop && c != wm->focused_client)
                {
                    clients.push_back(c);
                }
            }

            if (wm->focused_client != nullptr && wm->focused_client->desktop == desktop)
            {
                clients.push_back(wm->focused_client);
            }

            return clients;
        }

        vector<client *> get_clients_on_desktop_with_app(const uint8_t &desktop)
        {
            vector<client *>(clients);
            for (client *const &c : wm->client_list)
            {
                if (c->desktop == desktop && c != wm->focused_client) 
                {
                    clients.push_back(c);
                }
            }

            return clients;
        }

        void animate(vector<client *> clients, const DIRECTION &direction)
        {
            switch (direction)
            {
                case NEXT:
                {
                    for (client *const &c : clients)
                    {
                        if (c != nullptr)
                        {
                            animation_threads.emplace_back(&change_desktop::anim_cli, this, c, c->x - screen->width_in_pixels);
                        }
                    }

                    break;
                }

                case PREV:
                {
                    for (client *const &c : clients)
                    {
                        if (c != nullptr)
                        {
                            animation_threads.emplace_back(&change_desktop::anim_cli, this, c, c->x + screen->width_in_pixels);
                        }
                    }

                    break;
                }
            }
        }

        void anim_cli(client *c, const int &endx)
        {
            Mwm_Animator anim(c);
            anim.animate_client_x(
                c->x,
                endx,
                duration
            );
            c->update();
        }

        void thread_sleep(const double &milliseconds)
        {
            auto duration = chrono::duration<double, milli>(milliseconds);
            this_thread::sleep_for(duration);
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
            for (thread &t : animation_threads)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }

            animation_threads.clear();
            show.clear();
            hide.clear();

            vector<thread>().swap(animation_threads);
            vector<client *>().swap(show);
            vector<client *>().swap(hide);
        }
};

class resize_client
{
    public:
    // Constructor.
        resize_client(client * & c , int retard_int)/**
            
            THE REASON FOR THE 'retard_int' IS BECUSE WITHOUT IT 
            I CANNOT CALL THIS CLASS LIKE THIS 'resize_client(c)' 
            INSTEAD I WOULD HAVE TO CALL IT LIKE THIS 'resize_client rc(c)'
            AND NOW WITH THE 'retard_int' I CAN CALL IT LIKE THIS 'resize_client(c, 0)'
            
         */
        : c(c)
        {
            if (c->win.is_EWMH_fullscreen()) return;

            pointer.grab();
            pointer.teleport(c->x + c->width, c->y + c->height);
            run();
            pointer.ungrab();
        }

    // Subclasses.
        class no_border
        {
            public:
            // Constructor.
                no_border(client * & c, const uint32_t & x, const uint32_t & y)
                : c(c)
                {
                    if (c->win.is_EWMH_fullscreen()) return;

                    pointer.grab();
                    edge edge = wm->get_client_edge_from_pointer(c, 10);
                    teleport_mouse(edge);
                    run(edge);
                    pointer.ungrab();
                }
            
            private:
            // Variabels.
                client *&c;
                uint32_t x;
                pointer pointer;
                uint32_t y;
                const double frameRate = 120.0;
                chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
            
            // Methods.
                constexpr void teleport_mouse(edge edge)
                {
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

                void resize_client(const uint32_t x, const uint32_t y, edge edge)
                {
                    switch(edge)
                    {
                        case edge::LEFT:
                        {
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        }

                        case edge::RIGHT:
                        {
                            c->_width((x - c->x));
                            break;
                        }

                        case edge::TOP:
                        {
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        }

                        case edge::BOTTOM_edge:
                        {
                            c->_height((y - c->y));
                            break;
                        }

                        case edge::NONE:
                        {
                            return;
                        }

                        case edge::TOP_LEFT:
                        {
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        }

                        case edge::TOP_RIGHT:
                        {
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        }

                        case edge::BOTTOM_LEFT:
                        {
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        }

                        case edge::BOTTOM_RIGHT:{
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        }
                    }
                }

                void run(edge edge)
                {
                    xcb_generic_event_t *ev;
                    bool shouldContinue = true;
                    while (shouldContinue)
                    {
                        ev = xcb_wait_for_event(conn);
                        if (ev == nullptr) continue;

                        switch (ev->response_type & ~0x80)
                        {
                            case XCB_MOTION_NOTIFY:
                            {
                                const auto *e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
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

                        free(ev); 
                    }
                }

                bool isTimeToRender()
                {
                    const auto &currentTime = chrono::high_resolution_clock::now();
                    const chrono::duration<double, milli> &elapsedTime = currentTime - lastUpdateTime;

                    if (elapsedTime.count() >= frameDuration)
                    {
                        lastUpdateTime = currentTime;
                        return true;
                    }

                    return false;
                }
        };

        class border
        {
            public:
            // Constructor.
                border(client *&c, edge _edge)
                : c(c)
                {
                    if (c->win.is_EWMH_fullscreen()) return;

                    map<client *, edge> map = wm->get_client_next_to_client(c, _edge);
                    for (const auto &pair : map)
                    {
                        if (pair.first != nullptr)
                        {
                            c2 = pair.first;
                            c2_edge = pair.second;
                            pointer.grab();
                            teleport_mouse(_edge);
                            run_double(_edge);
                            pointer.ungrab();
                            return; 
                        }
                    }

                    pointer.grab();
                    teleport_mouse(_edge);
                    run(_edge);
                    pointer.ungrab();
                }

            private:
            // Variabels.
                client(*&c);
                client(*c2);
                edge(c2_edge);
                pointer(pointer); 
                const double frameRate = 120.0;
                chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
                private:

            // Methods.
                void teleport_mouse(edge edge)
                {
                    switch (edge)
                    {
                        case edge::TOP:
                        {
                            pointer.teleport(pointer.x(), (c->y + 1));
                            break;
                        }

                        case edge::BOTTOM_edge:
                        {
                            pointer.teleport(pointer.x(), ((c->y + c->height) - 1));
                            break;
                        }

                        case edge::LEFT:
                        {
                            pointer.teleport((c->x + 1), pointer.y());
                            break;
                        }

                        case edge::RIGHT:
                        {
                            pointer.teleport(((c->x + c->width) - 1), pointer.y());
                            break;
                        }

                        case edge::NONE:
                        {
                            break;
                        }

                        case edge::TOP_LEFT:
                        {
                            pointer.teleport((c->x + 1), (c->y + 1));
                            break;
                        }

                        case edge::TOP_RIGHT:
                        {
                            pointer.teleport(((c->x + c->width) - 2), (c->y + 2));
                            break;
                        }

                        case edge::BOTTOM_LEFT:
                        {
                            pointer.teleport((c->x + 1), ((c->y + c->height) - 1));
                            break;
                        }

                        case edge::BOTTOM_RIGHT:
                        {
                            pointer.teleport(((c->x + c->width) - 1), ((c->y + c->height) - 1));
                            break;
                        }
                    }
                }

                void resize_client(const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge)
                    {
                        case edge::LEFT:
                        {
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        }

                        case edge::RIGHT:
                        {
                            c->_width((x - c->x));
                            break;
                        }

                        case edge::TOP:
                        {
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        }

                        case edge::BOTTOM_edge:
                        {
                            c->_height((y - c->y));
                            break;
                        }

                        case edge::NONE:
                        {
                            return;
                        }

                        case edge::TOP_LEFT:
                        {
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        }
                        
                        case edge::TOP_RIGHT:
                        {
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        }

                        case edge::BOTTOM_LEFT:
                        {
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        }

                        case edge::BOTTOM_RIGHT:
                        {
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        }
                    }
                }
                
                void resize_client(client *c, const uint32_t x, const uint32_t y, edge edge)
                {
                    switch (edge)
                    {
                        case edge::LEFT:
                        {
                            c->x_width(x, (c->width + c->x - x));
                            break;
                        }

                        case edge::RIGHT:
                        {
                            c->_width((x - c->x));
                            break;
                        }
                        
                        case edge::TOP:
                        {
                            c->y_height(y, (c->height + c->y - y));   
                            break;
                        }

                        case edge::BOTTOM_edge:
                        {
                            c->_height((y - c->y));
                            break;
                        }

                        case edge::NONE:
                        {
                            return;
                        }
                        
                        case edge::TOP_LEFT:
                        {
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        }

                        case edge::TOP_RIGHT:
                        {
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        }

                        case edge::BOTTOM_LEFT:
                        {
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;
                        }

                        case edge::BOTTOM_RIGHT:
                        {
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        }
                    }
                }

                void snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox)
                {
                    uint16_t left_border(0), right_border(0), top_border(0), bottom_border(0);

                    // if ((c->width + c->x - x ) <= c->win.get_min_width()  || (c->width + c->x - x ) > (screen->width_in_pixels * 2) ) return;
                    // if ((c->height + c->y - y) <= c->win.get_min_height() || (c->height + c->y - y) > (screen->height_in_pixels * 2)) return; 
                    
                    for (client *const &c : wm->cur_d->current_clients)
                    {
                        if (c == this->c) continue;


                        left_border = c->x;
                        right_border = (c->x + c->width);
                        top_border = c->y;
                        bottom_border = (c->y + c->height);

                        if (edge != edge::RIGHT
                        &&  edge != edge::BOTTOM_RIGHT
                        &&  edge != edge::TOP_RIGHT)
                        {
                            if (x > right_border - prox && x < right_border + prox
                            &&  y > top_border && y < bottom_border)
                            {
                                resize_client(right_border, y, edge);
                                return;
                            }
                        }

                        if (edge != edge::LEFT
                        &&  edge != edge::TOP_LEFT
                        &&  edge != edge::BOTTOM_LEFT)
                        {
                            if (x > left_border - prox && x < left_border + prox
                            &&  y > top_border && y < bottom_border)
                            {
                                resize_client(left_border, y, edge);
                                return;
                            }
                        }

                        if (edge != edge::BOTTOM_edge 
                        &&  edge != edge::BOTTOM_LEFT 
                        &&  edge != edge::BOTTOM_RIGHT)
                        {
                            if (y > bottom_border - prox && y < bottom_border + prox
                            &&  x > left_border && x < right_border)
                            {
                                resize_client(x, bottom_border, edge);
                                return;
                            }
                        }

                        if (edge != edge::TOP
                        &&  edge != edge::TOP_LEFT
                        &&  edge != edge::TOP_RIGHT)
                        {
                            if (y > top_border - prox && y < top_border + prox
                            &&  x > left_border && x < right_border)
                            {
                                resize_client(x, top_border, edge);
                                return;
                            }
                        }
                    }

                    resize_client(x, y, edge);
                }

                void run(edge edge)
                {
                    xcb_generic_event_t *ev;
                    bool shouldContinue = true;
                    
                    while (shouldContinue)
                    {
                        if ((ev = xcb_wait_for_event(conn)) == nullptr) continue;

                        switch (ev->response_type & ~0x80)
                        {
                            case XCB_MOTION_NOTIFY:
                            {
                                RE_CAST_EV(xcb_motion_notify_event_t);
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

                            case XCB_EXPOSE:
                            {
                                RE_CAST_EV(xcb_expose_event_t);
                                status_bar->expose(e->window);
                                file_app->expose(e->window);
                                system_settings->expose(e->window);

                                client *c = wm->client_from_any_window(&e->window);
                                if (c != nullptr)
                                {
                                    if (e->window == c->titlebar)
                                    {
                                        c->draw_title(TITLE_INTR_DRAW);
                                    }
                                }

                                break;
                            }

                            case XCB_CONFIGURE_NOTIFY:
                            {
                                RE_CAST_EV(xcb_configure_notify_event_t);
                                file_app->configure(e->window, e->width, e->height);
                                system_settings->configure(e->window, e->width, e->height);

                                break;
                            }

                            case XCB_PROPERTY_NOTIFY:
                            {
                                RE_CAST_EV(xcb_property_notify_event_t);
                                client *c = wm->client_from_any_window(&e->window);
                                if (c != nullptr)
                                {
                                    if (e->atom   == ewmh->_NET_WM_NAME
                                    &&  e->window == c->win)
                                    {
                                        c->draw_title(TITLE_REQ_DRAW);
                                    }
                                }

                                break;
                            }
                        }

                        free(ev);
                    }
                }

                void run_double(edge edge, bool __double = false)
                {
                    xcb_generic_event_t *ev;
                    bool shouldContinue(true);
                    
                    while (shouldContinue)
                    {
                        ev = xcb_wait_for_event(conn);
                        if (ev == nullptr) continue;

                        switch (ev->response_type & ~0x80)
                        {
                            case XCB_MOTION_NOTIFY:
                            {
                                const auto *e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
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
                    const auto &currentTime = chrono::high_resolution_clock::now();
                    const chrono::duration<double, milli> & elapsedTime = currentTime - lastUpdateTime;
                    if (elapsedTime.count() >= frameDuration)
                    {
                        lastUpdateTime = currentTime; 
                        return true;
                    }

                    return false;
                }
        };

    private:
    // Variabels.
        client * & c;
        uint32_t x;
        pointer pointer;
        uint32_t y;
        const double frameRate = 120.0;
        chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
        const double frameDuration = 1000.0 / frameRate;
    
    // Methods.
        void snap(const uint16_t & x, const uint16_t & y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (const client *cli : wm->cur_d->current_clients)
            {
                if (cli == this->c) continue;
                
                // SNAP WINSOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
                if (x > cli->x - N && x < cli->x + N 
                &&  y + this->c->height > cli->y && y < cli->y + cli->height)
                {
                    c->width_height((cli->x - this->c->x), (y - this->c->y)); 
                    return;
                }

                // SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW
                if (y > cli->y - N && y < cli->y + N 
                &&  x + this->c->width > cli->x && x < cli->x + cli->width)
                {
                    c->width_height((x - this->c->x), (cli->y - this->c->y));
                    return;
                }
            }

            c->width_height((x - this->c->x), (y - this->c->y));
        }

        void run()
        {
            xcb_generic_event_t *ev;
            bool shouldContinue(true);

            while (shouldContinue)
            {
                ev = xcb_wait_for_event(conn);
                if (ev == nullptr) continue;
                
                switch (ev->response_type & ~0x80)
                {
                    case XCB_MOTION_NOTIFY:
                    {
                        auto e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
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
                    
                    case XCB_EXPOSE:
                    {
                        auto e = reinterpret_cast<const xcb_expose_event_t *>(ev);
                        status_bar->expose(e->window);

                        break;
                    }
                }

                free(ev);
            }
        }
     
        bool isTimeToRender()
        {
            const auto &currentTime = chrono::high_resolution_clock::now();
            const chrono::duration<double, milli> & elapsedTime = currentTime - lastUpdateTime;
            if (elapsedTime.count() >= frameDuration)
            {
                lastUpdateTime = currentTime;
                return true;
            }

            return false ;
        }
};

class max_win
{
    private:
    // Variabels.
        client(*c);

    // Methods.
        void max_win_animate(const int &endX, const int &endY, const int &endWidth, const int &endHeight)
        {
            animate_client(
                c, 
                endX, 
                endY, 
                endWidth, 
                endHeight, 
                MAXWIN_ANIMATION_DURATION
            );
            xcb_flush(conn);
        }
        
        void button_unmax_win()
        {
            if (c->max_button_ogsize.x > screen->width_in_pixels ) c->max_button_ogsize.x = screen->width_in_pixels  / 4;
            if (c->max_button_ogsize.y > screen->height_in_pixels) c->max_button_ogsize.y = screen->height_in_pixels / 4;

            if (c->max_button_ogsize.width  == 0 || c->max_button_ogsize.width  > screen->width_in_pixels ) c->max_button_ogsize.width  = screen->width_in_pixels  / 2;
            if (c->max_button_ogsize.height == 0 || c->max_button_ogsize.height > screen->height_in_pixels) c->max_button_ogsize.height = screen->height_in_pixels / 2;

            max_win_animate(
                c->max_button_ogsize.x,
                c->max_button_ogsize.y,
                c->max_button_ogsize.width,
                c->max_button_ogsize.height
            ); 
        }
        
        void button_max_win()
        {
            c->save_max_button_ogsize();
            max_win_animate(
                0,
                0,
                screen->width_in_pixels,
                screen->height_in_pixels
            );
        }
        
        void ewmh_max_win()
        {
            c->save_max_ewmh_ogsize();
            max_win_animate(
                - BORDER_SIZE,
                - TITLE_BAR_HEIGHT - BORDER_SIZE,
                screen->width_in_pixels + (BORDER_SIZE * 2),
                screen->height_in_pixels + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)
            );
            c->set_EWMH_fullscreen_state();
        }

        void ewmh_unmax_win()
        {
            if (c->max_ewmh_ogsize.width  > screen->width_in_pixels ) c->max_ewmh_ogsize.width  = screen->width_in_pixels  / 2;
            if (c->max_ewmh_ogsize.height > screen->height_in_pixels) c->max_ewmh_ogsize.height = screen->height_in_pixels / 2;
            
            if (c->max_ewmh_ogsize.x >= screen->width_in_pixels  - 1) c->max_ewmh_ogsize.x = ((screen->width_in_pixels / 2) - (c->max_ewmh_ogsize.width / 2) - BORDER_SIZE);
            if (c->max_ewmh_ogsize.y >= screen->height_in_pixels - 1) c->max_ewmh_ogsize.y = ((screen->height_in_pixels / 2) - (c->max_ewmh_ogsize.height / 2) - TITLE_BAR_HEIGHT - BORDER_SIZE);

            max_win_animate(
                c->max_ewmh_ogsize.x, 
                c->max_ewmh_ogsize.y, 
                c->max_ewmh_ogsize.width, 
                c->max_ewmh_ogsize.height
            );
            c->unset_EWMH_fullscreen_state();
        }

    public:
    // Variabels.
        enum max_win_type
        {
            BUTTON_MAXWIN,
            EWMH_MAXWIN 
        };

    // Constructor.
        max_win(client *c, max_win_type type)
        : c(c)
        {
            switch (type)
            {
                case EWMH_MAXWIN:
                {
                    if (c->is_EWMH_fullscreen())
                    {
                        ewmh_unmax_win();
                    }
                    else
                    {
                        ewmh_max_win();
                    }

                    break;
                }
                
                case BUTTON_MAXWIN:
                {
                    if (c->is_button_max_win())
                    {
                        button_unmax_win();
                    }
                    else
                    { 
                        button_max_win();
                    }

                    break; 
                }
            }
        }
};

/**
 *
 * @class tile
 * @brief Represents a tile obj.
 * 
 * The `tile` class is responsible for managing the tiling behavior of windows in the window manager.
 * It provides methods to tile windows to the left, right, *up, or *down positions on the screen.
 * The class also includes helper methods to check the current tile position of a window and set the size and position of a window.
 *
 */
class tile
{
    private:
    // Variabels.
        client(*c);

    // Methods.
        bool current_tile_pos(TILEPOS mode)/**
         *
         * @brief Checks if the current tile position of a window is the specified tile position.
         *
         * This method checks if the current tile position of a window is the specified tile position.
         * It takes a `TILEPOS` enum value as an argument, which specifies the tile position to check.
         * The method returns `true` if the current tile position is the specified tile position, and `false` otherwise.
         *
         * @param mode The tile position to check.
         * @return true if the current tile position is the specified tile position.
         * @return false if the current tile position is not the specified tile position.
         *
         */
        {
            switch (mode)
            {
                case TILEPOS::LEFT:
                {
                    if (c->x      == 0 
                    &&  c->y      == 0 
                    &&  c->width  == screen->width_in_pixels / 2 
                    &&  c->height == screen->height_in_pixels)
                    {
                        return true;
                    }

                    break;
                }

                case TILEPOS::RIGHT:
                {
                    if (c->x      == screen->width_in_pixels / 2 
                    &&  c->y      == 0 
                    &&  c->width  == screen->width_in_pixels / 2
                    &&  c->height == screen->height_in_pixels)
                    {
                        return true;
                    }

                    break;
                }    
                
                case TILEPOS::LEFT_DOWN:
                {
                    if (c->x      == 0
                    &&  c->y      == screen->height_in_pixels / 2
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2)
                    {
                        return true;
                    }

                    break;
                }
                
                case TILEPOS::RIGHT_DOWN:
                {
                    if (c->x      == screen->width_in_pixels  / 2
                    &&  c->y      == screen->height_in_pixels / 2
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2)
                    {    
                        return true;
                    }
                        
                    break;
                }
                
                case TILEPOS::LEFT_UP:
                {
                    if (c->x      == 0
                    &&  c->y      == 0
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2)
                    {
                        return true;
                    }
                    
                    break;
                }

                case TILEPOS::RIGHT_UP:
                {
                    if (c->x      == screen->width_in_pixels  / 2
                    &&  c->y      == 0
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2)
                    {
                        return true;
                    }

                    break;
                }
            }

            return false; 
        }

        void set_tile_sizepos(TILEPOS sizepos)/**
         *
         * @brief Sets the size and position of a window to a specific tile position.
         *
         * This method sets the size and position of a window to a specific tile position.
         * It takes a `TILEPOS` enum value as an argument, which specifies the tile position to set.
         * The method uses the `animate` method to animate the window to the specified tile position.
         *
         * @param sizepos The tile position to set.
         *
         */
        {
            switch (sizepos)
            {
                case TILEPOS::LEFT:
                {
                    animate(
                        0,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels
                    );
                    return;
                }

                case TILEPOS::RIGHT:
                {
                    animate(
                        screen->width_in_pixels / 2,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels
                    );
                    return;
                }

                case TILEPOS::LEFT_DOWN:
                {
                    animate(
                        0,
                        screen->height_in_pixels / 2,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                }

                case TILEPOS::RIGHT_DOWN:
                {
                    animate(
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                }

                case TILEPOS::LEFT_UP:
                {
                    animate(
                        0,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                }

                case TILEPOS::RIGHT_UP:
                {
                    animate(
                        screen->width_in_pixels / 2,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                }
            }
        }
        
        void restore_og_tile_pos()
        {
            animate(
                c->tile_ogsize.x,
                c->tile_ogsize.y,
                c->tile_ogsize.width,
                c->tile_ogsize.height
            );
        }

        void animate(const int &end_x, const int &end_y, const int &end_width, const int &end_height)
        {
            Mwm_Animator anim(c);
            anim.animate_client(
                c->x,
                c->y, 
                c->width, 
                c->height, 
                end_x,
                end_y, 
                end_width, 
                end_height, 
                TILE_ANIMATION_DURATION
            );
            c->update();
        }
  
    public:
    // Constructor.
        tile(client *&c, TILE tile)
        : c(c)
        {
            if (c->is_EWMH_fullscreen()) return;
            switch (tile)
            {
                case TILE::LEFT:
                {
                    // IF 'CURRENTLT_TILED' TO 'LEFT'
                    if (current_tile_pos(TILEPOS::LEFT))
                    {
                        restore_og_tile_pos();
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' TO 'RIGHT', 'LEFT_DOWN' OR 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT)
                    ||  current_tile_pos(TILEPOS::LEFT_DOWN)
                    ||  current_tile_pos(TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(TILEPOS::LEFT);
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' TO 'RIGHT_DOWN'
                    if (current_tile_pos(TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(TILEPOS::LEFT_DOWN);
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' TO 'RIGHT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT_UP))
                    {
                        set_tile_sizepos(TILEPOS::LEFT_UP);
                        return;
                    }

                    c->save_tile_ogsize();
                    set_tile_sizepos(TILEPOS::LEFT);
                    break;
                }
                    
                case TILE::RIGHT:
                {
                    // IF 'CURRENTLY_TILED' TO 'RIGHT'
                    if (current_tile_pos(TILEPOS::RIGHT))
                    {
                        restore_og_tile_pos();
                        return;
                    }
                    
                    // IF 'CURRENTLT_TILED' TO 'LEFT', 'RIGHT_DOWN' OR 'RIGHT_UP' 
                    if (current_tile_pos(TILEPOS::LEFT)
                    ||  current_tile_pos(TILEPOS::RIGHT_UP)
                    ||  current_tile_pos(TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(TILEPOS::RIGHT);
                        return;
                    }
                    
                    // IF 'CURRENTLT_TILED' 'LEFT_DOWN'
                    if (current_tile_pos(TILEPOS::LEFT_DOWN))
                    {
                        set_tile_sizepos(TILEPOS::RIGHT_DOWN);
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(TILEPOS::RIGHT_UP);
                        return;
                    }

                    c->save_tile_ogsize();
                    set_tile_sizepos(TILEPOS::RIGHT);
                    break;
                }
                
                case TILE::DOWN:
                {
                    // IF 'CURRENTLY_TILED' 'LEFT' OR 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::LEFT)
                    ||  current_tile_pos(TILEPOS::LEFT_UP))
                    {
                        set_tile_sizepos(TILEPOS::LEFT_DOWN);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR 'RIGHT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT) 
                    ||  current_tile_pos(TILEPOS::RIGHT_UP))
                    {
                        set_tile_sizepos(TILEPOS::RIGHT_DOWN);
                        return;
                    }
                    
                    // IF 'CURRENTLY_TILED' 'LEFT_DOWN' OR 'RIGHT_DOWN'
                    if (current_tile_pos(TILEPOS::LEFT_DOWN)
                    ||  current_tile_pos(TILEPOS::RIGHT_DOWN))
                    {
                        restore_og_tile_pos();
                        return;
                    }

                    break;
                }

                case TILE::UP:
                {
                    // IF 'CURRENTLY_TILED' 'LEFT'
                    if (current_tile_pos(TILEPOS::LEFT)
                    ||  current_tile_pos(TILEPOS::LEFT_DOWN))
                    {
                        set_tile_sizepos(TILEPOS::LEFT_UP);
                        return;
                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR RIGHT_DOWN
                    if (current_tile_pos(TILEPOS::RIGHT)
                    ||  current_tile_pos(TILEPOS::RIGHT_DOWN))
                    {
                        set_tile_sizepos(TILEPOS::RIGHT_UP);
                        return;
                    }

                    break;
                }
            }
        }
};

class Events
{
    public:
    // Constructor.
        Events() {}
    
    // Methods.
        void setup()
        {
            event_handler->setEventCallback(XCB_KEY_PRESS,         [&](Ev ev)-> void { key_press_handler(ev); });
            event_handler->setEventCallback(XCB_MAP_NOTIFY,        [&](Ev ev)-> void { map_notify_handler(ev); });
            event_handler->setEventCallback(XCB_MAP_REQUEST,       [&](Ev ev)-> void { map_req_handler(ev); });
            event_handler->setEventCallback(XCB_BUTTON_PRESS,      [&](Ev ev)-> void { button_press_handler(ev); });
            event_handler->setEventCallback(XCB_CONFIGURE_REQUEST, [&](Ev ev)-> void { configure_request_handler(ev); });
            event_handler->setEventCallback(XCB_FOCUS_IN,          [&](Ev ev)-> void { focus_in_handler(ev); });
            event_handler->setEventCallback(XCB_FOCUS_OUT,         [&](Ev ev)-> void { focus_out_handler(ev); });
            event_handler->setEventCallback(XCB_DESTROY_NOTIFY,    [&](Ev ev)-> void { destroy_notify_handler(ev); });
            event_handler->setEventCallback(XCB_UNMAP_NOTIFY,      [&](Ev ev)-> void { unmap_notify_handler(ev); });
            event_handler->setEventCallback(XCB_REPARENT_NOTIFY,   [&](Ev ev)-> void { reparent_notify_handler(ev); });
            event_handler->setEventCallback(XCB_ENTER_NOTIFY,      [&](Ev ev)-> void { enter_notify_handler(ev); });
            event_handler->setEventCallback(XCB_LEAVE_NOTIFY,      [&](Ev ev)-> void { leave_notify_handler(ev); });
            event_handler->setEventCallback(XCB_MOTION_NOTIFY,     [&](Ev ev)-> void { motion_notify_handler(ev); });
        }

    private:
    // Methods.
        void key_press_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_key_press_event_t);
            if (e->detail == wm->key_codes.t)
            {
                switch (e->state)
                {
                    case (CTRL + ALT):
                    {
                        wm->launcher.program((char *) "konsole");
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.q)
            {
                switch (e->state)
                {
                    case (SHIFT + ALT):
                    {
                        wm->quit(0);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.f11)
            {
                GET_CLIENT_FROM_WINDOW(e->event);
                max_win(c, max_win::EWMH_MAXWIN);
                return;
            }
            
            if (e->detail == wm->key_codes.n_1)
            {
                switch (e->state)
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(1);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.n_2)
            {
                switch (e->state)
                {
                    case ALT:
                    {
                        change_desktop::teleport_to(2);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.n_3)
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
            
            if (e->detail == wm->key_codes.n_4)
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
            
            if (e->detail == wm->key_codes.n_5)
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
            
            if (e->detail == wm->key_codes.r_arrow)
            {
                switch (e->state)
                {
                    case (SHIFT + CTRL + SUPER):
                    {
                        change_desktop cd(conn);
                        cd.change_with_app(change_desktop::NEXT);
                        return;
                    }
                    
                    case (CTRL + SUPER):
                    {
                        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::NEXT);
                        return;
                    }

                    case SUPER:
                    {
                        GET_CLIENT_FROM_WINDOW(e->event);
                        tile(c, TILE::RIGHT);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.l_arrow)
            {
                switch (e->state)
                {
                    case (SHIFT + CTRL + SUPER):
                    {
                        change_desktop cd(conn);
                        cd.change_with_app(change_desktop::PREV);
                        return;
                    }

                    case (CTRL + SUPER):
                    {
                        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::PREV);
                        return;
                    }
                    
                    case SUPER:
                    {
                        GET_CLIENT_FROM_WINDOW(e->event);
                        tile(c, TILE::LEFT);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.d_arrow)
            {
                switch (e->state)
                {
                    case SUPER:
                    {
                        GET_CLIENT_FROM_WINDOW(e->event);
                        tile(c, TILE::DOWN);
                        return;
                    }
                }
            }

            if (e->detail == wm->key_codes.u_arrow)
            {
                switch (e->state)
                {
                    case SUPER:
                    {
                        GET_CLIENT_FROM_WINDOW(e->event);
                        tile(c, TILE::UP);
                        return;
                    }
                }
            }

            if (e->detail == wm->key_codes.tab)
            {
                switch (e->state)
                {
                    case ALT:
                    {
                        wm->cycle_focus();
                        return;
                    }
                }
            }

            if (e->detail == wm->key_codes.k)
            {
                switch (e->state)
                {
                    case SUPER:
                    {
                        GET_CLIENT_FROM_WINDOW(e->event);
                        c->win.kill();
                        // c->win.x(BORDER_SIZE);
                        // c->win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
                        // xcb_flush(conn);

                        return;
                    }
                }
            }
        }

        void map_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_map_notify_event_t);
            client *c = wm->client_from_window(&e->window);
            if (c != nullptr)
            {
                c->update();
            } 
        }

        void map_req_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_map_request_event_t);
            client *c = wm->client_from_window(&e->window); 
            if (c != nullptr) return;
            wm->manage_new_client(e->window);
        }

        void button_press_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_button_press_event_t);
            client *c;
            if (BORDER_SIZE == 0)
            {
                c = wm->client_from_pointer(10);
                if (c == nullptr) return;
                
                if (e->detail == L_MOUSE_BUTTON)
                {
                    c->raise();
                    c->focus();
                    resize_client::no_border border(c, 0, 0);
                    wm->focused_client = c;
                }

                return;
            }

            if (e->event == wm->root)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    wm->context_menu->show();
                    return;
                }
            }

            c = wm->client_from_any_window(&e->event);
            if (c == nullptr) return;
                
            if (e->detail == L_MOUSE_BUTTON)
            {
                if (e->event == c->win)
                {
                    switch (e->state)
                    {
                        case ALT:
                        {
                            c->raise();
                            mv_client mv(c, e->event_x, e->event_y + 20);
                            c->focus();
                            wm->focused_client = c;
                            return;
                        }
                    }

                    if (wm->focused_client->atoms.is_modal) return;

                    c->raise();
                    c->focus();
                    wm->focused_client = c;
                    return;
                }
                
                if (e->event == c->titlebar)
                {
                    c->raise();
                    c->focus();
                    mv_client mv(c, e->event_x, e->event_y);
                    wm->focused_client = c;
                    return;
                }
                
                if (e->event == c->close_button)
                {
                    c->win.kill();
                    return;
                }
                
                if (e->event == c->max_button)
                {
                    max_win(c, max_win::BUTTON_MAXWIN);
                    return;
                }
                
                if (e->event == c->border.left)
                {
                    resize_client::border border(c, edge::LEFT);
                    return;
                }
                
                if (e->event == c->border.right)
                {
                    resize_client::border border(c, edge::RIGHT);
                    return;
                } 
                
                if (e->event == c->border.top)
                {
                    resize_client::border border(c, edge::TOP);
                    return;
                }
                
                if (e->event == c->border.bottom)
                {
                    resize_client::border border(c, edge::BOTTOM_edge);
                    return;
                }
                
                if (e->event == c->border.top_left)
                {
                    resize_client::border border(c, edge::TOP_LEFT);
                    return;
                }

                if (e->event == c->border.top_right)
                {
                    resize_client::border border(c, edge::TOP_RIGHT);
                    return;
                }
                
                if (e->event == c->border.bottom_left)
                {
                    resize_client::border border(c, edge::BOTTOM_LEFT);
                    return;
                }

                if (e->event == c->border.bottom_right)
                {
                    resize_client::border border(c, edge::BOTTOM_RIGHT);
                    return;
                }
            }

            if (e->detail == R_MOUSE_BUTTON)
            {
                switch (e->state)
                {
                    case ALT:
                    {
                        c->raise();
                        c->focus();
                        resize_client resize(c, 0);
                        wm->focused_client = c;
                        return;
                    }
                }
            }
        }
        
        void configure_request_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_configure_request_event_t);
            wm->data.width  = e->width;
            wm->data.height = e->height;
            wm->data.x      = e->x;
            wm->data.y      = e->y;
        }

        void focus_in_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_focus_in_event_t);
            GET_CLIENT_FROM_WINDOW(e->event);

            c->win.ungrab_button({
                { L_MOUSE_BUTTON, NULL }
            });

            c->raise();
            c->win.set_active_EWMH_window();
            wm->focused_client = c;
        }

        void focus_out_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_focus_out_event_t);
            GET_CLIENT_FROM_WINDOW(e->event);
            c->win.grab_button({
                { L_MOUSE_BUTTON, NULL }
            });
        }

        void destroy_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_destroy_notify_event_t);
            GET_CLIENT_FROM_WINDOW(e->event);
            wm->send_sigterm_to_client(c);
        }
        
        void unmap_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_unmap_notify_event_t);
            client *c = wm->client_from_window(&e->window);
            if (c == nullptr) return;
        }
        
        void reparent_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_reparent_notify_event_t);
            GET_CLIENT_FROM_WINDOW(e->window);

            c->win.x(BORDER_SIZE);
            c->win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
            xcb_flush(conn);
        }

        void enter_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_enter_notify_event_t);
        }

        void leave_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_leave_notify_event_t);   
        }

        void motion_notify_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_motion_notify_event_t);
        }
};

class test
{
    private:
    // Methods.
        void setup_events()
        {
            event_handler->setEventCallback(XCB_KEY_PRESS, [this](Ev ev)-> void
            {
                const xcb_key_press_event_t *e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                
                if (e->detail == wm->key_codes.k)
                {
                    if (e->state == (SUPER + ALT))
                    {
                        running = 1;
                        run_full();
                    }
                }

                if (e->detail == wm->key_codes.q)
                {
                    if (e->state == ALT)
                    {   
                        running = 0;
                    }
                }
            });
        }
        
        void run_full()
        {
            cd_test();
            mv_test();
        }
        
        void cd_test()
        {
            // first test
            int i(0), end(100);
            change_desktop cd(conn);
            const int og_duration = cd.duration;

            while (i < (end + 1))
            {
                cd.change_to(change_desktop::NEXT);
                cd.change_to(change_desktop::PREV);
                cd.duration = (cd.duration - 1);
                ++i;
                if (!running) break;
            }
        }

        void mv_test()
        {
            window(win_1);
            win_1.create_default(
                wm->root,
                0,
                0,
                300,
                300
            );
            win_1.raise();
            win_1.set_backround_color(RED);
            win_1.map();
            Mwm_Animator win_1_animator(win_1);
            
            for (int i = 0; i < 400; ++i)
            {
                win_1_animator.animate(
                    0,
                    0,
                    300,
                    300,
                    (screen->width_in_pixels - 300),
                    0,
                    300,
                    300,
                    (400 - i)
                );
                win_1_animator.animate(
                    (screen->width_in_pixels - 300),
                    0,
                    300, 
                    300, 
                    (screen->width_in_pixels - 300),
                    (screen->height_in_pixels - 300),
                    300,
                    300,
                    (400 - i)
                );
                win_1_animator.animate(
                    (screen->width_in_pixels - 300),
                    (screen->height_in_pixels - 300), 
                    300, 
                    300, 
                    0,
                    (screen->height_in_pixels - 300),
                    300, 
                    300, 
                    (400 - i)
                );
                win_1_animator.animate(
                    0,
                    (screen->height_in_pixels - 300),
                    300, 
                    300, 
                    0, 
                    0, 
                    300, 
                    300,
                    (400 - i)
                );

                if (!running) break;
            }
            
            win_1.unmap();
            win_1.kill(); 
        }

    public:
    // Variabels.
        int running = 1;

    // Methods.
        void init()
        {
            setup_events();
        }

    // Constructor.
        test() {}
};

void setup_wm()
{
    wm = new Window_Manager;
    wm->init();
    
    change_desktop::teleport_to(1);
    dock = new Dock;
    dock->add_app("konsole");
    dock->add_app("alacritty");
    dock->add_app("google-chrome-stable");
    dock->add_app("code");
    dock->add_app("falkon");
    dock->init();

    mwm_runner = new Mwm_Runner;
    mwm_runner->init();

    Events events;
    events.setup();

    file_app = new __file_app__;
    file_app->init();

    status_bar = new __status_bar__;
    status_bar->init();

    wifi = new __wifi__;
    wifi->init();

    network = new __network__;

    screen_settings = new __screen_settings__;
    screen_settings->init();
    
    system_settings = new __system_settings__;
    system_settings->init();
}

int main()
{
    net_logger = new __net_logger__;
    net_logger->init(ESP_SERVER);
    NET_LOG("Starting mwm.");

    function<void()> audio_thread = [&]()-> void
    {
        audio.init();
        audio.run();
    };
    thread(audio_thread).detach();

    LOG_start()
    setup_wm();

    audio.list_sinks();

    test tester;
    tester.init();

    event_handler->run();
    xcb_disconnect(conn);
    return 0;
}