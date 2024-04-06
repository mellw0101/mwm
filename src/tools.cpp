#include "tools.hpp"
#include <cstddef>

/**
 * @p '__s'
 * @brief function that @return's len of a 'const char *'
 **/
size_t slen(const char *__s) {
    size_t i(0);
    for(; __s[i]; ++i){}
    return i;

}