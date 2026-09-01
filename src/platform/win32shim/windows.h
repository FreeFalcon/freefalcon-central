// Artscout - 2026 (#104, Linux Ф1): native Win32 shim -- a <windows.h> replacement for the Linux build.
// This directory is on the include path ONLY for the Linux (clang) build; the Windows (MSVC) build never sees it
// and keeps the real SDK. So this header must NOT be touched by / must not affect the Windows path.
//
// Scope: the DATA vocabulary the codebase leans on (types, handles, structs, calling conventions, constants) so
// 900k lines of Win32-flavoured C++ compile under clang. FUNCTIONS with real behaviour (GetTickCount, CreateFile,
// CRITICAL_SECTION ops, ...) are declared in the sub-headers here and IMPLEMENTED over POSIX in win32compat.cpp --
// this file is types only, so including it is free of link dependencies.
//
// Coverage is data-driven (grep of the tree): DWORD 2360, BOOL 1860, WORD 885, HRESULT 610, BYTE 556, HANDLE 284,
// RECT 265, HWND 235, UINT 234, LPARAM 220, COLORREF 217, __int64 135, WPARAM 109, TCHAR 101, ... The long tail is
// added error-driven as files are compiled.
#ifndef FF_WIN32SHIM_WINDOWS_H
#define FF_WIN32SHIM_WINDOWS_H

// Artscout - 2026 (Linux port): real <windows.h> defines _INC_WINDOWS to mark its own inclusion;
// engine headers use `#ifdef _INC_WINDOWS` to gate declarations that need Win32 types (e.g.
// fsound.h's InitSoundManager(HWND,...)). The shim provides those types, so advertise the same.
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#endif

#ifdef _WIN32
#error                                                                         \
    "win32shim/windows.h is the LINUX shim; it must never be on the include path for the Windows build."
#endif

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cwchar>

// Artscout - 2026 (Linux Ф1): PRE-INCLUDE the C++ standard library BEFORE we define the min/max MACROS below.
// MSVC's <windows.h> also defines min/max as macros, and its STL tolerates them; libstdc++ does NOT -- a `min`/
// `max` macro expands inside <vector>/<algorithm> ("expected unqualified-id" in stl_bvector.h). Parsing the STL
// here, while min/max are still plain identifiers, means every later `#include <vector>` (etc.) is guard-skipped,
// so libstdc++ is macro-free, and the game code's bare `max(a,b)` still gets the macro it expects. Add any STL
// header the codebase pulls that is not already here if an "expected unqualified-id" surfaces from it.
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <string>
#include <list>
#include <deque>
#include <queue>
#include <stack>
#include <memory>
#include <utility>
#include <functional>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <iterator>
#include <numeric>
#include <bitset>
#include <limits>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale> // Artscout - 2026: locale_conv.h / locale_facets_nonio.tcc use std::min
#include <iomanip> // Artscout - 2026: pulls the locale facets above; keep macro-free
#include <cmath>

// ---- calling conventions / MSVC decorations: nothing on Linux/System V x86-64 ----
#define WINAPI
#define APIENTRY
#define CALLBACK
#define WINAPIV
#define __stdcall
#define __cdecl
#define __fastcall
// Single-underscore spellings: MSVC accepts both, and the engine uses them (e.g. context.h's
// `static HRESULT _stdcall CALLBACK EnumZBufferFormatsCallback(...)`).
#define _stdcall
#define _cdecl
#define _fastcall
#define __declspec(x)
#define __forceinline inline __attribute__((always_inline))
#ifndef __int64
#define __int64 long long
#endif
#define IN
#define OUT
#define OPTIONAL
#define FAR
#define NEAR
#define PASCAL
#define CONST const
#define VOID void

// ---- Structured Exception Handling (SEH) ----
// Artscout - 2026 (Linux port): clang has no SEH on a Linux target. The engine wraps its worker
// threads in __try/__except(RecordExceptionInfo(...)) purely to log a crash dump (crashhandler,
// which is Windows-only and excluded here). Neutralise the construct: the guarded body runs
// normally, the __except filter (with its GetExceptionInformation()/RecordExceptionInfo call) is
// dropped so no crashhandler symbol is referenced, and the handler block is compiled but never
// executed. Hardware faults on Linux therefore surface as signals instead of being caught -- the
// honest behaviour without SEH. TODO: route fatal signals through a POSIX handler for parity.
#define __try
#define __except(filter) while (0)
#define __finally
#define GetExceptionInformation() ((void*)0)
#define GetExceptionCode() (0UL)
#define EXCEPTION_EXECUTE_HANDLER 1
#define EXCEPTION_CONTINUE_SEARCH 0
#define EXCEPTION_CONTINUE_EXECUTION (-1)

// ---- integer base types ----
typedef uint32_t
    DWORD; // Win32 DWORD is exactly 32-bit -- uint32_t (NOT `unsigned long`, which is
// 64-bit on LP64 Linux) keeps the on-disk / SAVE 32-bit contract exact.
typedef int BOOL;
typedef unsigned char BOOLEAN;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef float FLOAT;
typedef unsigned int UINT;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int32_t INT32;
typedef int64_t INT64;
typedef int8_t INT8;
typedef int16_t INT16;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef int8_t CHAR8;
typedef uint64_t QWORD;
typedef long
    LONG; // matches Win LONG (32) only on ILP32; on LP64 code that stores LONG on
typedef unsigned long
    ULONG; // disk must use the 32-bit path -- audited separately (save code).
typedef unsigned long long ULONGLONG;
typedef long long LONGLONG;
typedef unsigned char UCHAR;
typedef char CHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int INT;
typedef uint16_t
    WCHAR; // Win WCHAR is 16-bit (UTF-16); NOT wchar_t (32-bit on Linux)
typedef DWORD COLORREF;
typedef WORD ATOM;
typedef int HFILE;
typedef DWORD LCID;
typedef unsigned int DWORD32;
typedef uintptr_t UINT_PTR;
typedef intptr_t INT_PTR;
typedef uintptr_t DWORD_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef ULONG_PTR SIZE_T;
typedef LONG_PTR SSIZE_T;
typedef ULONG_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;

// ---- pointer / string types ----
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef WCHAR* LPWSTR;
typedef const WCHAR* LPCWSTR;
typedef BYTE* LPBYTE;
typedef WORD* LPWORD;
typedef DWORD* LPDWORD;
typedef BOOL* LPBOOL;
typedef LONG* LPLONG;
typedef int* LPINT;
typedef char TCHAR; // ANSI build (the codebase is not UNICODE)
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
typedef LPSTR PSTR;
typedef LPCSTR PCSTR;
typedef LPTSTR PTSTR;
typedef LPCTSTR PCTSTR;

// ---- handles: opaque, pointer-sized ----
#define FF_DECLARE_HANDLE(name)                                                \
    typedef struct name##__                                                    \
    {                                                                          \
        int unused;                                                            \
    }* name
FF_DECLARE_HANDLE(HANDLE);
FF_DECLARE_HANDLE(HWND);
FF_DECLARE_HANDLE(HINSTANCE);
FF_DECLARE_HANDLE(HDC);
FF_DECLARE_HANDLE(HBITMAP);
FF_DECLARE_HANDLE(HFONT);
FF_DECLARE_HANDLE(HICON);
FF_DECLARE_HANDLE(HCURSOR);
FF_DECLARE_HANDLE(HBRUSH);
FF_DECLARE_HANDLE(HMENU);
FF_DECLARE_HANDLE(HKEY);
FF_DECLARE_HANDLE(HGLRC);
FF_DECLARE_HANDLE(HGDIOBJ);
FF_DECLARE_HANDLE(HPALETTE);
FF_DECLARE_HANDLE(HRGN);
FF_DECLARE_HANDLE(HMONITOR);
// HMODULE is not a handle type of its own -- windef.h spells it `typedef HINSTANCE HMODULE;`, i.e. the SAME type,
// and the codebase relies on that: comsup.h:140 passes an HINSTANCE straight to GetProcAddress(HMODULE, ...), which
// is legal on Windows and stopped resolving here while a separate FF_DECLARE_HANDLE(HMODULE) made them unrelated
// structs. The dead `HMODULE_ALIAS` typedef that used to sit here was a symptom of the same split -- an invented
// name for a relationship the SDK states directly. Nothing referenced it, so it goes.
typedef HINSTANCE HMODULE;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef HANDLE HRSRC;
typedef HANDLE SC_HANDLE;

// ---- HRESULT / error ----
typedef LONG HRESULT;
#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_POINTER ((HRESULT)0x80004003L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
typedef DWORD HKEYVAL;

// SECURITY_ATTRIBUTES: passed to CreateEvent/CreateThread/etc.; on Linux there is no security
// descriptor, so only the fields the engine reads/zeroes need to exist.
typedef struct _SECURITY_ATTRIBUTES
{
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

// ---- common structs ----
typedef struct tagPOINT
{
    LONG x, y;
} POINT, *LPPOINT, *PPOINT;
typedef struct tagPOINTS
{
    SHORT x, y;
} POINTS;
typedef struct tagSIZE
{
    LONG cx, cy;
} SIZE, *LPSIZE, *PSIZE;
typedef struct tagRECT
{
    LONG left, top, right, bottom;
} RECT, *LPRECT, *PRECT;
typedef struct _RECTL
{
    LONG left, top, right, bottom;
} RECTL, *LPRECTL;
typedef struct tagPALETTEENTRY
{
    BYTE peRed, peGreen, peBlue, peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;
typedef struct _GUID
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID, IID, CLSID, *LPGUID, *LPCLSID;
typedef const GUID& REFGUID;
typedef const IID& REFIID;
typedef const CLSID& REFCLSID;
typedef GUID UUID;
typedef union _LARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;
typedef union _ULARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;
typedef struct _FILETIME
{
    DWORD dwLowDateTime, dwHighDateTime;
} FILETIME, *LPFILETIME;
typedef struct _SYSTEMTIME
{
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond,
        wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

// ---- constants / helper macros ----
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif
#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR) - 1)
#define CALLBACK_NULL 0

#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | (((DWORD)((WORD)(b))) << 16)))
#define MAKELPARAM(l, h) ((LPARAM)MAKELONG(l, h))
#define RGB(r, g, b)                                                           \
    ((COLORREF)(((BYTE)(r)) | (((WORD)((BYTE)(g))) << 8) |                     \
                (((DWORD)((BYTE)(b))) << 16)))
#define GetRValue(c) ((BYTE)((c) & 0xff))
#define GetGValue(c) ((BYTE)(((c) >> 8) & 0xff))
#define GetBValue(c) ((BYTE)(((c) >> 16) & 0xff))

// min/max MACROS -- MSVC's <windows.h> defines these (unless NOMINMAX). The codebase RELIES on them being macros:
// e.g. `float max = MAXIMUM(r,g,b)` where MAXIMUM expands to `max(r,b)` -- the macro fires before the local `max`
// variable shadows the name. Without the macro, `max(r,b)` calls the float local -> "not a function". So we must
// match Windows here. (Code that wants std::min/max qualifies it or #defines NOMINMAX, exactly as on Windows.)
#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256
#endif

// ---- memory helpers (windows.h macros) -> mem* ----
#define ZeroMemory(dst, len) memset((dst), 0, (len))
#define SecureZeroMemory(dst, len) memset((dst), 0, (len))
#define FillMemory(dst, len, val) memset((dst), (val), (len))
#define CopyMemory(dst, src, len) memcpy((dst), (src), (len))
#define MoveMemory(dst, src, len) memmove((dst), (src), (len))

// The behavioural half (functions, CRITICAL_SECTION, COM) lives in the sibling headers, pulled in below so a bare
// #include <windows.h> keeps working. Each is guarded and Linux-only.
#include "win32_ui.h" // WM_*/VK_*/MB_*, BITMAPINFO, registry+page flags, module fns -> constants + PODs
#include "win32_com.h" // IUnknown, CoInitialize/CoCreateInstance, __uuidof         -> minimal COM core
#include "win32_strings.h" // stricmp/itoa/strlwr/_s variants, GetPrivateProfile*       -> libc / POSIX
#include "win32_sync.h" // CRITICAL_SECTION, Interlocked*, Sleep, threads, events  -> pthread/POSIX
#include "win32_time.h" // GetTickCount, QueryPerformanceCounter, timeGetTime      -> clock_gettime
#include "win32_file.h" // CreateFile/ReadFile/FindFirstFile handle API            -> POSIX fd/dirent

// ---- case-insensitive file open (Linux ext4 is case-sensitive; the game data tree is stored lowercase) --------
// The engine's file lists / hardcoded paths spell files in mixed case, so a verbatim open() on ext4 misses. Every
// engine `fopen(path, mode)` is redirected to FF_CIFopen (win32compat.cpp): it opens the path verbatim first and,
// only on a READ miss, resolves it component-by-component case-insensitively against the on-disk tree and retries
// (writes/creates keep the exact path). CreateFileA does the same. FF_CIResolvePath is exposed for other openers.
extern "C" FILE* FF_CIFopen(const char* path, const char* mode);
extern "C" int FF_CIResolvePath(const char* want, char* out, unsigned long cap);
#define fopen(p, m) FF_CIFopen((p), (m))

#endif // FF_WIN32SHIM_WINDOWS_H
