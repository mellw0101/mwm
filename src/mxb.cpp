#include "mxb.hpp"
#include <X11/Xauth.h>

namespace 
{
    static xcb_connection_t *conn;
    static xcb_screen_t *screen;
    xcb_generic_error_t * err;
    Logger log;
}

int
check(xcb_void_cookie_t cookie)
{
    err = xcb_request_check(conn, cookie);
    if (err)
    {
        const int & error_code = err->error_code;
        free(err);
        return error_code;
    }
    return 0;
}

int
mxb_create_win(xcb_window_t win, const xcb_window_t & parent, const uint16_t & x, const uint16_t & y, const uint16_t & width, const uint16_t & height)
{
    win = xcb_generate_id(conn);
    xcb_void_cookie_t cookie = xcb_create_window
    (
        conn, 
        XCB_COPY_FROM_PARENT, 
        win, 
        screen->root, 
        x,
        y, 
        width, 
        height, 
        0, 
        XCB_WINDOW_CLASS_INPUT_OUTPUT, 
        screen->root_visual, 
        0, 
        NULL
    );
    return check(cookie);
}

void 
mxb_kill_client(xcb_window_t window) 
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
getCurrentEventMask(xcb_connection_t* conn, xcb_window_t window) 
{
    // Get the window attributes
    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(conn, window);
    xcb_get_window_attributes_reply_t* attr_reply = xcb_get_window_attributes_reply(conn, attr_cookie, NULL);

    // Check if the reply is valid
    if (attr_reply) 
    {
        log_info("event_mask: " + std::to_string(attr_reply->your_event_mask));
        log_info("all_event_masks: " + std::to_string(attr_reply->all_event_masks));
        free(attr_reply);
    }
    else 
    {
        log_error("Unable to get window attributes.");
    }
}

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/un.h>

// Function to send a simple message to the X server
void 
sendXMessage() 
{
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket." << std::endl;
        return;
    }

    // Define the address of the X server (usually localhost:6000)
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(6000); // Default X server port
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost

    // Connect to the X server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error connecting to the X server." << std::endl;
        return;
    }

    // Compose a simple X protocol message (e.g., QueryVersion)
    // This is a highly simplified example. Real messages require precise formatting.
    char message[] = { /* ... X protocol bytes ... */ };

    // Send the message
    if (write(sockfd, message, sizeof(message)) < 0) {
        std::cerr << "Error writing to socket." << std::endl;
        return;
    }

    // Close the socket
    close(sockfd);
}

void 
configure_window(int window_id, int x, int y, int width, int height) 
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(6000); // X server usually runs on port 6000
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr); // X server is usually on localhost

    int STATUS = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (STATUS != 0) 
    {
        // handle error
    }

    char message[32];
    message[0] = 12; // opcode for ConfigureWindow
    // convert window_id to 4-byte value and put it in message[4] to message[7]
    message[4] = (window_id >> 24) & 0xFF;
    message[5] = (window_id >> 16) & 0xFF;
    message[6] = (window_id >> 8) & 0xFF;
    message[7] = window_id & 0xFF;
    // convert x, y, width, and height to 4-byte values and put them in the message
    message[8] = (x >> 24) & 0xFF;
    message[9] = (x >> 16) & 0xFF;
    message[10] = (x >> 8) & 0xFF;
    message[11] = x & 0xFF;
    message[12] = (y >> 24) & 0xFF;
    message[13] = (y >> 16) & 0xFF;
    message[14] = (y >> 8) & 0xFF;
    message[15] = y & 0xFF;
    message[16] = (width >> 24) & 0xFF;
    message[17] = (width >> 16) & 0xFF;
    message[18] = (width >> 8) & 0xFF;
    message[19] = width & 0xFF;
    message[20] = (height >> 24) & 0xFF;
    message[21] = (height >> 16) & 0xFF;
    message[22] = (height >> 8) & 0xFF;
    message[23] = height & 0xFF;

    send(sockfd, message, sizeof(message), 0);

    char response[32];
    recv(sockfd, response, sizeof(response), 0);
    // interpret the response

    close(sockfd);
}

class UnixSocket 
{
    public:
        UnixSocket(const char* file) 
        {
            // Initialize address
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, file, sizeof(addr.sun_path) - 1);

            // Create socket
            fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd == -1) 
            {
                throw std::runtime_error("Failed to create socket");
            }

            // Connect
            if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) 
            {
                ::close(fd);
                throw std::runtime_error("Failed to connect");
            }
        }

        ~UnixSocket() 
        {
            if (fd != -1) 
            {
                ::close(fd);
            }
        }

        int getFd() const 
        {
            return fd;
        }

    private:
        int fd;
        struct sockaddr_un addr;
};

#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

XConnection * 
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

int
mxb_connection_has_error(XConnection * conn)
{
    try 
    {
        conn->confirmConnection();
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

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <memory>
#include <cerrno>

class XConnection_iso_cpp 
{
    public:
        explicit XConnection_iso_cpp(const std::string& display = ":0")
        {
            std::string socketPath = getSocketPath(display);

            // Initialize address
            addr = {}; // Zero-initialize the structure
            addr.sun_family = AF_UNIX;
            if (socketPath.length() >= sizeof(addr.sun_path))
            {
                throw std::runtime_error("Socket path is too long");
            }
            std::copy(socketPath.begin(), socketPath.end(), addr.sun_path);

            // Create socket
            fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd == -1) 
            {
                throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
            }

            // Connect to the X server's socket
            if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) 
            {
                ::close(fd);
                throw std::runtime_error("Failed to connect to X server: " + std::string(strerror(errno)));
            }

            // Additional setup and authentication would go here
        }

        ~XConnection_iso_cpp() 
        {
            if (fd != -1) 
            {
                ::close(fd);
            }
        }

        int getFd() const 
        {
            return fd;
        }

    private:
        int fd;
        struct sockaddr_un addr;

        std::string getSocketPath(const std::string& display) 
        {
            // Extract the display number from the display string
            size_t colonPos = display.find(':');
            if (colonPos == std::string::npos || colonPos == display.length() - 1) 
            {
                throw std::runtime_error("Invalid display format");
            }

            int displayNumber = std::stoi(display.substr(colonPos + 1));
            return "/tmp/.X11-unix/X" + std::to_string(displayNumber);
        }
};

std::unique_ptr<XConnection> iso_cpp_xcb_connect(const char* display) 
{
    try 
    {
        return std::make_unique<XConnection>(display);
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return nullptr;
    }
}
