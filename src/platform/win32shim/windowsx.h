// Artscout - 2026 (#104, Linux Ф1): <windowsx.h> -- message-cracker macros. Only the coordinate extractors are used;
// they are pure arithmetic on an LPARAM the engine already has.
#ifndef FF_WIN32SHIM_WINDOWSX_H
#define FF_WIN32SHIM_WINDOWSX_H
#include <windows.h>
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
