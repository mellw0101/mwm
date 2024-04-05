// #include <cstddef>
#include <cstdint>
#include "Log.hpp"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "tools.hpp"


namespace tools {
    // size_t slen(const char *__s) {
    //     size_t i(0); for(; __s[i]; ++i){} return i;
        
    // }
    // size_t slen(const char *s) {
    //     for (size_t i(0);;) if (s[++i]) return i;
        
    // }
    // size_t slen(const char *s) {
    //     size_t i(0); for (; s[++i];){} return i;
        
    // }
    // size_t slen(const char *s) {
    //     for (; *s; ++s){} return s - &s[0];
        
    // }
    // size_t slen(const char *s) {
    //     // for (; *s; ++s){} return s - &s[0];
    //     while (s[*++s]) {} return s - &s[0];
    //     while (*++s) ++s; return s - &s[0];
    //     for (;;) if (!*++s) return s - &s[0];

    //     for (;;) if (!*++s) return s - &s[0];


    //     while (*s) ++s; return *s - *&s[0];

    //     do ++s; while (*s); return *s - *&s[0];

    // }
    // size_t slen(const char *s) {
    //     for (; *s; ++s){} return s - &s[0];
    //     while (&s[*++s]) {} return s - &s[0];
        
    // }

    // size_t slen(const char *__s) {
    //     for (size_t i(0);; ++i) if (__s[i] == '\0') return i;
        
    // }
    // size_t slen(const char *__s) {
    //     auto f = [__s]() -> size_t { size_t i(0); while (__s[i]) ++i; return i; }();
    //     return f;
    // }
    // size_t slen(const char *__s) {
    //     size_t i(0); while (__s[i]) ++i; return i;
    // }
    // size_t slen(const char *s) {
    //     size_t i(0); do i++; while (s[i]);  return i;
    
    // }
    // inline size_t slen(const char *s) {
    //     for (;;) if (!*++s) return s - &s[*s];

    // }
    // inline size_t slen(const char *s) {
    //     for (;;) if (!*++s) return s - &s[0];
        // for (;;) if (!*++s) return s[0] - *s;
        // while (s[*++s + 1]){} return *s;
        // for (; s[*++s + 1];){} return s[*s - 1] - *s;
    // }

    /**
     * @p '__s'
     * @brief function that @return's len of a 'const char *'
     */
    /* inline size_t slen(const char *__s) {
        for(;*__s;) ++__s;
        return __s - &__s[*__s];

    } */

} using namespace tools;
struct conn_t {
    static xcb_connection_t *conn;

    operator xcb_connection_t*(){
        return conn;
    }
    
};

namespace xcb {
    namespace { xcb_connection_t *conn; };

    uint32_t gen_Xid() {
        uint32_t w;
        if ((w = xcb_generate_id(conn))== -1) {
            loutE << "failed to generate Xid" << loutEND;

        }
        return w;

    }
    xcb_intern_atom_cookie_t intern_atom_cookie(const char *__name) {
        return xcb_intern_atom(
            conn,
            0,
            slen(__name),
            __name
            
        );

    }
    xcb_intern_atom_reply_t *intern_atom_reply(xcb_intern_atom_cookie_t __cookie) {
        return xcb_intern_atom_reply(conn, __cookie, nullptr);

    }
    xcb_atom_t intern_atom(const char *__name) {
        xcb_intern_atom_reply_t *rep = intern_atom_reply(intern_atom_cookie(__name));
        if (!rep) {
            loutE << "rep = nullptr could not get intern_atom_reply" << loutEND;
            return XCB_ATOM_NONE;
        
        } xcb_atom_t atom(rep->atom);
        free(rep);
        return atom;

    }
    

}