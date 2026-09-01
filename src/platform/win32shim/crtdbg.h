// Artscout - 2026 (#104, Linux Ф1): <crtdbg.h> -- MSVC's debug heap. There is no Linux equivalent and none is needed:
// the tracking macros compile away, and the tools that want real heap checking use ASan instead.
#ifndef FF_WIN32SHIM_CRTDBG_H
#define FF_WIN32SHIM_CRTDBG_H
#include <cassert>
#define _ASSERT(x) assert(x)
#define _ASSERTE(x) assert(x)
#define _CrtCheckMemory() 1
#define _CrtDumpMemoryLeaks() 0
#define _CrtSetDbgFlag(f) (0)
#define _CrtSetReportMode(t, m) (0)
#define _CrtSetReportFile(t, f) (0)
#define _CRTDBG_ALLOC_MEM_DF 0x01
#define _CRTDBG_LEAK_CHECK_DF 0x20
#define _CRTDBG_MODE_FILE 0x1
#define _CRTDBG_MODE_DEBUG 0x2
#define _CRT_WARN 0
#define _CRT_ERROR 1
#define _CRT_ASSERT 2
#define _malloc_dbg(s, t, f, l) malloc(s)
#define _free_dbg(p, t) free(p)
#endif
