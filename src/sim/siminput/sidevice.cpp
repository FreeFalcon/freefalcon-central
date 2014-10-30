#include "F4Thread.h"
#include "sinput.h"
#include "simio.h"

int gTotalJoy = 0;
_TCHAR* gDIDevNames[SIM_NUMDEVICES - SIM_JOYSTICK1] = {NULL};

//********************************************************************
//
// void AcquireDeviceInput()
//
// Acquires and releases control of the mouse to the system.
//
// Must acquire in order to receive mouse data....must unacquire for
//   other programs to receive mouse data since we use exclusive mode.
//*********************************************************************

void AcquireDeviceInput(int DeviceIndex, BOOL Flag)
{
    if (gpDIDevice[DeviceIndex])
    {
        if (Flag)
        {
            gpDeviceAcquired[DeviceIndex] = SUCCEEDED(gpDIDevice[DeviceIndex]->Acquire());
            // MonoPrint("Device Acquired\n");
        }
        else
        {
            gpDeviceAcquired[DeviceIndex] = SUCCEEDED(gpDIDevice[DeviceIndex]->Unacquire());
        }
    }
}

//********************************************************
// BOOL CheckDeviceAcquisition()
// Checks the current acquisition status of the mouse.
//********************************************************

BOOL CheckDeviceAcquisition(int DeviceIndex)
{
    HRESULT hResult;
    DWORD dwElements = INFINITE;

    if (gpDIDevice[DeviceIndex])
    {
        hResult = gpDIDevice[DeviceIndex]->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), NULL, &dwElements, DIGDD_PEEK);

        if (hResult == DIERR_NOTACQUIRED)
        {
            gpDeviceAcquired[DeviceIndex] = FALSE;
            return (FALSE);
        }
    }

    return (TRUE);
}

//***********************************
// BOOL CleanupDIDevice()
//***********************************

BOOL CleanupDIDevice(int DeviceIndex)
{

    BOOL CleanupResult = TRUE;

    if (CheckDeviceAcquisition(DeviceIndex))
    {
        AcquireDeviceInput(DeviceIndex, FALSE);
    }

    if (gpDIDevice[DeviceIndex])
    {

        if (DeviceIndex < SIM_JOYSTICK1)
            CleanupResult = VerifyResult(gpDIDevice[DeviceIndex]->SetEventNotification(NULL));

        if (CleanupResult)
        {
            CleanupResult = VerifyResult(gpDIDevice[DeviceIndex]->Release());
        }

        if (DeviceIndex < SIM_JOYSTICK1)
            if (CleanupResult)
            {
                gpDIDevice[DeviceIndex] = NULL;
                CleanupResult = CloseHandle(gphDeviceEvent[DeviceIndex]);
            }

        if (CleanupResult)
        {
            gphDeviceEvent[DeviceIndex] = NULL;
        }
    }
    else
    {
        CleanupResult = TRUE;
    }

    return (CleanupResult);
}

//***********************************
// BOOL SetupDIDevice()
//***********************************

BOOL SetupDIDevice(HWND hWnd, BOOL Exclusive, int DeviceIndex,
                   REFGUID pGUID, LPCDIDATAFORMAT pFormat, DIPROPDWORD* pDIPDW)
{

    BOOL SetupResult = TRUE;
    DWORD CooperationFlags = DISCL_FOREGROUND;

    gpDIDevice[DeviceIndex] = NULL;

    if (Exclusive)
    {
        CooperationFlags or_eq DISCL_EXCLUSIVE;
    }
    else
    {
        CooperationFlags or_eq DISCL_NONEXCLUSIVE;
    }

    // Obtain an interface to the system mouse device
    if (SetupResult)
    {
#ifndef USE_DINPUT_8 // Retro 15Jan2004
        SetupResult = VerifyResult(gpDIObject->CreateDeviceEx(pGUID, IID_IDirectInputDevice7, (void **) &gpDIDevice[DeviceIndex], NULL));
#else
        HRESULT hr = gpDIObject->CreateDevice(pGUID, &gpDIDevice[DeviceIndex], NULL);
        SetupResult = (hr == DI_OK) ? TRUE : FALSE;
#endif
    }

    // Set the data format to "device format"
    if (SetupResult)
    {
        SetupResult = VerifyResult(gpDIDevice[DeviceIndex]->SetDataFormat(pFormat));
    }

    // Set the cooperativity level.
    if (SetupResult)
    {
        SetupResult = VerifyResult(gpDIDevice[DeviceIndex]->SetCooperativeLevel(hWnd, CooperationFlags));
    }

    // Create the handle that tells us new data is available
    if (SetupResult)
    {
        gphDeviceEvent[DeviceIndex] = CreateEvent(0, 0, 0, 0);
        SetupResult = (gphDeviceEvent[DeviceIndex] not_eq NULL);
    }

    // Associate the event with the device
    if (SetupResult)
    {
        SetupResult = VerifyResult(gpDIDevice[DeviceIndex]->SetEventNotification(gphDeviceEvent[DeviceIndex]));
    }

    // Set the buffer size fo DINPUT_BUFFERSIZE elements.
    // The buffer size is a DWORD property associated wht the device.
    if (SetupResult)
    {
        SetupResult = VerifyResult(gpDIDevice[DeviceIndex]->SetProperty(DIPROP_BUFFERSIZE, &pDIPDW->diph));
    }

    if (SetupResult)
    {
        SetupResult = VerifyResult(gpDIDevice[DeviceIndex]->Acquire());
        gpDeviceAcquired[DeviceIndex] = SetupResult;
    }

    return (SetupResult);
}

