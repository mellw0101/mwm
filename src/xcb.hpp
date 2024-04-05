#ifndef XCB__HPP
#define XCB__HPP
#include <cstdint>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>

// static xcb_connection_t * conn = nullptr;
// static xcb_ewmh_connection_t * ewmh;
// static const xcb_setup_t * setup;
// static xcb_screen_iterator_t iter;
// static xcb_screen_t * screen;

// static xcb_connection_t *conn;

// namespace tools {
//     // size_t slen(const char *__s) {
//     //     size_t i(0); for(; __s[i]; ++i){} return i;
        
//     // }
//     // size_t slen(const char *s) {
//     //     for (size_t i(0);;) if (s[++i]) return i;
        
//     // }
//     // size_t slen(const char *s) {
//     //     size_t i(0); for (; s[++i];){} return i;
        
//     // }
//     // size_t slen(const char *s) {
//     //     for (; *s; ++s){} return s - &s[0];
        
//     // }
//     // size_t slen(const char *s) {
//     //     // for (; *s; ++s){} return s - &s[0];
//     //     while (s[*++s]) {} return s - &s[0];
//     //     while (*++s) ++s; return s - &s[0];
//     //     for (;;) if (!*++s) return s - &s[0];

//     //     for (;;) if (!*++s) return s - &s[0];


//     //     while (*s) ++s; return *s - *&s[0];

//     //     do ++s; while (*s); return *s - *&s[0];

//     // }
//     // size_t slen(const char *s) {
//     //     for (; *s; ++s){} return s - &s[0];
//     //     while (&s[*++s]) {} return s - &s[0];
        
//     // }

//     // size_t slen(const char *__s) {
//     //     for (size_t i(0);; ++i) if (__s[i] == '\0') return i;
        
//     // }
//     // size_t slen(const char *__s) {
//     //     auto f = [__s]() -> size_t { size_t i(0); while (__s[i]) ++i; return i; }();
//     //     return f;
//     // }
//     // size_t slen(const char *__s) {
//     //     size_t i(0); while (__s[i]) ++i; return i;
//     // }
//     // size_t slen(const char *s) {
//     //     size_t i(0); do i++; while (s[i]);  return i;
    
//     // }
//     // inline size_t slen(const char *s) {
//     //     for (;;) if (!*++s) return s - &s[*s];

//     // }
//     // inline size_t slen(const char *s) {
//     //     for (;;) if (!*++s) return s - &s[0];
//         // for (;;) if (!*++s) return s[0] - *s;
//         // while (s[*++s + 1]){} return *s;
//         // for (; s[*++s + 1];){} return s[*s - 1] - *s;
//     // }

//     /**
//      * @p '__s'
//      * @brief function that @return's len of a 'const char *'
//      */
//     /* inline size_t slen(const char *__s) {
//         for(;*__s;) ++__s;
//         return __s - &__s[*__s];

//     } */

// } using namespace tools;

namespace xcb {
    xcb_intern_atom_cookie_t intern_atom_cookie(xcb_connection_t *__c, const char *__name);
    xcb_intern_atom_reply_t *intern_atom_reply(xcb_connection_t *__c, xcb_intern_atom_cookie_t __cookie);
    xcb_atom_t intern_atom(const char *__name);
    bool window_exists(xcb_connection_t *__c, uint32_t __w);
    uint32_t gen_Xid();
    void window_stack(xcb_connection_t *__c, uint32_t __window1, uint32_t __window2, uint32_t __mode);

}

#endif/* XCB__HPP */