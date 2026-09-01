// Artscout - 2026 (#104, Linux Ф1): MSVC <io.h> low-level I/O -> POSIX. The `_`-prefixed CRT file API (_open,
// _read, _access, _findfirst, ...). The simple ones wrap POSIX inline here; the stateful _findfirst family is
// implemented in win32compat.cpp. Linux-only (this dir is on the include path only for the clang build).
#ifndef FF_WIN32SHIM_IO_H
#define FF_WIN32SHIM_IO_H
#ifdef _WIN32
#error "win32shim/io.h is the Linux shim."
#endif

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>

// ---- open mode flags: MSVC _O_* -> POSIX O_* ; text/binary are no-ops on Linux ----
#ifndef _O_RDONLY
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR O_RDWR
#define _O_APPEND O_APPEND
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_EXCL O_EXCL
#define _O_BINARY 0
#define _O_TEXT 0
#define _O_RANDOM 0
#define _O_SEQUENTIAL 0
#define _O_TEMPORARY 0
#endif
// permission bits for _open's third arg
#ifndef _S_IREAD
#define _S_IREAD 0400
#define _S_IWRITE 0200
#define _S_IEXEC 0100
#endif
// _access modes: MSVC 00=exist 02=write 04=read 06=r/w -- numerically compatible with POSIX F_OK/W_OK/R_OK.

// Case-insensitive path resolver (win32compat.cpp). Normalises '\'->'/' and matches the on-disk tree
// case-insensitively -- the same resolver fopen (FF_CIFopen) and CreateFileA use. Declared here so the
// low-level _open below resolves too: reslib opens its archives/loose files via _open, and the paths it
// builds are backslash-separated mixed case, which a bare ::open on ext4 would miss.
extern "C" int FF_CIResolvePath(const char* want, char* out, unsigned long cap);

// ---- simple wrappers (inline) ----
static inline int _open(const char* path, int oflag, int pmode = 0644)
{
    if (path &&
        (oflag &
         O_CREAT)) // create: resolve parents (case) + normalise '\' so the new file lands right
    {
        char cw[4096];
        cw[0] = '\0';
        FF_CIResolvePath(path, cw, sizeof(cw));
        if (cw[0])
            return ::open(cw, oflag, pmode);
    }
    int fd = ::open(path, oflag, pmode);
    if (fd < 0 && path &&
        !(oflag &
          O_CREAT)) // read miss: case-insensitively resolve against the tree and retry
    {
        char ci[4096];
        if (FF_CIResolvePath(path, ci, sizeof(ci)))
            fd = ::open(ci, oflag, pmode);
    }
    return fd;
}
static inline int _close(int fd)
{
    return ::close(fd);
}
static inline int _read(int fd, void* buf, unsigned n)
{
    return (int)::read(fd, buf, n);
}
static inline int _write(int fd, const void* buf, unsigned n)
{
    return (int)::write(fd, buf, n);
}
static inline long _lseek(int fd, long off, int origin)
{
    return (long)::lseek(fd, off, origin);
}
static inline int64_t _lseeki64(int fd, int64_t off, int origin)
{
    return (int64_t)::lseek(fd, (off_t)off, origin);
}
static inline long _tell(int fd)
{
    return (long)::lseek(fd, 0, SEEK_CUR);
}
static inline int _access(const char* path, int mode)
{
    int r = ::access(path, mode);
    if (r != 0 && path)
    {
        char ci[4096];
        if (FF_CIResolvePath(path, ci, sizeof(ci)))
            r = ::access(ci, mode);
    }
    return r;
}
static inline int _unlink(const char* path)
{
    char ci[4096];
    ci[0] = '\0';
    if (path)
        FF_CIResolvePath(path, ci, sizeof(ci));
    return ::unlink(ci[0] ? ci : path);
}
static inline int _dup(int fd)
{
    return ::dup(fd);
}
static inline int _dup2(int a, int b)
{
    return ::dup2(a, b);
}
static inline int _isatty(int fd)
{
    return ::isatty(fd);
}
static inline int _chmod(const char* path, int mode)
{
    return ::chmod(path, (mode_t)mode);
}
static inline int _chsize(int fd, long size)
{
    return ::ftruncate(fd, (off_t)size);
}
static inline int _commit(int fd)
{
    return ::fsync(fd);
}
static inline int _setmode(int /*fd*/, int /*mode*/)
{
    return _O_BINARY;
} // Linux has no text mode
static inline long _filelength(int fd)
{
    struct stat st;
    return ::fstat(fd, &st) == 0 ? (long)st.st_size : -1L;
}
static inline int64_t _filelengthi64(int fd)
{
    struct stat st;
    return ::fstat(fd, &st) == 0 ? (int64_t)st.st_size : -1;
}
static inline int _eof(int fd)
{
    off_t c = ::lseek(fd, 0, SEEK_CUR);
    struct stat st;
    if (::fstat(fd, &st) != 0)
        return -1;
    return c >= st.st_size ? 1 : 0;
}

// ---- _findfirst family (stateful) -> win32compat.cpp over opendir/fnmatch ----
typedef unsigned int _fsize_t;
struct _finddata_t
{
    unsigned attrib;
    long time_create, time_access, time_write;
    _fsize_t size;
    char name[260];
};
struct _finddata64_t
{
    unsigned attrib;
    long long time_create, time_access, time_write;
    long long size;
    char name[260];
};
// _A_* file-attribute bits reported in _finddata_t::attrib
#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH 0x20

extern "C"
{
    intptr_t _findfirst(const char* spec, struct _finddata_t* data);
    int _findnext(intptr_t handle, struct _finddata_t* data);
    int _findclose(intptr_t handle);
}

#endif // FF_WIN32SHIM_IO_H
