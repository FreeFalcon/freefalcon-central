// Artscout - 2026 (#104, Linux Ф1): <mbstring.h> -- MSVC's multibyte string helpers. The engine is ANSI/single-byte
// (see TCHAR == char in windows.h), so the _mbs* calls it makes are byte-wise and map straight onto str*.
#ifndef FF_WIN32SHIM_MBSTRING_H
#define FF_WIN32SHIM_MBSTRING_H
#include <cstring>
#include <cctype>
static inline unsigned char* _mbsinc(const unsigned char* s)
{
    return (unsigned char*)(s + 1);
}
static inline unsigned char* _mbsdec(const unsigned char* start,
                                     const unsigned char* cur)
{
    return (cur > start) ? (unsigned char*)(cur - 1) : nullptr;
}
static inline size_t _mbslen(const unsigned char* s)
{
    return ::strlen((const char*)s);
}
static inline unsigned char* _mbschr(const unsigned char* s, unsigned int c)
{
    return (unsigned char*)::strchr((const char*)s, (int)c);
}
static inline unsigned char* _mbsrchr(const unsigned char* s, unsigned int c)
{
    return (unsigned char*)::strrchr((const char*)s, (int)c);
}
static inline int _mbscmp(const unsigned char* a, const unsigned char* b)
{
    return ::strcmp((const char*)a, (const char*)b);
}
static inline unsigned char* _mbsstr(const unsigned char* s,
                                     const unsigned char* sub)
{
    return (unsigned char*)::strstr((const char*)s, (const char*)sub);
}
#endif
