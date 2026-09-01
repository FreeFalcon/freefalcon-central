// Artscout - 2026 (#104, Linux Ф1): <commctrl.h> -- Win32 common controls. Only the message/style constants are
// named by the engine (dispcfg's display-settings list); the controls themselves are the SDL/UI layer's job, so no
// functions are declared here on purpose -- a call site that needs one must be ported, not silently no-op'd.
#ifndef FF_WIN32SHIM_COMMCTRL_H
#define FF_WIN32SHIM_COMMCTRL_H
#include <windows.h>
#define LVM_FIRST 0x1000
#define LVM_GETITEMCOUNT (LVM_FIRST + 4)
#define LVM_INSERTITEMA (LVM_FIRST + 7)
#define LVM_DELETEALLITEMS (LVM_FIRST + 9)
#define LVM_SETITEMTEXTA (LVM_FIRST + 46)
#define CB_ADDSTRING 0x0143
#define CB_RESETCONTENT 0x014B
#define CB_SETCURSEL 0x014E
#define CB_GETCURSEL 0x0147
#define LB_ADDSTRING 0x0180
#define LB_RESETCONTENT 0x0184
#define LB_SETCURSEL 0x0186
#define LB_GETCURSEL 0x0188
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define BST_UNCHECKED 0x0000
#define BST_CHECKED 0x0001
#endif
