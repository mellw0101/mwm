#include <X11/Xauth.h>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <xcb/xproto.h>

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

typedef struct xpp_auth_info_t 
{
    int   namelen;  /**< Length of the string name (as returned by strlen). */
    char *name;     /**< String containing the authentication protocol name, such as "MIT-MAGIC-COOKIE-1" or "XDM-AUTHORIZATION-1". */
    int   datalen;  /**< Length of the data member. */
    char *data;   /**< Data interpreted in a protocol-specific manner. */
} xpp_auth_info_t;

class xpp_get_auth_info 
{
    public:
        xpp_get_auth_info(int fd, xpp_auth_info_t * info, int display)
        {
            authnames_str = {AUTH_PROTO_MIT_MAGIC_COOKIE};
            authnameslen[AUTH_MC1] = static_cast<const int>(sizeof(AUTH_PROTO_MIT_MAGIC_COOKIE) - 1);
            _xpp_get_auth_info(fd, info, display);
        }

        ~xpp_get_auth_info()
        {
            for (const auto& name : authnames_cstr) 
            {
                delete[] name;
            }
        }

    private:
        enum auth_protos 
        {
            AUTH_MC1,
            N_AUTH_PROTOS
        };
        static constexpr char AUTH_PROTO_MIT_MAGIC_COOKIE[] = "MIT-MAGIC-COOKIE-1";
        std::vector<std::string> authnames_str;
        std::vector<char*> authnames_cstr;
        int authnameslen[N_AUTH_PROTOS];
        
        int
        _xpp_get_auth_info(int fd, xpp_auth_info_t * info, int display)
        {
            struct sockaddr *sockname = NULL;
            int gotsockname = 0;
            Xauth *authptr = 0;
            int ret = 1;

            /* Some systems like hpux or Hurd do not expose peer names
            * for UNIX Domain Sockets, but this is irrelevant,
            * since compute_auth() ignores the peer name in this
            * case anyway.*/
            if ((sockname = get_peer_sock_name(getpeername, fd)) == NULL)
            {
                if ((sockname = get_peer_sock_name(getsockname, fd)) == NULL)
                {
                    return 0;   /* can only authenticate sockets */
                }

                if (sockname->sa_family != AF_UNIX)
                {
                    free(sockname);
                    return 0;   /* except for AF_UNIX, sockets should have peernames */
                }
                gotsockname = 1;
            }

            authptr = get_authptr(sockname, display);
            if (authptr == 0)
            {
                free(sockname);
                return 0;   /* cannot find good auth data */
            }

            info->namelen = memdup(&info->name, authptr->name, authptr->name_length);
            if (!info->namelen)
                goto no_auth;   /* out of memory */

            if (!gotsockname)
            {
                free(sockname);

                if ((sockname = get_peer_sock_name(getsockname, fd)) == NULL)
                {
                    free(info->name);
                    goto no_auth;   /* can only authenticate sockets */
                }
            }

            ret = compute_auth(info, authptr, sockname);
            if(!ret)
            {
                free(info->name);
                goto no_auth;   /* cannot build auth record */
            }

            free(sockname);
            sockname = NULL;

            XauDisposeAuth(authptr);
            return ret;

            no_auth:
            free(sockname);

            info->name = 0;
            info->namelen = 0;
            XauDisposeAuth(authptr);
        }

        size_t 
        memdup(char** dst, void* src, size_t len)
        {
            if(len)
            {
                *dst = new char[len];
            }
            else
            {
                *dst = nullptr;
            }

            if(!*dst)
            {
                return 0;
            }

            memcpy(*dst, src, len);
            return len;
        }

        Xauth *
        get_authptr(struct sockaddr *sockname, int display)
        {
            char *addr = nullptr;
            int addrlen = 0;
            unsigned short family;
            char hostnamebuf[256];  // big enough for max hostname
            char dispbuf[40];  // big enough to hold more than 2^64 base 10
            int dispbuflen;

            family = FamilyLocal; // Usually 256 for XCB_FAMILY_LOCAL
            switch(sockname->sa_family) 
            {
                case AF_INET6: 
                {
                    struct sockaddr_in6 *sockaddr_ipv6 = reinterpret_cast<struct sockaddr_in6 *>(sockname);
                    addr = reinterpret_cast<char *>(&sockaddr_ipv6->sin6_addr);
                    addrlen = sizeof(sockaddr_ipv6->sin6_addr);
                    if (!IN6_IS_ADDR_V4MAPPED(&sockaddr_ipv6->sin6_addr) 
                    && !IN6_IS_ADDR_LOOPBACK(&sockaddr_ipv6->sin6_addr)) 
                    {
                        family = XCB_FAMILY_INTERNET_6;
                    }
                    break;
                }
                case AF_INET: 
                {
                    struct sockaddr_in *sockaddr_ipv4 = reinterpret_cast<struct sockaddr_in *>(sockname);
                    addr = reinterpret_cast<char *>(&sockaddr_ipv4->sin_addr);
                    addrlen = sizeof(sockaddr_ipv4->sin_addr);
                    if (sockaddr_ipv4->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) 
                    {
                        family = XCB_FAMILY_INTERNET;
                    }
                    break;
                }
                case AF_UNIX: 
                {
                    // Nothing to do for Unix domain sockets
                    break;
                }
                default:
                {
                    return nullptr; // Cannot authenticate this family
                }
            }

            dispbuflen = snprintf(dispbuf, sizeof(dispbuf), "%d", display);
            
            if (dispbuflen < 0)
            {
                return nullptr;
            }
            
            dispbuflen = std::min(dispbuflen, static_cast<int>(sizeof(dispbuf) - 1));

            if (family == FamilyLocal) 
            {
                if (gethostname(hostnamebuf, sizeof(hostnamebuf)) == -1)
                {
                    return nullptr; // Do not know own hostname
                }
                addr = hostnamebuf;
                addrlen = strlen(addr);
            }

            for (const auto& name : authnames_str) 
            {
                char* cstr = new char[name.size() + 1]; // +1 for null terminator
                strcpy(cstr, name.c_str());
                authnames_cstr.push_back(cstr);
            }

            return XauGetBestAuthByAddr(family,
                                        static_cast<unsigned short>(addrlen), 
                                        addr,
                                        static_cast<unsigned short>(dispbuflen), 
                                        dispbuf,
                                        N_AUTH_PROTOS, 
                                        authnames_cstr.data(), 
                                        authnameslen);
        }

        /* `sockaddr_un.sun_path' typical size usually ranges between 92 and 108 */
        #define INITIAL_SOCKNAME_SLACK 108

        sockaddr * 
        get_peer_sock_name(int (*socket_func)(int, struct sockaddr *, socklen_t *), int fd)
        {
            socklen_t socknamelen = sizeof(sockaddr) + INITIAL_SOCKNAME_SLACK;
            socklen_t actual_socknamelen = socknamelen;
            sockaddr * sockname(static_cast<sockaddr *>(std::malloc(socknamelen)));

            if (!sockname)
            {
                return nullptr;
            }

            /* Both getpeername() and getsockname() truncates sockname if
            there is not enough space and set the required length in
            actual_socknamelen */
            if (socket_func(fd, sockname, &actual_socknamelen) == -1)
            {
                return nullptr;
            }

            if (actual_socknamelen > socknamelen)
            {
                socknamelen = actual_socknamelen;
                struct sockaddr* new_sockname = static_cast<struct sockaddr*>(std::realloc(sockname, socknamelen));

                if (!new_sockname)
                {
                    return nullptr;
                }

                sockname = (new_sockname);

                if (socket_func(fd, sockname, &actual_socknamelen) == -1 
                 || actual_socknamelen > socknamelen)
                {
                    return nullptr;
                }
            }

            return sockname;
        }

        int 
        authname_match(enum auth_protos kind, char *name, size_t namelen)
        {
            for (const auto& name : authnames_str) 
            {
                char* cstr = new char[name.size() + 1]; // +1 for null terminator
                strcpy(cstr, name.c_str());
                authnames_cstr.push_back(cstr);
            }

            if(authnameslen[kind] != namelen)
            {
                return 0;
            }

            if(memcmp(authnames_cstr[kind], name, namelen))
            {
                return 0;
            }

            return 1;
        }

        int 
        compute_auth(xpp_auth_info_t *info, Xauth *authptr, struct sockaddr *sockname)
        {
            if (authname_match(AUTH_MC1, authptr->name, authptr->name_length)) 
            {
                info->datalen = memdup(&info->data, authptr->data, authptr->data_length);
                if(!info->datalen)
                {
                    return 0;
                }
                return 1;
            }
            
            return 0;   /* Unknown authorization type  */
        }
};

class UnixSocket {
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