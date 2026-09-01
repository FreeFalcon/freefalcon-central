// Artscout - 2026 (#104, Linux Ф1): the Win32 window/message/GDI surface the codebase names but the shim lacked.
//
// This is the single biggest shim gap by file count: WM_USER alone was blocking 135 of the project's 829 sources, and
// IDRETRY / MB_OK / BITMAPINFO / VK_* / OutputDebugString most of the rest. None of it needs a window system -- these
// are plain constants and PODs the engine passes around (its own message ids are built as WM_USER + n, its UI code
// switches on VK_*, its bitmap loaders fill BITMAPINFO). The values are the real Win32 ones, because the engine both
// stores them and compares them against values it computed on Windows.
//
// Only DECLARATIONS and PODs live here. Anything that must *do* something on Linux (window creation, GDI drawing) is
// deliberately absent: those call sites belong to the SDL3 path (ffplatform), and a stub that silently succeeds would
// hide that. Functions defined here are the ones with an honest POSIX/no-op meaning.
#ifndef FF_WIN32SHIM_WIN32_UI_H
#define FF_WIN32SHIM_WIN32_UI_H

// ---- extra base types ----
typedef void* PVOID;
typedef void* LPCVOID_ALIAS;
typedef int (*FARPROC)();
typedef int (*PROC)();
// Window/dialog callback proc types. The engine only stores/passes these (the real message pump is SDL-side on
// Linux), so the signatures just need to exist for declarations like `DLGPROC pfn` to compile.
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef unsigned long u_long; // winsock spelling used by the comms code
typedef unsigned short u_short;
typedef unsigned char u_char;

FF_DECLARE_HANDLE(HPEN);
FF_DECLARE_HANDLE(HACCEL);
FF_DECLARE_HANDLE(HHOOK);

// ---- window messages. The engine defines its own ids as WM_USER + n, so the base value is load-bearing. ----
#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ENABLE 0x000A
#define WM_SETREDRAW 0x000B
#define WM_SETTEXT 0x000C
#define WM_GETTEXT 0x000D
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_ERASEBKGND 0x0014
#define WM_SYSCOLORCHANGE 0x0015
#define WM_SHOWWINDOW 0x0018
#define WM_ACTIVATEAPP 0x001C
#define WM_SETCURSOR 0x0020
#define WM_MOUSEACTIVATE 0x0021
#define WM_GETMINMAXINFO 0x0024
#define WM_SETFONT 0x0030
#define WM_GETFONT 0x0031
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_POWER 0x0048
#define WM_DISPLAYCHANGE 0x007E
#define WM_NCCREATE 0x0081
#define WM_NCDESTROY 0x0082
#define WM_NCCALCSIZE 0x0083
#define WM_NCHITTEST 0x0084
#define WM_NCPAINT 0x0085
#define WM_NCACTIVATE 0x0086
#define WM_NCMOUSEMOVE 0x00A0
#define WM_NCLBUTTONDOWN 0x00A1
#define WM_NCLBUTTONUP 0x00A2
#define WM_KEYFIRST 0x0100
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_DEADCHAR 0x0103
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_SYSCHAR 0x0106
#define WM_SYSDEADCHAR 0x0107
#define WM_KEYLAST 0x0108
#define WM_INITDIALOG 0x0110
#define WM_COMMAND 0x0111
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_HSCROLL 0x0114
#define WM_VSCROLL 0x0115
#define WM_INITMENU 0x0116
#define WM_INITMENUPOPUP 0x0117
#define WM_MENUSELECT 0x011F
#define WM_MENUCHAR 0x0120
#define WM_ENTERIDLE 0x0121
#define WM_CTLCOLORMSGBOX 0x0132
#define WM_CTLCOLOREDIT 0x0133
#define WM_CTLCOLORLISTBOX 0x0134
#define WM_CTLCOLORBTN 0x0135
#define WM_CTLCOLORDLG 0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC 0x0138
#define WM_MOUSEFIRST 0x0200
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_MOUSEWHEEL 0x020A
#define WM_MOUSELAST 0x020A
#define WM_PARENTNOTIFY 0x0210
#define WM_ENTERMENULOOP 0x0211
#define WM_EXITMENULOOP 0x0212
#define WM_SIZING 0x0214
#define WM_CAPTURECHANGED 0x0215
#define WM_MOVING 0x0216
#define WM_ENTERSIZEMOVE 0x0231
#define WM_EXITSIZEMOVE 0x0232
#define WM_DROPFILES 0x0233
#define WM_IME_SETCONTEXT 0x0281
#define WM_IME_NOTIFY 0x0282
#define WM_CUT 0x0300
#define WM_COPY 0x0301
#define WM_PASTE 0x0302
#define WM_CLEAR 0x0303
#define WM_UNDO 0x0304
#define WM_QUERYNEWPALETTE 0x030F
#define WM_PALETTEISCHANGING 0x0310
#define WM_PALETTECHANGED 0x0311
#define WM_HOTKEY 0x0312
#define WM_USER 0x0400
#define WM_APP 0x8000

// ---- MessageBox flags + the dialog results the engine compares against ----
#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_ABORTRETRYIGNORE 0x00000002L
#define MB_YESNOCANCEL 0x00000003L
#define MB_YESNO 0x00000004L
#define MB_RETRYCANCEL 0x00000005L
#define MB_ICONHAND 0x00000010L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_ICONASTERISK 0x00000040L
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONSTOP MB_ICONHAND
#define MB_ICONWARNING MB_ICONEXCLAMATION
#define MB_ICONINFORMATION MB_ICONASTERISK
#define MB_DEFBUTTON1 0x00000000L
#define MB_DEFBUTTON2 0x00000100L
#define MB_APPLMODAL 0x00000000L
#define MB_SYSTEMMODAL 0x00001000L
#define MB_TASKMODAL 0x00002000L
#define MB_SETFOREGROUND 0x00010000L
#define MB_TOPMOST 0x00040000L
#define IDOK 1
#define IDCANCEL 2
#define IDABORT 3
#define IDRETRY 4
#define IDIGNORE 5
#define IDYES 6
#define IDNO 7
#define IDCLOSE 8

// ---- virtual key codes ----
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_CANCEL 0x03
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SELECT 0x29
#define VK_PRINT 0x2A
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5

// ---- GetAsyncKeyState: real OS key state, needed by the Alt-Tab modifier re-sync (sikeybd.cpp) and the control
// binding UI (controltab.cpp). Declared here; DEFINED in the platform layer (ffplatform/ff_dinput.cpp) over
// SDL_GetKeyboardState -- the shim stays SDL-free. Returns the Win32 encoding: high bit (0x8000) = currently down.
extern "C" SHORT GetAsyncKeyState(int vKey);

// ---- GDI bitmap descriptors: PODs the texture/bitmap loaders fill in and read back ----
#pragma pack(push, 1)
typedef struct tagRGBQUAD
{
    BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved;
} RGBQUAD;
typedef struct tagRGBTRIPLE
{
    BYTE rgbtBlue, rgbtGreen, rgbtRed;
} RGBTRIPLE;
typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1, bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG biWidth, biHeight;
    WORD biPlanes, biBitCount;
    DWORD biCompression, biSizeImage;
    LONG biXPelsPerMeter, biYPelsPerMeter;
    DWORD biClrUsed, biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;

#define BI_RGB 0L
#define BI_RLE8 1L
#define BI_RLE4 2L
#define BI_BITFIELDS 3L
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1

// PALETTEENTRY itself already lives in windows.h -- only LOGPALETTE (which needs it) belongs here.
typedef struct tagLOGPALETTE
{
    WORD palVersion, palNumEntries;
    PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE;

// ---- font metrics -----------------------------------------------------------------------------------------------
// ui95's cfonts.h holds a TEXTMETRIC per font, which puts this struct in the include path of 136 of the project's
// sources. Layout is the real GDI one: the font code reads tmHeight/tmAscent/tmAveCharWidth to lay text out, and the
// ui95 font data was authored against those values on Windows.
typedef struct tagTEXTMETRICA
{
    LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading;
    LONG tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang;
    LONG tmDigitizedAspectX, tmDigitizedAspectY;
    BYTE tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar;
    BYTE tmItalic, tmUnderlined, tmStruckOut, tmPitchAndFamily, tmCharSet;
} TEXTMETRICA, *PTEXTMETRICA, *LPTEXTMETRICA;
typedef TEXTMETRICA TEXTMETRIC;
typedef PTEXTMETRICA PTEXTMETRIC;
typedef LPTEXTMETRICA LPTEXTMETRIC;

// LOGFONT -- the font description the UI passes to its font builder.
#define LF_FACESIZE 32
typedef struct tagLOGFONTA
{
    LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
    BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
    BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
    CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, *PLOGFONTA, *LPLOGFONTA;
typedef LOGFONTA LOGFONT;
typedef PLOGFONTA PLOGFONT;
typedef LPLOGFONTA LPLOGFONT;

#define FW_DONTCARE 0
#define FW_NORMAL 400
#define FW_BOLD 700
#define ANSI_CHARSET 0
#define DEFAULT_CHARSET 1
#define RUSSIAN_CHARSET 204
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define DRAFT_QUALITY 1
#define PROOF_QUALITY                                                          \
    2 // ui95's default font descriptor asks for it (cfonts.cpp:72)
#define NONANTIALIASED_QUALITY 3
#define ANTIALIASED_QUALITY 4
#define DEFAULT_PITCH 0
#define FIXED_PITCH 1
#define VARIABLE_PITCH 2
// Font FAMILY constants: the high nibble of LOGFONT::lfPitchAndFamily, OR-ed with the pitch above
// (cfonts.cpp:73 spells `VARIABLE_PITCH bitor FF_SWISS`) -- hence the <<4, which is why the real values matter.
// NOTE these GDI FF_ names are unrelated to the engine's own FF_ constants (FF_FOG, FF_COCKPIT, ... are fixed-
// function render-state bits); they only share a prefix, and none of the pairs collide.
#define FF_DONTCARE (0 << 4)
#define FF_ROMAN (1 << 4)
#define FF_SWISS (2 << 4)
#define FF_MODERN (3 << 4)
#define FF_SCRIPT (4 << 4)
#define FF_DECORATIVE (5 << 4)
#define OEM_CHARSET 255
#define OUT_TT_PRECIS 4
#define OUT_TT_ONLY_PRECIS 7
#define CLIP_STROKE_PRECIS 2
#define TRANSPARENT 1
#define OPAQUE 2

// ---- full font-weight ladder + charsets (ui95 cparser maps weight/charset names to these) ----
#define FW_THIN 100
#define FW_EXTRALIGHT 200
#define FW_ULTRALIGHT 200
#define FW_LIGHT 300
#define FW_REGULAR 400
#define FW_MEDIUM 500
#define FW_SEMIBOLD 600
#define FW_DEMIBOLD 600
#define FW_EXTRABOLD 800
#define FW_ULTRABOLD 800
#define FW_HEAVY 900
#define FW_BLACK 900
#define SYMBOL_CHARSET 2
#define SHIFTJIS_CHARSET 128
#define HANGEUL_CHARSET 129
#define HANGUL_CHARSET 129
#define GB2312_CHARSET 134
#define CHINESEBIG5_CHARSET 136
#define HEBREW_CHARSET 177
#define ARABIC_CHARSET 178
#define GREEK_CHARSET 161
#define TURKISH_CHARSET 162
#define BALTIC_CHARSET 186
#define EASTEUROPE_CHARSET 238
#define JOHAB_CHARSET 130
#define THAI_CHARSET 222
#define MAC_CHARSET 77
#define DIB_PAL_COLORS 1
// output-precision selectors (cparser maps font "precision" names to these)
#define OUT_STRING_PRECIS 1
#define OUT_CHARACTER_PRECIS 2
#define OUT_STROKE_PRECIS 3
#define OUT_DEVICE_PRECIS 5
#define OUT_RASTER_PRECIS 6
#define OUT_OUTLINE_PRECIS 8
// clip-precision selectors (cparser maps font "clip" names to these)
#define CLIP_CHARACTER_PRECIS 1
#define CLIP_STROKE_PRECIS 2
#define CLIP_MASK 0x0F
#define CLIP_LH_ANGLES (1 << 4)
#define CLIP_TT_ALWAYS (2 << 4)
#define CLIP_EMBEDDED (8 << 4)
// GetSystemMetrics selectors used by the UI
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1

// ---- GDI text/DC surface (Phase 3): honest software backend.
// Artscout - 2026 (Linux port): there is no GDI on Linux and no freetype dev headers in this
// environment, so glyph rasterisation is NOT implemented -- TextOut* draws an EMPTY glyph and logs a
// one-time TODO (see win32compat.cpp). Metrics (GetTextMetrics/GetCharWidth/GetTextExtentPoint32) return
// consistent, non-zero values derived from the requested font height so the UI's text layout still works;
// nothing here reports success while silently doing nothing. The DC/font/bitmap objects are a small tagged
// object model implemented in win32compat.cpp. TODO: swap in a freetype/SDL_ttf rasteriser for real glyphs.
extern "C"
{
    HDC CreateCompatibleDC(HDC hdc);
    BOOL DeleteDC(HDC hdc);
    HFONT CreateFontA(int height, int width, int esc, int orient, int weight,
                      DWORD italic, DWORD underline, DWORD strikeout,
                      DWORD charset, DWORD outPrec, DWORD clipPrec,
                      DWORD quality, DWORD pitchFamily, LPCSTR face);
    HFONT CreateFontIndirectA(const LOGFONTA* lf);
    // Params are void* so any GDI handle type (HFONT/HBITMAP/HGDIOBJ) converts without a cast -- the
    // shim's DECLARE_HANDLE makes those distinct struct pointers, unlike non-STRICT Windows where they
    // are all HANDLE and interchange freely.
    HGDIOBJ SelectObject(HDC hdc, void* obj);
    BOOL DeleteObject(void* obj);
    HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* bmi, UINT usage,
                             void** ppvBits, HANDLE section, DWORD off);
    BOOL TextOutA(HDC hdc, int x, int y, LPCSTR str, int count);
    COLORREF SetTextColor(HDC hdc, COLORREF c);
    COLORREF SetBkColor(HDC hdc, COLORREF c);
    int SetBkMode(HDC hdc, int mode);
    BOOL GetTextExtentPoint32A(HDC hdc, LPCSTR str, int count, LPSIZE size);
    BOOL GetTextMetricsA(HDC hdc, LPTEXTMETRIC tm);
    BOOL GetCharWidthA(HDC hdc, UINT first, UINT last, int* widths);
    int GetDeviceCaps(HDC hdc, int index);
    BOOL GdiFlush(void);
}
#define CreateFont CreateFontA
#define CreateFontIndirect CreateFontIndirectA
#define TextOut TextOutA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetTextMetrics GetTextMetricsA
#define GetCharWidth GetCharWidthA
#define GetCharWidth32 GetCharWidthA
#define LOGPIXELSY 90
#define LOGPIXELSX 88

// ---- version-info API (ctext.cpp reads the module's own version resource) ----
// Artscout - 2026 (Linux port): no PE version resource on Linux; report "not present" honestly so the
// caller falls back to its default string rather than reading a fabricated version.
extern "C"
{
    DWORD GetFileVersionInfoSizeA(LPCSTR file, LPDWORD handle);
    BOOL GetFileVersionInfoA(LPCSTR file, DWORD handle, DWORD len, LPVOID data);
    BOOL VerQueryValueA(LPCVOID block, LPCSTR sub, LPVOID* out, UINT* len);
}
#define GetFileVersionInfoSize GetFileVersionInfoSizeA
#define GetFileVersionInfo GetFileVersionInfoA
#define VerQueryValue VerQueryValueA

// ---- small window / input / thread helpers the UI toolkit calls ----
extern "C"
{
    BOOL GetClientRect(HWND hwnd, LPRECT r);
    BOOL GetUpdateRect(HWND hwnd, LPRECT r, BOOL erase);
    int GetSystemMetrics(int index);
    BOOL InvalidateRect(HWND hwnd, const RECT* r, BOOL erase);
    BOOL ValidateRect(HWND hwnd, const RECT* r);
    BOOL ClientToScreen(HWND hwnd, LPPOINT p);
    BOOL ScreenToClient(HWND hwnd, LPPOINT p);
    SHORT GetKeyState(int vk);
    UINT GetDoubleClickTime(void);
    LONG GetMessageTime(void);
    DWORD SuspendThread(HANDLE h);
    DWORD ResumeThread(HANDLE h);
}
static inline BOOL SetRect(LPRECT r, int l, int t, int rt, int b)
{
    if (!r)
        return FALSE;
    r->left = l;
    r->top = t;
    r->right = rt;
    r->bottom = b;
    return TRUE;
}

// Console cell -- the text-mode tools use it.
typedef struct _CHAR_INFO
{
    union
    {
        WCHAR UnicodeChar;
        CHAR AsciiChar;
    } Char;
    WORD Attributes;
} CHAR_INFO, *PCHAR_INFO;

// ---- Win32 status codes -----------------------------------------------------------------------------------------
// ERROR_SUCCESS alone is named 42 times (the Reg* callers all compare against it), and it was not defined anywhere in
// the shim. Values are the real Win32 ones -- they travel in variables the code compares and sometimes stores.
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#define ERROR_INVALID_HANDLE 6L
#define ERROR_ACCESS_DENIED 5L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_INSUFFICIENT_BUFFER 122L
#define ERROR_DLL_INIT_FAILED 1114L
#endif
typedef LONG LSTATUS;

// ---- registry roots ---------------------------------------------------------------------------------------------
#define HKEY_CLASSES_ROOT ((HKEY)(ULONG_PTR)0x80000000)
#define HKEY_CURRENT_USER ((HKEY)(ULONG_PTR)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)(ULONG_PTR)0x80000002)
#define HKEY_USERS ((HKEY)(ULONG_PTR)0x80000003)
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE 0x0002
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006
#define KEY_ALL_ACCESS 0xF003F
// WOW64 view selectors: on Win64 the 32-bit installer writes under Wow6432Node, so the readers ask for the 32-bit
// view explicitly (F4find.cpp:37, logbook.cpp:200, instant.cpp:153). Meaningless on Linux, but they are OR-ed into
// the samAccess argument, so the names must exist and must not collide with the KEY_* rights above.
#define KEY_WOW64_64KEY 0x0100
#define KEY_WOW64_32KEY 0x0200
#define REG_SZ 1
#define REG_BINARY 3
#define REG_DWORD 4

// ---- registry access --------------------------------------------------------------------------------------------
// Linux has no registry, and these report exactly that: every open/query fails with ERROR_FILE_NOT_FOUND, which is
// the same answer a Windows box without the key gives. That is a true statement, not a silent success -- and the
// callers already handle it (F4find.cpp:40 zeroes its buffer and returns FALSE, i.e. "fall back to the local path").
// Deliberately NOT returning ERROR_SUCCESS with an untouched output buffer: that would hand the caller uninitialised
// data it believes came from the registry. Where a value genuinely has to persist on Linux, the call site must move
// to the config layer -- these functions cannot invent a store, so they refuse rather than pretend.
inline LSTATUS RegOpenKeyExA(HKEY, LPCSTR, DWORD, DWORD, HKEY* phkResult)
{
    if (phkResult)
        *phkResult = 0;
    return ERROR_FILE_NOT_FOUND;
}
inline LSTATUS RegCreateKeyExA(HKEY, LPCSTR, DWORD, LPSTR, DWORD, DWORD, void*,
                               HKEY* phkResult, DWORD* pdw)
{
    if (phkResult)
        *phkResult = 0;
    if (pdw)
        *pdw = 0;
    return ERROR_FILE_NOT_FOUND;
}
inline LSTATUS RegQueryValueExA(HKEY, LPCSTR, DWORD*, DWORD* pType, LPBYTE,
                                DWORD* pcbData)
{
    if (pType)
        *pType = 0;
    if (pcbData)
        *pcbData = 0;
    return ERROR_FILE_NOT_FOUND;
}
inline LSTATUS RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD)
{
    return ERROR_ACCESS_DENIED;
} // nothing was written, so do not claim it was
inline LSTATUS RegCloseKey(HKEY)
{
    return ERROR_SUCCESS;
} // nothing was opened; closing nothing genuinely succeeds
#define RegOpenKeyEx RegOpenKeyExA
#define RegCreateKeyEx RegCreateKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA

// ---- memory-protection / mapping flags ----
#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_WRITECOPY 0x08
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define FILE_MAP_COPY 0x0001
#define FILE_MAP_WRITE 0x0002
#define FILE_MAP_READ 0x0004
#define FILE_MAP_ALL_ACCESS 0x000F001F
// SEC_*: section attributes OR-ed into CreateFileMapping's protection argument (FileMemMap.cpp:59 asks for
// PAGE_READONLY|SEC_COMMIT). Real values, so the flags word keeps its Windows meaning. SEC_COMMIT is the default
// behaviour on Windows and describes what mmap already does, so nothing is lost by the Linux mapping ignoring it.
#define SEC_COMMIT 0x8000000
#define SEC_IMAGE 0x1000000
#define SEC_RESERVE 0x4000000
#define SEC_NOCACHE 0x10000000

// ---- ShowWindow / SetWindowPos / window styles (stored in config, compared, passed around) ----
#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOWMINIMIZED 2
#define SW_SHOWMAXIMIZED 3
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_RESTORE 9
#define SW_SHOWDEFAULT 10
#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_CHILD 0x40000000L
#define WS_MINIMIZE 0x20000000L
#define WS_VISIBLE 0x10000000L
#define WS_DISABLED 0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MAXIMIZE 0x01000000L
#define WS_CAPTION 0x00C00000L
#define WS_BORDER 0x00800000L
#define WS_DLGFRAME 0x00400000L
#define WS_VSCROLL 0x00200000L
#define WS_HSCROLL 0x00100000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_MAXIMIZEBOX 0x00010000L
#define WS_OVERLAPPEDWINDOW                                                    \
    (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |                 \
     WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_EX_TOPMOST 0x00000008L
#define WS_EX_APPWINDOW 0x00040000L
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_WNDPROC (-4)
#define GWLP_USERDATA (-21)
#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOACTIVATE 0x0010
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define HWND_TOP ((HWND)0)
#define HWND_TOPMOST ((HWND) - 1)
#define HWND_NOTOPMOST ((HWND) - 2)

// SetFocus/GetFocus/SetActiveWindow: the game is a single SDL window, so keyboard focus is
// implicit and follows the OS. These calls only ever target that one window; returning it (the
// "previously focused" window) keeps the Win32 contract without a second window to juggle. Real
// focus changes, when needed, go through the SDL window layer, not these.
inline HWND SetFocus(HWND h)
{
    return h;
}
inline HWND GetFocus(void)
{
    return (HWND)0;
}
inline HWND SetActiveWindow(HWND h)
{
    return h;
}
inline HWND GetActiveWindow(void)
{
    return (HWND)0;
}
inline BOOL SetForegroundWindow(HWND)
{
    return 1;
}
// The SDL platform layer owns the real window; these legacy USER32 calls are no-ops on Linux (the game window is
// shown/updated by ffplatform). Stubbed so UI code that pokes the HWND directly still links.
inline BOOL ShowWindow(HWND, int)
{
    return 1;
}
inline BOOL UpdateWindow(HWND)
{
    return 1;
}

// ---- thread priorities the engine sets on its loader/sound threads ----
#ifndef THREAD_PRIORITY_IDLE
#define THREAD_PRIORITY_IDLE (-15)
#define THREAD_PRIORITY_LOWEST (-2)
#define THREAD_PRIORITY_BELOW_NORMAL (-1)
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_TIME_CRITICAL 15
#endif

// ---- MAKEINTRESOURCE / resource ids: the engine builds them, the SDL path resolves them by number ----
#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#endif
#define RT_RCDATA MAKEINTRESOURCE(10)
#define RT_BITMAP MAKEINTRESOURCE(2)

// ---- the handful of kernel/user calls with an honest Linux meaning ----
// OutputDebugString: the Windows debugger channel. stderr is the faithful Linux equivalent -- the engine uses it
// purely for tracing, so dropping the text would lose diagnostics we actively rely on (see VulkanBackend's VkbLog).
inline void OutputDebugStringA(LPCSTR s)
{
    if (s)
    {
        fputs(s, stderr);
    }
}
#define OutputDebugString OutputDebugStringA

// A process-wide "module handle" has no Linux analogue; the engine only ever passes it back to Win32 calls that the
// SDL path replaces. Returning a non-null token keeps the null-checks meaningful without pretending to load anything.
inline HMODULE GetModuleHandleA(LPCSTR)
{
    return (HMODULE)(ULONG_PTR)1;
}
#define GetModuleHandle GetModuleHandleA

// LoadLibrary/GetProcAddress: real dlopen/dlsym would need -ldl and a name-mangling policy per DLL the engine expects
// to exist on Windows only. Return failure -- the callers all test it, and a silent fake would route them into code
// that then calls through a null pointer.
inline HMODULE LoadLibraryA(LPCSTR)
{
    return (HMODULE)0;
}
#define LoadLibrary LoadLibraryA
inline BOOL FreeLibrary(HMODULE)
{
    return 1;
}
inline FARPROC GetProcAddress(HMODULE, LPCSTR)
{
    return (FARPROC)0;
}

// Heap*: the Win32 heap is just malloc/free here; the engine uses the default process heap only.
inline HANDLE GetProcessHeap(void)
{
    return (HANDLE)(ULONG_PTR)1;
}
inline LPVOID HeapAlloc(HANDLE, DWORD flags, SIZE_T bytes)
{
    void* p = ::malloc(bytes);
    if (p && (flags & 0x00000008 /*HEAP_ZERO_MEMORY*/))
        ::memset(p, 0, bytes);
    return p;
}
inline LPVOID HeapReAlloc(HANDLE, DWORD, LPVOID p, SIZE_T bytes)
{
    return ::realloc(p, bytes);
}
inline BOOL HeapFree(HANDLE, DWORD, LPVOID p)
{
    ::free(p);
    return 1;
}
#define HEAP_ZERO_MEMORY 0x00000008
// HeapCreate/HeapDestroy: private heaps collapse onto the process heap (malloc/free). Blocks
// allocated from a "private" heap are freed via HeapFree, which also uses free(), so a distinct
// handle value is enough -- allocations do not need to be tracked per-heap here.
inline HANDLE HeapCreate(DWORD /*flOptions*/, SIZE_T /*dwInitialSize*/,
                         SIZE_T /*dwMaximumSize*/)
{
    return (HANDLE)(ULONG_PTR)1;
}
inline BOOL HeapDestroy(HANDLE)
{
    return 1;
}

// UnmapViewOfFile is declared by win32_file.h and implemented over munmap in win32compat.cpp. It previously sat here
// as `inline BOOL UnmapViewOfFile(LPCVOID) { return 1; }` -- a stub that reported success while unmapping nothing,
// under a comment claiming the mapping layer already lived in win32_file.h, where it did not. It would have leaked
// every ACMI tape view silently. Now that the real one exists, the two collided outright (different language
// linkage), which is how it surfaced.

// PostMessage/SendMessage: the engine posts the internal FM_* command bus to its OWN render window (appWin). That
// window is registered with the ffplatform SDL event bus (ff_events.cpp), so these route there: Post -> enqueue,
// Send -> invoke the window proc synchronously. Messages to any OTHER hwnd (Win32 dialog CONTROLS -- GetDlgItem
// handles that are NOT registered) fall through to a no-op inside the bridge, preserving the old behaviour for the
// separate Win32-dialog subsystem. Bridge is defined in ff_events.cpp so this header stays free of ffplatform.
extern "C" BOOL FF_PostWindowMessage(HWND, UINT, WPARAM, LPARAM);
extern "C" LRESULT FF_SendWindowMessage(HWND, UINT, WPARAM, LPARAM);
inline BOOL PostMessageA(HWND h, UINT m, WPARAM w, LPARAM l)
{
    return FF_PostWindowMessage(h, m, w, l);
}
#define PostMessage PostMessageA
inline LRESULT SendMessageA(HWND h, UINT m, WPARAM w, LPARAM l)
{
    return FF_SendWindowMessage(h, m, w, l);
}
#define SendMessage SendMessageA
inline BOOL PostThreadMessageA(DWORD, UINT, WPARAM, LPARAM)
{
    return 0;
}
#define PostThreadMessage PostThreadMessageA

// ---- MessageBox -------------------------------------------------------------------------------------------------
// The most-named missing symbol in the whole codebase: 584 of the project's 829 sources reach it. A real dialog is
// the PLATFORM layer's job (SDL_ShowMessageBox), and the shim must not depend on SDL -- so this is a seam, not a
// stub: ffplatform installs the real implementation into ff_MessageBoxHook at startup, and until it does the text
// goes to stderr. That distinction matters, because the default ANSWERS the dialog (IDOK), and for an MB_YESNO or
// MB_OKCANCEL prompt that silently picks a branch for the user. Routing it through the hook means the answer is a
// real one wherever a UI exists, and the fallback stays visible in the log rather than being invisible.
extern int (*ff_MessageBoxHook)(void* hwnd, const char* text,
                                const char* caption, unsigned type);
int ff_MessageBoxDefault(void* hwnd, const char* text, const char* caption,
                         unsigned type);
inline int MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type)
{
    return (ff_MessageBoxHook ? ff_MessageBoxHook : ff_MessageBoxDefault)(
        (void*)hwnd, text, caption, (unsigned)type);
}
#define MessageBox MessageBoxA
#define MessageBoxEx(h, t, c, ty, l) MessageBoxA((h), (t), (c), (ty))

// GetCurrentTime is winuser.h's own alias for GetTickCount -- a millisecond tick, NOT a wall clock, despite the name.
// The callers time operations with it (campaign.cpp:294 brackets gGps->Update() and subtracts), so mapping it to the
// shim's real GetTickCount keeps those deltas meaningful.
#ifndef GetCurrentTime
#define GetCurrentTime() GetTickCount()
#endif

// ---- mouse cursor -----------------------------------------------------------------------------------------------
// DECLARED, NOT DEFINED -- on purpose. The UI drives a real cursor here (ui_main.cpp:394 keeps an HCURSOR gCursors[]
// filled by LoadCursor; campaign.cpp:291 flips to CRSR_WAIT around long operations), so an inline no-op would build
// clean and then leave the pointer visibly stuck on the wrong shape with nothing pointing at why. Bodies belong to
// the SDL3 input layer (Ф3, SDL_CreateCursor/SDL_SetCursor); until it lands, the link error names the exact gap.
HCURSOR LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
#define LoadCursor LoadCursorA
HCURSOR SetCursor(HCURSOR hCursor);
int ShowCursor(BOOL bShow);
BOOL DestroyCursor(HCURSOR hCursor);

#ifndef _isnan
#define _isnan(x) (std::isnan(x))
#endif
#ifndef _finite
#define _finite(x) (std::isfinite(x))
#endif

#endif // FF_WIN32SHIM_WIN32_UI_H
