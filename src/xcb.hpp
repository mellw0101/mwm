#ifndef XCB__HPP
#define XCB__HPP

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "tools.hpp"
#include "Log.hpp"
using namespace std;
#define X_CONN_ERROR 1
#define X_REQ_ERROR 2
#define X_ID_GEN_ERROR 3
#define X_CREATE_ERR 4
#define X_W_CREATION_ERR 5

#include <xcb/xcb.h>

class AtomMonitor {
    private:
        xcb_connection_t* _conn;
        xcb_atom_t* _atomPtr; // Pointer to atom value

    public:
        AtomMonitor(xcb_connection_t* conn) : _conn(conn), _atomPtr(new xcb_atom_t) {
            // Initialize the atom value to XCB_ATOM_NONE
            *_atomPtr = XCB_ATOM_NONE;
        }

        ~AtomMonitor() {
            // Clean up the dynamically allocated atom
            delete _atomPtr;
        }

        // Fetch and store an atom by name
        void fetchAndStoreAtom(const char* name) {
            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(_conn, 0, slen(name), name);
            xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(_conn, cookie, nullptr);

            if (reply) {
                *_atomPtr = reply->atom;
                free(reply);

            }

        }

        // Access the atom value
        xcb_atom_t getAtom() const {
            return _atomPtr ? *_atomPtr : XCB_ATOM_NONE;
        }
};
class XcbHelper {
    private:
        xcb_connection_t* _conn;
        xcb_atom_t _netWmNameAtom;

    public:
        XcbHelper(xcb_connection_t* conn) : _conn(conn), _netWmNameAtom(XCB_ATOM_NONE) {
            initAtoms();
        }

        void initAtoms() {
            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(_conn, 0, slen("_NET_WM_NAME"), "_NET_WM_NAME");
            xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(_conn, cookie, nullptr);
            if (reply) {
                _netWmNameAtom = reply->atom;
                free(reply);
            }
        }

        // Accessor for the atom
        xcb_atom_t getNetWmNameAtom() const {
            return _netWmNameAtom;
        }

        // Other methods...
};

typedef struct ___256__bit__data__t__ {
    uint64_t *data;
    
} _256__bit__data__t;

// Function to initialize the data
inline void init_256_bit_data(_256__bit__data__t *obj) {
    obj->data = (uint64_t*) malloc(4 * sizeof(uint64_t)); // Allocate memory for 256 bits
    // Make sure to check for malloc failure in production code
}

#define CHECK_VCOOKIE(cookie) do { \
    xcb_generic_error_t* error = xcb_request_check(conn, cookie); \
    if (error) { \
        fprintf(stderr, "XCB request failed with error code %d\n", error->error_code); \
        free(error); \
    } \
} while (0)

class void_err_t {
    private:
        xcb_generic_error_t *err;
        xcb_connection_t *conn;

    public:
        // Constructor takes the connection and the cookie
        void_err_t(xcb_connection_t* __conn, xcb_void_cookie_t cookie)
        : conn(__conn), err(nullptr) {
            checkErr(cookie);

        }
        ~void_err_t() {
            if (err) {
                free(err);/* Free the error pointer if it's not NULL */

            }

        }

        void checkErr(xcb_void_cookie_t cookie) {
            err = xcb_request_check(conn, cookie);
            if (err) {
                loutE << "XCB Error occurred. Error code: " << err->error_code << loutEND;

            }

        }
        bool hasErr() const {
            return err != nullptr;

        }

};

#define V_COKE( ...) do { \
    _err = xcb_request_check(_conn, _cookie); \
    if (_err) { \
        loutE << ERRNO_MSG(__VA_ARGS__) << loutEND; \
        free(_err); \
    } \
} while (0)

#define CHECK_BITFLAG_ERR(__flags, __BITVALUE, ...) do { \
    if (__BITVALUE) { \
        \
    } \
} while (0)

#define CHECK_BIT_ERR(__flags, __BITVALUE, ...)  do { \
    bool error_detected = false; \
    unsigned long __bits[] = { __VA_ARGS__ }; \
    for (size_t i = 0; i < sizeof(__bits) / sizeof(__bits[0]); ++i) { \
        if ((__flags & __bits[i]) == __BITVALUE) { \
            error_detected = true; \
            break; \
        } \
    } \
    error_detected; \
} while(0)

#define CheckUint64_t(__flags, __BITVALUE, __ACTION, ...)  do { \
    bool error_detected = false; \
    unsigned long __bits[] = { __VA_ARGS__ }; \
    for (size_t i = 0; i < sizeof(__bits) / sizeof(__bits[0]); ++i) { \
        if ((__flags & __bits[i]) == __BITVALUE) { \
            error_detected = true; \
            break; \
        } \
    } \
    if (error_detected) { \
        __ACTION; \
    } \
} while(0)

#define lambda(__refrence, ...)  do { \
    [__refrence](...) {\
        \
    } \
} while(0)

#define CHECK_BIT_E(__flags, ...)  do { \
    bool error_detected = false; \
    unsigned long __bits[] = { __VA_ARGS__ }; \
    for (size_t i = 0; i < sizeof(__bits) / sizeof(__bits[0]); ++i) { \
        if ((__flags & __bits[i]) != 0) { \
            error_detected = true; \
            break; \
        } \
    } \
    error_detected; \
} while(0)

#define CHECK_E(__flags, ...) [this](__flags, ...) -> bool { \
    bool error_detected = false; \
    unsigned long __bits[] = { __VA_ARGS__ }; \
    for (size_t i = 0; i < sizeof(__bits) / sizeof(__bits[0]); ++i) { \
        if ((__flags & __bits[i]) != 0) { \
            error_detected = true; \
            break; \
        } \
    } \
    error_detected; \
    return error_detected; \
}

class atom {
    private:
        

    public:

};

class xcb {
    private:
        xcb_connection_t *_conn;
        xcb_screen_t * _s;
        xcb_generic_error_t* _err;
        xcb_void_cookie_t _cookie;

        uint64_t _flags = 0xffffffffffffffff;
        // vector<uint32_t> _xid_vec;
        // void_err_t void_err;

        void check_error();

    public:
        bool is_flag_set(unsigned int __f);
        void set_flag(unsigned int __f);
        void clear_flag(unsigned int __f);
        void toggle_flag(unsigned int __f);

        xcb_intern_atom_cookie_t intern_atom_cookie(const char *__name);
        xcb_intern_atom_reply_t *intern_atom_reply(xcb_intern_atom_cookie_t __cookie);
        xcb_atom_t intern_atom(const char *__name);
        bool window_exists(uint32_t __w);
        uint32_t gen_Xid();
        void window_stack(uint32_t __window1, uint32_t __window2, uint32_t __mode);
        inline bool check_req_err() { return ((_flags & (1ULL << X_REQ_ERROR)) != 0); }
        void mapW(uint32_t __w);

        uint64_t &check_conn();
        void create_w(uint32_t __pw, uint32_t __w, int16_t __x, int16_t __y,
                      uint16_t __width, uint16_t __height);

        

        xcb(xcb_connection_t *__conn, xcb_screen_t *__s);

};
static xcb *xcb(nullptr);

inline class xcb *connect_to_server(xcb_connection_t *__conn, xcb_screen_t *__s) {
    return new class xcb(__conn, __s);
    
}
#endif/* XCB__HPP */