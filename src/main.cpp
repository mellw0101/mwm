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

win_data data;
static pointer * pointer;

static Launcher * launcher;
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
    ;
};

// class Window_Manager
// {
//     public: // constructor
//         Window_Manager() {}
//     ;
//     public: // variabels 
//         context_menu * context_menu = nullptr;
//         window root;
//         Event_Handler event_handler;
//     ;
//     public: // methods 
//         public: // main methods 
//             void
//             init()
//             {
//                 _conn(nullptr, nullptr);
//                 _setup();
//                 _iter();
//                 _screen();
//                 root = screen->root;
//                 root.width(screen->width_in_pixels);
//                 root.height(screen->height_in_pixels);
//                 setSubstructureRedirectMask();
//                 configure_root();
//                 _ewmh();

//                 create_new_desktop(1);
//                 create_new_desktop(2);
//                 create_new_desktop(3);
//                 create_new_desktop(4);
//                 create_new_desktop(5);

//                 context_menu = new class context_menu();
//                 context_menu->add_entry
//                 (   
//                     "konsole",
//                     []() 
//                     {
//                         launcher->program((char *) "/usr/bin/konsole");
//                     }
//                 );
//             }

//             void
//             launch_program(char * program)
//             {
//                 if (fork() == 0) 
//                 {
//                     setsid();
//                     execvp(program, (char *[]) { program, NULL });
//                 }
//             }

//             void
//             quit(const int & status)
//             {
//                 xcb_flush(conn);
//                 delete_client_vec(client_list);
//                 delete_desktop_vec(desktop_list);
//                 xcb_ewmh_connection_wipe(ewmh);
//                 xcb_disconnect(conn);
//                 exit(status);
//             }
//         ;
//         public: // client methods 
//             public: // focus methods 
//                 void 
//                 focus_client(client * c)
//                 {
//                     if (!c)
//                     {
//                         log_error("c is null");
//                         return;
//                     }

//                     focused_client = c;
//                     c->focus();
//                 }

//                 void
//                 cycle_focus()
//                 {
//                     bool focus = false;
//                     for (auto & c : client_list)
//                     {
//                         if (c)
//                         {
//                             if (c == focused_client)
//                             {
//                                 focus = true;
//                                 continue;
//                             }
                            
//                             if (focus)
//                             {
//                                 focus_client(c);
//                                 return;  
//                             }
//                         }
//                     }
//                 }
//             ;

//             public: // client fetch methods
//                 client * 
//                 client_from_window(const xcb_window_t * window) 
//                 {
//                     for (const auto & c : client_list) 
//                     {
//                         if (* window == c->win) 
//                         {
//                             return c;
//                         }
//                     }
//                     return nullptr;
//                 }

//                 client * 
//                 client_from_any_window(const xcb_window_t * window) 
//                 {
//                     for (const auto & c : client_list) 
//                     {
//                         if (* window == c->win 
//                          || * window == c->frame 
//                          || * window == c->titlebar 
//                          || * window == c->close_button 
//                          || * window == c->max_button 
//                          || * window == c->min_button 
//                          || * window == c->border.left 
//                          || * window == c->border.right 
//                          || * window == c->border.top 
//                          || * window == c->border.bottom
//                          || * window == c->border.top_left
//                          || * window == c->border.top_right
//                          || * window == c->border.bottom_left
//                          || * window == c->border.bottom_right) 
//                         {
//                             return c;
//                         }
//                     }
//                     return nullptr;
//                 }

//                 client *
//                 client_from_pointer(const int & prox)
//                 {
//                     const uint32_t & x = pointer->x();
//                     const uint32_t & y = pointer->y();
//                     for (const auto & c : cur_d->current_clients)
//                     {
//                         // LEFT EDGE OF CLIENT
//                         if (x > c->x - prox && x <= c->x)
//                         {
//                             return c;
//                         }

//                         // RIGHT EDGE OF CLIENT
//                         if (x >= c->x + c->width && x < c->x + c->width + prox)
//                         {
//                             return c;
//                         }

//                         // TOP EDGE OF CLIENT
//                         if (y > c->y - prox && y <= c->y)
//                         {
//                             return c;
//                         }

//                         // BOTTOM EDGE OF CLIENT
//                         if (y >= c->y + c->height && y < c->y + c->height + prox)
//                         {
//                             return c;
//                         }
//                     }
//                     return nullptr;
//                 }

//                 std::map<client *, edge>
//                 get_client_next_to_client(client * c, edge c_edge)
//                 {
//                     std::map<client *, edge> map;
//                     for (client * c2 : cur_d->current_clients)
//                     {
//                         if (c == c2)
//                         {
//                             continue;
//                         }

//                         if (c_edge == edge::LEFT)
//                         {
//                             if (c->x == c2->x + c2->width)
//                             {
//                                 map[c2] = edge::RIGHT;
//                                 return map;
//                             }
//                         }
                        
//                         if (c_edge == edge::RIGHT)
//                         {
//                             if (c->x + c->width == c2->x)
//                             {
//                                 map[c2] = edge::LEFT;
//                                 return map;
//                             }
//                         }
                        
//                         if (c_edge == edge::TOP)
//                         {
//                             if (c->y == c2->y + c2->height)
//                             {
//                                 map[c2] = edge::BOTTOM_edge;
//                                 return map;
//                             }
//                         }

//                         if (c_edge == edge::BOTTOM_edge)
//                         {
//                             if (c->y + c->height == c2->y)
//                             {
//                                 map[c2] = edge::TOP;
//                                 return map;
//                             }
//                         }
//                     }

//                     map[nullptr] = edge::NONE;
//                     return map;
//                 }

//                 static edge
//                 get_client_edge_from_pointer(client * c, const int & prox)
//                 {
//                     const uint32_t & x = pointer->x();
//                     const uint32_t & y = pointer->y();

//                     const uint32_t & top_border = c->y;
//                     const uint32_t & bottom_border = (c->y + c->height);
//                     const uint32_t & left_border = c->x;
//                     const uint32_t & right_border = (c->x + c->width);

//                     // TOP EDGE OF CLIENT
//                     if (((y > top_border - prox) && (y <= top_border))
//                      && ((x > left_border + prox) && (x < right_border - prox)))
//                     {
//                         return edge::TOP;
//                     }

//                     // BOTTOM EDGE OF CLIENT
//                     if (((y >= bottom_border) && (y < bottom_border + prox))
//                      && ((x > left_border + prox) && (x < right_border - prox)))
//                     {
//                         return edge::BOTTOM_edge;
//                     }

//                     // LEFT EDGE OF CLIENT
//                     if (((x > left_border) - prox && (x <= left_border))
//                      && ((y > top_border + prox) && (y < bottom_border - prox)))
//                     {
//                         return edge::LEFT;
//                     }

//                     // RIGHT EDGE OF CLIENT
//                     if (((x >= right_border) && (x < right_border + prox))
//                      && ((y > top_border + prox) && (y < bottom_border - prox)))
//                     {
//                         return edge::RIGHT;
//                     }

//                     // TOP LEFT CORNER OF CLIENT
//                     if (((x > left_border - prox) && x < left_border + prox)
//                      && ((y > top_border - prox) && y < top_border + prox))
//                     {
//                         return edge::TOP_LEFT;
//                     }

//                     // TOP RIGHT CORNER OF CLIENT
//                     if (((x > right_border - prox) && x < right_border + prox)
//                      && ((y > top_border - prox) && y < top_border + prox))
//                     {
//                         return edge::TOP_RIGHT;
//                     }

//                     // BOTTOM LEFT CORNER OF CLIENT
//                     if (((x > left_border - prox) && x < left_border + prox)
//                      && ((y > bottom_border - prox) && y < bottom_border + prox))
//                     {
//                         return edge::BOTTOM_LEFT;
//                     }

//                     // BOTTOM RIGHT CORNER OF CLIENT
//                     if (((x > right_border - prox) && x < right_border + prox)
//                      && ((y > bottom_border - prox) && y < bottom_border + prox))
//                     {
//                         return edge::BOTTOM_RIGHT;
//                     }

//                     return edge::NONE;
//                 }
//             ;

//             void 
//             manage_new_client(const uint32_t & window)
//             {
//                 client * c = make_client(window);
//                 if (!c)
//                 {
//                     log_error("could not make client");
//                     return;
//                 }

//                 c->win.x_y_width_height(c->x, c->y, c->width, c->height);
//                 c->win.map();
//                 c->win.grab_button(
//                 {
//                     {   L_MOUSE_BUTTON,     ALT },
//                     {   R_MOUSE_BUTTON,     ALT },
//                     {   L_MOUSE_BUTTON,     0   }
//                 });
//                 c->win.grab_keys(
//                 {
//                     {   T,          ALT | CTRL              },
//                     {   Q,          ALT | SHIFT             },
//                     {   F11,        NULL                    },
//                     {   N_1,        ALT                     },
//                     {   N_2,        ALT                     },
//                     {   N_3,        ALT                     },
//                     {   N_4,        ALT                     },
//                     {   N_5,        ALT                     },
//                     {   R_ARROW,    CTRL | SUPER            },
//                     {   L_ARROW,    CTRL | SUPER            },
//                     {   R_ARROW,    CTRL | SUPER | SHIFT    },
//                     {   L_ARROW,    CTRL | SUPER | SHIFT    },
//                     {   R_ARROW,    SUPER                   },
//                     {   L_ARROW,    SUPER                   },
//                     {   U_ARROW,    SUPER                   },
//                     {   D_ARROW,    SUPER                   },
//                     {   TAB,        ALT                     },
//                     {   K,          SUPER                   }
//                 });
//                 c->make_decorations();
//                 uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
//                 c->win.apply_event_mask(& mask);

//                 c->update();
//                 focus_client(c);
//             }

//             int
//             send_sigterm_to_client(client * c)
//             {
//                 if (!c)
//                 {
//                     return - 1;
//                 }

//                 c->win.unmap();
//                 c->close_button.unmap();
//                 c->max_button.unmap();
//                 c->min_button.unmap();
//                 c->titlebar.unmap();
//                 c->border.left.unmap();
//                 c->border.right.unmap();
//                 c->border.top.unmap();
//                 c->border.bottom.unmap();
//                 c->border.top_left.unmap();
//                 c->border.top_right.unmap();
//                 c->border.bottom_left.unmap();
//                 c->border.bottom_right.unmap();
//                 c->frame.unmap();

//                 c->win.kill();
//                 c->close_button.kill();
//                 c->max_button.kill();
//                 c->min_button.kill();
//                 c->titlebar.kill();
//                 c->border.left.kill();
//                 c->border.right.kill();
//                 c->border.top.kill();
//                 c->border.bottom.kill();
//                 c->border.top_left.kill();
//                 c->border.top_right.kill();
//                 c->border.bottom_left.kill();
//                 c->border.bottom_right.kill();
//                 c->frame.kill();
                
//                 remove_client(c);

//                 return 0;
//             }
//         ;
//         public: // desktop methods 
//             void
//             create_new_desktop(const uint16_t & n)
//             {
//                 desktop * d = new desktop;
//                 d->desktop  = n;
//                 d->width    = screen->width_in_pixels;
//                 d->height   = screen->height_in_pixels;
//                 cur_d       = d;
//                 desktop_list.push_back(d);
//             }
//         ;
//     ;
//     private: // variables 
//         window start_window;
//     ;
//     private: // functions 
//         private: // init functions 
//             void 
//             _conn(const char * displayname, int * screenp) 
//             {
//                 conn = xcb_connect(displayname, screenp);
//                 check_conn();
//             }

//             void 
//             _ewmh() 
//             {
//                 if (!(ewmh = static_cast<xcb_ewmh_connection_t *>(calloc(1, sizeof(xcb_ewmh_connection_t)))))
//                 {
//                     log_error("ewmh faild to initialize");
//                     quit(1);
//                 }    
                
//                 xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms(conn, ewmh);
                
//                 if (!(xcb_ewmh_init_atoms_replies(ewmh, cookie, 0)))
//                 {
//                     log_error("xcb_ewmh_init_atoms_replies:faild");
//                     quit(1);
//                 }

//                 const char * str = "mwm";
//                 check_error
//                 (
//                     conn, 
//                     xcb_ewmh_set_wm_name
//                     (
//                         ewmh, 
//                         screen->root, 
//                         strlen(str), 
//                         str
//                     ), 
//                     __func__, 
//                     "xcb_ewmh_set_wm_name"
//                 );
//             }

//             void 
//             _setup() 
//             {
//                 setup = xcb_get_setup(conn);
//             }

//             void 
//             _iter() 
//             {
//                 iter = xcb_setup_roots_iterator(setup);
//             }

//             void 
//             _screen() 
//             {
//                 screen = iter.data;
//             }

//             bool
//             setSubstructureRedirectMask() 
//             {
//                 // ATTEMPT TO SET THE SUBSTRUCTURE REDIRECT MASK
//                 xcb_void_cookie_t cookie = xcb_change_window_attributes_checked
//                 (
//                     conn,
//                     root,
//                     XCB_CW_EVENT_MASK,
//                     (const uint32_t[1])
//                     {
//                         XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
//                     }
//                 );

//                 // CHECK IF ANOTHER WINDOW MANAGER IS RUNNING
//                 xcb_generic_error_t * error = xcb_request_check(conn, cookie);
//                 if (error) 
//                 {
//                     log_error("Error: Another window manager is already running or failed to set SubstructureRedirect mask."); 
//                     free(error);
//                     return false;
//                 }
//                 return true;
//             }

//             void 
//             configure_root()
//             {
//                 root.set_backround_color(DARK_GREY);
//                 uint32_t mask = 
//                     XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
//                     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
//                     XCB_EVENT_MASK_ENTER_WINDOW |
//                     XCB_EVENT_MASK_LEAVE_WINDOW |
//                     XCB_EVENT_MASK_STRUCTURE_NOTIFY |
//                     XCB_EVENT_MASK_BUTTON_PRESS |
//                     XCB_EVENT_MASK_BUTTON_RELEASE |
//                     XCB_EVENT_MASK_KEY_PRESS |
//                     XCB_EVENT_MASK_KEY_RELEASE |
//                     XCB_EVENT_MASK_FOCUS_CHANGE |
//                     XCB_EVENT_MASK_POINTER_MOTION
//                 ; 
//                 root.apply_event_mask(& mask);
//                 root.clear();

//                 root.set_backround_png("/home/mellw/mwm_png/galaxy17.png");
//                 root.set_pointer(CURSOR::arrow);
//             }
//         ;
//         private: // check functions 
//             void
//             check_error(const int & code)
//             {
//                 switch (code) 
//                 {
//                     case CONN_ERR:
//                         log_error("Connection error.");
//                         quit(CONN_ERR);
//                         break;
//                     ;
//                     case EXTENTION_NOT_SUPPORTED_ERR:
//                         log_error("Extension not supported.");
//                         quit(EXTENTION_NOT_SUPPORTED_ERR);
//                         break;
//                     ;
//                     case MEMORY_INSUFFICIENT_ERR:
//                         log_error("Insufficient memory.");
//                         quit(MEMORY_INSUFFICIENT_ERR);
//                         break;
//                     ;
//                     case REQUEST_TO_LONG_ERR:
//                         log_error("Request to long.");
//                         quit(REQUEST_TO_LONG_ERR);
//                         break;
//                     ;
//                     case PARSE_ERR:
//                         log_error("Parse error.");
//                         quit(PARSE_ERR);
//                         break;
//                     ;
//                     case SCREEN_NOT_FOUND_ERR:
//                         log_error("Screen not found.");
//                         quit(SCREEN_NOT_FOUND_ERR);
//                         break;
//                     ;
//                     case FD_ERR:
//                         log_error("File descriptor error.");
//                         quit(FD_ERR);
//                         break;
//                     ;
//                 }
//             }

//             void
//             check_conn()
//             {
//                 int status = xcb_connection_has_error(conn);
//                 check_error(status);
//             }

//             int
//             cookie_error(xcb_void_cookie_t cookie , const char * sender_function)
//             {
//                 xcb_generic_error_t * err = xcb_request_check(conn, cookie);
//                 if (err)
//                 {
//                     log_error(err->error_code);
//                     free(err);
//                     return err->error_code;
//                 }
//                 return 0;
//             }

//             void
//             check_error(xcb_connection_t * connection, xcb_void_cookie_t cookie , const char * sender_function, const char * err_msg)
//             {
//                 xcb_generic_error_t * err = xcb_request_check(connection, cookie);
//                 if (err)
//                 {
//                     log_error_code(err_msg, err->error_code);
//                     free(err);
//                 }
//             }
//         ;

//         int
//         start_screen_window()
//         {
//             start_window.create_default(root, 0, 0, 0, 0);
//             start_window.set_backround_color(DARK_GREY);
//             start_window.map();
//             return 0;
//         }

//         private: // delete functions 
//             void
//             delete_client_vec(std::vector<client *> & vec)
//             {
//                 for (client * c : vec)
//                 {
//                     send_sigterm_to_client(c);
//                     xcb_flush(conn);                    
//                 }

//                 vec.clear();

//                 std::vector<client *>().swap(vec);
//             }

//             void
//             delete_desktop_vec(std::vector<desktop *> & vec)
//             {
//                 for (desktop * d : vec)
//                 {
//                     delete_client_vec(d->current_clients);
//                     delete d;
//                 }

//                 vec.clear();

//                 std::vector<desktop *>().swap(vec);
//             }

//             template <typename Type>
//             static void 
//             delete_ptr_vector(std::vector<Type *>& vec) 
//             {
//                 for (Type * ptr : vec) 
//                 {
//                     delete ptr;
//                 }
//                 vec.clear();

//                 std::vector<Type *>().swap(vec);
//             }

//             void
//             remove_client(client * c)
//             {
//                 client_list.erase(std::remove(client_list.begin(), client_list.end(), c), client_list.end());
//                 cur_d->current_clients.erase(std::remove(cur_d->current_clients.begin(), cur_d->current_clients.end(), c), cur_d->current_clients.end());
//                 delete c;
//             }

//             void
//             remove_client_from_vector(client * c, std::vector<client *> & vec)
//             {
//                 if (!c)
//                 {
//                     log_error("client is nullptr.");
//                 }
//                 vec.erase(std::remove(vec.begin(), vec.end(), c), vec.end());
//                 delete c;
//             }
//         ;
//         private: // client functions
//             client * 
//             make_client(const uint32_t & window) 
//             {
//                 client * c = new client;
//                 if (!c) 
//                 {
//                     log_error("Could not allocate memory for client");
//                     return nullptr;
//                 }

//                 c->win    = window;
//                 c->height = (data.height < 300) ? 300 : data.height;
//                 c->width  = (data.width < 400)  ? 400 : data.width;
//                 c->x      = (data.x <= 0)       ? (screen->width_in_pixels / 2)  - (c->width / 2)  : data.x;
//                 c->y      = (data.y <= 0)       ? (screen->height_in_pixels / 2) - (c->height / 2) : data.y;
//                 c->depth   = 24;
//                 c->desktop = cur_d->desktop;

//                 for (int i = 0; i < 256; ++i)
//                 {
//                     c->name[i] = '\0';
//                 }

//                 int i = 0;
//                 char * name = c->win.property("_NET_WM_NAME");
//                 while(name[i] != '\0' && i < 255)
//                 {
//                     c->name[i] = name[i];
//                     ++i;
//                 }
//                 c->name[i] = '\0';
//                 free(name);

//                 if (c->win.check_if_EWMH_fullscreen()) 
//                 {
//                     c->x      = 0;
//                     c->y      = 0;
//                     c->width  = screen->width_in_pixels;
//                     c->height = screen->height_in_pixels;
//                     c->win.set_EWMH_fullscreen_state();
//                 }
                
//                 client_list.push_back(c);
//                 cur_d->current_clients.push_back(c);
//                 return c;
//             }
//         ;
//     ;
// };
// static Window_Manager * wm;

class mv_client 
{
    public: // constructor 
        mv_client(client * & c, const uint16_t & start_x, const uint16_t & start_y) 
        : c(c), start_x(start_x), start_y(start_y)
        {
            if (c->win.check_if_EWMH_fullscreen())
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
        void 
        snap(const int16_t & x, const int16_t & y)
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

        void /*
            THIS IS THE MAIN EVENT LOOP FOR 'mv_client'
         */ 
        run() 
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
    public: // variables
        XCPPBAnimator(xcb_connection_t* connection, const xcb_window_t & window)
        : connection(connection), window(window) {}

        XCPPBAnimator(xcb_connection_t* connection, client * c)
        : connection(connection), c(c) {}

        XCPPBAnimator(xcb_connection_t * connection)
        : connection(connection) {}
    ;

    public: // methods
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

    private: // variabels
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
        const double frameRate = 120;
        const double frameDuration = 1000.0 / frameRate; 
    ;

    private: // funcrions        
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
                    c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                    break;
                }
                c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
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
        conf_client_x()
        {
            const uint32_t x = currentX;
            c->frame.x(x);
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
    c->update();
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
    c->update();
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

        static void
        teleport_to(const uint8_t & n)
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
            for (const auto & c : wm->client_list)
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

void
move_to_next_desktop_w_app()
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

void
move_to_previus_desktop_w_app()
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
            if (c->win.check_if_EWMH_fullscreen())
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
                    if (c->win.check_if_EWMH_fullscreen())
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
                void 
                teleport_mouse(edge edge) 
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

                void
                resize_client(const uint32_t x, const uint32_t y, edge edge)
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
                                c->update();
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
            public: // constructor 
                border(client * & c, edge _edge)
                : c(c)
                {
                    if (c->win.check_if_EWMH_fullscreen())
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
                void 
                teleport_mouse(edge edge) 
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

                void
                resize_client(const uint32_t x, const uint32_t y, edge edge)
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

                void
                resize_client(client * c, const uint32_t x, const uint32_t y, edge edge)
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

                void
                snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox)
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
                                c->update();
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

                bool 
                isTimeToRender() 
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
        void
        snap(const uint16_t & x, const uint16_t & y)
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
                        c->update();
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
                    if (c->win.check_if_EWMH_fullscreen())
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
            c->win.set_EWMH_fullscreen_state();
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
            c->win.unset_EWMH_fullscreen_state();
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

    void
    close_button_kill(client * c)
    {
        int result = wm->send_sigterm_to_client(c);

        if (result == -1)
        {
            log_error("client is nullptr");
        }
    }
}

/*
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
                                return format; / ** 
                                 *
                                 * If a visual with 32-bit depth is found, return its format 
                                 *
                                 * /
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
*/

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
            if (c->win.check_if_EWMH_fullscreen())
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
            c->update();
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
            c->update();
        }
    ;
};

class Event
{
    public: // constructor and destructor 
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
    ;
    public: // methods 
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

        void
        test()
        {
            wm->event_handler.setEventCallback(XCB_KEY_PRESS, [&](Ev ev){ key_press_handler(ev); });
            wm->event_handler.setEventCallback(XCB_MAP_NOTIFY, [&](Ev ev){ map_notify_handler(ev); });
            wm->event_handler.setEventCallback(XCB_MAP_REQUEST, [&](Ev ev){ map_req_handler(ev); });
            wm->event_handler.setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev){ button_press_handler(ev); });
            wm->event_handler.setEventCallback(XCB_CONFIGURE_REQUEST, [&](Ev ev){ configure_request_handler(ev); });
            wm->event_handler.setEventCallback(XCB_FOCUS_IN, [&](Ev ev){ focus_in_handler(ev); });
            wm->event_handler.setEventCallback(XCB_FOCUS_OUT, [&](Ev ev){ focus_out_handler(ev); });
            wm->event_handler.setEventCallback(XCB_DESTROY_NOTIFY, [&](Ev ev){ destroy_notify_handler(ev); });
            wm->event_handler.setEventCallback(XCB_UNMAP_NOTIFY, [&](Ev ev){ unmap_notify_handler(ev); });
            wm->event_handler.setEventCallback(XCB_REPARENT_NOTIFY, [&](Ev ev){ reparent_notify_handler(ev); });
            wm->event_handler.setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev){ enter_notify_handler(ev); });
            wm->event_handler.setEventCallback(XCB_LEAVE_NOTIFY, [&](Ev ev){ leave_notify_handler(ev); });
            wm->event_handler.setEventCallback(XCB_MOTION_NOTIFY, [&](Ev ev){ motion_notify_handler(ev); });
        }
    ;
    private: // variabels 
        xcb_key_symbols_t * keysyms;
        /*
            VARIABELS TO STORE KEYCODES
         */ 
        xcb_keycode_t t{}, q{}, f{}, f11{}, n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{}, l_arrow{}, u_arrow{}, d_arrow{}, tab{}, k{}; 
    ;
    private: // helper functions 
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
    ;
    private: // event handling functions 
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
                        launcher->program((char *) "/usr/bin/konsole");
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
                        wm->quit(0);
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
                client * c = wm->client_from_window(& e->event);
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
                        client * c = wm->client_from_window(& e->event);
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
                        client * c = wm->client_from_window(& e->event);
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
                        client * c = wm->client_from_window(& e->event);
                        tile(c, TILE::DOWN);
                        return;
                        break;
                    ;
                }
            }

            if (e->detail == u_arrow)
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

            /*
                CHECK IF 'ALT+TAB' WAS PRESSED
                IF SO CYCLE FOCUS
             */
            if (e->detail == tab) 
            {
                switch (e->state) 
                {
                    case ALT:
                        wm->cycle_focus();
                        break;
                    ;
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
                        client * c = wm->client_from_window(& e->event);
                        break;
                    ;
                }
            }
        }

        void 
        map_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_map_notify_event_t *>(ev);
            if (e->window == dock->main_window)
            {
                return;
            }
            if (e->window == wm->root)
            {
                return;
            }
            if (e->window == dock->add_app_dialog_window.window)
            {
                wm->manage_new_client(e->window);
            }
            client * c = wm->client_from_window(& e->window);
            if (c)
            {
                c->update();
            }
        }
        
        void 
        map_req_handler(const xcb_generic_event_t * & ev) 
        {
            const auto * e = reinterpret_cast<const xcb_map_request_event_t *>(ev);
            log_win("e->window: " ,e->window);
            wm->manage_new_client(e->window);
        }
        
        void 
        button_press_handler(const xcb_generic_event_t * & ev) 
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
            
            // dock
            if (e->detail == L_MOUSE_BUTTON)
            {
                dock->buttons.run_action(e->event);
            }
            
            if (e->event == dock->main_window)
            {
                if (e->detail == R_MOUSE_BUTTON)
                {
                    dock->context_menu.show();
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
            client * c = wm->client_from_window( & e->event);
            if (c)
            {
                c->win.ungrab_button({ { L_MOUSE_BUTTON, NULL } });
                c->raise();
                c->win.set_active_EWMH_window();
                wm->focused_client = c;
            }
        }

        void
        focus_out_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_focus_out_event_t *>(ev);
            
            client * c = wm->client_from_window(& e->event);
            if (!c)
            {
                return;
            }
            
            c->win.grab_button({ { L_MOUSE_BUTTON, NULL } });
        }

        void
        destroy_notify_handler(const xcb_generic_event_t * & ev)
        {
            const auto * e = reinterpret_cast<const xcb_destroy_notify_event_t *>(ev);
            client * c = wm->client_from_any_window(& e->window);
            int result = wm->send_sigterm_to_client(c);
            if (result == -1)
            {
                log_error("send_sigterm_to_client: failed");
            }
        }

        void
        unmap_notify_handler(const xcb_generic_event_t * & ev)
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
    // while (true)
    // {
    //     xcb_generic_event_t * ev = xcb_wait_for_event(conn);
    //     if (!ev) 
    //     {
    //         continue;
    //     }
    //     event.handler(ev);
    //     free(ev);
    // }
    event.test();
    wm->event_handler.run();
}

void
setup_wm()
{
    wm = new Window_Manager;
    wm->init();
    
    change_desktop::teleport_to(1);
    dock = new Dock;
    dock->add_app("konsole");
    dock->add_app("alacritty");
    dock->add_app("falkon");
    dock->init();

    pointer = new class pointer;
    launcher = new Launcher;
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