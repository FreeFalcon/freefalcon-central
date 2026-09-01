// Artscout - 2026 (#104, Linux Ф1): <direct.h> -- MSVC's directory/CWD calls. All have direct POSIX equivalents.
#ifndef FF_WIN32SHIM_DIRECT_H
#define FF_WIN32SHIM_DIRECT_H
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// #104: the engine spells these paths with '\' in mixed case (e.g. FalconDataDirectory + "\\Pictures"), while the
// data tree lives lowercase on ext4. A bare ::mkdir/::chdir on that string would create/miss a single oddly-named
// entry with a literal backslash (winmain _mkdir(FalconPictureDirectory) did exactly that every launch). Route
// through FF_CIResolvePath (win32compat.cpp): it normalises '\'->'/' and case-folds the components that already
// exist, keeping any new tail as spelled -- so mkdir lands the new dir under the correctly-cased parents.
#ifdef __cplusplus
extern "C"
#endif
    int FF_CIResolvePath(const char* want, char* out, unsigned long cap);
static inline const char* ff__ci_dir(const char* p, char* buf,
                                     unsigned long cap)
{
    if (!p)
        return p;
    buf[0] = '\0';
    FF_CIResolvePath(p, buf, cap);
    return buf[0] ? buf : p;
}

static inline int _mkdir(const char* p)
{
    char b[4096];
    return ::mkdir(ff__ci_dir(p, b, sizeof b), 0775);
}
static inline int _rmdir(const char* p)
{
    char b[4096];
    return ::rmdir(ff__ci_dir(p, b, sizeof b));
}
static inline int _chdir(const char* p)
{
    char b[4096];
    return ::chdir(ff__ci_dir(p, b, sizeof b));
}
static inline char* _getcwd(char* b, int n)
{
    return ::getcwd(b, (size_t)n);
}
static inline int _getdrive(void)
{
    return 3;
} // no drive letters on Linux; 3 == "C:" keeps callers sane
static inline int _chdrive(int)
{
    return 0;
}
#endif
