#ifndef XCB__HPP
#define XCB__HPP

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <utility>
#include <vector>
#include <xcb/xproto.h>
#include "data.hpp"
#include "tools.hpp"
#include "Log.hpp"
#include "globals.h"

using namespace std;


#define X_CONN_ERROR 1
#define X_REQ_ERROR 2
#define X_ID_GEN_ERROR 3
#define X_CREATE_ERR 4
#define X_W_CREATION_ERR 5


#include <xcb/xcb.h>

class AtomMonitor {
    private:
        /* xcb_connection_t* _conn; */
        xcb_atom_t* _atomPtr; // Pointer to atom value

    public:
        AtomMonitor(xcb_connection_t* conn) : /* _conn(conn), */ _atomPtr(new xcb_atom_t) {
            // Initialize the atom value to XCB_ATOM_NONE
            *_atomPtr = XCB_ATOM_NONE;
        }

        ~AtomMonitor() {
            // Clean up the dynamically allocated atom
            delete _atomPtr;
        }

        // Fetch and store an atom by name
        void fetchAndStoreAtom(const char* name) {
            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, slen(name), name);
            xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(conn, cookie, nullptr);

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
        /* xcb_connection_t *conn; */

    public:
        // Constructor takes the connection and the cookie
        void_err_t(xcb_connection_t* __conn, xcb_void_cookie_t cookie)
        : /* conn(__conn), */ err(nullptr) {
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
    _err = xcb_request_check(conn, _cookie); \
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

#define Vlist(__type, ...) (__type[]){__VA_ARGS__}

class iAtomC
{
    xcb_intern_atom_cookie_t _cookie;

public:
        /* intern_atom_cok_t(xcb_intern_atom_cookie_t __cookie) { this->_cookie = __cookie; } */
        iAtomC(bool __only_if_exists, const char *__name)
        : _cookie(
            xcb_intern_atom(
                conn,
                __only_if_exists,
                slen(__name),
                __name
            )
        ) {}

        operator xcb_intern_atom_cookie_t() { return _cookie; }
        operator const xcb_intern_atom_cookie_t&() const { return this->_cookie; }

};

inline void setErrState() {}

#define ERR_STATE(__int) __int = 1 << 7

template<typename T, typename... Args>
inline void setErrState(T& first, Args&... args)
{
    ERR_STATE(first);    
    setErrState(args...); // Recurse for the rest of the arguments
}

#define set_ERR_STATE(...) do { \
    setErrState(__VA_ARGS__); \
} while(0)


class iAtomR
{
    uint8_t    response_type;
    uint8_t    pad0;
    uint16_t   sequence;
    uint32_t   length;
    xcb_atom_t atom;

public:
    iAtomR(const iAtomC &__cookie)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, __cookie, nullptr);
        if (!reply)
        {
            set_ERR_STATE(response_type, pad0, sequence, length, atom);
            return;
        }
        
        response_type = reply->response_type;
        pad0          = reply->pad0;
        sequence      = reply->sequence;
        length        = reply->sequence;
        atom          = reply->atom;
        
        free(reply);
    }

    iAtomR(bool __only_if_exists, const char *__name)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, iAtomC(__only_if_exists, __name), nullptr);
        if ( !reply )
        {
            set_ERR_STATE(response_type, pad0, sequence, length, atom);
            return;
        }

        response_type = reply->response_type;
        pad0          = reply->pad0;
        sequence      = reply->sequence;
        length        = reply->sequence;
        atom          = reply->atom;

        free(reply);            
    }        

    operator xcb_atom_t() { return atom; }
    operator xcb_atom_t&() { return atom; }

    bool is_not_valid()
    {
        return (
            response_type == 1 << 7
        &&  pad0          == 1 << 7
        &&  sequence      == 1 << 7
        &&  length        == 1 << 7
        &&  atom          == 1 << 7);
    }

    uint32_t Atom() const
    {
        return atom;
    }
};

class wAttrC
{
    xcb_get_window_attributes_cookie_t _cookie;

public:
    wAttrC(uint32_t __w)
    : _cookie(xcb_get_window_attributes(conn, __w)) {}

    operator xcb_get_window_attributes_cookie_t () { return _cookie; }
    operator const xcb_get_window_attributes_cookie_t &() const { return this->_cookie; }
};

class wAttrR
{
public:
    uint8_t        response_type;
    uint8_t        backing_store;
    uint16_t       sequence;
    uint32_t       length;
    xcb_visualid_t visual;
    uint16_t       _class;
    uint8_t        bit_gravity;
    uint8_t        win_gravity;
    uint32_t       backing_planes;
    uint32_t       backing_pixel;
    uint8_t        save_under;
    uint8_t        map_is_installed;
    uint8_t        map_state;
    uint8_t        override_redirect;
    xcb_colormap_t colormap;
    uint32_t       all_event_masks;
    uint32_t       your_event_mask;
    uint16_t       do_not_propagate_mask;
    uint8_t        pad0[2];

    wAttrR(xcb_get_window_attributes_cookie_t __cookie)
    {
        xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, __cookie, nullptr);
        if ( reply )
        {
            response_type = reply->response_type;
            backing_store = reply->backing_store;
            sequence = reply->sequence;
            length = reply->length;
            visual = reply->visual;
            _class = reply->_class;
            bit_gravity = reply->bit_gravity;
            win_gravity = reply->win_gravity;
            backing_planes = reply->backing_planes;
            backing_pixel = reply->backing_pixel;
            save_under = reply->save_under;
            map_is_installed = reply->map_is_installed;
            map_state = reply->map_state;
            override_redirect = reply->override_redirect;
            colormap = reply->colormap;
            all_event_masks = reply->all_event_masks;
            your_event_mask = reply->your_event_mask;
            do_not_propagate_mask = reply->do_not_propagate_mask;
            pad0[0] = reply->pad0[0];
            pad0[1] = reply->pad0[1];
            free(reply);
        }
        else
        {
            set_ERR_STATE(response_type, backing_store, sequence, length, visual, _class, bit_gravity, win_gravity, backing_planes, backing_pixel, save_under, map_is_installed, map_state, override_redirect, colormap, all_event_masks, your_event_mask, do_not_propagate_mask, pad0[0], pad0[1]);
        }
    }

    bool is_not_valid()
    {
        return (
            (   response_type == 1 << 7
            &&  backing_store == 1 << 7
            &&  sequence == 1 << 7
            &&  length == 1 << 7
            &&  visual == 1 << 7
            &&  _class == 1 << 7
            &&  bit_gravity == 1 << 7
            &&  win_gravity == 1 << 7
            &&  backing_planes == 1 << 7
            &&  backing_pixel == 1 << 7
            &&  save_under == 1 << 7
            &&  map_is_installed == 1 << 7
            &&  map_state == 1 << 7
            &&  override_redirect == 1 << 7
            &&  colormap == 1 << 7
            &&  all_event_masks == 1 << 7
            &&  your_event_mask == 1 << 7
            &&  do_not_propagate_mask == 1 << 7
            &&  pad0[0] == 1 << 7
            &&  pad0[1] == 1 << 7 )
        );
    }

    wAttrR(uint32_t __w)
    {
        xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(conn, wAttrC(__w), nullptr);
        if ( reply )
        {
            response_type = reply->response_type;
            backing_store = reply->backing_store;
            sequence = reply->sequence;
            length = reply->length;
            visual = reply->visual;
            _class = reply->_class;
            bit_gravity = reply->bit_gravity;
            win_gravity = reply->win_gravity;
            backing_planes = reply->backing_planes;
            backing_pixel = reply->backing_pixel;
            save_under = reply->save_under;
            map_is_installed = reply->map_is_installed;
            map_state = reply->map_state;
            override_redirect = reply->override_redirect;
            colormap = reply->colormap;
            all_event_masks = reply->all_event_masks;
            your_event_mask = reply->your_event_mask;
            do_not_propagate_mask = reply->do_not_propagate_mask;
            pad0[0] = reply->pad0[0];
            pad0[1] = reply->pad0[1];
            free(reply);
        }
    }
};

typedef struct {
    uint8_t    response_type;
    uint8_t    pad0;
    uint16_t   sequence;
    uint32_t   length;
    xcb_atom_t atom;
    
} atom_t;

class atoms_t {
    private:
        void fetch_atom_data(xcb_connection_t *conn, char *__name);
        xcb_connection_t *conn;

        unordered_map<const char *, uint32_t> data;


    public:
        uint32_t get(const char *__atom_name) {
            auto it = data.find(__atom_name);
            if (it == data.end()) return 0; 
            
            return it->second;

        }
        
        void add(char *__atoms[])
        {
            for (size_t i(0); __atoms[i]; ++i)
            {
                data[__atoms[i]] = iAtomR(0, __atoms[i]).Atom();
            }    
        }

        // atoms_t(xcb_connection_t *conn, char **__atoms);
        atoms_t() {};
        // ~atoms_t();

};

#define init_atom(...) do {\
    \
} while(0)

class xcb {
    /* xcb_connection_t *_conn; */
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
    xcb_atom_t get_atom(uint8_t __only_if_exists, const char *name);
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

    xcb(/* xcb_connection_t *__conn, */ xcb_screen_t *__s);
};
static class xcb *xcb(nullptr);

inline class xcb *connect_to_server(xcb_connection_t *__conn, xcb_screen_t *__s)
{
    return new class xcb(/* __conn, */ __s);
}

#endif/* XCB__HPP */