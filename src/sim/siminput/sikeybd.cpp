#include "stdhdr.h"
#include "otwdrive.h"
#include "debuggr.h"
#include "commands.h"
#include "sinput.h"

void CallInputFunction(unsigned long val, int state);

//***********************************
// void OnSimKeyboardInput()
//***********************************

void OnSimKeyboardInput()
{
    DIDEVICEOBJECTDATA ObjData[DKEYBOARD_BUFFERSIZE];
    DWORD dwElements;
    HRESULT hResult;
    UINT i;
    static int ShiftCount = 0;
    static int CtrlCount = 0;
    static int AltCount = 0;
    int state;

    // Artscout - 2026 (#93): re-sync the modifier counts from the REAL OS key state at the START of every
    // pass (before processing this pass's key events), so the FIRST key after an Alt-Tab (e.g. ESC) already
    // sees the correct modifiers. On Alt-Tab OUT the app catches the Alt key-DOWN (held for the gesture) but
    // the key-UP is lost while the exclusive DI keyboard is (re)acquiring -> the DI GetDeviceState stayed
    // stuck-down and reported Alt as HELD forever -> ESC read as Alt+ESC. GetAsyncKeyState reflects the true
    // OS state, so once Alt is physically up the count is 0 immediately -- and doing it at the TOP (not the
    // end of the pass) fixes the "first ESC ignored, second works" one-frame lag.
    ShiftCount = ((GetAsyncKeyState(VK_LSHIFT) bitand 0x8000) ? 1 : 0) +
                 ((GetAsyncKeyState(VK_RSHIFT) bitand 0x8000) ? 1 : 0);
    CtrlCount = ((GetAsyncKeyState(VK_LCONTROL) bitand 0x8000) ? 1 : 0) +
                ((GetAsyncKeyState(VK_RCONTROL) bitand 0x8000) ? 1 : 0);
    AltCount = ((GetAsyncKeyState(VK_LMENU) bitand 0x8000) ? 1 : 0) +
               ((GetAsyncKeyState(VK_RMENU) bitand 0x8000) ? 1 : 0);

    dwElements = DKEYBOARD_BUFFERSIZE;
    hResult = gpDIDevice[SIM_KEYBOARD]->GetDeviceData(
        sizeof(DIDEVICEOBJECTDATA), ObjData, &dwElements, 0);

    // PHASE 5 (fix 'keyboard dead after Alt+Tab'): on focus loss the device
    // becomes INPUTLOST/NOTACQUIRED. Previously the code just marked FALSE and didn't reclaim
    // the acquisition -> input wasn't restored. Now we re-Acquire and retry.
    if (hResult == DIERR_INPUTLOST or hResult == DIERR_NOTACQUIRED)
    {
        if (SUCCEEDED(gpDIDevice[SIM_KEYBOARD]->Acquire()))
        {
            gpDeviceAcquired[SIM_KEYBOARD] = TRUE;
            dwElements = DKEYBOARD_BUFFERSIZE;
            hResult = gpDIDevice[SIM_KEYBOARD]->GetDeviceData(
                sizeof(DIDEVICEOBJECTDATA), ObjData, &dwElements, 0);
        }
        else
        {
            gpDeviceAcquired[SIM_KEYBOARD] = FALSE;
            return;
        }
    }

    if (SUCCEEDED(hResult))
    {
        for (i = 0; i < dwElements; i++)
        {

            // OK. Here we go. You can get the current state of the key
            // from the dwData member. If the High order bit of the low order byte
            // is set then the key was down at the time of the event. Unset then it
            // wasn't. We can use this little piece of data to get rid of the array
            // we use to maintain state of each key. If this works it should also
            // eliminate the lost keyboard problem.

            //MonoPrint (
            // "i:%d  Key Id %d state %d Array %d\n",
            // ObjData[i].dwOfs, ObjData[i].dwData bitand 0x7F, ObjData[i].dwData bitand 0x80
            //);

            if (ObjData[i].dwData bitand 0x80)
            {
                // key is down
                switch (ObjData[i].dwOfs)
                {
                case DIK_LSHIFT:
                case DIK_RSHIFT:
                {
                    ShiftCount++;
                }
                break;

                case DIK_LCONTROL:
                case DIK_RCONTROL:
                    CtrlCount++;
                    break;

                case DIK_LMENU:
                case DIK_RMENU:
                    AltCount++;
                    break;

                default:
                    state = KEY_DOWN;
                    state or_eq (ShiftCount > 0 ? SHIFT_KEY : 0);
                    state or_eq (CtrlCount > 0 ? CTRL_KEY : 0);
                    state or_eq (AltCount > 0 ? ALT_KEY : 0);
                    CallInputFunction(ObjData[i].dwOfs, state);
                    break;
                }
            }
            else
            {
                switch (ObjData[i].dwOfs)
                {
                case DIK_LSHIFT:
                case DIK_RSHIFT:
                    ShiftCount--;
                    break;

                case DIK_LCONTROL:
                case DIK_RCONTROL:
                    CtrlCount--;
                    break;

                case DIK_LMENU:
                case DIK_RMENU:
                    AltCount--;
                    break;

                default:
                    state = (ShiftCount > 0 ? SHIFT_KEY : 0);
                    state or_eq (CtrlCount > 0 ? CTRL_KEY : 0);
                    state or_eq (AltCount > 0 ? ALT_KEY : 0);
                    CallInputFunction(ObjData[i].dwOfs, state);
                    break;
                }
            }
        }

        // Artscout - 2026 (#93): re-sync the modifier counts every pass from the REAL OS key state
        // (GetAsyncKeyState), NOT the DirectInput GetDeviceState. On Alt-Tab BACK the app catches the Alt
        // key-DOWN (Alt is held as part of the gesture) but the key-UP on release is lost while the exclusive
        // DI keyboard is (re)acquiring -> DI's GetDeviceState reported Alt as HELD forever -> ESC read as
        // Alt+ESC until a real Alt-up (Alt+F4). GetAsyncKeyState reflects the true OS state, so the instant
        // Alt is physically released the count self-corrects to 0 -- independent of the lost DI up-event.
        ShiftCount = ((GetAsyncKeyState(VK_LSHIFT) bitand 0x8000) ? 1 : 0) +
                     ((GetAsyncKeyState(VK_RSHIFT) bitand 0x8000) ? 1 : 0);
        CtrlCount = ((GetAsyncKeyState(VK_LCONTROL) bitand 0x8000) ? 1 : 0) +
                    ((GetAsyncKeyState(VK_RCONTROL) bitand 0x8000) ? 1 : 0);
        AltCount = ((GetAsyncKeyState(VK_LMENU) bitand 0x8000) ? 1 : 0) +
                   ((GetAsyncKeyState(VK_RMENU) bitand 0x8000) ? 1 : 0);
    }
}
