// Artscout - 2026 (#104, Linux Ф1): Win32 file/handle API -> POSIX (implemented in win32compat.cpp over a HANDLE
// table of fds/DIR*). Declarations + constants only here. NOTE most of the codebase does file I/O via the CRT
// (fopen/fread), which needs no shim; this covers the ~49 files that use the CreateFile/FindFirstFile handle API.
#ifndef FF_WIN32SHIM_FILE_H
#define FF_WIN32SHIM_FILE_H

// ---- access / share / disposition / attribute flags ----
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define GENERIC_EXECUTE 0x20000000u
#define FILE_SHARE_READ 0x00000001u
#define FILE_SHARE_WRITE 0x00000002u
#define FILE_SHARE_DELETE 0x00000004u
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5
#define FILE_ATTRIBUTE_READONLY 0x00000001u
#define FILE_ATTRIBUTE_HIDDEN 0x00000002u
#define FILE_ATTRIBUTE_SYSTEM 0x00000004u
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010u
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020u
#define FILE_ATTRIBUTE_NORMAL 0x00000080u
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#define INVALID_FILE_SIZE 0xFFFFFFFFu
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000u
// The companion cache hint to SEQUENTIAL_SCAN, which stood here alone. Both are OR-ed into CreateFile's flags word
// (FileMemMap.cpp:45, AcmiTape.cpp:2422), so it carries the real value and the word keeps its Windows meaning. The
// Linux equivalent lever is posix_fadvise(POSIX_FADV_RANDOM); nothing acts on the hint yet, which costs speed, not
// correctness -- it is a hint on Windows too.
#define FILE_FLAG_RANDOM_ACCESS 0x10000000u
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100u
#define MAX_PATH_SHIM MAX_PATH

// O_BINARY: Windows opens in text mode by default and needs this flag to stop CRLF translation. Linux has no text
// mode, so binary is the only behaviour there is -- 0 is the accurate value, not a placeholder. Same answer the
// codebase's own unzip.h:307 reaches for on non-Windows. Guarded: resmgr.h:640 also defines it.
#ifndef O_BINARY
#define O_BINARY 0
#endif
// O_TEXT is the other half of that same MSVC choice -- request CRLF translation. Linux cannot translate, so 0 is not
// a shrug: "no translation" is the only behaviour the platform has, and it is the right one for files written on it.
// (fileio.cpp:24 picks between the two from a `binary` flag; on Linux both arms now agree, as they must.)
#ifndef O_TEXT
#define O_TEXT 0
#endif

typedef struct _WIN32_FIND_DATAA
{
    DWORD dwFileAttributes;
    FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
    DWORD nFileSizeHigh, nFileSizeLow;
    DWORD dwReserved0, dwReserved1;
    char cFileName[MAX_PATH];
    char cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _BY_HANDLE_FILE_INFORMATION
{
    DWORD dwFileAttributes;
    FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
    DWORD dwVolumeSerialNumber, nFileSizeHigh, nFileSizeLow, nNumberOfLinks,
        nFileIndexHigh, nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

extern "C"
{
    HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD share, void* sec,
                       DWORD disp, DWORD flags, HANDLE tmpl);
    BOOL ReadFile(HANDLE h, LPVOID buf, DWORD toRead, LPDWORD read, void* ov);
    BOOL WriteFile(HANDLE h, LPCVOID buf, DWORD toWrite, LPDWORD written,
                   void* ov);
    DWORD SetFilePointer(HANDLE h, LONG lo, LONG* hi, DWORD method);
    DWORD GetFileSize(HANDLE h, LPDWORD hi);
    BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* size);
    BOOL FlushFileBuffers(HANDLE h);
    BOOL SetEndOfFile(HANDLE h);
    DWORD GetFileAttributesA(LPCSTR name);
    BOOL DeleteFileA(LPCSTR name);
    BOOL MoveFileA(LPCSTR from, LPCSTR to);
    BOOL CopyFileA(LPCSTR from, LPCSTR to, BOOL failIfExists);
    BOOL CreateDirectoryA(LPCSTR path, void* sec);
    BOOL RemoveDirectoryA(LPCSTR path);
    HANDLE FindFirstFileA(LPCSTR pattern, LPWIN32_FIND_DATAA data);
    BOOL FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA data);
    BOOL FindClose(HANDLE h);
    DWORD GetCurrentDirectoryA(DWORD len, LPSTR buf);
    BOOL SetCurrentDirectoryA(LPCSTR path);
    DWORD GetModuleFileNameA(HMODULE mod, LPSTR buf, DWORD size);
    DWORD GetLastError(void);
    void SetLastError(DWORD err);
    // GetFullPathName resolves a possibly-relative path against the cwd. POSIX realpath() is the same job, with one
    // real difference the implementation has to absorb: realpath() fails if the file does not exist, while Windows is
    // happy to normalise a path to something not yet created. Implemented in win32compat.cpp over realpath()+manual
    // join, NOT inline, because that difference needs actual code. (aiinput.cpp:173, dxutil.cpp:71, bspbuild.cpp:258.)
    DWORD GetFullPathNameA(LPCSTR name, DWORD len, LPSTR buf, LPSTR* filePart);
    // GetComputerName -> gethostname(): genuinely the same value, the machine's name. (dispcfg.cpp:60, DEBUG only.)
    BOOL GetComputerNameA(LPSTR buf, LPDWORD size);
    // FormatMessage(FROM_SYSTEM) turns a GetLastError() code into readable text -- which is exactly strerror(). The
    // mapping is real rather than approximate because this shim's SetLastError stores errno verbatim (win32compat.cpp:31
    // and every t_lastError = errno), so the code handed back here IS an errno. Five headers wrap it as PutErrorString
    // (f4error.h:16, shierror.h:20, shierror2.h:20, autoplay.cpp:47) and it is what the terrain/texture loaders print
    // when a read fails, so a stub would have turned real disk errors into blank strings.
    DWORD FormatMessageA(DWORD flags, const void* src, DWORD msgId,
                         DWORD langId, LPSTR buf, DWORD size, void* args);
    // LocalAlloc/LocalFree back FORMAT_MESSAGE_ALLOCATE_BUFFER (cfonts.cpp:166 LocalFree's what FormatMessage handed it).
    void* LocalAlloc(UINT flags, size_t bytes);
    void* LocalFree(void* mem);
    // ---- memory-mapped files -> mmap (win32compat.cpp) ----
    // A real implementation, because this is load-bearing: the ACMI tape is read through it (acmitape.cpp:2434) and
    // filemap.cpp serves bulk data from it. Win32's split into a mapping object plus views has no POSIX counterpart, so
    // the object is bookkeeping over the fd and MapViewOfFile does the mmap. See win32compat.cpp for the size-tracking
    // UnmapViewOfFile needs (Windows knows a view's length; munmap has to be told).
    HANDLE CreateFileMappingA(HANDLE hFile, void* sec, DWORD protect,
                              DWORD maxHigh, DWORD maxLow, LPCSTR name);
    LPVOID MapViewOfFile(HANDLE hMap, DWORD access, DWORD offHigh, DWORD offLow,
                         size_t bytes);
    LPVOID MapViewOfFileEx(HANDLE hMap, DWORD access, DWORD offHigh,
                           DWORD offLow, size_t bytes, LPVOID base);
    BOOL UnmapViewOfFile(LPCVOID addr);
    BOOL FlushViewOfFile(LPCVOID addr, size_t bytes);
}
// ANSI-name aliases (the codebase is not UNICODE, so the un-suffixed names map to the A variants)
#define CreateFile CreateFileA
#define GetFileAttributes GetFileAttributesA
#define DeleteFile DeleteFileA
#define MoveFile MoveFileA
#define CopyFile CopyFileA
#define CreateDirectory CreateDirectoryA
#define RemoveDirectory RemoveDirectoryA
#define FindFirstFile FindFirstFileA
#define FindNextFile FindNextFileA
#define GetCurrentDirectory GetCurrentDirectoryA
#define SetCurrentDirectory SetCurrentDirectoryA
#define GetModuleFileName GetModuleFileNameA
#define GetFullPathName GetFullPathNameA
#define GetComputerName GetComputerNameA
#define FormatMessage FormatMessageA
#define CreateFileMapping CreateFileMappingA

// ---- FormatMessage flags / language ids ----
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100u
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200u
#define FORMAT_MESSAGE_FROM_STRING 0x00000400u
#define FORMAT_MESSAGE_FROM_HMODULE 0x00000800u
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000u
#define FORMAT_MESSAGE_ARGUMENT_ARRAY 0x00002000u
// Language selection is meaningless here: strerror() answers in the process locale and there is no message table to
// pick a language from. The macro exists because the callers all pass MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
// the value is the real Win32 one so nothing that stores a LANGID changes shape.
#define LANG_NEUTRAL 0x00
#define SUBLANG_DEFAULT 0x01
#define MAKELANGID(p, s) ((((WORD)(s)) << 10) | (WORD)(p))
#define LMEM_FIXED 0x0000
#define LMEM_ZEROINIT 0x0040

// ---- legacy MSVC <io.h> low-level file functions on a CRT fd (tell/eof). A few engine files (graphics/3dlib
// fileio.cpp) still use the un-prefixed <io.h> spellings on the int fd returned by _open/open. Map them to POSIX
// lseek. C++ only (the shim's C-mode path is not built); qualified as ::tell / ::eof by the callers.
#ifdef __cplusplus
#include <unistd.h>
static inline long tell(int fd)
{
    return ::lseek(fd, 0, SEEK_CUR);
}
static inline int eof(int fd)
{ // 1 at end-of-file, 0 otherwise (Win32 _eof semantics)
    long cur = ::lseek(fd, 0, SEEK_CUR);
    long end = ::lseek(fd, 0, SEEK_END);
    ::lseek(fd, cur, SEEK_SET);
    return (cur >= end) ? 1 : 0;
}
#endif

#endif // FF_WIN32SHIM_FILE_H
