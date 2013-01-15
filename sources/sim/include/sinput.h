#ifndef _SINPUT_H
#define _SINPUT_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include <tchar.h>

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK

#define USE_DINPUT_8
#ifndef USE_DINPUT_8	// Retro 15Jan2004
#define DIRECTINPUT_VERSION 0x0700
#else
#define DIRECTINPUT_VERSION 0x0800
#endif
#include "dinput.h"

#include "tchar.h"
#include "stdhdr.h"
#include "Graphics\Include\grtypes.h"
#include "Graphics\Include\imagebuf.h"
#include "vu2.h"

#define SI_MOUSE_TIME_DELTA 1500 //in ms

//directional cursors
#define DEFAULT_CURSOR 0
#define N_CURSOR	1
#define NE_CURSOR 2
#define E_CURSOR	3
#define SE_CURSOR 4
#define S_CURSOR	5
#define SW_CURSOR 6
#define W_CURSOR	7
#define NW_CURSOR	8

#define SIM_CURSOR_FILE						"6_cursor.dat"
#define SIM_CURSOR_DIR						"\\art\\ckptart\\"

typedef struct {
	UInt16			Width;
	UInt16			Height;
	UInt16			xHotspot;
	UInt16			yHotspot;
	ImageBuffer*	CursorBuffer;
	BYTE*			CursorRenderBuffer;	//Wombat778 3-24-04
	std::vector<TextureHandle *> CursorRenderTexture;
	PaletteHandle *CursorRenderPalette;
} SimCursor;

#define SDIERR_INVALIDPARAM				"SDIERR_INVALIDPARAM"
#define SDIERR_OUTOFMEMORY					"SDIERR_OUTOFMEMORY"
#define SDIERR_OLDDIRECTINPUTVERSION	"SDIERR_OLDDIRECTINPUTVERSION"
#define SDIERR_BETADIRECTINPUTVERSION	"SDIERR_BETADIRECTINPUTVERSION"
#define SDIERR_NOINTERFACE					"SDIERR_NOINTERFACE"
#define SDIERR_DEVICENOTREG				"SDIERR_DEVICENOTREG"
#define SDIERR_ACQUIRED						"SDIERR_ACQUIRED"
#define SDIERR_HANDLEEXISTS				"SDIERR_HANDLEEXISTS"
#define SDI_PROPNOEFFECT					"SDI_PROPNOEFFECT"	
#define SDIERR_OBJECTNOTFOUND				"SDIERR_OBJECTNOTFOUND"
#define SDIERR_UNSUPPORTED					"SDIERR_UNSUPPORTED"
#define SDIERR_OTHERAPPHASPRIO				"SDIERR_OTHERAPPHASPRIO"
		
#define SSI_GENERAL							"General SIM Input Error"
#define SSI_NO_DI_INIT						"Unable to Create Direct Input Object, Cannot Continue!"
#define SSI_NO_MOUSE_INIT					"Unable to Initialize Mouse, Click OK to Continue without Mouse"
#define SSI_NO_JOYSTICK_INIT				"Unable to Initialize Joystick, Click OK to Continue without Joystick"
#define SSI_NO_CURSOR_INIT					"Unable to Load Cursors, Click OK to Continue without Cursors"
#define SSI_NO_KEYBOARD_INIT				"Unable to Initialize Keyboard, Cannot Continue!"

#define DINPUT_BUFFERSIZE	16
//	#define DMOUSE_BUFFERSIZE	16	// Retro 15Feb2004 -aaaargh
#define DMOUSE_BUFFERSIZE	128		// Retro 15Feb2004 - minimum !
#define DKEYBOARD_BUFFERSIZE 256
#define DJOYSTICK_BUFFERSIZE 16

#define SIM_MOUSE		0
#define SIM_KEYBOARD	1
#define SIM_JOYSTICK1	2
#define SIM_NUMDEVICES 16

#define SIMKEY_SHIFTED 256

#define LO_SENSITIVITY		0
#define NORM_SENSITIVITY	1
#define HI_SENSITIVITY		2


#define POV_N		0
#define POV_NE		4500
#define POV_E		9000
#define POV_SE		13500
#define POV_S		18000
#define POV_SW		22500
#define POV_W		27000
#define POV_NW		31500

#define POV_HALF_RANGE 2250


extern int						gMouseSensitivity;
extern int						gxFuzz;
extern int						gyFuzz;
extern int						gxPos;
extern int						gyPos;
extern int						gxLast;
extern int						gyLast;
extern SimCursor*				gpSimCursors;
extern int						gTotalCursors;

#ifndef USE_DINPUT_8	// Retro 15Jan2004
extern LPDIRECTINPUT7			gpDIObject;
extern LPDIRECTINPUTDEVICE7		gpDIDevice[SIM_NUMDEVICES];
#else
extern LPDIRECTINPUT8			gpDIObject;
extern LPDIRECTINPUTDEVICE8		gpDIDevice[SIM_NUMDEVICES];
#endif

extern HANDLE					gphDeviceEvent[SIM_NUMDEVICES];
extern BOOL						gpDeviceAcquired[SIM_NUMDEVICES];
// sfr: no need for this, not used
//extern BOOL						gWindowActive;
extern BOOL						gOccupiedBySim;
extern ImageBuffer*				gpSaveBuffer;
extern int						gSelectedCursor;
extern BOOL						gSimInputEnabled;
extern VU_TIME					gTimeLastMouseMove;
extern VU_TIME					gTimeLastCursorUpdate;			//Wombat778 1-24-04
extern int						gTotalJoy;
extern _TCHAR*					gDIDevNames[SIM_NUMDEVICES - SIM_JOYSTICK1];
extern DIDEVCAPS				gCurJoyCaps;

// Functions called by other modules
BOOL SetupDIJoystick(HINSTANCE hInst, HWND hWnd);
BOOL SetupDIMouseAndKeyboard(HINSTANCE, HWND);
BOOL CleanupDIJoystick(void);
BOOL CleanupDIMouseAndKeyboard(void);
// sfr: touch buddy support
/** stops hardware mouse processing of events */
void SimMouseStopProcessing();
/** resumes hardware mouse processing of events
* @param in x cursor x
* @param in y cursor y
*/
void SimMouseResumeProcessing(const int x, const int y);
// end touch buddy
void CleanupDIAll(void);
void InputCycle(void);
void NoInputCycle(void);
void GetJoystickInput(void);		
float ReadThrottle(void);

// Functions used only used internaly by this module
BOOL SetupDIDevice( HWND, BOOL, int, REFGUID, LPCDIDATAFORMAT, DIPROPDWORD*);
BOOL CleanupDIDevice(int);
void OnSimKeyboardInput(void);
void OnSimMouseInput(HWND);
void ProcessJoyButtonAndPOVHat(void);
void AcquireDeviceInput(int, BOOL);
BOOL CheckDeviceAcquisition(int DeviceIndex);
BOOL CreateSimCursors(void);
void CleanupSimCursors(void);
void UpdateCursorPosition(DWORD, DWORD);
void ClipAndDrawCursor(int, int);
BOOL VerifyResult(HRESULT);
BOOL DIMessageBox(int, int, char*);
void JoystickReleaseEffects (void);

#endif
