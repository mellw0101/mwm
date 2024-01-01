#ifdef  main_cpp

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
    #include <unordered_map>
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
    #include <future>
    #include <iostream>
    #include <algorithm>
    #include <memory>
    #include <png.h>
    #include <unordered_map>
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


    #include "structs.hpp"
    #include "Log.hpp"
    #include "defenitions.hpp"
    #include "tools.hpp"

#endif

#ifdef  STRUCTS_HPP

    #include <cstdint>
    #include <vector>
    #include <xcb/xproto.h>
    #include <memory>

#endif

#ifdef XCB_HPP

    #include <xcb/xcb.h>

    #include "Log.hpp"

#endif

#ifdef MXB_HPP

    #include <xcb/xcb.h>
    #include "structs.hpp"
    #include <cstdint>
    #include <cstdlib>

    #include "Log.hpp"
    #include "defenitions.hpp"

#endif