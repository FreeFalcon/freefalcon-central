// Artscout - 2026 (#104, Linux Ф1): POSIX implementation of the handle-based Win32 API the shim declares
// (win32_file.h / win32_sync.h). Compiled ONLY into the Linux build. A HANDLE is a pointer to a Win32Handle that
// tags what it wraps (file fd, find dir, thread, event) so one CloseHandle serves all of them, as on Windows.
#ifdef _WIN32
#error "win32compat.cpp is Linux-only."
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE                                                            \
    1 // pthread_setaffinity_np / cpu_set_t (used by SetThreadAffinityMask)
#endif
#include <sched.h>

#include <windows.h> // resolves to the shim on the Linux include path
#include <io.h> // _finddata_t + the _findfirst family declared here
#include <cerrno>
#include <cstring>
#include <string>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sys/mman.h> // mmap/munmap/msync -- the real backing for the file-mapping API
#include <sys/stat.h>

struct Win32Handle
{
    enum Kind
    {
        FILE_K,
        FIND_K,
        THREAD_K,
        EVENT_K,
        MAP_K
    } kind;
    int fd = -1; // FILE, MAP (the fd the mapping was created over)
    DIR* dir = nullptr; // FIND
    std::string findDir, findPat; // FIND: directory + glob pattern
    pthread_t thread{}; // THREAD
    bool joined = false;
    pthread_mutex_t em = PTHREAD_MUTEX_INITIALIZER; // EVENT
    pthread_cond_t ec = PTHREAD_COND_INITIALIZER;
    bool esignaled = false, emanual = false;
    int ewaiters =
        0; // EVENT: threads currently inside WaitForSingleObject (for a safe close)
    int mapProt =
        0; // MAP: PROT_* implied by the protection CreateFileMapping was given
};

static thread_local DWORD t_lastError = 0;
extern "C" DWORD GetLastError(void)
{
    return t_lastError;
}
extern "C" void SetLastError(DWORD e)
{
    t_lastError = e;
}
static inline HANDLE hwrap(Win32Handle* h)
{
    return reinterpret_cast<HANDLE>(h);
}
static inline Win32Handle* hunwrap(HANDLE h)
{
    return reinterpret_cast<Win32Handle*>(h);
}

// ---------------------------------------------------------------- files ----
extern "C" int FF_CIResolvePath(const char* want, char* out,
                                unsigned long cap); // defined at end of file

// #104: resolve a Windows-style path for a POSIX WRITE/CREATE/STAT/mkdir. The engine spells every path with '\'
// and in mixed case (e.g. FalconDataDirectory + "\\Campaign\\Save"), while the shipped data tree lives lowercase
// on case-sensitive ext4. A bare ::mkdir/::open/::stat on that string would create/miss a single oddly-named
// entry with literal backslashes instead of descending the tree. FF_CIResolvePath normalises '\'->'/' and
// case-folds the components that already exist, keeping any not-yet-existing tail exactly as spelled -- so a new
// file/dir lands where asked, under the correctly-cased parents. Falls back to the raw name if the buffer is too
// small to hold the result (only then is `out` left untouched). Reads use the verbatim-first path below instead.
static const char* ci_for_write(const char* name, char* buf, size_t cap)
{
    if (!name)
        return name;
    buf[0] = '\0';
    FF_CIResolvePath(name, buf, (unsigned long)cap);
    return buf[0] ? buf : name;
}

extern "C" HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD /*share*/,
                              void* /*sec*/, DWORD disp, DWORD /*flags*/,
                              HANDLE /*tmpl*/)
{
    int oflag = 0;
    const bool rd = (access & GENERIC_READ) != 0,
               wr = (access & GENERIC_WRITE) != 0;
    if (rd && wr)
        oflag = O_RDWR;
    else if (wr)
        oflag = O_WRONLY;
    else
        oflag = O_RDONLY;
    switch (disp)
    {
    case CREATE_NEW:
        oflag |= O_CREAT | O_EXCL;
        break;
    case CREATE_ALWAYS:
        oflag |= O_CREAT | O_TRUNC;
        break;
    case OPEN_EXISTING:
        break;
    case OPEN_ALWAYS:
        oflag |= O_CREAT;
        break;
    case TRUNCATE_EXISTING:
        oflag |= O_TRUNC;
        break;
    }
    // #104 creates: resolve up front so the new file lands under correctly-cased parents with '/'-separators.
    char cw[4096];
    int fd =
        ::open((oflag & O_CREAT) ? ci_for_write(name, cw, sizeof(cw)) : name,
               oflag, 0644);
    // #104: on a miss for a pure read, case-insensitively resolve against the (lowercase) data tree and retry.
    if (fd < 0 && name && !(oflag & O_CREAT))
    {
        char ci[4096];
        if (FF_CIResolvePath(name, ci, sizeof(ci)))
            fd = ::open(ci, oflag, 0644);
    }
    if (fd < 0)
    {
        t_lastError = (DWORD)errno;
        return INVALID_HANDLE_VALUE;
    }
    Win32Handle* h = new Win32Handle();
    h->kind = Win32Handle::FILE_K;
    h->fd = fd;
    return hwrap(h);
}
extern "C" BOOL ReadFile(HANDLE h, LPVOID buf, DWORD n, LPDWORD read, void*)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return FALSE;
    ssize_t r = ::read(o->fd, buf, n);
    if (r < 0)
    {
        t_lastError = (DWORD)errno;
        return FALSE;
    }
    if (read)
        *read = (DWORD)r;
    return TRUE;
}
extern "C" BOOL WriteFile(HANDLE h, LPCVOID buf, DWORD n, LPDWORD wrote, void*)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return FALSE;
    ssize_t r = ::write(o->fd, buf, n);
    if (r < 0)
    {
        t_lastError = (DWORD)errno;
        return FALSE;
    }
    if (wrote)
        *wrote = (DWORD)r;
    return TRUE;
}
extern "C" DWORD SetFilePointer(HANDLE h, LONG lo, LONG* hi, DWORD method)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return INVALID_FILE_SIZE;
    int whence = method == FILE_BEGIN ? SEEK_SET :
                 method == FILE_END   ? SEEK_END :
                                        SEEK_CUR;
    int64_t off = hi ? ((int64_t)(*hi) << 32) | (uint32_t)lo : (int64_t)lo;
    off_t r = ::lseek(o->fd, (off_t)off, whence);
    if (r == (off_t)-1)
    {
        t_lastError = (DWORD)errno;
        return INVALID_FILE_SIZE;
    }
    if (hi)
        *hi = (LONG)((uint64_t)r >> 32);
    return (DWORD)((uint64_t)r & 0xFFFFFFFFu);
}
extern "C" DWORD GetFileSize(HANDLE h, LPDWORD hi)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return INVALID_FILE_SIZE;
    struct stat st;
    if (::fstat(o->fd, &st) != 0)
    {
        t_lastError = (DWORD)errno;
        return INVALID_FILE_SIZE;
    }
    if (hi)
        *hi = (DWORD)((uint64_t)st.st_size >> 32);
    return (DWORD)((uint64_t)st.st_size & 0xFFFFFFFFu);
}
extern "C" BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* size)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return FALSE;
    struct stat st;
    if (::fstat(o->fd, &st) != 0)
    {
        t_lastError = (DWORD)errno;
        return FALSE;
    }
    if (size)
        size->QuadPart = (LONGLONG)st.st_size;
    return TRUE;
}
extern "C" BOOL FlushFileBuffers(HANDLE h)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return FALSE;
    return ::fsync(o->fd) == 0 ? TRUE : FALSE;
}
extern "C" BOOL SetEndOfFile(HANDLE h)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FILE_K)
        return FALSE;
    off_t p = ::lseek(o->fd, 0, SEEK_CUR);
    return ::ftruncate(o->fd, p) == 0 ? TRUE : FALSE;
}

extern "C" DWORD GetFileAttributesA(LPCSTR name)
{
    char cw[4096];
    name = ci_for_write(name, cw, sizeof(cw));
    struct stat st;
    if (::stat(name, &st) != 0)
    {
        t_lastError = (DWORD)errno;
        return INVALID_FILE_ATTRIBUTES;
    }
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY :
                                 FILE_ATTRIBUTE_NORMAL;
}
extern "C" BOOL DeleteFileA(LPCSTR name)
{
    char cw[4096];
    return ::unlink(ci_for_write(name, cw, sizeof(cw))) == 0 ? TRUE : FALSE;
}
extern "C" BOOL MoveFileA(LPCSTR a, LPCSTR b)
{
    char ca[4096], cb[4096];
    return ::rename(ci_for_write(a, ca, sizeof(ca)),
                    ci_for_write(b, cb, sizeof(cb))) == 0 ?
               TRUE :
               FALSE;
}
extern "C" BOOL CreateDirectoryA(LPCSTR p, void*)
{
    char cw[4096];
    return ::mkdir(ci_for_write(p, cw, sizeof(cw)), 0755) == 0 ? TRUE : FALSE;
}
extern "C" BOOL RemoveDirectoryA(LPCSTR p)
{
    char cw[4096];
    return ::rmdir(ci_for_write(p, cw, sizeof(cw))) == 0 ? TRUE : FALSE;
}
extern "C" BOOL CopyFileA(LPCSTR from, LPCSTR to, BOOL failIfExists)
{
    char cf[4096], ct[4096];
    from = ci_for_write(from, cf, sizeof(cf));
    to = ci_for_write(to, ct, sizeof(ct));
    int in = ::open(from, O_RDONLY);
    if (in < 0)
        return FALSE;
    int out = ::open(to, O_WRONLY | O_CREAT | (failIfExists ? O_EXCL : O_TRUNC),
                     0644);
    if (out < 0)
    {
        ::close(in);
        return FALSE;
    }
    char buf[65536];
    ssize_t r;
    BOOL ok = TRUE;
    while ((r = ::read(in, buf, sizeof buf)) > 0)
        if (::write(out, buf, (size_t)r) != r)
        {
            ok = FALSE;
            break;
        }
    ::close(in);
    ::close(out);
    return ok;
}
extern "C" DWORD GetCurrentDirectoryA(DWORD len, LPSTR buf)
{
    return ::getcwd(buf, len) ? (DWORD)strlen(buf) : 0;
}
extern "C" BOOL SetCurrentDirectoryA(LPCSTR p)
{
    char cw[4096];
    return ::chdir(ci_for_write(p, cw, sizeof(cw))) == 0 ? TRUE : FALSE;
}
extern "C" DWORD GetModuleFileNameA(HMODULE, LPSTR buf, DWORD size)
{
    ssize_t n = ::readlink("/proc/self/exe", buf, size - 1);
    if (n < 0)
    {
        buf[0] = 0;
        return 0;
    }
    buf[n] = 0;
    return (DWORD)n;
}

// GetFullPathNameA: make a path absolute and lexically clean. realpath() is the POSIX counterpart, but it cannot be
// called bare here: it FAILS on a path that does not exist yet, while Windows happily normalises one (bspbuild.cpp:258
// hands it an argv path, aiinput.cpp:173 a data file), and it resolves symlinks, which Windows does not do. So the
// join+normalise is done textually below -- which is precisely what Windows documents this call to do.
// Contract kept as on Win32: returns length written, or the required size if the buffer is too small, or 0 on error.
extern "C" DWORD GetFullPathNameA(LPCSTR name, DWORD len, LPSTR buf,
                                  LPSTR* filePart)
{
    if (not name or not buf)
    {
        t_lastError = 87 /*ERROR_INVALID_PARAMETER*/;
        return 0;
    }
    std::string p;
    if (name[0] == '/')
        p = name;
    else
    {
        char cwd[4096];
        if (not ::getcwd(cwd, sizeof(cwd)))
        {
            t_lastError = (DWORD)errno;
            return 0;
        }
        p = std::string(cwd) + "/" + name;
    }
    std::string out;
    size_t i = 0;
    while (i < p.size())
    {
        size_t j = p.find('/', i);
        if (j == std::string::npos)
            j = p.size();
        std::string seg = p.substr(i, j - i);
        if (seg == "..")
        {
            size_t k = out.rfind('/');
            if (k != std::string::npos)
                out.erase(k);
        }
        else if (not seg.empty() and seg != ".")
        {
            out += "/";
            out += seg;
        }
        i = j + 1;
    }
    if (out.empty())
        out = "/";
    if (out.size() + 1 > len)
        return (DWORD)(out.size() +
                       1); // Win32: needed size; buffer left untouched
    ::memcpy(buf, out.c_str(), out.size() + 1);
    if (filePart)
    {
        char* s = ::strrchr(buf, '/');
        *filePart = (s and s[1]) ? s + 1 : nullptr;
    }
    return (DWORD)out.size();
}

// GetComputerNameA -> gethostname(): the same fact under another name, the machine's own name.
extern "C" BOOL GetComputerNameA(LPSTR buf, LPDWORD size)
{
    if (not buf or not size or *size == 0)
    {
        t_lastError = 87 /*ERROR_INVALID_PARAMETER*/;
        return FALSE;
    }
    if (::gethostname(buf, *size) != 0)
    {
        t_lastError = (DWORD)errno;
        return FALSE;
    }
    buf[*size - 1] =
        0; // gethostname need not terminate on truncation; Windows always does
    *size =
        (DWORD)::strlen(buf); // Win32 reports back the length actually written
    return TRUE;
}

// ------------------------------------------------- memory-mapped files ----
// Real mmap, not a stand-in: this is how the ACMI tape is read (acmitape.cpp:2434-2479) and how filemap.cpp serves
// bulk data, so a stub returning NULL would simply break playback. Win32 splits the job in two -- a mapping OBJECT
// (CreateFileMapping) and VIEWS onto it (MapViewOfFile) -- while POSIX has only mmap. The object is therefore a
// bookkeeping handle here, holding the fd and the protection until a view asks to be mapped.
//
// One genuine gap has to be bridged: UnmapViewOfFile takes only an address, because Windows knows each view's size;
// munmap demands the length. So view sizes are remembered here. The map is guarded -- CreateFileMapping is called
// from loader threads.
static std::map<void*, size_t> g_views;
static pthread_mutex_t g_viewsMtx = PTHREAD_MUTEX_INITIALIZER;

extern "C" HANDLE CreateFileMappingA(HANDLE hFile, void* /*sec*/, DWORD protect,
                                     DWORD /*maxHigh*/, DWORD /*maxLow*/,
                                     LPCSTR /*name*/)
{
    // Named mappings (the `name` argument) are a cross-process facility with no one-line POSIX equal; nothing in the
    // codebase passes a name (both call sites pass NULL), so refusing here would be inventing a problem. If one ever
    // does, it needs shm_open and a deliberate design -- not a silent reinterpretation as a private mapping.
    Win32Handle* f = hunwrap(hFile);
    if (!f || hFile == INVALID_HANDLE_VALUE || f->kind != Win32Handle::FILE_K)
    {
        t_lastError = 6 /*ERROR_INVALID_HANDLE*/;
        return nullptr; // Win32 returns NULL here, not INVALID_HANDLE_VALUE
    }
    int prot = PROT_READ;
    if (protect & 0x04u /*PAGE_READWRITE*/)
        prot = PROT_READ | PROT_WRITE;
    else if (protect & 0x08u /*PAGE_WRITECOPY*/)
        prot = PROT_READ | PROT_WRITE; // paired with MAP_PRIVATE below
    else if (protect & 0x01u /*PAGE_NOACCESS*/)
        prot = PROT_NONE;
    Win32Handle* o = new Win32Handle();
    o->kind = Win32Handle::MAP_K;
    o->fd =
        f->fd; // borrowed: the mapping object does not own the file, and does not close it
    o->mapProt = prot;
    return hwrap(o);
}
#define FF_MAP_WRITECOPY_PROT (PROT_READ | PROT_WRITE)

extern "C" LPVOID MapViewOfFileEx(HANDLE hMap, DWORD access, DWORD offHigh,
                                  DWORD offLow, size_t bytes, LPVOID base)
{
    Win32Handle* o = hunwrap(hMap);
    if (!o || o->kind != Win32Handle::MAP_K)
    {
        t_lastError = 6 /*ERROR_INVALID_HANDLE*/;
        return nullptr;
    }
    off_t off = ((off_t)offHigh << 32) | (off_t)offLow;
    // bytes == 0 means "to the end of the file" on Windows; mmap needs a real length, so ask the file.
    if (bytes == 0)
    {
        struct stat st;
        if (::fstat(o->fd, &st) != 0)
        {
            t_lastError = (DWORD)errno;
            return nullptr;
        }
        if (st.st_size <= off)
        {
            t_lastError = 87 /*ERROR_INVALID_PARAMETER*/;
            return nullptr;
        }
        bytes = (size_t)(st.st_size - off);
    }
    int prot = o->mapProt;
    if (access & 0x0004u /*FILE_MAP_READ*/)
        prot = PROT_READ;
    if (access & 0x0002u /*FILE_MAP_WRITE*/)
        prot = PROT_READ | PROT_WRITE;
    // FILE_MAP_COPY is copy-on-write: writes stay private to this process. MAP_PRIVATE is exactly that.
    const bool copyOnWrite = (access & 0x0001u /*FILE_MAP_COPY*/) != 0;
    void* p = ::mmap(base, bytes, prot, copyOnWrite ? MAP_PRIVATE : MAP_SHARED,
                     o->fd, off);
    if (p == MAP_FAILED)
    {
        t_lastError = (DWORD)errno;
        return nullptr;
    }
    pthread_mutex_lock(&g_viewsMtx);
    g_views[p] = bytes;
    pthread_mutex_unlock(&g_viewsMtx);
    return p;
}
extern "C" LPVOID MapViewOfFile(HANDLE hMap, DWORD access, DWORD offHigh,
                                DWORD offLow, size_t bytes)
{
    return MapViewOfFileEx(hMap, access, offHigh, offLow, bytes, nullptr);
}

extern "C" BOOL UnmapViewOfFile(LPCVOID addr)
{
    if (!addr)
        return FALSE;
    pthread_mutex_lock(&g_viewsMtx);
    auto it = g_views.find(const_cast<void*>(addr));
    size_t n = (it == g_views.end()) ? 0 : it->second;
    if (it != g_views.end())
        g_views.erase(it);
    pthread_mutex_unlock(&g_viewsMtx);
    // An address we never handed out is a caller bug, and munmap cannot be guessed at -- report it rather than
    // unmapping something arbitrary.
    if (n == 0)
    {
        t_lastError = 87 /*ERROR_INVALID_PARAMETER*/;
        return FALSE;
    }
    if (::munmap(const_cast<void*>(addr), n) != 0)
    {
        t_lastError = (DWORD)errno;
        return FALSE;
    }
    return TRUE;
}
extern "C" BOOL FlushViewOfFile(LPCVOID addr, size_t bytes)
{
    if (!addr)
        return FALSE;
    if (bytes == 0)
    { // Windows: 0 means "the whole view from addr"
        pthread_mutex_lock(&g_viewsMtx);
        auto it = g_views.find(const_cast<void*>(addr));
        if (it != g_views.end())
            bytes = it->second;
        pthread_mutex_unlock(&g_viewsMtx);
    }
    if (::msync(const_cast<void*>(addr), bytes, MS_SYNC) != 0)
    {
        t_lastError = (DWORD)errno;
        return FALSE;
    }
    return TRUE;
}

// ------------------------------------------------------- error messages ----
// LocalAlloc/LocalFree: plain malloc/free. The Win32 local heap is a distinct allocator only for historical reasons;
// what the callers actually rely on is "FormatMessage allocated it, LocalFree releases it", and that pairing holds.
extern "C" void* LocalAlloc(UINT flags, size_t bytes)
{
    void* p = ::malloc(bytes);
    if (p and (flags bitand 0x0040 /*LMEM_ZEROINIT*/))
        ::memset(p, 0, bytes);
    return p;
}
extern "C" void* LocalFree(void* mem)
{
    ::free(mem);
    return nullptr;
}

// FormatMessageA over strerror(): a true translation, not a placeholder -- the codes reaching here ARE errno values,
// because this file's own error path stores errno into t_lastError. Both buffer modes are honoured, since callers use
// both: ALLOCATE_BUFFER writes a pointer through `buf` for the caller to LocalFree (cfonts.cpp:166-173), while the
// PutErrorString macro passes a fixed char[] and its sizeof. Returns chars written, as on Win32.
extern "C" DWORD FormatMessageA(DWORD flags, const void* /*src*/, DWORD msgId,
                                DWORD /*langId*/, LPSTR buf, DWORD size,
                                void* /*args*/)
{
    if (not buf)
    {
        t_lastError = 87 /*ERROR_INVALID_PARAMETER*/;
        return 0;
    }
    const char* msg = ::strerror((int)msgId);
    if (not msg)
        msg = "Unknown error";
    size_t n = ::strlen(msg);

    if (flags bitand 0x00000100u /*FORMAT_MESSAGE_ALLOCATE_BUFFER*/)
    {
        // `buf` is not a buffer in this mode -- it is the address of the caller's pointer. Win32 ignores `size` here
        // (it is a MINIMUM, not a cap), so the allocation is sized to the message.
        char* p = (char*)::malloc(n + 1);
        if (not p)
        {
            t_lastError = 8 /*ERROR_NOT_ENOUGH_MEMORY*/;
            return 0;
        }
        ::memcpy(p, msg, n + 1);
        *(char**)buf = p;
        return (DWORD)n;
    }
    if (size == 0)
        return 0;
    size_t copy =
        (n < size - 1) ? n : size - 1; // Win32 truncates rather than overruns
    ::memcpy(buf, msg, copy);
    buf[copy] = 0;
    return (DWORD)copy;
}

// ---------------------------------------------------------------- find ----
static void fill_find(Win32Handle* o, struct dirent* de,
                      LPWIN32_FIND_DATAA data)
{
    memset(data, 0, sizeof *data);
    std::string full = o->findDir + "/" + de->d_name;
    struct stat st;
    DWORD attr = FILE_ATTRIBUTE_NORMAL;
    if (::stat(full.c_str(), &st) == 0)
    {
        attr = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY :
                                     FILE_ATTRIBUTE_NORMAL;
        data->nFileSizeLow = (DWORD)((uint64_t)st.st_size & 0xFFFFFFFFu);
        data->nFileSizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
    }
    data->dwFileAttributes = attr;
    strncpy(data->cFileName, de->d_name, MAX_PATH - 1);
}
extern "C" HANDLE FindFirstFileA(LPCSTR pattern, LPWIN32_FIND_DATAA data)
{
    std::string pat(pattern);
    size_t slash = pat.find_last_of("/\\");
    std::string dir = slash == std::string::npos ? "." : pat.substr(0, slash);
    std::string glob = slash == std::string::npos ? pat : pat.substr(slash + 1);
    for (char& c : dir)
        if (c == '\\')
            c = '/';
    DIR* d = ::opendir(dir.c_str());
    if (!d)
    {
        t_lastError = (DWORD)errno;
        return INVALID_HANDLE_VALUE;
    }
    Win32Handle* o = new Win32Handle();
    o->kind = Win32Handle::FIND_K;
    o->dir = d;
    o->findDir = dir;
    o->findPat = glob;
    struct dirent* de;
    while ((de = ::readdir(d)) != nullptr)
        if (::fnmatch(o->findPat.c_str(), de->d_name, FNM_CASEFOLD) == 0)
        {
            fill_find(o, de, data);
            return hwrap(o);
        }
    ::closedir(d);
    delete o;
    t_lastError = 2 /*ERROR_FILE_NOT_FOUND*/;
    return INVALID_HANDLE_VALUE;
}
extern "C" BOOL FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA data)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FIND_K)
        return FALSE;
    struct dirent* de;
    while ((de = ::readdir(o->dir)) != nullptr)
        if (::fnmatch(o->findPat.c_str(), de->d_name, FNM_CASEFOLD) == 0)
        {
            fill_find(o, de, data);
            return TRUE;
        }
    return FALSE;
}
extern "C" BOOL FindClose(HANDLE h)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::FIND_K)
        return FALSE;
    if (o->dir)
        ::closedir(o->dir);
    delete o;
    return TRUE;
}

// ---------------------------------------------------------------- threads / events ----
struct ThreadStart
{
    LPTHREAD_START_ROUTINE fn;
    LPVOID arg;
};
static void* thread_trampoline(void* p)
{
    ThreadStart ts = *reinterpret_cast<ThreadStart*>(p);
    delete reinterpret_cast<ThreadStart*>(p);
    return reinterpret_cast<void*>((uintptr_t)ts.fn(ts.arg));
}

extern "C" HANDLE CreateThread(void*, SIZE_T, LPTHREAD_START_ROUTINE start,
                               LPVOID param, DWORD, DWORD* tid)
{
    Win32Handle* o = new Win32Handle();
    o->kind = Win32Handle::THREAD_K;
    ThreadStart* ts = new ThreadStart{start, param};
    if (::pthread_create(&o->thread, nullptr, thread_trampoline, ts) != 0)
    {
        delete ts;
        delete o;
        return nullptr;
    }
    if (tid)
        *tid = 0;
    return hwrap(o);
}
extern "C" HANDLE CreateEventA(void*, BOOL manualReset, BOOL initialState,
                               LPCSTR)
{
    Win32Handle* o = new Win32Handle();
    o->kind = Win32Handle::EVENT_K;
    o->emanual = manualReset != 0;
    o->esignaled = initialState != 0;
    return hwrap(o);
}
extern "C" BOOL SetEvent(HANDLE h)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::EVENT_K)
        return FALSE;
    pthread_mutex_lock(&o->em);
    o->esignaled = true;
    pthread_cond_broadcast(&o->ec);
    pthread_mutex_unlock(&o->em);
    return TRUE;
}
extern "C" BOOL ResetEvent(HANDLE h)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::EVENT_K)
        return FALSE;
    pthread_mutex_lock(&o->em);
    o->esignaled = false;
    pthread_mutex_unlock(&o->em);
    return TRUE;
}
// Mutex over the same auto-reset-event machinery: a mutex is an auto-reset event that starts signaled (available)
// unless the caller asks to own it. WaitForSingleObject on an auto-reset (emanual=false) event acquires it and
// clears the signal; ReleaseMutex re-signals so the next waiter proceeds. Sufficient for the resource manager's
// coarse file-handle lock (recursion is not modelled -- the engine does not recursively take these).
extern "C" HANDLE CreateMutexA(void*, BOOL initialOwner, LPCSTR)
{
    Win32Handle* o = new Win32Handle();
    o->kind = Win32Handle::EVENT_K;
    o->emanual = false;
    o->esignaled = (initialOwner == 0);
    return hwrap(o);
}
extern "C" BOOL ReleaseMutex(HANDLE h)
{
    return SetEvent(h);
}
extern "C" DWORD WaitForSingleObject(HANDLE h, DWORD ms)
{
    Win32Handle* o = hunwrap(h);
    if (!o)
        return WAIT_FAILED;
    if (o->kind == Win32Handle::THREAD_K)
    {
        if (!o->joined)
        {
            ::pthread_join(o->thread, nullptr);
            o->joined = true;
        }
        return WAIT_OBJECT_0;
    }
    if (o->kind == Win32Handle::EVENT_K)
    {
        pthread_mutex_lock(&o->em);
        o->ewaiters++; // announce presence so CloseHandle can wait us out before destroying
        DWORD rv = WAIT_OBJECT_0;
        if (ms == INFINITE)
        {
            while (!o->esignaled)
                pthread_cond_wait(&o->ec, &o->em);
        }
        else
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += ms / 1000;
            ts.tv_nsec += (long)(ms % 1000) * 1000000L;
            if (ts.tv_nsec >= 1000000000L)
            {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
            }
            while (!o->esignaled)
                if (pthread_cond_timedwait(&o->ec, &o->em, &ts) != 0)
                {
                    rv = WAIT_TIMEOUT;
                    break;
                }
        }
        if (rv == WAIT_OBJECT_0 && !o->emanual)
            o->esignaled = false; // auto-reset
        o->ewaiters--;
        pthread_mutex_unlock(&o->em);
        return rv;
    }
    return WAIT_FAILED;
}
extern "C" DWORD WaitForMultipleObjects(DWORD count, const HANDLE* h,
                                        BOOL waitAll, DWORD ms)
{ // minimal: sequential wait-all, or first-signalled poll. Sufficient for the join/event use in the codebase.
    if (waitAll)
    {
        for (DWORD i = 0; i < count; ++i)
            WaitForSingleObject(h[i], ms);
        return WAIT_OBJECT_0;
    }
    for (DWORD i = 0; i < count; ++i)
        if (WaitForSingleObject(h[i], 0) == WAIT_OBJECT_0)
            return WAIT_OBJECT_0 + i;
    return WAIT_TIMEOUT;
}
extern "C" BOOL CloseHandle(HANDLE h)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o == (Win32Handle*)INVALID_HANDLE_VALUE)
        return FALSE;
    switch (o->kind)
    {
    case Win32Handle::FILE_K:
        if (o->fd >= 0)
            ::close(o->fd);
        break;
    case Win32Handle::FIND_K:
        if (o->dir)
            ::closedir(o->dir);
        break;
    case Win32Handle::THREAD_K:
        if (!o->joined)
            ::pthread_detach(o->thread);
        break;
    case Win32Handle::EVENT_K:
    {
        // A thread may still be blocked in WaitForSingleObject (pthread_cond_wait) on this event -- at exit
        // StopSim closes sim events that StartLoop/campaign_wait_for_sim are still waiting on. Destroying the
        // cond/mutex (or `delete o` below) while a thread waits hangs glibc's pthread_cond_destroy and is a
        // use-after-free. Force the event permanently signalled and wake everyone so each waiter's
        // `while(!esignaled)` exits, then wait until they have all left before destroying.
        pthread_mutex_lock(&o->em);
        o->esignaled = true;
        o->emanual = true;
        pthread_cond_broadcast(&o->ec);
        pthread_mutex_unlock(&o->em);
        for (;;)
        {
            pthread_mutex_lock(&o->em);
            int w = o->ewaiters;
            pthread_mutex_unlock(&o->em);
            if (w == 0)
                break;
            struct timespec ns = {0, 100000};
            nanosleep(&ns, nullptr); // 100us
        }
        pthread_cond_destroy(&o->ec);
        pthread_mutex_destroy(&o->em);
        break;
    }
    // MAP_K owns no fd of its own (it borrows the file's) and no address: on Windows closing the mapping handle
    // does not unmap the views, and existing views stay valid. munmap belongs to UnmapViewOfFile, as it does there.
    case Win32Handle::MAP_K:
        break;
    }
    delete o;
    return TRUE;
}
extern "C" DWORD GetCurrentThreadId(void)
{
    return (DWORD)(uintptr_t)::pthread_self();
}
extern "C" DWORD GetCurrentProcessId(void)
{
    return (DWORD)::getpid();
}
extern "C" HANDLE GetCurrentThread(void)
{
    return (HANDLE)-2;
}
// ---------------------------------------------------------------- ANSI <-> wide (UTF-8 <-> UTF-16) ----
// Artscout - 2026 (Linux port): faithful codec for MultiByteToWideChar/WideCharToMultiByte. WCHAR is
// 16-bit (UTF-16); the narrow side is UTF-8 (the Linux locale). Surrogate pairs are handled, matching
// the Windows originals' code-unit counting. cbSrc/cchSrc < 0 means the input is null-terminated (and
// the terminator is converted too). With a zero-size / null destination, the required length is returned.
extern "C" int MultiByteToWideChar(UINT, DWORD, LPCSTR src, int cbSrc,
                                   LPWSTR dst, int cchDst)
{
    if (!src)
        return 0;
    size_t srcLen = (cbSrc < 0) ? ::strlen(src) + 1 : (size_t)cbSrc;
    bool measure = (dst == nullptr || cchDst == 0);
    int out = 0;
    for (size_t i = 0; i < srcLen;)
    {
        unsigned char c = (unsigned char)src[i];
        unsigned int cp;
        int adv;
        if (c < 0x80)
        {
            cp = c;
            adv = 1;
        }
        else if ((c >> 5) == 0x6 && i + 1 < srcLen)
        {
            cp = ((c & 0x1F) << 6) | (src[i + 1] & 0x3F);
            adv = 2;
        }
        else if ((c >> 4) == 0xE && i + 2 < srcLen)
        {
            cp = ((c & 0x0F) << 12) | ((src[i + 1] & 0x3F) << 6) |
                 (src[i + 2] & 0x3F);
            adv = 3;
        }
        else if ((c >> 3) == 0x1E && i + 3 < srcLen)
        {
            cp = ((c & 0x07) << 18) | ((src[i + 1] & 0x3F) << 12) |
                 ((src[i + 2] & 0x3F) << 6) | (src[i + 3] & 0x3F);
            adv = 4;
        }
        else
        {
            cp = c;
            adv = 1;
        } // invalid -> Latin-1
        i += (size_t)adv;
        if (cp <= 0xFFFF)
        {
            if (!measure)
            {
                if (out >= cchDst)
                    return 0;
                dst[out] = (WCHAR)cp;
            }
            out += 1;
        }
        else
        {
            cp -= 0x10000;
            WCHAR hi = (WCHAR)(0xD800 + (cp >> 10)),
                  lo = (WCHAR)(0xDC00 + (cp & 0x3FF));
            if (!measure)
            {
                if (out + 1 >= cchDst)
                    return 0;
                dst[out] = hi;
                dst[out + 1] = lo;
            }
            out += 2;
        }
    }
    return out;
}
extern "C" int WideCharToMultiByte(UINT, DWORD, LPCWSTR src, int cchSrc,
                                   LPSTR dst, int cbDst, LPCSTR,
                                   LPBOOL usedDefault)
{
    if (usedDefault)
        *usedDefault = 0;
    if (!src)
        return 0;
    size_t srcLen = 0;
    if (cchSrc < 0)
    {
        while (src[srcLen])
            ++srcLen;
        ++srcLen;
    } // include terminator
    else
        srcLen = (size_t)cchSrc;
    bool measure = (dst == nullptr || cbDst == 0);
    int out = 0;
    for (size_t i = 0; i < srcLen;)
    {
        unsigned int cp = (unsigned int)src[i++];
        if (cp >= 0xD800 && cp <= 0xDBFF && i < srcLen)
        { // high surrogate
            unsigned int lo = (unsigned int)src[i];
            if (lo >= 0xDC00 && lo <= 0xDFFF)
            {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                ++i;
            }
        }
        char buf[4];
        int n;
        if (cp < 0x80)
        {
            buf[0] = (char)cp;
            n = 1;
        }
        else if (cp < 0x800)
        {
            buf[0] = (char)(0xC0 | (cp >> 6));
            buf[1] = (char)(0x80 | (cp & 0x3F));
            n = 2;
        }
        else if (cp < 0x10000)
        {
            buf[0] = (char)(0xE0 | (cp >> 12));
            buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
            buf[2] = (char)(0x80 | (cp & 0x3F));
            n = 3;
        }
        else
        {
            buf[0] = (char)(0xF0 | (cp >> 18));
            buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
            buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
            buf[3] = (char)(0x80 | (cp & 0x3F));
            n = 4;
        }
        if (!measure)
        {
            if (out + n > cbDst)
                return 0;
            for (int k = 0; k < n; ++k)
                dst[out + k] = buf[k];
        }
        out += n;
    }
    return out;
}

extern "C" BOOL SetThreadPriority(HANDLE, int)
{
    return TRUE;
}
// Real thread affinity: the HANDLE wraps a pthread_t, so pin it with pthread_setaffinity_np.
// Win32 returns the previous affinity mask (0 == failure); we don't track the prior mask, so we
// return the applied mask as a non-zero success token, which is all callers here test.
extern "C" DWORD_PTR SetThreadAffinityMask(HANDLE h, DWORD_PTR mask)
{
    Win32Handle* o = hunwrap(h);
    if (!o || o->kind != Win32Handle::THREAD_K || mask == 0)
        return 0;
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int i = 0; i < (int)(sizeof(DWORD_PTR) * 8); ++i)
        if (mask & ((DWORD_PTR)1 << i))
            CPU_SET(i, &set);
    if (::pthread_setaffinity_np(o->thread, sizeof(set), &set) != 0)
        return 0;
    return mask;
}

// ---------------------------------------------------------------- _findfirst family (CRT <io.h>) ----
struct FindState
{
    DIR* dir;
    std::string dirPath, pat;
};
static void fill_finddata(const std::string& dirPath, struct dirent* de,
                          struct _finddata_t* data)
{
    memset(data, 0, sizeof *data);
    std::string full = dirPath + "/" + de->d_name;
    unsigned attr = _A_NORMAL;
    struct stat st;
    if (::stat(full.c_str(), &st) == 0)
    {
        attr = S_ISDIR(st.st_mode) ? _A_SUBDIR : _A_NORMAL;
        data->size = (_fsize_t)st.st_size;
        data->time_write = (long)st.st_mtime;
    }
    data->attrib = attr;
    strncpy(data->name, de->d_name, sizeof(data->name) - 1);
}
extern "C" intptr_t _findfirst(const char* spec, struct _finddata_t* data)
{
    std::string s(spec);
    size_t slash = s.find_last_of("/\\");
    std::string dir = slash == std::string::npos ? "." : s.substr(0, slash);
    std::string glob = slash == std::string::npos ? s : s.substr(slash + 1);
    for (char& c : dir)
        if (c == '\\')
            c = '/';
    DIR* d = ::opendir(dir.c_str());
    if (!d)
    {
        t_lastError = (DWORD)errno;
        return -1;
    }
    FindState* fs = new FindState{d, dir, glob};
    struct dirent* de;
    while ((de = ::readdir(d)) != nullptr)
        if (::fnmatch(fs->pat.c_str(), de->d_name, FNM_CASEFOLD) == 0)
        {
            fill_finddata(fs->dirPath, de, data);
            return (intptr_t)fs;
        }
    ::closedir(d);
    delete fs;
    return -1;
}
extern "C" int _findnext(intptr_t handle, struct _finddata_t* data)
{
    FindState* fs = (FindState*)handle;
    if (!fs)
        return -1;
    struct dirent* de;
    while ((de = ::readdir(fs->dir)) != nullptr)
        if (::fnmatch(fs->pat.c_str(), de->d_name, FNM_CASEFOLD) == 0)
        {
            fill_finddata(fs->dirPath, de, data);
            return 0;
        }
    return -1;
}
extern "C" int _findclose(intptr_t handle)
{
    FindState* fs = (FindState*)handle;
    if (!fs)
        return -1;
    if (fs->dir)
        ::closedir(fs->dir);
    delete fs;
    return 0;
}

// ---------------------------------------------------------------- .ini profile API ----
// Minimal INI reader: [section] then key=value lines. Enough for the leftover .ini reads (most config is XML now).
static bool ini_find(const char* file, const char* sect, const char* key,
                     std::string& out)
{
    FILE* f = ::fopen(file, "r");
    if (!f)
        return false;
    char line[1024];
    bool inSect = (sect == nullptr);
    bool found = false;
    while (::fgets(line, sizeof line, f))
    {
        char* p = line;
        while (*p == ' ' || *p == '\t')
            ++p;
        if (*p == ';' || *p == '#' || *p == '\n' || *p == '\r' || *p == 0)
            continue;
        if (*p == '[')
        {
            char* e = strchr(p, ']');
            if (e)
            {
                *e = 0;
                inSect = sect && strcasecmp(p + 1, sect) == 0;
            }
            continue;
        }
        if (!inSect)
            continue;
        char* eq = strchr(p, '=');
        if (!eq)
            continue;
        *eq = 0;
        char* k = p;
        char* kv_end = k + strlen(k);
        while (kv_end > k && (kv_end[-1] == ' ' || kv_end[-1] == '\t'))
            *--kv_end = 0;
        if (strcasecmp(k, key) == 0)
        {
            char* v = eq + 1;
            while (*v == ' ' || *v == '\t')
                ++v;
            char* ve = v + strlen(v);
            while (ve > v && (ve[-1] == '\n' || ve[-1] == '\r' ||
                              ve[-1] == ' ' || ve[-1] == '\t'))
                *--ve = 0;
            out = v;
            found = true;
            break;
        }
    }
    ::fclose(f);
    return found;
}
extern "C" DWORD GetPrivateProfileIntA(LPCSTR sect, LPCSTR key, int def,
                                       LPCSTR file)
{
    std::string v;
    return ini_find(file, sect, key, v) ? (DWORD)atoi(v.c_str()) : (DWORD)def;
}
extern "C" DWORD GetPrivateProfileStringA(LPCSTR sect, LPCSTR key, LPCSTR def,
                                          LPSTR buf, DWORD size, LPCSTR file)
{
    std::string v;
    const char* src =
        ini_find(file, sect, key, v) ? v.c_str() : (def ? def : "");
    if (!buf || size == 0)
        return 0;
    strncpy(buf, src, size - 1);
    buf[size - 1] = 0;
    return (DWORD)strlen(buf);
}
extern "C" BOOL WritePrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPCSTR)
{
    return TRUE;
} // writes stubbed (config is XML)

// ---- MessageBox seam (#104) -------------------------------------------------------------------------------------
// Null until the platform layer (ffplatform/SDL3) installs a real dialog. See the note in win32_ui.h: the default
// deliberately reports the message instead of swallowing it, because it also has to ANSWER the prompt, and answering
// invisibly is how a "why did it skip that warning?" bug is born.
int (*ff_MessageBoxHook)(void* hwnd, const char* text, const char* caption,
                         unsigned type) = nullptr;

int ff_MessageBoxDefault(void* /*hwnd*/, const char* text, const char* caption,
                         unsigned /*type*/)
{
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "(no caption)",
            text ? text : "(no text)");
    return 1; // IDOK
}

// ================================================================ GDI text (Phase 3, UI fonts) ====
// Artscout - 2026 (Linux port): honest software GDI for the ui95 font path. There is no GDI and no
// freetype dev headers here, so glyphs are NOT rasterised: TextOut* draws nothing (empty glyph) and
// logs a one-time TODO. Metrics are derived from the requested font height so text layout still has
// consistent, non-zero extents. A small tagged object model backs the DC/font/DIB handles.
#include <cstdlib>
namespace
{
struct GdiObject
{
    enum Kind
    {
        DC_K,
        FONT_K,
        DIB_K
    } kind;
    int fontHeight = 16; // FONT_K: requested cell height (pixels)
    int curFontHeight =
        16; // DC_K:   height of the font currently selected into the DC
    void* bits = nullptr; // DIB_K: pixel memory
    size_t bytes = 0;
};
inline GdiObject* G(void* h)
{
    return reinterpret_cast<GdiObject*>(h);
}
} // namespace

extern "C" HDC CreateCompatibleDC(HDC)
{
    return (HDC) new GdiObject{GdiObject::DC_K};
}
extern "C" BOOL DeleteDC(HDC hdc)
{
    if (hdc)
        delete G(hdc);
    return TRUE;
}

extern "C" HFONT CreateFontA(int height, int, int, int, int, DWORD, DWORD,
                             DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR)
{
    GdiObject* o = new GdiObject{GdiObject::FONT_K};
    o->fontHeight = height < 0 ? -height : (height ? height : 16);
    return (HFONT)o;
}
extern "C" HFONT CreateFontIndirectA(const LOGFONTA* lf)
{
    GdiObject* o = new GdiObject{GdiObject::FONT_K};
    long h = lf ? lf->lfHeight : 16;
    o->fontHeight = (int)(h < 0 ? -h : (h ? h : 16));
    return (HFONT)o;
}
extern "C" HGDIOBJ SelectObject(HDC hdc, void* obj)
{
    if (hdc && obj)
    {
        GdiObject* d = G(hdc);
        GdiObject* o = G(obj);
        if (o->kind == GdiObject::FONT_K)
            d->curFontHeight = o->fontHeight;
    }
    return (HGDIOBJ)
        obj; // echo as the "previously selected" object (callers only restore it)
}
extern "C" BOOL DeleteObject(void* obj)
{
    if (!obj)
        return FALSE;
    // #104 (Linux): GDI object handles are real heap GdiObject* pointers here, but some shim handles are
    // sentinels -- LoadCursorA() returns (HCURSOR)1, and UI_Cleanup() DeleteObject()s the cursor array.
    // Dereferencing 0x1 as a GdiObject crashed. Match Windows (DeleteObject on a stock/system handle just
    // returns FALSE): reject anything that isn't a plausible canonical heap pointer.
    uintptr_t h = (uintptr_t)obj;
    if (h < 0x10000u || h >= 0x800000000000ULL)
        return FALSE;
    GdiObject* o = G(obj);
    if (o->kind == GdiObject::DIB_K && o->bits)
        ::free(o->bits);
    delete o;
    return TRUE;
}
extern "C" HBITMAP CreateDIBSection(HDC, const BITMAPINFO* bmi, UINT,
                                    void** ppvBits, HANDLE, DWORD)
{
    long w = bmi ? bmi->bmiHeader.biWidth : 0;
    long h = bmi ? bmi->bmiHeader.biHeight : 0;
    if (h < 0)
        h = -h;
    int bpp = bmi ? bmi->bmiHeader.biBitCount : 32;
    size_t bytes =
        (size_t)(w < 0 ? 0 : w) * (size_t)h * (size_t)((bpp + 7) / 8);
    if (bytes == 0)
        bytes = 4;
    GdiObject* o = new GdiObject{GdiObject::DIB_K};
    o->bits = ::calloc(1, bytes); // zeroed -> transparent/empty glyph bitmap
    o->bytes = bytes;
    if (ppvBits)
        *ppvBits = o->bits;
    return (HBITMAP)o;
}
extern "C" BOOL TextOutA(HDC, int, int, LPCSTR, int)
{
    static bool warned = false;
    if (!warned)
    {
        warned = true;
        fputs("[gdi/font] TextOut: glyph rasterisation not implemented on "
              "Linux (empty glyph). "
              "TODO: freetype/SDL_ttf backend.\n",
              stderr);
    }
    return TRUE; // the DIB stays zeroed -> blank glyph; layout still advances via the metrics below
}
extern "C" COLORREF SetTextColor(HDC, COLORREF)
{
    return 0;
}
extern "C" COLORREF SetBkColor(HDC, COLORREF)
{
    return 0;
}
extern "C" int SetBkMode(HDC, int)
{
    return 0;
}
extern "C" BOOL GetTextExtentPoint32A(HDC hdc, LPCSTR, int count, LPSIZE size)
{
    if (!size)
        return FALSE;
    int h = hdc ? G(hdc)->curFontHeight : 16;
    int cw = h / 2 > 0 ? h / 2 : 1;
    size->cx = (count < 0 ? 0 : count) * cw;
    size->cy = h;
    return TRUE;
}
extern "C" BOOL GetTextMetricsA(HDC hdc, LPTEXTMETRIC tm)
{
    if (!tm)
        return FALSE;
    ::memset(tm, 0, sizeof(*tm));
    int h = hdc ? G(hdc)->curFontHeight : 16;
    tm->tmHeight = h;
    tm->tmAscent = h * 4 / 5;
    tm->tmDescent = h - tm->tmAscent;
    tm->tmAveCharWidth = h / 2 > 0 ? h / 2 : 1;
    tm->tmMaxCharWidth = h;
    tm->tmWeight = 400 /*FW_NORMAL*/;
    tm->tmFirstChar = 0;
    tm->tmLastChar = 255;
    tm->tmDefaultChar = (unsigned char)'?';
    tm->tmBreakChar = (unsigned char)' ';
    return TRUE;
}
extern "C" BOOL GetCharWidthA(HDC hdc, UINT first, UINT last, int* widths)
{
    if (!widths || last < first)
        return FALSE;
    int cw = hdc ? (G(hdc)->curFontHeight / 2) : 8;
    if (cw <= 0)
        cw = 1;
    for (UINT i = first; i <= last; ++i)
        widths[i - first] = cw;
    return TRUE;
}
extern "C" int GetDeviceCaps(HDC, int index)
{
    return index == 90 /*LOGPIXELSY*/ || index == 88 ? 96 : 0;
}
extern "C" BOOL GdiFlush(void)
{
    return TRUE;
}

// ---- version-info: no PE version resource on Linux; report "absent" so callers use their default ----
extern "C" DWORD GetFileVersionInfoSizeA(LPCSTR, LPDWORD handle)
{
    if (handle)
        *handle = 0;
    return 0;
}
extern "C" BOOL GetFileVersionInfoA(LPCSTR, DWORD, DWORD, LPVOID)
{
    return FALSE;
}
extern "C" BOOL VerQueryValueA(LPCVOID, LPCSTR, LPVOID* out, UINT* len)
{
    if (out)
        *out = nullptr;
    if (len)
        *len = 0;
    return FALSE;
}

// ---- misc window / input / thread helpers ----
extern "C" BOOL GetClientRect(HWND, LPRECT r)
{
    if (r)
    {
        r->left = r->top = r->right = r->bottom = 0;
    }
    return TRUE;
}
extern "C" BOOL GetUpdateRect(HWND, LPRECT r, BOOL)
{
    if (r)
    {
        r->left = r->top = r->right = r->bottom = 0;
    }
    return FALSE;
} // SDL repaints whole frame; no GDI dirty region
// Screen dimensions. TODO: query the actual SDL display; a common default keeps size math sane meanwhile.
extern "C" int GetSystemMetrics(int index)
{
    if (index == 0 /*SM_CXSCREEN*/)
        return 1920;
    if (index == 1 /*SM_CYSCREEN*/)
        return 1080;
    return 0;
}
extern "C" BOOL InvalidateRect(HWND, const RECT*, BOOL)
{
    return TRUE;
} // SDL repaints each frame
extern "C" BOOL ValidateRect(HWND, const RECT*)
{
    return TRUE;
} // no GDI dirty-region model here
extern "C" BOOL ClientToScreen(HWND, LPPOINT)
{
    return TRUE;
} // single full-screen window: client==screen
extern "C" BOOL ScreenToClient(HWND, LPPOINT)
{
    return TRUE;
}
extern "C" SHORT GetKeyState(int)
{
    return 0;
} // live key state comes from the SDL input layer
extern "C" UINT GetDoubleClickTime(void)
{
    return 500;
}
extern "C" LONG GetMessageTime(void)
{
    return (LONG)GetTickCount();
}
// pthreads cannot be suspended cooperatively from outside; report no-op (previous suspend count 0).
// TODO: gate the affected ui95 loader threads on a condition variable instead of SuspendThread.
extern "C" DWORD SuspendThread(HANDLE)
{
    return 0;
}
extern "C" DWORD ResumeThread(HANDLE)
{
    return 0;
}

// Artscout - 2026 (#104): minimal cursor entry points (declared in win32_ui.h). The engine calls these ~50x for
// the mouse cursor (gCursors[] via LoadCursor, CRSR_WAIT flips via SetCursor). Real per-shape cursors belong to
// the SDL3 input port (phase 3, SDL_CreateSystemCursor/SDL_SetCursor); until then these link cleanly and the OS
// window keeps SDL's default cursor. A non-NULL handle is returned so `if (gCursors[i])` guards stay truthy.
// C++ linkage, to match the (non-extern-"C") declarations in win32_ui.h.
HCURSOR LoadCursorA(HINSTANCE /*hInstance*/, LPCSTR /*lpCursorName*/)
{
    return (HCURSOR)1;
}
HCURSOR SetCursor(HCURSOR hCursor)
{
    return hCursor;
} // returns "previous"
// ff_events.cpp: SDL relative mouse mode (hides OS cursor in 3D). WEAK: standalone tools/tests link win32compat
// without the SDL event layer; for them the symbol is null and ShowCursor just keeps its counter semantics.
extern "C" void FF_SetRelativeMouse(int enable) __attribute__((weak));

int ShowCursor(BOOL bShow)
{
    // Win32 semantics: a display counter (cursor visible when >= 0). ShowCursor(TRUE) increments,
    // ShowCursor(FALSE) decrements, returning the NEW count. The UI spins on this to force a state:
    //   ui_main.cpp: `while (ShowCursor(FALSE) >= 0);`  and  `while (ShowCursor(TRUE) < 0);`
    // A fixed return of 0 makes the FALSE loop (0 >= 0) spin forever -- that is the exit-time hang in
    // UI_Cleanup (main thread stuck in ShowCursor on EndUI). Emulate the counter so the loops terminate.
    static int s_count = 0; // Win32 initial state with a mouse present
    s_count += bShow ? 1 : -1;
    // Linux: actually hide/show the OS cursor. The engine hides it (count<0) entering 3D, where its own
    // delta-driven cursor takes over -> SDL relative mode hides the OS cursor AND unbinds it from the window
    // edge (the source of the "two cursors" + "press to the edge" divergence). Visible (count>=0) = 2D menu.
    if (FF_SetRelativeMouse)
        FF_SetRelativeMouse(
            s_count < 0 ? 1 : 0); // weak: absent in standalone tools
    return s_count;
}
BOOL DestroyCursor(HCURSOR /*hCursor*/)
{
    return TRUE;
}

// ============================================================================================================
// Artscout - 2026 (#104): case-insensitive path resolution for the Linux (case-sensitive ext4) data tree.
// The game data is stored lowercase, but the engine's .lst lists / hardcoded paths spell files in mixed case,
// so a verbatim open() misses. Resolve component-by-component: keep any component that already exists as spelled,
// otherwise match the single on-disk entry whose name is case-insensitively equal. '\' is normalised to '/'.
// Applied by CreateFileA below and by FF_CIFopen (to which the shim redirects fopen). Returns 1 if fully
// resolved to an existing path, 0 otherwise (out is filled best-effort so the caller still gets a sane error).
#include <sys/stat.h>
extern "C" int FF_CIResolvePath(const char* want, char* out, unsigned long cap)
{
    if (!want || !out || cap == 0)
        return 0;
    std::string w;
    w.reserve(strlen(want));
    for (const char* p = want; *p; ++p)
        w.push_back(*p == '\\' ? '/' : *p);
    if (::access(w.c_str(), F_OK) == 0)
    {
        if (w.size() < cap)
        {
            memcpy(out, w.c_str(), w.size() + 1);
            return 1;
        }
        return 0;
    }

    std::string cur;
    size_t i = 0;
    if (!w.empty() && w[0] == '/')
    {
        cur = "/";
        i = 1;
    }
    int ok = 1;
    while (i < w.size())
    {
        size_t slash = w.find('/', i);
        std::string comp =
            (slash == std::string::npos) ? w.substr(i) : w.substr(i, slash - i);
        size_t next = (slash == std::string::npos) ? w.size() : slash + 1;
        if (!comp.empty() && comp != ".")
        {
            std::string cand =
                cur.empty() ? comp :
                              (cur + (cur.back() == '/' ? "" : "/") + comp);
            if (::access(cand.c_str(), F_OK) == 0)
            {
                cur = cand;
            }
            else
            {
                DIR* d = ::opendir(cur.empty() ? "." : cur.c_str());
                std::string hit;
                if (d)
                {
                    for (struct dirent* e; (e = ::readdir(d)) != nullptr;)
                        if (::strcasecmp(e->d_name, comp.c_str()) == 0)
                        {
                            hit = e->d_name;
                            break;
                        }
                    ::closedir(d);
                }
                if (!hit.empty())
                    cur = cur.empty() ?
                              hit :
                              (cur + (cur.back() == '/' ? "" : "/") + hit);
                else
                {
                    cur = cand;
                    ok = 0;
                }
            }
        }
        i = next;
    }
    if (cur.size() < cap)
    {
        memcpy(out, cur.c_str(), cur.size() + 1);
        return ok;
    }
    return 0;
}

// fopen over the resolver. The shim's <windows.h> does `#define fopen FF_CIFopen`, so every engine fopen lands
// here. Try the path verbatim first (fast, no directory walk); only a READ that missed is case-insensitively
// re-resolved (writes/creates keep the exact path so new files land where asked).
#undef fopen
extern "C" FILE* FF_CIFopen(const char* path, const char* mode)
{
    if (!path)
        return nullptr;
    // Writes/appends ('w'/'a', or 'r+') create the file, so resolve up front: normalise '\'->'/' and case-fold
    // the existing parent dirs, keeping the new leaf as spelled -- otherwise a backslash path would spawn one
    // oddly-named file instead of landing in the tree. Pure reads stay verbatim-first (fast) and CI-retry on miss.
    if (mode && (mode[0] == 'w' || mode[0] == 'a' ||
                 (mode[0] == 'r' && strchr(mode, '+'))))
    {
        char cw[4096];
        return ::fopen(ci_for_write(path, cw, sizeof(cw)), mode);
    }
    FILE* f = ::fopen(path, mode);
    if (f || !mode)
        return f;
    if (mode[0] == 'r')
    {
        char buf[4096];
        if (FF_CIResolvePath(path, buf, sizeof(buf)))
            return ::fopen(buf, mode);
    }
    return nullptr;
}
