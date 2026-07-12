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
    int xOffset;
    int yOffset;
    int width[NumModes];
    int height[NumModes];
    int depth[NumModes];
    int doubleBuffer[NumModes];
    HWND appWin;
    int windowStyle;
    // Device managment
    DeviceManager devmgr;
    DisplayDevice theDisplayDevice;
    int deviceNumber;
	bool displayFullScreen = true;

    // #33: lightweight windowed/fullscreen switch for the 3D session only. The shared app
    // window is restyled in place (no DestroyWindow/MakeWindow -> avoids the #41 enter/exit
    // hang area) and restored on leaving 3D. The swap chain is NOT recreated; DXGI stretches
    // the back buffer to the client area.
    bool mInSimWinMode = false;
    long mSavedWinStyle = 0;
    RECT mSavedWinRect = { 0, 0, 0, 0 };
    bool mSavedFullScreen = true;

    void Setup(int languageNum);
    void Cleanup();
    void SetSimMode(int width, int height, int depth);
    void EnterMode(DisplayMode newMode, int theDevice = 0, int Driver = 0);
    void LeaveMode();
    void ToggleFullScreen();
    void EnterSimWindowMode(bool windowed); // #33 (call from any thread; marshals to main)
    void LeaveSimWindowMode();              // #33
    void MakeWindow();
    ImageBuffer* GetImageBuffer()
    {
        return theDisplayDevice.GetImageBuffer();
    };

    // OW
protected:
    void _LeaveMode();
    void _EnterMode(DisplayMode newMode, int theDevice = 0, int Driver = 0);
    void _ToggleFullScreen();
    void _EnterSimWindowMode(bool windowed); // #33 (runs on the main thread)
    void _LeaveSimWindowMode();              // #33 (runs on the main thread)

    friend LRESULT CALLBACK SimWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    friend LRESULT CALLBACK FalconMessageHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

extern FalconDisplayConfiguration FalconDisplay;

#endif
