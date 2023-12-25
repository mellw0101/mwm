// // enum auth_protos 
// //         {
// //             AUTH_MC1,
// //             N_AUTH_PROTOS
// //         };

// //         #define AUTH_PROTO_XDM_AUTHORIZATION "XDM-AUTHORIZATION-1"
// //         #define AUTH_PROTO_MIT_MAGIC_COOKIE "MIT-MAGIC-COOKIE-1"

// //         // Initialize the vector with the protocol names
// //         std::vector<std::string> authnames_str = {
// //             AUTH_PROTO_MIT_MAGIC_COOKIE
// //         };

// //         // Create a vector of char* pointers to the C-style strings
// //         std::vector<char*> authnames_cstr;


// //         int authnameslen[N_AUTH_PROTOS] = 
// //         {
// //             static_cast<const int>(sizeof(AUTH_PROTO_MIT_MAGIC_COOKIE) - 1),
// //         };

// //         Xauth *
// //         get_authptr(struct sockaddr *sockname, int display) 
// //         {
// //             char *addr = nullptr;
// //             int addrlen = 0;
// //             unsigned short family;
// //             char hostnamebuf[256];  // big enough for max hostname
// //             char dispbuf[40];  // big enough to hold more than 2^64 base 10
// //             int dispbuflen;

// //             family = FamilyLocal; // Usually 256 for XCB_FAMILY_LOCAL
// //             switch(sockname->sa_family) 
// //             {
// //                 case AF_INET6: 
// //                 {
// //                     struct sockaddr_in6 *sockaddr_ipv6 = reinterpret_cast<struct sockaddr_in6 *>(sockname);
// //                     addr = reinterpret_cast<char *>(&sockaddr_ipv6->sin6_addr);
// //                     addrlen = sizeof(sockaddr_ipv6->sin6_addr);
// //                     if (!IN6_IS_ADDR_V4MAPPED(&sockaddr_ipv6->sin6_addr) 
// //                     && !IN6_IS_ADDR_LOOPBACK(&sockaddr_ipv6->sin6_addr)) 
// //                     {
// //                         family = XCB_FAMILY_INTERNET_6;
// //                     }
// //                     break;
// //                 }
// //                 case AF_INET: 
// //                 {
// //                     struct sockaddr_in *sockaddr_ipv4 = reinterpret_cast<struct sockaddr_in *>(sockname);
// //                     addr = reinterpret_cast<char *>(&sockaddr_ipv4->sin_addr);
// //                     addrlen = sizeof(sockaddr_ipv4->sin_addr);
// //                     if (sockaddr_ipv4->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) 
// //                     {
// //                         family = XCB_FAMILY_INTERNET;
// //                     }
// //                     break;
// //                 }
// //                 case AF_UNIX: 
// //                 {
// //                     // Nothing to do for Unix domain sockets
// //                     break;
// //                 }
// //                 default:
// //                 {
// //                     return nullptr; // Cannot authenticate this family
// //                 }
// //             }

// //             dispbuflen = snprintf(dispbuf, sizeof(dispbuf), "%d", display);
// //             if (dispbuflen < 0)
// //             {
// //                 return nullptr;
// //             }
// //             dispbuflen = std::min(dispbuflen, static_cast<int>(sizeof(dispbuf) - 1));

// //             if (family == FamilyLocal) 
// //             {
// //                 if (gethostname(hostnamebuf, sizeof(hostnamebuf)) == -1)
// //                 {
// //                     return nullptr; // Do not know own hostname
// //                 }
// //                 addr = hostnamebuf;
// //                 addrlen = strlen(addr);
// //             }

// //             for (const auto& name : authnames_str) 
// //             {
// //                 char* cstr = new char[name.size() + 1]; // +1 for null terminator
// //                 std::strcpy(cstr, name.c_str());
// //                 authnames_cstr.push_back(cstr);
// //             }

// //             return XauGetBestAuthByAddr(family,
// //                                         static_cast<unsigned short>(addrlen), 
// //                                         addr,
// //                                         static_cast<unsigned short>(dispbuflen), 
// //                                         dispbuf,
// //                                         N_AUTH_PROTOS, 
// //                                         authnames_cstr.data(), 
// //                                         authnameslen);
// //         }

        
// // #include "xcb_source/libxcb/xcbint.h"
// // #include "xcb_source/libxcb/xcb.h"
// // #include "xcb_source/libxcb/xcb_util.c"
// #include "xcb_source/libxcb/xcbint.h"
// #include <X11/Xauth.h>
// #include <iostream>
// #include <string>
// #include <cstring>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <stdlib.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <sys/un.h>
// #include <vector>
// #include <memory>
// #include <cassert>
// #include <cstdint>
// #include <ctime>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <fcntl.h>



// // #define MIN(a, b) ((a) < (b) ? (a) : (b))

// struct xpp_connection_t 
// {
//     int has_error;

//     /* constant data */
//     xcb_setup_t *setup;
//     int fd;

//     /* I/O data */
//     pthread_mutex_t iolock;
//     _xcb_in in;
//     _xcb_out out;

//     /* misc data */
//     _xcb_ext ext;
//     _xcb_xid xid;
// };

// /**
//  * @brief Container for authorization information.
//  *
//  * A container for authorization information to be sent to the X server.
//  */
// typedef struct xpp_auth_info_t 
// {
//     int   namelen;  /**< Length of the string name (as returned by strlen). */
//     char *name;     /**< String containing the authentication protocol name, such as "MIT-MAGIC-COOKIE-1" or "XDM-AUTHORIZATION-1". */
//     int   datalen;  /**< Length of the data member. */
//     char *data;   /**< Data interpreted in a protocol-specific manner. */
// } xpp_auth_info_t;

// class UnixSocket {
//     public:
//         UnixSocket(const char* file) 
//         {
//             // Initialize address
//             memset(&addr, 0, sizeof(addr));
//             addr.sun_family = AF_UNIX;
//             strncpy(addr.sun_path, file, sizeof(addr.sun_path) - 1);

//             // Create socket
//             fd = socket(AF_UNIX, SOCK_STREAM, 0);
//             if (fd == -1) 
//             {
//                 throw std::runtime_error("Failed to create socket");
//             }

//             // Connect
//             if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) 
//             {
//              ::close(fd);
//              throw std::runtime_error("Failed to connect");
//             }
//         }

//         ~UnixSocket() 
//         {
//             if (fd != -1) 
//             {
//                 ::close(fd);
//             }
//         }

//         int getFd() const 
//         {
//             return fd;
//         }

//     private:
//         int fd;
//         struct sockaddr_un addr;
// };

// int 
// _xcb_open_unix(char * protocol, const char * file) 
// {
//     try 
//     {
//         UnixSocket socket(file);
//         return socket.getFd();
//     } 
//     catch (const std::runtime_error & e) 
//     {
//         // Handle or log the error as needed
//         return -1;
//     }
// }

// static int 
// _xpp_open(const char * host, char * protocol, const int & display) 
// {
//     int fd;
//     static const char unix_base[] = "/tmp/.X11-unix/X";
//     const char *base = unix_base;
//     size_t filelen;
//     char *file = NULL;
//     int actual_filelen;

//     /* 
//         If protocol or host is "unix", fall through to Unix socket code below 
//      */
//     if (!protocol 
//      || (strcmp("unix", protocol) != 0) && (*host != '\0') && (strcmp("unix", host) != 0)) 
//     {
//         /* display specifies TCP */
//         unsigned short port = X_TCP_PORT + display;
//         return _xcb_open_tcp(host, protocol, port);
//     }

//     filelen = strlen(base) + 1 + sizeof(display) * 3 + 1;
//     file = static_cast<char*>(malloc(sizeof(char) * filelen));
//     if (!file)
//     {
//         return -1;
//     }

//     actual_filelen = snprintf(file, filelen, "%s%d", base, display);
    
//     if (actual_filelen < 0) 
//     {
//         free(file);
//         return -1;
//     }
    
//     /* snprintf may truncate the file */
//     filelen = MIN(actual_filelen, filelen - 1);

//     #ifdef HAVE_ABSTRACT_SOCKETS
//         fd = _xcb_open_abstract(protocol, file, filelen);
//         if (fd >= 0 || (errno != ENOENT && errno != ECONNREFUSED)) {
//             free(file);
//             return fd;
//         }
//     #endif

//     fd = _xcb_open_unix(protocol, file);
//     free(file);

//     if (fd < 0 && !protocol && *host == '\0') 
//     {
//         unsigned short port = X_TCP_PORT + display;
//         fd = _xcb_open_tcp(host, protocol, port);
//     }

//     return fd;
// }

// // bool 
// // _xpp_parse_display(  const std::string & name_input,
// //                     std::unique_ptr<std::string>& host, 
// //                     std::unique_ptr<std::string>& protocol, 
// //                     int& display, int* screen) 
// // {
// //     std::string name = name_input;
// //     if (name.empty()) 
// //     {
// //         const char * env_display = std::getenv("DISPLAY");
// //         if (!env_display) 
// //         {
// //             return false;
// //         }
// //         name = std::string(env_display);
// //     }

// //     auto slash_pos = name.rfind('/');
// //     auto colon_pos = name.rfind(':');

// //     if (slash_pos != std::string::npos) 
// //     {
// //         protocol = std::make_unique<std::string>(name.substr(0, slash_pos));
// //         name.erase(0, slash_pos + 1);
// //     }
// //     else 
// //     {
// //         protocol.reset();
// //     }

// //     if (colon_pos == std::string::npos) 
// //     {
// //         return false;
// //     }

// //     std::string display_str = name.substr(colon_pos + 1);
// //     char* end;
// //     display = std::strtol(display_str.c_str(), &end, 10);
    
// //     if (end == display_str.c_str()) 
// //     {
// //         return false;
// //     }

// //     int screen_temp = 0;

// //     if (*end != '\0') 
// //     {
// //         if (*end != '.') 
// //         {
// //             return false;
// //         }

// //         screen_temp = std::strtol(end + 1, &end, 10);
        
// //         if (end == display_str.c_str() || *end != '\0') 
// //         {
// //             return false;
// //         }
// //     }

// //     host = std::make_unique<std::string>(name.substr(0, colon_pos));
// //     display = std::stoi(display_str);
    
// //     if (screen) 
// //     {
// //         *screen = screen_temp;
// //     }

// //     return true;
// // }

// bool 
// _xpp_parse_display( const char * name_input,
//                     char** host, 
//                     char** protocol, 
//                     int * displayp, int* screen) 
// {
//     int display = 0;
//     std::string name = name_input;
//     if (name.empty()) 
//     {
//         const char * env_display = std::getenv("DISPLAY");
//         if (!env_display) 
//         {
//             return false;
//         }
//         name = std::string(env_display);
//     }

//     auto slash_pos = name.rfind('/');
//     auto colon_pos = name.rfind(':');

//     if (slash_pos != std::string::npos) 
//     {
//         *protocol = strdup(name.substr(0, slash_pos).c_str());
//         name.erase(0, slash_pos + 1);
//     }
//     else 
//     {
//         *protocol = nullptr;
//     }

//     if (colon_pos == std::string::npos) 
//     {
//         return false;
//     }

//     std::string display_str = name.substr(colon_pos + 1);
//     char* end;
//     display = std::strtol(display_str.c_str(), &end, 10);
    
//     if (end == display_str.c_str()) 
//     {
//         return false;
//     }

//     int screen_temp = 0;

//     if (*end != '\0') 
//     {
//         if (*end != '.') 
//         {
//             return false;
//         }

//         screen_temp = std::strtol(end + 1, &end, 10);
        
//         if (end == display_str.c_str() || *end != '\0') 
//         {
//             return false;
//         }
//     }

//     *host = strdup(name.substr(0, colon_pos).c_str());
//     display = std::stoi(display_str);

//     *displayp = display;

//     if (screen) 
//     {
//         *screen = screen_temp;
//     }

//     return true;
// }

// #define XPP_CONN_ERROR 1
// #define XPP_CONN_CLOSED_MEM_INSUFFICIENT 2
// #define XPP_CONN_CLOSED_PARSE_ERR 3
// #define XPP_CONN_CLOSED_INVALID_SCREEN 4

// static const int xpp_con_error = XPP_CONN_ERROR;
// static const int xpp_con_closed_mem_er = XPP_CONN_CLOSED_MEM_INSUFFICIENT;
// static const int xpp_con_closed_parse_er = XPP_CONN_CLOSED_PARSE_ERR;
// static const int xpp_con_closed_screen_er = XPP_CONN_CLOSED_INVALID_SCREEN;

// xpp_connection_t *
// _xpp_conn_ret_error(int err)
// {

//     switch(err)
//     {
//         case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
//         {
//             return (xpp_connection_t *) &xpp_con_closed_mem_er;
//         }
//         case XCB_CONN_CLOSED_PARSE_ERR:
//         {
//             return (xpp_connection_t *) &xpp_con_closed_parse_er;
//         }
//         case XCB_CONN_CLOSED_INVALID_SCREEN:
//         {
//             return (xpp_connection_t *) &xpp_con_closed_screen_er;
//         }
//         case XCB_CONN_ERROR: 
//         {}
//         default:
//         {
//             return (xpp_connection_t *) &xpp_con_error;
//         }
//     }
// }

// int 
// set_fd_flags(const int fd)
// {
//     int flags = fcntl(fd, F_GETFL, 0);
//     if(flags == -1)
//         return 0;
//     flags |= O_NONBLOCK;
//     if(fcntl(fd, F_SETFL, flags) == -1)
//         return 0;
//     if(fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
//         return 0;
//     return 1;
// }

// int 
// write_setup(xpp_connection_t *c, xcb_auth_info_t *auth_info)
// {
//     const char pad[3];
//     xcb_setup_request_t out;
//     struct iovec parts[6];
//     int count = 0;
//     static const uint32_t endian = 0x01020304;
//     int ret;

//     memset(&out, 0, sizeof(out));

//     /* B = 0x42 = MSB first, l = 0x6c = LSB first */
//     if(htonl(endian) == endian)
//         out.byte_order = 0x42;
//     else
//         out.byte_order = 0x6c;
//     out.protocol_major_version = X_PROTOCOL;
//     out.protocol_minor_version = X_PROTOCOL_REVISION;
//     out.authorization_protocol_name_len = 0;
//     out.authorization_protocol_data_len = 0;
//     parts[count].iov_len = sizeof(xcb_setup_request_t);
//     parts[count++].iov_base = &out;
//     parts[count].iov_len = XCB_PAD(sizeof(xcb_setup_request_t));
//     parts[count++].iov_base = (char *) pad;

//     if(auth_info)
//     {
//         parts[count].iov_len = out.authorization_protocol_name_len = auth_info->namelen;
//         parts[count++].iov_base = auth_info->name;
//         parts[count].iov_len = XCB_PAD(out.authorization_protocol_name_len);
//         parts[count++].iov_base = (char *) pad;
//         parts[count].iov_len = out.authorization_protocol_data_len = auth_info->datalen;
//         parts[count++].iov_base = auth_info->data;
//         parts[count].iov_len = XCB_PAD(out.authorization_protocol_data_len);
//         parts[count++].iov_base = (char *) pad;
//     }
//     assert(count <= (int) (sizeof(parts) / sizeof(*parts)));

//     pthread_mutex_lock(&c->iolock);
//     ret = _xpp_out_send(c, parts, count);
//     pthread_mutex_unlock(&c->iolock);
//     return ret;
// }

// xpp_connection_t *
// xpp_connect_to_fd(int fd, xcb_auth_info_t *auth_info)
// {
//     xpp_connection_t* c;

//     #ifndef _WIN32
//         #ifndef USE_POLL
//             if(fd >= FD_SETSIZE) /* would overflow in FD_SET */
//             {
//                 close(fd);
//                 return _xpp_conn_ret_error(XPP_CONN_ERROR);
//             }
//         #endif
//     #endif /* !_WIN32*/

//     c = static_cast<xpp_connection_t *>(calloc(1, sizeof(xpp_connection_t)));
//     if(!c) {
//         close(fd);
//         return _xpp_conn_ret_error(XPP_CONN_CLOSED_MEM_INSUFFICIENT) ;
//     }

//     c->fd = fd;

//     if(!(
//         set_fd_flags(fd) &&
//         pthread_mutex_init(&c->iolock, 0) == 0 &&
//         _xcb_in_init(&c->in) &&
//         _xcb_out_init(&c->out) &&
//         write_setup(c, auth_info) &&
//         read_setup(c) &&
//         _xcb_ext_init(c) &&
//         _xcb_xid_init(c)
//         ))
//     {
//         xcb_disconnect(c);
//         return _xcb_conn_ret_error(XCB_CONN_ERROR);
//     }

//     return c;
// }


// class xpp_get_auth_info 
// {
//     public:
//         xpp_get_auth_info(int fd, xcb_auth_info_t * info, int display)
//         {
//             authnames_str = {AUTH_PROTO_MIT_MAGIC_COOKIE};
//             authnameslen[AUTH_MC1] = static_cast<const int>(sizeof(AUTH_PROTO_MIT_MAGIC_COOKIE) - 1);
//             _xpp_get_auth_info(fd, info, display);
//         }

//         ~xpp_get_auth_info()
//         {
//             for (const auto& name : authnames_cstr) 
//             {
//                 delete[] name;
//             }
//         }

//     private:
//         enum auth_protos 
//         {
//             AUTH_MC1,
//             N_AUTH_PROTOS
//         };
//         static constexpr char AUTH_PROTO_MIT_MAGIC_COOKIE[] = "MIT-MAGIC-COOKIE-1";
//         std::vector<std::string> authnames_str;
//         std::vector<char*> authnames_cstr;
//         int authnameslen[N_AUTH_PROTOS];
        
//         int
//         _xpp_get_auth_info(int fd, xcb_auth_info_t * info, int display)
//         {
//             struct sockaddr *sockname = NULL;
//             int gotsockname = 0;
//             Xauth *authptr = 0;
//             int ret = 1;

//             /* Some systems like hpux or Hurd do not expose peer names
//             * for UNIX Domain Sockets, but this is irrelevant,
//             * since compute_auth() ignores the peer name in this
//             * case anyway.*/
//             if ((sockname = get_peer_sock_name(getpeername, fd)) == NULL)
//             {
//                 if ((sockname = get_peer_sock_name(getsockname, fd)) == NULL)
//                 {
//                     return 0;   /* can only authenticate sockets */
//                 }

//                 if (sockname->sa_family != AF_UNIX)
//                 {
//                     free(sockname);
//                     return 0;   /* except for AF_UNIX, sockets should have peernames */
//                 }
//                 gotsockname = 1;
//             }

//             authptr = get_authptr(sockname, display);
//             if (authptr == 0)
//             {
//                 free(sockname);
//                 return 0;   /* cannot find good auth data */
//             }

//             info->namelen = memdup(&info->name, authptr->name, authptr->name_length);
//             if (!info->namelen)
//                 goto no_auth;   /* out of memory */

//             if (!gotsockname)
//             {
//                 free(sockname);

//                 if ((sockname = get_peer_sock_name(getsockname, fd)) == NULL)
//                 {
//                     free(info->name);
//                     goto no_auth;   /* can only authenticate sockets */
//                 }
//             }

//             ret = compute_auth(info, authptr, sockname);
//             if(!ret)
//             {
//                 free(info->name);
//                 goto no_auth;   /* cannot build auth record */
//             }

//             free(sockname);
//             sockname = NULL;

//             XauDisposeAuth(authptr);
//             return ret;

//             no_auth:
//             free(sockname);

//             info->name = 0;
//             info->namelen = 0;
//             XauDisposeAuth(authptr);
//         }

//         size_t 
//         memdup(char** dst, void* src, size_t len)
//         {
//             if(len)
//             {
//                 *dst = new char[len];
//             }
//             else
//             {
//                 *dst = nullptr;
//             }

//             if(!*dst)
//             {
//                 return 0;
//             }

//             std::memcpy(*dst, src, len);
//             return len;
//         }

//         Xauth *
//         getAuthPtr(struct sockaddr *sockname, int display)
//         {
//             char *addr = nullptr;
//             int addrlen = 0;
//             unsigned short family;
//             char hostnamebuf[256];  // big enough for max hostname
//             char dispbuf[40];  // big enough to hold more than 2^64 base 10
//             int dispbuflen;

//             family = FamilyLocal; // Usually 256 for XCB_FAMILY_LOCAL
//             switch(sockname->sa_family) 
//             {
//                 case AF_INET6: 
//                 {
//                     struct sockaddr_in6 *sockaddr_ipv6 = reinterpret_cast<struct sockaddr_in6 *>(sockname);
//                     addr = reinterpret_cast<char *>(&sockaddr_ipv6->sin6_addr);
//                     addrlen = sizeof(sockaddr_ipv6->sin6_addr);
//                     if (!IN6_IS_ADDR_V4MAPPED(&sockaddr_ipv6->sin6_addr) 
//                     && !IN6_IS_ADDR_LOOPBACK(&sockaddr_ipv6->sin6_addr)) 
//                     {
//                         family = XCB_FAMILY_INTERNET_6;
//                     }
//                     break;
//                 }
//                 case AF_INET: 
//                 {
//                     struct sockaddr_in *sockaddr_ipv4 = reinterpret_cast<struct sockaddr_in *>(sockname);
//                     addr = reinterpret_cast<char *>(&sockaddr_ipv4->sin_addr);
//                     addrlen = sizeof(sockaddr_ipv4->sin_addr);
//                     if (sockaddr_ipv4->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) 
//                     {
//                         family = XCB_FAMILY_INTERNET;
//                     }
//                     break;
//                 }
//                 case AF_UNIX: 
//                 {
//                     // Nothing to do for Unix domain sockets
//                     break;
//                 }
//                 default:
//                 {
//                     return nullptr; // Cannot authenticate this family
//                 }
//             }

//             dispbuflen = snprintf(dispbuf, sizeof(dispbuf), "%d", display);
            
//             if (dispbuflen < 0)
//             {
//                 return nullptr;
//             }
            
//             dispbuflen = std::min(dispbuflen, static_cast<int>(sizeof(dispbuf) - 1));

//             if (family == FamilyLocal) 
//             {
//                 if (gethostname(hostnamebuf, sizeof(hostnamebuf)) == -1)
//                 {
//                     return nullptr; // Do not know own hostname
//                 }
//                 addr = hostnamebuf;
//                 addrlen = strlen(addr);
//             }

//             for (const auto& name : authnames_str) 
//             {
//                 char* cstr = new char[name.size() + 1]; // +1 for null terminator
//                 std::strcpy(cstr, name.c_str());
//                 authnames_cstr.push_back(cstr);
//             }

//             return XauGetBestAuthByAddr(family,
//                                         static_cast<unsigned short>(addrlen), 
//                                         addr,
//                                         static_cast<unsigned short>(dispbuflen), 
//                                         dispbuf,
//                                         N_AUTH_PROTOS, 
//                                         authnames_cstr.data(), 
//                                         authnameslen);
//         }

//         /* `sockaddr_un.sun_path' typical size usually ranges between 92 and 108 */
//         #define INITIAL_SOCKNAME_SLACK 108

//         sockaddr * 
//         get_peer_sock_name(int (*socket_func)(int, struct sockaddr *, socklen_t *), int fd)
//         {
//             socklen_t socknamelen = sizeof(sockaddr) + INITIAL_SOCKNAME_SLACK;
//             socklen_t actual_socknamelen = socknamelen;
//             sockaddr * sockname(static_cast<sockaddr *>(std::malloc(socknamelen)));

//             if (!sockname)
//             {
//                 return nullptr;
//             }

//             /* Both getpeername() and getsockname() truncates sockname if
//             there is not enough space and set the required length in
//             actual_socknamelen */
//             if (socket_func(fd, sockname, &actual_socknamelen) == -1)
//             {
//                 return nullptr;
//             }

//             if (actual_socknamelen > socknamelen)
//             {
//                 socknamelen = actual_socknamelen;
//                 struct sockaddr* new_sockname = static_cast<struct sockaddr*>(std::realloc(sockname, socknamelen));

//                 if (!new_sockname)
//                 {
//                     return nullptr;
//                 }

//                 sockname = (new_sockname);

//                 if (socket_func(fd, sockname, &actual_socknamelen) == -1 
//                  || actual_socknamelen > socknamelen)
//                 {
//                     return nullptr;
//                 }
//             }

//             return sockname;
//         }

//         int 
//         authname_match(enum auth_protos kind, char *name, size_t namelen)
//         {
//             for (const auto& name : authnames_str) 
//             {
//                 char* cstr = new char[name.size() + 1]; // +1 for null terminator
//                 std::strcpy(cstr, name.c_str());
//                 authnames_cstr.push_back(cstr);
//             }

//             if(authnameslen[kind] != namelen)
//             {
//                 return 0;
//             }

//             if(memcmp(authnames_cstr[kind], name, namelen))
//             {
//                 return 0;
//             }

//             return 1;
//         }

//         int 
//         compute_auth(xcb_auth_info_t *info, Xauth *authptr, struct sockaddr *sockname)
//         {
//             if (authname_match(AUTH_MC1, authptr->name, authptr->name_length)) 
//             {
//                 info->datalen = memdup(&info->data, authptr->data, authptr->data_length);
//                 if(!info->datalen)
//                 {
//                     return 0;
//                 }
//                 return 1;
//             }
            
//             return 0;   /* Unknown authorization type */
//         }
// };

// namespace xpp 
// {

//     xpp_connection_t *
//     xpp_connect(const char *displayname, int *screenp)
//     {
//         return xpp_connect_to_display_with_auth_info(displayname, NULL, screenp);
//     }

//     xpp_connection_t *
//     xpp_connect_to_display_with_auth_info(const char *displayname, xpp_auth_info_t *auth, int *screenp)
//     {
//         int fd, display = 0;
//         char *host = NULL;
//         char *protocol = NULL;
//         xpp_auth_info_t ourauth;
//         xpp_connection_t *c;

//         int parsed = _xpp_parse_display(displayname, &host, &protocol, &display, screenp);
        
//         if(!parsed) 
//         {
//             c = _xpp_conn_ret_error(XPP_CONN_CLOSED_PARSE_ERR);
//             goto out;
//         } 
//         else 
//         {
//             fd = _xpp_open(host, protocol, display);
//         }

//         if(fd == -1) 
//         {
//             c = _xpp_conn_ret_error(XPP_CONN_ERROR);
//             #ifdef _WIN32
//                     WSACleanup();
//             #endif
//             goto out;
//         }

//         if(auth) 
//         {
//             c = xcb_connect_to_fd(fd, auth);
//             goto out;
//         }

//         if(_xcb_get_auth_info(fd, &ourauth, display))
//         {
//             c = xcb_connect_to_fd(fd, &ourauth);
//             free(ourauth.name);
//             free(ourauth.data);
//         }
//         else
//             c = xcb_connect_to_fd(fd, 0);

//         if(c->has_error)
//             goto out;

//         /* Make sure requested screen number is in bounds for this server */
//         if((screenp != NULL) && (*screenp >= (int) c->setup->roots_len)) {
//             xcb_disconnect(c);
//             c = _xcb_conn_ret_error(XCB_CONN_CLOSED_INVALID_SCREEN);
//             goto out;
//         }

//     out:
//         free(host);
//         free(protocol);
//         return c;
//     }
// }