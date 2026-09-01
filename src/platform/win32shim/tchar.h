// Artscout - 2026 (#104, Linux Ф1): MSVC <tchar.h> -> the ANSI mapping. The codebase is NOT compiled UNICODE, so
// TCHAR == char and every _t* maps to the plain narrow CRT function. Linux-only shim.
#ifndef FF_WIN32SHIM_TCHAR_H
#define FF_WIN32SHIM_TCHAR_H
#ifdef _WIN32
#error "win32shim/tchar.h is the Linux shim."
#endif
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstdarg>

#ifndef TCHAR
typedef char _TCHAR;
#endif
#define _T(x) x
#define _TEXT(x) x
#define __T(x) x
#define TEXT(x) x

// string length / copy / concat / compare
#define _tcslen strlen
#define _tcscpy strcpy
#define _tcsncpy strncpy
#define _tcscat strcat
#define _tcsncat strncat
#define _tcscmp strcmp
#define _tcsncmp strncmp
#define _tcsicmp strcasecmp
#define _tcsnicmp strncasecmp
#define _tcsncicmp strncasecmp
#define _tcscoll strcoll
#define _tcsdup strdup
// search
#define _tcschr strchr
#define _tcsrchr strrchr
#define _tcsstr strstr
#define _tcspbrk strpbrk
#define _tcsspn strspn
#define _tcscspn strcspn
#define _tcstok strtok
// _tcsspnp (MSVC extension): pointer to the first char in `s` NOT in `set`, or NULL if `s`
// is composed entirely of characters from `set`.
static inline char* ff_strspnp(const char* s, const char* set)
{
    s += strspn(s, set);
    return *s ? (char*)s : (char*)0;
}
#define _tcsspnp ff_strspnp
#define _strspnp ff_strspnp
// case
#define _tcslwr(s)                                                             \
    ({                                                                         \
        char* _p = (s);                                                        \
        for (; *_p; ++_p)                                                      \
            *_p = (char)tolower((unsigned char)*_p);                           \
        (s);                                                                   \
    })
#define _tcsupr(s)                                                             \
    ({                                                                         \
        char* _p = (s);                                                        \
        for (; *_p; ++_p)                                                      \
            *_p = (char)toupper((unsigned char)*_p);                           \
        (s);                                                                   \
    })
// convert
#define _tstoi atoi
#define _ttoi atoi
#define _ttol atol
#define _tstol atol
#define _tcstol strtol
#define _tcstoul strtoul
#define _tcstod strtod
#define _tstof atof
#define _tcstoll strtoll
// char classification
#define _istdigit isdigit
#define _istalpha isalpha
#define _istalnum isalnum
#define _istspace isspace
#define _istupper isupper
#define _istlower islower
#define _totupper toupper
#define _totlower tolower
// stdio / formatted
#define _tprintf printf
#define _ftprintf fprintf
#define _stprintf sprintf
#define _sntprintf snprintf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _tfopen fopen
#define _tfreopen freopen
#define _fgetts fgets
#define _fputts fputs
#define _tremove remove
#define _trename rename
#define _tprintf_s printf
// single char
#define _TCHAR char
#define _TINT int
#define _tmain main
#define _tWinMain WinMain
#define _tcsclen strlen
#define _tcsnccpy strncpy

#endif // FF_WIN32SHIM_TCHAR_H
