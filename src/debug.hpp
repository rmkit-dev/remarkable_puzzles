#ifndef DEBUG_HPP
#define DEBUG_HPP

#ifdef NDEBUG
#define debugf(...)
#else
#include <cstdarg>
#include <cstdio>
inline void debugf(const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
}
#endif

#endif // DEBUG_HPP
