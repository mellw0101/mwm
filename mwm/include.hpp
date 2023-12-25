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

    #include "structs.hpp"
    #include "Log.hpp"
    #include "xcb.hpp"
    #include "defenitions.hpp"
    #include "Screen.hpp"
    #include "tools.hpp"

#endif

#ifdef  STRUCTS_HPP

    #include <cstdint>
    #include <vector>
    #include <xcb/xproto.h>

#endif

#ifdef XCB_HPP

    #include <xcb/xcb.h>

    #include "Log.hpp"

#endif