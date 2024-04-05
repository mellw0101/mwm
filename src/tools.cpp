#include "tools.hpp"

namespace /* slen */ {
    /* size_t slen(const char *__s) {
        size_t i(0); for(; __s[i]; ++i){} return i;
        
    } */
    /* size_t slen(const char *s) {
        for (size_t i(0);;) if (s[++i]) return i;
        
    } */
    /* size_t slen(const char *s) {
        size_t i(0); for (; s[++i];){} return i;
        
    } */
    /* size_t slen(const char *s) {
        for (; *s; ++s){} return s - &s[0];
        
    } */
    /* size_t slen(const char *s) {
        // for (; *s; ++s){} return s - &s[0];
        while (s[*++s]) {} return s - &s[0];
        while (*++s) ++s; return s - &s[0];
        for (;;) if (!*++s) return s - &s[0];

        for (;;) if (!*++s) return s - &s[0];


        while (*s) ++s; return *s - *&s[0];

        do ++s; while (*s); return *s - *&s[0];

    } */
    /* size_t slen(const char *s) {
        for (; *s; ++s){} return s - &s[0];
        while (&s[*++s]) {} return s - &s[0];
        
    } */
    /* size_t slen(const char *__s) {
        for (size_t i(0);; ++i) if (__s[i] == '\0') return i;
        
    } */
    /* size_t slen(const char *__s) {
        auto f = [__s]() -> size_t { size_t i(0); while (__s[i]) ++i; return i; }();
        return f;
    } */
    /* size_t slen(const char *__s) {
        size_t i(0); while (__s[i]) ++i; return i;
    } */
    /* size_t slen(const char *s) {
        size_t i(0); do i++; while (s[i]);  return i;
    
    } */
    /* inline size_t slen(const char *s) {
        for (;;) if (!*++s) return s - &s[*s];

    } */
    /* inline size_t slen(const char *s) {
        for (;;) if (!*++s) return s - &s[0];
        for (;;) if (!*++s) return s[0] - *s;
        while (s[*++s + 1]){} return *s;
        for (; s[*++s + 1];){} return s[*s - 1] - *s;

    } */
    /* size_t slen(const char *s) {
        for (;;) if (!*++s) return s - &s[0];

    } */
    /* inline size_t while_slen(const char *s) {
        while(*s)++s; return s - &s[*s];
        
    } */
    // inline size_t slen(const char *s) {
    //     for (; *s; ++s);
    //     return s - &s[*s];

    // }

}
inline size_t slen(const char *__s) {
    for (; *__s;) ++__s;
    return __s - &__s[*__s];

}