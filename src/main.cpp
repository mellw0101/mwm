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

#include "Log.hpp"
Logger log;
#include "defenitions.hpp"
#include "structs.hpp"

static xcb_connection_t * conn;
static xcb_ewmh_connection_t * ewmh; 
static const xcb_setup_t * setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t * screen;


class mxb {
    public: 
        class XConnection {
            public:
                struct mxb_auth_info_t {
                    int namelen;
                    char* name;
                    int datalen;
                    char* data;
                };
                XConnection(const char * display) {
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
                ~XConnection() {
                    if (fd != -1) 
                    {
                        ::close(fd);
                    }
                    delete[] auth_info.name;
                    delete[] auth_info.data;
                }
                int getFd() const {
                    return fd;
                }
                void confirmConnection() {
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
                std::string sendMessage(const std::string & extensionName) {
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
                
                bool authenticate_x11_connection(int display_number, mxb_auth_info_t & auth_info) { // Function to authenticate an X11 connection
                    const char* xauthority_env = std::getenv("XAUTHORITY"); // Try to get the XAUTHORITY environment variable; fall back to default
                    std::string xauthority_file = xauthority_env ? xauthority_env : "~/.Xauthority";

                    FILE* auth_file = fopen(xauthority_file.c_str(), "rb"); // Open the Xauthority file
                    if (!auth_file) { // Handle error: Failed to open .Xauthority file
                        
                        return false;
                    }

                    Xauth* xauth_entry;
                    bool found = false;
                    while ((xauth_entry = XauReadAuth(auth_file)) != nullptr) {
                        // Check if the entry matches your display number
                        // Assuming display_number is the display you're interested in
                        if (std::to_string(display_number) == std::string(xauth_entry->number, xauth_entry->number_length)) {
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
                std::string getSocketPath(const char * display) {
                    std::string displayStr;

                    if (display == nullptr) {
                        char* envDisplay = std::getenv("DISPLAY");
                        
                        if (envDisplay != nullptr) {
                            displayStr = envDisplay;
                        } else {
                            displayStr = ":0";
                        }
                    } else {
                        displayStr = display;
                    }

                    int displayNumber = 0;
                    size_t colonPos = displayStr.find(':'); // Extract the display number from the display string
                    if (colonPos != std::string::npos) {
                        displayNumber = std::stoi(displayStr.substr(colonPos + 1));
                    }

                    return "/tmp/.X11-unix/X" + std::to_string(displayNumber);
                }
                int parseDisplayNumber(const char * display) {
                    if (!display) {
                        display = std::getenv("DISPLAY");
                    }

                    if (!display) {
                        return 0;  // default to display 0
                    }

                    const std::string displayStr = display;
                    size_t colonPos = displayStr.find(':');
                    if (colonPos != std::string::npos) {
                        return std::stoi(displayStr.substr(colonPos + 1));
                    }

                    return 0;  // default to display 0
                }
            ;
        };
        static XConnection * mxb_connect(const char* display) {
            try {
                return new XConnection(display);
            } catch (const std::exception & e) {
                // Handle exceptions or errors here
                std::cerr << "Connection error: " << e.what() << std::endl;
                return nullptr;
            }
        }
        static int mxb_connection_has_error(XConnection * conn) {
            try {
                // conn->confirmConnection();
                std::string response = conn->sendMessage("BIG-REQUESTS");
                log_info(response);
            } catch (const std::exception & e) {
                log_error(e.what());
                return 1;
            }
            return 0;
        }
        class File {
            public: // subclasses
                class search {
                    public: // construcers and operators
                        search(const std::string& filename)
                        : file(filename) {}

                        operator bool() const {
                            return file.good();
                        }
                    ;
                    private: // variables
                        std::ifstream file;
                    ;
                };
            ;
        };
    ;
};
class pointer {
    public: // methods
        uint32_t x() {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t * reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply) {
                log_error("reply is nullptr.");
                return 0;                            
            } 

            uint32_t x;
            x = reply->root_x;
            free(reply);
            return x;
        }
        uint32_t y() {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t * reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply) {
                log_error("reply is nullptr.");
                return 0;                            
            } 

            uint32_t y;
            y = reply->root_y;
            free(reply);
            return y;
        }
        void teleport(const int16_t & x, const int16_t & y) {
            xcb_warp_pointer(
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
        void grab(const xcb_window_t & window) {
            xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(
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
            if (!reply) {
                log_error("reply is nullptr.");
                free(reply);
                return;
            }
            if (reply->status != XCB_GRAB_STATUS_SUCCESS) {
                log_error("Could not grab pointer");
                free(reply);
                return;
            }
            free(reply);
        }
    ;
    private: // functions
        const char * pointer_from_enum(CURSOR CURSOR) {
            switch (CURSOR) {
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
    private: // variables
        Logger log;
    ;
};
class fast_vector {
    public: // operators
        operator std::vector<const char*>() const {
            return data;
        }
    ;
    public: // Destructor
        ~fast_vector() {
            for (auto str : data)  {
                delete[] str;
            }
        }
    ;
    public: // [] operator Access an element in the vector
        const char* operator[](size_t index) const {
            return data[index];
        }
    ;
    public: // methods
        void push_back(const char* str) {
            char* copy = new char[strlen(str) + 1];
            strcpy(copy, str);
            data.push_back(copy);
        }
        void append(const char* str) {
            char* copy = new char[strlen(str) + 1];
            strcpy(copy, str);
            data.push_back(copy);
        }
        size_t size() const {
            return data.size();
        }
        size_t index_size() const {
            if (data.size() == 0) {
                return 0;
            }

            return data.size() - 1;
        }
        void clear() {
            data.clear();
        }
    ;
    private: // variabels
        std::vector<const char*> data; // Internal vector to store const char* strings
    ;
};
class string_tokenizer {
    public: // constructors and destructor
        string_tokenizer() {}
        
        string_tokenizer(const char* input, const char* delimiter) {
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
        ~string_tokenizer() {
            delete[] str;
        }
    ;
    public: // methods
        const fast_vector & tokenize(const char* input, const char* delimiter) {
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
        const fast_vector & get_tokens() const {
            return tokens;
        }
        void clear() {
            tokens.clear();
        }
    ;
    private: // variables
        char* str;
        fast_vector tokens;
    ;
};
class str {
    public: // Constructor
        str(const char* str = "") {
            length = strlen(str);
            data = new char[length + 1];
            strcpy(data, str);
        }
    ;
    public: // Copy constructor
        str(const str& other) {
            length = other.length;
            data = new char[length + 1];
            strcpy(data, other.data);
        }
    ;
    public: // Move constructor
        str(str&& other) noexcept 
        : data(other.data), length(other.length) {
            other.data = nullptr;
            other.length = 0;
        }
    ;
    public: // Destructor
        ~str() {
            delete[] data;
        }
    ;
    public: // Copy assignment operator
        str& operator=(const str& other) {
            if (this != &other) {
                delete[] data;
                length = other.length;
                data = new char[length + 1];
                strcpy(data, other.data);
            }
            return *this;
        }
    ;
    public: // Move assignment operator
        str& operator=(str&& other) noexcept {
            if (this != &other) {
                delete[] data;
                data = other.data;
                length = other.length;
                other.data = nullptr;
                other.length = 0;
            }
            return *this;
        }
    ;
    public: // Concatenation operator
        str operator+(const str& other) const {
            str result;
            result.length = length + other.length;
            result.data = new char[result.length + 1];
            strcpy(result.data, data);
            strcat(result.data, other.data);
            return result;
        }
    ;
    public: // methods
        const char * c_str() const { // Access to underlying C-string
            return data;
        }        
        size_t size() const { // Get the length of the string
            return length;
        }
        bool isEmpty() const { // Method to check if the string is empty
            return length == 0;
        }
        bool is_nullptr() const {
            if (data == nullptr) {
                delete [] data;
                return true;
            }
            return false;
        }
    ;
    private: // variables
        char* data;
        size_t length;
        bool is_null = false;
    ;
};
class fast_str_vector {
    public: // operators
        operator std::vector<str>() const {
            return data;
        }
    ;
    public: // [] operator Access an element in the vector
        str operator[](size_t index) const {
            return data[index];
        }
    ;
    public: // methods
        void push_back(str str) { // Add a string to the vector
            data.push_back(str);
        }
        void append(str str) { // Add a string to the vector
            data.push_back(str);
        }
        size_t size() const { // Get the size of the vector
            return data.size();
        }
        size_t index_size() const { // get the index of the last element in the vector
            if (data.size() == 0)
            {
                return 0;
            }

            return data.size() - 1;
        }
        void clear() { // Clear the vector
            data.clear();
        }
    ;
    private: // variabels
        std::vector<str> data; // Internal vector to store const char* strings
    ;
};
class Directory_Searcher {
    public:
        Directory_Searcher() {}

        void search(const std::vector<const char *>& directories, const std::string& searchString) {
            results.clear();
            searchDirectories = directories;

            for (const auto& dir : searchDirectories) {
                DIR *d = opendir(dir);
                if (d == nullptr) {
                    log_error("opendir() failed for directory: " + std::string(dir));
                    continue;
                }

                struct dirent *entry;
                while ((entry = readdir(d)) != nullptr) {
                    std::string fileName = entry->d_name;
                    if (fileName.find(searchString) != std::string::npos) {
                        bool already_found = false;
                        for (const auto & result : results) {
                            if (result == fileName) {
                                already_found = true;
                                break;
                            }
                        }
                        if (!already_found) {
                            results.push_back(fileName);
                        }
                    }
                }

                closedir(d);
            }
        }
        const std::vector<std::string>& getResults() const {
            return results;
        }
    ;
    private:
        std::vector<const char *> searchDirectories;
        std::vector<std::string> results;
        Logger log;
    ;
};
class Directory_Lister {
    public:
        Directory_Lister() {}
    ;
    public: // methods
        std::vector<std::string> list(const std::string& Directory) {
            std::vector<std::string> results;

            DIR *d = opendir(Directory.c_str());
            if (d == nullptr) {
                log_error("Failed to open Directory: " + Directory);
                return results;
            }

            struct dirent *entry;
            while ((entry = readdir(d)) != nullptr) {
                std::string fileName = entry->d_name;
                results.push_back(fileName);
            }
            closedir(d);
            return results;   
        }
    ;
    private:
        Logger log;
    ;
};
class File {
    public: // subclasses
        class search {
            public: // construcers and operators
                search(const std::string& filename)
                : file(filename) {}

                operator bool() const {
                    return file.good();
                }
            ;
            private: // private variables
                std::ifstream file;
            ;
        };
    ;
    public: // construcers
        File() {}
    ;
    public: // variabels
        Directory_Lister directory_lister;
    ;
    public: // methods
        std::string find_png_icon(std::vector<const char *> dirs, const char * app) {
            std::string name = app;
            name += ".png";

            for (const auto & dir : dirs) {
                std::vector<std::string> files = list_dir_to_vector(dir);
                for (const auto & file : files) {
                    if (file == name) {
                        return dir + file;
                    }
                }
            }
            return "";
        }
        std::string findPngFile(const std::vector<const char *>& dirs, const char * name) {
            for (const auto& dir : dirs) {
                if (std::filesystem::is_directory(dir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                        if (entry.is_regular_file()) {
                            std::string filename = entry.path().filename().string();
                            if (filename.find(name) != std::string::npos && filename.find(".png") != std::string::npos) {
                                return entry.path().string();
                            }
                        }
                    }
                }
            }
            return "";
        }
        bool check_if_binary_exists(const char * name) {
            std::vector<const char *> dirs = split_$PATH_into_vector();
            return check_if_file_exists_in_DIRS(dirs, name);
        }
        std::vector<std::string> search_for_binary(const char * name) {
            ds.search({ "/usr/bin" }, name);
            return ds.getResults();
        }
        std::string get_current_directory() {
            return get_env_var("PWD");
        }
    ;
    private: // variables
        Logger log;
        string_tokenizer st;
        Directory_Searcher ds;
    ;
    private: // functions
        bool check_if_file_exists_in_DIRS(std::vector<const char *> dirs, const char * app) {
            std::string name = app;

            for (const auto & dir : dirs) {
                std::vector<std::string> files = list_dir_to_vector(dir);
                for (const auto & file : files) {
                    if (file == name) {
                        return true;
                    }
                }
            }
            return false;
        }
        std::vector<std::string> list_dir_to_vector(const char * directoryPath) {
            std::vector<std::string> files;
            DIR* dirp = opendir(directoryPath);
            if (dirp) {
                struct dirent* dp;
                while ((dp = readdir(dirp)) != nullptr) {
                    files.push_back(dp->d_name);
                }
                closedir(dirp);
            }
            return files;
        }
        std::vector<const char *> split_$PATH_into_vector() {
            str $PATH(get_env_var("PATH"));
            return st.tokenize($PATH.c_str(), ":");
        }
        const char * get_env_var(const char * var) {
            const char * _var = getenv(var);
            if (_var == nullptr) {
                return nullptr;
            }
            return _var;
        }
    ;
};
class Launcher {
    public:
        void program(char * program) {
            if (!file.check_if_binary_exists(program)) {
                return;
            }
            if (fork() == 0) {
                setsid();
                execvp(program, (char *[]) { program, NULL });
            }
        }
    ;
    private:
        File file;
    ;
};
using Ev = const xcb_generic_event_t *;
class Event_Handler {
    public: // methods
        using EventCallback = std::function<void(Ev)>;
        
        void run() {
            xcb_generic_event_t *ev;
            shouldContinue = true;

            while (shouldContinue) {
                ev = xcb_wait_for_event(conn);
                if (!ev) {
                    continue;
                }

                uint8_t responseType = ev->response_type & ~0x80;
                auto it = eventCallbacks.find(responseType);
                if (it != eventCallbacks.end()) {
                    for (const auto& callback : it->second) {
                        callback.second(ev);
                    }
                }

                free(ev);
            }
        }
        void end() {
            shouldContinue = false;
        }
        using CallbackId = int;

        CallbackId setEventCallback(uint8_t eventType, EventCallback callback) {
            CallbackId id = nextCallbackId++;
            eventCallbacks[eventType].emplace_back(id, std::move(callback));
            return id;
        }
        void removeEventCallback(uint8_t eventType, CallbackId id) {
            auto& callbacks = eventCallbacks[eventType];
            callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                        [id](const auto& pair) { return pair.first == id; }),
                            callbacks.end());
        }
    ;
    private: // variables
        std::unordered_map<uint8_t, std::vector<std::pair<CallbackId, EventCallback>>> eventCallbacks;
        bool shouldContinue = false;
        CallbackId nextCallbackId = 0;
    ;
}; static Event_Handler * event_handler;
class Bitmap {
    public: // constructor
        Bitmap(int width, int height) 
        : width(width), height(height), bitmap(height, std::vector<bool>(width, false)) {}
    ;
    public: // methods
        void modify(int row, int startCol, int endCol, bool value) {
            if (row < 0 || row >= height || startCol < 0 || endCol > width) {
                log_error("Invalid row or column indices");
            }
        
            for (int i = startCol; i < endCol; ++i) {
                bitmap[row][i] = value;
            }
        }
        void exportToPng(const char * file_name) const {
            FILE * fp = fopen(file_name, "wb");
            if (!fp) {
                // log_error("Failed to create PNG file");
                return;
            }

            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr) {
                fclose(fp);
                // log_error("Failed to create PNG write struct");
                return;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr) {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, nullptr);
                // log_error("Failed to create PNG info struct");
                return;
            }

            if (setjmp(png_jmpbuf(png_ptr))) {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                // log_error("Error during PNG creation");
                return;
            }

            png_init_io(png_ptr, fp);
            png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
            png_write_info(png_ptr, info_ptr);

            png_bytep row = new png_byte[width];
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
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
    private: // variables
        int width, height;
        std::vector<std::vector<bool>> bitmap;
        Logger log;
    ;
};
class _scale {
    public:
        static uint16_t from_8_to_16_bit(const uint8_t & n) {
            return (n << 8) | n;
        }
    ;
};
class window {
    public: // construcers and operators
        window() {}

        operator uint32_t() const {
            return _window;
        }
        window& operator=(uint32_t new_window) { // Overload the assignment operator for uint32_t
            _window = new_window;
            return *this;
        }
    ;
    public: // methods
        public: // main methods
            void create(const uint8_t  & depth,
                        const uint32_t & parent,
                        const int16_t  & x,
                        const int16_t  & y,
                        const uint16_t & width,
                        const uint16_t & height,
                        const uint16_t & border_width,
                        const uint16_t & _class,
                        const uint32_t & visual,
                        const uint32_t & value_mask,
                        const void     * value_list) { 
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
            void create_default(const uint32_t & parent, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height) {
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
            void create_client_window(const uint32_t & parent, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height) {
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
            void raise() {
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
            void map() {
                xcb_map_window(conn, _window);
                xcb_flush(conn);
            }
            void unmap() {
                xcb_unmap_window(conn, _window);
                xcb_flush(conn);
            }
            void reparent(const uint32_t & new_parent, const int16_t & x, const int16_t & y) {
                xcb_reparent_window(
                    conn, 
                    _window, 
                    new_parent, 
                    x, 
                    y
                );
                xcb_flush(conn);
            }
            void make_child(const uint32_t & window_to_make_child, const uint32_t & child_x, const uint32_t & child_y) {
                xcb_reparent_window(
                    conn,
                    window_to_make_child,
                    _window,
                    child_x,
                    child_y
                );
                xcb_flush(conn);
            }
            void kill() {
                xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, 12, "WM_PROTOCOLS");
                xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, NULL);

                xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
                xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply(conn, delete_cookie, NULL);

                if (!protocols_reply) {
                    log_error("protocols reply is null");
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }
                if (!delete_reply) {
                    log_error("delete reply is null");
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }

                send_event(
                    make_client_message_event(
                        32,
                        protocols_reply->atom,
                        delete_reply->atom
                    )
                );

                free(protocols_reply);
                free(delete_reply);

                xcb_flush(conn);
            }
            void clear() {
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
            void focus_input() {
                xcb_set_input_focus(
                    conn, 
                    XCB_INPUT_FOCUS_POINTER_ROOT, 
                    _window, 
                    XCB_CURRENT_TIME
                );
                xcb_flush(conn);
            }
        ;
        public: // check methods
            bool is_EWMH_fullscreen() {
                xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(ewmh, _window);
                xcb_ewmh_get_atoms_reply_t wm_state;
                if (xcb_ewmh_get_wm_state_reply(ewmh, cookie, &wm_state, NULL) == 1) {
                    for (unsigned int i = 0; i < wm_state.atoms_len; i++) {
                        if (wm_state.atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN) {
                            xcb_ewmh_get_atoms_reply_wipe(&wm_state);
                            return true;
                        }
                    }
                    xcb_ewmh_get_atoms_reply_wipe(&wm_state);
                }
                return false;
            }
            bool is_active_EWMH_window() {
                uint32_t active_window = 0;
                xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, 0), &active_window, NULL);
                return _window == active_window;
            }
            uint32_t check_event_mask_sum() {
                uint32_t mask = 0;
                xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(conn, _window);
                xcb_get_window_attributes_reply_t * reply = xcb_get_window_attributes_reply(conn, cookie, NULL);

                if (!reply) { // Check if the reply is valid
                    log_error("Unable to get window attributes.");
                    return 0;
                }

                mask = reply->all_event_masks;
                free(reply);
                return mask;
            }
            std::vector<xcb_event_mask_t> check_event_mask_codes() {
                uint32_t maskSum = check_event_mask_sum();
                std::vector<xcb_event_mask_t> setMasks;
                for (int mask = XCB_EVENT_MASK_KEY_PRESS; mask <= XCB_EVENT_MASK_OWNER_GRAB_BUTTON; mask <<= 1) {
                    if (maskSum & mask) {
                        setMasks.push_back(static_cast<xcb_event_mask_t>(mask));
                    }
                }
                return setMasks;
            }
            bool is_mask_active(const uint32_t & event_mask) {
                std::vector<xcb_event_mask_t> masks = check_event_mask_codes();
                for (const auto & ev_mask : masks) {
                    if (ev_mask == event_mask) {
                        return true;
                    }
                }
                return false;
            }
            bool is_mapped() {
                xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(conn, _window);
                xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, cookie, NULL);
                if (!reply) {
                    log_error("Unable to get window attributes.");
                    return false;
                }

                bool isMapped = (reply->map_state == XCB_MAP_STATE_VIEWABLE);
                free(reply);
                return isMapped;    
            }
        ;
        public: // set methods
            void set_active_EWMH_window() {
                xcb_ewmh_set_active_window(ewmh, 0, _window); // 0 for the first (default) screen
                xcb_flush(conn);
            }
            void set_EWMH_fullscreen_state() {
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
        ;
        public: // unset methods
            void unset_EWMH_fullscreen_state() {
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
        ;
        public: // get methods
            char * property(const char * atom_name) {
                xcb_get_property_reply_t *reply;
                unsigned int reply_len;
                char * propertyValue;

                reply = xcb_get_property_reply(
                    conn,
                    xcb_get_property(
                        conn,
                        false,
                        _window,
                        atom
                        (
                            atom_name
                        ),
                        XCB_GET_PROPERTY_TYPE_ANY,
                        0,
                        60
                    ),
                    nullptr
                );

                if (!reply || xcb_get_property_value_length(reply) == 0) {
                    if (reply != nullptr) {
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
            uint32_t root_window() {
                xcb_query_tree_cookie_t cookie;
                xcb_query_tree_reply_t *reply;

                cookie = xcb_query_tree(conn, _window);
                reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (!reply) {
                    log_error("Unable to query the window tree.");
                    return (uint32_t) 0; // Invalid window ID
                }

                uint32_t root_window = reply->root;
                free(reply);
                return root_window;
            }
            uint32_t parent() {
                xcb_query_tree_cookie_t cookie;
                xcb_query_tree_reply_t *reply;

                cookie = xcb_query_tree(conn, _window);
                reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (!reply) {
                    log_error("Unable to query the window tree.");
                    return (uint32_t) 0; // Invalid window ID
                }

                uint32_t parent_window = reply->parent;
                free(reply);
                return parent_window;
            }
            uint32_t * children(uint32_t * child_count) {
                * child_count = 0;
                xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, _window);
                xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, NULL);

                if (!reply) {
                    log_error("Unable to query the window tree.");
                    return nullptr;
                }

                * child_count = xcb_query_tree_children_length(reply);
                uint32_t * children = static_cast<uint32_t *>(malloc(* child_count * sizeof(xcb_window_t)));

                if (!children) {
                    log_error("Unable to allocate memory for children.");
                    free(reply);
                    return nullptr;
                }

                memcpy(children, xcb_query_tree_children(reply), * child_count * sizeof(uint32_t));
                free(reply);
                return children;
            }
            int16_t x_from_req() {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t x;
                if (geometry) {
                    x = geometry->x;
                    free(geometry);
                } else {
                    x = 200;
                }
                return x;
            }
            int16_t y_from_req() {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t y;
                if (geometry) {
                    y = geometry->y;
                    free(geometry);
                } else {
                    y = 200;
                }
                return y;
            }
            int16_t width_from_req() {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t width;
                if (geometry) {
                    width = geometry->width;
                    free(geometry);
                } else {
                    width = 200;
                }
                return width;
            }
            int16_t height_from_req() {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(conn, geometry_cookie, nullptr);

                int16_t height;
                if (geometry) {
                    height = geometry->height;
                    free(geometry);
                } else {
                    height = 200;
                }
                return height;
            }
        ;
        public: // configuration methods
            void apply_event_mask(const std::vector<uint32_t> & values) {
                if (values.empty()) {
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
            void apply_event_mask(const uint32_t * mask) {
                xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    mask
                );
                xcb_flush(conn);
            }
            void set_pointer(CURSOR cursor_type) {
                xcb_cursor_context_t * ctx;
                if (xcb_cursor_context_new(conn, screen, &ctx) < 0) {
                    log_error("Unable to create cursor context.");
                    return;
                }

                xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, pointer_from_enum(cursor_type));
                if (!cursor) {
                    log_error("Unable to load cursor.");
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
            void draw_text(const char * str , const COLOR & text_color, const COLOR & backround_color, const char * font_name, const int16_t & x, const int16_t & y) {
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
            public: // size_pos configuration methods
                public: // fetch methods
                    uint32_t x() {
                        return _x;
                    }
                    uint32_t y() {
                        return _y;
                    }
                    uint32_t width() {
                        return _width;
                    }
                    uint32_t height() {
                        return _height;
                    }
                ;    
                void x(const uint32_t & x) {
                    config_window(MWM_CONFIG_x, x);
                    update(x, _y, _width, _height);
                }
                void y(const uint32_t & y) {
                    config_window(XCB_CONFIG_WINDOW_Y, y);
                    update(_x, y, _width, _height);
                }
                void width(const uint32_t & width) {
                    config_window(MWM_CONFIG_width, width);
                    update(_x, _y, width, _height);
                }
                void height(const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, height);
                    update(_x, _y, _width, height);
                }
                void x_y(const uint32_t & x, const uint32_t & y) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, {x, y});
                    update(x, y, _width, _height);
                }
                void width_height(const uint32_t & width, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {width, height});
                    update(_x, _y, width, height);
                }
                void x_y_width_height(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {x, y, width, height});
                    update(x, y, width, height);
                }
                void x_width_height(const uint32_t & x, const uint32_t & width, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {x, width, height});
                    update(x, _y, width, height);
                }
                void y_width_height(const uint32_t & y, const uint32_t & width, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, {y, width, height});
                    update(_x, y, width, height);
                }
                void x_width(const uint32_t & x, const uint32_t & width) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, {x, width});
                    update(x, _y, width, _height);
                }
                void x_height(const uint32_t & x, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_HEIGHT, {x, height});
                    update(x, _y, _width, height);
                }
                void y_width(const uint32_t & y, const uint32_t & width) {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, {y, width});
                    update(_x, y, width, _height);
                }
                void y_height(const uint32_t & y, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, {y, height});
                    update(_x, y, _width, height);
                }
                void x_y_width(const uint32_t & x, const uint32_t & y, const uint32_t & width) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, {x, y, width});
                    update(x, y, width, _height);
                }
                void x_y_height(const uint32_t & x, const uint32_t & y, const uint32_t & height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, {x, y, height});
                    update(x, y, _width, height);
                }
            ;
            public: // backround methods
                void set_backround_color(COLOR color) {
                    change_back_pixel(get_color(color));
                }
                void set_backround_color_8_bit(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value) {
                    change_back_pixel(get_color(red_value, green_value, blue_value));
                }
                void set_backround_color_16_bit(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value) {
                    change_back_pixel(get_color(red_value, green_value, blue_value));
                }
                void set_backround_png(const char * imagePath) {
                    Imlib_Image image = imlib_load_image(imagePath);
                    if (!image) {
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

                    if (newWidth > _width) {
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
                    
                    DATA32 * data = imlib_image_get_data(); // Get the scaled image data

                    xcb_image_t * xcb_image = xcb_image_create_native( // Create an XCB image from the scaled data
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
                    int x = (_width - newWidth) / 2;
                    int y = (_height - newHeight) / 2;

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
                void make_then_set_png(const char * file_name, const std::vector<std::vector<bool>>& bitmap) {
                    create_png_from_vector_bitmap(file_name, bitmap);
                    set_backround_png(file_name);
                }
            ;
        ;
        public: // keys
            void grab_default_keys() {
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
                    {   R,          SUPER                   }, // key_binding for runner_window
                    {   F,          SUPER                   }, // key_binding for file_app
                });
            }
            void grab_keys(std::initializer_list<std::pair<const uint32_t, const uint16_t>> bindings) {
                xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(conn);
            
                if (!keysyms) {
                    log_error("keysyms could not get initialized");
                    return;
                }

                for (const auto & binding : bindings) {
                    xcb_keycode_t * keycodes = xcb_key_symbols_get_keycode(keysyms, binding.first);
                    if (keycodes) {
                        for (auto * kc = keycodes; * kc; kc++) {
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
            void grab_keys_for_typing() {
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
        ;
        public: // buttons
            void grab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings) {
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
            void ungrab_button(std::initializer_list<std::pair<const uint8_t, const uint16_t>> bindings) {
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
        ;
    ;
    private: // variables
        private: // main variables 
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
            const void     * _value_list;
        ;
        xcb_gcontext_t gc;
        xcb_gcontext_t font_gc;
        xcb_font_t     font;
        xcb_pixmap_t   pixmap;
        Logger log;
    ;
    private: // functions
        private: // main functions 
            void make_window() {
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
            void clear_window() {
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
            void update(const uint32_t & x, const uint32_t & y, const uint32_t & width, const uint32_t & height) {
                _x = x;
                _y = y;
                _width = width;
                _height = height;
            }
            /**
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
            void config_window(const uint16_t & mask, const uint16_t & value) {
                xcb_configure_window(
                    conn,
                    _window,
                    mask,
                    (const uint32_t[1]) {
                        static_cast<const uint32_t &>(value)
                    }
                );
            }
            void config_window(uint32_t mask, const std::vector<uint32_t> & values) {
                if (values.empty()) {
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
        ;
        void send_event(xcb_client_message_event_t ev) {
            xcb_send_event(
                conn,
                0,
                _window,
                XCB_EVENT_MASK_NO_EVENT,
                (char *) & ev
            );
        }
        xcb_client_message_event_t make_client_message_event(const uint32_t & format, const uint32_t & type, const uint32_t & data) {
            xcb_client_message_event_t ev = { 0 };
            ev.response_type = XCB_CLIENT_MESSAGE;
            ev.window = _window;
            ev.format = format;
            ev.sequence = 0;
            ev.type = type;
            ev.data.data32[0] = data;
            ev.data.data32[1] = XCB_CURRENT_TIME;

            return ev;
        }
        private: // pointer functions 
            const char * pointer_from_enum(CURSOR CURSOR) {
                switch (CURSOR) {
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
        private: // create functions 
            private: // gc functions 
                void create_graphics_exposure_gc() {
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
                void create_font_gc(const COLOR & text_color, const COLOR & backround_color, xcb_font_t font) {
                    font_gc = xcb_generate_id(conn);
                    xcb_create_gc(
                        conn, 
                        font_gc, 
                        _window, 
                        XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT, 
                        (const uint32_t[3]) {
                            get_color(text_color),
                            get_color(backround_color),
                            font
                        }
                    );
                }
            ;
            private: // pixmap functions 
                void create_pixmap() {
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
            ;
            private: // png functions
                void create_png_from_vector_bitmap(const char * file_name, const std::vector<std::vector<bool>> & bitmap) {
                    int width = bitmap[0].size();
                    int height = bitmap.size();

                    FILE *fp = fopen(file_name, "wb");
                    if (!fp) {
                        log_error("Failed to open file: " + std::string(file_name));
                        return;
                    }

                    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    if (!png_ptr) {
                        fclose(fp);
                        log_error("Failed to create PNG write struct");
                        return;
                    }

                    png_infop info_ptr = png_create_info_struct(png_ptr);
                    if (!info_ptr) {
                        fclose(fp);
                        png_destroy_write_struct(&png_ptr, NULL);
                        log_error("Failed to create PNG info struct");
                        return;
                    }

                    if (setjmp(png_jmpbuf(png_ptr))) {
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
            ;
        ;
        private: // get functions 
            xcb_atom_t atom(const char * atom_name) {
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
            std::string AtomName(xcb_atom_t atom) {
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
            void get_font(const char * font_name) {
                font = xcb_generate_id(conn);
                xcb_open_font(
                    conn, 
                    font, 
                    strlen(font_name),
                    font_name
                );
                xcb_flush(conn);
            }
        ;
        private: // backround functions 
            void change_back_pixel(const uint32_t & pixel) {
                xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_BACK_PIXEL,
                    (const uint32_t[1]) {
                        pixel
                    }
                );
                xcb_flush(conn);
            }
            uint32_t get_color(COLOR color) {
                uint32_t pixel = 0;
                xcb_colormap_t colormap = screen->default_colormap;
                rgb_color_code color_code = rgb_code(color);
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
            uint32_t get_color(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value) {
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
            uint32_t get_color(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value) {
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
            rgb_color_code rgb_code(COLOR COLOR) {
                rgb_color_code color;
                uint8_t r;
                uint8_t g;
                uint8_t b;
                
                switch (COLOR) {
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
        ;
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
                make_titlebar();
                make_close_button();
                make_max_button();
                make_min_button();
                
                if (BORDER_SIZE > 0) {
                    make_borders();
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
        std::vector<std::vector<bool>> CLOSE_BUTTON_BITMAP = {
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
class desktop {
    public: // variabels
        std::vector<client *> current_clients;
        uint16_t desktop;
        const uint16_t x = 0;
        const uint16_t y = 0;
        uint16_t width;
        uint16_t height;
    ;
};
class Key_Codes {
    public: // constructor and destructor
        Key_Codes() 
        : keysyms(nullptr) {}

        ~Key_Codes() {
            free(keysyms);
        }
    ;
    public: // methods
        void init() {
            keysyms = xcb_key_symbols_alloc(conn);
            if (keysyms) {
                std::map<uint32_t, xcb_keycode_t *> key_map = {
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
                    { TAB,          &tab       },
                    { K,            &k         }
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
    ;
    public: // variabels
        xcb_keycode_t
            a{},
            b{},
            c{},
            d{},
            e{},
            f{},
            g{},
            h{},
            i{},
            j{},
            k{},
            l{},
            m{},
            n{},
            o{},
            p{},
            q{},
            r{},
            s{},
            t{},
            u{},
            v{},
            w{},
            x{},
            y{},
            z{},

            space_bar{},
            enter{},

            f11{},
            n_1{},
            n_2{},
            n_3{},
            n_4{},
            n_5{},
            r_arrow{},
            l_arrow{},
            u_arrow{},
            d_arrow{},
            tab{},
            _delete{}
        ;
    ;
    private: // variabels
        xcb_key_symbols_t * keysyms;
    ;
};
class Entry {
    public: // constructor
        Entry() {}
    ;
    public: // variabels
        window window;
        bool menu = false;
    ;
    public: // public methods
        void add_name(const char * name) {
            entryName = name;
        }
        void add_action(std::function<void()> action) {
            entryAction = action;
        }
        void activate() const {
            entryAction();
        }
        const char * getName() const {
            return entryName;
        }
        void make_window(const xcb_window_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height) {
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
class context_menu {
    public: // consructor
        context_menu() {
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
        void init() {
            configure_events();
        }
        void show() {
            size_pos.x = pointer.x();
            size_pos.y = pointer.y();
            
            uint32_t height = entries.size() * size_pos.height;
            if (size_pos.y + height > screen->height_in_pixels) {
                size_pos.y = (screen->height_in_pixels - height);
            }

            if (size_pos.x + size_pos.width > screen->width_in_pixels) {
                size_pos.x = (screen->width_in_pixels - size_pos.width);
            }

            context_window.x_y_height((size_pos.x - BORDER_SIZE), (size_pos.y - BORDER_SIZE), height);
            context_window.map();
            context_window.raise();
            make_entries();
        }
        void add_entry(const char * name, std::function<void()> action) {
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
        void create_dialog_win() {
            context_window.create_default(screen->root, 0, 0, size_pos.width, size_pos.height);
            uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION;
            context_window.apply_event_mask(& mask);
            context_window.set_backround_color(DARK_GREY);
            context_window.raise();
        }
        void hide() {
            context_window.unmap();
            context_window.kill();
        }
        void configure_events() {
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                const auto & e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->detail == L_MOUSE_BUTTON) {
                    run_action(& e->event);
                    hide();
                }
            });
            event_handler->setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_enter_notify_event_t *>(ev);
                if (e->event == screen->root) {
                    hide();
                } 
            });
        }
        void run_action(const xcb_window_t * w) {
            for (const auto & entry : entries) {
                if (* w == entry.window) {
                    entry.activate();
                }
            }
        }
        void make_entries() {
            int y = 0;
            for (auto & entry : entries) {
                entry.make_window(context_window, (0 + (BORDER_SIZE / 2)), (y + (BORDER_SIZE / 2)), (size_pos.width - BORDER_SIZE), (size_pos.height - BORDER_SIZE));
                entry.window.draw_text(entry.getName(), WHITE, BLACK, "7x14", 2, 14);
                y += size_pos.height;
            }
        }
    ;
};
class Window_Manager {
    public: // constructor
        Window_Manager() {}
    ;
    public: // variabels 
        context_menu * context_menu = nullptr;
        window root;
        Launcher launcher;
        std::vector<client *> client_list;
        std::vector<desktop *> desktop_list;
        client * focused_client = nullptr;
        desktop * cur_d = nullptr;
        Logger log;
        pointer pointer;
        win_data data;
        Key_Codes key_codes;
    ;
    public: // methods 
        public: // main methods 
            void init() {
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
                context_menu->add_entry(   
                    "konsole",
                    [this]() 
                    {
                        launcher.program((char *) "konsole");
                    }
                );
                context_menu->init();
            }
            void launch_program(char * program) {
                if (fork() == 0) {
                    setsid();
                    execvp(program, (char *[]) { program, NULL });
                }
            }
            void quit(const int & status) {
                xcb_flush(conn);
                delete_client_vec(client_list);
                delete_desktop_vec(desktop_list);
                xcb_ewmh_connection_wipe(ewmh);
                xcb_disconnect(conn);
                exit(status);
            }
        ;
        public: // client methods 
            public: // focus methods 
                void focus_client(client * c) {
                    if (!c) {
                        log_error("c is null");
                        return;
                    }

                    focused_client = c;
                    c->focus();
                }
                void cycle_focus() {
                    bool focus = false;
                    for (auto & c : client_list) {
                        if (c) {
                            if (c == focused_client) {
                                focus = true;
                                continue;
                            }
                            
                            if (focus) {
                                focus_client(c);
                                return;  
                            }
                        }
                    }
                }
            ;
            public: // client fetch methods
                client * client_from_window(const xcb_window_t * window) {
                    for (const auto & c : client_list) {
                        if (* window == c->win) {
                            return c;
                        }
                    }
                    return nullptr;
                }
                client * client_from_any_window(const xcb_window_t * window) {
                    for (const auto & c : client_list) {
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
                    return nullptr;
                }
                client * client_from_pointer(const int & prox) {
                    const uint32_t & x = pointer.x();
                    const uint32_t & y = pointer.y();
                    for (const auto & c : cur_d->current_clients) {
                        if (x > c->x - prox && x <= c->x) { // LEFT EDGE OF CLIENT
                            return c;
                        }
                        if (x >= c->x + c->width && x < c->x + c->width + prox) { // RIGHT EDGE OF CLIENT
                            return c;
                        }
                        if (y > c->y - prox && y <= c->y) { // TOP EDGE OF CLIENT
                            return c;
                        }
                        if (y >= c->y + c->height && y < c->y + c->height + prox) { // BOTTOM EDGE OF CLIENT
                            return c;
                        }
                    }
                    return nullptr;
                }
                std::map<client *, edge> get_client_next_to_client(client * c, edge c_edge) {
                    std::map<client *, edge> map;
                    for (client * c2 : cur_d->current_clients) {
                        if (c == c2) {
                            continue;
                        }

                        if (c_edge == edge::LEFT) {
                            if (c->x == c2->x + c2->width) {
                                map[c2] = edge::RIGHT;
                                return map;
                            }
                        }
                        if (c_edge == edge::RIGHT) {
                            if (c->x + c->width == c2->x) {
                                map[c2] = edge::LEFT;
                                return map;
                            }
                        }
                        if (c_edge == edge::TOP) {
                            if (c->y == c2->y + c2->height) {
                                map[c2] = edge::BOTTOM_edge;
                                return map;
                            }
                        }
                        if (c_edge == edge::BOTTOM_edge) {
                            if (c->y + c->height == c2->y) {
                                map[c2] = edge::TOP;
                                return map;
                            }
                        }
                    }

                    map[nullptr] = edge::NONE;
                    return map;
                }
                edge get_client_edge_from_pointer(client * c, const int & prox) {
                    const uint32_t & x = pointer.x();
                    const uint32_t & y = pointer.y();

                    const uint32_t & top_border = c->y;
                    const uint32_t & bottom_border = (c->y + c->height);
                    const uint32_t & left_border = c->x;
                    const uint32_t & right_border = (c->x + c->width);

                    if (((y > top_border - prox) && (y <= top_border)) // TOP EDGE OF CLIENT
                     && ((x > left_border + prox) && (x < right_border - prox))) {
                        return edge::TOP;
                    }
                    if (((y >= bottom_border) && (y < bottom_border + prox)) // BOTTOM EDGE OF CLIENT
                     && ((x > left_border + prox) && (x < right_border - prox))) {
                        return edge::BOTTOM_edge;
                    }
                    if (((x > left_border) - prox && (x <= left_border)) // LEFT EDGE OF CLIENT
                     && ((y > top_border + prox) && (y < bottom_border - prox))) {
                        return edge::LEFT;
                    }
                    if (((x >= right_border) && (x < right_border + prox)) // RIGHT EDGE OF CLIENT
                     && ((y > top_border + prox) && (y < bottom_border - prox))) {
                        return edge::RIGHT;
                    }
                    if (((x > left_border - prox) && x < left_border + prox) // TOP LEFT CORNER OF CLIENT
                     && ((y > top_border - prox) && y < top_border + prox)) {
                        return edge::TOP_LEFT;
                    }
                    if (((x > right_border - prox) && x < right_border + prox) // TOP RIGHT CORNER OF CLIENT
                     && ((y > top_border - prox) && y < top_border + prox)) {
                        return edge::TOP_RIGHT;
                    }
                    if (((x > left_border - prox) && x < left_border + prox) // BOTTOM LEFT CORNER OF CLIENT
                     && ((y > bottom_border - prox) && y < bottom_border + prox)) {
                        return edge::BOTTOM_LEFT;
                    }
                    if (((x > right_border - prox) && x < right_border + prox) // BOTTOM RIGHT CORNER OF CLIENT
                     && ((y > bottom_border - prox) && y < bottom_border + prox)) {
                        return edge::BOTTOM_RIGHT;
                    }

                    return edge::NONE;
                }
            ;
            void manage_new_client(const uint32_t & window) {
                client * c = make_client(window);
                if (!c) {
                    log_error("could not make client");
                    return;
                }

                c->win.x_y_width_height(c->x, c->y, c->width, c->height);
                c->win.map();
                c->win.grab_button({
                    {   L_MOUSE_BUTTON,     ALT },
                    {   R_MOUSE_BUTTON,     ALT },
                    {   L_MOUSE_BUTTON,     0   }
                });
                c->win.grab_default_keys();
                c->make_decorations();
                uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
                c->win.apply_event_mask(& mask);

                c->update();
                focus_client(c);
            }
            client * make_internal_client(window window) {
                client * c = new client;
                c->win = window;
                c->x = window.x();
                c->y = window.y();
                c->width = window.width();
                c->height = window.height();
                
                c->make_decorations();
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                c->focus();

                return c;
            }
            void send_sigterm_to_client(client * c) {
                c->kill();
                remove_client(c);
            }
        ;
        public: // desktop methods 
            void create_new_desktop(const uint16_t & n) {
                desktop * d = new desktop;
                d->desktop  = n;
                d->width    = screen->width_in_pixels;
                d->height   = screen->height_in_pixels;
                cur_d       = d;
                desktop_list.push_back(d);
            }
        ;
    ;
    private: // variables 
        window start_window;
    ;
    private: // functions 
        private: // init functions 
            void _conn(const char * displayname, int * screenp) {
                conn = xcb_connect(displayname, screenp);
                check_conn();
            }
            void _ewmh() {
                if (!(ewmh = static_cast<xcb_ewmh_connection_t *>(calloc(1, sizeof(xcb_ewmh_connection_t))))) {
                    log_error("ewmh faild to initialize");
                    quit(1);
                }    
                
                xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms(conn, ewmh);
                if (!(xcb_ewmh_init_atoms_replies(ewmh, cookie, 0))) {
                    log_error("xcb_ewmh_init_atoms_replies:faild");
                    quit(1);
                }

                const char * str = "mwm";
                check_error(
                    conn, 
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
            void _setup() {
                setup = xcb_get_setup(conn);
            }
            void _iter() {
                iter = xcb_setup_roots_iterator(setup);
            }
            void _screen() {
                screen = iter.data;
            }
            bool setSubstructureRedirectMask() {
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
                if (error) {
                    log_error("Error: Another window manager is already running or failed to set SubstructureRedirect mask."); 
                    free(error);
                    return false;
                }
                return true;
            }
            void configure_root() {
                root.set_backround_color(DARK_GREY);
                uint32_t mask = 
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_ENTER_WINDOW |
                    XCB_EVENT_MASK_LEAVE_WINDOW |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_BUTTON_RELEASE |
                    XCB_EVENT_MASK_KEY_PRESS |
                    XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_FOCUS_CHANGE |
                    XCB_EVENT_MASK_POINTER_MOTION
                ; 
                root.apply_event_mask(& mask);
                root.clear();

                root.set_backround_png("/home/mellw/mwm_png/galaxy17.png");
                root.set_pointer(CURSOR::arrow);
            }
        ;
        private: // check functions 
            void check_error(const int & code) {
                switch (code) {
                    case CONN_ERR:
                        log_error("Connection error.");
                        quit(CONN_ERR);
                        break;
                    case EXTENTION_NOT_SUPPORTED_ERR:
                        log_error("Extension not supported.");
                        quit(EXTENTION_NOT_SUPPORTED_ERR);
                        break;
                    case MEMORY_INSUFFICIENT_ERR:
                        log_error("Insufficient memory.");
                        quit(MEMORY_INSUFFICIENT_ERR);
                        break;
                    case REQUEST_TO_LONG_ERR:
                        log_error("Request to long.");
                        quit(REQUEST_TO_LONG_ERR);
                        break;
                    case PARSE_ERR:
                        log_error("Parse error.");
                        quit(PARSE_ERR);
                        break;
                    case SCREEN_NOT_FOUND_ERR:
                        log_error("Screen not found.");
                        quit(SCREEN_NOT_FOUND_ERR);
                        break;
                    case FD_ERR:
                        log_error("File descriptor error.");
                        quit(FD_ERR);
                        break;
                }
            }
            void check_conn() {
                int status = xcb_connection_has_error(conn);
                check_error(status);
            }
            int cookie_error(xcb_void_cookie_t cookie , const char * sender_function) {
                xcb_generic_error_t * err = xcb_request_check(conn, cookie);
                if (err) {
                    log_error(err->error_code);
                    free(err);
                    return err->error_code;
                }
                return 0;
            }
            void check_error(xcb_connection_t * connection, xcb_void_cookie_t cookie , const char * sender_function, const char * err_msg) {
                xcb_generic_error_t * err = xcb_request_check(connection, cookie);
                if (err)
                {
                    log_error_code(err_msg, err->error_code);
                    free(err);
                }
            }
        ;
        int start_screen_window() {
            start_window.create_default(root, 0, 0, 0, 0);
            start_window.set_backround_color(DARK_GREY);
            start_window.map();
            return 0;
        }
        private: // delete functions 
            void delete_client_vec(std::vector<client *> & vec) {
                for (client * c : vec) {
                    send_sigterm_to_client(c);
                    xcb_flush(conn);                    
                }

                vec.clear();

                std::vector<client *>().swap(vec);
            }
            void delete_desktop_vec(std::vector<desktop *> & vec) {
                for (desktop * d : vec) {
                    delete_client_vec(d->current_clients);
                    delete d;
                }

                vec.clear();

                std::vector<desktop *>().swap(vec);
            }
            template <typename Type> 
            static void delete_ptr_vector(std::vector<Type *>& vec) {
                for (Type * ptr : vec) {
                    delete ptr;
                }
                vec.clear();

                std::vector<Type *>().swap(vec);
            }
            void remove_client(client * c) {
                client_list.erase(std::remove(client_list.begin(), client_list.end(), c), client_list.end());
                cur_d->current_clients.erase(std::remove(cur_d->current_clients.begin(), cur_d->current_clients.end(), c), cur_d->current_clients.end());
                delete c;
            }
            void remove_client_from_vector(client * c, std::vector<client *> & vec) {
                if (!c) {
                    log_error("client is nullptr.");
                }
                vec.erase(std::remove(vec.begin(), vec.end(), c), vec.end());
                delete c;
            }
        ;
        private: // client functions
            client * make_client(const uint32_t & window) {
                client * c = new client;
                if (!c) {
                    log_error("Could not allocate memory for client");
                    return nullptr;
                }

                c->win    = window;
                c->height = (data.height < 300) ? 300 : data.height;
                c->width  = (data.width < 400)  ? 400 : data.width;
                c->x      = c->win.x_from_req();
                log_info(std::to_string(c->x));
                c->y      = c->win.y_from_req();
                log_info(std::to_string(c->y));
                c->depth   = 24;
                c->desktop = cur_d->desktop;

                if (c->x <= 0 && c->y <= 0 && c->width != screen->width_in_pixels && c->height != screen->height_in_pixels) {
                    c->x = (screen->width_in_pixels - c->width) / 2;
                    c->y = (((screen->height_in_pixels - c->height) / 2) + (BORDER_SIZE * 3)); // ???? Why * 3
                }

                if (c->height > screen->height_in_pixels) {
                    c->height = screen->height_in_pixels;
                }
                if (c->width > screen->width_in_pixels) {
                    c->width = screen->width_in_pixels;
                }

                if (c->win.is_EWMH_fullscreen()) {
                    c->x      = 0;
                    c->y      = 0;
                    c->width  = screen->width_in_pixels;
                    c->height = screen->height_in_pixels;
                    c->win.set_EWMH_fullscreen_state();
                }
                
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                return c;
            }
        ;
        private: // window functions
            void getWindowParameters(const uint32_t & window) {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);

                if (geometry_reply != NULL) {
                    log_info("Window Parameters");
                    log_info(std::to_string(geometry_reply->x));
                    log_info(std::to_string(geometry_reply->y));
                    log_info(std::to_string(geometry_reply->width));
                    log_info(std::to_string(geometry_reply->height));

                    free(geometry_reply);
                } else {
                    std::cerr << "Unable to get window geometry." << std::endl;
                }
            }
            void get_window_parameters(const uint32_t & window, int16_t * x, int16_t * y, uint16_t * width, uint16_t * height) {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);
                if (!geometry_reply) {
                    log_error("unable to get window parameters for window: " + std::to_string(window));
                }

                x = &geometry_reply->x;
                y = &geometry_reply->y;
                width = &geometry_reply->width;
                height = &geometry_reply->height;

                free(geometry_reply);
            }
            int16_t get_window_x(const uint32_t & window) {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);
                if (!geometry_reply) {
                    log_error("Unable to get window geometry.");
                    return (screen->width_in_pixels / 2);
                } 
            
                int16_t x = geometry_reply->x;
                free(geometry_reply);
                return x;
            }
            int16_t get_window_y(const uint32_t & window) {
                xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(conn, window);
                xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(conn, geometry_cookie, NULL);
                if (!geometry_reply) {
                    log_error("Unable to get window geometry.");
                    return (screen->height_in_pixels / 2);
                } 
            
                int16_t y = geometry_reply->y;
                free(geometry_reply);
                return y;
            }
        ;
    ;
}; static Window_Manager * wm;
/**
 *
 * @class XCPPBAnimator
 * @brief Class for animating the position and size of an XCB window.
 *
 */
class Mwm_Animator {
    public: // variables
        Mwm_Animator(const uint32_t & window)
        : window(window) {}

        Mwm_Animator(client * c)
        : c(c) {}
    ;
    public: // destructor
        /**
         * @brief Destructor to ensure the animation threads are stopped when the object is destroyed.
         */
        ~Mwm_Animator() {
            stopAnimations();
        }
    ;
    public: // methods 
        /**
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
        void animate(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration) {
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
            XAnimationThread = std::thread(&Mwm_Animator::XAnimation, this, endX);
            YAnimationThread = std::thread(&Mwm_Animator::YAnimation, this, endY);
            WAnimationThread = std::thread(&Mwm_Animator::WAnimation, this, endWidth);
            HAnimationThread = std::thread(&Mwm_Animator::HAnimation, this, endHeight);

            std::this_thread::sleep_for(std::chrono::milliseconds(duration)); // WAIT FOR ANIMATION TO COMPLETE
            stopAnimations(); // STOP THE ANIMATION
        }
        void animate_client(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration) {
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
            GAnimationThread = std::thread(&Mwm_Animator::GFrameAnimation, this, endX, endY, endWidth, endHeight);
            XAnimationThread = std::thread(&Mwm_Animator::CliXAnimation, this, endX);
            YAnimationThread = std::thread(&Mwm_Animator::CliYAnimation, this, endY);
            WAnimationThread = std::thread(&Mwm_Animator::CliWAnimation, this, endWidth);
            HAnimationThread = std::thread(&Mwm_Animator::CliHAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
        enum DIRECTION {
            NEXT,
            PREV
        };
        void animate_client_x(int startX, int endX, int duration) {
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
            GAnimationThread = std::thread(&Mwm_Animator::GFrameAnimation_X, this, endX);
            XAnimationThread = std::thread(&Mwm_Animator::CliXAnimation, this, endX);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
    ;
    private: // variabels
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
        const double frameRate = 120;
        const double frameDuration = 1000.0 / frameRate; 
    ;
    private: // funcrions
        /**
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
        void XAnimation(const int & endX) {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentX == endX) {
                    config_window(XCB_CONFIG_WINDOW_X, endX);
                    break;
                }
                XStep();
                thread_sleep(XAnimDuration);
            }
        }
        /**
         * @brief Performs a step in the X direction.
         * 
         * This function increments the currentX variable by the stepX value.
         * If it is time to render, it configures the window's X position using the currentX value.
         * 
         * @note This function assumes that the connection and window variables are properly initialized.
         */
        void XStep() {
            currentX += stepX;
            
            if (XisTimeToRender()) {
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
        /**
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
        void YAnimation(const int & endY) {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentY == endY) {
                    config_window(XCB_CONFIG_WINDOW_Y, endY);
                    break;
                }
                YStep();
                thread_sleep(YAnimDuration);
            }
        }
        /**
         * @brief Performs a step in the Y direction.
         * 
         * This function increments the currentY variable by the stepY value.
         * If it is time to render, it configures the window's Y position using xcb_configure_window
         * and flushes the connection using xcb_flush.
         */
        void YStep() {
            currentY += stepY;
            
            if (YisTimeToRender()) {
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
        /**
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
        void WAnimation(const int & endWidth) {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentWidth == endWidth) {
                    config_window(XCB_CONFIG_WINDOW_WIDTH, endWidth);
                    break;
                }
                WStep();
                thread_sleep(WAnimDuration);
            }
        }
        /**
         *
         * @brief Performs a step in the width calculation and updates the window width if it is time to render.
         * 
         * This function increments the current width by the step width. If it is time to render, it configures the window width
         * using the XCB library and flushes the connection.
         *
         */
        void WStep() {
            currentWidth += stepWidth;

            if (WisTimeToRender()) {
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
        /**
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
        void HAnimation(const int & endHeight) {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentHeight == endHeight) {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, endHeight);
                    break;
                }
                HStep();
                thread_sleep(HAnimDuration);
            }
        }
        /**
         *
         * @brief Increases the current height by the step height and updates the window height if it's time to render.
         * 
         * This function is responsible for incrementing the current height by the step height and updating the window height
         * if it's time to render. It uses the xcb_configure_window function to configure the window height and xcb_flush to
         * flush the changes to the X server.
         *
         */
        void HStep() {
            currentHeight += stepHeight;
            
            if (HisTimeToRender()) {
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
        void GFrameAnimation(const int & endX, const int & endY, const int & endWidth, const int & endHeight) {
            while (true) {
                if (currentX == endX && currentY == endY && currentWidth == endWidth && currentHeight == endHeight) {
                    c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                    break;
                }
                c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                thread_sleep(GAnimDuration);
            }
        }
        void GFrameAnimation_X(const int & endX) {
            while (true) {
                if (currentX == endX) {
                    conf_client_x();
                    break;
                }
                conf_client_x();
                thread_sleep(GAnimDuration);
            }
        }
        /**
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
        void CliXAnimation(const int & endX) {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentX == endX) {
                    break;
                }
                currentX += stepX;
                thread_sleep(XAnimDuration);
            }
        }
        /**
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
        void CliYAnimation(const int & endY) {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentY == endY) {
                    break;
                }
                currentY += stepY;
                thread_sleep(YAnimDuration);
            }
        }
        /**
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
        void CliWAnimation(const int & endWidth) {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentWidth == endWidth) {
                    break;
                }
                currentWidth += stepWidth;
                thread_sleep(WAnimDuration);
            }
        }
        /**
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
        void CliHAnimation(const int & endHeight) {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) {
                if (currentHeight == endHeight) {
                    break;
                }
                currentHeight += stepHeight;
                thread_sleep(HAnimDuration);
            }
        }
        /**
         *
         * @brief Stops all animations by setting the corresponding flags to true and joining the animation threads.
         *        After joining the threads, the flags are set back to false.
         *
         */
        void stopAnimations() {
            stopHFlag.store(true);
            stopXFlag.store(true);
            stopYFlag.store(true);
            stopWFlag.store(true);
            stopHFlag.store(true);

            if (GAnimationThread.joinable()) {
                GAnimationThread.join();
                stopGFlag.store(false);
            }
            if (XAnimationThread.joinable()) {
                XAnimationThread.join();
                stopXFlag.store(false);
            }
            if (YAnimationThread.joinable()) {
                YAnimationThread.join();
                stopYFlag.store(false);
            }
            if (WAnimationThread.joinable()) {
                WAnimationThread.join();
                stopWFlag.store(false);
            }
            if (HAnimationThread.joinable()) {
                HAnimationThread.join();
                stopHFlag.store(false);
            }
        }
        /**
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
        void thread_sleep(const double & milliseconds) {
            auto duration = std::chrono::duration<double, std::milli>(milliseconds); // Creating a duration with a 'double' in milliseconds
            std::this_thread::sleep_for(duration); // Sleeping for the duration
        }
        /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        bool XisTimeToRender() {
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - XlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration) {
                XlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        /**
         *
         * Checks if it is time to render a frame based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        bool YisTimeToRender() {
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - YlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration) {
                YlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        bool WisTimeToRender() {
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - WlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration) 
            {
                WlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        bool HisTimeToRender() {
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - HlastUpdateTime; // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            if (elapsedTime.count() >= frameDuration) {
                HlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        /**
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
        void config_window(const uint32_t & mask, const uint32_t & value) {
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
        /**
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
        void config_window(const xcb_window_t & win, const uint32_t & mask, const uint32_t & value) {
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
        /**
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
        void config_client(const uint32_t & mask, const uint32_t & value) {
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
        void conf_client_x() {
            const uint32_t x = currentX;
            c->frame.x(x);
            xcb_flush(conn);
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
class button {
    public: // constructor 
        button() {}
    ;
    public: // public variables 
        window window;
        const char * name;
    ;
    public: // public methods 
        void create(const uint32_t & parent_window, const int16_t & x, const int16_t & y, const uint16_t & width, const uint16_t & height, COLOR color) {
            window.create_default(parent_window, x, y, width, height);
            window.set_backround_color(color);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });
            window.map();
            xcb_flush(conn);
        }
        void action(std::function<void()> action) {
            button_action = action;
        }
        void add_event(std::function<void(Ev ev)> action) {
            ev_a = action;
            event_handler->setEventCallback(XCB_BUTTON_PRESS, ev_a);
        }
        void activate() const {
            button_action();
        }
        void put_icon_on_button() {
            std::string icon_path = file.findPngFile(
                {
                    "/usr/share/icons/gnome/256x256/apps/",
                    "/usr/share/icons/hicolor/256x256/apps/",
                    "/usr/share/icons/gnome/48x48/apps/",
                    "/usr/share/icons/gnome/32x32/apps/",
                    "/usr/share/pixmaps"
                },
                name
            );

            if (icon_path == "") {
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
    public: // constructors
        buttons() {}
    ;
    public: // variables
        std::vector<button> list;
    ;
    public: // methods
        void add(const char * name, std::function<void()> action) {
            button button;
            button.name = name;
            button.action(action);
            list.push_back(button);
        }
        int size() {
            return list.size();
        }
        int index() {
            return list.size() - 1;
        }
        void run_action(const uint32_t & window) {
            for (const auto & button : list) {
                if (window == button.window) {
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
                if (e->event == main_window) {
                    main_window.raise();
                    main_window.focus_input();
                }
            });
        }
        void draw_text() {
            main_window.draw_text(search_string.c_str(), WHITE, BLACK, "7x14", 2, 14);
            if (search_string.length() > 0) {
                results = file.search_for_binary(search_string.c_str());
                int entry_list_size = results.size(); 
                if (results.size() > 7) {
                    entry_list_size = 7;
                }
                main_window.height(20 * entry_list_size);
                xcb_flush(conn);
                for (int i = 0; i < entry_list_size; ++i) {
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
            search_window.create(
                main_window,
                2,
                2,
                main_window.width() - (BORDER * 2),
                main_window.height() - (BORDER * 2)
            );
            search_window.init();
            search_window.add_enter_action([this]() {
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
            event_handler->setEventCallback(XCB_KEY_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_key_press_event_t *>(ev);
                if (e->detail == wm->key_codes.r) {
                    if (e->state == SUPER) {
                        show();
                    }
                }
            });
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (!main_window.is_mapped()) {
                    return;
                }
                if (e->event != main_window && e->event != search_window.main_window) {
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
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == search_window.main_window) {
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
    private: // variables
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
            add_app_dialog_window.search_window.add_enter_action([this]() {
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
            if (num_of_buttons == 0) {
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
                buttons.add(
                    app,
                    [app, this] () {
                        {
                            launcher.program((char *) app);
                        }
                    }
                );
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
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                const auto * e = reinterpret_cast<const xcb_button_press_event_t *>(ev);
                if (e->event == main_window) {
                    if (e->detail == R_MOUSE_BUTTON) {
                        context_menu.show();
                    }
                }
                for (int i = 0; i < buttons.size(); ++i) {
                    if (e->event == buttons.list[i].window) {
                        if (e->detail == L_MOUSE_BUTTON) {
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

            pointer.grab(wm->root);
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
        #define RIGHT_  screen->width_in_pixels  - c->width
        #define BOTTOM_ screen->height_in_pixels - c->height
        const double frameRate = 120.0;
        std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
        const double frameDuration = 1000.0 / frameRate;
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
                // SNAP WINDOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW
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
std::mutex mtx;
class change_desktop {
    public: // constructor
        change_desktop(xcb_connection_t * connection) 
        : connection(connection) {}
    ;
    public: // methods
        enum DIRECTION {
            NEXT,
            PREV
        };
        enum DURATION {
            DURATION = 100
        };
        void change_to(const DIRECTION & direction) {
            switch (direction) {
                case NEXT: {
                    if (wm->cur_d->desktop == wm->desktop_list.size()) {
                        return;
                    }

                    hide = get_clients_on_desktop(wm->cur_d->desktop);
                    show = get_clients_on_desktop(wm->cur_d->desktop + 1);
                    animate(show, NEXT);
                    animate(hide, NEXT);
                    wm->cur_d = wm->desktop_list[wm->cur_d->desktop];
                    break;
                }
                case PREV: {
                    if (wm->cur_d->desktop == 1) {
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
        static void teleport_to(const uint8_t & n) {
            if (wm->cur_d == wm->desktop_list[n - 1] || n == 0 || n == wm->desktop_list.size()) {
                return;
            }
            
            for (const auto & c : wm->cur_d->current_clients) {
                if (c) {
                    if (c->desktop == wm->cur_d->desktop) {
                        c->unmap();
                    }
                }
            }

            wm->cur_d = wm->desktop_list[n - 1];
            for (const auto & c : wm->cur_d->current_clients) {
                if (c) {
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
        std::vector<client *> get_clients_on_desktop(const uint8_t & desktop) {
            std::vector<client *> clients;
            for (const auto & c : wm->client_list) {
                if (c->desktop == desktop) {
                    clients.push_back(c);
                }
            }
            return clients;
        }
        void animate(std::vector<client *> clients, const DIRECTION & direction) {
            switch (direction) {
                case NEXT: {
                    for (const auto c : clients) {
                        if (c) {
                            animation_threads.emplace_back(&change_desktop::anim_cli, this, c, c->x - screen->width_in_pixels);
                        }
                    }
                    break;
                }
                case PREV: {
                    for (const auto & c : clients) {
                        if (c) {
                            animation_threads.emplace_back(&change_desktop::anim_cli, this, c, c->x + screen->width_in_pixels);
                        }
                    }
                    break;
                }
            }
        }
        void anim_cli(client * c, const int & endx) {
            Mwm_Animator anim(c);
            anim.animate_client_x(c->x, endx, DURATION);
            c->update();
        }
        void thread_sleep(const double & milliseconds) {
            // Creating a duration with double milliseconds
            auto duration = std::chrono::duration<double, std::milli>(milliseconds);

            // Sleeping for the duration
            std::this_thread::sleep_for(duration);
        }
        void stopAnimations() {
            stop_show_flag.store(true);
            stop_hide_flag.store(true);
            
            if (show_thread.joinable()) {
                show_thread.join();
                stop_show_flag.store(false);
            }
            if (hide_thread.joinable()) {
                hide_thread.join();
                stop_hide_flag.store(false);
            }
        }
        void joinAndClearThreads() {
            for (std::thread & t : animation_threads) {
                if (t.joinable()) {
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
    if (wm->cur_d->desktop == 1) {
        return;
    }

    if (wm->focused_client) {
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
        : c(c) {
            if (c->win.is_EWMH_fullscreen()) {
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
                : c(c) {
                    if (c->win.is_EWMH_fullscreen()) {
                        return;
                    }

                    std::map<client *, edge> map = wm->get_client_next_to_client(c, _edge);
                    for (const auto & pair : map) {
                        if (pair.first != nullptr) {
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
                void resize_client(client * c, const uint32_t x, const uint32_t y, edge edge) {
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
                void snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox) {
                    uint16_t left_border   = 0;
                    uint16_t right_border  = 0;
                    uint16_t top_border    = 0;
                    uint16_t bottom_border = 0;

                    for (const auto & c : wm->cur_d->current_clients) {
                        if (c == this->c) {
                            continue;
                        }

                        left_border = c->x;
                        right_border = (c->x + c->width);
                        top_border = c->y;
                        bottom_border = (c->y + c->height);

                        if (edge != edge::RIGHT
                         && edge != edge::BOTTOM_RIGHT
                         && edge != edge::TOP_RIGHT) {
                            if ((x > right_border - prox && x < right_border + prox)
                             && (y > top_border && y < bottom_border)) {
                                resize_client(right_border, y, edge);
                                return;
                            }
                        }

                        if (edge != edge::LEFT
                         && edge != edge::TOP_LEFT
                         && edge != edge::BOTTOM_LEFT) {
                            if ((x > left_border - prox && x < left_border + prox)
                             && (y > top_border && y < bottom_border)) {
                                resize_client(left_border, y, edge);
                                return;
                            }
                        }

                        if (edge != edge::BOTTOM_edge 
                         && edge != edge::BOTTOM_LEFT 
                         && edge != edge::BOTTOM_RIGHT) {
                            if ((y > bottom_border - prox && y < bottom_border + prox)
                             && (x > left_border && x < right_border)) {
                                resize_client(x, bottom_border, edge);
                                return;
                            }
                        }

                        if (edge != edge::TOP
                         && edge != edge::TOP_LEFT
                         && edge != edge::TOP_RIGHT) {
                            if ((y > top_border - prox && y < top_border + prox)
                             && (x > left_border && x < right_border)) {
                                resize_client(x, top_border, edge);
                                return;
                            }
                        }
                    }
                    resize_client(x, y, edge);
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
                void run_double(edge edge) {
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
                                    resize_client(c, e->root_x, e->root_y, edge);
                                    resize_client(c2, e->root_x, e->root_y, c2_edge);
                                    xcb_flush(conn); 
                                }
                                break;
                            }
                            case XCB_BUTTON_RELEASE: {
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

                    if (elapsedTime.count() >= frameDuration) {
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
        void snap(const uint16_t & x, const uint16_t & y) {
            // WINDOW TO WINDOW SNAPPING 
            for (const auto & cli : wm->cur_d->current_clients) {
                if (cli == this->c) {
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
        void run() {
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
                            snap(e->root_x, e->root_y);
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
        bool isTimeToRender() 
        {
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
class max_win {
    public: // constructor
        enum max_win_type {
            BUTTON_MAXWIN,
            EWMH_MAXWIN
        };
        max_win(client * c, max_win_type type) 
        : c(c) {
            switch (type) {
                case EWMH_MAXWIN:
                    if (c->win.is_EWMH_fullscreen()) {
                        ewmh_unmax_win();
                    } else {
                        ewmh_max_win();
                    }
                    break;
                case BUTTON_MAXWIN:
                    if (is_max_win()) {
                        button_unmax_win();
                    } else {
                        button_max_win();
                    }
                    break;
            }
        }
    ;
    private:
        client * c;
    ;
    private: // functions
        void max_win_animate(const int & endX, const int & endY, const int & endWidth, const int & endHeight) {
            animate_client(
                c, 
                endX, 
                endY, 
                endWidth, 
                endHeight, 
                MAXWIN_ANIMATION_DURATION
            );
        }
        void save_max_ewmh_ogsize() {
            c->max_ewmh_ogsize.x      = c->x;
            c->max_ewmh_ogsize.y      = c->y;
            c->max_ewmh_ogsize.width  = c->width;
            c->max_ewmh_ogsize.height = c->height;
        }
        void ewmh_max_win() {
            save_max_ewmh_ogsize();
            max_win_animate(
                - BORDER_SIZE,
                - TITLE_BAR_HEIGHT - BORDER_SIZE,
                screen->width_in_pixels + (BORDER_SIZE * 2),
                screen->height_in_pixels + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)
            );
            c->win.set_EWMH_fullscreen_state();
            xcb_flush(conn);
        }
        void ewmh_unmax_win() {
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
                c->max_ewmh_ogsize.x, 
                c->max_ewmh_ogsize.y, 
                c->max_ewmh_ogsize.width, 
                c->max_ewmh_ogsize.height
            );
            c->win.unset_EWMH_fullscreen_state();
            xcb_flush(conn);
        }
        void save_max_button_ogsize() {
            c->max_button_ogsize.x      = c->x;
            c->max_button_ogsize.y      = c->y;
            c->max_button_ogsize.width  = c->width;
            c->max_button_ogsize.height = c->height;
        }
        void button_max_win() {
            save_max_button_ogsize();
            max_win_animate(
                0,
                0,
                screen->width_in_pixels,
                screen->height_in_pixels
            );
            xcb_flush(conn);
        }
        void button_unmax_win() {
            max_win_animate(
                c->max_button_ogsize.x,
                c->max_button_ogsize.y,
                c->max_button_ogsize.width,
                c->max_button_ogsize.height
            );
            xcb_flush(conn);
        }
        bool is_max_win() {
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
    xcb_visualtype_t * find_argb_visual(xcb_connection_t *conn, xcb_screen_t *screen) {
        xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

        for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
            xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
                if (depth_iter.data->depth == 32) {
                    return visual_iter.data;
                }
            }
        }
        return NULL;
    }
    void close_button_kill(client * c) {
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
        : c(c) {
            if (c->win.is_EWMH_fullscreen()) {
                return;
            }
            switch (tile) {
                case TILE::LEFT:
                    // IF 'CURRENTLT_TILED' TO 'LEFT'
                    if (current_tile_pos(TILEPOS::LEFT)) { 
                        set_tile_ogsize();
                        return;
                    }
                    // IF 'CURRENTLY_TILED' TO 'RIGHT', 'LEFT_DOWN' OR 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT)
                     || current_tile_pos(TILEPOS::LEFT_DOWN)
                     || current_tile_pos(TILEPOS::LEFT_UP)) {
                        set_tile_sizepos(TILEPOS::LEFT);
                        return;
                    }
                    // IF 'CURRENTLY_TILED' TO 'RIGHT_DOWN'
                    if (current_tile_pos(TILEPOS::RIGHT_DOWN)) { 
                        set_tile_sizepos(TILEPOS::LEFT_DOWN);
                        return;
                    }
                    // IF 'CURRENTLY_TILED' TO 'RIGHT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT_UP)) {
                        set_tile_sizepos(TILEPOS::LEFT_UP);
                        return;
                    }

                    save_tile_ogsize();
                    set_tile_sizepos(TILEPOS::LEFT);
                    break;
                case TILE::RIGHT:
                    // IF 'CURRENTLY_TILED' TO 'RIGHT'
                    if (current_tile_pos(TILEPOS::RIGHT)) {
                        set_tile_ogsize();
                        return;
                    }
                    // IF 'CURRENTLT_TILED' TO 'LEFT', 'RIGHT_DOWN' OR 'RIGHT_UP' 
                    if (current_tile_pos(TILEPOS::LEFT)
                     || current_tile_pos(TILEPOS::RIGHT_UP)
                     || current_tile_pos(TILEPOS::RIGHT_DOWN)) {
                        set_tile_sizepos(TILEPOS::RIGHT);
                        return;
                    }
                    // IF 'CURRENTLT_TILED' 'LEFT_DOWN'
                    if (current_tile_pos(TILEPOS::LEFT_DOWN)) {
                        set_tile_sizepos(TILEPOS::RIGHT_DOWN);
                        return;
                    }
                    // IF 'CURRENTLY_TILED' 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::LEFT_UP)) {
                        set_tile_sizepos(TILEPOS::RIGHT_UP);
                        return;
                    }

                    save_tile_ogsize();
                    set_tile_sizepos(TILEPOS::RIGHT);
                    break;
                case TILE::DOWN:
                    // IF 'CURRENTLY_TILED' 'LEFT' OR 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::LEFT)
                     || current_tile_pos(TILEPOS::LEFT_UP)) {
                        set_tile_sizepos(TILEPOS::LEFT_DOWN);
                        return;
                    }
                    // IF 'CURRENTLY_TILED' 'RIGHT' OR 'RIGHT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT) 
                     || current_tile_pos(TILEPOS::RIGHT_UP)) {
                        set_tile_sizepos(TILEPOS::RIGHT_DOWN);
                        return;
                    }
                    // IF 'CURRENTLY_TILED' 'LEFT_DOWN' OR 'RIGHT_DOWN'
                    if (current_tile_pos(TILEPOS::LEFT_DOWN)
                     || current_tile_pos(TILEPOS::RIGHT_DOWN)) {
                        set_tile_ogsize();
                        return;
                    }
                    break;
                case TILE::UP:
                    // IF 'CURRENTLY_TILED' 'LEFT'
                    if (current_tile_pos(TILEPOS::LEFT)
                     || current_tile_pos(TILEPOS::LEFT_DOWN)) {
                        set_tile_sizepos(TILEPOS::LEFT_UP);
                        return;
                    }
                    // IF 'CURRENTLY_TILED' 'RIGHT' OR RIGHT_DOWN
                    if (current_tile_pos(TILEPOS::RIGHT)
                     || current_tile_pos(TILEPOS::RIGHT_DOWN)) {
                        set_tile_sizepos(TILEPOS::RIGHT_UP);
                        return;
                    }
                    break;
            }
        }
    ;
    private: // variabels
        client * c;
    ;
    private: // functions
        void save_tile_ogsize() {
            c->tile_ogsize.x      = c->x;
            c->tile_ogsize.y      = c->y;
            c->tile_ogsize.width  = c->width;
            c->tile_ogsize.height = c->height;
        }
        bool current_tile_pos(TILEPOS mode) {
            switch (mode) {
                case TILEPOS::LEFT:
                    if (c->x        == 0 
                     && c->y        == 0 
                     && c->width    == screen->width_in_pixels / 2 
                     && c->height   == screen->height_in_pixels) {
                        return true;
                    }
                    break;
                case TILEPOS::RIGHT:
                    if (c->x        == screen->width_in_pixels / 2 
                     && c->y        == 0 
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels) {
                        return true;
                    }
                    break;
                case TILEPOS::LEFT_DOWN:
                    if (c->x        == 0
                     && c->y        == screen->height_in_pixels / 2
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2) {
                        return true;
                    }
                    break;
                case TILEPOS::RIGHT_DOWN:
                    if (c->x        == screen->width_in_pixels / 2
                     && c->y        == screen->height_in_pixels / 2
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2) {
                        return true;
                    }
                    break;
                case TILEPOS::LEFT_UP:
                    if (c->x        == 0
                     && c->y        == 0
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2) {
                        return true;
                    }
                    break;
                case TILEPOS::RIGHT_UP:
                    if (c->x        == screen->width_in_pixels / 2
                     && c->y        == 0
                     && c->width    == screen->width_in_pixels / 2
                     && c->height   == screen->height_in_pixels / 2) {
                        return true;
                    }
                    break;
            }
            return false;
        }
        void set_tile_sizepos(TILEPOS sizepos) {
            switch (sizepos) {
                case TILEPOS::LEFT:
                    animate(
                        0,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels
                    );
                    return;
                case TILEPOS::RIGHT:
                    animate(
                        screen->width_in_pixels / 2,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels
                    );
                    return;
                case TILEPOS::LEFT_DOWN:
                    animate(
                        0,
                        screen->height_in_pixels / 2,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                case TILEPOS::RIGHT_DOWN:
                    animate(
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                case TILEPOS::LEFT_UP:
                    animate(
                        0,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
                case TILEPOS::RIGHT_UP:
                    animate(
                        screen->width_in_pixels / 2,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2
                    );
                    return;
            }
        }
        void set_tile_ogsize() {
            animate(
                c->tile_ogsize.x,
                c->tile_ogsize.y,
                c->tile_ogsize.width,
                c->tile_ogsize.height
            );
        }
        void animate(const int & end_x, const int & end_y, const int & end_width, const int & end_height) {
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
    ;
};
class Events {
    public: // constructor and destructor
        Events() {}
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
                switch (e->state) {
                    case CTRL + ALT:
                        wm->launcher.program((char *) "konsole");
                        break;
                }
            }
            if (e->detail == wm->key_codes.q) {
                switch (e->state) {
                    case SHIFT + ALT:
                        wm->quit(0);
                        break;
                }
            }
            if (e->detail == wm->key_codes.f11) {
                client * c = wm->client_from_window(& e->event);
                max_win(c, max_win::EWMH_MAXWIN);
            }
            if (e->detail == wm->key_codes.n_1) {
                switch (e->state) {
                    case ALT:
                        change_desktop::teleport_to(1);
                        break;
                }
            }
            if (e->detail == wm->key_codes.n_2) {
                switch (e->state) {
                    case ALT:
                        change_desktop::teleport_to(2);
                        break;
                }
            }
            if (e->detail == wm->key_codes.n_3) {
                switch (e->state) {
                    case ALT:
                        change_desktop::teleport_to(3);
                        break;
                }
            }
            if (e->detail == wm->key_codes.n_4) {
                switch (e->state) {
                    case ALT:
                        change_desktop::teleport_to(4);
                        break;
                }
            }
            if (e->detail == wm->key_codes.n_5) {
                switch (e->state) {
                    case ALT:
                        change_desktop::teleport_to(5);
                        break;
                }
            }
            if (e->detail == wm->key_codes.r_arrow) {
                switch (e->state) {
                    case SHIFT + CTRL + SUPER: {
                        move_to_next_desktop_w_app();
                        break;
                    }
                    case CTRL + SUPER: {
				        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::NEXT);
                        break;
                    }
                    case SUPER: {
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::RIGHT);
                        break;
                    }
                    return;
                }
            }
            if (e->detail == wm->key_codes.l_arrow) {
                switch (e->state) {
                    case SHIFT + CTRL + SUPER: {
                        move_to_previus_desktop_w_app();
                        break;
                    }
                    case CTRL + SUPER: {
				        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::PREV);
                        break;
                    }
                    case SUPER: {
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::LEFT);
                        break;
                    }
                }
            }
            if (e->detail == wm->key_codes.d_arrow) {
                switch (e->state) {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::DOWN);
                        return;
                        break;
                }
            }
            if (e->detail == wm->key_codes.u_arrow) {
                switch (e->state) {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::UP);
                        break;
                }
            }
            if (e->detail == wm->key_codes.tab) {
                switch (e->state) {
                    case ALT:
                        wm->cycle_focus();
                        break;
                }
            }
            if (e->detail == wm->key_codes.k) {
                switch (e->state) {
                    case SUPER:
                        client * c = wm->client_from_window(& e->event);
                        if (!c) {
                            return;
                        }

                        if (c->win.is_mask_active(XCB_EVENT_MASK_ENTER_WINDOW)) {
                            log_info("event_mask is active");
                        } else {
                            log_info("event_mask is NOT active");
                        }
                        break;
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
                if (c) {
                    if (e->detail == L_MOUSE_BUTTON) {
                        c->raise();
                        resize_client::no_border border(c, 0, 0);
                        wm->focus_client(c);
                    }
                    return;
                }
            }
            if (e->event == wm->root) {
                if (e->detail == R_MOUSE_BUTTON) {
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
                    switch (e->state) {
                        case ALT:
                            c->raise();
                            mv_client mv(c, e->event_x, e->event_y + 20);
                            wm->focus_client(c);
                            break;
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
    dock->add_app("google-chrome-stable");
    dock->add_app("code");
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