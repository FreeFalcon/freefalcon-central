// Artscout - 2026 (#104, Linux Ф1): MSVC CRT string helpers the codebase uses that libc lacks under those names:
// stricmp/strnicmp (-> strcasecmp), _strdup, itoa/ltoa/ultoa, strlwr/strupr, the secure _s variants, and the
// GetPrivateProfile* .ini API (implemented in win32compat.cpp). Pulled in by windows.h. Linux-only.
#ifndef FF_WIN32SHIM_STRINGS_H
#define FF_WIN32SHIM_STRINGS_H
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstdarg>
#include <strings.h> // strcasecmp / strncasecmp

// ---- case-insensitive compare ----
static inline int stricmp(const char* a, const char* b)
{
    return ::strcasecmp(a, b);
}
static inline int _stricmp(const char* a, const char* b)
{
    return ::strcasecmp(a, b);
}
static inline int strcmpi(const char* a, const char* b)
{
    return ::strcasecmp(a, b);
}
static inline int _strcmpi(const char* a, const char* b)
{
    return ::strcasecmp(a, b);
}
static inline int strnicmp(const char* a, const char* b, size_t n)
{
    return ::strncasecmp(a, b, n);
}
static inline int _strnicmp(const char* a, const char* b, size_t n)
{
    return ::strncasecmp(a, b, n);
}
static inline int _stricoll(const char* a, const char* b)
{
    return ::strcasecmp(a, b);
}
static inline int _memicmp(const void* a, const void* b, size_t n)
{
    const unsigned char *x = (const unsigned char*)a,
                        *y = (const unsigned char*)b;
    for (size_t i = 0; i < n; ++i)
    {
        int d = tolower(x[i]) - tolower(y[i]);
        if (d)
            return d;
    }
    return 0;
}

// ---- dup / case convert (in place) ----
static inline char* _strdup(const char* s)
{
    return ::strdup(s);
}
static inline char* strlwr(char* s)
{
    for (char* p = s; *p; ++p)
        *p = (char)tolower((unsigned char)*p);
    return s;
}
static inline char* _strlwr(char* s)
{
    return strlwr(s);
}
static inline char* strupr(char* s)
{
    for (char* p = s; *p; ++p)
        *p = (char)toupper((unsigned char)*p);
    return s;
}
static inline char* _strupr(char* s)
{
    return strupr(s);
}

// ---- integer -> string (base) ----
static inline char* itoa(int v, char* buf, int base)
{
    if (base == 10)
    {
        sprintf(buf, "%d", v);
        return buf;
    }
    if (base == 16)
    {
        sprintf(buf, "%x", (unsigned)v);
        return buf;
    }
    if (base == 8)
    {
        sprintf(buf, "%o", (unsigned)v);
        return buf;
    }
    // generic base 2..36
    char tmp[64];
    int i = 0;
    unsigned u = (unsigned)v;
    if (u == 0)
        tmp[i++] = '0';
    while (u)
    {
        int d = u % base;
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        u /= base;
    }
    int j = 0;
    while (i > 0)
        buf[j++] = tmp[--i];
    buf[j] = 0;
    return buf;
}
static inline char* _itoa(int v, char* buf, int base)
{
    return itoa(v, buf, base);
}
static inline char* ltoa(long v, char* buf, int base)
{
    return itoa((int)v, buf, base);
}
static inline char* _ltoa(long v, char* buf, int base)
{
    return itoa((int)v, buf, base);
}
static inline char* ultoa(unsigned long v, char* buf, int base)
{
    sprintf(buf, base == 16 ? "%lx" : "%lu", v);
    return buf;
}
static inline char* _ultoa(unsigned long v, char* buf, int base)
{
    return ultoa(v, buf, base);
}
static inline char* _i64toa(long long v, char* buf, int)
{
    sprintf(buf, "%lld", v);
    return buf;
}

// ---- non-standard printf names ----
#ifndef _snprintf
#define _snprintf snprintf
#endif
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
static inline int _vscprintf(const char* fmt, va_list ap)
{
    va_list c;
    va_copy(c, ap);
    int n = vsnprintf(nullptr, 0, fmt, c);
    va_end(c);
    return n;
}

// MSVC CRT math spelling used by the audio code.
#include <cmath>
static inline double _copysign(double x, double y)
{
    return ::copysign(x, y);
}

// ---- secure CRT (_s): the MSVC bounds-checked variants. Map to the bounded libc calls (behaviour is close
// enough for a port; the size arg is honoured). errno_t return -> 0 on success. ----
typedef int errno_t;
static inline errno_t strcpy_s(char* d, size_t n, const char* s)
{
    if (!d || !s || n == 0)
        return 22;
    strncpy(d, s, n);
    d[n - 1] = 0;
    return 0;
}
static inline errno_t strcat_s(char* d, size_t n, const char* s)
{
    if (!d || !s)
        return 22;
    size_t l = strlen(d);
    if (l < n)
        strncat(d, s, n - l - 1);
    return 0;
}
static inline errno_t strncpy_s(char* d, size_t n, const char* s, size_t c)
{
    if (!d || !s || n == 0)
        return 22;
    size_t m = c < n - 1 ? c : n - 1;
    strncpy(d, s, m);
    d[m] = 0;
    return 0;
}
#define sprintf_s snprintf
#define _snprintf_s(buf, size, count, ...) snprintf((buf), (size), __VA_ARGS__)
static inline int vsprintf_s(char* d, size_t n, const char* fmt, va_list ap)
{
    return vsnprintf(d, n, fmt, ap);
}
static inline int _vsnprintf_s(char* d, size_t n, size_t, const char* fmt,
                               va_list ap)
{
    return vsnprintf(d, n, fmt, ap);
}

// ---- .ini profile API -> win32compat.cpp (parses the file; config is mostly XML now but some .ini reads remain) ----
extern "C"
{
    DWORD GetPrivateProfileIntA(LPCSTR sect, LPCSTR key, int def, LPCSTR file);
    DWORD GetPrivateProfileStringA(LPCSTR sect, LPCSTR key, LPCSTR def,
                                   LPSTR buf, DWORD size, LPCSTR file);
    BOOL WritePrivateProfileStringA(LPCSTR sect, LPCSTR key, LPCSTR val,
                                    LPCSTR file);
}
#define GetPrivateProfileInt GetPrivateProfileIntA
#define GetPrivateProfileString GetPrivateProfileStringA
#define WritePrivateProfileString WritePrivateProfileStringA

// ---- IsBadStringPtr: a validity guard the engine wraps in ShiAssert (controltab.cpp:2937, render2d.cpp:1221) -----
// The Win32 API returns nonzero when the process lacks read access to the string. There is no portable way to probe
// arbitrary addresses on Linux without catching a fault, and Microsoft itself deprecated these calls as unreliable
// (a valid pointer can fault a guard page mid-probe). The honest, defensible subset is the NULL check: NULL is bad,
// anything else is assumed readable -- exactly what the modern guidance ("just trust the pointer") amounts to. This
// is NOT a silent success stub: it answers the one case it can answer truthfully and does not pretend to do more.
static inline BOOL IsBadStringPtrA(LPCSTR lpsz, UINT_PTR /*ucchMax*/)
{
    return lpsz == NULL ? TRUE : FALSE;
}
static inline BOOL IsBadStringPtrW(const WCHAR* lpsz, UINT_PTR /*ucchMax*/)
{
    return lpsz == NULL ? TRUE : FALSE;
}
#define IsBadStringPtr IsBadStringPtrA

// ---- wsprintf / wvsprintf ---------------------------------------------------------------------------------------
// Despite the `w`, these are the ANSI USER32 formatters, not wide-char ones -- wsprintfA is what the un-suffixed name
// resolves to in a non-UNICODE build, and the callers pass char* (comsup.h:196, misseval.cpp:866, path.cpp:36).
// sprintf/vsprintf are a superset of what they do: Windows' versions support no floating-point conversions and cap
// output at 1024 chars, so anything they could format, these format identically. Deliberately NOT the bounded
// snprintf: the Win32 signature carries no buffer size, so there is nothing truthful to pass as a limit -- inventing
// one would silently truncate where Windows does not. The 1024-char cap is the caller's contract on both platforms.
static inline int wvsprintfA(char* buf, const char* fmt, va_list ap)
{
    return vsprintf(buf, fmt, ap);
}
static inline int wsprintfA(char* buf, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}
#define wsprintf wsprintfA
#define wvsprintf wvsprintfA

// ---- wide strings over WCHAR ------------------------------------------------------------------------------------
// On MSVC wchar_t IS 16-bit, so wcslen/wcscpy/wcsncpy take a WCHAR* directly. Here they cannot: WCHAR is uint16_t
// (windows.h:113 -- correct, Win WCHAR is UTF-16) while Linux's wchar_t is 32-bit, so glibc's wcs* family wants a
// different type and every call on a WCHAR* failed to resolve (comsup.h:124/128, reached by six sources; also
// dxutil.cpp:498+). Widening WCHAR to wchar_t is not an option -- it is a wire/ABI type read from files.
//
// These are OVERLOADS, not replacements: uint16_t and wchar_t are distinct types, so glibc's wchar_t versions stay
// visible and unambiguous. They count UTF-16 code units rather than code points -- which is precisely what the
// Windows originals do (a surrogate pair counts as 2 there too), so callers sizing buffers keep the same meaning.
inline size_t wcslen(const WCHAR* s)
{
    const WCHAR* p = s;
    while (*p)
        ++p;
    return (size_t)(p - s);
}
inline WCHAR* wcscpy(WCHAR* d, const WCHAR* s)
{
    WCHAR* r = d;
    while ((*d++ = *s++) != 0)
    {
    }
    return r;
}
inline WCHAR* wcsncpy(WCHAR* d, const WCHAR* s, size_t n)
{
    WCHAR* r = d;
    size_t i = 0;
    for (; i < n and s[i]; ++i)
        d[i] = s[i];
    for (; i < n; ++i)
        d[i] = 0;
    return r;
}
inline int wcscmp(const WCHAR* a, const WCHAR* b)
{
    while (*a and *a == *b)
    {
        ++a;
        ++b;
    }
    return (int)*a - (int)*b;
}
inline WCHAR* wcscat(WCHAR* d, const WCHAR* s)
{
    WCHAR* r = d;
    while (*d)
        ++d;
    while ((*d++ = *s++) != 0)
    {
    }
    return r;
}

// ---- ANSI/wide code-page conversion ----
// Artscout - 2026 (Linux port): MultiByteToWideChar/WideCharToMultiByte convert between a narrow
// code page and UTF-16 (WCHAR is 16-bit). On Linux the narrow encoding is UTF-8 (the standard
// locale), so CP_ACP/CP_UTF8/CP_OEMCP all map to a real UTF-8 <-> UTF-16 codec (see win32compat.cpp).
// Used by dxutil.cpp (GUID/string helpers) and the Vulkan backend's file-path widening.
#ifndef CP_ACP
#define CP_ACP 0
#define CP_OEMCP 1
#define CP_UTF8 65001
#endif
extern "C" int MultiByteToWideChar(UINT codePage, DWORD dwFlags, LPCSTR src,
                                   int cbSrc, LPWSTR dst, int cchDst);
extern "C" int WideCharToMultiByte(UINT codePage, DWORD dwFlags, LPCWSTR src,
                                   int cchSrc, LPSTR dst, int cbDst,
                                   LPCSTR defChar, LPBOOL usedDefault);

// _stscanf (TCHAR sscanf) -> narrow sscanf (the codebase is not compiled UNICODE).
#ifndef _stscanf
#define _stscanf sscanf
#endif

#endif // FF_WIN32SHIM_STRINGS_H
