#include "falclib.h"
#include "dispcfg.h"
#include "f4thread.h"

#include "sinput.h"
#include "cpmanager.h"
#include "otwdrive.h"
#include "playerop.h"
#include "simio.h"
#include "dispopts.h"
#include "simdrive.h"
#include "aircrft.h"

#include "mouselook.h"	// Retro 18Jan2004

#pragma warning(push,4)

#ifdef USE_DINPUT_8	// Retro 15Jan2004
#pragma message("______________Compiling with DirectInputVersion 0x0800 !!______________________")
#else
#pragma message("______________Compiling with DirectInputVersion 0x0700 !!______________________")
#endif

// sfr: removed
//BOOL						gWindowActive;
BOOL						gOccupiedBySim;

#ifndef USE_DINPUT_8	// Retro 15Jan2004
LPDIRECTINPUT7				gpDIObject;
LPDIRECTINPUTDEVICE7		gpDIDevice[SIM_NUMDEVICES];
#else
LPDIRECTINPUT8				gpDIObject;
LPDIRECTINPUTDEVICE8		gpDIDevice[SIM_NUMDEVICES];
#endif
HANDLE						gphDeviceEvent[SIM_NUMDEVICES];
BOOL						gpDeviceAcquired[SIM_NUMDEVICES];
BOOL						gSimInputEnabled = FALSE;
BOOL						gDIEnabled = FALSE;
extern int NoRudder;

void SetupInputFunctions (void);
void CleanupInputFunctions (void);

// Callback for joystick enumeration
BOOL FAR PASCAL InitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);
#ifdef USE_DINPUT_8	// Retro 17Jan2004
void CheckForMouseAxis(void);
#endif
void JoystickStopAllEffects (void);

DWORD nextJoyRead = 0;
DWORD minJoyReadInterval = 84; //12 times pre second

extern unsigned int NumberOfPOVs;	// Retro 26Dec2003

// Retro 31Dec2003
extern AxisMapping AxisMap;
extern GameAxisSetup_t AxisSetup[AXIS_MAX];
extern AxisIDStuff DIAxisNames[SIM_NUMDEVICES*8];		/* '8' is defined by dinput: 8 axis maximum per device */

//*******************************************
//	void InputCycle()
// One iteration of the input loop.  This
// may now poke directly into Sim/Graphics
// state, so it _must_ be run on the Sim
// thread.
//*******************************************
void InputCycle(void)
{
	DWORD	dw;
	HWND	hWnd;
	
	hWnd = FalconDisplay.appWin;
	
	// sfr: removed
	//if (gWindowActive){
		//check for focus
		// Always be sure we have these devices acquired if we have focus 
		//AcquireDeviceInput(gCurController, TRUE);
		// sfr: commented here
		//AcquireDeviceInput(SIM_MOUSE, TRUE);
		if(!CheckDeviceAcquisition(SIM_KEYBOARD)){
			AcquireDeviceInput(SIM_KEYBOARD, TRUE);
		}
		if(!CheckDeviceAcquisition(SIM_MOUSE)){
			AcquireDeviceInput(SIM_MOUSE, TRUE);
		}
	//}
	
	
	// Went to Wait for single to make sure both keyboard and mouse are processed
	// each time, instead of one or the other
	//dw = MsgWaitForMultipleObjects(SIM_NUMDEVICES - 1, gphDeviceEvent, 0, 0, QS_ALLINPUT); //VWF Kludge until DINPUT supports joysticks

	dw = WaitForSingleObject(gphDeviceEvent[SIM_MOUSE], 0);
	if(dw == WAIT_OBJECT_0){
		OnSimMouseInput(FalconDisplay.appWin);
	}
	
	dw = WaitForSingleObject(gphDeviceEvent[SIM_KEYBOARD], 0);
	if(dw == WAIT_OBJECT_0){
		OnSimKeyboardInput();
	}

	//We need to simply read the joystick every time through
	//using a minimum read interval for faster computers
	//if(GetTickCount() >= nextJoyRead)
	{
		GetJoystickInput();
		ProcessJoyButtonAndPOVHat();
		nextJoyRead = GetTickCount() +  minJoyReadInterval;
	}
}

//*******************************************
//	void NoInputCycle()
// One iteration of the input loop.  This
// may now poke directly into Sim/Graphics
// state, so it _must_ be run on the Sim
// thread. - simulates no inputs
//*******************************************
void NoInputCycle(void)
{
#if 0	// Retro 10Jan2004
	unsigned int i;

	/* here the THROTTLE is also set to 0, dunno if this is correct or not */
	for (i = 0; i < SIMLIB_MAX_ANALOG; i++)
	{
		IO.analog[i].engrValue = 0.0f;
	}
	
	for(i=0;i<SIMLIB_MAX_DIGITAL*SIM_NUMDEVICES;i++)
	{
		IO.digital[i] = FALSE;
	}

	for (i = 0; i < NumberOfPOVs; i++)	// Retro 26Dec2003
	{
		IO.povHatAngle[i] = (unsigned long)-1;	
	}
#else
	IO.ResetAllInputs();	// Retro 10Jan2004
#endif
}

//***********************************
//	BOOL SetupDIMouseAndKeyboard()
//***********************************
BOOL SetupDIMouseAndKeyboard(HINSTANCE, HWND hWnd){
	
	BOOL	SetupResult = TRUE;
	BOOL	MouseSetupResult;
	BOOL	KeyboardSetupResult;
	BOOL	CursorSetupResult;

	if(!gDIEnabled)
	{
		ShiAssert(gDIEnabled != FALSE);
		return FALSE;
	}
	
	gOccupiedBySim		= TRUE;
	gyFuzz				= gxFuzz = 0;
	gxPos					= DisplayOptions.DispWidth/2;
	gyPos					= DisplayOptions.DispHeight/2;
	gMouseSensitivity	= NORM_SENSITIVITY;
	// sfr: removed
	//gWindowActive		= TRUE;
	
	// Register with DirectInput and get an IDirectInput to play with
	
	DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE}, DINPUT_BUFFERSIZE};
	
	//sfr: mouse grab
	// we use exlusive only if touch buddy disabled
	MouseSetupResult	= SetupDIDevice(hWnd, (PlayerOptions.GetTouchBuddy() == false), SIM_MOUSE, GUID_SysMouse, &c_dfDIMouse, &dipdw); // Mouse
//	KeyboardSetupResult	= SetupDIDevice(hWnd, FALSE, SIM_KEYBOARD, GUID_SysKeyboard, &c_dfDIKeyboard, &dipdw); // Keyboard
	// Retro 25 Nov 2003 - this forces foreground/exclusive for the keyboard and makes it possible to use the keyboard while
	// falcon is running in a background window
	KeyboardSetupResult	= SetupDIDevice(hWnd, TRUE, SIM_KEYBOARD, GUID_SysKeyboard, &c_dfDIKeyboard, &dipdw); // Keyboard
	
	
	CursorSetupResult		= CreateSimCursors();
	
	// JB 010618 Disable these messages as they don't appear to do anything useful
	/*
	if(!KeyboardSetupResult){
		DIMessageBox(999, MB_OK, SSI_NO_KEYBOARD_INIT);
		SetupResult = FALSE;
	}
	
	if(SetupResult && !MouseSetupResult){
		SetupResult = DIMessageBox(999, MB_YESNO, SSI_NO_MOUSE_INIT);
	}
	else if (MouseSetupResult)
	{
		//      ShowCursor(FALSE);
	}
*/	
	
	if(SetupResult && !CursorSetupResult){
		SetupResult = DIMessageBox(999, MB_YESNO, SSI_NO_CURSOR_INIT);
	}
	
	if(SetupResult){
		gSelectedCursor = 0; //VWF Kludge for now
	}
	
	if(SetupResult){
		gSimInputEnabled = TRUE;
	}
	else{
		gSimInputEnabled = FALSE;
	}
	
	SetupInputFunctions ();
	
	gTimeLastMouseMove = vuxRealTime;
	gTimeLastCursorUpdate = vuxRealTime;		//Wombat778 1-24-04
	
	return (SetupResult);
}

/*****************************************************************************************/
// This code checks if the axis specified by a user are really existing on the specified
// devices.. however..
// a problem is that the properties (range and deadzone) are set before that check
// I can´t do this check before enumerating the devices however
// I don´t know if I can do the property-setting here either
//
/*****************************************************************************************/
void SetupGameAxis()
{
	DIDEVICEOBJECTINSTANCE devobj;
	HRESULT hres;
	
	devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

	int AxisOffsets[] = {	DIJOFS_X,
							DIJOFS_Y,
							DIJOFS_Z,
							DIJOFS_RX,
							DIJOFS_RY,
							DIJOFS_RZ,
							DIJOFS_SLIDER(0),
							DIJOFS_SLIDER(1)};

	for (int i = AXIS_START; i < AXIS_MAX; i++)
		IO.SetAnalogIsUsed((GameAxis_t)i,false);

	// loop through all enumerated DEVICES
	for (int DeviceIndex = SIM_JOYSTICK1; DeviceIndex < gTotalJoy + SIM_JOYSTICK1; DeviceIndex++)
	{
		// loop through all our game-axis that have to be mapped..
		for (int GameAxisIndex = 0; GameAxisIndex < AXIS_MAX; GameAxisIndex++)
		{
			// if a real device is indeed mapped to a game axis..
			if (*AxisSetup[GameAxisIndex].device == DeviceIndex)
			{
				// look what axis on that real device is mapped to that game axis..
				if ((*AxisSetup[GameAxisIndex].axis != -1)&&(*AxisSetup[GameAxisIndex].axis < 8))	// 8 is again the max DX axiscount..
				{
					// ok there´s one mapped. now see if it is indeed located on the device..
					if (gpDIDevice[DeviceIndex]->GetObjectInfo(&devobj, AxisOffsets[*AxisSetup[GameAxisIndex].axis], DIPH_BYOFFSET) == DI_OK)
					{
						// found it :) now set it up..

						// apply range. bipolar is -10000...10000, unipolar is 0...15000
						DIPROPRANGE				diprg;
						diprg.diph.dwSize       = sizeof(diprg);
						diprg.diph.dwHeaderSize = sizeof(diprg.diph);
						diprg.diph.dwObj        = AxisOffsets[*AxisSetup[GameAxisIndex].axis];
						diprg.diph.dwHow        = DIPH_BYOFFSET;
						if (AxisSetup[GameAxisIndex].isUniPolar == false)
						{
							diprg.lMin              = -10000;
							diprg.lMax              = +10000;
						}
						else
						{
							diprg.lMin              = 0;
							diprg.lMax              = 15000;
						}

						hres = gpDIDevice[DeviceIndex]->Unacquire();	// Retro 31Dec2003 - have to unacquire in order to change props

						hres = gpDIDevice[DeviceIndex]->SetProperty( DIPROP_RANGE, &diprg.diph);
						ShiAssert(hres == DI_OK);


						/*****************************************************************************/
						//	Retro 23Jan2004
						//	Custom axis shaping.. setting the CPOINTS correctly is done by the outside
						//	program.
						/*****************************************************************************/
						if ((PlayerOptions.GetAxisShaping() == true)&&(AxisShapes.active[GameAxisIndex] == true))
						{
							DIPROPCPOINTS dipcp;

							dipcp = AxisShapes.CalibPoints[GameAxisIndex];

							// not trusting this struct, I fill in the rest by myself..
							dipcp.diph.dwSize = sizeof(DIPROPCPOINTS);
							dipcp.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
							dipcp.diph.dwObj = AxisOffsets[*AxisSetup[GameAxisIndex].axis]; 
							dipcp.diph.dwHow = DIPH_BYOFFSET; 
							ShiAssert(dipcp.dwCPointsNum);	// if this is 0 nothing happens !

							hres = gpDIDevice[DeviceIndex]->SetProperty(DIPROP_CPOINTS,&dipcp.diph);
							ShiAssert(hres == DI_OK);
						}
						else	// not using custom axis shaping, standard deadzone/saturation zones apply
						{
							// if a deadzone is defined, apply it.. values are from 10000 (100%) to 0 (0%) of
							// physical range to both sides of the '0' point. Default to 100 (1%)
							// unipolar axis don´t have a deadzone !
							if ((AxisSetup[GameAxisIndex].deadzone)&&(*AxisSetup[GameAxisIndex].deadzone))
							{
								DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE},DJOYSTICK_BUFFERSIZE};
								dipdw.dwData = *AxisSetup[GameAxisIndex].deadzone;
								dipdw.diph.dwObj = AxisOffsets[*AxisSetup[GameAxisIndex].axis];
								dipdw.diph.dwHow = DIPH_BYOFFSET;
								
								hres = gpDIDevice[DeviceIndex]->SetProperty(DIPROP_DEADZONE, &dipdw.diph);
								ShiAssert(hres == DI_OK);
							}

							// Retro 2Jan2004
							// This code enables a saturation zone for ALL axis on ALL devices. Should be used with sticks
							// that for some reason can not reach their extreme values.
							if (*AxisSetup[GameAxisIndex].saturation != -1)
							{
								DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE},DJOYSTICK_BUFFERSIZE};
								dipdw.dwData = *AxisSetup[GameAxisIndex].saturation;
								dipdw.diph.dwObj = AxisOffsets[*AxisSetup[GameAxisIndex].axis];
								dipdw.diph.dwHow = DIPH_BYOFFSET;
								
								hres = gpDIDevice[DeviceIndex]->SetProperty(DIPROP_SATURATION, &dipdw.diph);
								ShiAssert(hres == DI_OK);
							}
						}	// no custom axis shaping


						// tell the program that it´s done.
						IO.SetAnalogIsUsed((GameAxis_t)GameAxisIndex, true);
					}
#pragma warning(disable:4127)	// Getting rid of "conditional expression is constant"
					else
					{
						ShiAssert(false);	// did not find a specified axis on the specified device
					}
				}
				else
				{
					ShiAssert(false);	// out-of-array ! DI only specifies 8 axis (0-7) !!
				}
#pragma warning(default:4127)
			}
		}
	}

	// Retro 17Jan2004 - Handling the MouseWheel (ugh)
	if (IO.MouseWheelExists() == true)
	{
		theMouseWheelAxis.SetWheelInactive();	// Retro 21Jan2004

		for (int GameAxisIndex = 0; GameAxisIndex < AXIS_MAX; GameAxisIndex++)
		{
			if ((*AxisSetup[GameAxisIndex].device) == SIM_MOUSE)
			{
				ShiAssert((*AxisSetup[GameAxisIndex].axis) == DX_MOUSEWHEEL);
				IO.SetAnalogIsUsed((GameAxis_t)GameAxisIndex, true);

				// Retro 18Jan2004
				theMouseWheelAxis.SetAxis((GameAxis_t)GameAxisIndex);
			}
		}
	}
}

/*******************************************************************************/
//	Retro 6Jan2004
//	This (not too elegant) routine checks if the devices specified in the 
//	AxisMap structures are indeed found in the array of DIDevices. Executed at
//	startup, if a device is not found the return value is false and all
//	axis settings will be reset to keyboard, prompting the user to reset his
//	axis.
/*******************************************************************************/
bool CheckDeviceArray()
{
	bool retval = true;

	if (AxisMap.Pitch.Device  != -1)
		if (!gpDIDevice[AxisMap.Pitch.Device])
			retval = false;
	if (AxisMap.Bank.Device  != -1)
		if (!gpDIDevice[AxisMap.Bank.Device])
			retval = false;
	if (AxisMap.Yaw.Device  != -1)
		if (!gpDIDevice[AxisMap.Yaw.Device])
			retval = false;
	if (AxisMap.Throttle.Device  != -1)
		if (!gpDIDevice[AxisMap.Throttle.Device])
			retval = false;
	if (AxisMap.Throttle2.Device  != -1)
		if (!gpDIDevice[AxisMap.Throttle2.Device])
			retval = false;
	if (AxisMap.BrakeLeft.Device  != -1)
		if (!gpDIDevice[AxisMap.BrakeLeft.Device])
			retval = false;
	if (AxisMap.BrakeRight.Device  != -1)
		if (!gpDIDevice[AxisMap.BrakeRight.Device])
			retval = false;
	if (AxisMap.FOV.Device  != -1)
		if (!gpDIDevice[AxisMap.FOV.Device])
			retval = false;
	if (AxisMap.PitchTrim.Device  != -1)
		if (!gpDIDevice[AxisMap.PitchTrim.Device])
			retval = false;
	if (AxisMap.YawTrim.Device  != -1)
		if (!gpDIDevice[AxisMap.YawTrim.Device])
			retval = false;
	if (AxisMap.BankTrim.Device  != -1)
		if (!gpDIDevice[AxisMap.BankTrim.Device])
			retval = false;
	if (AxisMap.AntElev.Device  != -1)
		if (!gpDIDevice[AxisMap.AntElev.Device])
			retval = false;
	if (AxisMap.RngKnob.Device  != -1)
		if (!gpDIDevice[AxisMap.RngKnob.Device])
			retval = false;
	if (AxisMap.CursorX.Device  != -1)
		if (!gpDIDevice[AxisMap.CursorX.Device])
			retval = false;
	if (AxisMap.CursorY.Device  != -1)
		if (!gpDIDevice[AxisMap.CursorY.Device])
			retval = false;
	if (AxisMap.Comm1Vol.Device  != -1)
		if (!gpDIDevice[AxisMap.Comm1Vol.Device])
			retval = false;
	if (AxisMap.Comm2Vol.Device  != -1)
		if (!gpDIDevice[AxisMap.Comm2Vol.Device])
			retval = false;
	if (AxisMap.MSLVol.Device  != -1)
		if (!gpDIDevice[AxisMap.MSLVol.Device])
			retval = false;
	if (AxisMap.ThreatVol.Device  != -1)
		if (!gpDIDevice[AxisMap.ThreatVol.Device])
			retval = false;
	if (AxisMap.HudBrt.Device  != -1)
		if (!gpDIDevice[AxisMap.HudBrt.Device])
			retval = false;
	if (AxisMap.RetDepr.Device  != -1)
		if (!gpDIDevice[AxisMap.RetDepr.Device])
			retval = false;
	if (AxisMap.Zoom.Device != -1)
		if (!gpDIDevice[AxisMap.Zoom.Device])
			retval = false;

	return retval;
}
//***********************************
//	BOOL SetupDIJoystick()
//
//	Creates the DI interface, enumerates
//	devices, loads FFB effects and the
//	works..
//***********************************
#ifndef USE_DINPUT_8	// Retro 15Jan2004
BOOL SetupDIJoystick(HINSTANCE hInst, HWND hWnd)
#else
BOOL SetupDIJoystick(HINSTANCE, HWND hWnd)
#endif
{
	BOOL	JoystickSetupResult;
	HRESULT hres;

	/*******************************************************************************/
	//	Load the current real-device(-axis) to in-game-axis mapping
	//	returns FALSE on error, this should maybe pop up an error msg box..
	/*******************************************************************************/
#ifndef NDEBUG
	//int result = IO.ReadAxisMappingFile();	 // Retro 31Dec2003
//	ShiAssert(result != 0);
#else
	IO.ReadAxisMappingFile();	 // Retro 31Dec2003
#endif

	/*******************************************************************************/
	//	Create our interface to DInput7/8..
	/*******************************************************************************/
#ifndef USE_DINPUT_8	// Retro 15Jan2004
	gDIEnabled = VerifyResult(DirectInputCreateEx(hInst, DIRECTINPUT_VERSION, IID_IDirectInput7, (void **) &gpDIObject, NULL));
#else
	hres = DirectInput8Create(GetModuleHandle(NULL),DIRECTINPUT_VERSION,IID_IDirectInput8,(void **)&gpDIObject, NULL);
	gDIEnabled = (hres == DI_OK)?TRUE:FALSE;
#endif
	if(!gDIEnabled)
		return gDIEnabled;

	/*******************************************************************************/
	//	Enumerate all available (attached) joysticks.. the enum routine itself
	//	also calls up all kinds of stuff..
	/*******************************************************************************/
#ifndef USE_DINPUT_8	// Retro 15Jan2004
	hres = gpDIObject->EnumDevices(DIDEVTYPE_JOYSTICK,InitJoystick, &hWnd, DIEDFL_ATTACHEDONLY);
#else
	hres = gpDIObject->EnumDevices(DI8DEVCLASS_GAMECTRL,InitJoystick, &hWnd, DIEDFL_ATTACHEDONLY);
#endif

	/*******************************************************************************/
	//	ok we enumerated sticks, this doesn´t mean however that they are mapped yet !!
	/*******************************************************************************/
	if(gTotalJoy)
	{
#ifdef USE_DINPUT_8
		CheckForMouseAxis();	// Retro 17Jan2004
#endif
		/*******************************************************************************/
		// Some sanity checks..
		// if the number of enumerated devices is the same as when mapping, and if
		// a flight control device is specified, and if all other specified devices were
		// indeed enumerated, AND the GUI of the primary control stick did not change,
		// configure them. Else everything is set to keyboard and the user has to
		// reconfigure.
		/*******************************************************************************/
		if ((AxisMap.totalDeviceCount == gTotalJoy)&&(AxisMap.FlightControlDevice != -1)/*&&(CheckDeviceArray() == true)*/)
		{
			DIDEVICEINSTANCE devinst;
			devinst.dwSize = sizeof(DIDEVICEINSTANCE);

			hres = gpDIDevice[AxisMap.FlightControlDevice]->GetDeviceInfo(&devinst);
			if (!memcmp(&AxisMap.FlightControllerGUID , &devinst.guidInstance,sizeof(GUID)))
			{
				BOOL result;
				// wohoo.. user changed nothing !
				result = IO.ReadFile();	// To get info about any center (or ABDetent) offsets - 
								// this (and the 'isReversed' info are the only things that are effectively read there

				ShiAssert(result == TRUE);

				if (PlayerOptions.GetAxisShaping() == true)
				{
					result = IO.LoadAxisCalibrationFile();
					ShiAssert(result == TRUE);

					if (result == FALSE)
						PlayerOptions.SetAxisShaping(false);
				}

				SetupGameAxis();	// set axis properties according to saves values

				DIDEVCAPS CurJoyCaps;						// Retro 26Dec2003
				CurJoyCaps.dwSize = sizeof(DIDEVCAPS);		// Retro 26Dec2003

				hres = gpDIDevice[AxisMap.FlightControlDevice]->GetCapabilities(&CurJoyCaps);

				//NumberOfPOVs = (CurJoyCaps.dwPOVs>0)?1:0;	// Retro 26Dec2003 - either 0 or 1 POV hat
				NumberOfPOVs = CurJoyCaps.dwPOVs;			// Wombat778 4-27-04 Dont limit to 1 POV
			}
			else
			{
				IO.Reset();	// Retro 11Jan2004 - nuke the axismaps struct
				PlayerOptions.SetFFB(false);
			}
		}
		else
		{
			// nothing happens, as obviously the number/nature of enumerated devices has changed.
			IO.Reset();	// Retro 11Jan2004 - nuke the axismaps struct
			PlayerOptions.SetFFB(false);
		}

		/* however we still acquire everything we´ve got in order to poll it in the setup screen.. */
		for (int i = SIM_JOYSTICK1; i < gTotalJoy + SIM_JOYSTICK1; i++)
		{
			JoystickSetupResult = VerifyResult(gpDIDevice[i]->Acquire());
			gpDeviceAcquired[i] = JoystickSetupResult;
		}
	}
	else
	{
		MonoPrint("No Joysticks found. It would be useful to have one!! :)\n");
	}

	gDIEnabled = TRUE;

	return gDIEnabled;

}

//**********************************************************
//	BOOL CleanupDIMouseAndKeyboard()
// Closes all input devices and terminates the input thread.
//**********************************************************
BOOL CleanupDIMouseAndKeyboard()
{
	BOOL CleanupResult = TRUE;

	AcquireDeviceInput(SIM_MOUSE, FALSE);
	AcquireDeviceInput(SIM_KEYBOARD, FALSE);

	CleanupResult = CleanupResult && CleanupDIDevice(SIM_MOUSE);
	CleanupResult = CleanupResult && CleanupDIDevice(SIM_KEYBOARD);

	CleanupSimCursors();

	gOccupiedBySim = FALSE;
	gSimInputEnabled = FALSE;

	//	ShowCursor(TRUE);

	//CleanupInputFunctions();
	return(CleanupResult);
}

//**********************************************************
//	BOOL CleanupDIJoystick()
// Closes all input devices and terminates the input thread.
//**********************************************************
BOOL CleanupDIJoystick(void)
{
	BOOL CleanupResult = TRUE;

	// have to release the ffb effects and restore autocenter before cleaning up the rest
	if (AxisMap.FlightControlDevice != -1)
	{
		DIDEVCAPS devcaps;
		devcaps.dwSize = sizeof(DIDEVCAPS);

		HRESULT hr = gpDIDevice[AxisMap.FlightControlDevice]->GetCapabilities(&devcaps);

		if(devcaps.dwFlags & DIDC_FORCEFEEDBACK)
		{
			DIPROPDWORD DIPropAutoCenter;

			JoystickStopAllEffects();
			JoystickReleaseEffects();

			// have to release stick in order to change props
			AcquireDeviceInput(AxisMap.FlightControlDevice, FALSE);
			// Reenable stock auto-center
			DIPropAutoCenter.diph.dwSize = sizeof(DIPropAutoCenter);
			DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			DIPropAutoCenter.diph.dwObj = 0;
			DIPropAutoCenter.diph.dwHow = DIPH_DEVICE;
			DIPropAutoCenter.dwData = DIPROPAUTOCENTER_ON;

			hr = gpDIDevice[AxisMap.FlightControlDevice]->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph);
		}
	}
	// ok now nuke the rest

	while(gTotalJoy > 0)
	{
		CleanupResult = CleanupResult && CleanupDIDevice(SIM_JOYSTICK1 + gTotalJoy - 1);
		delete gDIDevNames[SIM_JOYSTICK1 + gTotalJoy - 1];
		gDIDevNames[SIM_JOYSTICK1 + gTotalJoy - 1] = NULL;
		gTotalJoy--;
	}

	gOccupiedBySim = FALSE;
	gSimInputEnabled = FALSE;

	return(CleanupResult);
}

//**********************************************************
//	Retro 14Feb2004 - my own version of the above, which
//	does not go into 100.000 subroutines to do its thing..
//**********************************************************
BOOL CleanupDIJoystickMk2(void)
{
	BOOL CleanupResult = TRUE;

	// have to release the ffb effects and restore autocenter before cleaning up the rest
	if (AxisMap.FlightControlDevice != -1)
	{
		DIDEVCAPS devcaps;
		devcaps.dwSize = sizeof(DIDEVCAPS);

		if (gpDIDevice[AxisMap.FlightControlDevice] != NULL)	// Retro 7May2004
		{
			HRESULT hr = gpDIDevice[AxisMap.FlightControlDevice]->GetCapabilities(&devcaps);

			if(devcaps.dwFlags & DIDC_FORCEFEEDBACK)
			{
				DIPROPDWORD DIPropAutoCenter;

				JoystickStopAllEffects();
				JoystickReleaseEffects();

				// have to release stick in order to change props
				AcquireDeviceInput(AxisMap.FlightControlDevice, FALSE);
				// Reenable stock auto-center
				DIPropAutoCenter.diph.dwSize = sizeof(DIPropAutoCenter);
				DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
				DIPropAutoCenter.diph.dwObj = 0;
				DIPropAutoCenter.diph.dwHow = DIPH_DEVICE;
				DIPropAutoCenter.dwData = DIPROPAUTOCENTER_ON;

				hr = gpDIDevice[AxisMap.FlightControlDevice]->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph);
			}
		}
	}
	// ok now nuke the rest

	while(gTotalJoy > 0)
	{
//		CleanupResult = CleanupResult && CleanupDIDevice(SIM_JOYSTICK1 + gTotalJoy - 1);
		if (gpDIDevice[SIM_JOYSTICK1 + gTotalJoy -1])
		{
			HRESULT hr = gpDIDevice[SIM_JOYSTICK1 + gTotalJoy -1]->Unacquire();
			hr = gpDIDevice[SIM_JOYSTICK1 + gTotalJoy -1]->Release();
			gpDIDevice[SIM_JOYSTICK1 + gTotalJoy -1] = 0;
			delete gDIDevNames[SIM_JOYSTICK1 + gTotalJoy - 1];
			gDIDevNames[SIM_JOYSTICK1 + gTotalJoy - 1] = 0;
		}
		gTotalJoy--;
	}

	gOccupiedBySim = FALSE;
	gSimInputEnabled = FALSE;

	return(CleanupResult);
}

/*****************************************************************************/
//	
/*****************************************************************************/
void CleanupDIAll(void)
{
//    CleanupDIMouseAndKeyboard();
#if 0						// Retro 14Feb2004
	CleanupDIJoystick();
#else
	CleanupDIJoystickMk2();	// Retro 14Feb2004
#endif

	// Retro 31Dec2003
	for (int i = 0; i < SIM_NUMDEVICES*8; i++)
	{
		free(DIAxisNames[i].DXAxisName);
		DIAxisNames[i].DXAxisName = 0;
	}

	if (gpDIObject)
	{
		gpDIObject->Release();
		gpDIObject = NULL;
	}
}
#pragma warning(pop)