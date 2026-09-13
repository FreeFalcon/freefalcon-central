// fflog.h -- Artscout - 2026: mirror the diagnostic stream to a FILE.
//
// The engine's three diagnostic loggers (TClipLog "[terr-mesh]", R12Log
// "[D3D12R]", VkbLog "[Vulkan]" / "[VKPROF]") all wrote to OutputDebugStringA
// and nothing else. That is invisible without DebugView or a debugger attached,
// which makes capturing a profiler run -- let alone A/B-ing two backends --
// needlessly awkward. Route them through here as well and the same lines land
// in FFDebug.log next to the executable.
//
// Header-only on purpose: an inline function's local static has vague linkage,
// so all three translation units share ONE handle without a new .cpp and
// without touching any .vcxproj.
#ifndef _FFLOG_H_
#define _FFLOG_H_

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Roll over rather than grow without bound: these lines are periodic (the
// profiler every ~120 frames, the clipmap status every 5s), so a long session
// would otherwise leave a file nobody wants to open.
#define FFLOG_MAX_BYTES (4 * 1024 * 1024)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996) // strcpy/fopen: the portable spellings, on purpose
#endif

inline FILE* FFDebugLogOpen()
{
    char path[1024];

#ifdef _WIN32
    // Next to the EXE, not the working directory: the game's cwd depends on how
    // it was launched, and a log you cannot find is a log you do not have.
    const DWORD n = GetModuleFileNameA(NULL, path, (DWORD)sizeof(path));
    if (n == 0 || n >= sizeof(path))
        return 0;
    char* slash = strrchr(path, '\\');
    if (!slash || (size_t)(slash - path) + sizeof("FFDebug.log") >= sizeof(path))
        return 0;
    strcpy(slash + 1, "FFDebug.log");
#else
    strcpy(path, "FFDebug.log");
#endif

    // Size check on its own handle: ftell() on a freshly opened append stream is
    // not required to report the end, so asking it would be a coin flip.
    bool truncate = false;
    FILE* probe = fopen(path, "rb");
    if (probe)
    {
        fseek(probe, 0, SEEK_END);
        truncate = (ftell(probe) > FFLOG_MAX_BYTES);
        fclose(probe);
    }

    FILE* f = fopen(path, truncate ? "wb" : "ab");
    if (!f)
        return 0;

    // Banner per run: the file is append-mode, so two backends' profiler runs
    // sit in one file and this is what separates them.
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "\n===== FreeFalcon session %04d-%02d-%02d %02d:%02d:%02d =====\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
    fputs("\n===== FreeFalcon session =====\n", f);
#endif
    fflush(f);
    return f;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// One handle for the process. Opened lazily on the first diagnostic line, so a
// run that logs nothing never creates the file.
inline FILE* FFDebugLogFile()
{
    static FILE* s_f = FFDebugLogOpen();
    return s_f;
}

// Same text to the debugger stream and the file. Flushed per line: a crash or a
// hard kill is exactly when the last few lines matter most, and these are far
// too infrequent for the flush to cost anything.
inline void FFDebugLog(const char* text)
{
    if (!text || !*text)
        return;

#ifdef _WIN32
    OutputDebugStringA(text);
#endif

    FILE* f = FFDebugLogFile();
    if (f)
    {
        fputs(text, f);
        fflush(f);
    }
}

#endif // _FFLOG_H_
