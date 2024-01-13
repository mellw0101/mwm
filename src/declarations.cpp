
#include "structs.hpp"
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <cstdlib>
#include <initializer_list>
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
#include <png.h>
#include <xcb/xcb_image.h>
#include <Imlib2.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <X11/Xauth.h>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "Log.hpp"
#include "defenitions.hpp"

Logger log;

static xcb_connection_t * conn;
static xcb_ewmh_connection_t * ewmh; 
static const xcb_setup_t * setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t * screen;
