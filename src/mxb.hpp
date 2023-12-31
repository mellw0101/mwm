#ifndef MXB_HPP
#define MXB_HPP
#include "include.hpp"
#include <netinet/in.h>

int
check(xcb_void_cookie_t cookie);

int
mxb_create_win(xcb_window_t win, const xcb_window_t & parent, const uint16_t & x, const uint16_t & y, const uint16_t & width, const uint16_t & height);

void 
mxb_kill_client(xcb_window_t window);

void
getCurrentEventMask(xcb_connection_t* conn, xcb_window_t window);

#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <X11/Xauth.h>

// Structure to hold authentication information
struct xpp_auth_info_t 
{
    int namelen;
    char* name;
    int datalen;
    char* data;
};

class XConnection 
{
    public:
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

    private:
        int fd;
        struct sockaddr_un addr;
        xpp_auth_info_t auth_info;
        Logger log;

        // Function to authenticate an X11 connection
        bool 
        authenticate_x11_connection(int display_number, xpp_auth_info_t& auth_info) 
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
};

XConnection * 
mxb_connect(const char* display);

int
mxb_connection_has_error(XConnection * conn);

#endif /* MXB_HPP */