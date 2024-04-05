#include <cstddef>
#include <cstdint>
#include "Log.hpp"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
static xcb_connection_t *conn;

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
    size_t slen(const char *s) {
        // for (; *s; ++s){} return s - &s[0];
        while (s[*++s]) {} return s - &s[0];
        while (*++s) ++s; return s - &s[0];
        for (;;) if (!*++s) return s - &s[0];

        for (;;) if (!*++s) return *s - *&s[0];


        while (*s) ++s; return *s - *&s[0];

        do ++s; while (*s); return *s - *&s[0];

    }
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

 

} using namespace tools;

namespace xcb {
    uint32_t gen_Xid() {
        uint32_t w = xcb_generate_id(conn);
        if (w == UINT32_MAX) {
            loutE << "failed to generate Xid" << loutEND;

        } return w;

    }
    xcb_intern_atom_cookie_t intern_atom(const char *__name) {

        return xcb_intern_atom(
            conn,
            0,
            slen(__name),
            __name
            
        );
        
    }

}