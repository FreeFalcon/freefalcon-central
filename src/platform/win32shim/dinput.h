// Artscout - 2026 (#104, Linux Ф1): minimal <dinput.h> stub. DirectInput is REPLACED by SDL3 (Ф3); this only
// provides the types/constants so the ~34 files that hold DI pointers or DI structs in their headers compile.
// The interfaces are opaque (pointer use only); any code that CALLS DI methods is rewritten in Ф3. Linux-only.
#ifndef FF_WIN32SHIM_DINPUT_H
#define FF_WIN32SHIM_DINPUT_H
#ifdef _WIN32
#error "win32shim/dinput.h is the Linux shim."
#endif
#include <windows.h>

#define DIRECTINPUT_VERSION 0x0800
typedef HRESULT DIRESULT;
#define DI_OK S_OK
#define DI_NOEFFECT S_FALSE
#define DIERR_INPUTLOST ((HRESULT)0x8007001EL)
#define DIERR_NOTACQUIRED ((HRESULT)0x8007000CL)
#define DIENUM_CONTINUE 1
#define DIENUM_STOP 0

// ---- DIK_* keyboard scancodes ------------------------------------------------------------------------------------
// The shim had none of these while the engine names 120 of them, so every source that mentions a key stopped here.
// These are the SDK's real values (PS/2 scan code set 1) and they are NOT free to renumber: the engine treats a DIK
// code as a dense index and as persisted data. ascii.h:24 does Key_Chart[(DIK_KEY) & 0xff], a 256-entry table whose
// slots ARE these numbers (ckbd.h's rows are commented with the scancode they sit on); setupinp.cpp:463 tests
// `val >= DIK_1 and val <= DIK_9`, which only holds because 1..9 are contiguous at 0x02..0x0A; and the same numbers
// live in the .key binding files on disk. Sequential invented values would compile and then silently mis-map every
// key. SDL3 (Ф3) will translate its own scancodes into these, so this table stays the wire format either way.
#define DIK_ESCAPE 0x01
#define DIK_1 0x02
#define DIK_2 0x03
#define DIK_3 0x04
#define DIK_4 0x05
#define DIK_5 0x06
#define DIK_6 0x07
#define DIK_7 0x08
#define DIK_8 0x09
#define DIK_9 0x0A
#define DIK_0 0x0B
#define DIK_MINUS 0x0C /* - on main keyboard */
#define DIK_EQUALS 0x0D
#define DIK_BACK 0x0E /* backspace */
#define DIK_TAB 0x0F
#define DIK_Q 0x10
#define DIK_W 0x11
#define DIK_E 0x12
#define DIK_R 0x13
#define DIK_T 0x14
#define DIK_Y 0x15
#define DIK_U 0x16
#define DIK_I 0x17
#define DIK_O 0x18
#define DIK_P 0x19
#define DIK_LBRACKET 0x1A
#define DIK_RBRACKET 0x1B
#define DIK_RETURN 0x1C /* Enter on main keyboard */
#define DIK_LCONTROL 0x1D
#define DIK_A 0x1E
#define DIK_S 0x1F
#define DIK_D 0x20
#define DIK_F 0x21
#define DIK_G 0x22
#define DIK_H 0x23
#define DIK_J 0x24
#define DIK_K 0x25
#define DIK_L 0x26
#define DIK_SEMICOLON 0x27
#define DIK_APOSTROPHE 0x28
#define DIK_GRAVE 0x29 /* accent grave */
#define DIK_LSHIFT 0x2A
#define DIK_BACKSLASH 0x2B
#define DIK_Z 0x2C
#define DIK_X 0x2D
#define DIK_C 0x2E
#define DIK_V 0x2F
#define DIK_B 0x30
#define DIK_N 0x31
#define DIK_M 0x32
#define DIK_COMMA 0x33
#define DIK_PERIOD 0x34 /* . on main keyboard */
#define DIK_SLASH 0x35 /* / on main keyboard */
#define DIK_RSHIFT 0x36
#define DIK_MULTIPLY 0x37 /* * on numeric keypad */
#define DIK_LMENU 0x38 /* left Alt */
#define DIK_SPACE 0x39
#define DIK_CAPITAL 0x3A /* caps lock */
#define DIK_F1 0x3B
#define DIK_F2 0x3C
#define DIK_F3 0x3D
#define DIK_F4 0x3E
#define DIK_F5 0x3F
#define DIK_F6 0x40
#define DIK_F7 0x41
#define DIK_F8 0x42
#define DIK_F9 0x43
#define DIK_F10 0x44
#define DIK_NUMLOCK 0x45
#define DIK_SCROLL 0x46 /* scroll lock */
#define DIK_NUMPAD7 0x47
#define DIK_NUMPAD8 0x48
#define DIK_NUMPAD9 0x49
#define DIK_SUBTRACT 0x4A /* - on numeric keypad */
#define DIK_NUMPAD4 0x4B
#define DIK_NUMPAD5 0x4C
#define DIK_NUMPAD6 0x4D
#define DIK_ADD 0x4E /* + on numeric keypad */
#define DIK_NUMPAD1 0x4F
#define DIK_NUMPAD2 0x50
#define DIK_NUMPAD3 0x51
#define DIK_NUMPAD0 0x52
#define DIK_DECIMAL 0x53 /* . on numeric keypad */
#define DIK_OEM_102 0x56 /* < > | on UK/Germany keyboards */
#define DIK_F11 0x57
#define DIK_F12 0x58
#define DIK_F13 0x64
#define DIK_F14 0x65
#define DIK_F15 0x66
#define DIK_KANA 0x70 /* (Japanese keyboard) */
#define DIK_ABNT_C1 0x73 /* / ? on Portuguese (Brazilian) keyboards */
#define DIK_CONVERT 0x79 /* (Japanese keyboard) */
#define DIK_NOCONVERT 0x7B /* (Japanese keyboard) */
#define DIK_YEN 0x7D /* (Japanese keyboard) */
#define DIK_ABNT_C2 0x7E /* numpad . on Portuguese (Brazilian) keyboards */
#define DIK_NUMPADEQUALS 0x8D /* = on numeric keypad (NEC PC98) */
#define DIK_PREVTRACK                                                          \
    0x90 /* previous track; also circumflex on Japanese keyboard */
#define DIK_AT 0x91 /* (NEC PC98) */
#define DIK_COLON 0x92 /* (NEC PC98) */
#define DIK_UNDERLINE 0x93 /* (NEC PC98) */
#define DIK_KANJI 0x94 /* (Japanese keyboard) */
#define DIK_STOP 0x95 /* (NEC PC98) */
#define DIK_AX 0x96 /* (Japan AX) */
#define DIK_UNLABELED 0x97 /* (J3100) */
#define DIK_NEXTTRACK 0x99 /* next track */
#define DIK_NUMPADENTER 0x9C /* Enter on numeric keypad */
#define DIK_RCONTROL 0x9D
#define DIK_MUTE 0xA0
#define DIK_CALCULATOR 0xA1
#define DIK_PLAYPAUSE 0xA2
#define DIK_MEDIASTOP 0xA4
#define DIK_VOLUMEDOWN 0xAE
#define DIK_VOLUMEUP 0xB0
#define DIK_WEBHOME 0xB2
#define DIK_NUMPADCOMMA 0xB3 /* , on numeric keypad (NEC PC98) */
#define DIK_DIVIDE 0xB5 /* / on numeric keypad */
#define DIK_SYSRQ 0xB7
#define DIK_RMENU 0xB8 /* right Alt */
#define DIK_PAUSE 0xC5
#define DIK_HOME 0xC7
#define DIK_UP 0xC8 /* up arrow on arrow keypad */
#define DIK_PRIOR 0xC9 /* page up */
#define DIK_LEFT 0xCB
#define DIK_RIGHT 0xCD
#define DIK_END 0xCF
#define DIK_DOWN 0xD0
#define DIK_NEXT 0xD1 /* page down */
#define DIK_INSERT 0xD2
#define DIK_DELETE 0xD3
#define DIK_LWIN 0xDB
#define DIK_RWIN 0xDC
#define DIK_APPS 0xDD
#define DIK_POWER 0xDE
#define DIK_SLEEP 0xDF
// SDK aliases -- alternate spellings of the codes above, not new keys.
#define DIK_BACKSPACE DIK_BACK
#define DIK_NUMPADSTAR DIK_MULTIPLY
#define DIK_LALT DIK_LMENU
#define DIK_CAPSLOCK DIK_CAPITAL
#define DIK_NUMPADMINUS DIK_SUBTRACT
#define DIK_NUMPADPLUS DIK_ADD
#define DIK_NUMPADPERIOD DIK_DECIMAL
#define DIK_NUMPADSLASH DIK_DIVIDE
#define DIK_RALT DIK_RMENU
#define DIK_UPARROW DIK_UP
#define DIK_PGUP DIK_PRIOR
#define DIK_LEFTARROW DIK_LEFT
#define DIK_RIGHTARROW DIK_RIGHT
#define DIK_DOWNARROW DIK_DOWN
#define DIK_PGDN DIK_NEXT
#define DIK_CIRCUMFLEX DIK_PREVTRACK
#define DI8DEVCLASS_GAMECTRL 4
#define DIEDFL_ATTACHEDONLY 0x00000001
#define DISCL_EXCLUSIVE 0x00000001
#define DISCL_NONEXCLUSIVE 0x00000002
#define DISCL_FOREGROUND 0x00000004
#define DISCL_BACKGROUND 0x00000008
#define DIPH_DEVICE 0
#define DIPH_BYOFFSET 1
#define DIPH_BYID 2
// MAKEDIPROP: a small integer property id smuggled as a GUID lvalue. SetProperty takes REFGUID (const GUID&), so
// the token must dereference to a GUID at address N; the backend recovers N with (UINT_PTR)&rguidProp. This is the
// SDK's own trick (dinput.h spells it `(*(const GUID*)(prop))`); the earlier shim used a bare pointer, which stopped
// binding the instant SetProperty became a real, typed method.
#define MAKEDIPROP(prop) (*(const GUID*)(UINT_PTR)(prop))
#define DIPROP_BUFFERSIZE MAKEDIPROP(1)
#define DIPROP_RANGE MAKEDIPROP(4)
#define DIPROP_DEADZONE MAKEDIPROP(5)
#define DIPROP_SATURATION MAKEDIPROP(6)
#define DIDFT_ALL 0x00000000
#define DIDFT_AXIS 0x00000003
#define DIDFT_BUTTON 0x0000000C
#define DIDFT_POV 0x00000010
#define DIDFT_GETTYPE(n) LOBYTE(HIWORD(n))
#define DIDOI_ASPECTPOSITION 0x00000100

typedef struct _DIDEVCAPS
{
    DWORD dwSize, dwFlags, dwDevType, dwAxes, dwButtons, dwPOVs;
    DWORD dwFFSamplePeriod, dwFFMinTimeResolution, dwFirmwareRevision,
        dwHardwareRevision, dwFFDriverVersion;
} DIDEVCAPS, *LPDIDEVCAPS;

typedef struct DIPROPHEADER
{
    DWORD dwSize, dwHeaderSize, dwObj, dwHow;
} DIPROPHEADER, *LPDIPROPHEADER;
typedef struct DIPROPDWORD
{
    DIPROPHEADER diph;
    DWORD dwData;
} DIPROPDWORD, *LPDIPROPDWORD;
typedef struct DIPROPRANGE
{
    DIPROPHEADER diph;
    LONG lMin, lMax;
} DIPROPRANGE, *LPDIPROPRANGE;
typedef struct DIPROPSTRING
{
    DIPROPHEADER diph;
    WCHAR wsz[260];
} DIPROPSTRING, *LPDIPROPSTRING;
typedef struct DIPROPGUIDANDPATH
{
    DIPROPHEADER diph;
    GUID guidClass;
    WCHAR wszPath[260];
} DIPROPGUIDANDPATH, *LPDIPROPGUIDANDPATH;
// The axis-shaping calibration point. The SDK names it CPOINT with members {lP, dwLog}; the shim had invented
// "DIPROPPOINTS {lP, lV}", whose second member does not exist anywhere. simio.h:195 assigns cp[j].dwLog, so every
// source reaching simio.h died here (39 files -- the second-largest failure class). Matching the SDK matters beyond
// compiling: siloop.cpp:327 hands this layout to SetProperty(DIPROP_CPOINTS), i.e. straight to a driver, so an
// invented field list would silently differ from the shape the Windows build ships.
typedef struct _CPOINT
{
    LONG lP; // raw proportional value
    DWORD dwLog; // logical_value / max_logical_value
} CPOINT, *PCPOINT;
// MAXCPOINTSNUM is the DirectInput SDK's own name for this bound, and the engine uses it directly (simio.h iterates
// CalibPoints[i].cp[] with it). It was missing while the array was spelled 8 -- so the array existed and the loop
// over it did not compile. Same value as the SDK.
#define MAXCPOINTSNUM 8
typedef struct DIPROPCPOINTS
{
    DIPROPHEADER diph;
    DWORD dwCPointsNum;
    CPOINT cp[MAXCPOINTSNUM];
} DIPROPCPOINTS, *LPDIPROPCPOINTS;

typedef struct DIOBJECTDATAFORMAT
{
    const GUID* pguid;
    DWORD dwOfs, dwType, dwFlags;
} DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;
typedef struct DIDATAFORMAT
{
    DWORD dwSize, dwObjSize, dwFlags, dwDataSize, dwNumObjs;
    LPDIOBJECTDATAFORMAT rgodf;
} DIDATAFORMAT, *LPDIDATAFORMAT;
typedef const DIDATAFORMAT* LPCDIDATAFORMAT;

typedef struct DIDEVICEINSTANCEA
{
    DWORD dwSize;
    GUID guidInstance, guidProduct;
    DWORD dwDevType;
    char tszInstanceName[MAX_PATH], tszProductName[MAX_PATH];
    GUID guidFFDriver;
    WORD wUsagePage, wUsage;
} DIDEVICEINSTANCEA, *LPDIDEVICEINSTANCEA;
#define DIDEVICEINSTANCE DIDEVICEINSTANCEA
#define LPDIDEVICEINSTANCE LPDIDEVICEINSTANCEA
typedef const DIDEVICEINSTANCEA* LPCDIDEVICEINSTANCEA;
// The const flavour needs the same unsuffixed alias the two lines above give the non-const ones; without it the
// enumeration callbacks (siloop.cpp:46, which spells the SDK's own LPCDIDEVICEINSTANCE) had no such type.
#define LPCDIDEVICEINSTANCE LPCDIDEVICEINSTANCEA

typedef struct DIDEVICEOBJECTINSTANCEA
{
    DWORD dwSize;
    GUID guidType;
    DWORD dwOfs, dwType, dwFlags;
    char tszName[MAX_PATH];
    DWORD dwFFMaxForce, dwFFForceResolution;
    WORD wCollectionNumber, wDesignatorIndex, wUsagePage, wUsage;
    DWORD dwDimension;
    WORD wExponent, wReportId;
} DIDEVICEOBJECTINSTANCEA, *LPDIDEVICEOBJECTINSTANCEA;
#define DIDEVICEOBJECTINSTANCE DIDEVICEOBJECTINSTANCEA
#define LPDIDEVICEOBJECTINSTANCE LPDIDEVICEOBJECTINSTANCEA
typedef const DIDEVICEOBJECTINSTANCEA* LPCDIDEVICEOBJECTINSTANCEA;

typedef struct DIJOYSTATE
{
    LONG lX, lY, lZ, lRx, lRy, lRz;
    LONG rglSlider[2];
    DWORD rgdwPOV[4];
    BYTE rgbButtons[32];
} DIJOYSTATE, *LPDIJOYSTATE;
typedef struct DIJOYSTATE2
{
    LONG lX, lY, lZ, lRx, lRy, lRz;
    LONG rglSlider[2];
    DWORD rgdwPOV[4];
    BYTE rgbButtons[128];
    LONG lVX, lVY, lVZ, lVRx, lVRy, lVRz;
    LONG rglVSlider[2];
    LONG lAX, lAY, lAZ, lARx, lARy, lARz;
    LONG rglASlider[2];
    LONG lFX, lFY, lFZ, lFRx, lFRy, lFRz;
    LONG rglFSlider[2];
} DIJOYSTATE2, *LPDIJOYSTATE2;
typedef struct DIMOUSESTATE
{
    LONG lX, lY, lZ;
    BYTE rgbButtons[4];
} DIMOUSESTATE, *LPDIMOUSESTATE;
typedef struct DIMOUSESTATE2
{
    LONG lX, lY, lZ;
    BYTE rgbButtons[8];
} DIMOUSESTATE2, *LPDIMOUSESTATE2;
typedef struct DIDEVICEOBJECTDATA
{
    DWORD dwOfs, dwData, dwTimeStamp, dwSequence;
    UINT_PTR uAppData;
} DIDEVICEOBJECTDATA, *LPDIDEVICEOBJECTDATA;

// ---- DIJOFS_*: byte offsets of the axes within DIJOYSTATE ---------------------------------------------------------
// DirectInput identifies an axis by its offset in the state struct, and the engine switches on that (sijoy.cpp:1014
// `case DIJOFS_X:`), so these must be constant expressions -- offsetof is one, and works in a case label.
// Derived rather than hardcoded on purpose: the SDK's literal values (0, 4, 8, ...) assume a 32-bit LONG, and LONG
// is `long` here, i.e. 64-bit under LP64 (windows.h:104 flags this as a separate audit). Hardcoding the SDK numbers
// would silently disagree with this build's own struct; offsetof stays true to whatever LONG is, and becomes
// byte-identical to Windows the moment LONG is narrowed to 32 bits.
// offsetof, not the SDK's `&((type*)0)->field` spelling: that one is a null dereference the C++ front end refuses to
// fold, so `int AxisOffsets[] = { DIJOFS_X, ... }` (siloop.cpp:255) rejected it as a narrowing of a non-constant.
// __builtin_offsetof folds, so these stay usable in case labels and initialisers -- which is where they are used.
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((LONG) __builtin_offsetof(type, field))
#endif
#define DIJOFS_X FIELD_OFFSET(DIJOYSTATE, lX)
#define DIJOFS_Y FIELD_OFFSET(DIJOYSTATE, lY)
#define DIJOFS_Z FIELD_OFFSET(DIJOYSTATE, lZ)
#define DIJOFS_RX FIELD_OFFSET(DIJOYSTATE, lRx)
#define DIJOFS_RY FIELD_OFFSET(DIJOYSTATE, lRy)
#define DIJOFS_RZ FIELD_OFFSET(DIJOYSTATE, lRz)
#define DIJOFS_SLIDER(n)                                                       \
    (FIELD_OFFSET(DIJOYSTATE, rglSlider) + (n) * sizeof(LONG))
#define DIJOFS_POV(n) (FIELD_OFFSET(DIJOYSTATE, rgdwPOV) + (n) * sizeof(DWORD))
#define DIJOFS_BUTTON(n) (FIELD_OFFSET(DIJOYSTATE, rgbButtons) + (n))

// ---- Ф3 (input port): DirectInput is implemented ON TOP OF SDL3 ------------------------------------------------
// The interfaces below are CONCRETE (methods declared here, defined in platform/ffplatform/ff_dinput.cpp against
// SDL3). The engine's ~22k lines of input code (sijoy/siloop/simouse/sikeybd/Io/...) were written against the
// DirectInput8 API and its data formats -- those formats are also the on-disk wire format for the .key/.dat binding
// files -- so the correct port keeps that API and swaps the driver underneath (DI method -> SDL_Joystick /
// SDL_GetKeyboardState / SDL_GetMouseState), rather than rewriting every call site. The shim stays SDL-free (SDL is
// only pulled in ff_dinput.cpp, in the platform layer that links SDL3); here we only declare the vocabulary.

// ---- extra DI_/DIERR_ result codes (sierror.cpp's DiErrTable names them all) ----
// SDK values. The engine only branches on DI_OK / DIERR_INPUTLOST / DIERR_NOTACQUIRED (already above); the rest just
// need to exist as distinct HRESULTs for the diagnostic lookup table to compile.
#define DI_POLLEDDEVICE ((HRESULT)0x00000002L)
#define DI_DOWNLOADSKIPPED ((HRESULT)0x00000003L)
#define DI_EFFECTRESTARTED ((HRESULT)0x00000004L)
#define DI_TRUNCATED ((HRESULT)0x00000008L)
#define DI_TRUNCATEDANDRESTARTED ((HRESULT)0x0000000CL)
#define DI_BUFFEROVERFLOW S_FALSE
#define DI_NOTATTACHED S_FALSE
#define DI_PROPNOEFFECT S_FALSE
#define DIERR_INSUFFICIENTPRIVS ((HRESULT)0x80040200L)
#define DIERR_DEVICEFULL ((HRESULT)0x80040201L)
#define DIERR_MOREDATA ((HRESULT)0x80040202L)
#define DIERR_NOTDOWNLOADED ((HRESULT)0x80040203L)
#define DIERR_HASEFFECTS ((HRESULT)0x80040204L)
#define DIERR_NOTEXCLUSIVEACQUIRED ((HRESULT)0x80040205L)
#define DIERR_INCOMPLETEEFFECT ((HRESULT)0x80040206L)
#define DIERR_NOTBUFFERED ((HRESULT)0x80040207L)
#define DIERR_EFFECTPLAYING ((HRESULT)0x80040208L)
#define DIERR_UNPLUGGED ((HRESULT)0x80040209L)
#define DIERR_REPORTFULL ((HRESULT)0x8004020AL)
#define DIERR_ACQUIRED                                                         \
    ((HRESULT)0x8007000CL) /* DI_OK-family Win32 ERROR_BUSY as HRESULT */
#define DIERR_ALREADYINITIALIZED ((HRESULT)0x800700B7L)
#define DIERR_BADDRIVERVER ((HRESULT)0x80070077L)
#define DIERR_BETADIRECTINPUTVERSION ((HRESULT)0x8007047FL)
#define DIERR_OLDDIRECTINPUTVERSION ((HRESULT)0x8007047EL)
#define DIERR_DEVICENOTREG ((HRESULT)0x80040154L) /* REGDB_E_CLASSNOTREG */
#define DIERR_GENERIC E_FAIL
#define DIERR_HANDLEEXISTS ((HRESULT)0x80070005L) /* E_ACCESSDENIED */
#define DIERR_READONLY ((HRESULT)0x80070005L)
#define DIERR_INVALIDPARAM E_INVALIDARG
#define DIERR_NOAGGREGATION ((HRESULT)0x80040110L) /* CLASS_E_NOAGGREGATION */
#define DIERR_NOINTERFACE E_NOINTERFACE
#define DIERR_NOTFOUND ((HRESULT)0x80070002L)
#define DIERR_NOTINITIALIZED ((HRESULT)0x80070015L)
#define DIERR_OTHERAPPHASPRIO ((HRESULT)0x80070005L)
#define DIERR_OUTOFMEMORY E_OUTOFMEMORY
#define DIERR_UNSUPPORTED E_NOTIMPL
#define DIERR_OBJECTNOTFOUND ((HRESULT)0x80070002L)
#ifndef E_HANDLE
#define E_HANDLE ((HRESULT)0x80070006L)
#endif
#ifndef E_PENDING
#define E_PENDING ((HRESULT)0x8000000AL)
#endif
#ifndef E_ACCESSDENIED
#define E_ACCESSDENIED ((HRESULT)0x80070005L)
#endif

// ---- device kinds / caps / GetDeviceData flag ----
#define DIGDD_PEEK 0x00000001
#define DIDEVTYPE_JOYSTICK 4
#define DIDC_FORCEFEEDBACK                                                     \
    0x00000100 /* DIDEVCAPS.dwFlags: device supports force feedback */

// ---- more DI properties (MAKEDIPROP tokens: a small int cast to const GUID*, matched by pointer identity) ----
#define DIPROP_AUTOCENTER MAKEDIPROP(9)
#define DIPROP_CPOINTS MAKEDIPROP(20)
#define DIPROPAUTOCENTER_OFF 0
#define DIPROPAUTOCENTER_ON 1

// ---- mouse axis/button offsets within DIMOUSESTATE (see DIJOFS_* note above re offsetof under LP64) ----
#define DIMOFS_X FIELD_OFFSET(DIMOUSESTATE, lX)
#define DIMOFS_Y FIELD_OFFSET(DIMOUSESTATE, lY)
#define DIMOFS_Z FIELD_OFFSET(DIMOUSESTATE, lZ)
#define DIMOFS_BUTTON0 FIELD_OFFSET(DIMOUSESTATE, rgbButtons[0])
#define DIMOFS_BUTTON1 FIELD_OFFSET(DIMOUSESTATE, rgbButtons[1])
#define DIMOFS_BUTTON2 FIELD_OFFSET(DIMOUSESTATE, rgbButtons[2])
#define DIMOFS_BUTTON3 FIELD_OFFSET(DIMOUSESTATE, rgbButtons[3])

// ---- force-feedback effect vocabulary (sijoy.cpp) --------------------------------------------------------------
#define DIEFT_ALL 0x00000000
#define DIEFT_CONSTANTFORCE 0x00000001
#define DIEFT_RAMPFORCE 0x00000002
#define DIEFT_PERIODIC 0x00000003
#define DIEFT_CONDITION 0x00000004
#define DIEFT_CUSTOMFORCE 0x00000005
#define DIEFT_HARDWARE 0x000000FF
#define DIEFT_GETTYPE(n) LOBYTE(n)
#define DIEFF_OBJECTIDS 0x00000001
#define DIEFF_OBJECTOFFSETS 0x00000002
#define DIEFF_CARTESIAN 0x00000010
#define DIEFF_POLAR 0x00000020
#define DIEFF_SPHERICAL 0x00000040
#define DIEP_DURATION 0x00000001
#define DIEP_SAMPLEPERIOD 0x00000002
#define DIEP_GAIN 0x00000004
#define DIEP_TRIGGERBUTTON 0x00000008
#define DIEP_TRIGGERREPEATINTERVAL 0x00000010
#define DIEP_AXES 0x00000020
#define DIEP_DIRECTION 0x00000040
#define DIEP_ENVELOPE 0x00000080
#define DIEP_TYPESPECIFICPARAMS 0x00000100
#define DIEP_STARTDELAY 0x00000200
#define DIEP_ALLPARAMS 0x000003FF
#define DIES_SOLO 0x00000001
#define DIEB_NOTRIGGER 0xFFFFFFFF
#define INFINITE_FF 0xFFFFFFFF

typedef struct DIENVELOPE
{
    DWORD dwSize;
    DWORD dwAttackLevel, dwAttackTime, dwFadeLevel, dwFadeTime;
} DIENVELOPE, *LPDIENVELOPE;
typedef struct DICONSTANTFORCE
{
    LONG lMagnitude;
} DICONSTANTFORCE, *LPDICONSTANTFORCE;
typedef struct DIRAMPFORCE
{
    LONG lStart, lEnd;
} DIRAMPFORCE, *LPDIRAMPFORCE;
typedef struct DIPERIODIC
{
    DWORD dwMagnitude;
    LONG lOffset;
    DWORD dwPhase, dwPeriod;
} DIPERIODIC, *LPDIPERIODIC;
typedef struct DICUSTOMFORCE
{
    DWORD cChannels, dwSamplePeriod, cSamples;
    LONG* rglForceData;
} DICUSTOMFORCE, *LPDICUSTOMFORCE;
typedef struct DICONDITION
{
    LONG lOffset, lPositiveCoefficient, lNegativeCoefficient;
    DWORD dwPositiveSaturation, dwNegativeSaturation;
    LONG lDeadBand;
} DICONDITION, *LPDICONDITION;
typedef struct DIEFFECT
{
    DWORD dwSize, dwFlags, dwDuration, dwSamplePeriod, dwGain, dwTriggerButton,
        dwTriggerRepeatInterval, cAxes;
    DWORD* rgdwAxes;
    LONG* rglDirection;
    LPDIENVELOPE lpEnvelope;
    DWORD cbTypeSpecificParams;
    void* lpvTypeSpecificParams;
    DWORD dwStartDelay;
} DIEFFECT, *LPDIEFFECT;
typedef const DIEFFECT* LPCDIEFFECT;
typedef struct DIEFFECTINFOA
{
    DWORD dwSize;
    GUID guid;
    DWORD dwEffType, dwStaticParams, dwDynamicParams;
    char tszName[MAX_PATH];
} DIEFFECTINFOA, *LPDIEFFECTINFOA;
typedef const DIEFFECTINFOA* LPCDIEFFECTINFOA;
#define DIEFFECTINFO DIEFFECTINFOA
#define LPDIEFFECTINFO LPDIEFFECTINFOA
#define LPCDIEFFECTINFO LPCDIEFFECTINFOA

// data-format + well-known GUIDs (defined in ff_dinput.cpp so the addresses are unique)
extern "C" DIDATAFORMAT c_dfDIJoystick2, c_dfDIJoystick, c_dfDIMouse,
    c_dfDIMouse2, c_dfDIKeyboard;
extern "C" const GUID GUID_SysMouse, GUID_SysKeyboard, GUID_XAxis, GUID_YAxis,
    GUID_ZAxis, GUID_RxAxis, GUID_RyAxis, GUID_RzAxis, GUID_Slider, GUID_POV,
    GUID_Button;
// force-feedback effect-type GUIDs (sijoy.cpp selects effects by these)
extern "C" const GUID GUID_ConstantForce, GUID_RampForce, GUID_Square,
    GUID_Sine, GUID_Triangle, GUID_SawtoothUp, GUID_SawtoothDown, GUID_Spring,
    GUID_Damper, GUID_Inertia, GUID_Friction, GUID_CustomForce;
// interface IIDs used by the create calls (compared by identity only)
extern "C" const IID IID_IDirectInput8, IID_IDirectInput7,
    IID_IDirectInputDevice8, IID_IDirectInputDevice7;
#ifndef GUID_NULL_DEFINED
#define GUID_NULL_DEFINED
extern "C" const GUID GUID_NULL;
#endif

// callback prototypes (used as function-pointer typedefs in the input headers)
typedef BOOL(CALLBACK* LPDIENUMDEVICESCALLBACKA)(LPCDIDEVICEINSTANCEA, LPVOID);
typedef BOOL(CALLBACK* LPDIENUMDEVICEOBJECTSCALLBACKA)(
    LPCDIDEVICEOBJECTINSTANCEA, LPVOID);
typedef BOOL(CALLBACK* LPDIENUMEFFECTSCALLBACKA)(LPCDIEFFECTINFOA, LPVOID);
#define LPDIENUMDEVICESCALLBACK LPDIENUMDEVICESCALLBACKA
#define LPDIENUMDEVICEOBJECTSCALLBACK LPDIENUMDEVICEOBJECTSCALLBACKA
#define LPDIENUMEFFECTSCALLBACK LPDIENUMEFFECTSCALLBACKA

typedef const DIPROPHEADER* LPCDIPROPHEADER;

// ---- concrete interfaces (methods defined in ff_dinput.cpp on SDL3) --------------------------------------------
// Plain (non-virtual) member functions: this is one process, one implementation -- no COM vtable/ABI to honour, and
// the engine calls dev->Method() directly. IDirectInputDevice7A is an alias of the 8 flavour so the engine's
// `(LPDIRECTINPUTDEVICE2)dev` casts stay no-ops and Poll()/etc. remain callable.
struct IDirectInputEffect
{
    void* impl;
    ULONG AddRef();
    ULONG Release();
    HRESULT Start(DWORD dwIterations, DWORD dwFlags);
    HRESULT Stop();
    HRESULT SetParameters(LPCDIEFFECT peff, DWORD dwFlags);
    HRESULT Download();
    HRESULT Unload();
};
typedef IDirectInputEffect* LPDIRECTINPUTEFFECT;

struct IDirectInputDevice8A
{
    void* impl;
    ULONG AddRef();
    ULONG Release();
    HRESULT SetDataFormat(LPCDIDATAFORMAT lpdf);
    HRESULT SetCooperativeLevel(HWND hwnd, DWORD dwFlags);
    HRESULT SetEventNotification(HANDLE hEvent);
    HRESULT SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pdiph);
    HRESULT GetProperty(REFGUID rguidProp, LPDIPROPHEADER pdiph);
    HRESULT Acquire();
    HRESULT Unacquire();
    HRESULT Poll();
    HRESULT GetDeviceState(DWORD cbData, LPVOID lpvData);
    HRESULT GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod,
                          LPDWORD pdwInOut, DWORD dwFlags);
    HRESULT GetCapabilities(LPDIDEVCAPS lpDIDevCaps);
    HRESULT GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj,
                          DWORD dwHow);
    HRESULT GetDeviceInfo(LPDIDEVICEINSTANCEA pdidi);
    HRESULT EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback, LPVOID pvRef,
                        DWORD dwFlags);
    HRESULT EnumEffects(LPDIENUMEFFECTSCALLBACKA lpCallback, LPVOID pvRef,
                        DWORD dwEffType);
    HRESULT CreateEffect(REFGUID rguid, LPCDIEFFECT lpeff,
                         LPDIRECTINPUTEFFECT* ppdeff, LPUNKNOWN punkOuter);
};
typedef IDirectInputDevice8A IDirectInputDevice7A;
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE8;
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE7;
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE2;
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE;

struct IDirectInput8A
{
    void* impl;
    ULONG AddRef();
    ULONG Release();
    HRESULT CreateDevice(REFGUID rguid,
                         LPDIRECTINPUTDEVICE8* lplpDirectInputDevice,
                         LPUNKNOWN pUnkOuter);
    HRESULT CreateDeviceEx(REFGUID rguid, REFIID riid, void** ppvOut,
                           LPUNKNOWN pUnkOuter);
    HRESULT EnumDevices(DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback,
                        LPVOID pvRef, DWORD dwFlags);
    HRESULT GetDeviceStatus(REFGUID rguidInstance);
};
typedef IDirectInput8A IDirectInput7A;
typedef IDirectInput8A* LPDIRECTINPUT8;
typedef IDirectInput8A* LPDIRECTINPUT8A;
typedef IDirectInput8A* LPDIRECTINPUT7;
typedef IDirectInput8A* LPDIRECTINPUT;

// factory (siloop.cpp). hInst is ignored on Linux; SDL3 owns device discovery.
extern "C" HRESULT DirectInput8Create(HINSTANCE hinst, DWORD dwVersion,
                                      REFIID riidltf, void** ppvOut,
                                      LPUNKNOWN punkOuter);

#endif // FF_WIN32SHIM_DINPUT_H
