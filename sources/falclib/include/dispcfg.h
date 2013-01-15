#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "Graphics/Include/devmgr.h"
#include "Graphics/Include/device.h"

class FalconDisplayConfiguration
{
	public:
	FalconDisplayConfiguration();
	~FalconDisplayConfiguration();

	enum DisplayMode {Movie, UI, UILarge, Planner, Layout, Sim, NumModes};
	DisplayMode currentMode;
	int	xOffset;
	int	yOffset;
	int	width[NumModes];
	int	height[NumModes];
	int	depth[NumModes];
	int	doubleBuffer[NumModes];
	HWND appWin;
	int	windowStyle;
	// Device managment
	DeviceManager	devmgr;
	DisplayDevice	theDisplayDevice;
	int	deviceNumber;
	int	displayFullScreen;

	void Setup( int languageNum );
	void Cleanup();
	void SetSimMode(int width, int height, int depth);
	void EnterMode (DisplayMode newMode, int theDevice = 0,int Driver = 0);
	void LeaveMode();
	void ToggleFullScreen();
	void MakeWindow();
	ImageBuffer* GetImageBuffer() {return theDisplayDevice.GetImageBuffer();};

	// OW
	protected:
	void _LeaveMode();
	void _EnterMode(DisplayMode newMode, int theDevice = 0,int Driver = 0);
	void _ToggleFullScreen();

	friend LRESULT CALLBACK SimWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	friend LRESULT CALLBACK FalconMessageHandler (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

extern FalconDisplayConfiguration FalconDisplay;

#endif
