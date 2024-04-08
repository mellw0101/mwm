#include <array>
#include <cstdlib>
#include <exception>
#include <features.h>
#include <iterator>
// #include <numeric>
// #include <optional>
// #include <limits>
// #include <new>
#include <queue>
#include <ratio>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>
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
#include <spawn.h>
#include <sys/stat.h>
// #include <any>
#include "data.hpp"
#include "tools.hpp"

#include "Log.hpp"
// #include "data.hpp"
#include "xcb.hpp"
#include "xcb.hpp"
Logger logger;
#include "structs.hpp"
#include "defenitions.hpp"
#include "tools.hpp"
#include "thread.hpp"
// #include "Event.hpp"

static xcb_connection_t * conn;
static xcb_ewmh_connection_t * ewmh;
static const xcb_setup_t * setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t *screen;
ThreadPool tPool{6};

// #include "xcb.hpp"

#define DEFAULT_FONT "7x14"
#define DEFAULT_FONT_WIDTH 7
#define DEFAULT_FONT_HEIGHT 14

using namespace std;

/* vector<pair<uint32_t, function<void()>>> expose_tasks;
vector<client *> cli_tasks; */
UInt32UnorderedMap<client *> cw_map;

#define NET_DEBUG false

#define NEW_CLASS(__class_inst, __class_name) \
    __class_inst = new __class_name;                                    \
    if (__class_inst == nullptr)                                        \
    {                                                                   \
        loutE << "Failed to allocate memory to make new class" << '\n'; \
    }                                                                   \
    else

#define EV_CALL(__type) \
    __type, [&](Ev ev) -> void

#define CENTER_TEXT(__width, __str_len) \
    ((__width / 2) - ((__str_len * DEFAULT_FONT_WIDTH) / 2))

#define CENTER_TEXT_Y(__height) \
    (__height - (20 - (DEFAULT_FONT_HEIGHT + 1))) - ((__height - 20) / 2)

#define TEXT_LEN(__str) \
    (size(__str) - 1)

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

#define BUTTON_EVENT_MASK \
    XCB_EVENT_MASK_ENTER_WINDOW  | \
    XCB_EVENT_MASK_LEAVE_WINDOW | \
    XCB_EVENT_MASK_BUTTON_PRESS | \
    XCB_EVENT_MASK_EXPOSURE

#define ROOT_EVENT_MASK \
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | \
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   | \
    XCB_EVENT_MASK_STRUCTURE_NOTIFY      | \
    XCB_EVENT_MASK_BUTTON_PRESS          | \
    XCB_EVENT_MASK_FOCUS_CHANGE 

#define GC_MASK \
    XCB_GC_FOREGROUND         |\
    XCB_GC_BACKGROUND         |\
    XCB_GC_GRAPHICS_EXPOSURES

/**
 *
 * @brief General purpose definition to reinterpret cast.
 *
 */
#define RE_CAST(__type, __from) \
    reinterpret_cast<__type>(__from)

/**
 *
 * @brief Specialized definition to reinterpret a const xcb_generic_event_t *
 *        into a const __type * (the event that 'e' should represent) with the name 'e' for use in event handling.
 *
 */
#define RE_CAST_EV(__type) \
    auto e = RE_CAST(const __type *, ev)

/**
 *
 * @brief Defenition to set an event callback to the global event handler
 *        from within the class that holds the internal client
 *        as this way it can be properly cleaned when killed
 *
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

#define RETURN_IF(__statement)  \
    if (__statement)            \
    {                           \
        return;                 \
    }

#define CONTINUE_IF(__statement) \
    if (__statement)             \
    {                            \
        continue;                \
    }

#define GET_CLIENT_FROM_WINDOW(__window) \
    client *c = wm->client_from_any_window(&__window); \
    RETURN_IF(c == nullptr)

static string user;
#define USER \
    user

#define USER_PATH_PREFIX(__address) \
    "/home/" + user + __address

#define USER_PATH_PREFIX_C_STR(__address) \
    string("/home/" + user + __address).c_str()

#define SCREEN_CENTER_X(__window_width)  ((screen->width_in_pixels / 2) - (__window_width / 2))
#define SCREEN_CENT_X()                  (screen->width_in_pixels / 2)
#define SCREEN_CENTER_Y(__window_height) ((screen->height_in_pixels / 2) - (__window_height / 2))
#define SCREEN_BOTTOM_Y(__window_height) (screen->height_in_pixels - __window_height)

template<typename Type>
constexpr Type make_constexpr(Type value) { return value; }
#define CONSTEXPR(__name, __value) \
    constexpr auto __name = make_constexpr(__value)

#define CONSTEXPR_TYPE(__type, __name, __value) \
    constexpr __type __name = make_constexpr<__type>((__type)__value)

#define STATIC_CONSTEXPR(__name, __value) \
    static CONSTEXPR(__name, __value)

#define STATIC_CONSTEXPR_TYPE(__type, __name, __value) \
    static constexpr __type __name = make_constexpr<__type>((__type)__value)

#define TEMP(__T1) \
    template<__T1>

#define TEMP_CB \
    TEMP(typename Callback)

#define CB \
    Callback &&callback

namespace { // Tools

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

    /**
     * @brief Function to return username and store it globally
     *        first tries to get username from env var 'USER'.
     *        If 'USER' is not set, then as a fallback, try to get user name
     *        from env var 'LOGNAME'.
     *
     * NOTE: This function should be used once to retrieve the username
     *       and then cached for later use to avoid making unnecessary calls.
     * 
     * NOTE: This function will return "unknown" upon error.
     *
     * @return The username as a const reference to std::string.
     */
    const string& get_user_name()
    {
        static const string username = []() -> string
        {
            const char* name = getenv("USER");
            if (!name)
            {
                name = getenv("LOGNAME");
            }
        
            return name ? name : "unknown";
        }();

        return username;
    }

    namespace {
        /**
        *
        * @brief Flushes any X server requests in que and checks for errors 
        *
        */
        void flush_x(const char *__calling_function, uint32_t __window = 0)
        {
            int err = xcb_flush(conn);
            if (err <= 0)
            {
                if (__window != 0)
                {
                    loutE << WINDOW_ID_BY_INPUT(__window) << "Error flushing the x server: " << loutCEcode(err) << '\n';
                }

                loutE << "Error flushing the x server: " << loutCEcode(err) << '\n';
            }
        }
        #define FLUSH_X()    flush_x(__func__)
        #define FLUSH_XWin() flush_x(__func__, _window)
        #define FlushX_Win( __window ) do { flush_x( __func__, __window ); } while(0)

        void check_xcb_void_cookie(xcb_void_cookie_t cookie, const char *__calling_function)
        {
            xcb_generic_error_t *error = xcb_request_check(conn, cookie);
            if (error)
            {
                loutEerror_code(__calling_function, error->error_code) << '\n';
                free(error); // Remember to free the error
            }
        }
        #define VOID_COOKIE xcb_void_cookie_t void_cookie
        #define VOID_cookie void_cookie
        #define CHECK_VOID_COOKIE() check_xcb_void_cookie(void_cookie, __func__)

        class __window_geo__ {
            public:
                // Use std::unique_ptr to manage the xcb_get_geometry_reply_t pointer
                std::unique_ptr<xcb_get_geometry_reply_t, decltype(&free)> reply{nullptr, free};

                // Constructor that attempts to retrieve the window geometry
                __window_geo__(uint32_t __window)
                {
                    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, __window);
                    xcb_generic_error_t* error = nullptr;
                    reply.reset(xcb_get_geometry_reply(conn, cookie, &error));
                    
                    // Handle potential errors
                    if (error)
                    {
                        loutE << "Failed to get window geometry, XCB error code:" << error->error_code << loutEND;
                        free(error);  // Remember to free the error
                    }

                    if (!reply)
                    {
                        loutE << "Failed to get window geometry, reply is null." << loutEND;
                    }
                }

                // Deleted copy constructor and copy assignment operator to prevent copying
                __window_geo__(const __window_geo__&) = delete;
                __window_geo__& operator=(const __window_geo__&) = delete;

                // Default move constructor and move assignment operator
                __window_geo__(__window_geo__&& other) noexcept = default;
                __window_geo__& operator=(__window_geo__&& other) noexcept = default;

                // Public method to access the reply
                xcb_get_geometry_reply_t* get() const
                {
                    return reply.get();
                }

                operator xcb_get_geometry_reply_t *() const
                {
                    return reply.get();
                }
        };

    }/* xcb error checking */

    void pop_last_ss(stringstream & __ss)
    {
        if (__ss.str().length() > 0)
        {
            string temp = __ss.str();
            temp.pop_back();
            __ss.str("");
            __ss << temp;
        }
    }

    template<typename Type>
    bool remove_element_from_vec(vector<Type>& vec, size_t index)
    {
        if (index < vec.size())
        {
            vec.erase(vec.begin() + index);
            return true;
        }
        else
        {
            loutE << "index out of bounds" << '\n';
            return false;
        }
    }
}
namespace {
    enum {
        KILL = 1,
    };

    template<typename T1, typename T2>
    using umap = unordered_map<T1, T2>;
    
    template<typename T1, typename T2 = int, typename T3 = void>
    struct uumap {
        using uumap_type = umap<T1, umap<T2, function<void(T3)>>>;
    };

    template<typename T1, typename T2>
    struct uumap<T1, T2, void> {
        using uumap_type = umap<T1, umap<T2, function<void()>>>;
    };

    template<typename T1>
    struct uumap<T1, int, void> {
        using uumap_type = umap<T1, umap<int, function<void()>>>;
    };

    template<typename T1, typename T2 = int, typename T3 = void>
    using uumap_t = typename uumap<T1, T2, T3>::uumap_type;


    template<typename T1, typename T2 = int, typename T3 = void>
    class __uumap__ {
        public:
            uumap_t<T1, T2, T3> _data;

            template<typename Callback>
            void conect(T1 __window, T2 __signal_id, Callback &&callback)
            {
                _data[__window][__signal_id] = std::forward<Callback>(callback);
            }

            void emit(uint32_t __window, EV __signal_id)
            {
                auto it = _data[__window].find(__signal_id);
                if (it == _data[__window].end()) return;

                it->second();
            }

            void remove(T1 __value)
            {
                auto it = _data.find(__value);
                if (it == _data.end()) return;

                _data.erase(it);
            }
    };

    class __window_signals__ {
        public:
            umap<uint32_t, umap<int, function<void(uint32_t)>>> _data;

            template<typename Callback>
            void conect(uint32_t __w, uint8_t __sig, Callback &&__cb) {
                _data[__w][__sig] = std::forward<Callback>(__cb);

            }

            void emit(uint32_t __w, uint8_t __sig) {
                auto it = _data[__w].find(__sig);
                if (it != _data[__w].end()) {
                    it->second(__w);
                    FLUSH_X();

                }

            }

            void emit(uint32_t __w, uint8_t __sig, uint32_t __w2) {
                auto it = _data[__w].find(__sig);
                if (it != _data[__w].end()) {
                    it->second(__w2);
                    FLUSH_X();

                }

            }

            void remove(uint32_t __w) {
                auto it = _data.find(__w);
                if (it == _data.end()) return;
                _data.erase(it);

            }
    };

    /*
     *
     * Primary template with two parameters, Type for the map's key and ArgType for the function's argument.
     *
     */
    template<typename Type, typename ArgType = void>
    struct umap_w_id {
        using type = unordered_map<Type, vector<pair<int, function<void(ArgType)>>>>;
    };

    /*
     *
     * Specialization for when ArgType is void, meaning the function takes no arguments.
     *
     */
    template<typename Type>
    struct umap_w_id<Type, void> {
        using type = unordered_map<Type, vector<pair<int, function<void()>>>>;
    };

    /*
     *
     * Helper type alias to simplify usage
     *
     */
    template<typename Type, typename ArgType = void>
    using umap_w_id_t = typename umap_w_id<Type, ArgType>::type;

    /*
     *
     * Forward declaration of the class template to allow its use in the specialization below
     *
     */
    template<typename Type, typename ArgType = void>
    class UMapWithID;

    // Specialization for when ArgType is void, meaning the function takes no arguments.
    template<typename Type>
    class UMapWithID<Type, void> {
        public:
        /* Vatiables */
            using SignalMap = unordered_map<Type, vector<pair<int, function<void()>>>>;
            SignalMap signalMap;

        /* Methods */
            // Connect a signal with no arguments
            void connect(Type key, int signalID, function<void()> callback)
            {
                signalMap[key].emplace_back(signalID, std::move(callback));
            }

            // Emit a signal with no arguments
            void emit(Type key, int signalID)
            {
                auto it = signalMap.find(key);
                if (it != signalMap.end())
                {
                    for (auto& pair : it->second)
                    {
                        if (pair.first == signalID)
                        {
                            pair.second(); // Invoke the callback
                        }
                    }
                }
            }
    };

    // Primary template for cases where ArgType is not void
    template<typename Type, typename ArgType>
    class UMapWithID {
        public:
        /* Variables */
            using SignalMap = unordered_map<Type, vector<pair<int, function<void(ArgType)>>>>;
            SignalMap signalMap;

        /* Methods */
            // Connect a signal with arguments
            void connect(Type key, int signalID, function<void(ArgType)> callback)
            {
                signalMap[key].emplace_back(signalID, std::move(callback));
            }

            // Emit a signal with arguments
            void emit(Type key, int signalID, ArgType arg)
            {
                auto it = signalMap.find(key);
                if (it != signalMap.end())
                {
                    for (auto& pair : it->second)
                    {
                        if (pair.first == signalID)
                        {
                            pair.second(arg); // Invoke the callback with argument
                        }
                    }
                }
            }
    };

    #define CLI_SIG    [](client *c) -> void

    template<typename ArgType>
    class UMapWithID<client *, ArgType> {
        public:
        /* Variables */
            using SignalMap = unordered_map<client *, vector<pair<int, function<void(ArgType)>>>>;
            SignalMap signalMap;

        /* Methods */
            template<typename Callback>
            void connect(client *key, int signalID, Callback&& callback)
            {
                signalMap[key].emplace_back(signalID, function<void(ArgType)>(std::forward<Callback>(callback)));
            }

            // Emit a signal with arguments
            void emit(client *key, int signalID, ArgType arg)
            {
                auto it = signalMap.find(key);
                if (it != signalMap.end())
                {
                    for (auto& pair : it->second)
                    {
                        if (pair.first == signalID)
                        {
                            pair.second(arg); // Invoke the callback with argument
                        }
                    }
                }
            }
    };

    template<>
    class UMapWithID<client *, void> {
        public:
        /* Vatiables */
            using SignalMap = unordered_map<client *, vector<pair<int, function<void()>>>>;
            SignalMap signalMap;

        /* Methods */
            // Connect a signal with no arguments
            void connect(client *key, int signalID, function<void()> callback)
            {
                signalMap[key].emplace_back(signalID, std::move(callback));
            }

            // Emit a signal with no arguments
            void emit(client *key, int signalID)
            {
                auto it = signalMap.find(key);
                if (it != signalMap.end())
                {
                    for (auto& pair : it->second)
                    {
                        if (pair.first == signalID)
                        {
                            pair.second(); // Invoke the callback
                        }
                    }
                }
            }
    };

    template<>
    class UMapWithID<client *, client *> {
        public:
        /* Variables */
            using SignalMap = unordered_map<client *, vector<pair<int, function<void(client *)>>>>;
            SignalMap signalMap;

        /* Methods */
            template<typename Callback>
            void connect(client *key, int signalID, Callback&& callback)
            {
                signalMap[key].emplace_back(signalID, function<void(client *)>(std::forward<Callback>(callback)));
            }

            // Emit a signal with arguments
            void emit(client *key, int signalID)
            {
                auto it = signalMap.find(key);
                if (it != signalMap.end())
                {
                    for (auto& pair : it->second)
                    {
                        if (pair.first == signalID)
                        {
                            pair.second(key); // Invoke the callback with argument
                        }
                    }
                }
            }

            void remove(client *key)
            {
                auto it = signalMap.find(key);
                if (it == signalMap.end()) return;
                signalMap.erase(it);
            }
    };

    template<>
    class UMapWithID<uint32_t> {
        public:
        /* Vatiables */
            using SignalMap = unordered_map<uint32_t, vector<pair<int, function<void()>>>>;
            SignalMap signalMap;

        /* Methods */
            template<enum_signal_t __enum_signal, typename Callback>
            void connect(Callback &&callback, uint32_t key) {
                if constexpr (__enum_signal == SET_EV_CALLBACK__RESIZE_NO_BORDER) {
                    signalMap[key].emplace_back(SET_EV_CALLBACK__RESIZE_NO_BORDER, std::forward<Callback>(callback));

                } else if constexpr (__enum_signal == HIDE_DOCK) {
                    signalMap[key].emplace_back(HIDE_DOCK, std::forward<Callback>(callback));

                }

            }

            template<enum_signal_t __enum_signal>
            void emit(uint32_t key) {
                auto it = signalMap.find(key);
                if (it != signalMap.end()) {
                    for (auto &pair : it->second) {
                        CONSTEXPR_TYPE(int , signal_id, __enum_signal); 
                        if (pair.first == signal_id) {
                            pair.second();
                            
                        }

                    }

                }

            }

            template<int __event>
            void remove(uint32_t __key) {
                auto key = signalMap.find(__key);

                if (key != signalMap.end()) {
                    if constexpr (__event == 0) {
                        signalMap.erase(key);

                    } else {
                        for (int i = 0; i < signalMap[__event].size(); ++i) {
                            if constexpr (signalMap[__event][i].first == __event) {
                                remove_element_from_vec(signalMap[__event], i);
                                
                            }

                        }

                    }

                }

            }

    };

    template<> /* for windows to pass the window as well */
    class UMapWithID<uint32_t, window> {
        public:
        /* Vatiables */
            using SignalMap = unordered_map<uint32_t, vector<pair<int, function<void(window &)>>>>;
            SignalMap signalMap;

        /* Methods */
            template<EV __signal_id, typename Callback>
            void connect(uint32_t __key, Callback &&__callback)
            {
                if constexpr (__signal_id == L_MOUSE_BUTTON_EVENT)
                {
                    signalMap[__key].emplace_back(L_MOUSE_BUTTON_EVENT, std::forward<Callback>(__callback));
                }

                if constexpr (__signal_id == EXPOSE)
                {
                    signalMap[__key].emplace_back(EXPOSE, std::forward<Callback>(__callback));
                }
            }

            template<EV __signal_id>
            void emit(uint32_t key, window &window)
            {
                int signal_id;
                if constexpr (__signal_id == EXPOSE)
                {
                    signal_id = EXPOSE;
                }

                auto it = signalMap.find(key);
                if (it != signalMap.end())
                {
                    for (auto &pair : it->second)
                    {
                        if (pair.first == signal_id)
                        {
                            pair.second(window);
                        }
                    }
                }
            }

            void remove_window(uint32_t __key)
            {
                auto it = signalMap.find(__key);
                if (it == signalMap.end()) return;
                signalMap.erase(it);
            }
    };

    template<>
    class UMapWithID<window &, void> {
        public:
            unordered_map<window *, vector<std::pair<int, function<void()>>>> windowSignalMap;

            void connect(window &win, int signalID, function<void()> callback)
            {
                windowSignalMap[&win].emplace_back(signalID, std::move(callback));
                loutI << "map size:" << windowSignalMap.size() << loutEND;
            }

            void emit(window &win, int signalID)
            {
                auto it = windowSignalMap.find(&win);
                if (it != windowSignalMap.end())
                {
                    for (auto& pair : it->second)
                    {
                        if (pair.first == signalID)
                        {
                            pair.second(); // Invoke the callback
                        }
                    }
                }
            }

            void remove_window(window &win)
            {
                auto it = windowSignalMap.find(&win);
                if (it != windowSignalMap.end())
                {
                    windowSignalMap.erase(it);
                    loutI << "Deleted window, Current vec size:" << size() << loutEND;
                }
            }

            void remove_window_signal(window &win, int __signal_id)
            {
                auto it = windowSignalMap.find(&win);
                if (it != windowSignalMap.end())
                {
                    for (int i = 0; i < it->second.size(); ++i)
                    {
                        if (it->second[i].first == __signal_id)
                        {
                            if (remove_element_from_vec(it->second, i))
                            {
                                loutI << "Succesfully deleted window from map current size:" << windowSignalMap.size() << loutEND;
                            }
                        }
                    }
                }
            }

            size_t size() const
            {
                return windowSignalMap.size();
            }
    };


    class __window_client_map__ {
        public:
            umap<uint32_t, client *> _data;

            void connect(uint32_t __window, client *__c) {
                _data[__window] = __c;

            }
            client *retrive(uint32_t __window) {
                auto it = _data.find(__window);
                if (it != _data.end()) {
                    return it->second;

                } return nullptr;

            }/** @brief @returns @p 'client' from uint32_t */
            void remove(uint32_t __window) {
                _data.erase(__window);

            }
    };

}
namespace XCB {
    inline xcb_get_geometry_cookie_t g_cok(uint32_t __w) {
        return xcb_get_geometry(conn, __w);                             
        
    }
    inline xcb_get_geometry_reply_t *g_r(xcb_get_geometry_cookie_t __cok) {
        return xcb_get_geometry_reply(conn, __cok, nullptr);           
        
    }
    inline xcb_get_window_attributes_cookie_t attributes_cookie(uint32_t __w) {
        return xcb_get_window_attributes(conn, __w);                      
        
    }    
    inline xcb_get_window_attributes_reply_t *attributes_reply(xcb_get_window_attributes_cookie_t __cok) {
        return xcb_get_window_attributes_reply(conn, __cok, nullptr);  
        
    }    
    inline xcb_intern_atom_cookie_t atom_cok(const char *__s) {
        if (strcmp(__s, "_NET_WM_STATE")) {
            return xcb_intern_atom(conn, 1, 12, __s);

        } return {};
            
    }
    inline xcb_intern_atom_reply_t* atom_r(xcb_intern_atom_cookie_t __atom_cok) {
        return xcb_intern_atom_reply(conn, __atom_cok, NULL);
    
    }
    bool is_mapped(uint32_t __window) {
        xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(conn, __window);
        xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, cookie, nullptr);
        if (reply == nullptr) {
            loutE << "Unable to get window attributes" << loutEND;
            return false;

        }
        uint8_t state = reply->map_state;
        free(reply);
        return (state == XCB_MAP_STATE_VIEWABLE);
    
    }

}

class __signal_manager__ {
    /* Defines   */
        #define WS_conn signal_manager->_window_signals.conect
        #define WS_emit(_window, _event) signal_manager->_window_signals.emit(_window, _event)
        #define WS_emit_Win(_window, _event, _w2) signal_manager->_window_signals.emit(_window, _event, _w2)
        #define WS_emit_root(_event, _w2) signal_manager->_window_signals.emit(screen->root, _event, _w2)
        #define W_callback \
            [this](uint32_t __window)

        #define CONN_Win(__window, __event, __callback) \
            signal_manager->_window_signals.conect(this->__window, __event, W_callback {__callback})

        #define CONN(__e, __cb, __w) \
            signal_manager->_window_signals.conect(__w, __e, W_callback {__cb})

        #define SIG(__window, __callback, __event) \
            signal_manager->_window_signals.conect(__window, __event, __callback)

        #define CONN_root(__event, __callback) \
            signal_manager->_window_signals.conect(screen->root, __event, __callback)

        #define CONN_Win2(window, __event, __ref, __callback) \
            signal_manager->_window_signals.conect(this->window, __event, [ref](uint32_t __window)  __callback)

        #define CONNECT_window_client(__window, __c) signal_manager->_window_client_map.connect(__window, __c)
        #define CWC(__window) CONNECT_window_client(this->__window, this)

        #define C_SIG(__c, __callback, __sig) \
            signal_manager->client_signals.connect(__c, __sig, [this](client *c) {__callback});

        #define C_RETRIVE(__window) \
            signal_manager->_window_client_map.retrive(__window)

    private:
    /* Variabels */
        unordered_map<string, vector<function<void()>>> signals;
        unordered_map<uint32_t, vector<pair<int, function<void()>>>> client_signal_map;

    public:
    /* Variabels */
        __window_signals__ _window_signals;
        __window_client_map__ _window_client_map;
        __c_func_arr__ client_arr;

    /* Methods   */
        template<typename Callback>
        void connect(const string &__signal_name, Callback &&callback) {
            signals[__signal_name].emplace_back(std::forward<Callback>(callback));

        }/* Connect a slot to a signal  */
        template<typename Callback>
        void connect_window(uint32_t __window, const string &__function, Callback &&callback) {
            signals[to_string(__window) + "__" + __function].emplace_back(std::forward<Callback>(callback));

        }/* Connect a slot to a signal */
        template<typename Callback>
        void connect_client(uint32_t __frame_window_id, int __client_signal, Callback &&callback) {
            client_signal_map[__frame_window_id].emplace_back(__client_signal, std::forward<Callback>(callback));

        }/* Connect a slot to a signal */
        void emit(const string &__signal_name) {
            auto it = signals.find(__signal_name);
            if (it != signals.end()) {
                for (auto& slot : it->second) {
                    slot();
                }

            }

        }/* Emit a signal, calling all connected slots */
        void emit_window(uint32_t __window, const string &__function) {
            auto it = signals.find(to_string(__window) + "__" + __function);
            if (it != signals.end()) {
                for (auto& slot : it->second) {
                    slot();

                }

            }

        }/* Emit a signal, calling all connected slots */
        void emit_client(uint32_t __frame_window_id, int __client_signal) {
            auto it = client_signal_map.find(__frame_window_id);
            if (it == client_signal_map.end()) {
                loutE << "client could not be found frame_window_id:" << __frame_window_id << loutEND;
                return;

            }            
            for (const auto &pair : it->second) {
                if (pair.first == __client_signal) {
                    pair.second();

                }

            }

        }
        void remove_client(uint32_t __frame_window_id) {
            client_signal_map.erase(__frame_window_id);

        }
        void init() {
            signals.reserve(40);

        }

}; static __signal_manager__ *signal_manager(nullptr);

class __window_attr__ {
    private:
        xcb_get_window_attributes_reply_t* reply = nullptr;
        uint32_t _window;

    /* Methods */
        xcb_get_window_attributes_cookie_t get_cookie__(uint32_t __window) {
            return xcb_get_window_attributes(conn, __window);

        }
        void get_reply__() {
            reply = xcb_get_window_attributes_reply(conn, get_cookie__(_window), nullptr);  // Error handling omitted for brevity
            
        }
    
    public:
        // Constructor
        __window_attr__(uint32_t __window) :
        _window(__window) {
            get_reply__();

        }

        // Destructor
        ~__window_attr__() {
            free(reply);  // Free the allocated memory

        }

        // Deleted copy constructor and copy assignment operator to prevent copying
        __window_attr__(const __window_attr__&) = delete;
        __window_attr__& operator=(const __window_attr__&) = delete;

        // Move constructor and move assignment operator
        __window_attr__(__window_attr__&& other) noexcept : reply(other.reply)
        {
            other.reply = nullptr;
        }

        __window_attr__& operator=(__window_attr__&& other) noexcept
        {
            if (this != &other)
            {
                free(reply);  // Free existing reply
                reply = other.reply;
                other.reply = nullptr;
            }

            return *this;
        }

        // Public method to access the reply
        xcb_get_window_attributes_reply_t *
        get() const
        {
            return reply;
        }

};

class __atoms__ {
    public:
        xcb_atom_t WM_DELETE_WINDOW, WM_PROTOCOLS;
    
}; static __atoms__ *atoms(nullptr);

class __crypto__ {
    /* Defines */
        #define HASH(__input) \
            crypro->str_hash_32(__input)

    public:
    /* Methods */
        vector<unsigned long> hash_str_32_bit_fixed(const string &__input) {
            hash<string> hasher;
            auto hashedValue = hasher(__input);
            vector<unsigned long> hashSequence(32);
            for (size_t i = 0; i < hashSequence.size(); ++i) {
                hashSequence[i] = hashedValue ^ (hashedValue >> (i % (sizeof(size_t) * 8)));

            } return hashSequence;

        }
        string hash_vec_to_str(const vector<unsigned long> &__hash_vec)
        {
            stringstream ss;
            for (size_t i = 0; i < __hash_vec.size(); ++i) {
                // Convert each number to a string and concatenate them
                // You might want to use a delimiter if you need to parse this string back into numbers
                ss << __hash_vec[i];
                if (i < __hash_vec.size() - 1)
                {
                    ss << ", "; // Add a delimiter (comma) between the numbers
                }
            }
            return ss.str();
        }

        string str_hash_32(const string &__input)
        {
            // Hash the input
            hash<string> hashFn;
            auto hash = hashFn(__input);

            // Convert hash to a hexadecimal string
            stringstream hexStream;
            hexStream << hex << hash;

            // Get the hexadecimal string
            string hexString = hexStream.str();

            // Ensure the hexadecimal string is 32 characters long
            // Note: This involves padding and possibly trimming,
            // assuming size_t is less than or equal to 64 bits.
            string paddedHexString = string(32 - std::min(32, static_cast<int>(hexString.length())), '0') + hexString;
            if (paddedHexString.length() > 32)
            {
                paddedHexString = paddedHexString.substr(0, 32);
            }

            // Optionally, convert hex to a purely numeric string if needed
            // For simplicity, we'll assume the hex string suffices for demonstration

            return paddedHexString;
        }

}; static __crypto__ *crypro(nullptr);

namespace fs = filesystem;
class __file_system__ {
    /* Defines */
        #define FS_ACC(__folder, __sub_path) file_system->accessor(__folder, __sub_path)
        #define ICONFOLDER __file_system__::ICON_FOLDER
        #define ICON_FOLDER_HASH(__class) FS_ACC(ICONFOLDER, HASH(__class))
        #define PNG_HASH(__class) FS_ACC(ICONFOLDER, HASH(__class) + "/" + HASH("png"))

    private:
    /* Variabels */
        typedef enum : uint8_t {
            FOLDER
        } create_type_t;

    public:
    /* Variabels */
        string config_folder = USER_PATH_PREFIX("/.config/mwm"), icon_folder = config_folder + "/icons"; bool status;
        typedef enum : uint8_t {
            CONDIG_FOLDER,
            ICON_FOLDER
        } accessor_t;
    
    /* Methods */
        void create(const string &__path, create_type_t __type = FOLDER) {
            if (__type == FOLDER) {
                bool create_bool = false;

                if (!fs::exists(__path)) {
                    loutI << "dir:" << loutPath(__path) << " does not exist creating dir" << '\n';    
                    try { create_bool = fs::create_directory(__path); 
                    } catch (exception &__e) { loutE << "Failed to create dir:" << loutPath(__path) << " error: " << __e.what() << "\n"; status = false; }

                    if (create_bool == true) {
                        loutI << "Successfully created dir:" << loutPath(__path) << '\n';
                        status = true;

                    }

                }

                if (!fs::is_directory(__path))
                {
                    bool remove_bool = false;
                    loutI << "dir:" << loutPath(__path) << " exists but is not a dir deleting and remaking as dir" << '\n';

                    try
                    {
                        remove_bool = fs::remove(__path);
                    }
                    catch (exception &__e)
                    {
                        loutE << "Failed to remove non-dir:" << loutPath(__path) << " error: " << __e.what() << '\n';
                        status = false;
                    }


                    if (remove_bool == true)
                    {
                        try
                        {
                            create_bool = fs::create_directory(__path);
                        }
                        catch (exception &__e)
                        {
                            loutE << "Failed to create dir:" << loutPath(__path) << " error: " << __e.what() << '\n';
                            status = false;
                        }

                        if (create_bool == true)
                        {
                            loutI << "Successfully created dir:" << loutPath(__path) << '\n';
                            status = false;
                        }
                    }
                }
            }
        }
        void init_check() {
            create(config_folder);
            create(icon_folder);

        }
        bool check_status() {
            return status;

        }
        string accessor(accessor_t __folder, const string &__sub_path)  {
            if (__folder == CONDIG_FOLDER) {
                return (config_folder + "/" + __sub_path);

            } else if (__folder == ICON_FOLDER) {
                return (icon_folder + "/" + __sub_path);

            } return string();

        }

}; static __file_system__ *file_system(nullptr);

class __net_logger__ {
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
        /**
         *
         * @brief fetches the calling class's name and extracts a substr containing only the class name
         *
         * NOTE: this only works for class's with prefix '__'
         *
         */
        #define CLASS_NAME                          CLASS_NAME_STR.substr(CLASS_NAME_STR.find("__"))
        #define NET_LOG_CLASS                       "\033[35m(CLASS) " + CLASS_NAME + "\033[0m"
        
        // #define NET_LOG(__type)                     net_logger->send_to_server(__type)

        #ifdef NET_LOG_ENABLED
            #define NET_LOG(__type) \
                net_logger->send_to_server(__type)
        #else
            #define NET_LOG(__type) \
                (void)0
        #endif

        #ifdef NET_LOG_ENABLED
            #define INIT_NET_LOG(__address) \
                net_logger = new __net_logger__; \
                net_logger->init(__address);
        #else
            #define INIT_NET_LOG(__address) \
                (void)0
        #endif

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
#ifdef NET_LOG_ENABLED
    static __net_logger__ *net_logger(nullptr);
#endif

struct size_pos {
    void save(const int & x, const int & y, const int & width, const int & height) {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;

    } int16_t x, y; uint16_t width, height;

};

class mxb {
    class XConnection {
        public:
        // constructor and destructor
            struct mxb_auth_info_t
            {
                int namelen;
                char* name;
                int datalen;
                char* data;
            };

            XConnection(const char * display)
            {
                string socketPath = getSocketPath(display);
                memset(&addr, 0, sizeof(addr)); // Initialize address
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
                fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create socket 
                if (fd == -1)
                {
                    throw runtime_error("Failed to create socket");
                }

                if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) // Connect to the X server's socket
                { 
                    ::close(fd);
                    throw
                    std::runtime_error("Failed to connect to X server");
                }
                int displayNumber = parseDisplayNumber(display); // Perform authentication 
                if (!authenticate_x11_connection(displayNumber, auth_info))
                {
                    ::close(fd);
                    throw runtime_error("Failed to authenticate with X server");
                }
            }
            
            ~XConnection()
            {
                if(fd != -1)
                {
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
                
                while(received < sizeof(reply)) /* Read the response from the server */
                {
                    int n = recv(fd, reply + received, sizeof(reply) - received, 0);
                    if(n == -1)
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
                    throw runtime_error("Invalid response received from X server");
                }
                
                bool extensionPresent = reply[1]; 
                // Check if the extension is present
                if (extensionPresent)
                {
                    return("Extension is supported by the X server."); 
                }
                else
                {
                    return "Extension is not supported by the X server.";
                }
            }

        private:
            int(fd);
            struct sockaddr_un(addr);
            mxb_auth_info_t(auth_info);
            Logger(log);
            
            bool authenticate_x11_connection(int display_number, mxb_auth_info_t & auth_info)
            {
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

            string getSocketPath(const char * display)
            {
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

            int parseDisplayNumber(const char * display)
            {
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
            loutI << response << loutEND;
        }
        catch (const exception &e)
        {
            loutE << e.what() << loutEND;
            return 1;
        }

        return 0;
    };
};

class pointer {
    public:
    /* Methods   */
        int16_t x() {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply) {
                loutE << "reply is nullptr" << loutEND;
                return 0;

            } int16_t x = reply->root_x;
            
            free(reply);
            return x;

        }

        int16_t y()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply)
            {
                loutE << "reply is nullptr" << loutEND;
                return 0;
            }

            int16_t y = reply->root_y;
            free(reply);
            return y;
        }

        void teleport(const int16_t &x, const int16_t &y)
        {
            VOID_COOKIE = xcb_warp_pointer(
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
            FLUSH_X();
            CHECK_VOID_COOKIE();
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
                loutE << "reply is nullptr." << loutEND;
                free(reply);
                return;
            }

            if (reply->status != XCB_GRAB_STATUS_SUCCESS)
            {
                loutE << "Could not grab pointer" << loutEND;
                free(reply);
                return;
            }

            free(reply);
        }

        void ungrab()
        {
            VOID_COOKIE = xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            FLUSH_X();
            CHECK_VOID_COOKIE();
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

};

class __pointer__ {
    public:
    /* Methods   */
        int16_t x()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply)
            {
                loutE << "reply is nullptr" << loutEND;
                return 0;
            }

            int16_t x = reply->root_x;
            free(reply);
            return x;
        }

        int16_t y()
        {
            xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, screen->root);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, cookie, nullptr);
            if (!reply)
            {
                loutE << "reply is nullptr" << loutEND;
                return 0;
            }

            int16_t y = reply->root_y;
            free(reply);
            return y;
        }

        void teleport(const int16_t &x, const int16_t &y)
        {
            VOID_COOKIE = xcb_warp_pointer(
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
            FLUSH_X();
            CHECK_VOID_COOKIE();
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
                loutE << "reply is nullptr." << loutEND;
                free(reply);
                return;
            }

            if (reply->status != XCB_GRAB_STATUS_SUCCESS)
            {
                loutE << "Could not grab pointer" << loutEND;
                free(reply);
                return;
            }

            free(reply);
        }

        void ungrab()
        {
            VOID_COOKIE = xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
            FLUSH_X();
            CHECK_VOID_COOKIE();
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

}; static __pointer__ *m_pointer(nullptr);

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

class string_tokenizer {
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

class fast_str_vector {
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

class Directory_Searcher {
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
                    loutE << "opendir() failed for directory: " << dir << loutEND;
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

};

class Directory_Lister {
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
                loutE << "Failed to open Directory: " << Directory << loutEND;
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

};

class File {
    public:
    // Sub Classes.
        class search {
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

extern char **environ;
class __pid_manager__ {
    private:
    /* Structs     */
        typedef struct {
            pid_t  pid;
            string name;
        } pid_data_t;

    /* Variabels   */
        vector<pid_data_t> _pid_vec;

    /* Methods     */
        /* Pid Info    */
            string pid_status__(pid_t __pid) {
                string line_str = "/proc/" + to_string(__pid) + "/status";
                ifstream file; file.open(line_str);
                string var;
                stringstream buffer;
                while (getline(file, var)) {
                    buffer << var << '\n';

                } string result; result = buffer.str();
                
                loutI << result << '\n';
                file.close();
                return string();

            }
            string get_process_name_by_pid__(pid_t pid) {
                string path = "/proc/" + std::to_string(pid) + "/comm";
                ifstream commFile(path);
                string name;

                if (commFile.good()) {
                    getline(commFile, name);
                    return name;
                
                } else {
                    return "Process not found";

                }

            }
            string pid_cmd_line__(pid_t __pid) {
                string line_str = "/proc/" + to_string(__pid) + "/cmdline";
                ifstream file;
                file.open(line_str);
                string var;
                stringstream buffer;
                while (getline(file, var)) {
                    buffer << var << '\n';
                
                } string result; result = buffer.str();

                loutI << result << '\n';
                string test = result;
                ifstream iss(test);
                string token;

                vector<string> parts;
                while (getline(iss, token, ' ')) {
                    parts.push_back(token);

                }
                if (parts.size() == 1) {
                    file.close();
                    return "mainPid";

                }
                file.close();
                
                return string();

            }
            string get_correct_process_name__(const string &__launchName) {
                DIR* dir;
                struct dirent* ent;
                string path;
                string line;

                vector<string> parts;
                for (int i(0), start(0); i < __launchName.length(); ++i) {
                    if (__launchName[i] == '-') {
                        string s = __launchName.substr(start, i - start);
                        parts.push_back(s);
                        start = i + 1;

                    }
                    if (i == (__launchName.length() - 1)) {
                        string s = __launchName.substr(start, i - start);
                        parts.push_back(s);

                    }

                }
                for (int i = 0; i < parts.size(); ++i) {
                    if ((dir = opendir("/proc")) != NULL) {
                        while ((ent = readdir(dir)) != NULL) {
                            if (ent->d_type == DT_DIR) {
                                path = std::string("/proc/") + ent->d_name + "/comm";
                                std::ifstream comm(path.c_str());
                                if (comm.good()) {
                                    getline(comm, line);
                                    if (line == parts[i]) {
                                        return parts[i];

                                    }

                                }

                            }/* Check if the directory is a PID */

                        } closedir(dir);

                    }

                } return string();

            }
            bool is_process_running__(const pid_t __pid) {
                struct stat statBuf;
                string procPath = "/proc/" + to_string(__pid);
                return stat(procPath.c_str(), &statBuf) == 0;
            }

        /* Pid Killing */
            bool send_signal__(const pid_t pid, int signal) {
                return kill(pid, signal) == 0;

            }
            bool send_sigterm__(pid_t __pid, const string &__name) {
                if (kill(__pid, SIGTERM) == -1) {
                    loutE << ERRNO_MSG("Error sending SIGTERM") << " Process " << __name << __pid << loutEND;
                    return false;

                }

                int status;
                pid_t result = waitpid(__pid, &status, 0); // Wait for the process to change state
                if (result == -1) {
                    loutE << ERRNO_MSG("Error waiting for process") << " Process " << __name << __pid << loutEND;
                    return false;
                    
                } 
                if (!is_process_running__(__pid))/* Check if the child exited normally */ {
                    loutI << "Process " << __name << __pid << " terminated successfully with exit status " << WEXITSTATUS(status) << loutEND;
                    return true;

                } else {
                    loutI << "Process " << __name << __pid << " did not terminate successfully." << loutEND;
                    return false;

                }

            }
            void send_sigkill__(pid_t __pid, const string &__name) {
                if (send_signal__(__pid, SIGKILL)) {
                    loutI << "SIGKILL signal sent to process " << __name << __pid << " for forceful termination." << loutEND;

                } else {
                    loutE << "Failed to send SIGKILL to process " << __name << __pid << loutEND;

                }

            }
            void kill_pid__(pid_t __pid, const string &__name) {
                if (is_process_running__(__pid)) {
                    if (!send_sigterm__(__pid, __name)) {
                        loutI << "Process " << __name << __pid << " still running forcefully killing" << loutEND;
                        send_sigkill__(__pid, __name);

                    }

                }

            }

        void check_vec__() {
            for (int i = 0; i < _pid_vec.size(); ++i) {
                if (!is_process_running__(_pid_vec[i].pid)) {
                    remove_element_from_vec(_pid_vec, i);

                }

            }

        }

    public:
    /* Methods     */
        void add_pid(pid_t __pid) {
            _pid_vec.push_back({__pid, get_process_name_by_pid__(__pid)});
            check_vec__();

        }
        void kill_all_pids() {
            for (pid_data_t pid_data : _pid_vec) {
                if (pid_data.name == "code") continue;
                kill_pid__(pid_data.pid, pid_data.name);

            }

        }
        void check_pid(pid_t __pid) {
            if (__pid == 0) return;

            bool found = false;
            for (int i = 0; i < _pid_vec.size(); ++i) {
                if (__pid == _pid_vec[i].pid) {
                    found = true;
                    break;

                }

            }
            if (!found) {
                add_pid(__pid);

            }

        }
        void remove_pid(pid_t __pid) {
            for (int i = 0; i < _pid_vec.size(); ++i) {
                if (_pid_vec[i].pid == __pid) {
                    remove_element_from_vec(_pid_vec, i);

                }

            }

        }
        void list_pids() {
            check_vec__();
            for (int i = 0; i < _pid_vec.size(); ++i) {
                loutI << "pid" << _pid_vec[i].pid << " name: " << _pid_vec[i].name << '\n';

            } loutI << "Total running pids" << _pid_vec.size() << loutEND;

        }

    /* Constructor */
        __pid_manager__() {}

}; static __pid_manager__ *pid_manager(nullptr);

class Launcher {
    public:
    /* Methods   */
        int program(char *program) {
            if (!file.check_if_binary_exists(program)) {
                return 1;

            }
            if (fork() == 0) {
                setsid();
                execvp(program, (char *[]) { program, NULL });

            } return 0;

        }
        int launch_child_process(const char *command) {
            pid_t pid;
            posix_spawnattr_t attr;

            string command_path = "/bin/" + string(command);
            char *argv[] = {const_cast<char*>(command_path.c_str()), NULL};
            
            posix_spawnattr_init(&attr);
            posix_spawnattr_setpgroup(&attr, 0);
            
            int result = posix_spawn(&pid, command_path.c_str(), NULL, &attr, argv, environ);
            posix_spawnattr_destroy(&attr);

            if (result == 0) {
                pid_manager->add_pid(pid);
                return 0;

            } else {
                loutErrno("posix_spawn failed");
                return result;

            }

        }
        static int static_launch_child_process(const char *command) {
            pid_t pid;
            posix_spawnattr_t attr;

            string command_path = "/bin/" + string(command);
            char *argv[] = {const_cast<char*>(command_path.c_str()), NULL};
            
            posix_spawnattr_init(&attr);
            posix_spawnattr_setpgroup(&attr, 0);
            
            int result = posix_spawn(&pid, command_path.c_str(), NULL, &attr, argv, environ);
            posix_spawnattr_destroy(&attr);

            if (result == 0) {
                pid_manager->add_pid(pid);
                return 0;

            } else {
                loutErrno("posix_spawn failed");
                return result;

            }

        }
    
    private:
    /* Variabels */
        File file;
};

class __key_codes__ {
    public:
    // constructor and destructor.
        __key_codes__() 
        : keysyms(nullptr) {}

        ~__key_codes__()
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
                    { TAB,          &tab       },
                    { SUPER_L,      &super_l   },
                    { MINUS,        &minus     },
                    { UNDERSCORE,   &underscore}
                };
                
                for (auto &pair : key_map)
                {
                    xcb_keycode_t * keycode = xcb_key_symbols_get_keycode(keysyms, pair.first);
                    if (keycode)
                    {
                        *(pair.second) = *keycode;
                        free(keycode);
                    }
                }
            }
        }

        constexpr uint8_t char_to_keycode__(int8_t c)
        {
            switch (c)
            {
                case 'a': return this->a;
                case 'b': return this->b;
                case 'c': return this->c;
                case 'd': return this->d;
                case 'e': return this->e;
                case 'f': return this->f;
                case 'g': return this->g;
                case 'h': return this->h;
                case 'i': return this->i;
                case 'j': return this->j;
                case 'k': return this->k;
                case 'l': return this->l;
                case 'm': return this->m;
                case 'n': return this->n;
                case 'o': return this->o;
                case 'p': return this->p;
                case 'q': return this->q;
                case 'r': return this->r;
                case 's': return this->s;
                case 't': return this->t;
                case 'u': return this->u;
                case 'v': return this->v;
                case 'w': return this->w;
                case 'x': return this->x;
                case 'y': return this->y;
                case 'z': return this->z;
                case '-': return this->minus;
                case ' ': return this->space_bar;
            }

            return (uint8_t)0;
        }

    // variabels.
        xcb_keycode_t
            a{}, b{}, c{}, d{}, e{}, f{}, g{}, h{}, i{}, j{}, k{}, l{}, m{},
            n{}, o{}, p{}, q{}, r{}, s{}, t{}, u{}, v{}, w{}, x{}, y{}, z{},
            
            space_bar{}, enter{},

            f11{}, n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{},
            l_arrow{}, u_arrow{}, d_arrow{}, tab{}, _delete{},
            super_l{}, minus{}, underscore{};

    private:
    // variabels.
        xcb_key_symbols_t * keysyms;
};

queue<xcb_expose_event_t *> expose_events;

using Ev = const xcb_generic_event_t *;
class __event_handler__ {
    /* Defines   */
        #define EV_ID \
            int event_id

        #define ADD_EV_ID_WIN(__window, __event_type) \
            __window.add_event_id(__event_type, event_id)

        #define ADD_EV_ID_IWIN(__event_type) \
            add_event_id(__event_type, event_id)

        #define EV_LOOP(__type)                                 \
            case __type:                                        \
            {                                                   \
                for (const auto &pair : eventCallbacks[__type]) \
                {                                               \
                    pair.second(ev);                            \
                }                                               \
                continue;                                       \
            }

        #define MAKE_HANDLE_THREAD(__event_type) \
            thread(handleEvent<__event_type>, (__event_type*)ev).detach()

    public:
    /* Variabels */
        __key_codes__ key_codes;
        mutex event_mutex;

    /* Methods   */
        #define Emit signal_manager->_window_signals.emit
        #define handle_template(__type) template<> void handle_event<__type> (uint32_t __w)
        template<uint8_t __sig>
        static void handle_event(uint32_t __w) { Emit(__w, __sig); }
            handle_template(ROOT_SIGNAL)               { Emit(screen->root, __w); }
            handle_template(XCB_MAP_REQUEST)           { Emit(screen->root, XCB_MAP_REQUEST, __w); }
            handle_template(XCB_MAP_NOTIFY)            { Emit(screen->root, XCB_MAP_NOTIFY,  __w); }
            handle_template(MOVE_TO_NEXT_DESKTOP)      { Emit(screen->root, MOVE_TO_NEXT_DESKTOP, __w); }
            handle_template(MOVE_TO_PREV_DESKTOP)      { Emit(screen->root, MOVE_TO_PREV_DESKTOP, __w); }
            handle_template(MOVE_TO_NEXT_DESKTOP_WAPP) { Emit(screen->root, MOVE_TO_NEXT_DESKTOP_WAPP, __w); }
            handle_template(MOVE_TO_PREV_DESKTOP_WAPP) { Emit(screen->root, MOVE_TO_PREV_DESKTOP_WAPP, __w); }
            handle_template(TILE_RIGHT)                { C_EMIT(C_RETRIVE(__w), TILE_RIGHT); }
            handle_template(TILE_LEFT)                 { C_EMIT(C_RETRIVE(__w), TILE_LEFT ); }
            handle_template(TILE_UP)                   { C_EMIT(C_RETRIVE(__w), TILE_UP   ); }
            handle_template(TILE_DOWN)                 { C_EMIT(C_RETRIVE(__w), TILE_DOWN ); }
            handle_template(EWMH_MAXWIN_SIGNAL)        { C_EMIT(C_RETRIVE(__w), EWMH_MAXWIN_SIGNAL); }

        #define HANDLE_EVENT(__type ) thread(handle_event<__type>, e->event    ).detach()
        #define HANDLE_WINDOW(__type) thread(handle_event<__type>, e->window   ).detach()
        #define HANDLE_ROOT(__type)   thread(handle_event<__type>, screen->root).detach()
        #define HANDLE(__type, __w)   thread(handle_event<__type>, __w         ).detach()

        using EventCallback = function<void(Ev)>;
        void run() {
            key_codes.init();
            shouldContinue = true;
            xcb_generic_event_t *ev;

            setEventCallback(XCB_EXPOSE, [&](Ev ev) {
                RE_CAST_EV(xcb_expose_event_t);
                Emit(e->window, XCB_EXPOSE);/* 
                thread_pool.enqueue([](uint32_t __w) {
                }, e->window); */
            });
            setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_enter_notify_event_t);
                Emit(e->event, XCB_ENTER_NOTIFY);/* 
                thread_pool.enqueue([](uint32_t w) {
                }, e->event); */
            });
            setEventCallback(XCB_LEAVE_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_leave_notify_event_t);
                Emit(e->event, XCB_LEAVE_NOTIFY);
                /* thread_pool.enqueue([](uint32_t w) {
                }, e->event); */
            });
            setEventCallback(XCB_FOCUS_IN, [&](Ev ev) {
                RE_CAST_EV(xcb_focus_in_event_t);
                Emit(e->event, XCB_FOCUS_IN);/* 
                thread_pool.enqueue([](uint32_t w) {
                }, e->event); */
            });
            setEventCallback(XCB_FOCUS_OUT, [&](Ev ev) {
                RE_CAST_EV(xcb_focus_out_event_t);
                Emit(e->event, XCB_FOCUS_OUT);/* 
                thread_pool.enqueue([](uint32_t w) {
                }, e->event); */
            });
            setEventCallback(XCB_KEY_PRESS, [&](Ev ev) {
                RE_CAST_EV(xcb_key_press_event_t);
                /* switch (e->state) { */
                    /* case   CTRL  + ALT          :{
                        if (e->detail == key_codes.t) {
                            thread_pool.enqueue([](){
                                Emit(screen->root, TERM_KEY_PRESS);
                                
                            });

                        } // Terminal keybinding
                        break;

                    } *//*  case SHIFT + CTRL + SUPER :{
                        if (e->detail == key_codes.r_arrow) {
                            thread_pool.enqueue([](uint32_t w) {
                                Emit(w, MOVE_TO_NEXT_DESKTOP_WAPP);
                                
                            }, e->event);
                            thread(handle_event<MOVE_TO_NEXT_DESKTOP_WAPP>, e->event).detach();

                        } else if (e->detail == key_codes.l_arrow) {
                            thread(handle_event<MOVE_TO_PREV_DESKTOP_WAPP>, e->event).detach();

                        } break;

                    } */ /* case SHIFT + ALT          :{
                        if (e->detail == key_codes.q) {
                            thread(handle_event<ROOT_SIGNAL>, QUIT_KEY_PRESS).detach();

                        } // Quit keybinding
                        break;

                    } */ /* case ALT                  :{
                        if (e->detail == key_codes.n_1) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_DESKTOP_1).detach();

                        } else if (e->detail == key_codes.n_2) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_DESKTOP_2).detach();

                        } else if (e->detail == key_codes.n_3) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_DESKTOP_3).detach();
                            
                        } else if (e->detail == key_codes.n_4) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_DESKTOP_4).detach();

                        } else if (e->detail == key_codes.n_5) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_DESKTOP_5).detach();
                            
                        } */ /* else if (e->detail == key_codes.tab) {
                            thread(handle_event<ROOT_SIGNAL>, CYCLE_FOCUS_KEY_PRESS).detach();

                        } */
                        /* break;

                    } */ /* case CTRL  + SUPER        :{
                        if (e->detail == key_codes.r_arrow) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_NEXT_DESKTOP).detach();

                        } else if (e->detail == key_codes.l_arrow) {
                            thread(handle_event<ROOT_SIGNAL>, MOVE_TO_PREV_DESKTOP).detach();

                        }
                        break;

                    }  *//* case SUPER                :{
                        if (e->detail == key_codes.r_arrow)        {
                            HANDLE_EVENT(TILE_RIGHT);

                        } else if (e->detail == key_codes.l_arrow) {
                            HANDLE_EVENT(TILE_LEFT);

                        } else if (e->detail == key_codes.u_arrow) {
                            HANDLE_EVENT(TILE_UP);

                        } else if (e->detail == key_codes.d_arrow) {
                            HANDLE_EVENT(TILE_DOWN);

                        } else  */
                        /* if (e->detail == key_codes.k)       {
                            thread(handle_event<ROOT_SIGNAL>, DEBUG_KEY_PRESS).detach();
                        
                        }
                        break;

                    } */

                /* } */
                if (e->detail == key_codes.f11)
                {
                    HANDLE(EWMH_MAXWIN_SIGNAL, e->event);
                }

            });
            setEventCallback(XCB_DESTROY_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_destroy_notify_event_t);
                Emit(e->event, XCB_DESTROY_NOTIFY);
                Emit(e->window, XCB_DESTROY_NOTIFY);
                /* client *c;
                if ((c = signal_manager->_window_client_map.retrive(e->event)) != nullptr) {
                    HANDLE(DESTROY_NOTIF_EV, e->event);

                } else if ((c = signal_manager->_window_client_map.retrive(e->window)) != nullptr) {
                    HANDLE(DESTROY_NOTIF_W, e->window);

                } */

            });
            /* setEventCallback(XCB_MAP_REQUEST, [&](Ev ev) {
                RE_CAST_EV(xcb_map_request_event_t);
                HANDLE(XCB_MAP_REQUEST, e->window); 

            }); */
            /* setEventCallback(XCB_MOTION_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_motion_notify_event_t);
                thread_pool.enqueue([](uint32_t w) {
                    Emit(w, XCB_MOTION_NOTIFY);
                    
                }, e->event);

            }); */
            setEventCallback( XCB_BUTTON_PRESS, [&]( Ev ev )
            {
                RE_CAST_EV(xcb_button_press_event_t);
                if ( e->detail == L_MOUSE_BUTTON )
                {
                    if (( e->state & ALT ) != 0 )
                    {
                        Emit(e->event, L_MOUSE_BUTTON_EVENT__ALT);
                        /* thread_pool.enqueue([] (uint32_t w) {
                        }, e->event); */
                    }
                    else
                    {
                        Emit(e->event, L_MOUSE_BUTTON_EVENT);
                        /* thread_pool.enqueue([] (uint32_t w) { 
                        }, e->event); */

                    }
                }
                else if ( e->detail == R_MOUSE_BUTTON )
                {
                    if (( e->state & ALT ) != 0 )
                    {
                        /* thread_pool.enqueue([] (uint32_t w) {
                        }, e->event); */
                        Emit( e->event, R_MOUSE_BUTTON_EVENT__ALT );
                    }
                    else
                    {
                        Emit( e->event, R_MOUSE_BUTTON_EVENT );
                        /* thread_pool.enqueue([] (uint32_t w) {
                        }, e->event); */
                    }
                }
            });
            /* setEventCallback(XCB_MAP_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_map_notify_event_t);
                thread_pool.enqueue([](uint32_t w) {
                    Emit(screen->root, XCB_MAP_NOTIFY, w);
                    
                }, e->event);

            }); */
            setEventCallback( XCB_PROPERTY_NOTIFY, [&]( Ev ev ) {
                RE_CAST_EV(xcb_property_notify_event_t);
                Emit( e->window, XCB_PROPERTY_NOTIFY );/* 
                thread_pool.enqueue([](uint32_t __w) {
                }, e->window); */
            });

            while (shouldContinue) {
                ev = xcb_wait_for_event(conn);
                if (!ev) continue;

                uint8_t res = ev->response_type & ~0x80;   
                auto it = eventCallbacks.find(res);
                
                if (it != eventCallbacks.end()) {
                    for (const auto &pair : it->second) {
                        pair.second(ev);
                    }
                }
                free(ev);
            }
        }
        constexpr uint8_t char_to_keycode__(int8_t c) const {
            switch (c) {
                case 'a': return this->key_codes.a;
                case 'b': return this->key_codes.b;
                case 'c': return this->key_codes.c;
                case 'd': return this->key_codes.d;
                case 'e': return this->key_codes.e;
                case 'f': return this->key_codes.f;
                case 'g': return this->key_codes.g;
                case 'h': return this->key_codes.h;
                case 'i': return this->key_codes.i;
                case 'j': return this->key_codes.j;
                case 'k': return this->key_codes.k;
                case 'l': return this->key_codes.l;
                case 'm': return this->key_codes.m;
                case 'n': return this->key_codes.n;
                case 'o': return this->key_codes.o;
                case 'p': return this->key_codes.p;
                case 'q': return this->key_codes.q;
                case 'r': return this->key_codes.r;
                case 's': return this->key_codes.s;
                case 't': return this->key_codes.t;
                case 'u': return this->key_codes.u;
                case 'v': return this->key_codes.v;
                case 'w': return this->key_codes.w;
                case 'x': return this->key_codes.x;
                case 'y': return this->key_codes.y;
                case 'z': return this->key_codes.z;
                case '-': return this->key_codes.minus;
                case ' ': return this->key_codes.space_bar;

            } return (uint8_t)0;

        }
        constexpr void processEvent(xcb_generic_event_t* ev) {
            uint8_t responseType = ev->response_type & ~0x80;
            switch (responseType) {
                case XCB_KEY_PRESS:{
                    RE_CAST_EV(xcb_key_press_event_t);
                    switch (e->state) {
                        case (CTRL + ALT): {
                            if (e->detail == key_codes.t) {
                                HANDLE_EVENT(TERM_KEY_PRESS);

                            } return;

                        } case (SHIFT + CTRL + SUPER): {
                            if (e->detail == key_codes.r_arrow) {
                                HANDLE_EVENT(MOVE_TO_NEXT_DESKTOP_WAPP);

                            } else if (e->detail == key_codes.l_arrow) {
                                HANDLE_EVENT(MOVE_TO_PREV_DESKTOP_WAPP);

                            } return;

                        } case (SHIFT + ALT): {
                            if (e->detail == key_codes.q) {
                                HANDLE_EVENT(QUIT_KEY_PRESS);

                            } return;

                        } case ALT: {
                            if (e->detail == key_codes.n_1) {
                                HANDLE_EVENT(MOVE_TO_DESKTOP_1);

                            } else if (e->detail == key_codes.n_2) {
                                HANDLE_EVENT(MOVE_TO_DESKTOP_2);

                            } else if (e->detail == key_codes.n_3) {
                                HANDLE_EVENT(MOVE_TO_DESKTOP_3);
                                
                            } else if (e->detail == key_codes.n_4) {
                                HANDLE_EVENT(MOVE_TO_DESKTOP_4);

                            } else if (e->detail == key_codes.n_5) {
                                HANDLE_EVENT(MOVE_TO_DESKTOP_5);
                                
                            } else if (e->detail == key_codes.tab) {
                                HANDLE_EVENT(CYCLE_FOCUS_KEY_PRESS);

                            } return;

                        } case (CTRL + SUPER): {
                            if (e->detail == key_codes.r_arrow) {
                                HANDLE_EVENT(MOVE_TO_NEXT_DESKTOP);

                            } else if (e->detail == key_codes.l_arrow) {
                                HANDLE_EVENT(MOVE_TO_PREV_DESKTOP);

                            } break;

                        } case SUPER: {
                            if (e->detail == key_codes.r_arrow) {
                                HANDLE_EVENT(TILE_RIGHT);

                            } else if (e->detail == key_codes.l_arrow) {
                                HANDLE_EVENT(TILE_LEFT);

                            } else if (e->detail == key_codes.u_arrow) {
                                HANDLE_EVENT(TILE_UP);

                            } else if (e->detail == key_codes.d_arrow) {
                                HANDLE_EVENT(TILE_DOWN);

                            } else if (e->detail == key_codes.k) {
                                HANDLE_ROOT(DEBUG_KEY_PRESS);
                            
                            } return;

                        }

                    } if (e->detail == key_codes.f11) {
                        // HANDLE_EVENT(EWMH_MAXWIN);

                    } return;

                } case XCB_BUTTON_PRESS:{
                    RE_CAST_EV(xcb_button_press_event_t);
                    if (e->detail == L_MOUSE_BUTTON) {
                        if (e->state == ALT) {
                            HANDLE_EVENT(L_MOUSE_BUTTON_EVENT__ALT);

                        } else {
                            HANDLE_EVENT(L_MOUSE_BUTTON_EVENT);

                        } return;

                    } else if (e->detail == R_MOUSE_BUTTON) {
                        if (e->state == ALT) {
                            HANDLE_EVENT(R_MOUSE_BUTTON_EVENT__ALT);
                            
                        } else {
                            HANDLE_EVENT(R_MOUSE_BUTTON_EVENT);

                        } return;

                    } return;

                } case XCB_EXPOSE:{
                    RE_CAST_EV(xcb_expose_event_t);
                    HANDLE_WINDOW(XCB_EXPOSE);

                    return;

                } case XCB_PROPERTY_NOTIFY:{
                    RE_CAST_EV(xcb_property_notify_event_t);
                    HANDLE_WINDOW(XCB_PROPERTY_NOTIFY);

                    return;

                } case XCB_ENTER_NOTIFY:{
                    RE_CAST_EV(xcb_enter_notify_event_t);
                    HANDLE_EVENT(XCB_ENTER_NOTIFY);

                    return;

                } case XCB_LEAVE_NOTIFY:{
                    RE_CAST_EV(xcb_leave_notify_event_t);
                    HANDLE_EVENT(LEAVE_NOTIFY);

                    return;

                } case XCB_MAP_REQUEST: {
                    RE_CAST_EV(xcb_map_request_event_t);
                    HANDLE_WINDOW(MAP_REQ);

                    return;

                } case XCB_MAP_NOTIFY:{
                    RE_CAST_EV(xcb_map_notify_event_t);
                    HANDLE_EVENT(MAP_NOTIFY);

                    return;

                } case XCB_FOCUS_IN:{
                    RE_CAST_EV(xcb_focus_in_event_t);
                    HANDLE_EVENT(FOCUS_IN);

                    return;

                } case XCB_FOCUS_OUT:{
                    RE_CAST_EV(xcb_focus_out_event_t);
                    HANDLE_EVENT(FOCUS_OUT);

                    return;

                } case XCB_DESTROY_NOTIFY:{
                    RE_CAST_EV(xcb_destroy_notify_event_t);
                    HANDLE_EVENT(FOCUS_OUT);

                    return;

                } case XCB_MOTION_NOTIFY:{
                    RE_CAST_EV(xcb_motion_notify_event_t);
                    HANDLE_EVENT(MOTION_NOTIFY);
                    return;

                }
            }
        }
        void end() {
            shouldContinue = false;

        }
        using CallbackId = int; template<typename Callback>
        CallbackId setEventCallback(uint8_t eventType, Callback&& callback) {
            CallbackId id = nextCallbackId++;
            eventCallbacks[eventType].emplace_back(id, std::forward<Callback>(callback));
            return id;

        }

        void removeEventCallback(uint8_t eventType, CallbackId id) {
            auto& callbacks = eventCallbacks[eventType];
            callbacks.erase(
            remove_if(
                callbacks.begin(),
                callbacks.end(),
                [id](const auto &pair) {
                    return pair.first == id;

                }

            ), callbacks.end());            
            loutI << "Deleting id" << id << " " << EVENT_TYPE(eventType) << " vecsize" << eventCallbacks[eventType].size() << loutEND;

        }
        template<typename Callback>
        void set_key_press_callback(const uint16_t &__mask, const xcb_keycode_t &__key, Callback &&callback) {
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
        template<typename Callback>
        void set_button_press_callback(uint32_t __window, const uint16_t &__mask, const xcb_keycode_t &__key, Callback &&callback) {
            setEventCallback(XCB_KEY_PRESS, [this, __window, callback, __key, __mask](Ev ev) {
                RE_CAST_EV(xcb_key_press_event_t);
                if (e->event != __window) return;
                if (e->detail == __key) {
                    if (e->state == __mask) {
                        callback();

                    }

                }

            });

        }
        void iter_and_log_map_size() {
            uint16_t total_events = 0;
            for (const auto &pair : eventCallbacks)
            {
                loutI << EVENT_TYPE(pair.first) << " vecsize" << pair.second.size() << " res" << pair.second.capacity() << loutEND;
                total_events += pair.second.size();
            }
            
            loutI << "Total events in map" << total_events << loutEND;
            // loutI << "Total _window_arr size" << _window_arr.getSize() << loutEND;
        }
        // void init_map() {
        //     eventCallbacks.reserve(34);

        //     vector<uint8_t> event_type_keys = {
        //         XCB_KEY_PRESS,
        //         XCB_KEY_RELEASE,
        //         XCB_BUTTON_PRESS,
        //         XCB_BUTTON_RELEASE,
        //         XCB_MOTION_NOTIFY,
        //         XCB_ENTER_NOTIFY,
        //         XCB_LEAVE_NOTIFY,
        //         XCB_FOCUS_IN,
        //         XCB_FOCUS_OUT,
        //         XCB_KEYMAP_NOTIFY,
        //         XCB_EXPOSE,
        //         XCB_GRAPHICS_EXPOSURE,
        //         XCB_NO_EXPOSURE,
        //         XCB_VISIBILITY_NOTIFY,
        //         XCB_CREATE_NOTIFY,
        //         XCB_DESTROY_NOTIFY,
        //         XCB_UNMAP_NOTIFY,
        //         XCB_MAP_NOTIFY,
        //         XCB_MAP_REQUEST,
        //         XCB_REPARENT_NOTIFY,
        //         XCB_CONFIGURE_NOTIFY,
        //         XCB_CONFIGURE_REQUEST,
        //         XCB_GRAVITY_NOTIFY,
        //         XCB_RESIZE_REQUEST,
        //         XCB_CIRCULATE_NOTIFY,
        //         XCB_CIRCULATE_REQUEST,
        //         XCB_PROPERTY_NOTIFY,
        //         XCB_SELECTION_CLEAR,
        //         XCB_SELECTION_REQUEST,
        //         XCB_SELECTION_NOTIFY,
        //         XCB_COLORMAP_NOTIFY,
        //         XCB_CLIENT_MESSAGE,
        //         XCB_MAPPING_NOTIFY
        //     };

        //     for (int i = 0; i < event_type_keys.size(); ++i) {
        //         eventCallbacks[event_type_keys[i]].reserve(100);

        //     }
        // }

    private:
    /* Variables */
        unordered_map<uint8_t, vector<pair<CallbackId, EventCallback>>> eventCallbacks;
        bool shouldContinue = false;
        CallbackId nextCallbackId = 0;
        ThreadPool thread_pool{20};

}; static __event_handler__ *event_handler(nullptr);
using EventCallback = function<void(Ev)>;

class Bitmap {
    private:
    // variables
        int width, height;
        vector<vector<bool>>(bitmap);
        
    public:
    // methods
        void modify(int row, int startCol, int endCol, bool value)
        {
            if (row < 0 || row >= height || startCol < 0 || endCol > width)
            {
                loutE << "Invalid row or column indices" << loutEND;
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
                loutE << "Failed to create PNG file" << loutEND;
                return;
            }

            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr)
            {
                fclose(fp);
                loutE << "Failed to create PNG write struct" << loutEND;
                return;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr)
            {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, nullptr);
                loutE << "Failed to create PNG info struct" << loutEND;
                return;
            }

            if (setjmp(png_jmpbuf(png_ptr)))
            {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                loutE << "Error during PNG creation" << loutEND;
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

    // constructor
        Bitmap(int width, int height) 
        : width(width), height(height), bitmap(height, std::vector<bool>(width, false)) {}
    
};

class __color_bitmap__ {
    private:
    /* Variabels   */
        int width, height;
        std::vector<uint32_t> pixels; // Pixel data in ARGB format

    public:
    /* Constructor */
        __color_bitmap__(int width, int height, const vector<uint32_t>& pixels)
        : width(width), height(height), pixels(pixels) {}

    /* Methods     */
        void exportToPng(const char *fileName) const
        {
            FILE *fp = fopen(fileName, "wb");
            if (!fp)
            {
                loutE << "Could not open file: " << fileName << '\n';
                return;
            }

            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png_ptr)
            {
                fclose(fp);
                loutE << "Could not create write struct" << '\n';
                return;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr)
            {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, nullptr);
                loutE << "Could not create info struct" << '\n';
                return;
            }

            if (setjmp(png_jmpbuf(png_ptr)))
            {
                fclose(fp);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                loutE << "setjmp failed" << '\n';
                return;
            }

            png_init_io(png_ptr, fp);
            // Note: Using PNG_COLOR_TYPE_RGB_ALPHA to include alpha channel
            png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
            png_write_info(png_ptr, info_ptr);

            // Prepare row pointers
            png_bytep row_pointers[height];
            for (int y = 0; y < height; y++)
            {
                row_pointers[y] = (png_bytep)&pixels[y * width];
            }

            png_write_image(png_ptr, row_pointers);
            png_write_end(png_ptr, nullptr);
            
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
        }

};

class _scale {
    public:
    /* Methods */
        static uint16_t from_8_to_16_bit(const uint8_t & n)
        {
            return (n << 8) | n;
        }
};

namespace { /* 'window' class Namespace */
    enum BORDER {
        NONE  = 0,
        LEFT  = 1 << 0, /* 1  */
        RIGHT = 1 << 1, /* 2  */
        UP    = 1 << 2, /* 4  */
        DOWN  = 1 << 3, /* 8  */
        ALL   = LEFT | RIGHT | UP | DOWN
    };

    enum window_flags {
        MAP             = 1 << 0, /* 1 */
        DEFAULT_KEYS    = 1 << 1, /* 2 */
        FOCUS_INPUT     = 1 << 2, /* 4 */
        KEYS_FOR_TYPING = 1 << 3, /* 8 */ 
        RAISE           = 1 << 4  /* 16 */
    };

    enum window_event_mask : uint32_t {
        KILL_WINDOW = 1 << 25 /* 33554432 */
    };

    void get_atom(char *__name, xcb_atom_t *__atom)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, xcb_intern_atom(conn, 0, strlen(__name), __name), NULL);
        if (reply == nullptr)
        {
            *__atom = XCB_NONE;
            free(reply);
            loutE << "reply is nullptr." << loutEND;
            return;
        }

        *__atom = reply->atom;
        free(reply);
    }

    /**
     *
     * @brief Struct representing the _MOTIF_WM_HINTS property format 
     *
     */
    typedef struct {
        uint32_t flags;
        uint32_t functions;
        uint32_t decorations;
        int32_t  input_mode;
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

    namespace { // WINDOW_CONFIG.
        enum class WINDOW_CONF : uint32_t {
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

        DEFINE_BITWISE_ENUM(WINDOW_CONFIG, uint16_t) {
            X      = 1 << 0,
            Y      = 1 << 1,
            WIDTH  = 1 << 2,
            HEIGHT = 1 << 3
        };
    }

    /** EXPEREMENT: TODO: implement  */
    enum DataType {
        TYPE_NONE,
        TYPE_INT,
        TYPE_FLOAT,
        // Add more types as needed
        // TYPE_KEYS_DATA,
        // TYPE_OTHER_DATA,
    };

    struct WindowData {
        DataType type;
        union {
            int intValue;
            float floatValue;
            // Define other data types here
            // keys_data_t keysData;
            // other_data_t otherData;
        };
    };

    enum WINDOW_BIT_STATES
    {
        Xid_gen_success = 0
    };
}

class window {
    public:
    /* Constructor */
        window() {}

    /* Operators   */
        operator uint32_t()
        {
            return _window;

        }    
        window& operator=( uint32_t new_window ) // Overload the assignment operator for uint32_t
        { 
            _window = new_window;
            return *this;
        }
    
    /* Methods     */
        /* Create        */
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
                        const void     *value_list) {
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
            void create_default(const uint32_t &parent, const int16_t &x, const int16_t &y, const uint16_t &width, const uint16_t &height) {
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
            void create_def(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask) {
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

                if (__mask > 0) {
                    apply_event_mask(&__mask);
                }

            }
            void create_def_no_keys(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask) {
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
            void create_def_and_map(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask) {
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
            void create_def_and_map_no_keys(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask) {
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
            void create_def_and_map_no_keys_with_borders(const uint32_t &__parent, const int16_t &__x, const int16_t &__y, const uint16_t &__width, const uint16_t &__height, COLOR __color, const uint32_t &__mask, int __border_info[3]) {
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
                                void    *__border_data = nullptr,
                                CURSOR   __cursor = CURSOR::arrow) {
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
                if (__flags & DEFAULT_KEYS   ) grab_default_keys();
                if (__flags & KEYS_FOR_TYPING) grab_keys_for_typing();
                if (__flags & FOCUS_INPUT    ) focus_input();
                if (__flags & MAP) {
                    map();
                    raise();

                }
                if (__event_mask > 0) apply_event_mask(&__event_mask);

                if (__border_data != nullptr) {
                    int *border_data = static_cast<int *>(__border_data);
                    make_borders(border_data[0], border_data[1], border_data[2]);

                }
                if (__cursor != CURSOR::arrow) {
                    set_pointer(__cursor);

                }
                if (__flags & RAISE) {
                    raise();

                }
            
            }
            void create_client_window(const uint32_t &parent, const int16_t &x, const int16_t &y, const uint16_t &width, const uint16_t &height) {
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

        /* Borders       */
            void make_borders(int __border_mask, uint32_t __size, int __color) {
                if (__border_mask & UP   ) make_border_window(UP,    __size, __color);
                if (__border_mask & DOWN ) make_border_window(DOWN,  __size, __color);
                if (__border_mask & LEFT ) make_border_window(LEFT,  __size, __color);
                if (__border_mask & RIGHT) make_border_window(RIGHT, __size, __color);
            
            }
            #define CHANGE_BORDER_COLOR(__window) \
                if (__window != 0)                                   \
                {                                                    \
                    change_back_pixel(get_color(__color), __window); \
                    FLUSH_X();                                       \
                    clear_window(__window);                          \
                    FLUSH_X();                                       \
                }
            ;
            void change_border_color(int __color, int __border_mask = ALL) {
                if (__border_mask & UP   ) CHANGE_BORDER_COLOR(_border[0]);
                if (__border_mask & DOWN ) CHANGE_BORDER_COLOR(_border[1]);
                if (__border_mask & LEFT ) CHANGE_BORDER_COLOR(_border[2]);
                if (__border_mask & RIGHT) CHANGE_BORDER_COLOR(_border[3]);
            
            }
            void make_xcb_borders(const int &__color) {
                VOID_COOKIE = xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_BORDER_PIXEL,
                    (uint32_t[1]) {
                        get_color(__color)

                    }

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
        
        /* Main          */
            void raise() {
                VOID_COOKIE = xcb_configure_window(
                    conn,
                    _window,
                    XCB_CONFIG_WINDOW_STACK_MODE, 
                    (const uint32_t[1]) {
                        XCB_STACK_MODE_ABOVE

                    }
                
                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }            
            void map() {
                VOID_COOKIE = xcb_map_window(conn, _window);
                CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            void unmap(){
                VOID_COOKIE = xcb_unmap_window(conn, _window);
                CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }            
            void reparent(uint32_t __new_parent, int16_t __x, int16_t __y) {
                VOID_COOKIE = xcb_reparent_window(
                    conn,
                    _window,
                    __new_parent,
                    __x,
                    __y
                
                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            void make_child(uint32_t __window_to_make_child, uint32_t __child_x, uint32_t __child_y) {
                VOID_COOKIE = xcb_reparent_window(
                    conn,
                    __window_to_make_child,
                    _window,
                    __child_x,
                    __child_y
                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }            
            void kill()
            {
                uint32_t w = this->_window;
                
                xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom( conn, 1, 12, "WM_PROTOCOLS" );
                xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply( conn, protocols_cookie, nullptr );

                xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom( conn, 0, 16, "WM_DELETE_WINDOW" );
                xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply( conn, delete_cookie, nullptr );

                if ( ! protocols_reply )
                {
                    loutE << "protocols reply is null" << loutEND;
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }

                if ( ! delete_reply )
                {
                    loutE << "delete reply is null" << loutEND;
                    free(protocols_reply);
                    free(delete_reply);
                    return;
                }

                int i = 0;
                do {
                    send_event( KILL_WINDOW, (uint32_t[3]){ 32, protocols_reply->atom, delete_reply->atom });
                    FLUSH_XWin();

                    if ( ! is_mapped() )
                    {
                        loutIWin << "is_mapped = false" << '\n';
                        break;
                    }

                } while ( ++i < 3 );

                free( delete_reply );
                free( protocols_reply );

                if ( is_mapped() )
                {
                    loutEWin << "Failed to kill window by asking nicely iters" << i << loutEND;
                    return;
                }

                signal_manager->_window_signals.remove( w );
                signal_manager->_window_client_map.remove( w );
                /* for ( int i = 0; i < _ev_id_vec.size(); ++i )
                {
                    event_handler->removeEventCallback( _ev_id_vec[i].first, _ev_id_vec[i].second );
                } */

            }
            void kill_test() {
                uint32_t w = this->_window;

                xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(conn, 1, slen("WM_PROTOCOLS"), "WM_PROTOCOLS");
                xcb_intern_atom_reply_t *protocols_reply = xcb_intern_atom_reply(conn, protocols_cookie, nullptr);

                xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
                xcb_intern_atom_reply_t *delete_reply = xcb_intern_atom_reply(conn, delete_cookie, nullptr);

                if (protocols_reply == nullptr) {
                    loutE << "protocols reply is null" << loutEND;
                    free(protocols_reply);
                    free(delete_reply);
                    return;

                }

                if (delete_reply == nullptr) {
                    loutE << "delete reply is null" << loutEND;
                    free(protocols_reply);
                    free(delete_reply);
                    return;

                }

                send_event(KILL_WINDOW, (uint32_t[]){32, protocols_reply->atom, delete_reply->atom});
                
                free(protocols_reply);
                free(delete_reply);

                signal_manager->_window_signals.remove(w);
                signal_manager->_window_client_map.remove(w);

            }    
            void clear() {
                VOID_COOKIE = xcb_clear_area(
                    conn, 
                    0,
                    _window,
                    0, 
                    0,
                    _width,
                    _height
                
                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }            
            void focus_input() {
                VOID_COOKIE = xcb_set_input_focus(
                    conn,
                    XCB_INPUT_FOCUS_POINTER_ROOT,
                    _window,
                    XCB_CURRENT_TIME

                );
                CHECK_VOID_COOKIE();
                FLUSH_XWin();
            }
            void send_event(uint32_t __event_mask, void *__value_list = nullptr) {
                if (__event_mask & XCB_EVENT_MASK_EXPOSURE) {
                    xcb_expose_event_t expose_event = {
                        .response_type = XCB_EXPOSE,
                        .window = _window,
                        .x      = 0,                              /* < Top-left x coordinate of the area to be redrawn                 */
                        .y      = 0,                              /* < Top-left y coordinate of the area to be redrawn                 */
                        .width  = static_cast<uint16_t>(_width),  /* < Width of the area to be redrawn                                 */
                        .height = static_cast<uint16_t>(_height), /* < Height of the area to be redrawn                                */
                        .count  = 0                               /* < Number of expose events to follow if this is part of a sequence */
                    };

                    VOID_COOKIE = xcb_send_event(
                        conn,
                        false,
                        _window,
                        XCB_EVENT_MASK_EXPOSURE,
                        (char *)&expose_event
                    );
                    CHECK_VOID_COOKIE();
                    FLUSH_XWin();
                }
                if (__event_mask & XCB_EVENT_MASK_STRUCTURE_NOTIFY) {
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

                    VOID_COOKIE = xcb_send_event(
                        conn,
                        false,
                        _window,
                        XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                        (char *)&event
                    );
                    CHECK_VOID_COOKIE();
                    FLUSH_XWin();
                }
                if (__event_mask & KILL_WINDOW) {
                    uint32_t *value_list = reinterpret_cast<uint32_t *>(__value_list);

                    xcb_client_message_event_t ev;
                    ev.response_type  = XCB_CLIENT_MESSAGE;
                    ev.format         = value_list[0];
                    ev.sequence       = 0;
                    ev.window         = _window;
                    ev.type           = value_list[1];
                    ev.data.data32[0] = value_list[2];
                    ev.data.data32[1] = XCB_CURRENT_TIME;

                    VOID_COOKIE = xcb_send_event(
                        conn,
                        0,
                        _window,
                        XCB_EVENT_MASK_NO_EVENT,
                        (char *)&ev

                    );
                    CHECK_VOID_COOKIE();
                    FLUSH_XWin();
                }
            }
            void update(uint32_t __x, uint32_t __y, uint32_t __width, uint32_t __height) {
                _x      = __x;
                _y      = __y;
                _width  = __width;
                _height = __height;
            }

        /* Signal System */
            template<typename Callback>
            void setup_WIN_SIG(EV __signal_id, Callback &&__callback)
            {
                signal_manager->_window_signals.conect(this->_window, __signal_id,  std::forward<Callback>(__callback));
            }

            void emit_WIN_SIG(EV __signal_id)
            {
                signal_manager->_window_signals.emit(this->_window, __signal_id);
            }

        /* Experimental  */
            // template<typename Type>
            // void setValue(Type&& value)
            // {
            //     _storedValue = forward<T>(value);
            // }

            // template<typename Type>
            // Type getValue() const
            // {
            //     try
            //     {
            //         return any_cast<Type>(_storedValue);
            //     }
            //     catch (const bad_any_cast& e)
            //     {
            //         loutE << "Bad any_cast: " << e.what() << loutEND;
            //         throw;
            //     }
            // }

            // template<typename Func>
            // void setAction(Func&& func)
            // {
            //     // Ensure the function is wrapped in a std::function with a known signature.
            //     _action = function<void()>(forward<Func>(func));
            // }

            // void triggerAction()
            // {
            //     if (_action.has_value())
            //     {
            //         try
            //         {
            //             // We need to cast back to std::function<void()> before calling.
            //             any_cast<function<void()>>(_action)();
            //         }
            //         catch (const bad_any_cast& e)
            //         {
            //             loutE << "Failed to invoke action. " << e.what() << loutEND;
            //         }
            //     }
            // }

        /* Check         */
            bool check_atom(xcb_atom_t __atom) {
                xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(ewmh, _window);
                xcb_ewmh_get_atoms_reply_t reply;
                if (xcb_ewmh_get_wm_state_reply(ewmh, cookie, &reply, NULL)) {
                    for (unsigned int i = 0; i < reply.atoms_len; ++i) {
                        if (reply.atoms[i] == __atom) {
                            xcb_ewmh_get_atoms_reply_wipe(&reply);
                            return true;
                        } /* 'true' if the specified atom is found */
                    }
                    xcb_ewmh_get_atoms_reply_wipe(&reply); // Clean up
                }
                return false; /* The atom was not found or the property could not be retrieved */
            }
            bool check_frameless_window_hint() {
                bool is_frameless = false;
                xcb_atom_t property;
                get_atom((char *)"_MOTIF_WM_HINTS", &property);
                
                xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, _window, property, XCB_ATOM_ANY, 0, sizeof(motif_wm_hints) / 4);
                xcb_get_property_reply_t* reply = xcb_get_property_reply(conn, cookie, NULL);
                if (reply) {
                    if (reply->type != XCB_NONE && reply->format == 32 && reply->length >= 5) {
                        motif_wm_hints* hints = (motif_wm_hints*)xcb_get_property_value(reply);
                        if (hints->decorations == 0) {
                            is_frameless = true;
                        }
                    }
                    else {
                        loutEWin << "No _MOTIF_WM_HINTS property found." << loutEND;
                    }
                    free(reply);
                }
                return is_frameless;
            
            } /* Function to fetch and check the _MOTIF_WM_HINTS property */
            bool is_EWMH_fullscreen() {
                xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(ewmh, _window);
                xcb_ewmh_get_atoms_reply_t wm_state;
                if (xcb_ewmh_get_wm_state_reply(ewmh, cookie, &wm_state, NULL) == 1) {
                    for (unsigned int i = 0; i < wm_state.atoms_len; i++) {
                        if (wm_state.atoms[i] == ewmh->_NET_WM_STATE_FULLSCREEN) {
                            xcb_ewmh_get_atoms_reply_wipe(&wm_state);
                            return true;

                        }

                    } xcb_ewmh_get_atoms_reply_wipe(&wm_state);

                } return false;

            }
            bool is_active_EWMH_window() {
                uint32_t active_window = 0;
                uint8_t err = xcb_ewmh_get_active_window_reply(
                    ewmh,
                    xcb_ewmh_get_active_window(ewmh, 0),
                    &active_window,
                    nullptr

                ); if (err != 1) loutE << "xcb_ewmh_get_active_window_reply failed" << loutEND;
                return (_window == active_window);

            }
            uint32_t check_event_mask_sum() {
                xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(conn, _window);
                xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, cookie, nullptr);
                uint32_t mask = (reply == nullptr) ? 0 : reply->all_event_masks;
                if (mask == 0) loutE << "Error retriving window attributes" << loutEND;
                free(reply);
                return mask;

            }
            vector<xcb_event_mask_t> check_event_mask_codes() {
                uint32_t maskSum = check_event_mask_sum();
                vector<xcb_event_mask_t>(setMasks);
                for (int mask = XCB_EVENT_MASK_KEY_PRESS; mask <= XCB_EVENT_MASK_OWNER_GRAB_BUTTON; mask <<= 1) {
                    if (maskSum & mask) {
                        setMasks.push_back(static_cast<xcb_event_mask_t>(mask));

                    }

                } return setMasks;
                
            }
            bool is_mask_active(const uint32_t & event_mask) {
                vector<xcb_event_mask_t>(masks) = check_event_mask_codes();
                for (const auto &ev_mask : masks) {
                    if (ev_mask == event_mask) {
                        return true;

                    }

                } return false;

            }
            bool is_mapped()
            {
                xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes( conn, _window );
                xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply( conn, cookie, nullptr );
                if ( ! reply )
                {
                    loutEWin << "Unable to get window attributes" << '\n';
                    return false;
                }

                bool isMapped = ( reply->map_state == XCB_MAP_STATE_VIEWABLE );
                free(reply);

                return isMapped;
            }
            bool should_be_decorated() {
                // Atom for the _MOTIF_WM_HINTS property, used to control window decorations
                const char* MOTIF_WM_HINTS = "_MOTIF_WM_HINTS";
                
                // Get the atom for the _MOTIF_WM_HINTS property
                xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(MOTIF_WM_HINTS), MOTIF_WM_HINTS);
                xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(conn, cookie, NULL);
                if (!reply) {
                    log_error("reply = nullptr");
                    return true; // Default to decorating if we can't check

                } xcb_atom_t motif_wm_hints_atom = reply->atom; free(reply);

                // Try to get the _MOTIF_WM_HINTS property from the window
                xcb_get_property_cookie_t prop_cookie = xcb_get_property(conn, 0, _window, motif_wm_hints_atom, XCB_ATOM_ANY, 0, sizeof(uint32_t) * 5);
                xcb_get_property_reply_t* prop_reply = xcb_get_property_reply(conn, prop_cookie, NULL);

                if (prop_reply && xcb_get_property_value_length(prop_reply) >= sizeof(uint32_t) * 5) {
                    uint32_t* hints = (uint32_t*)xcb_get_property_value(prop_reply);
                    
                    // The second uint32_t in the hints indicates decoration status
                    // flags are in the second uint32_t of the property
                    bool decorate = !(hints[1] & (1 << 1)); // Check if decorations are disabled

                    free(prop_reply);
                    return decorate;

                } if (prop_reply) free(prop_reply);

                return true; // Default to decorating if we can't find or interpret the hints

            }
        
        /* Set           */
            void set_active_EWMH_window() {
                VOID_COOKIE = xcb_ewmh_set_active_window(
                    ewmh,
                    0,
                    _window
                );
                CHECK_VOID_COOKIE();
                FLUSH_X();
            }
            void set_EWMH_fullscreen_state() {
                VOID_COOKIE = xcb_change_property(
                    conn,
                    XCB_PROP_MODE_REPLACE,
                    _window,
                    ewmh->_NET_WM_STATE,
                    XCB_ATOM_ATOM,
                    32,
                    1,
                    &ewmh->_NET_WM_STATE_FULLSCREEN
                );
                CHECK_VOID_COOKIE();
                FLUSH_XWin();
        
            }

        /* Unset         */
            void unset_EWMH_fullscreen_state() {
                VOID_COOKIE = xcb_change_property(
                    conn,
                    XCB_PROP_MODE_REPLACE,
                    _window,
                    ewmh->_NET_WM_STATE, 
                    XCB_ATOM_ATOM,
                    32,
                    0,
                    0

                ); CHECK_VOID_COOKIE();
                FLUSH_X();

            }
        
        /* Get           */
            /* ewmh  */
                vector<uint32_t> get_window_icon(uint32_t *__width = nullptr, uint32_t *__height = nullptr) {
                    xcb_intern_atom_cookie_t* ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
                    if (!xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, nullptr)) {
                        loutEWin << "Failed to initialize EWMH atoms" << '\n';
                        free(ewmh_cookie);
                        return {};

                    }

                    // Request the _NET_WM_ICON property
                    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, _window, ewmh->_NET_WM_ICON, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT32_MAX);
                    xcb_get_property_reply_t* reply = xcb_get_property_reply(conn, cookie, nullptr);
                    
                    if (reply == nullptr) {
                        loutE << "reply = nullptr" << '\n';
                        free(reply);
                        return {};

                    }

                    vector<uint32_t> icon_data;
                    if (reply && reply->value_len > 0) {
                        // The value is an array of 32-bit items, so cast the reply's value to a uint32_t pointer
                        uint32_t* data = (uint32_t*)xcb_get_property_value(reply);
                        uint32_t len = xcb_get_property_value_length(reply) / sizeof(uint32_t);

                        // Parse icon data (width, height, and ARGB data)
                        for (uint32_t i = 0; i < len;) {
                            uint32_t width = data[i++]; if (__width == nullptr) {
                                loutEWin << "icon data is invalid" << loutEND;
                                return {};
                            
                            } *__width = width;

                            uint32_t height = data[i++]; if (__height == nullptr) {
                                loutEWin << "icon data is invalid" << loutEND;
                                return {};
                                
                            } *__height = height;

                            uint32_t size = width * height;

                            loutIWin << "width"  << width  << loutEND;
                            loutIWin << "height" << height << loutEND;
                            loutIWin << "size"   << size   << loutEND;

                            if (i + size > len) break; // Sanity check

                            // Assuming we want the first icon for simplicity
                            icon_data.assign(data + i, data + i + size);
                            break; // This example only processes the first icon

                        }

                    }

                    free(reply);
                    return icon_data;
                }
                vector<uint32_t> get_best_quality_window_icon(uint32_t *__width = nullptr, uint32_t *__height = nullptr)
                {
                    xcb_intern_atom_cookie_t* ewmh_cookie = xcb_ewmh_init_atoms(conn, ewmh);
                    if (!xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookie, nullptr))
                    {
                        loutE << "Failed to initialize EWMH atoms" << '\n';
                        free(ewmh_cookie);
                        return {};
                    }

                    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, _window, ewmh->_NET_WM_ICON, XCB_ATOM_CARDINAL, 0, UINT32_MAX);
                    xcb_get_property_reply_t* reply = xcb_get_property_reply(conn, cookie, nullptr);

                    vector<uint32_t> best_icon_data;
                    uint32_t best_size = 0;
                    uint32_t best_width = 0, best_height = 0;

                    if (reply && reply->value_len > 0)
                    {
                        uint32_t* data = (uint32_t*)xcb_get_property_value(reply);
                        uint32_t len = xcb_get_property_value_length(reply) / sizeof(uint32_t);

                        for (uint32_t i = 0; i < len; )
                        {
                            uint32_t width = data[i++];
                            uint32_t height = data[i++];
                            uint32_t size = width * height;

                            if (size > best_size)
                            {
                                best_size = size;
                                best_width = width;
                                best_height = height;
                                best_icon_data.assign(data + i, data + i + size);
                            }

                            i += size; // Move to the next icon in the data
                        }
                    }

                    if (__width  != nullptr) *__width  = best_width;
                    if (__height != nullptr) *__height = best_height;

                    free(reply);
                    return best_icon_data;
                }

                vector<uint32_t> invert_color_vec(vector<uint32_t> __icon_data)
                {
                    vector<uint32_t> icon_data = __icon_data;
                    for (auto& pixel : icon_data)
                    {
                        uint8_t* colors = reinterpret_cast<uint8_t*>(&pixel);
                        colors[0] = 255 - colors[0]; // R
                        colors[1] = 255 - colors[1]; // G
                        colors[2] = 255 - colors[2]; // B
                        // Alpha (colors[3]) remains unchanged
                    }

                    return icon_data;
                }

                void make_png_from_icon()
                {
                    if (fs::exists(PNG_HASH(get_icccm_class())))
                    {
                        return;
                    }

                    uint32_t width, height;
                    vector<uint32_t> vec = get_best_quality_window_icon(&width, &height);

                    __color_bitmap__ color_bitmap(width, height, vec);
                    if (!fs::exists(ICON_FOLDER_HASH(get_icccm_class())))
                    {
                        file_system->create(ICON_FOLDER_HASH(get_icccm_class()));
                    }

                    if (file_system->check_status())
                    {
                        color_bitmap.exportToPng(PNG_HASH(get_icccm_class()).c_str());
                    }
                }

            /* icccm */
                void get_override_redirect() {
                    xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(conn, xcb_get_window_attributes(conn, _window), NULL);
                    if (wa == nullptr) {
                        loutEWin << "wa == nullptr" << loutEND;
                        free(wa);
                        return;

                    } _override_redirect = wa->override_redirect; free(wa);
                    if (_override_redirect == true) loutIWin << "override_redirect = true" << loutEND;

                }
                void get_min_window_size_hints() {
                    xcb_size_hints_t hints;
                    xcb_icccm_get_wm_normal_hints_reply(conn, xcb_icccm_get_wm_normal_hints(conn, _window), &hints, NULL);

                    if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
                        _min_width  = hints.min_width;
                        _min_height = hints.min_height;
                        
                        loutI << "min_width"  << hints.min_width  << loutEND;
                        loutI << "min_height" << hints.min_height << loutEND;

                    }

                }
                void get_window_size_hints() {
                    xcb_size_hints_t hints;
                    memset(&hints, 0, sizeof(xcb_size_hints_t)); // Initialize hints structure

                    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_normal_hints(conn, _window);
                    xcb_generic_error_t* error = NULL; /* To capture any error */
                    bool result = xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, &error);

                    if (!result || error) {
                        if (error) {
                            loutEWin << "Error retrieving window hints" << '\n';
                            free(error);

                        } return;

                    } /* Now, check and use the hints as needed */

                    if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
                        _min_width  = hints.min_width;
                        _min_height = hints.min_height;

                        loutIWin << "min_width:"  << hints.min_width  << '\n';
                        loutIWin << "min_height:" << hints.min_height << '\n';
                        loutIWin << "width:"      << hints.width      << '\n';
                        loutIWin << "height:"     << hints.height     << '\n';
                        loutIWin << "x:"          << hints.x          << '\n';
                        loutIWin << "y:"          << hints.y          << '\n';

                    } else {
                        loutEWin << "No minimum size hints available" << '\n';

                    }

                }
                void set_client_size_as_hints(int16_t *__x, int16_t *__y, uint16_t *__width, uint16_t *__height) {
                    xcb_size_hints_t hints;
                    memset(&hints, 0, sizeof(xcb_size_hints_t)); // Initialize hints structure

                    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_normal_hints(conn, _window);
                    xcb_generic_error_t* error = NULL; // To capture any error
                    bool result = xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, &error);

                    if (!result || error) {
                        if (error) {
                            loutEWin << "Error retrieving window hints" << loutEND;
                            free(error);

                        } return;

                    } /* Now, check and use the hints as needed */

                    if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
                        _min_width  = hints.min_width;
                        _min_height = hints.min_height;
                        *__x        = hints.x;
                        *__y        = hints.y;
                        *__width    = hints.width;
                        *__height   = hints.height;

                        if (hints.width < hints.min_width) {
                            loutE << "something went wrong hints.width" << hints.width << " is less then hints.min_width" << hints.min_width << '\n';

                        }
                        if (hints.height < hints.min_height) {
                            loutE << "something went wrong hints.height" << hints.height << " is less then hints.min_height" << hints.min_height << '\n';

                        }

                    } else {
                        loutEWin << "Could not get size hints" << '\n';

                    }

                }
                string get_window_icon_name() {
                    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_icon_name(conn, _window);
                    xcb_icccm_get_text_property_reply_t reply; string icon_name;

                    if (xcb_icccm_get_wm_icon_name_reply(conn, cookie, &reply, NULL)) {
                        icon_name = string(reply.name, reply.name_len);
                        xcb_icccm_get_text_property_reply_wipe(&reply);
                        loutIWin << "icon_name: " << icon_name << loutEND;

                    } else {
                        loutEWin << "Failed to retrieve the window icon name" << loutEND;

                    } return icon_name;

                } /** @brief @return the icon name of a window */

            uint32_t get_transient_for_window() {
                uint32_t t_for = 0; // Default to 0 (no parent)
                xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, _window, XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 0, sizeof(uint32_t));
                xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);

                if (reply && xcb_get_property_value_length(reply) == sizeof(uint32_t)) {
                    t_for = *(uint32_t *)xcb_get_property_value(reply);
                    free(reply); /* Remember to free the reply */

                } return t_for;

            }
            void print_window_states() {
                xcb_intern_atom_reply_t *atom_r = XCB::atom_r(XCB::atom_cok("_NET_WM_STATE"));
                if (atom_r == nullptr) {
                    loutEWin << "Failed to get _NET_WM_STATE atom." << loutEND;
                    return;

                } xcb_atom_t atom = atom_r->atom; free(atom_r);

                xcb_get_property_cookie_t property_cookie = xcb_get_property(conn, 0, _window, atom, XCB_ATOM_ATOM, 0, 1024);
                xcb_get_property_reply_t* property_reply = xcb_get_property_reply(conn, property_cookie, NULL);
                if (property_reply == nullptr) {
                    loutEWin << "Failed to get property." << loutEND;
                    return;

                } xcb_atom_t* atoms = static_cast<xcb_atom_t*>(xcb_get_property_value(property_reply));
                int atom_count = xcb_get_property_value_length(property_reply) / sizeof(xcb_atom_t);

                for (int i = 0; i < atom_count; ++i) {
                    xcb_get_atom_name_cookie_t atom_name_cookie = xcb_get_atom_name(conn, atoms[i]);
                    xcb_get_atom_name_reply_t* atom_name_reply = xcb_get_atom_name_reply(conn, atom_name_cookie, NULL);

                    if (atom_name_reply) {
                        int name_len = xcb_get_atom_name_name_length(atom_name_reply);
                        char* name = xcb_get_atom_name_name(atom_name_reply);
                        loutIWin << "Window   State: " << string(name, name_len) << loutEND;
                        free(atom_name_reply);

                    }

                } free(property_reply);

            }
            uint32_t get_pid() {
                xcb_atom_t property = XCB_ATOM_NONE;
                xcb_atom_t type = XCB_ATOM_CARDINAL;
                xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, 11, "_NET_WM_PID");
                xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(conn, cookie, NULL);
                if (reply) {
                    property = reply->atom;
                    free(reply);
                
                }
                if (property == XCB_ATOM_NONE) {
                    loutEWin << "Unable to find _NET_WM_PID atom." << '\n';
                    return 0;

                }

                xcb_get_property_cookie_t prop_cookie = xcb_get_property(conn, 0, _window, property, type, 0, sizeof(uint32_t));
                xcb_get_property_reply_t* prop_reply = xcb_get_property_reply(conn, prop_cookie, NULL);
                if (!prop_reply) {
                    loutEWin << "Unable to get window property." << '\n';
                    return 0;

                }
                if (xcb_get_property_value_length(prop_reply) == 0) {
                    free(prop_reply); 
                    loutEWin << "The window does not have the _NET_WM_PID property." << '\n';
                    return 0;

                } uint32_t pid = *(uint32_t*)xcb_get_property_value(prop_reply); free(prop_reply);

                _pid = pid;
                return pid;

            }
            pid_t pid() const {
                return _pid;

            }
            string get_window_property(xcb_atom_t __atom) {
                xcb_get_property_cookie_t cookie = xcb_get_property(ewmh->connection, 0, _window, __atom, XCB_GET_PROPERTY_TYPE_ANY, 0, 1024);
                xcb_get_property_reply_t* reply = xcb_get_property_reply(ewmh->connection, cookie, NULL);
                string propertyValue = "";

                if (reply && (reply->type == XCB_ATOM_STRING || reply->type == ewmh->UTF8_STRING)) {
                    // Assuming the property is a UTF-8 string, but you might check and convert depending on the actual type
                    char* value = (char*)xcb_get_property_value(reply);
                    int len = xcb_get_property_value_length(reply);

                    if (value && len > 0) {
                        propertyValue.assign(value, len);
                    
                    }

                } free(reply);
                return propertyValue;
            
            }
            string get_net_wm_name_by_req() {
                xcb_ewmh_get_utf8_strings_reply_t wm_name;
                string windowName = "";

                if (xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, _window), &wm_name, NULL)) {
                    windowName.assign(wm_name.strings, wm_name.strings_len);
                    xcb_ewmh_get_utf8_strings_reply_wipe(&wm_name);

                } _name = windowName;
                return windowName;

            }
            string get_net_wm_name() const {
                return _name;

            }
            void print_wm_class(xcb_connection_t* conn, xcb_window_t window) {
                xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, XCB_ATOM_WM_CLASS, XCB_GET_PROPERTY_TYPE_ANY, 0, 1024);
                xcb_get_property_reply_t* reply = xcb_get_property_reply(conn, cookie, NULL);

                if (reply && xcb_get_property_value_length(reply) > 0) {
                    // WM_CLASS property value is a null-separated string "instance\0class\0"
                    char* value = (char *)xcb_get_property_value(reply);
                    printf("WM_CLASS: %s\n", value); // Prints the instance name
                    printf("WM_CLASS: %s\n", value + strlen(value) + 1); // Prints the class name

                } free(reply);

            }
            void print_icccm_wm_class() {
                xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(conn, _window); /* Request WM_CLASS property */
                xcb_icccm_get_wm_class_reply_t wm_class_reply;

                if (xcb_icccm_get_wm_class_reply(conn, cookie, &wm_class_reply, NULL)) {
                    loutIWin << "Instance Name: " << wm_class_reply.instance_name << '\n';
                    loutIWin << "Class Name: "    << wm_class_reply.class_name    << '\n';
                    xcb_icccm_get_wm_class_reply_wipe(&wm_class_reply);

                } /* Retrieve the WM_CLASS property */
                else {
                    loutEWin << "Failed to retrieve WM_CLASS for window" << '\n';

                }

            }
            string get_icccm_class() {
                xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(conn, _window);
                xcb_icccm_get_wm_class_reply_t wm_class_reply;
                string result = "";

                if (xcb_icccm_get_wm_class_reply(conn, cookie, &wm_class_reply, NULL)) {
                    result = string(wm_class_reply.class_name);
                    xcb_icccm_get_wm_class_reply_wipe(&wm_class_reply);
                
                } /* Retrieve the WM_CLASS property */
                else {
                    loutEWin << "Failed to retrieve WM_CLASS for window" << '\n';
                
                } return result;
            
            }
            char * property(const char *atom_name) {
                unsigned int reply_len;
                char * propertyValue;

                xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, xcb_get_property(
                    conn,
                    false,
                    _window,
                    atom(
                        atom_name

                    ),
                    XCB_GET_PROPERTY_TYPE_ANY,
                    0,
                    60
                
                ), nullptr);

                if (reply == nullptr || xcb_get_property_value_length(reply) == 0) {
                    if (reply != nullptr) {
                        loutE << "reply length for property(" << string(atom_name) << ") = 0" << loutEND;
                        free(reply);
                        return (char *) "";

                    } loutE << "reply == nullptr" << loutEND;
                    return (char *) "";

                } reply_len = xcb_get_property_value_length(reply);
                
                propertyValue = static_cast<char *>(malloc(sizeof(char) * (reply_len + 1)));
                memcpy(propertyValue, xcb_get_property_value(reply), reply_len);
                propertyValue[reply_len] = '\0';

                if (reply) {
                    free(reply);
                    loutI << "property(" << string(atom_name) << ") = " << string(propertyValue) << loutEND;
                
                } return propertyValue;

            }
            xcb_rectangle_t get_xcb_rectangle_t_by_req() {
                xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, _window), nullptr);
                if (g == nullptr) return (xcb_rectangle_t){0, 0, 0, 0};
                xcb_rectangle_t rect = (xcb_rectangle_t) {g->x, g->y, g->width, g->height};
                free(g);

                return rect;

            }
            xcb_rectangle_t get_xcb_rectangle_t() const {
                return (xcb_rectangle_t) {_x, _y, _width, _height};
            
            }
            uint32_t root_window() {
                xcb_query_tree_cookie_t cok = xcb_query_tree(conn, _window);
                xcb_query_tree_reply_t *r = xcb_query_tree_reply(conn, cok, NULL);
                uint32_t w = 0;
                if (r == nullptr) {
                    loutE << "Unable to query the window tree" << loutEND;

                } w = r->root; free(r); return w;

            }
            uint32_t parent_by_req() {
                uint32_t p = 0;
                xcb_query_tree_cookie_t cok = xcb_query_tree(conn, _window);
                xcb_query_tree_reply_t *r = xcb_query_tree_reply(conn, cok, nullptr);

                if (r == nullptr) {
                    loutE << "Unable to query the window tree" << loutEND;

                } p = r->parent;

                if (_parent != p) {
                    loutE << "internal parent: " <<  _parent << " does not match 'retrived_parent': " << p << loutEND;
                    _parent = p;

                } free(r);

                return p;
                
            }
            uint32_t parent() const {
                return _parent;
            
            }
            uint32_t *children(uint32_t *child_count) {
                *child_count = 0;
                xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, _window);
                xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, nullptr);

                if (reply == nullptr) {
                    log_error("Unable to query the window tree.");
                    return nullptr;

                }

                *child_count = xcb_query_tree_children_length(reply);
                uint32_t *children = static_cast<uint32_t *>(malloc(*child_count *sizeof(xcb_window_t)));

                if (children == nullptr) {
                    log_error("Unable to allocate memory for children.");
                    free(reply);
                    return nullptr;

                } memcpy(children, xcb_query_tree_children(reply), * child_count * sizeof(uint32_t));
                
                free(reply);
                return children;

            }
            uint32_t get_min_width() const {
                return _min_width;

            }
            uint32_t get_min_height() const {
                return _min_height;

            }        
            int16_t x_from_req() {
                xcb_get_geometry_cookie_t g_cok = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(conn, g_cok, nullptr);
                
                int16_t x = 200; if (g == nullptr) {
                    loutE << "xcb_get_geometry_reply = nullptr" << loutEND;
                    
                } x = g->x; free(g); return x;

            }
            int16_t y_from_req() {
                xcb_get_geometry_cookie_t g_cok = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t * g = xcb_get_geometry_reply(conn, g_cok, nullptr);

                int16_t y = 200; if (g == nullptr) {
                    loutE << "xcb_get_geometry_reply = nullptr" << loutEND;
                    
                } y = g->y; free(g); return y;

            }
            uint16_t width_from_req() {
                xcb_get_geometry_cookie_t g_cok = xcb_get_geometry(conn, _window);
                xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(conn, g_cok, nullptr);

                uint16_t width = 200; if (g == nullptr) {
                    loutEWin << "xcb_get_geometry_reply_t = nullptr" << loutEND;

                } width = g->width; free(g); return width;

            }
            uint16_t height_from_req() {
                xcb_get_geometry_cookie_t g_cok = XCB::g_cok(_window);
                xcb_get_geometry_reply_t *g_r = XCB::g_r(g_cok);

                uint16_t height = 200; if (g_r == nullptr) {
                    loutEWin << "reply == nullptr" << loutEND;

                } height = g_r->height; free(g_r); return height;
            }
        
        /* Configuration */
            void apply_event_mask(const vector<uint32_t> &values) {
                if (values.empty()) { 
                    log_error("values vector is empty");
                    return;

                }
                VOID_COOKIE = xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    values.data()

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            void set_event_mask(uint32_t __mask) {
                apply_event_mask(&__mask);

            }
            void apply_event_mask(const uint32_t *__mask) {
                VOID_COOKIE = xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_EVENT_MASK,
                    __mask

                ); CHECK_VOID_COOKIE();
                FLUSH_X();

            }
            void set_pointer(CURSOR cursor_type) {
                xcb_cursor_context_t *ctx;
                if (xcb_cursor_context_new(conn, screen, &ctx) < 0) {
                    loutEWin << "Unable to create cursor context" << '\n';
                    return;

                }
                xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, pointer_from_enum(cursor_type));
                if (!cursor) {
                    loutEWin << "Unable to load cursor" << '\n';
                    xcb_cursor_context_free(ctx);
                    xcb_free_cursor(conn, cursor);
                    return;

                }
                VOID_COOKIE = xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_CURSOR,
                    (uint32_t[1]) {
                        cursor

                    }

                ); CHECK_VOID_COOKIE();
                FLUSH_X();

                xcb_cursor_context_free(ctx);
                xcb_free_cursor(conn, cursor);

            }

            /* Size_pos  */
                /* Fetch */
                    int16_t x() const {
                        return _x;

                    }                
                    int16_t y() const {
                        return _y;

                    }                
                    uint16_t width() const {
                        return _width;

                    }                
                    uint16_t height() const {
                        return _height;

                    }

                void configure(uint16_t __mask, const void *__value_list) {
                    xcb_configure_window(conn, _window, __mask, __value_list);
                    xcb_flush(conn);

                } 
                void x(uint32_t x) {
                    config_window(XCB_CONFIG_WINDOW_X, (uint32_t[1]){x});
                    update(x, _y, _width, _height);

                }
                void y(uint32_t y) {
                    config_window(XCB_CONFIG_WINDOW_Y, (uint32_t[1]){y});
                    update(_x, y, _width, _height);

                }
                void width(uint32_t width) {
                    config_window(XCB_CONFIG_WINDOW_WIDTH, (uint32_t[1]){width});
                    update(_x, _y, width, _height);

                }
                void height(uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[1]){height});
                    update(_x, _y, _width, height);

                }
                void x_y(uint32_t x, uint32_t y) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, (uint32_t[2]){x, y});
                    update(x, y, _width, _height);

                }
                void width_height(uint32_t width, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[4]){width, height});
                    update(_x, _y, width, height);

                }
                void x_y_width_height(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[4]){x, y, width, height});
                    update(x, y, width, height);

                }
                void x_width_height(uint32_t x, uint32_t width, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[3]){x, width, height});
                    update(x, _y, width, height);

                }
                void y_width_height(uint32_t y, uint32_t width, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[4]){y, width, height});
                    update(_x, y, width, height);

                }
                void x_width(uint32_t x, uint32_t width) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[2]){x, width});
                    update(x, _y, width, _height);

                }
                void x_height(uint32_t x, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[2]){x, height});
                    update(x, _y, _width, height);

                }
                void y_width(uint32_t y, uint32_t width) {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[2]){y, width});
                    update(_x, y, width, _height);

                }
                void y_height(uint32_t y, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[2]){y, height});
                    update(_x, y, _width, height);

                }
                void x_y_width(uint32_t x, uint32_t y, uint32_t width) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[3]){x, y, width});
                    update(x, y, width, _height);

                }
                void x_y_height(uint32_t x, uint32_t y, uint32_t height) {
                    config_window(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[3]){x, y, height});
                    update(x, y, _width, height);

                }

        /* Backround     */
            void set_backround_color(int __color) {
                _color = __color;
                change_back_pixel(get_color(__color));

            }
            void change_backround_color(int __color) {
                set_backround_color(__color);
                clear();
                xcb_flush(conn);
                send_event(XCB_EVENT_MASK_EXPOSURE);

            }
            void set_backround_color_8_bit(const uint8_t &red_value, const uint8_t &green_value, const uint8_t &blue_value) {
                change_back_pixel(get_color(red_value, green_value, blue_value));

            }
            void set_backround_color_16_bit(const uint16_t & red_value, const uint16_t & green_value, const uint16_t & blue_value) {
                change_back_pixel(get_color(red_value, green_value, blue_value));

            }
            void set_backround_png(const char * imagePath) {
                Imlib_Image image = imlib_load_image(imagePath);
                if (!image) {
                    loutE << "Failed to load image: " << imagePath << endl;
                    return;
                }

                imlib_context_set_image(image);
                
                int originalWidth  = imlib_image_get_width();
                int originalHeight = imlib_image_get_height();

                // Calculate new size maintaining aspect ratio
                double aspectRatio = (double)originalWidth / originalHeight;
                int      newHeight = _height;
                int       newWidth =  (int)(newHeight * aspectRatio);

                if (newWidth > _width) {
                    newWidth = _width;
                    newHeight = (int)(newWidth / aspectRatio);

                } Imlib_Image scaledImage = imlib_create_cropped_scaled_image(
                    0, 
                    0, 
                    originalWidth, 
                    originalHeight, 
                    newWidth, 
                    newHeight

                ); imlib_free_image();/* Free original image */

                imlib_context_set_image(scaledImage);
                DATA32 *data = imlib_image_get_data();/* Get the scaled image data */

                xcb_image_t *xcb_image = xcb_image_create_native( 
                    conn, 
                    newWidth, 
                    newHeight,
                    XCB_IMAGE_FORMAT_Z_PIXMAP, 
                    screen->root_depth, 
                    NULL, 
                    ~0, (uint8_t*)data

                );/* Create an XCB image from the scaled data */
                create_pixmap();
                create_graphics_exposure_gc();
                xcb_rectangle_t rect = {0, 0, _width, _height};
                xcb_poly_fill_rectangle(
                    conn, 
                    pixmap, 
                    _gc, 
                    1, 
                    &rect

                );/* Init x and y */
                int x(0), y(0);
                if (newWidth != _width) {
                    x = (_width - newWidth) / 2;

                }/* Calculate position to center the image */
                if (newHeight != _height) {
                    y = (_height - newHeight) / 2;

                }/* Put the scaled image onto the pixmap at the calculated position */
                VOID_COOKIE = xcb_image_put(
                    conn, 
                    pixmap, 
                    _gc,
                    xcb_image, 
                    x,
                    y, 
                    0

                ); CHECK_VOID_COOKIE();
                /* Set the pixmap as the background of the window */
                VOID_cookie = xcb_change_window_attributes( 
                    conn,
                    _window,
                    XCB_CW_BACK_PIXMAP,
                    &pixmap

                ); CHECK_VOID_COOKIE();

                // Cleanup

                VOID_cookie = xcb_free_gc(conn, _gc);/* Free the GC */CHECK_VOID_COOKIE();
                xcb_image_destroy(xcb_image);
                imlib_free_image(); // Free scaled image
                clear_window();

            }
            void set_backround_png(const string &__imagePath) {
                set_backround_png(__imagePath.c_str());
                // Imlib_Image image = imlib_load_image(__imagePath.c_str());
                // if (!image)
                // {
                //     loutE << "Failed to load image: " << __imagePath << endl;
                //     return;
                // }

                // imlib_context_set_image(image);
                // int originalWidth = imlib_image_get_width();
                // int originalHeight = imlib_image_get_height();

                // // Calculate new size maintaining aspect ratio
                // double aspectRatio = (double)originalWidth / originalHeight;
                // int newHeight = _height;
                // int newWidth = (int)(newHeight * aspectRatio);

                // if (newWidth > _width)
                // {
                //     newWidth = _width;
                //     newHeight = (int)(newWidth / aspectRatio);
                // }

                // Imlib_Image scaledImage = imlib_create_cropped_scaled_image(
                //     0, 
                //     0, 
                //     originalWidth, 
                //     originalHeight, 
                //     newWidth, 
                //     newHeight
                // );
                // imlib_free_image(); // Free original image
                // imlib_context_set_image(scaledImage);
                // DATA32 *data = imlib_image_get_data(); // Get the scaled image data
                
                // // Create an XCB image from the scaled data
                // xcb_image_t *xcb_image = xcb_image_create_native( 
                //     conn, 
                //     newWidth, 
                //     newHeight,
                //     XCB_IMAGE_FORMAT_Z_PIXMAP, 
                //     screen->root_depth, 
                //     NULL, 
                //     ~0, (uint8_t*)data
                // );

                // create_pixmap();
                // create_graphics_exposure_gc();
                // xcb_rectangle_t rect = {0, 0, _width, _height};
                // xcb_poly_fill_rectangle(
                //     conn, 
                //     pixmap, 
                //     _gc, 
                //     1, 
                //     &rect
                // );

                // // Calculate position to center the image
                // int x(0), y(0);
                // if (newWidth != _width)
                // {
                //     x = (_width - newWidth) / 2;
                // }
                // if (newHeight != _height)
                // {
                //     y = (_height - newHeight) / 2;
                // }
                
                // xcb_image_put( // Put the scaled image onto the pixmap at the calculated position
                //     conn, 
                //     pixmap, 
                //     _gc,
                //     xcb_image, 
                //     x,
                //     y, 
                //     0
                // );

                // xcb_change_window_attributes( // Set the pixmap as the background of the window
                //     conn,
                //     _window,
                //     XCB_CW_BACK_PIXMAP,
                //     &pixmap
                // );

                // // Cleanup
                // xcb_free_gc(conn, _gc); // Free the GC
                // xcb_image_destroy(xcb_image);
                // imlib_free_image(); // Free scaled image

                // clear_window();
            }
            void make_then_set_png(const char * file_name, const std::vector<std::vector<bool>> &bitmap) {
                create_png_from_vector_bitmap(file_name, bitmap);
                set_backround_png(file_name);

            }
            void make_then_set_png(const string &__file_name, const std::vector<std::vector<bool>> &bitmap) {
                create_png_from_vector_bitmap(__file_name.c_str(), bitmap);
                set_backround_png(__file_name);

            }
            void make_then_set_png(const string &__file_name, bool bitmap[20][20]) {
                create_png_from_vector_bitmap(__file_name.c_str(), bitmap);
                set_backround_png(__file_name);

            }
            int get_current_backround_color() const {
                return _color;

            }

        /* Draw          */
            #define AUTO -16
            void draw(const string &__str, int __text_color = WHITE, int __backround_color = AUTO, int16_t __x = AUTO, int16_t __y = AUTO, const char *__font_name = DEFAULT_FONT) {
                get_font(__font_name);
                if (__backround_color == AUTO) __backround_color = _color;
                if (__backround_color == WHITE) __text_color = BLACK;
                create_font_gc(__text_color, __backround_color, font);
                if (__x == AUTO) __x = CENTER_TEXT(_width, __str.length());
                if (__y == AUTO) __y = CENTER_TEXT_Y(_height);
                VOID_COOKIE = xcb_image_text_8(
                    conn,
                    strlen(__str.c_str()),
                    _window,
                    font_gc,
                    __x,
                    __y,
                    __str.c_str()
                );
                FLUSH_XWin();
                CHECK_VOID_COOKIE();
            }
            void draw_text(const char *str , const int &text_color, const int &backround_color, const char *font_name, const int16_t &x, const int16_t &y) {
                get_font(font_name);
                create_font_gc(text_color, backround_color, font);
                VOID_COOKIE = xcb_image_text_8(
                    conn, 
                    strlen(str), 
                    _window, 
                    font_gc,
                    x, 
                    y, 
                    str
                );
                FLUSH_XWin();
                CHECK_VOID_COOKIE();
            }
            void draw_text_auto_color(const char *__str, int16_t __x, int16_t __y, int __text_color = WHITE, int __backround_color = 0, const char *__font_name = DEFAULT_FONT) {
                get_font(__font_name);
                if (__backround_color == 0) __backround_color = _color;
                create_font_gc(__text_color, __backround_color, font);
                VOID_COOKIE = xcb_image_text_8(
                    conn,
                    strlen(__str),
                    _window,
                    font_gc,
                    __x,
                    __y,
                    __str
                );
                FLUSH_XWin();
                CHECK_VOID_COOKIE();
            }
            /**
             *
             * @brief Function that draws text on a window auto centering and coloring(FOLLOWS WINDOWS BACKROUND COLOR)
             * 
             * @param __str The string to draw to the window
             * @param __text_color The color of the text to draw
             * NOTE: If backround color is 'WHITE' text is auto switched to 'BLACK'
             * @param __backround_color The color of the text backround, This is auto set to windows backround
             * @param __font_name The name of the font to use auto selects a define that is called 'DEFAULT_FONT' to make switching alot esier  
             *
             * NOTE: THESE ARE MUTEBLE JUST TYPE IN ALL THE PARAMS WHEN MAKING A CALL TO OVERIDE AUTO FUNCTIONALITY
             *       ALSO IF YOU ONLY WANT AUTO COLOR NOT CENTERING USE 'draw_text_auto_color' FUNCTION
             *
             */
            void draw_acc(const string &__str, int __text_color = WHITE, int __backround_color = 0, const char *__font_name = DEFAULT_FONT) {
                get_font(__font_name);
                if (__backround_color == 0) __backround_color = _color;
                if (__backround_color == WHITE) __text_color = BLACK;
                create_font_gc(__text_color, __backround_color, font);
                VOID_COOKIE = xcb_image_text_8(
                    conn,
                    strlen(__str.c_str()),
                    _window,
                    font_gc,
                    CENTER_TEXT(_width, __str.length()),
                    CENTER_TEXT_Y(_height),
                    __str.c_str()
                );
                FLUSH_XWin();
                CHECK_VOID_COOKIE();

            }
            void draw_text_16(const char *str, const int &text_color, const int &background_color, const char *font_name, const int16_t &x, const int16_t &y) {
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
            void draw_text_16_auto_color(const char *__str, const int16_t &__x, const int16_t &__y, const int &__text_color = WHITE, const int &__background_color = 0, const char *__font_name = DEFAULT_FONT) {
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
            void draw_acc_16(const string &__str, int __text_color = WHITE, int __background_color = 0, const char *__font_name = DEFAULT_FONT) {
                get_font(__font_name);
                if (__background_color == 0) __background_color = _color;
                if (__background_color == WHITE) __text_color = BLACK;
                create_font_gc(__text_color, __background_color, font);

                int len;
                xcb_char2b_t *char2b_str = convert_to_char2b(__str.c_str(), &len);

                int16_t x = CENTER_TEXT(_width, __str.length());
                int16_t y = CENTER_TEXT_Y(_height);
                
                VOID_COOKIE = xcb_image_text_16(
                    conn,
                    len,
                    _window,
                    font_gc,
                    x,
                    y,
                    char2b_str

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();
                free(char2b_str);

            }

        /* Keys          */
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
                    {   S,          SUPER                   }, // key_binding for 'system_settings'
                    {   K,          SUPER                   }/* ,
                    {   R,          SUPER                   }, // key_binding for 'runner_window'
                    {   F,          SUPER                   }, // key_binding for 'file_app'
                    {   D,          SUPER                   }  // key_binding for 'debub menu' */

                });

            }
            void grab_keys(initializer_list<pair<const uint32_t, const uint16_t>> bindings) {
                xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(conn);
                if (!keysyms) {
                    loutE << "keysyms could not get initialized" << loutEND;
                    return;

                }

                for (const auto & binding : bindings) {
                    xcb_keycode_t * keycodes = xcb_key_symbols_get_keycode(keysyms, binding.first);
                    if (keycodes) {
                        for (auto * kc = keycodes; * kc; kc++) {
                            VOID_COOKIE = xcb_grab_key(
                                conn,
                                1,
                                _window,
                                binding.second, 
                                *kc,        
                                XCB_GRAB_MODE_ASYNC, 
                                XCB_GRAB_MODE_ASYNC  

                            ); CHECK_VOID_COOKIE();

                        } free(keycodes);

                    }

                } xcb_key_symbols_free(keysyms); FLUSH_XWin();
 
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
                    {MINUS, NULL},

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
                    {MINUS, SHIFT},

                    { SPACE_BAR,    NULL        },
                    { SPACE_BAR,    SHIFT       },
                    { ENTER,        NULL        },
                    { ENTER,        SHIFT       },
                    { DELETE,       NULL        },
                    { DELETE,       SHIFT       },
                });

            }
        
        /* Buttons       */
            void grab_button(initializer_list<pair<uint8_t, uint16_t>> __bindings) {
                for (const auto &pair : __bindings) {
                    VOID_COOKIE = xcb_grab_button(
                        conn, 
                        1, 
                        _window, 
                        XCB_EVENT_MASK_BUTTON_PRESS, 
                        XCB_GRAB_MODE_ASYNC, 
                        XCB_GRAB_MODE_ASYNC, 
                        XCB_NONE,
                        XCB_NONE,
                        pair.first,
                        pair.second

                    ); CHECK_VOID_COOKIE();
                    FLUSH_XWin();

                }

            }
            void ungrab_button(initializer_list<pair<uint8_t, uint16_t>> __bindings) {
                for (const auto &pair : __bindings) {
                    VOID_COOKIE = xcb_ungrab_button(
                        conn,
                        pair.first,
                        _window,
                        pair.second

                    );
                    CHECK_VOID_COOKIE();
                    FLUSH_XWin();
                }
            }
        
        /* Events */
            template <typename Callback>
            void add_action_on_L_button_event(Callback &&__callback)
            {
                do {
                    int id = event_handler->setEventCallback( XCB_BUTTON_PRESS, [ this, __callback ]( Ev ev )-> void
                    {
                        RE_CAST_EV( xcb_button_press_event_t );
                        if (( e->event == this->_window ) && ( e->detail == L_MOUSE_BUTTON ))
                        {
                            __callback();
                        }
                    });
                    _ev_id_vec.push_back( { XCB_BUTTON_PRESS, id });

                } while ( 0 );
            }
            
    private:
    /* Variables   */
        /* Main */
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
        
        uint32_t _gc;
        xcb_gcontext_t font_gc;
        xcb_font_t     font;
        xcb_pixmap_t   pixmap;
        string _name{};
        int _color = 0;

        uint32_t _min_width  = 200;
        uint32_t _min_height = 100;
        pid_t    _pid = 0;

        FixedArray<uint32_t, 4> _border{};

        uint8_t  _override_redirect = 0;
        uint8_t _bit_state = 0;

        vector<pair<uint8_t, int>> _ev_id_vec;
        /* vector<pair<uint8_t, function<void(const xcb_generic_event_t *)>>> _func_vec; */

    /* Methods     */
        /* Main       */
            void make_window()
            {
                if (( _window = xcb_generate_id( conn )) == 0xFFFFFFFF )
                {
                    loutEWin << "Could not generate id for window" << loutEND;
                    /** NOTE: Clering the @p Xid_gen_success bit of @class member variable @p '_bit_state' */
                    _bit_state &= ~( 1 << Xid_gen_success );
                    return;
                }

                VOID_COOKIE = xcb_create_window(
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
                CHECK_VOID_COOKIE();
                FLUSH_XWin();
                /** NOTE: Setting @p Xid_gen_success bit of @class member variable @p '_bit_state' */
                _bit_state |= ( 1 << Xid_gen_success );
                if ( _bit_state & ( 1 << Xid_gen_success ))
                {
                    loutI << "Xid_gen_success bit is set" << '\n';
                }

                do {
                    int id = event_handler->setEventCallback( XCB_DESTROY_NOTIFY, [ this ](Ev ev)-> void
                    {
                        RE_CAST_EV( xcb_destroy_notify_event_t );
                        if ( e->window == this->_window )
                        {
                            for ( int i = 0; i < this->_ev_id_vec.size(); ++i )
                            {
                                event_handler->removeEventCallback( this->_ev_id_vec[i].first, this->_ev_id_vec[i].second );
                            }
                        }
                    });
                    this->_ev_id_vec.push_back( { XCB_DESTROY_NOTIFY, id } );
                } while ( 0 );

            }
            void clear_window() {
                VOID_COOKIE = xcb_clear_area(
                    conn, 
                    0,
                    this->_window,
                    0, 
                    0,
                    this->_width,
                    this->_height

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            void clear_window(uint32_t __window)
            {
                VOID_COOKIE = xcb_clear_area(
                    conn, 
                    0,
                    __window,
                    0,
                    0,
                    20,
                    20
                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            /**
             * @brief Configures the window with the specified mask and value.
             * 
             * This function configures the window using the XCB library. It takes in a mask and a value
             * as parameters and applies the configuration to the window.
             * 
             * @param mask The mask specifying which attributes to configure.
             * @param value The value to set for the specified attributes.
             * 
             */
            void config_window( const uint16_t & mask, const uint16_t & value )
            {
                xcb_configure_window(
                    conn,
                    _window,
                    mask,
                    (const uint32_t[1]) {
                        static_cast<const uint32_t &>(value)

                    }

                );

            }
            void config_window(uint16_t __mask, const void *__value) {
                VOID_COOKIE = xcb_configure_window(
                    conn,
                    _window,
                    __mask,
                    __value
                
                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            void config_window(uint32_t mask, const vector<uint32_t> & values) {
                if (values.empty()) {
                    loutEWin << "values vector is empty" << loutEND;
                    return;

                }
                VOID_COOKIE = xcb_configure_window(
                    conn,
                    _window,
                    mask,
                    values.data()

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }

        /* Create     */
            /* Gc     */
                void create_graphics_exposure_gc() {
                    _gc = xcb_generate_id(conn);
                    // uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;

                    VOID_COOKIE = xcb_create_gc(
                        conn,
                        _gc,
                        _window,
                        GC_MASK,
                        (uint32_t[3]) {
                            screen->black_pixel,
                            screen->white_pixel,
                            0

                        }

                    ); CHECK_VOID_COOKIE();
                    FLUSH_XWin();

                }
                void create_font_gc(int text_color, int backround_color, xcb_font_t font) {
                    font_gc = xcb_generate_id(conn);
                    VOID_COOKIE = xcb_create_gc(
                        conn, 
                        font_gc, 
                        _window, 
                        XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT, 
                        (const uint32_t[3]) {
                            get_color(text_color),
                            get_color(backround_color),
                            font

                        }

                    ); CHECK_VOID_COOKIE();
                    FLUSH_XWin();

                }
            
            /* Pixmap */
                void create_pixmap() {
                    pixmap = xcb_generate_id(conn);
                    VOID_COOKIE = xcb_create_pixmap(
                        conn, 
                        screen->root_depth, 
                        pixmap, 
                        _window, 
                        _width, 
                        _height

                    ); CHECK_VOID_COOKIE();
                    FLUSH_XWin();

                }
            
            /* Png    */
                void create_png_from_vector_bitmap(const char *file_name, const vector<vector<bool>> &bitmap) {
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

                        } png_write_row(png_ptr, row);

                    } delete[] row;
                    
                    png_write_end(png_ptr, NULL);
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);

                }
                void create_png_from_vector_bitmap(const char *file_name, bool bitmap[20][20]) {
                    int width  = 20, height = 20;

                    FILE *fp = fopen(file_name, "wb");
                    if (!fp) {
                        loutE << "Failed to open file:" << file_name << loutEND;
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

                        } png_write_row(png_ptr, row);

                    } delete[] row;

                    png_write_end(png_ptr, NULL);
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);

                }

        /* Get        */
            xcb_atom_t atom(const char *atom_name) {
                xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
                    conn, 
                    0, 
                    strlen(atom_name), 
                    atom_name

                );
                
                xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
                if (!reply) {
                    log_error("could not get atom");
                    return XCB_ATOM_NONE;

                } xcb_atom_t atom = reply->atom; free(reply);

                return atom;

            }
            string AtomName(xcb_atom_t atom) {
                xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
                xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(conn, cookie, nullptr);

                if (!reply) {
                    log_error("reply is nullptr.");
                    return "";

                }
                int name_len = xcb_get_atom_name_name_length(reply);
                char* name = xcb_get_atom_name_name(reply);
                
                string atomName(name, name + name_len);
                free(reply);

                return atomName;
                
            }
            void get_font(const char *font_name) {
                font = xcb_generate_id(conn);
                VOID_COOKIE = xcb_open_font(
                    conn, 
                    font, 
                    slen(font_name),
                    font_name

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
        
        /* Background */
            void change_back_pixel(uint32_t pixel) {
                VOID_COOKIE = xcb_change_window_attributes(
                    conn,
                    _window,
                    XCB_CW_BACK_PIXEL,
                    (const uint32_t[1]) {
                        pixel

                    }

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            void change_back_pixel(uint32_t pixel, uint32_t __window) {
                VOID_COOKIE = xcb_change_window_attributes(
                    conn,
                    __window,
                    XCB_CW_BACK_PIXEL,
                    (const uint32_t[1]) {
                        pixel

                    }

                ); CHECK_VOID_COOKIE();
                FLUSH_XWin();

            }
            uint32_t get_color(int __color) {
                uint32_t                pi = 0;
                xcb_colormap_t        cmap = screen->default_colormap;
                rgb_color_code       ccode = rgb_code(__color);
                xcb_alloc_color_reply_t *r = xcb_alloc_color_reply(conn, xcb_alloc_color(
                    conn,
                    cmap,
                    _scale::from_8_to_16_bit(ccode.r), 
                    _scale::from_8_to_16_bit(ccode.g),
                    _scale::from_8_to_16_bit(ccode.b)

                ), NULL); pi = r->pixel; free(r);
                
                return pi;
                
            }
            uint32_t get_color(const uint16_t &red_value, const uint16_t &green_value, const uint16_t &blue_value) {
                uint32_t                pi = 0;
                xcb_colormap_t        cmap = screen->default_colormap;
                xcb_alloc_color_reply_t *r = xcb_alloc_color_reply(conn, xcb_alloc_color(
                    conn,
                    cmap,
                    red_value, 
                    green_value,
                    blue_value

                ), NULL); pi = r->pixel; free(r);
                
                return pi;

            }
            uint32_t get_color(const uint8_t & red_value, const uint8_t & green_value, const uint8_t & blue_value) {
                uint32_t                pi = 0;
                xcb_colormap_t        cmap = screen->default_colormap;
                xcb_alloc_color_reply_t *r = xcb_alloc_color_reply(conn, xcb_alloc_color(
                    conn,
                    cmap,
                    _scale::from_8_to_16_bit(red_value), 
                    _scale::from_8_to_16_bit(green_value),
                    _scale::from_8_to_16_bit(blue_value)

                ), NULL); pi = r->pixel; free(r);
                
                return pi;

            }
            rgb_color_code rgb_code(int __color) {
                rgb_color_code color;
                uint8_t r, g, b;
                
                switch (__color) {
                    case WHITE: {
                        r = 255; g = 255; b = 255;
                        break;

                    }
                    case BLACK: {
                        r = 0; g = 0; b = 0;
                        break;

                    }
                    case RED: {
                        r = 255; g = 0; b = 0;
                        break;

                    }
                    case GREEN: {
                        r = 0; g = 255; b = 0;
                        break;

                    }
                    case BLUE: {
                        r = 0; g = 0; b = 255;
                        break;

                    }
                    case BLUE_2: {
                        r = 0; g = 0; b = 230;
                        break;

                    }
                    case BLUE_3: {
                        r = 0; g = 0; b = 204;
                        break;

                    }
                    case BLUE_4: {
                        r = 0; g = 0; b = 178;
                        break;

                    }
                    case BLUE_5: {
                        r = 0; g = 0; b = 153;
                        break;

                    }
                    case BLUE_6: {
                        r = 0; g = 0; b = 128;
                        break;

                    }
                    case BLUE_7: {
                        r = 0; g = 0; b = 102;
                        break;

                    }
                    case BLUE_8: {
                        r = 0; g = 0; b = 76;
                        break;

                    }
                    case BLUE_9: {
                        r = 0; g = 0; b = 51;
                        break;

                    }
                    case BLUE_10: {
                        r = 0; g = 0; b = 26;
                        break;

                    }
                    case YELLOW: {
                        r = 255; g = 255; b = 0;
                        break;

                    }
                    case CYAN: {
                        r = 0; g = 255; b = 255;
                        break;
                        
                    }
                    case MAGENTA: {
                        r = 255; g = 0; b = 255;
                        break;
                        
                    }
                    case GREY: {
                        r = 128; g = 128; b = 128;
                        break;

                    }
                    case LIGHT_GREY: {
                        r = 192; g = 192; b = 192;
                        break;

                    }
                    case DARK_GREY: {
                        r = 64; g = 64; b = 64;
                        break;

                    }
                    case DARK_GREY_2: {
                        r = 70; g = 70; b = 70;
                        break;

                    }
                    case DARK_GREY_3: {
                        r = 76; g = 76; b = 76;
                        break;

                    }
                    case DARK_GREY_4: {
                        r = 82; g = 82; b = 82;
                        break;

                    }
                    case ORANGE: {
                        r = 255; g = 165; b = 0;
                        break;

                    }
                    case PURPLE: {
                        r = 128; g = 0; b = 128;
                        break;

                    }
                    case BROWN: {
                        r = 165; g = 42; b = 42;
                        break;

                    }
                    case PINK: {
                        r = 255; g = 192; b = 203;
                        break;

                    }
                    default: {
                        r = 0; g = 0; b = 0; 
                        break;

                    }

                } color.r = r; color.g = g; color.b = b;

                return color;

            }
        
        /* Borders    */
            void create_border_window(BORDER __border, int __color, uint32_t __x, uint32_t __y, uint32_t __width, uint32_t __height) {
                uint32_t window;
                if ((window = xcb->gen_Xid()) == -1) {
                    loutEWin << "Failed to create border window: " << WINDOW_ID_BY_INPUT(window) << loutEND;
                    return; 

                }
                xcb->create_w(_window, window, __x, __y, __width, __height);

                change_back_pixel(get_color(__color), window);
                VOID_COOKIE = xcb_map_window(conn, window); CHECK_VOID_COOKIE(); FLUSH_XWin();
                CHECK_BITFLAG_ERR(xcb->check_conn(), X_W_CREATION_ERR);

                if (__border == UP   ) _border[0] = window;
                if (__border == DOWN ) _border[1] = window;
                if (__border == LEFT ) _border[2] = window;
                if (__border == RIGHT) _border[3] = window;

            }
            #define CREATE_UP_BORDER(__size, __color)    create_border_window(UP,    __color, 0, 0, _width, __size)
            #define CREATE_DOWN_BORDER(__size, __color)  create_border_window(DOWN,  __color, 0, (_height - __size), _width, __size)
            #define CREATE_LEFT_BORDER(__size, __color)  create_border_window(LEFT,  __color, 0, 0, __size, _height)
            #define CREATE_RIGHT_BORDER(__size, __color) create_border_window(RIGHT, __color, (_width - __size), 0, __size, _height)
            #define Create_Border(__flag, __color, ...)  do { \
                unsigned int __bits[] = { __VA_ARGS__ }; \
                create_border_window( \
                    __flag, \
                    __color, \
                    __bits[0], \
                    __bits[1], \
                    __bits[2], \
                    __bits[3]); \
                \
            } while(0)
            void make_border_window(int __border, const uint32_t &__size, const int &__color) {
                if (__border & UP   ) CREATE_UP_BORDER(__size, __color);
                Create_Border(UP, __color, 0, 0, _width, __size);
                if (__border & DOWN ) CREATE_DOWN_BORDER(__size, __color);
                if (__border & LEFT ) CREATE_LEFT_BORDER(__size, __color);
                if (__border & RIGHT) CREATE_RIGHT_BORDER(__size, __color);

            }

        /* Font       */
            /**
             * @brief Decodes a single UTF-8 encoded character from the input string
             *        and returns the Unicode code point.
             *        Also advances the input string by the number of bytes used for
             *        the decoded character. 
             */
            uint32_t decode_utf8_char(const char **input) {
                const unsigned char *str = (const unsigned char *)*input;
                uint32_t codepoint = 0;
                if (str[0] <= 0x7F) {
                    
                    codepoint = str[0];
                    *input += 1;

                }/* 1-byte character */
                else if ((str[0] & 0xE0) == 0xC0) {
                    codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
                    *input += 2;

                }/* 2-byte character */
                else if ((str[0] & 0xF0) == 0xE0) {
                    codepoint = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
                    *input += 3;

                }/* 3-byte character */
                else if ((str[0] & 0xF8) == 0xF0) {
                    
                    codepoint = 0xFFFD; // Replacement character, as UCS-2 cannot represent this
                    *input += 4;

                }/* 4-byte character (will not be fully represented in UCS-2) */
                else {
                    codepoint = 0xFFFD;
                    *input += 1; // Advance past the invalid byte

                }/* Invalid UTF-8, return replacement character */

                return codepoint;

            }
            xcb_char2b_t *convert_to_char2b(const char *input, int *len) {
                size_t utf8_len = slen(input);
                size_t max_chars = utf8_len; // Maximum possible number of characters (all 1-byte)

                xcb_char2b_t *char2b = (xcb_char2b_t *)malloc(max_chars * sizeof(xcb_char2b_t));
                int count = 0;
                while (*input != '\0' && count < max_chars) {
                    uint32_t codepoint = decode_utf8_char(&input);

                    // Convert Unicode codepoint to xcb_char2b_t
                    char2b[count].byte1 = (codepoint >> 8) & 0xFF;
                    char2b[count].byte2 = codepoint & 0xFF;
                    count++;

                }

                *len = count; // Actual number of characters converted
                return char2b;

            }/* Converts a UTF-8 string to an array of xcb_char2b_t for xcb_image_text_16 */

};
#define CLI_TITLE_BAR_EXPOSE_BIT 1
#define CLI_TITLE_REQ_BIT 2
class client {
    public:
    /* Sub Classes */
        class client_border_decor {
            /* Defines */
                #define left 0
                #define right 1
                #define top 2
                #define bottom 3
                #define top_left 4
                #define top_right 6
                #define bottom_left 5
                #define bottom_right 7
            
            public:
            // /* Variables */
            //     window left;
            //     window right;
            //     window top;
            //     window bottom;

            //     window top_left;
            //     window top_right;
            //     window bottom_left;
            //     window bottom_right;
            // void create_arr()
            // {
            //     for (int i = 0; i < 8; ++i)
            //     {
            //         window w;
            //         border.push_back(w);
            //     }
            // }

            window &operator[](size_t index) {
                return border[index];

            }

            // client_border_decor() { create_arr(); }
            // DynamicArray<window> border;
            FixedArray<window, 8> border;

        };
        struct __atoms__ {
            bool is_modal = false;
            bool has_modal = false;

        };
        struct __modal_data__ {
            uint32_t transient_for = 0;
            uint32_t modal_window = 0;
        };

    /* Variabels   */
        window win;
        window frame;
        window titlebar;
        window close_button;
        window max_button;
        window min_button;
        window icon;

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
        bool moving = false;
        mutex mtx;
        ThreadPool thread_pool{8};
        
        uint64_t bit_state = 0xffffffffffffffff;
        vector<pair<uint8_t, int>> ev_id_vec;

    /* Methods     */
        /* Main     */
            void make_decorations() {
                make_frame();
                set_icon_png();
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
                win.set_active_EWMH_window();
                frame.raise();

            }
            void update() {
                x = frame.x();
                y = frame.y();
                width = frame.width();
                height = frame.height();
                win.send_event(XCB_EVENT_MASK_STRUCTURE_NOTIFY, (uint32_t[]){(static_cast<uint32_t>(x) + BORDER_SIZE), (static_cast<uint32_t>(y) + (TITLE_BAR_HEIGHT + BORDER_SIZE)), win.width(), win.height()});
        
            }
            void map() {
                frame.map();
            }
            void unmap() {
                frame.unmap();
        
            }
            void kill()
            {
                frame.unmap();
                win.unmap();
                close_button.unmap();
                max_button.unmap();
                min_button.unmap();
                titlebar.unmap();
                border[ left ].unmap();
                border[ right ].unmap();
                border[ top ].unmap();
                border[ bottom ].unmap();
                border[ top_left ].unmap();
                border[ top_right ].unmap();
                border[ bottom_left ].unmap();
                border[ bottom_right ].unmap();

                // vector<window> window_vec = {
                //     win,
                //     close_button,
                //     max_button,
                //     min_button,
                //     titlebar,
                //     border.left,
                //     border.right,
                //     border.top,
                //     border.bottom,
                //     border.top_left,
                //     border.top_right,
                //     border.bottom_left,
                //     border.bottom_right,
                //     frame
                // };
                // vector<thread> threads;

                // for (auto &window : window_vec)
                // {
                //     threads.emplace_back([&]() -> void { window.kill(); });
                // }

                // for (auto &t : threads)
                // {
                //     t.join();   
                // }

                win.kill();
                close_button.kill();
                max_button.kill();
                min_button.kill();
                titlebar.kill();
                border[left].kill();
                border[right].kill();
                border[top].kill();
                border[bottom].kill();
                border[top_left].kill();
                border[top_right].kill();
                border[bottom_left].kill();
                border[bottom_right].kill();
                frame.kill();

                remove_from_map();
                for ( int i = 0; i < ev_id_vec.size(); ++i )
                {
                    event_handler->removeEventCallback( ev_id_vec[i].first, ev_id_vec[i].second );
                }
            }
            void add_to_map() {
                cw_map.insert(win,          this);
                cw_map.insert(frame,        this);
                cw_map.insert(titlebar,     this);
                cw_map.insert(close_button, this);
                cw_map.insert(max_button,   this);
                cw_map.insert(min_button,   this);
                cw_map.insert(icon,         this);
                cw_map.insert(border[0],    this);
                cw_map.insert(border[1],    this);
                cw_map.insert(border[2],    this);
                cw_map.insert(border[3],    this);
                cw_map.insert(border[4],    this);
                cw_map.insert(border[5],    this);
                cw_map.insert(border[6],    this);
                cw_map.insert(border[7],    this);
            }
            void remove_from_map() {
                cw_map.remove(win);
                cw_map.remove(frame);
                cw_map.remove(titlebar);
                cw_map.remove(close_button);
                cw_map.remove(max_button);
                cw_map.remove(min_button);
                cw_map.remove(icon);
                cw_map.remove(border[0]);
                cw_map.remove(border[1]);
                cw_map.remove(border[2]);
                cw_map.remove(border[3]);
                cw_map.remove(border[4]);
                cw_map.remove(border[5]);
                cw_map.remove(border[6]);
                cw_map.remove(border[7]);
            }
            void align() {
                win.x(BORDER_SIZE);
                win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
                FLUSH_X();

            }
            #define TITLE_REQ_DRAW  (uint32_t)1 << 0
            #define TITLE_INTR_DRAW (uint32_t)1 << 1
            void draw_title( uint32_t __mode )
            {
                titlebar.clear();
                if ( __mode & TITLE_REQ_DRAW )  { titlebar.draw_acc_16( win.get_net_wm_name_by_req() ); }
                if ( __mode & TITLE_INTR_DRAW ) { titlebar.draw_acc_16( win.get_net_wm_name() ); }

            }
            #define CLI_RIGHT  screen->width_in_pixels  - this->width
            #define CLI_BOTTOM screen->height_in_pixels - this->height
            void snap(int x, int y, const vector<client *> &__vec )
            {
                for (client *const &c : __vec) {
                    if (c == this) continue;
                    
                    if (x > c->x + c->width - N && x < c->x + c->width + N
                    &&  y + this->height > c->y && y < c->y + c->height) {
                        /* SNAP WINDOW TO 'RIGHT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        if (y > c->y - NC && y < c->y + NC) {
                            this->frame.x_y((c->x + c->width), c->y);
                            return;

                        }
                        
                        if (y + this->height > c->y + c->height - NC && y + this->height < c->y + c->height + NC) {
                            this->frame.x_y((c->x + c->width), (c->y + c->height) - this->height);
                            return;

                        } /* SNAP WINDOW TO 'RIGHT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        this->frame.x_y((c->x + c->width), y);
                        return;

                    } /* SNAP WINDOW TO 'RIGHT' BORDER OF 'NON_CONTROLLED' WINDOW */
                    
                    if (x + this->width > c->x - N && x + this->width < c->x + N
                    &&  y + this->height > c->y && y < c->y + c->height) {
                        if (y > c->y - NC && y < c->y + NC)                                                       {
                            this->frame.x_y((c->x - this->width), c->y);
                            return;

                        } /* SNAP WINDOW TO 'LEFT_TOP' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        if (y + this->height > c->y + c->height - NC && y + this->height < c->y + c->height + NC) {
                            this->frame.x_y((c->x - this->width), (c->y + c->height) - this->height);
                            return;

                        } /* SNAP WINDOW TO 'LEFT_BOTTOM' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        this->frame.x_y((c->x - this->width), y);
                        return;

                    } /* SNAP WINDOW TO 'LEFT' BORDER OF 'NON_CONTROLLED' WINDOW */
                    
                    if (y > c->y + c->height - N && y < c->y + c->height + N 
                    &&  x + this->width > c->x && x < c->x + c->width)   {
                        if (x > c->x - NC && x < c->x + NC)                                                   {
                            this->frame.x_y(c->x, (c->y + c->height));
                            return;

                        } /* SNAP WINDOW TO 'BOTTOM_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        if (x + this->width > c->x + c->width - NC && x + this->width < c->x + c->width + NC) {
                            this->frame.x_y(((c->x + c->width) - this->width), (c->y + c->height));
                            return;

                        } /* SNAP WINDOW TO 'BOTTOM_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        this->frame.x_y(x, (c->y + c->height));
                        return;

                    } /* SNAP WINDOW TO 'BOTTOM' BORDER OF 'NON_CONTROLLED' WINDOW */
                    
                    if (y + this->height > c->y - N && y + this->height < c->y + N     
                    &&  x + this->width > c->x && x < c->x + c->width)   {
                        if (x > c->x - NC && x < c->x + NC)                                                       {
                            this->frame.x_y(c->x, (c->y - this->height));
                            return;

                        } /* SNAP WINDOW TO 'TOP_LEFT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        if (x + this->width > c->x + c->width - NC && x + this->width < c->x + c->width + NC) {
                            this->frame.x_y(((c->x + c->width) - this->width), (c->y - this->height));
                            return;

                        } /* SNAP WINDOW TO 'TOP_RIGHT' CORNER OF NON_CONROLLED WINDOW WHEN APPROPRIET */
                        
                        this->frame.x_y(x, (c->y - this->height));
                        return;

                    } /* SNAP WINDOW TO 'TOP' BORDER OF 'NON_CONTROLLED' WINDOW */

                } /* WINDOW TO WINDOW SNAPPING */

                /* WINDOW TO EDGE OF SCREEN SNAPPING */
                if        (((x < N) && (x > -N)) && ((y < N) && (y > -N)))                                          {
                    this->frame.x_y(0, 0);

                } else if  ((x < CLI_RIGHT + N && x > CLI_RIGHT - N) && (y < N && y > -N))                          {
                    this->frame.x_y(CLI_RIGHT, 0);

                } else if  ((y < CLI_BOTTOM + N && y > CLI_BOTTOM - N) && (x < N && x > -N))                        {
                    this->frame.x_y(0, CLI_BOTTOM);

                } else if  ((x < N) && (x > -N))                                                                    {
                    this->frame.x_y(0, y);

                } else if   (y < N && y > -N)                                                                       {
                    this->frame.x_y(x, 0);

                } else if  ((x < CLI_RIGHT + N && x > CLI_RIGHT - N) && (y < CLI_BOTTOM + N && y > CLI_BOTTOM - N)) {
                    this->frame.x_y(CLI_RIGHT, CLI_BOTTOM);

                } else if  ((x < CLI_RIGHT + N) && (x > CLI_RIGHT - N))                                             {
                    this->frame.x_y(CLI_RIGHT, y);

                } else if   (y < CLI_BOTTOM + N && y > CLI_BOTTOM - N)                                              {
                    this->frame.x_y(x, CLI_BOTTOM);
                    
                } else {
                    this->frame.x_y(x, y);

                }

            }/** @brief client to client snaping */
        
        /* Config   */
            void x_y(const uint32_t &x, const uint32_t &y) {
                frame.x_y(x, y);
                // frame.configure(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, (uint32_t[2]){x, y});
            }
            void _x(int16_t x) {
                frame.x(x);
        
            }
            void _y(int16_t y) {
                frame.y(y);
        
            }
            void _width(uint16_t width) {
                win.width((width - (BORDER_SIZE * 2)));
                xcb_flush(conn);
                frame.width((width));
                titlebar.width(width - (BORDER_SIZE * 2));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border[right].x((width - BORDER_SIZE));
                border[top].width((width - (BORDER_SIZE * 2)));
                border[bottom].width((width - (BORDER_SIZE * 2)));
                border[top_right].x((width - BORDER_SIZE));
                border[bottom_right].x((width - BORDER_SIZE));
                xcb_flush(conn);
        
            }
            void _height(const uint32_t &height) {
                win.height((height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                frame.height(height);
                border[left].height((height - (BORDER_SIZE * 2)));
                border[right].height((height - (BORDER_SIZE * 2)));
                border[bottom].y((height - BORDER_SIZE));
                border[bottom_left].y((height - BORDER_SIZE));
                border[bottom_right].y((height - BORDER_SIZE));
        
            }
            void x_width(const uint32_t &x, const uint32_t &width) {
                win.width((width - (BORDER_SIZE * 2)));
                frame.x_width(x, width);
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border[right].x((width - BORDER_SIZE));
                border[top].width((width - (BORDER_SIZE * 2)));
                border[bottom].width((width - (BORDER_SIZE * 2)));
                border[top_right].x((width - BORDER_SIZE));
                border[bottom_right].x((width - BORDER_SIZE));
                xcb_flush(conn);
        
            }
            void y_height(const uint32_t & y, const uint32_t & height) {
                win.height((height - TITLE_BAR_HEIGHT) - (BORDER_SIZE * 2));
                xcb_flush(conn);
                frame.y_height(y, height);
                border[left].height((height - (BORDER_SIZE * 2)));
                border[right].height((height - (BORDER_SIZE * 2)));
                border[bottom].y((height - BORDER_SIZE));
                border[bottom_left].y((height - BORDER_SIZE));
                border[bottom_right].y((height - BORDER_SIZE));
                xcb_flush(conn);
        
            }
            void x_width_height(const uint32_t &x, const uint32_t &width, const uint32_t &height) {
                frame.x_width_height(x, width, height);
                win.width_height((width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border[left].height((height - (BORDER_SIZE * 2)));
                border[right].x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border[top].width((width - (BORDER_SIZE * 2)));
                border[bottom].y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border[top_right].x((width - BORDER_SIZE));
                border[bottom_left].y((height - BORDER_SIZE));
                border[bottom_right].x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
        
            }
            void y_width_height(const uint32_t &y, const uint32_t &width, const uint32_t &height) {
                frame.y_width_height(y, width, height);
                win.width_height((width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                titlebar.width((width - BORDER_SIZE * 2));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border[left].height((height - (BORDER_SIZE * 2)));
                border[right].x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border[top].width((width - (BORDER_SIZE * 2)));
                border[bottom].y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border[top_right].x((width - BORDER_SIZE));
                border[bottom_left].y((height - BORDER_SIZE));
                border[bottom_right].x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
        
            }
            void x_y_width_height(const uint32_t &x, const uint32_t &y, const uint32_t &width, const uint32_t &height) {
                win.width_height((width - (BORDER_SIZE * 2)), (height - TITLE_BAR_HEIGHT - (BORDER_SIZE * 2)));
                xcb_flush(conn);
                frame.x_y_width_height(x, y, width, height);
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border[left].height((height - (BORDER_SIZE * 2)));
                border[right].x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border[top].width((width - (BORDER_SIZE * 2)));
                border[bottom].y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border[top_right].x((width - BORDER_SIZE));
                border[bottom_right].x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
                border[bottom_left].y((height - BORDER_SIZE));
                xcb_flush(conn);
        
            }
            void width_height(const uint32_t &width, const uint32_t &height) {
                win.width_height((width - (BORDER_SIZE * 2)), (height - (BORDER_SIZE * 2) - TITLE_BAR_HEIGHT));
                xcb_flush(conn);
                frame.width_height(width, height);
                titlebar.width((width - (BORDER_SIZE * 2)));
                close_button.x((width - BUTTON_SIZE - BORDER_SIZE));
                max_button.x((width - (BUTTON_SIZE * 2) - BORDER_SIZE));
                min_button.x((width - (BUTTON_SIZE * 3) - BORDER_SIZE));
                border[left].height((height - (BORDER_SIZE * 2)));
                border[right].x_height((width - BORDER_SIZE), (height - (BORDER_SIZE * 2)));
                border[top].width((width - (BORDER_SIZE * 2)));
                border[bottom].y_width((height - BORDER_SIZE), (width - (BORDER_SIZE * 2)));
                border[top_right].x((width - BORDER_SIZE));
                border[bottom_right].x_y((width - BORDER_SIZE), (height - BORDER_SIZE));
                border[bottom_left].y((height - BORDER_SIZE));
                xcb_flush(conn);

            }
        
        /* Size_pos */
            void save_ogsize() {
                ogsize.save(x, y, width, height);
            }
            void save_tile_ogsize() {
                tile_ogsize.save(x, y, width, height);
            }
            void save_max_ewmh_ogsize() {
                max_ewmh_ogsize.save(x, y, width, height);
            }
            void save_max_button_ogsize() {
                max_button_ogsize.save(x, y, width, height);
            }
        
        /* Check    */
            bool is_active_EWMH_window() {
                return win.is_active_EWMH_window();
        
            }
            bool is_EWMH_fullscreen() {
                return win.is_EWMH_fullscreen();
        
            }
            bool is_button_max_win() {
                if (x      == -BORDER_SIZE
                &&  y      == -BORDER_SIZE
                &&  width  == (screen->width_in_pixels  + (BORDER_SIZE * 2))
                &&  height == (screen->height_in_pixels + (BORDER_SIZE * 2))) {
                    return true;

                } return false;

            }
        
        /* Set      */
            void set_active_EWMH_window() {
                win.set_active_EWMH_window();
    
            }
            void set_EWMH_fullscreen_state() {
                win.set_EWMH_fullscreen_state();

            }
            void set_client_params() {
                win.set_client_size_as_hints(&x, &y, &width, &height);
                if (win.get_min_width()  > width
                &&  win.get_min_height() > height) {
                    get_window_parameters();

                }

            }

        /* Get      */
            void get_window_parameters() {
                xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, win);
                xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, nullptr);
                if (reply == nullptr) {
                    loutE << "Unable to get window geometry" << '\n';
                    return;
                }

                x      = reply->x;
                y      = reply->y;
                width  = reply->width;
                height = reply->height;

                // loutI << "reply->x"      << reply->x      << '\n';
                // loutI << "reply->y"      << reply->y      << '\n';
                // loutI << "reply->width"  << reply->width  << '\n';
                // loutI << "reply->height" << reply->height << '\n';

                free(reply);

            }
            void get_window_parameters_and_log() {
                xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, win);
                xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, nullptr);
                if (reply == nullptr) {
                    loutE << "Unable to get window geometry" << '\n';
                    return;
                }

                loutI << "reply->x"      << reply->x      << '\n';
                loutI << "reply->y"      << reply->y      << '\n';
                loutI << "reply->width"  << reply->width  << '\n';
                loutI << "reply->height" << reply->height << '\n';

                free(reply);
            }

        /* Unset    */
            void unset_EWMH_fullscreen_state() {
                win.unset_EWMH_fullscreen_state();
            }

        /* Bit State */
            bool check_bit( uint8_t __pos ) {
                return ((this->bit_state & ( 1ULL << __pos )) != 0);
            }
            void set_bit( uint8_t __pos ) {
                bit_state |= (1ULL << __pos);
            }
            void clear_bit( uint8_t __pos ) {
                bit_state &= ~(1ULL << __pos);
            }
            void flip_bit( uint8_t __pos ) {
                bit_state ^= ( 1ULL << __pos );
            }
    
    private:
    /* Methods     */
        void make_frame()
        {
            frame.create_window(
                screen->root,
                (x - BORDER_SIZE),
                (y - TITLE_BAR_HEIGHT - BORDER_SIZE),
                (width + (BORDER_SIZE * 2)),
                (height + TITLE_BAR_HEIGHT + (BORDER_SIZE * 2)),
                DARK_GREY

            ); win.reparent(frame, BORDER_SIZE, (TITLE_BAR_HEIGHT + BORDER_SIZE));
            FLUSH_X();
            update();
    
            CONN(XCB_FOCUS_IN,
                this->win.ungrab_button({{L_MOUSE_BUTTON, NULL}});
                

            , this->win);

            CONN(XCB_FOCUS_OUT,
                this->win.grab_button({{L_MOUSE_BUTTON, NULL}});

            , this->win);

            CONN(L_MOUSE_BUTTON_EVENT,
                this->win.set_active_EWMH_window();
                this->focus();
                this->win.focus_input();

            , this->win);

            frame.set_event_mask(FRAME_EVENT_MASK);
            frame.map();

            CONN(KILL_SIGNAL,
                if (this->win.is_mapped()) {
                    this->win.kill();
                }
                else if (this->frame.is_mapped()) {
                    this->kill();
                }

            , this->win);

            CONN(XCB_DESTROY_NOTIFY,
                if (!this->win.is_mapped()) {
                    this->kill();
                }
                else {
                    this->win.kill();
                }
            
            , this->win);

            CWC(frame);
            CWC(win);
        
        }
        void make_titlebar()
        {
            titlebar.create_window(
                frame,
                BORDER_SIZE,
                BORDER_SIZE,
                ( width - ( BORDER_SIZE * 2 )),
                TITLE_BAR_HEIGHT,
                BLACK,
                XCB_EVENT_MASK_EXPOSURE,
                MAP
            
            );
            CWC( titlebar );

            titlebar.grab_button({ { L_MOUSE_BUTTON, NULL } });
            draw_title( TITLE_REQ_DRAW );
            icon.raise();

            /* expose_tasks.push_back({this->titlebar, [&]() {
                this->titlebar.clear();
                this->titlebar.draw_acc_16(this->win.get_net_wm_name());
                FLUSH_X();
            }}); */
            /* CONN(XCB_EXPOSE, this->set_bit(CLI_TITLE_BAR_EXPOSE_BIT);, this->titlebar); */

            // // cli_tasks.push_back(this);
            // CONN(XCB_PROPERTY_NOTIFY, this->bit_state &= ( 1ULL << CLI_TITLE_REQ_BIT ); , this->titlebar);
            /* CONN( XCB_PROPERTY_NOTIFY,
                this->titlebar.clear();
                this->titlebar.draw_acc_16(this->win.get_net_wm_name_by_req());
                FLUSH_X();
            , this->win); */
            do {
                int id = event_handler->setEventCallback( XCB_PROPERTY_NOTIFY, [ & ]( Ev ev )-> void
                {
                    RE_CAST_EV( xcb_property_notify_event_t );
                    if ( e->window == this->win )
                    {
                        this->titlebar.clear( );
                        this->titlebar.draw_acc_16( this->win.get_net_wm_name_by_req( ) );
                        FlushX_Win( this->titlebar );
                    }
                });
                ev_id_vec.push_back({XCB_PROPERTY_NOTIFY, id});
            
            } while ( 0 );

            do {
                int id = event_handler->setEventCallback( XCB_EXPOSE, [&](Ev ev)
                {
                    RE_CAST_EV( xcb_expose_event_t );
                    if ( e->window == this->titlebar )
                    {
                        this->titlebar.clear();
                        this->titlebar.draw_acc_16( this->win.get_net_wm_name() );
                        FlushX_Win( this->titlebar );
                    }
                });
                ev_id_vec.push_back( { XCB_EXPOSE, id } );

            } while ( 0 );

            /* CONN( XCB_EXPOSE,
                this->titlebar.clear( );
                this->titlebar.draw_acc_16( this->win.get_net_wm_name( ) );
                FLUSH_X( );

            , this->titlebar ); */
        }
        void make_close_button()
        {
            this->close_button.create_window(
                frame,
                (width - BUTTON_SIZE - BORDER_SIZE),
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                BLUE,
                BUTTON_EVENT_MASK,
                MAP,
                (int[]){ALL, 1, BLACK},
                CURSOR::hand2

            );
            CWC( close_button );
            FlushX_Win( this->close_button );
            close_button.make_then_set_png( USER_PATH_PREFIX( "/close.png" ), CLOSE_BUTTON_BITMAP );
            /* do {
                int id = event_handler->setEventCallback( XCB_BUTTON_PRESS, [ & ]( Ev ev )-> void
                {
                    RE_CAST_EV( xcb_button_press_event_t );
                    if ( e->event != this->close_button ) return;
                    if ( e->detail != L_MOUSE_BUTTON ) return;
                    WS_emit( this->win, KILL_SIGNAL );
                });
                ev_id_vec.push_back( { XCB_BUTTON_PRESS, id } );

            } while ( 0 ); */
            close_button.add_action_on_L_button_event(
            [ this ]()-> void
            {
                if ( this->win.is_mapped() )
                {
                    this->win.kill();
                }
                else
                {
                    this->kill();
                }
                /* Emit( this->win, KILL_SIGNAL ); */
            });

            do {
                int id = event_handler->setEventCallback( XCB_ENTER_NOTIFY, [ & ]( Ev ev )-> void
                {
                    RE_CAST_EV( xcb_enter_notify_event_t );
                    if ( e->event == this->close_button )
                    {
                        this->close_button.change_border_color(WHITE);
                    }
                });
                ev_id_vec.push_back( { XCB_ENTER_NOTIFY, id } );
                
            } while ( 0 );
            do {
                int id = event_handler->setEventCallback( XCB_LEAVE_NOTIFY, [ & ]( Ev ev )-> void
                {
                    RE_CAST_EV( xcb_leave_notify_event_t );
                    if ( e->event == this->close_button )
                    {
                        this->close_button.change_border_color( BLACK );
                    }
                });
                ev_id_vec.push_back( { XCB_LEAVE_NOTIFY, id } );

            } while ( 0 );
        }
        void make_max_button() {
            max_button.create_window(
                frame,
                (width - (BUTTON_SIZE * 2) - BORDER_SIZE),
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                RED,
                BUTTON_EVENT_MASK,
                MAP,
                (int[3]) {ALL, 1, BLACK},
                CURSOR::hand2

            ); CWC(max_button);
            max_button.grab_button({ { L_MOUSE_BUTTON, NULL } });

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
            string s = USER_PATH_PREFIX("/max.png");
            bitmap.exportToPng(s.c_str());


            max_button.set_backround_png(USER_PATH_PREFIX("/max.png"));

            /* CONN(L_MOUSE_BUTTON_EVENT, if (__window == this->max_button) {
                C_EMIT(this, BUTTON_MAXWIN_PRESS);

            }, this->max_button); */
            event_handler->setEventCallback(XCB_BUTTON_PRESS, [&](Ev ev) {
                RE_CAST_EV(xcb_button_press_event_t);
                if (e->event == this->max_button) C_EMIT(this, BUTTON_MAXWIN_PRESS);

            });

            event_handler->setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_enter_notify_event_t);
                if (e->event != this->max_button) return;
                this->max_button.change_border_color(WHITE);
                
            });

            event_handler->setEventCallback(XCB_LEAVE_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_leave_notify_event_t);
                if (e->event != this->max_button) return;
                this->max_button.change_border_color(BLACK);
 
            });

        }
        void make_min_button() {
            min_button.create_window(
                frame,
                (width - (BUTTON_SIZE * 3) - BORDER_SIZE),
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                GREEN,
                BUTTON_EVENT_MASK,
                MAP,
                (int[3]) {ALL, 1, BLACK},
                CURSOR::hand2

            ); CWC(min_button);
            FLUSH_X();
            min_button.grab_button({ { L_MOUSE_BUTTON, NULL } });

            Bitmap bitmap(20, 20);
            bitmap.modify(9, 4, 16, true);
            bitmap.modify(10, 4, 16, true);
            string s = USER_PATH_PREFIX("/min.png");
            bitmap.exportToPng(s.c_str());

            min_button.set_backround_png(USER_PATH_PREFIX("/min.png"));

            event_handler->setEventCallback(XCB_ENTER_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_enter_notify_event_t);
                if (e->event == this->min_button) {
                    this->min_button.change_border_color(WHITE);
                }
            });

            event_handler->setEventCallback(XCB_LEAVE_NOTIFY, [&](Ev ev) {
                RE_CAST_EV(xcb_leave_notify_event_t);
                if (e->event == this->min_button) {
                    this->min_button.change_border_color(BLACK);
                }
            });

        }
        void make_borders() {
            border[left].create_window(
                frame,
                0,
                BORDER_SIZE,
                BORDER_SIZE,
                (height - (BORDER_SIZE * 2)),
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::left_side

            ); CWC(border[left]);
            border[left].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[right].create_window(
                frame,
                (width - BORDER_SIZE),
                BORDER_SIZE,
                BORDER_SIZE,
                (height - (BORDER_SIZE * 2)),
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::right_side

            ); CWC(border[right]);
            border[right].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[top].create_window(
                frame,
                BORDER_SIZE,
                0,
                (width - (BORDER_SIZE * 2)),
                BORDER_SIZE,
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::top_side

            ); CWC(border[top]);
            border[top].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[bottom].create_window(
                frame,
                BORDER_SIZE,
                (height - BORDER_SIZE),
                (width  - (BORDER_SIZE * 2)),
                BORDER_SIZE,
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::bottom_side

            ); CWC(border[bottom]);
            border[bottom].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[top_left].create_window(
                frame,
                0,
                0,
                BORDER_SIZE,
                BORDER_SIZE,
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::top_left_corner

            ); CWC(border[top_left]);
            border[top_left].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[top_right].create_window(
                frame,
                (width - BORDER_SIZE),
                0,
                BORDER_SIZE,
                BORDER_SIZE,
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::top_right_corner

            ); CWC(border[top_right]);
            border[top_right].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[bottom_left].create_window(
                frame,
                0,
                (height - BORDER_SIZE),
                BORDER_SIZE,
                BORDER_SIZE,
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::bottom_left_corner

            ); CWC(border[bottom_left]);
            border[bottom_left].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

            border[bottom_right].create_window(
                frame,
                (width - BORDER_SIZE),
                (height - BORDER_SIZE),
                BORDER_SIZE,
                BORDER_SIZE,
                BLACK,
                NONE,
                MAP,
                nullptr,
                CURSOR::bottom_right_corner

            ); CWC(border[bottom_right]);
            border[bottom_right].grab_button({ { L_MOUSE_BUTTON, NULL } });
            FLUSH_X();

        }
        void set_icon_png() {
            icon.create_window(
                frame,
                BORDER_SIZE,
                BORDER_SIZE,
                BUTTON_SIZE,
                BUTTON_SIZE,
                BLACK,
                NONE,
                MAP
            
            );
            CWC(icon);

            win.make_png_from_icon();
            icon.set_backround_png(PNG_HASH(win.get_icccm_class()));

        }
    
    /* Variables   */
        bool CLOSE_BUTTON_BITMAP[20][20] {
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

        
};
class desktop {
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
class Key_Codes {
    public:
    // constructor and destructor.
        Key_Codes() 
        : keysyms(nullptr) {}

        ~Key_Codes() {
            free(keysyms);
        }

    // methods.
        void init()
        {
            keysyms = xcb_key_symbols_alloc( conn );
            if ( keysyms )
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
                    { F12,          &f12       },
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
                    { SUPER_L,      &super_l   },
                    { MINUS,        &minus     },
                    { UNDERSCORE,   &underscore}
                };
                for ( auto &pair : key_map )
                {
                    xcb_keycode_t *keycode = xcb_key_symbols_get_keycode( keysyms, pair.first );
                    if ( keycode )
                    { 
                        *( pair.second ) = *keycode;
                        free( keycode );
                    }
                }
            }
        }

    // variabels.
        xcb_keycode_t
            a{}, b{}, c{}, d{}, e{}, f{}, g{}, h{}, i{}, j{}, k{}, l{}, m{},
            n{}, o{}, p{}, q{}, r{}, s{}, t{}, u{}, v{}, w{}, x{}, y{}, z{},
            
            space_bar{}, enter{},

            f11{}, f12{},
            
            n_1{}, n_2{}, n_3{}, n_4{}, n_5{}, r_arrow{},
            l_arrow{}, u_arrow{}, d_arrow{}, tab{}, _delete{},
            super_l{}, minus{}, underscore{};

    private:
    // variabels.
        xcb_key_symbols_t *keysyms;
};
class Entry {
    public:
    /* Constructor */
        Entry() {}

    /* Variabels */
        window window;
        bool menu = false;
        string name;
        function<void()> action;

    /* Methods */
        void make_window(uint32_t __parent, int16_t __x, int16_t __y, uint16_t __width, uint16_t __height) {
            window.create_window(
                __parent,
                __x,
                __y,
                __width,
                __height,
                BLACK,
                BUTTON_EVENT_MASK,
                MAP

            );
            CONN(EXPOSE, if (__window == this->window) this->window.draw_acc(name);, this->window);
            CONN(L_MOUSE_BUTTON_EVENT, if (__window == this->window && this->action != nullptr) this->action(); WS_emit(window.parent(), HIDE_CONTEXT_MENU);, this->window);
            CONN(ENTER_NOTIFY, if (__window == this->window) this->window.change_backround_color(WHITE);, this->window);
            CONN(LEAVE_NOTIFY, if (__window == this->window) this->window.change_backround_color(BLACK);, this->window);
            window.grab_button({ { L_MOUSE_BUTTON, NULL } });

        }

};
class context_menu {
    private:
    /* Variabels */
        int16_t  _x = 0, _y = 0;
        uint32_t _width = 120, _height = 20;

        int border_size = 1;
        vector<Entry>(entries);

    /* Methods   */
        void create_dialog_win__() {
            context_window.create_window(
                screen->root,
                0,
                0,
                _width,
                _height,
                DARK_GREY,
                XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION,
                RAISE
            );
            CONN(L_MOUSE_BUTTON_EVENT, WS_emit(this->context_window, HIDE_CONTEXT_MENU);, this->context_window);

        }
        void hide__() {
            context_window.unmap();
            context_window.kill();

            for (int i = 0; i < entries.size(); ++i) {
                entries[i].window.kill();
            }
        }
        void make_entries__() {
            for (int i(0), y(0); i < entries.size(); ++i, y += _height) {
                entries[i].make_window(context_window, 0, y, _width, _height);
                signal_manager->_window_signals.emit(entries[i].window, EXPOSE);
            }
        }
    
    public:
        window context_window;
    
        void show() {
            _x = m_pointer->x();
            _y = m_pointer->y();
            
            for (int i(0), max_len(0); i < entries.size(); ++i) {
                if (entries[i].name.length() > max_len) {
                    max_len = entries[i].name.length();
                    _width = ((max_len + 2) * DEFAULT_FONT_WIDTH);
                }
            }
            uint16_t new_height = (entries.size() * _height);

            if (_y + new_height > screen->height_in_pixels) {
                _y = (screen->height_in_pixels - new_height);
            }
            if (_x + _width > screen->width_in_pixels) {
                _x = (screen->width_in_pixels - _width);
            }

            context_window.x_y_width_height((_x - BORDER_SIZE), (_y - BORDER_SIZE), _width, new_height);
            context_window.map();
            context_window.raise();
            make_entries__();

            CONN(HIDE_CONTEXT_MENU, this->hide__();, this->context_window);

        }
        void add_entry(string name, function<void()> action) {
            Entry entry;
            entry.name = name;
            entry.action = action;
            entries.push_back(entry);
        }
    
    context_menu() { create_dialog_win__(); }

};
class Window_Manager {
    /* Defines     */
        #define INIT_NEW_WM(__inst, __class) \
            __inst = new __class;                                               \
            if (__inst == nullptr)                                              \
            {                                                                   \
                loutE << "failed to allocate memory for wm exeting" << loutEND; \
                return;                                                         \
            }                                                                   \
            else
    public:
    /* Constructor */
        Window_Manager() {}
    
    /* Variabels   */
        window root;
        Launcher launcher;
        pointer pointer;
        win_data data;
        Key_Codes key_codes;
        
        context_menu *context_menu = nullptr;
        vector<client *> client_list;
        vector<desktop *> desktop_list;
        client *focused_client = nullptr;
        desktop *cur_d = nullptr;

    /* Methods     */
        /* Main         */
            void init()
            {
                _conn(nullptr, nullptr);
                _setup();
                _iter();
                _screen();
                xcb = connect_to_server( conn, screen );
                if (( xcb->check_conn() & ( 1ULL << X_CONN_ERROR )) != 0 )
                {
                    loutE << "x not connected" << loutEND;
                }
                else
                {
                    loutI << "x succesfully connected" << loutEND;
                }

                root = screen->root;
                root.width( screen->width_in_pixels );
                root.height( screen->height_in_pixels );

                setSubstructureRedirectMask();
                configure_root();
                _ewmh();
                
                key_codes.init();
                event_handler = new __event_handler__();
                
                create_new_desktop( 1 );
                create_new_desktop( 2 );
                create_new_desktop( 3 );
                create_new_desktop( 4 );
                create_new_desktop( 5 );

                context_menu = new class context_menu();
                context_menu->add_entry( "konsole",              [ this ]() -> void { launcher.program( (char *) "konsole" ); });
                context_menu->add_entry( "google-chrome-stable", [ this ]() -> void { launcher.launch_child_process( "google-chrome-stable" ); });
                context_menu->add_entry( "code",                 [ this ]() -> void { launcher.launch_child_process( "code" ); });
                context_menu->add_entry( "dolphin",              [ this ]() -> void { launcher.launch_child_process( "dolphin" ); });

                setup_events(); 

            }
            void quit( int __status )
            {
                pid_manager->kill_all_pids();
                xcb_flush( conn );
                delete_client_vec( client_list );
                delete_desktop_vec( desktop_list );
                xcb_ewmh_connection_wipe( ewmh );
                xcb_disconnect( conn );
                exit( __status );
            }
            void get_atom(char *name, xcb_atom_t *atom) {
                xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, xcb_intern_atom(conn, 0, slen(name), name), NULL);
                if (reply != NULL) {
                    *atom = reply->atom;

                } else {
                    *atom = XCB_NONE;

                } free(reply);

            }
            void focus_none() {
                VOID_COOKIE = xcb_set_input_focus(
                    conn,
                    XCB_NONE,
                    screen->root,
                    XCB_CURRENT_TIME
                );
                CHECK_VOID_COOKIE();
                FLUSH_X();
            }

        /* Window       */
            bool window_exists(uint32_t __window) {
                xcb_generic_error_t *err;
                free(xcb_query_tree_reply(conn, xcb_query_tree(conn, __window), &err));

                if (err != NULL) {
                    free(err);
                    return false;

                } return true;

            }
            void window_stack(uint32_t __window1, uint32_t __window2, uint32_t __mode) {
                if (__window2 == XCB_NONE) return;
                
                uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
                uint32_t values[] = {__window2, __mode};
                
                xcb_configure_window(conn, __window1, mask, values);
                
            }
            void window_above(const uint32_t &__window1, const uint32_t &__window2) {
                window_stack(
                    __window1,
                    __window2,
                    XCB_STACK_MODE_ABOVE

                );

            }/* stack '__window1' above '__window2' */
            void window_below(const uint32_t &__window1, const uint32_t &__window2) {
                window_stack(
                    __window1,
                    __window2,
                    XCB_STACK_MODE_BELOW
                
                );

            }/* stack '__window1' below '__window2' */
            void unmap_window(uint32_t __window) {
                VOID_COOKIE = xcb_unmap_window(conn, __window); CHECK_VOID_COOKIE();
                FLUSH_X();

            }

        /* Client       */
            /* Focus */
                void cycle_focus() {
                    if (focused_client == nullptr) {
                        if (cur_d->current_clients.size() == 0) return;
                        for (long i(0); i < cur_d->current_clients.size(); ++i) {
                            if (cur_d->current_clients[i] == nullptr) continue;
                            
                            cur_d->current_clients[i]->focus();
                            focused_client = cur_d->current_clients[i];
                            
                            return;

                        }

                    }
                    for (int i(0); i < cur_d->current_clients.size(); ++i) {
                        if (cur_d->current_clients[i] == nullptr) continue;
                        if (cur_d->current_clients[i] == focused_client) {
                            if (i == (cur_d->current_clients.size() - 1)) {
                                cur_d->current_clients[0]->focus();
                                focused_client = cur_d->current_clients[0];
                            
                                return;

                            }
                            cur_d->current_clients[i + 1]->focus();
                            focused_client = cur_d->current_clients[i + 1];

                            return;

                        } if (i == (cur_d->current_clients.size() - 1)) i = 0;

                    }

                }
                void unfocus() {
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
                    focused_client = nullptr;

                }

            /* Fetch */
                client *client_from_window(const xcb_window_t *window) {
                    for (const auto &c:client_list) {
                        if (*window == c->win) {
                            return c;
                        }
                    }
                    return nullptr;
                }
                client *client_from_any_window(const xcb_window_t *window) {
                    for (const auto &c:client_list) {
                        if (*window == c->win
                        ||  *window == c->frame
                        ||  *window == c->titlebar
                        ||  *window == c->close_button
                        ||  *window == c->max_button
                        ||  *window == c->min_button
                        ||  *window == c->border[left]
                        ||  *window == c->border[right]
                        ||  *window == c->border[top]
                        ||  *window == c->border[bottom]
                        ||  *window == c->border[top_left]
                        ||  *window == c->border[top_right]
                        ||  *window == c->border[bottom_left]
                        ||  *window == c->border[bottom_right]) {
                            return c;

                        }

                    } return nullptr;

                }
                client *client_from_pointer(const int &prox) {
                    const uint32_t &x = pointer.x();
                    const uint32_t &y = pointer.y();
                    for (const auto &c : cur_d->current_clients) {
                        // LEFT EDGE OF CLIENT
                        if (x > c->x - prox && x <= c->x) return c;
                        
                        // RIGHT EDGE OF CLIENT
                        if (x >= c->x + c->width && x < c->x + c->width + prox) return c;
                        
                        // TOP EDGE OF CLIENT
                        if (y > c->y - prox && y <= c->y) return c;
                        
                        // BOTTOM EDGE OF CLIENT
                        if (y >= c->y + c->height && y < c->y + c->height + prox) return c;

                    } return nullptr;

                }
                map<client *, edge> get_client_next_to_client(client *c, edge c_edge) {
                    map<client *, edge> map;
                    for (client *c2:cur_d->current_clients) {
                        if (c == c2) continue;

                        if (c_edge == edge::LEFT) {
                            if(c->x == c2->x + c2->width) {
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

                    } map[nullptr] = edge::NONE;
                    
                    return map;

                }
                edge get_client_edge_from_pointer(client *c, const int &prox) {
                    const uint32_t &x = pointer.x();
                    const uint32_t &y = pointer.y();

                    const uint32_t &top_border    = c->y;
                    const uint32_t &bottom_border = (c->y + c->height);
                    const uint32_t &left_border   = c->x;
                    const uint32_t &right_border  = (c->x + c->width);
 
                    if (((y > top_border - prox) && (y <= top_border))
                    && ((x > left_border + prox) && (x < right_border - prox))) {
                        return edge::TOP;

                    } /* TOP EDGE OF CLIENT */

                    if (((y >= bottom_border) && (y < bottom_border + prox))
                    && ((x > left_border + prox) && (x < right_border - prox))) {
                        return edge::BOTTOM_edge;

                    } /* BOTTOM EDGE OF CLIENT */
                    
                    if (((x > left_border) - prox && (x <= left_border))
                    && ((y > top_border + prox) && (y < bottom_border - prox))) {
                        return edge::LEFT;

                    } /* LEFT EDGE OF CLIENT */
                    
                    if (((x >= right_border) && (x < right_border + prox))
                    && ((y > top_border + prox) && (y < bottom_border - prox))) {
                        return edge::RIGHT;

                    } /* RIGHT EDGE OF CLIENT */
 
                    if (((x > left_border - prox) && x < left_border + prox)
                    && ((y > top_border - prox) && y < top_border + prox)) {
                        return edge::TOP_LEFT;

                    } /* TOP LEFT CORNER OF CLIENT */

                    if (((x > right_border - prox) && x < right_border + prox)
                    && ((y > top_border - prox) && y < top_border + prox)) {
                        return edge::TOP_RIGHT;

                    } /* TOP RIGHT CORNER OF CLIENT */

                    if (((x > left_border - prox) && x < left_border + prox) 
                    && ((y > bottom_border - prox) && y < bottom_border + prox)) {
                        return edge::BOTTOM_LEFT;

                    } /* BOTTOM LEFT CORNER OF CLIENT */

                    if (((x > right_border - prox) && x < right_border + prox)
                    && ((y > bottom_border - prox) && y < bottom_border + prox)) {
                        return edge::BOTTOM_RIGHT;

                    } /* BOTTOM RIGHT CORNER OF CLIENT */

                    return edge::NONE;

                }
                client *get_client_from_pointer() {
                    const int16_t x = pointer.x();
                    const int16_t y = pointer.y();

                    for (client *const &c : cur_d->current_clients) {
                        if (x > c->x && x < c->x + c->width
                        &&  y > c->y && y < c->y + c->height) {
                            return c;

                        }

                    } return nullptr;

                }

            void manage_new_client(const uint32_t &__window) {
                client *c = make_client(__window);
                if (c == nullptr) {
                    loutE << "could not make client" << loutEND;
                    return;
                }

                c->win.get_override_redirect();
                c->win.x_y_width_height(c->x, c->y, c->width, c->height);
                FLUSH_X();
                pid_manager->check_pid(c->win.get_pid());

                c->win.map();
                c->win.grab_button({
                    { L_MOUSE_BUTTON, ALT },
                    { R_MOUSE_BUTTON, ALT },
                    { L_MOUSE_BUTTON, 0   }
                });

                if (!c->win.check_frameless_window_hint()) {
                    c->make_decorations();
                    c->frame.set_event_mask(FRAME_EVENT_MASK);
                    c->win.set_event_mask(CLIENT_EVENT_MASK);
                    c->update();
                    c->focus();
                    focused_client = c;
                    check_client(c);
                    c->frame.grab_default_keys();
                    c->add_to_map();
                }
                c->win.grab_default_keys();

            }
            client *make_internal_client(window &window) {
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
            void send_sigterm_to_client(client *c) {
                c->kill();
                remove_client(c);

            }
            void remove_client(client *c) {
                client_list.erase(remove(
                    client_list.begin(),
                    client_list.end(),
                    c

                ), client_list.end());

                cur_d->current_clients.erase(remove(
                    cur_d->current_clients.begin(),
                    cur_d->current_clients.end(),
                    c

                ), cur_d->current_clients.end());

                delete c;

            }

        /* Desktop      */
            void create_new_desktop(const uint16_t &n)
            {
                desktop *d = new desktop;
                
                d->desktop = n;
                d->width   = screen->width_in_pixels;
                d->height  = screen->height_in_pixels;
                cur_d      = d;

                desktop_list.push_back(d);
            }

        /* Experimental */
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

        /* Xcb          */
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
    /* Functions   */
        /* Init   */
            void _conn(const char *displayname, int *screenp)
            {
                conn = xcb_connect( displayname, screenp );
                check_conn();
            }
            void _ewmh()
            {
                if ( !( ewmh = static_cast<xcb_ewmh_connection_t *>( calloc( 1, sizeof( xcb_ewmh_connection_t )))))
                {
                    loutE << "ewmh faild to initialize" << loutEND;
                    quit( 1 );
                }                    
                
                xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms( conn, ewmh );
                if ( !( xcb_ewmh_init_atoms_replies( ewmh, cookie, 0 )))
                {
                    loutE << "xcb_ewmh_init_atoms_replies:faild" << loutEND;
                    quit( 1 );
                }
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
                xcb_generic_error_t *error = xcb_request_check( conn, cookie );
                if ( !error ) return true;
                
                loutE << "Error: Another window manager is already running or failed to set SubstructureRedirect mask" << loutEND;
                free( error );
                return false;
            }
            void configure_root()
            {
                root.set_backround_color( DARK_GREY );
                root.set_event_mask( ROOT_EVENT_MASK );
                root.grab_keys( {
                    { Q,       SHIFT  | ALT   },
                    { T,       CTRL   | ALT   },
                    { L_ARROW, CTRL   | SUPER },
                    { R_ARROW, CTRL   | SUPER },
                    { F12,     NULL           },
                    { SUPER_L, SHIFT          }
                
                }); root.clear();

                #ifndef ARMV8_BUILD
                    root.set_backround_png( USER_PATH_PREFIX( "/mwm_png/galaxy21.png" ));
                    // root.set_backround_png(USER_PATH_PREFIX("/mwm_png/galaxy16-17-3840x1200.png"));
                #endif
                root.set_pointer( CURSOR::arrow );

            }
            void setup_events()
            {
                event_handler->setEventCallback( XCB_MAP_REQUEST, [ & ]( Ev ev ) -> void
                {
                    RE_CAST_EV( xcb_map_request_event_t );
                    client *c = signal_manager->_window_client_map.retrive( e->window );
                    if ( c ) return;
                    this->manage_new_client( e->window );
                });
                event_handler->setEventCallback( XCB_BUTTON_PRESS, [ & ]( Ev ev ) -> void
                {
                    RE_CAST_EV( xcb_button_press_event_t );
                    if ( e->event != screen->root ) return;
                    
                    if ( e->detail == L_MOUSE_BUTTON )
                    {
                        this->unfocus();
                        if ( this->context_menu->context_window.is_mapped() )
                        {
                            Emit( this->context_menu->context_window, HIDE_CONTEXT_MENU );
                        }
                    }
                    else if ( e->detail == R_MOUSE_BUTTON )
                    {
                        this->context_menu->show();    
                    }
                    
                });
                event_handler->setEventCallback( XCB_KEY_PRESS, [ & ]( Ev ev ) -> void
                {
                    RE_CAST_EV( xcb_key_press_event_t );
                    if ( e->detail == key_codes.tab && (( e->state & ALT ) != 0 ))
                    {
                        this->cycle_focus();
                    }
                    else if ( e->detail == key_codes.t && (( e->state & ALT ) != 0 ) && (( e->state & CTRL ) != 0 ))
                    {
                        this->launcher.launch_child_process( "konsole" );
                    }
                    else if ( e->detail == key_codes.q && (( e->state & ALT ) != 0 ) && (( e->state & SHIFT ) != 0 ))
                    {
                        this->quit( 0 );
                    }
                });
                event_handler->setEventCallback( XCB_FOCUS_IN, [ & ]( Ev ev ) -> void
                {
                    RE_CAST_EV(xcb_focus_in_event_t);
                    client *c = signal_manager->_window_client_map.retrive(e->event);
                    if (c) {
                        focused_client = c;
                        if (!c->win.is_active_EWMH_window()) {
                            loutE << "c->win is not 'EWMH active window'" << loutEND;
                        }
                    }
                });
                
                if (BORDER_SIZE == 0) {
                    signal_manager->emit("SET_EV_CALLBACK__RESIZE_NO_BORDER");
                    
                }
            }

        /* Check  */
            void check_error(const int &code) {
                switch (code) {
                    case CONN_ERR: {
                        log_error("Connection error.");
                        quit(CONN_ERR);
                        break;

                    }
                    case EXTENTION_NOT_SUPPORTED_ERR: {
                        log_error("Extension not supported.");
                        quit(EXTENTION_NOT_SUPPORTED_ERR);
                        break;

                    }                        
                    case MEMORY_INSUFFICIENT_ERR: {
                        log_error("Insufficient memory.");
                        quit(MEMORY_INSUFFICIENT_ERR);
                        break;

                    }                        
                    case REQUEST_TO_LONG_ERR: {
                        log_error("Request to long.");
                        quit(REQUEST_TO_LONG_ERR);
                        break;

                    }
                    case PARSE_ERR: {
                        log_error("Parse error.");
                        quit(PARSE_ERR);
                        break;

                    }
                    case SCREEN_NOT_FOUND_ERR: {
                        log_error("Screen not found.");
                        quit(SCREEN_NOT_FOUND_ERR);
                        break;

                    }
                    case FD_ERR: {
                        log_error("File descriptor error.");
                        quit(FD_ERR);
                        break;

                    }

                }

            }
            void check_conn() {
                int status = xcb_connection_has_error(conn);
                check_error(status);
            
            }
            int cookie_error(xcb_void_cookie_t cookie , const char *sender_function) {
                xcb_generic_error_t *err = xcb_request_check(conn, cookie);
                uint8_t err_code = 0;
                if (err) {
                    err_code = err->error_code;
                    free(err);

                } return err_code;
            
            }
            void check_error(xcb_void_cookie_t cookie , const char *sender_function, const char *err_msg) {
                xcb_generic_error_t * err = xcb_request_check(conn, cookie);
                if (err) {
                    log_error_code(err_msg, err->error_code);
                    free(err);

                }

            }

        /* Delete */
            void delete_client_vec(vector<client *> &vec) {
                for (client *c : vec) {
                    send_sigterm_to_client(c);
                    xcb_flush(conn);
                    
                } vec.clear();
                vector<client *>().swap(vec);
            
            }
            void delete_desktop_vec(vector<desktop *> &vec) {
                for (desktop *d : vec) {
                    delete_client_vec(d->current_clients);
                    delete d;

                } vec.clear();
                vector<desktop *>().swap(vec);
                
            }
            template <typename Type>
            static void delete_ptr_vector(vector<Type *>& vec) {
                for (Type *ptr : vec) {
                    delete ptr;

                } vec.clear(); vector<Type *>().swap(vec);

            }
            void remove_client_from_vector(client * c, vector<client *> &vec) {
                if (c == nullptr) {
                    loutE << "client is nullptr." << loutEND;
                    return;

                } vec.erase(std::remove(
                    vec.begin(),
                    vec.end(),
                    c
                
                ), vec.end());
                delete c;

            }

        /* Client */
            client *make_client(const uint32_t &window) {
                client *c = new client;
                if (c == nullptr) {
                    loutE << "Could not allocate memory for client" << '\n';
                    return nullptr;

                }

                c->win = window;
                c->set_client_params();

                if (c->width  < c->win.get_min_width() ) c->width  = 200;
                if (c->height < c->win.get_min_height()) c->height = 100;
                
                c->depth   = 24;
                c->desktop = cur_d->desktop;

                if (c->x <= 0 && c->y <= 0 && c->width != screen->width_in_pixels && c->height != screen->height_in_pixels) {
                    c->x = ((screen->width_in_pixels - c->width) / 2);
                    c->y = (((screen->height_in_pixels - c->height) / 2) + (BORDER_SIZE * 2));

                }
                if (c->height > screen->height_in_pixels) c->height = screen->height_in_pixels;
                if (c->width  > screen->width_in_pixels ) c->width  = screen->width_in_pixels;

                if (c->win.is_EWMH_fullscreen()) {
                    c->x      = 0;
                    c->y      = 0;
                    c->width  = screen->width_in_pixels;
                    c->height = screen->height_in_pixels;
                    c->win.set_EWMH_fullscreen_state();

                }
                if (c->win.check_atom(ewmh->_NET_WM_STATE_MODAL)) {
                    c->atoms.is_modal = true;
                    c->modal_data.transient_for = c->win.get_transient_for_window();

                }
                client_list.push_back(c);
                cur_d->current_clients.push_back(c);
                return c;

            }
            void check_client(client *c) {
                c->win.x(BORDER_SIZE);
                FLUSH_X();

                c->win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
                FLUSH_X();

                // if client if full_screen but 'y' is offset for some reason, make 'y' (0)
                if (c->x == 0
                &&  c->y != 0
                &&  c->width == screen->width_in_pixels
                &&  c->height == screen->height_in_pixels) {
                    c->_y(0);
                    FLUSH_X();
                    return;

                }
                // if client is full_screen 'width' and 'height' but position is offset from (0, 0) then make pos (0, 0)
                if (c->x != 0
                &&  c->y != 0
                &&  c->width == screen->width_in_pixels
                &&  c->height == screen->height_in_pixels) {
                    c->x_y(0,0);
                    FLUSH_X();
                    return;

                }
                if (c->x < 0) {
                    c->_x(0);
                    FLUSH_X();

                }
                if (c->y < 0) {
                    c->_y(0);
                    FLUSH_X();

                }
                if (c->width > screen->width_in_pixels) {
                    c->_width(screen->width_in_pixels);
                    FLUSH_X();

                }
                if (c->height > screen->height_in_pixels) {
                    c->_height(screen->height_in_pixels);
                    FLUSH_X();

                }                
                if ((c->x + c->width) > screen->width_in_pixels) {
                    c->_width(screen->width_in_pixels - c->x);
                    FLUSH_X();

                }                
                if ((c->y + c->height) > screen->height_in_pixels) {
                    c->_height(screen->height_in_pixels - c->y);
                    FLUSH_X();

                }
            }

        /* Window */
            void get_window_parameters(const uint32_t &__window, int16_t *__x, int16_t *__y, uint16_t *__width,  uint16_t *__height) {
                xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, __window);
                xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, nullptr);
                if (reply == nullptr) {
                    loutE << "Unable to get window geometry." << loutEND;
                    return;
                
                }
                *__x      = reply->x;
                *__y      = reply->y;
                *__width  = reply->width;
                *__height = reply->height;
                free(reply);

            }

        /* Status */
            void check_volt() {
                loutE << "running" << loutEND;
                this_thread::sleep_for(chrono::minutes(1));
                check_volt();

            }

}; static Window_Manager *wm(nullptr);

class __network__ {
    public:
    /* Methods     */
        enum {
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

    /* Constructor */
        __network__() {}

}; static __network__ *network(nullptr);

class __wifi__ {
    private:
    /* Methods     */
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
                loutE << "could not open socket" << loutEND;
                return;
            }

            // Get the range of settings
            if (iw_get_range_info(sock, interface, &range) < 0)
            {
                loutE << "could not get range info" << loutEND;
                iw_sockets_close(sock);
                return;
            }

            // Perform the scan
            if (iw_scan(sock, const_cast<char*>(interface), range.we_version_compiled, &head) < 0)
            {
                loutE << "could not scan" << loutEND;
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
                    loutI <<
                        "\nSSID: "    << ssid                     <<
                        "\nSTATUS: "  << result->stats.status     <<
                        "\nQUAL: "    << result->stats.qual.qual  <<
                        "\nLEVEL: "   << result->stats.qual.level <<
                        "\nNOICE: "   << result->stats.qual.noise <<
                        "\nBITRATE: " << result->maxbitrate.value <<
                    loutEND;
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
                        loutI << "Interface: " << ifa->ifa_name << " Address: " << addressBuffer << loutEND;
                    }
                }
                else if (ifa->ifa_addr->sa_family == AF_INET6) // check it is IP6
                {
                    // is a valid IP6 Address
                    tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
                    char addressBuffer[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                    loutI << "Interface: " << ifa->ifa_name << " Address: " << addressBuffer << loutEND;
                } 
                // Here, you could attempt to deduce the interface type (e.g., wlan for WiFi) by name or other means
            }

            if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
            return;
        }

    public:
    /* Methods     */
        void init()
        {
            event_handler->set_key_press_callback((ALT + SUPER), wm->key_codes.f, [this]()-> void { scan__("wlo1"); });
        }

    /* Constructor */
        __wifi__() {}

}; static __wifi__ *wifi(nullptr);

class __audio__ {
    private:
    /* Variabels   */
        pa_mainloop* mainloop = nullptr;
        pa_mainloop_api* mainloop_api = nullptr;
        pa_context* context = nullptr;

    public:
    /* Methods     */
        static void sink_list_cb(pa_context* c, const pa_sink_info* l, int eol, void* userdata)
        {
            // End of list
            if (eol > 0)
            {
                // loutE << "end of list" << '\n';
                return;
            }

            // loutI << "Sink name: " << l->name << " Description: " << l->description << '\n';
        }

        static void success_cb(pa_context* c, int success, void* userdata)
        {
            if (success) {
                // loutI << "Default sink changed successfully" << '\n';

            } else {
                // loutE << "Failed to change the default sink" << '\n';

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
                loutE << "Failed to run mainloop." << loutEND;
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
        
    /* Constructor */
        __audio__() {}

}; __audio__ audio;

namespace {
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
class __status_bar__ {
    private:
    // Methods.
        void create_windows__() {
            _w[_BAR].create_window(
                screen->root,
                BAR_WINDOW_X,
                BAR_WINDOW_Y,
                BAR_WINDOW_WIDTH,
                BAR_WINDOW_HEIGHT,
                DARK_GREY,
                NONE,
                MAP

            );
            _w[_TIME_DATE].create_window(
                _w[_BAR],
                TIME_DATE_WINDOW_X,
                TIME_DATE_WINDOW_Y,
                TIME_DATE_WINDOW_WIDTH,
                TIME_DATE_WINDOW_HEIGHT,
                DARK_GREY,
                XCB_EVENT_MASK_EXPOSURE,
                MAP

            );
            CONN_Win(_w[_TIME_DATE], EXPOSE,
                if (__window != this->_w[_TIME_DATE]) return;
                this->_w[_TIME_DATE].draw_acc(this->get_time_and_date__());

            );
            /* expose_tasks.push_back({this->_w[_TIME_DATE], [&]() {
                this->_w[_TIME_DATE].draw_acc(this->get_time_and_date__());
            }}); */
            _w[_WIFI].create_window(
                _w[_BAR],
                WIFI_WINDOW_X,
                WIFI_WINDOW_Y,
                WIFI_WINDOW_WIDTH,
                WIFI_WINDOW_HEIGHT,
                DARK_GREY,
                XCB_EVENT_MASK_BUTTON_PRESS,
                MAP

            );
            WS_conn(_w[_WIFI], L_MOUSE_BUTTON_EVENT, W_callback {
                if (__window != this->_w[_WIFI]) return;

                if (this->_w[_WIFI_DROPWOWN].is_mapped()) {
                    this->hide__(this->_w[_WIFI_DROPWOWN]);
                }
                else {
                    if (this->_w[_AUDIO_DROPDOWN].is_mapped()) {
                        this->hide__(this->_w[_AUDIO_DROPDOWN]);
                    }
                    show__(this->_w[_WIFI_DROPWOWN]);
                    this->_w[_WIFI_INFO].send_event(XCB_EVENT_MASK_EXPOSURE);
                    this->_w[_WIFI_CLOSE].send_event(XCB_EVENT_MASK_EXPOSURE);
                }

            });

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

            string s = USER_PATH_PREFIX("/wifi.png");
            bitmap.exportToPng(s.c_str());
            _w[_WIFI].set_backround_png(USER_PATH_PREFIX("/wifi.png"));
            _w[_WIFI].set_pointer(CURSOR::hand2);
            _w[_AUDIO].create_window(
                _w[_BAR],
                (WIFI_WINDOW_X - 50),
                0,
                50,
                20,
                DARK_GREY,
                BUTTON_EVENT_MASK,
                MAP,
                (int[]){ALL, 2, BLACK},
                CURSOR::hand2

            );
            WS_conn( this->_w[_AUDIO], EXPOSE, W_callback {
                if ( __window != this->_w[_AUDIO] ) return;
                this->_w[_AUDIO].draw( "Audio" ); 

            });/* 
            expose_tasks.push_back({this->_w[_AUDIO], [&]() { this->_w[_AUDIO].draw("Audio"); }}); */
            /* this->_w[_AUDIO].send_event(XCB_EVENT_MASK_EXPOSURE); */
            WS_conn(this->_w[_AUDIO], L_MOUSE_BUTTON_EVENT, W_callback {
                if (__window != this->_w[_AUDIO]) return;

                if (this->_w[_AUDIO_DROPDOWN].is_mapped()) {
                    this->hide__(this->_w[_AUDIO_DROPDOWN]);
                
                } else {
                    if (this->_w[_WIFI_DROPWOWN].is_mapped()) {
                        this->hide__(this->_w[_WIFI_DROPWOWN]);

                    } this->show__(this->_w[_AUDIO_DROPDOWN]);

                }

            });
            WS_conn(this->_w[_AUDIO], ENTER_NOTIFY, W_callback {
                if (__window != this->_w[_AUDIO]) return;
                this->_w[_AUDIO].change_backround_color(WHITE);

            });
            WS_conn(this->_w[_AUDIO], LEAVE_NOTIFY, W_callback {
                if (__window != this->_w[_AUDIO]) return;
                this->_w[_AUDIO].change_backround_color(DARK_GREY);

            });

        }
        void show__(const uint32_t &__window) {
            if (__window == _w[_WIFI_DROPWOWN])
            {
                _w[_WIFI_DROPWOWN].create_window(
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
                
                _w[_WIFI_CLOSE].create_window(
                    _w[_WIFI_DROPWOWN],
                    WIFI_CLOSE_WINDOW_X,
                    WIFI_CLOSE_WINDOW_Y,
                    WIFI_CLOSE_WINDOW_WIDTH,
                    WIFI_CLOSE_WINDOW_HEIGHT,
                    DARK_GREY,
                    BUTTON_EVENT_MASK,
                    MAP,
                    (int[3]){ALL, WIFI_DROPDOWN_BORDER, BLACK},
                    CURSOR::hand2
                );

                WS_conn(this->_w[_WIFI_CLOSE], L_MOUSE_BUTTON_EVENT, W_callback
                {
                    if (__window != this->_w[_WIFI_CLOSE]) return;
                    this->hide__(this->_w[_WIFI_DROPWOWN]);
                });

                WS_conn(this->_w[_WIFI_CLOSE], EXPOSE, W_callback
                {
                    if (__window != this->_w[_WIFI_CLOSE]) return;
                    this->_w[_WIFI_CLOSE].draw_acc("Close");
                });

                this->_w[_WIFI_CLOSE].send_event(XCB_EVENT_MASK_EXPOSURE);

                WS_conn(this->_w[_WIFI_CLOSE], ENTER_NOTIFY, W_callback
                {
                    if (__window != this->_w[_WIFI_CLOSE]) return;
                    this->_w[_WIFI_CLOSE].change_backround_color(WHITE);
                });

                WS_conn(this->_w[_WIFI_CLOSE], LEAVE_NOTIFY, W_callback
                {
                    if (__window != this->_w[_WIFI_CLOSE]) return;
                    this->_w[_WIFI_CLOSE].change_backround_color(DARK_GREY);
                });

                _w[_WIFI_INFO].create_window(
                    _w[_WIFI_DROPWOWN],
                    WIFI_INFO_WINDOW_X,
                    WIFI_INFO_WINDOW_Y,
                    WIFI_INFO_WINDOW_WIDTH,
                    WIFI_INFO_WINDOW_HEIGHT,
                    WHITE,
                    XCB_EVENT_MASK_EXPOSURE,
                    MAP,
                    (int[3]){ALL, WIFI_DROPDOWN_BORDER, BLACK}
                );

                WS_conn(this->_w[_WIFI_INFO], EXPOSE, W_callback
                {
                    string local_ip("Local ip: " + network->get_local_ip_info(__network__::LOCAL_IP));
                    this->_w[_WIFI_INFO].draw_text_auto_color(local_ip.c_str(), 4, 16, BLACK);

                    string local_interface("interface: " + network->get_local_ip_info(__network__::INTERFACE_FOR_LOCAL_IP));
                    this->_w[_WIFI_INFO].draw_text_auto_color(local_interface.c_str(), 4, 30, BLACK);
                });

                this->_w[_WIFI_INFO].send_event(XCB_EVENT_MASK_EXPOSURE);
            }
            
            if (__window == _w[_AUDIO_DROPDOWN])
            {
                _w[_AUDIO_DROPDOWN].create_window(
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
        void hide__(const uint32_t &__window) {
            if (__window == _w[_WIFI_DROPWOWN])
            {
                _w[_WIFI_CLOSE].unmap();
                _w[_WIFI_CLOSE].kill();
                _w[_WIFI_INFO].unmap();
                _w[_WIFI_INFO].kill();
                _w[_WIFI_DROPWOWN].unmap();
                _w[_WIFI_DROPWOWN].kill();
            }

            if (__window == _w[_AUDIO_DROPDOWN])
            {
                _w[_AUDIO_DROPDOWN].unmap();
                _w[_AUDIO_DROPDOWN].kill();
            }
        }
        void setup_thread__(const uint32_t &__window) {
            if (__window == _w[_TIME_DATE]) {
                function<void()> __time__ = [this]() -> void {
                    while (true) {
                        this->_w[_TIME_DATE].send_event(XCB_EVENT_MASK_EXPOSURE);
                        this_thread::sleep_for(chrono::seconds(1));

                    }

                }; thread(__time__).detach();

            }

        }

    public:
        string get_time_and_date__() {
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

        FixedArray<window, 8> _w{};
        typedef enum {
            _BAR,
            _TIME_DATE,
            _WIFI,
            _WIFI_DROPWOWN,
            _WIFI_CLOSE,
            _WIFI_INFO,
            _AUDIO,
            _AUDIO_DROPDOWN
        } window_type_t;

        void init() {
            create_windows__();
            setup_thread__(_w[_TIME_DATE]);
        }

        __status_bar__() {}

}; static __status_bar__ *status_bar(nullptr);

/**
 *
 * @class Mwm_Animator
 * @brief Class for animating the position and size of an XCB window.
 *
 */
class Mwm_Animator {
    public:
    // Constructors And Destructor.
        Mwm_Animator(const uint32_t &__window)
        : window(__window) {}

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
        uint32_t window;
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
                c->titlebar.send_event(XCB_EVENT_MASK_EXPOSURE);
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
                c->titlebar.send_event(XCB_EVENT_MASK_EXPOSURE);
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

class __animate__ {
    public:
    /* Methods */
        static void window(window &__window, int __end_x, int __end_y, int __end_width, int __end_height, int __duration)
        {
            Mwm_Animator anim(__window);
            anim.animate(
                __window.x(),
                __window.y(),
                __window.width(),
                __window.height(),
                __end_x,
                __end_y,
                __end_width,
                __end_height,
                __duration
            );
            __window.update(__end_x, __end_y, __end_width, __end_height);
        }
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

    ); c->update();

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

    ); c->update();

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

        // void run_action(const uint32_t & window)
        // {
        //     for (const auto &button : list)
        //     {
        //         if (window == button.window)
        //         {
        //             button.activate();
        //             return;
        //         }
        //     }
        // }
};

class search_window {
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
        vector<string> results;
        vector<window> entry_list;

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

};

/** NOTE: DEPRECATED */
class Mwm_Runner {
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
                launcher.program((char *) search_window.search_string.c_str());
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
            search_window.search_string.clear();
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
}; /* static Mwm_Runner * mwm_runner(nullptr); */

class add_app_dialog_window {
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

class __file_app__ {
    /* Defines     */
        #define FILE_APP_LEFT_MENU_WIDTH 120
        #define FILE_APP_LEFT_MENU_ENTRY_HEIGHT 20
        #define FILE_APP_BORDER_SIZE 2

    private:
    /* Subclasses  */
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

    /* Methods     */
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
    /* Variabels   */
        window(main_window);
        client *c = nullptr;

    /* Methods     */
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

    /* Constructor */
        __file_app__() {}
}; static __file_app__ *file_app;

class __screen_settings__ {
    private:
    /* Methods     */
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
                loutE << "Could not get screen resources" << endl;
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
        
        /**
         *
         * @brief Function to calculate the refresh rate from mode info
         *
         */
        static float calculate_refresh_rate(const xcb_randr_mode_info_t *mode_info)
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
                loutE << "Could not get screen resources" << loutEND;
                return {};
            }

            xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            if (!res_reply->num_outputs)
            {
                loutE << "No outputs found" << loutEND;
                free(res_reply);
                return {};
            }

            // Assuming the first output is the primary one
            xcb_randr_output_t output = outputs[0];
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                loutE << "Output is not currently connected to a CRTC" << loutEND;
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

            if (!crtc_info_reply)
            {
                loutE << "Could not get CRTC info" << loutEND;
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            mode_info = xcb_randr_get_screen_resources_current_modes(res_reply);
            mode_count = xcb_randr_get_screen_resources_current_modes_length(res_reply);

            for (int i = 0; i < mode_count; i++) // Iterate through all modes
            {
                string s = to_string(mode_info[i].width) + "x" + to_string(mode_info[i].height) + " " + to_string(calculate_refresh_rate(&mode_info[i])) + " Hz";

                #ifdef ARMV8_BUILD
                    loutI << s << loutEND;
                #endif

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
                loutE << "Could not get screen resources" << loutEND;
                return;
            }

            // Get outputs; assuming the first output is the one we want to change
            outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            outputs_len = xcb_randr_get_screen_resources_current_outputs_length(res_reply);

            if (!outputs_len)
            {
                loutE << "No outputs found" << loutEND;
                free(res_reply);
                return;
            }

            xcb_randr_output_t output = outputs[0]; // Assuming we change the first output

            // Get the current configuration for the output
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, NULL);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                loutE << "Output is not connected to any CRTC" << loutEND;
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
                loutE << "Failed to set mode" << loutEND;
            }
            else
            {
                loutI << "Mode set successfully" << loutEND;
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
                loutE << "Could not get screen resources" << loutEND;
                return {};
            }

            xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            if (!res_reply->num_outputs)
            {
                loutE << "No outputs found" << loutEND;
                free(res_reply);
                return {};
            }

            // Assuming the first output is the primary one
            xcb_randr_output_t output = outputs[0];
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                loutE << "Output is not currently connected to a CRTC" << loutEND;
                free(output_info_reply);
                free(res_reply);
                return {};
            }

            xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

            if (!crtc_info_reply)
            {
                loutE << "Could not get CRTC info" << loutEND;
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
                loutE << "Could not get screen resources" << loutEND;
                return {};
            }

            xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            if (!res_reply->num_outputs)
            {
                loutE << "No outputs found" << loutEND;
                free(res_reply);
                return {};
            }

            // Assuming the first output is the primary one
            xcb_randr_output_t output = outputs[0];
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, nullptr);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                loutE << "Output is not currently connected to a CRTC" << loutEND;
                free(output_info_reply);
                free(res_reply);
                return{};
            }

            xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(conn, output_info_reply->crtc, XCB_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t *crtc_info_reply = xcb_randr_get_crtc_info_reply(conn, crtc_info_cookie, nullptr);

            if (!crtc_info_reply)
            {
                loutE << "Could not get CRTC info" << loutEND;
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
    /* Variabels   */
        xcb_randr_mode_t(_current_resolution);
        string(_current_resoluton_string);
        vector<pair<xcb_randr_mode_t, string>>(_avalible_resolutions);

    /* Methods     */
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
                loutE << "Could not get screen resources" << endl;
                return;
            }

            // Get outputs; assuming the first output is the one we want to change
            outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);
            outputs_len = xcb_randr_get_screen_resources_current_outputs_length(res_reply);

            if (!outputs_len)
            {
                loutE << "No outputs found" << endl;
                free(res_reply);
                return;
            }

            xcb_randr_output_t output = outputs[0]; // Assuming we change the first output

            // Get the current configuration for the output
            xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info(conn, output, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_reply = xcb_randr_get_output_info_reply(conn, output_info_cookie, NULL);

            if (!output_info_reply || output_info_reply->crtc == XCB_NONE)
            {
                loutE << "Output is not connected to any CRTC" << endl;
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
                loutE << "Failed to set mode" << loutEND;
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

    /* Constructor */
        __screen_settings__() {}
}; static __screen_settings__ *screen_settings(nullptr);

typedef struct dropdown_menu_t {
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
} dropdown_menu_t;

class __system_settings__ {
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
        void query_input_devices__()
        {
            xcb_input_xi_query_device_cookie_t cookie = xcb_input_xi_query_device(conn, XCB_INPUT_DEVICE_ALL);
            xcb_input_xi_query_device_reply_t* reply = xcb_input_xi_query_device_reply(conn, cookie, NULL);

            if (reply == nullptr)
            {
                loutE << "xcb_input_xi_query_device_reply_t == nullptr" << loutEND;
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

                    loutI << "Found pointing device: " << device_name << loutEND;
                    loutI << "Device ID" << device->deviceid << loutEND;
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

            pointer_vec.clear();
            query_input_devices__();
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
                        XCB_EVENT_MASK_EXPOSURE,
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
        
        vector<window> _screen_resolution_options_vector;
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
                    _screen_resolution_options_vector[i].draw_text_auto_color(
                        screen_settings->_avalible_resolutions[i].second.c_str(),
                        4,
                        14
                    );
                }
            }

            for (int i = 0; i < pointer_vec.size(); ++i)
            {
                if (__window == pointer_vec[i]._window)
                {
                    pointer_vec[i]._window.draw_text_auto_color(
                        pointer_vec[i]._device_name.c_str(),
                        4,
                        14,
                        BLACK
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

}; static __system_settings__ *system_settings(nullptr);

/** NOTE: DEPRECATED */
class Dock {
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

class __dock_search__ {
    /* Defines   */
        constexpr uint8_t char_to_keycode__(int8_t c)
        {
            switch (c)
            {
                case 'a': return wm->key_codes.a;
                case 'b': return wm->key_codes.b;
                case 'c': return wm->key_codes.c;
                case 'd': return wm->key_codes.d;
                case 'e': return wm->key_codes.e;
                case 'f': return wm->key_codes.f;
                case 'g': return wm->key_codes.g;
                case 'h': return wm->key_codes.h;
                case 'i': return wm->key_codes.i;
                case 'j': return wm->key_codes.j;
                case 'k': return wm->key_codes.k;
                case 'l': return wm->key_codes.l;
                case 'm': return wm->key_codes.m;
                case 'n': return wm->key_codes.n;
                case 'o': return wm->key_codes.o;
                case 'p': return wm->key_codes.p;
                case 'q': return wm->key_codes.q;
                case 'r': return wm->key_codes.r;
                case 's': return wm->key_codes.s;
                case 't': return wm->key_codes.t;
                case 'u': return wm->key_codes.u;
                case 'v': return wm->key_codes.v;
                case 'w': return wm->key_codes.w;
                case 'x': return wm->key_codes.x;
                case 'y': return wm->key_codes.y;
                case 'z': return wm->key_codes.z;
                case '-': return wm->key_codes.minus;
                case ' ': return wm->key_codes.space_bar;
            }

            return (uint8_t)0;
        }

        constexpr int8_t lower_to_upper_case__(int8_t c)
        {
            if (c == '-') return '_';
            if (c == ' ') return ' ';
            return (c - 32);
        }

        #define APPEND_TO_STR(__char) \
            if (e->detail == char_to_keycode__(__char))             \
            {                                                       \
                if (e->state == SHIFT)                              \
                {                                                   \
                    search_string << lower_to_upper_case__(__char); \
                }                                                   \
                else                                                \
                {                                                   \
                    search_string << __char;                        \
                }                                                   \
            }
    
    private:
    /* Methods   */
        void setup_events__()
        {
            event_handler->setEventCallback(EV_CALL(XCB_KEY_PRESS)
            {
                RE_CAST_EV(xcb_key_press_event_t);
                if (e->event == main_window)
                {
                    for (int i = 0; i < _char_vec.size(); ++i)
                    {
                        APPEND_TO_STR(_char_vec[i]);
                    }

                    if (e->detail == wm->key_codes._delete)
                    {
                        pop_last_ss(search_string);
                        main_window.clear();
                        FLUSH_X();
                    }

                    if (e->detail == wm->key_codes.enter)
                    {
                        if (enter_function)
                        {
                            enter_function();
                        }
                    
                        search_string.str("");
                        main_window.clear();
                        FLUSH_X();
                    }

                    draw_text();
                }
            });

            event_handler->setEventCallback(EV_CALL(XCB_BUTTON_PRESS)
            {
                RE_CAST_EV(xcb_button_press_event_t);
                if (e->event == main_window)
                {
                    main_window.raise();
                    main_window.focus_input();
                }
            });
        }

    /* Variables */
        function<void()> enter_function;
        File file;
        vector<string> results;
        vector<window> entry_list;
        Launcher launcher;

        vector<int8_t> _char_vec = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '-', ' '};

    public:
    /* Variabels */
        window(main_window);
        stringstream search_string{};
    
    /* Methods   */
        void create(uint32_t __parent_window, int16_t __x, int16_t __y, uint16_t __width, uint16_t __height)
        {
            main_window.create_window(
                __parent_window,
                __x,
                __y,
                __width,
                __height,
                BLACK,
                XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE,
                DEFAULT_KEYS | KEYS_FOR_TYPING,
                (int[]){ALL, 2, BLUE}
            );

            main_window.grab_button({
                { L_MOUSE_BUTTON, NULL }
            });
        }

        void add_enter_action(function<void()> enter_action)
        {
            enter_function = enter_action;
        }

        void init()
        {
            setup_events__();
            add_enter_action([this]() -> void
            {
                int status = launcher.launch_child_process(search_string.str().c_str());
                if (status == 0)
                {
                    search_string.str().clear();
                    main_window.clear();
                    signal_manager->emit("HIDE_DOCK");
                }
            });
        }

        void show()
        {
            main_window.map();
            main_window.raise();
            main_window.focus_input();
        }

        void draw_text()
        {
            main_window.draw_acc(search_string.str());
        }
};

class __dock__ {
    private:
    /* Variabels   */
        window dock_menu;
        __dock_search__ dock_search;
        STATIC_CONSTEXPR_TYPE(uint16_t, _width, 400);
        STATIC_CONSTEXPR_TYPE(uint16_t, _height, 200);

        client *f_c = nullptr;

    /* Methods     */
        typedef enum {
            MAX,
            MIN
        } dock_anim_t;

        void anim_dock_menu(dock_anim_t __state)
        {
            const int duration = 200;

            if (__state == MAX)
            {
                __animate__::window(
                    dock_menu,
                    SCREEN_CENTER_X(_width),
                    SCREEN_BOTTOM_Y(_height),
                    _width,
                    _height,
                    duration
                );
                dock_menu.raise();
            }

            if (__state == MIN)
            {
                dock_menu.raise();
                dock_menu.focus_input();
                __animate__::window(
                    dock_menu,
                    SCREEN_CENTER_X(2),
                    SCREEN_BOTTOM_Y(2),
                    2,
                    2,
                    duration
                );
            }
        }

        void create_window__()
        {
            dock_menu.create_window(
                screen->root,
                SCREEN_CENTER_X(2),
                SCREEN_BOTTOM_Y(2),
                2,
                2,
                RED
            );

            dock_search.create(dock_menu, 0, 0, _width, 20);
            dock_search.init();
        }

        void show__(uint32_t __window)
        {
            if (__window == dock_menu)
            {
                if (wm->focused_client != nullptr)
                {
                    f_c = wm->focused_client;
                    wm->root.focus_input();
                    wm->focused_client = nullptr;
                }

                dock_menu.map();
                dock_menu.raise();
                dock_menu.focus_input();
                anim_dock_menu(MAX);
                dock_menu.raise();
                dock_menu.focus_input();
                dock_search.show();
            }
        }

        void hide__(uint32_t __window)
        {
            if (__window == dock_menu)
            {
                anim_dock_menu(MIN);
                dock_menu.unmap();

                if (f_c != nullptr)
                {
                    f_c->focus();
                    f_c = nullptr;
                }
            }
        }

    public:
    /* Methods     */
        void init()
        {
            create_window__();

            event_handler->setEventCallback(EV_CALL(XCB_KEY_PRESS)
            {
                RE_CAST_EV(xcb_key_press_event_t);
                if (e->detail == wm->key_codes.super_l)
                {
                    if (e->state == SHIFT)
                    {
                        if (dock_menu.is_mapped())
                        {
                            hide__(dock_menu);
                        }
                        else
                        {
                            show__(dock_menu);
                        }
                    }
                }
            });

            signal_manager->connect("HIDE_DOCK",
            [this]() -> void
            {
                hide__(dock_menu);
            });
        }
    
    /* Constructor */
        __dock__() {}

}; static __dock__ *dock( nullptr );

class DropDownTerm {
    public:
        window w;
        vector<window> w_vec;

        void init()
        {
            w.create_window(
                screen->root,
                0,
                - ( screen->height_in_pixels / 2 ),
                screen->width_in_pixels,
                ( screen->height_in_pixels / 2 ),
                BLACK,
                NONE,
                MAP
            );
            FlushX_Win( w );
            for ( int i = 0; i < (( screen->height_in_pixels / 2 ) / 20 ); ++i )
            {
                window window;
                window.create_window(
                    w,
                    0,
                    ((( screen->height_in_pixels / 2 ) - 20 ) - ( i * 20 )),
                    screen->width_in_pixels,
                    20,
                    WHITE,
                    NONE,
                    MAP,
                    ( int[] ){ DOWN | LEFT | RIGHT, 1, BLACK }
                );
                w_vec.push_back( window );
            }

            event_handler->setEventCallback( XCB_KEY_PRESS, [ & ]( Ev ev ) -> void
            {
                RE_CAST_EV( xcb_key_press_event_t );
                if ( e->detail == wm->key_codes.f12 )
                {
                    if ( w.y() == ( - ( screen->height_in_pixels / 2 )))
                    {
                        w.raise();
                        w.y( 0 );
                        FlushX_Win( w );
                    }
                    else
                    {
                        w.y( - ( screen->height_in_pixels / 2 ));
                        FlushX_Win( w );
                    }
                }
            });
        }
};
static DropDownTerm *ddTerm( nullptr );

class mv_client {
    /* Defines     */
        #define RIGHT_  screen->width_in_pixels  - c->width
        #define BOTTOM_ screen->height_in_pixels - c->height
   
    public:
    /* Constructor */
        mv_client(client * c, int start_x, int start_y)
        : c(c), start_x(start_x), start_y(start_y)
        {
            if (c->win.is_EWMH_fullscreen()) return;

            pointer.grab();
            run();
            pointer.ungrab();
        }

    private:
    /* Variabels   */
        client(*c);
        pointer(pointer);
        int start_x, start_y;
        bool shouldContinue = true;
        xcb_generic_event_t(*ev);
        chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
        
        #ifdef ARMV8_BUILD
            STATIC_CONSTEXPR_TYPE(double, frameRate, 30.0);
        #else
            STATIC_CONSTEXPR_TYPE(double, frameRate, 120.0);
        #endif
        STATIC_CONSTEXPR_TYPE(double, frameDuration, (1000 / frameRate));
    
    /* Methods     */
        void snap(int x, int y)
        {
            // WINDOW TO WINDOW SNAPPING 
            for (client *const &cli : wm->cur_d->current_clients)
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

        void run() {
            while (shouldContinue) {
                ev = xcb_wait_for_event(conn);
                if (ev == nullptr) continue;

                switch (ev->response_type & ~0x80) {
                    case XCB_MOTION_NOTIFY: {
                        if (isTimeToRender()) {
                            RE_CAST_EV(xcb_motion_notify_event_t);
                            int new_x = e->root_x - start_x - BORDER_SIZE;
                            int new_y = e->root_y - start_y - BORDER_SIZE;
                            snap(new_x, new_y);
                            FLUSH_X();

                        } break;
                        
                    }
                    case XCB_BUTTON_RELEASE: {
                        shouldContinue = false;
                        c->update();
                        break;

                    }

                } free(ev);

            }
        }

        bool isTimeToRender() {
            auto currentTime = chrono::high_resolution_clock::now();
            const chrono::duration<double, milli> elapsedTime = currentTime - lastUpdateTime;
            if (elapsedTime.count() >= frameDuration) {
                lastUpdateTime = currentTime;
                return true;

            } return false;

        }
};

mutex mtx;
class change_desktop {
    public:
    /* Constructor */
        change_desktop(xcb_connection_t *connection) {}

    /* Variabels   */
        int duration = 100;

        enum DIRECTION {
            NEXT,
            PREV
        };

    /* Methods     */
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
    /* Variabels   */
        // xcb_connection_t(*connection);
        vector<client *>(show);
        vector<client *>(hide);
        thread(show_thread);
        thread(hide_thread);
        atomic<bool>(stop_show_flag){false};
        atomic<bool>(stop_hide_flag){false};
        atomic<bool>(reverse_animation_flag){false};
        vector<thread>(animation_threads);

    /* Methods     */
        vector<client *> get_clients_on_desktop(const uint8_t &desktop)
        {
            vector<client *>(clients);
            for (client *const &c : wm->client_list)
            {
                if (c == nullptr) continue;

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

class resize_client {
    public:
    /* Constructor */
        /**
         *
         * THE REASON FOR THE 'retard_int' IS BECUSE WITHOUT IT 
         * I CANNOT CALL THIS CLASS LIKE THIS 'resize_client(c)' 
         * INSTEAD I WOULD HAVE TO CALL IT LIKE THIS 'resize_client rc(c)'
         * AND NOW WITH THE 'retard_int' I CAN CALL IT LIKE THIS 'resize_client(c, 0)'
         * 
         */
        resize_client(client * & c , int retard_int) : c(c) {
            if (c->win.is_EWMH_fullscreen()) return;
            pointer.grab();
            pointer.teleport(c->x + c->width, c->y + c->height);
            run();
            pointer.ungrab();

        }

    /* Subclasses  */
        class no_border {
            public:
            /* Constructor */
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
            /* Variabels   */
                client *&c;
                uint32_t x;
                pointer pointer;
                uint32_t y;
                const double frameRate = 120.0;
                chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
                const double frameDuration = 1000.0 / frameRate;
            
            /* Methods     */
                constexpr void teleport_mouse(edge edge) {
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

                void resize_client(const uint32_t x, const uint32_t y, edge edge) {
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

                void run(edge edge) {
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

                bool isTimeToRender() {
                    const auto &currentTime = chrono::high_resolution_clock::now();
                    const chrono::duration<double, milli> &elapsedTime = currentTime - lastUpdateTime;

                    if (elapsedTime.count() >= frameDuration) {
                        lastUpdateTime = currentTime;
                        return true;
                    }

                    return false;
                }
        };

        class border {
            public:
            /* Constructor */
                border(client *&c, edge _edge) : c(c) {
                    if (c->win.is_EWMH_fullscreen()) return;
                    map<client *, edge> map = wm->get_client_next_to_client(c, _edge);
                    for (const auto &pair : map) {
                        if (pair.first != nullptr) {
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
            /* Variabels   */
                client(*&c);
                client(*c2);
                edge(c2_edge);
                pointer(pointer); 
                chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
                #ifdef ARMV8_BUILD
                    STATIC_CONSTEXPR_TYPE(double, frameRate, 15.0);
                #else
                    STATIC_CONSTEXPR_TYPE(double, frameRate, 120.0);
                #endif
                STATIC_CONSTEXPR_TYPE(double, frameDuration, (1000 / frameRate));

            /* Methods     */
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

                void resize_client(const uint32_t x, const uint32_t y, edge edge) {
                    switch (edge) {
                        case edge::LEFT: {
                            c->x_width(x, (c->width + c->x - x));
                            break;

                        }
                        case edge::RIGHT: {
                            c->_width((x - c->x));
                            break;

                        }
                        case edge::TOP: {
                            c->y_height(y, (c->height + c->y - y));   
                            break;

                        }
                        case edge::BOTTOM_edge: {
                            c->_height((y - c->y));
                            break;

                        }
                        case edge::NONE: {
                            return;

                        }
                        case edge::TOP_LEFT: {
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;
                        
                        }
                        case edge::TOP_RIGHT: {
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;
                        
                        }
                        case edge::BOTTOM_LEFT: {
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;

                        }
                        case edge::BOTTOM_RIGHT: {
                            c->width_height((x - c->x), (y - c->y));   
                            break;

                        }

                    }

                }
                
                void resize_client(client *c, const uint32_t x, const uint32_t y, edge edge) {
                    switch (edge) {
                        case edge::LEFT: {
                            c->x_width(x, (c->width + c->x - x));
                            break;

                        }
                        case edge::RIGHT: {
                            c->_width((x - c->x));
                            break;

                        }
                        case edge::TOP: {
                            c->y_height(y, (c->height + c->y - y));   
                            break;

                        }
                        case edge::BOTTOM_edge: {
                            c->_height((y - c->y));
                            break;

                        }
                        case edge::NONE: {
                            return;
                        
                        }
                        case edge::TOP_LEFT: {
                            c->x_y_width_height(x, y, (c->width + c->x - x), (c->height + c->y - y));
                            break;

                        }
                        case edge::TOP_RIGHT: {
                            c->y_width_height(y, (x - c->x), (c->height + c->y - y));
                            break;

                        }
                        case edge::BOTTOM_LEFT: {
                            c->x_width_height(x, (c->width + c->x - x), (y - c->y));   
                            break;

                        }
                        case edge::BOTTOM_RIGHT: {
                            c->width_height((x - c->x), (y - c->y));   
                            break;
                        
                        }

                    }

                }

                void snap(const uint32_t x, const uint32_t y, edge edge, const uint8_t & prox) {
                    uint16_t left_border(0), right_border(0), top_border(0), bottom_border(0);

                    for (client *const &c : wm->cur_d->current_clients) {
                        if (c == this->c) {
                            continue;

                        }
                        left_border = c->x;
                        right_border = (c->x + c->width);
                        top_border = c->y;
                        bottom_border = (c->y + c->height);

                        if (edge != edge::RIGHT
                        &&  edge != edge::BOTTOM_RIGHT
                        &&  edge != edge::TOP_RIGHT) {
                            if (x > right_border - prox && x < right_border + prox
                            &&  y > top_border && y < bottom_border) {
                                resize_client(right_border, y, edge);
                                return;

                            }

                        }
                        if (edge != edge::LEFT
                        &&  edge != edge::TOP_LEFT
                        &&  edge != edge::BOTTOM_LEFT) {
                            if (x > left_border - prox && x < left_border + prox
                            &&  y > top_border && y < bottom_border) {
                                resize_client(left_border, y, edge);
                                return;

                            }

                        }
                        if (edge != edge::BOTTOM_edge 
                        &&  edge != edge::BOTTOM_LEFT 
                        &&  edge != edge::BOTTOM_RIGHT) {
                            if (y > bottom_border - prox && y < bottom_border + prox
                            &&  x > left_border && x < right_border) {
                                resize_client(x, bottom_border, edge);
                                return;

                            }

                        }
                        if (edge != edge::TOP
                        &&  edge != edge::TOP_LEFT
                        &&  edge != edge::TOP_RIGHT) {
                            if (y > top_border - prox && y < top_border + prox
                            &&  x > left_border && x < right_border) {
                                resize_client(x, top_border, edge);
                                return;

                            }

                        }

                    } resize_client(x, y, edge);

                }

                void run(edge edge) {
                    xcb_generic_event_t *ev;
                    bool shouldContinue = true;

                    while (shouldContinue) {
                        if ((ev = xcb_wait_for_event(conn)) == nullptr) continue;

                        switch (ev->response_type & ~0x80) {
                            case XCB_MOTION_NOTIFY: {
                                if (isTimeToRender()) {
                                    RE_CAST_EV(xcb_motion_notify_event_t);
                                    snap(e->root_x, e->root_y, edge, 12);
                                    c->update();
                                    xcb_flush(conn);

                                } break;

                            }
                            case XCB_BUTTON_RELEASE: {
                                shouldContinue = false;                        
                                c->update();
                                break;

                            }
                            case XCB_PROPERTY_NOTIFY: {
                                RE_CAST_EV(xcb_property_notify_event_t);
                                client *c = wm->client_from_any_window(&e->window);
                                if (c) {
                                    if (e->atom   == ewmh->_NET_WM_NAME
                                    &&  e->window == c->win) {
                                        c->draw_title(TITLE_REQ_DRAW);
                                    
                                    }
                                
                                } break;
                            
                            }
                        
                        } free(ev);
                    
                    }
                
                }

                void run_double(edge edge, bool __double = false) {
                    xcb_generic_event_t *ev;
                    bool shouldContinue(true);
                    
                    while (shouldContinue) {
                        ev = xcb_wait_for_event(conn);
                        if (ev == nullptr) continue;

                        switch (ev->response_type & ~0x80) {
                            case XCB_MOTION_NOTIFY: {
                                const auto *e = reinterpret_cast<const xcb_motion_notify_event_t *>(ev);
                                if (isTimeToRender()) {
                                    resize_client(c, e->root_x, e->root_y, edge);
                                    resize_client(c2, e->root_x, e->root_y, c2_edge);
                                    xcb_flush(conn);

                                } break;
                            
                            }
                            case XCB_BUTTON_RELEASE: {
                                shouldContinue = false;                        
                                c->update();
                                c2->update();
                                break;

                            }

                        } free(ev);
                    
                    }
                
                }

                bool isTimeToRender() {
                    const auto &currentTime = chrono::high_resolution_clock::now();
                    const chrono::duration<double, milli> & elapsedTime = currentTime - lastUpdateTime;

                    if (elapsedTime.count() >= frameDuration) {
                        lastUpdateTime = currentTime; 
                        return true;
                    
                    } return false;

                }

        };

    private:
    /* Variabels   */
        client * & c;
        uint32_t x;
        pointer pointer;
        uint32_t y;
        chrono::high_resolution_clock::time_point lastUpdateTime = chrono::high_resolution_clock::now();
        
        const double frameRate = 120.0;
        const double frameDuration = 1000.0 / frameRate;
    
    /* Methods     */
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
                        WS_emit(e->window, EXPOSE);

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

class max_win {
    private:
    /* Variabels   */
        client(*c);

    /* Methods     */
        void max_win_animate(const int &endX, const int &endY, const int &endWidth, const int &endHeight) {
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
            if (c->max_button_ogsize.x > screen->width_in_pixels ) c->max_button_ogsize.x = (screen->width_in_pixels  / 4);
            if (c->max_button_ogsize.y > screen->height_in_pixels) c->max_button_ogsize.y = (screen->height_in_pixels / 4);

            if (c->max_button_ogsize.width  == 0 || c->max_button_ogsize.width  > screen->width_in_pixels ) c->max_button_ogsize.width  = (screen->width_in_pixels  / 2);
            if (c->max_button_ogsize.height == 0 || c->max_button_ogsize.height > screen->height_in_pixels) c->max_button_ogsize.height = (screen->height_in_pixels / 2);

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
                -BORDER_SIZE,
                -BORDER_SIZE,
                (screen->width_in_pixels  + (BORDER_SIZE * 2)),
                (screen->height_in_pixels + (BORDER_SIZE * 2))
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
            if (c->max_ewmh_ogsize.width  > (screen->width_in_pixels  + (BORDER_SIZE * 2))) c->max_ewmh_ogsize.width  = screen->width_in_pixels  / 2;
            if (c->max_ewmh_ogsize.height > (screen->height_in_pixels + (BORDER_SIZE * 2))) c->max_ewmh_ogsize.height = screen->height_in_pixels / 2;
            
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
    /* Variabels   */
        typedef enum {
            BUTTON_MAXWIN,
            EWMH_MAXWIN
        } max_win_type;

    /* Constructor */
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
class tile {
    private:
    /* Variabels   */
        client(*c);

    /* Methods     */
        /**
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
        bool current_tile_pos(TILEPOS mode) {
            switch (mode) {
                case   TILEPOS::LEFT       :{
                    if (c->x      == 0 
                    &&  c->y      == 0 
                    &&  c->width  == screen->width_in_pixels / 2 
                    &&  c->height == screen->height_in_pixels) {
                        return true;

                    } break;

                } case TILEPOS::RIGHT      :{
                    if (c->x      == screen->width_in_pixels / 2 
                    &&  c->y      == 0 
                    &&  c->width  == screen->width_in_pixels / 2
                    &&  c->height == screen->height_in_pixels) {
                        return true;

                    } break;

                } case TILEPOS::LEFT_DOWN  :{
                    if (c->x      == 0
                    &&  c->y      == screen->height_in_pixels / 2
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2) {
                        return true;

                    } break;

                } case TILEPOS::RIGHT_DOWN :{
                    if (c->x      == screen->width_in_pixels  / 2
                    &&  c->y      == screen->height_in_pixels / 2
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2) {
                        return true;

                    } break;

                } case TILEPOS::LEFT_UP    :{
                    if (c->x      == 0
                    &&  c->y      == 0
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2) {
                        return true;

                    } break;

                } case TILEPOS::RIGHT_UP   :{
                    if (c->x      == screen->width_in_pixels  / 2
                    &&  c->y      == 0
                    &&  c->width  == screen->width_in_pixels  / 2
                    &&  c->height == screen->height_in_pixels / 2) {
                        return true;

                    } break;

                }

            } return false; 

        }
        /**
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
        void set_tile_sizepos(TILEPOS sizepos) {
            switch (sizepos) {
                case   TILEPOS::LEFT       : {
                    animate(
                        0,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels

                    ); return;

                } case TILEPOS::RIGHT      : {
                    animate(
                        screen->width_in_pixels / 2,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels

                    ); return;

                } case TILEPOS::LEFT_DOWN  : {
                    animate(
                        0,
                        screen->height_in_pixels / 2,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2

                    ); return;

                } case TILEPOS::RIGHT_DOWN : {
                    animate(
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2

                    ); return;

                } case TILEPOS::LEFT_UP    : {
                    animate(
                        0,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2

                    ); return;

                } case TILEPOS::RIGHT_UP   : {
                    animate(
                        screen->width_in_pixels / 2,
                        0,
                        screen->width_in_pixels / 2,
                        screen->height_in_pixels / 2

                    ); return;
                    
                }

            }

        }
        void restore_og_tile_pos() {
            animate(
                c->tile_ogsize.x,
                c->tile_ogsize.y,
                c->tile_ogsize.width,
                c->tile_ogsize.height

            );

        }
        void animate(const int &end_x, const int &end_y, const int &end_width, const int &end_height) {
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

            ); c->update();

        }

    public:
    /* Constructor */
        tile(client *&c, TILE tile) : c(c) {
            if (c->is_EWMH_fullscreen()) return;
            switch (tile) {
                case   TILE::LEFT  : {
                    // IF 'CURRENTLT_TILED' TO 'LEFT'
                    if (current_tile_pos(TILEPOS::LEFT)) {
                        restore_og_tile_pos();
                        return;

                    }

                    // IF 'CURRENTLY_TILED' TO 'RIGHT', 'LEFT_DOWN' OR 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT)
                    ||  current_tile_pos(TILEPOS::LEFT_DOWN)
                    ||  current_tile_pos(TILEPOS::LEFT_UP)) {
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

                    c->save_tile_ogsize();
                    set_tile_sizepos(TILEPOS::LEFT);
                    break;

                } case TILE::RIGHT : {
                    // IF 'CURRENTLY_TILED' TO 'RIGHT'
                    if (current_tile_pos(TILEPOS::RIGHT)) {
                        restore_og_tile_pos();
                        return;

                    }
                    
                    // IF 'CURRENTLT_TILED' TO 'LEFT', 'RIGHT_DOWN' OR 'RIGHT_UP' 
                    if (current_tile_pos(TILEPOS::LEFT)
                    ||  current_tile_pos(TILEPOS::RIGHT_UP)
                    ||  current_tile_pos(TILEPOS::RIGHT_DOWN)) {
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

                    c->save_tile_ogsize();
                    set_tile_sizepos(TILEPOS::RIGHT);
                    break;

                } case TILE::DOWN  : {
                    // IF 'CURRENTLY_TILED' 'LEFT' OR 'LEFT_UP'
                    if (current_tile_pos(TILEPOS::LEFT)
                    ||  current_tile_pos(TILEPOS::LEFT_UP)) {
                        set_tile_sizepos(TILEPOS::LEFT_DOWN);
                        return;

                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR 'RIGHT_UP'
                    if (current_tile_pos(TILEPOS::RIGHT) 
                    ||  current_tile_pos(TILEPOS::RIGHT_UP)) {
                        set_tile_sizepos(TILEPOS::RIGHT_DOWN);
                        return;

                    }
                    
                    // IF 'CURRENTLY_TILED' 'LEFT_DOWN' OR 'RIGHT_DOWN'
                    if (current_tile_pos(TILEPOS::LEFT_DOWN)
                    ||  current_tile_pos(TILEPOS::RIGHT_DOWN)) {
                        restore_og_tile_pos();
                        return;

                    }

                    break;

                } case TILE::UP    : {
                    // IF 'CURRENTLY_TILED' 'LEFT'
                    if (current_tile_pos(TILEPOS::LEFT)
                    ||  current_tile_pos(TILEPOS::LEFT_DOWN)) {
                        set_tile_sizepos(TILEPOS::LEFT_UP);
                        return;

                    }

                    // IF 'CURRENTLY_TILED' 'RIGHT' OR RIGHT_DOWN
                    if (current_tile_pos(TILEPOS::RIGHT)
                    ||  current_tile_pos(TILEPOS::RIGHT_DOWN)) {
                        set_tile_sizepos(TILEPOS::RIGHT_UP);
                        return;

                    }

                    break;

                }

            }

        }

};
class Events {
    public:
    /* Variabels */
        pointer p;

    /* Constructor */
        Events() {}
    
    /* Methods     */
        void setup()
        {
            event_handler->setEventCallback(EV_CALL(XCB_KEY_PRESS)    { key_press_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_MAP_NOTIFY)        { map_notify_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_MAP_REQUEST)       { map_req_handler(ev); });
            event_handler->setEventCallback(EV_CALL(XCB_BUTTON_PRESS) { button_press_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_CONFIGURE_REQUEST) { configure_request_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_FOCUS_IN)          { focus_in_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_FOCUS_OUT)         { focus_out_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_DESTROY_NOTIFY)    { destroy_notify_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_UNMAP_NOTIFY)      { unmap_notify_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_REPARENT_NOTIFY)   { reparent_notify_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_ENTER_NOTIFY)      { enter_notify_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_LEAVE_NOTIFY)      { leave_notify_handler(ev); });
            // event_handler->setEventCallback(EV_CALL(XCB_MOTION_NOTIFY)     { motion_notify_handler(ev); });

            init_signals();

        }

        void init_signals() {
            CONN_root(MOVE_TO_DESKTOP_1, W_callback -> void { change_desktop::teleport_to(1); });
            CONN_root(MOVE_TO_DESKTOP_2, W_callback -> void { change_desktop::teleport_to(2); });
            CONN_root(MOVE_TO_DESKTOP_3, W_callback -> void { change_desktop::teleport_to(3); });
            CONN_root(MOVE_TO_DESKTOP_4, W_callback -> void { change_desktop::teleport_to(4); });
            CONN_root(MOVE_TO_DESKTOP_5, W_callback -> void { change_desktop::teleport_to(5); });

            CONN_root(MOVE_TO_NEXT_DESKTOP, W_callback -> void { change_desktop(conn).change_to(change_desktop::NEXT); });
            CONN_root(MOVE_TO_PREV_DESKTOP, W_callback -> void { change_desktop(conn).change_to(change_desktop::PREV); });
            
            CONN_root(FOCUS_CLIENT_FROM_POINTER, W_callback -> void {
                client *c = wm->get_client_from_pointer();
                if (c == nullptr) return;
                loutI << "got client from pointer" << loutEND;
                c->focus();

            });

            C_SIGNAL(if (__c) tile(__c, TILE::LEFT );, TILE_LEFT);
            C_SIGNAL(if (__c) tile(__c, TILE::RIGHT);, TILE_RIGHT);
            C_SIGNAL(if (__c) tile(__c, TILE::UP   );, TILE_UP);
            C_SIGNAL(if (__c) tile(__c, TILE::DOWN );, TILE_DOWN);

            CONN_root(MOVE_TO_NEXT_DESKTOP_WAPP, W_callback -> void {
                change_desktop cd(conn);
                cd.change_with_app(change_desktop::NEXT);

            });

            CONN_root(MOVE_TO_PREV_DESKTOP_WAPP, W_callback -> void {
                change_desktop cd(conn);
                cd.change_with_app(change_desktop::PREV);
            
            });

            CONN_root(DEBUG_KEY_PRESS, W_callback -> void {
                pid_manager->list_pids();
                event_handler->iter_and_log_map_size();
                
            });

            C_SIGNAL(if (__c) {
                if (!__c->win.is_mapped()) {
                    __c->kill();

                } else {
                    __c->win.kill();
                    __c->kill();

                }

            }, KILL_SIGNAL);

            C_SIGNAL(if (__c) {
                __c->raise();
                __c->focus();
                wm->focused_client = __c;

            }, FOCUS_CLIENT);
            C_SIGNAL(if (__c) mv_client(__c, wm->pointer.x() - __c->x - BORDER_SIZE, wm->pointer.y() - __c->y - BORDER_SIZE);, MOVE_CLIENT_MOUSE);
            C_SIGNAL(if (__c) max_win(__c, max_win::BUTTON_MAXWIN);, BUTTON_MAXWIN_PRESS);
            C_SIGNAL(if (__c) max_win(__c, max_win::EWMH_MAXWIN  );, EWMH_MAXWIN_SIGNAL );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::LEFT        );, RESIZE_CLIENT_BORDER_LEFT        );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::RIGHT       );, RESIZE_CLIENT_BORDER_RIGHT       );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::TOP         );, RESIZE_CLIENT_BORDER_TOP         );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::BOTTOM_edge );, RESIZE_CLIENT_BORDER_BOTTOM      );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::TOP_LEFT    );, RESIZE_CLIENT_BORDER_TOP_LEFT    );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::TOP_RIGHT   );, RESIZE_CLIENT_BORDER_TOP_RIGHT   );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::BOTTOM_LEFT );, RESIZE_CLIENT_BORDER_BOTTOM_LEFT );
            C_SIGNAL(if (__c) resize_client::border(__c, edge::BOTTOM_RIGHT);, RESIZE_CLIENT_BORDER_BOTTOM_RIGHT);

            CONN_root(CONF_REQ_WIDTH,  W_callback -> void { wm->data.width  = __window; });
            CONN_root(CONF_REQ_HEIGHT, W_callback -> void { wm->data.height = __window; });
            CONN_root(CONF_REQ_X,      W_callback -> void { wm->data.x      = __window; });
            CONN_root(CONF_REQ_Y,      W_callback -> void { wm->data.y      = __window; });

            // signal_manager->_window_signals.conect(screen->root, EWMH_MAXWIN_SIGNAL, [this](uint32_t __w) -> void {
            //     client *c = signal_manager->_window_client_map.retrive(__w);
            //     if (c != nullptr) C_EMIT(c, EWMH_MAXWIN_SIGNAL);

            // });

        }

    private:
    /* Methods     */
        void key_press_handler(const xcb_generic_event_t *&ev) {
            RE_CAST_EV(xcb_key_press_event_t);
            if (e->detail == wm->key_codes.r_arrow) {
                switch (e->state) {
                    case (SHIFT + CTRL + SUPER): {
                        change_desktop cd(conn);
                        cd.change_with_app(change_desktop::NEXT);
                        return;
                    }
                    
                    case (CTRL + SUPER): {
                        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::NEXT);
                        return;
                    }

                    case SUPER: {
                        client *c = signal_manager->_window_client_map.retrive(e->event);
                        tile(c, TILE::RIGHT);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.l_arrow) {
                switch (e->state) {
                    case (SHIFT + CTRL + SUPER): {
                        change_desktop cd(conn);
                        cd.change_with_app(change_desktop::PREV);
                        return;
                    }

                    case (CTRL + SUPER): {
                        change_desktop change_desktop(conn);
                        change_desktop.change_to(change_desktop::PREV);
                        return;
                    }
                    
                    case SUPER: {
                        client *c = signal_manager->_window_client_map.retrive(e->event);
                        tile(c, TILE::LEFT);
                        return;
                    }
                }
            }
            
            if (e->detail == wm->key_codes.d_arrow) {
                switch (e->state) {
                    case SUPER: {
                        client *c = signal_manager->_window_client_map.retrive(e->event);
                        tile(c, TILE::DOWN);
                        return;
                    }
                }
            }

            if (e->detail == wm->key_codes.u_arrow) {
                switch (e->state) {
                    case SUPER: {
                        client *c = signal_manager->_window_client_map.retrive(e->event);
                        tile(c, TILE::UP);
                        return;
                    }
                }
            }

            /* if (e->detail == wm->key_codes.tab)
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
                        pid_manager->list_pids();
                        event_handler->iter_and_log_map_size();
                        // wm->root.set_backround_png(USER_PATH_PREFIX("/mwm_png/galaxy16-17-3840x1200.png"));
                        // GET_CLIENT_FROM_WINDOW(e->event);
                        // c->kill();
                        // c->win.x(BORDER_SIZE);
                        // c->win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
                        // xcb_flush(conn);

                        return;
                    }
                }
            } */
        }

        // void map_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_map_notify_event_t);
        //     client *c = signal_manager->_window_client_map.retrive(e->window);
        //     if (!c) return;
        //     c->update(); 
        // }

        // void map_req_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_map_request_event_t);
        //     client *c = signal_manager->_window_client_map.retrive(e->window);
        //     if (c != nullptr) return;
        //     wm->manage_new_client(e->window);
        // }

        void button_press_handler(const xcb_generic_event_t *&ev)
        {
            RE_CAST_EV(xcb_button_press_event_t);
            client *c = signal_manager->_window_client_map.retrive(e->event);
            if (c == nullptr) return;
            // {
            //     c = wm->get_client_from_pointer();
            //     if (c == nullptr) return;
            //     loutI << "got client from pointer" << loutEND;
            //     c->focus();
            //     return;
            // }

            if (e->detail == L_MOUSE_BUTTON)
            {
                // if (e->event == c->win) {
                //     switch (e->state)
                //     {
                //         case ALT:
                //         {
                //             c->raise();
                //             mv_client mv(c, e->event_x, e->event_y + 20);
                //             c->focus();
                //             wm->focused_client = c;
                //             return;
                //         }
                //     }

                //     c->raise();
                //     c->focus();
                //     wm->focused_client = c;
                //     return;
                // }
                
                if (e->event == c->titlebar)
                {
                    c->raise();
                    c->focus();
                    mv_client mv(c, e->event_x, e->event_y);
                    wm->focused_client = c;
                    return;
                }
                
                if (e->event == c->border[left])
                {
                    resize_client::border border(c, edge::LEFT);
                    return;
                }
                
                if (e->event == c->border[right])
                {
                    resize_client::border border(c, edge::RIGHT);
                    return;
                } 
                
                if (e->event == c->border[top])
                {
                    resize_client::border border(c, edge::TOP);
                    return;
                }
                
                if (e->event == c->border[bottom])
                {
                    resize_client::border border(c, edge::BOTTOM_edge);
                    return;
                }
                
                if (e->event == c->border[top_left])
                {
                    resize_client::border border(c, edge::TOP_LEFT);
                    return;
                }

                if (e->event == c->border[top_right])
                {
                    resize_client::border border(c, edge::TOP_RIGHT);
                    return;
                }
                
                if (e->event == c->border[bottom_left])
                {
                    resize_client::border border(c, edge::BOTTOM_LEFT);
                    return;
                }

                if (e->event == c->border[bottom_right])
                {
                    resize_client::border border(c, edge::BOTTOM_RIGHT);
                    return;
                }
            }

            // if (e->detail == R_MOUSE_BUTTON)
            // {
            //     switch (e->state)
            //     {
            //         case ALT:
            //         {
            //             c->raise();
            //             c->focus();
            //             resize_client resize(c, 0);
            //             wm->focused_client = c;
            //             return;
            //         }
            //     }
            // }
        }
        
        // void configure_request_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_configure_request_event_t);
        //     wm->data.width  = e->width;
        //     wm->data.height = e->height;
        //     wm->data.x      = e->x;
        //     wm->data.y      = e->y;

        //     // loutI << WINDOW_ID_BY_INPUT(e->window) << " e->x" << e->x << " e->y" << e->y << " e->width" << e->width << "e->height" << e->height << '\n';
        // }

        // void focus_in_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_focus_in_event_t);
        //     GET_CLIENT_FROM_WINDOW(e->event);

        //     c->win.ungrab_button({
        //         { L_MOUSE_BUTTON, NULL }
        //     });

        //     c->raise();
        //     c->win.set_active_EWMH_window();
        //     wm->focused_client = c;
        // }

        // void focus_out_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_focus_out_event_t);
        //     GET_CLIENT_FROM_WINDOW(e->event);
        //     c->win.grab_button({
        //         { L_MOUSE_BUTTON, NULL }
        //     });
        // }

        // void destroy_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_destroy_notify_event_t);
        //     GET_CLIENT_FROM_WINDOW(e->event);
        //     if (c->atoms.is_modal) {
        //         client *c_trans = wm->client_from_any_window(&c->modal_data.transient_for);
        //         if (c_trans != nullptr) {
        //             c_trans->focus();
        //             wm->focused_client = c_trans;

        //         }

        //     }
            
        //     pid_manager->remove_pid(c->win.pid());
        //     wm->send_sigterm_to_client(c);
        // }
        
        // void unmap_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_unmap_notify_event_t);
        //     client *c = wm->client_from_window(&e->window);
        //     if (c == nullptr) return;

        //     // loutI << WINDOW_ID_BY_INPUT(e->window) << '\n';
        // }
        
        // void reparent_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_reparent_notify_event_t);
        //     GET_CLIENT_FROM_WINDOW(e->window);

        //     c->win.x(BORDER_SIZE);
        //     c->win.y(TITLE_BAR_HEIGHT + BORDER_SIZE);
        //     xcb_flush(conn);
        // }

        // void enter_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_enter_notify_event_t);
        // }

        // void leave_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_leave_notify_event_t);   
        // }

        // void motion_notify_handler(const xcb_generic_event_t *&ev)
        // {
        //     RE_CAST_EV(xcb_motion_notify_event_t);
        // }
};
class test {
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

            while ( i < ( end + 1 ))
            {
                cd.change_to(change_desktop::NEXT);
                cd.change_to(change_desktop::PREV);
                cd.duration = (cd.duration - 1);
                ++i;
                if ( !running ) break;
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
    user = get_user_name();
    loutCUser(USER);

    NEW_CLASS(signal_manager, __signal_manager__) { signal_manager->init(); }
    NEW_CLASS(file_system,    __file_system__   ) { file_system->init_check(); }

    /* crypro = new __crypto__; */

    crypro = Malloc<__crypto__>().allocate();

    INIT_NEW_WM(wm, Window_Manager) { wm->init(); }
    change_desktop::teleport_to(1);

    NEW_CLASS(m_pointer, __pointer__) {}
    
    Events events;
    events.setup();

    NEW_CLASS(file_app,        __file_app__       ) { file_app->init(); }
    NEW_CLASS(status_bar,      __status_bar__     ) { status_bar->init(); }

    NEW_CLASS(wifi,            __wifi__           ) { wifi->init(); }
    NEW_CLASS(network,         __network__        ) { }
    NEW_CLASS(screen_settings, __screen_settings__) { screen_settings->init(); }
    NEW_CLASS(system_settings, __system_settings__) { system_settings->init(); }
    NEW_CLASS(dock,            __dock__           ) { dock->init(); }
    NEW_CLASS(pid_manager,     __pid_manager__    ) { }
    NEW_CLASS(ddTerm,          DropDownTerm       ) { ddTerm->init(); }

}
int main()
{
    loutI << "\n\n          -- mwm starting --\n" << '\n';

    function<void()> audio_thread = [ & ]()-> void
    {
        audio.init();
        audio.run();

    }; thread( audio_thread ).detach();

    setup_wm();
    audio.list_sinks();

    test tester;
    tester.init();

    event_handler->run();
    xcb_disconnect(conn);
    return 0;

}