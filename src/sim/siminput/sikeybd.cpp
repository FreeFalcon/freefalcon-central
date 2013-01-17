#include "stdhdr.h"
#include "otwdrive.h"
#include "debuggr.h"
#include "commands.h"
#include "sinput.h"

void CallInputFunction (unsigned long val, int state);

//***********************************
//	void OnSimKeyboardInput()
//***********************************

void OnSimKeyboardInput()
{
	DIDEVICEOBJECTDATA	ObjData[DKEYBOARD_BUFFERSIZE];
	DWORD						dwElements;
	HRESULT					hResult;
	UINT						i;
	static int				ShiftCount = 0;
	static int				CtrlCount = 0;
	static int				AltCount = 0;
	int state;
	char     buffer[256];

	dwElements = DKEYBOARD_BUFFERSIZE;
	hResult = gpDIDevice[SIM_KEYBOARD]->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), ObjData, &dwElements, 0);

	if(hResult == DIERR_INPUTLOST){
		gpDeviceAcquired[SIM_KEYBOARD] = FALSE;
		return;
	}

	if(SUCCEEDED(hResult)){
		for(i = 0; i < dwElements; i++){

			// OK. Here we go. You can get the current state of the key
			// from the dwData member. If the High order bit of the low order byte
			// is set then the key was down at the time of the event. Unset then it
			// wasn't. We can use this little piece of data to get rid of the array
			// we use to maintain state of each key. If this works it should also
			// eliminate the lost keyboard problem.

			//MonoPrint (
			// "i:%d  Key Id %d state %d Array %d\n", 
			// ObjData[i].dwOfs, ObjData[i].dwData & 0x7F, ObjData[i].dwData & 0x80
			//);

			if(ObjData[i].dwData & 0x80){ 
				// key is down
				switch (ObjData[i].dwOfs){
					case DIK_LSHIFT:
					case DIK_RSHIFT:
					{
						ShiftCount ++;
					}
					break;

					case DIK_LCONTROL:
					case DIK_RCONTROL:
						CtrlCount ++;
					break;

					case DIK_LMENU:
					case DIK_RMENU:
						AltCount ++;
					break;

					default:
						state  = KEY_DOWN;
						state |= (ShiftCount > 0 ? SHIFT_KEY : 0);
						state |= (CtrlCount > 0 ? CTRL_KEY : 0);
						state |= (AltCount > 0 ? ALT_KEY : 0);
						CallInputFunction(ObjData[i].dwOfs, state);
					break;
				}
			}
			else {
				switch (ObjData[i].dwOfs){
					case DIK_LSHIFT:
					case DIK_RSHIFT:
						ShiftCount --;
					break;

					case DIK_LCONTROL:
					case DIK_RCONTROL:
						CtrlCount --;
					break;

					case DIK_LMENU:
					case DIK_RMENU:
						AltCount --;
					break;

					default:
						state = (ShiftCount > 0 ? SHIFT_KEY : 0);
						state |= (CtrlCount > 0 ? CTRL_KEY : 0);
						state |= (AltCount > 0 ? ALT_KEY : 0);
						CallInputFunction(ObjData[i].dwOfs, state);
					break;
				}
			}
		}

		//after every pass we reset the counts to try to keep them sane
		hResult = gpDIDevice[SIM_KEYBOARD]->GetDeviceState(sizeof(buffer),(LPVOID)&buffer); 
		if(hResult == DI_OK){
			if(buffer[DIK_LSHIFT] & 0x80){
				ShiftCount = 1;
			}
			else {
				ShiftCount = 0;
			}

			if (buffer[DIK_RSHIFT] & 0x80){
				ShiftCount++;
			}

			if (buffer[DIK_LCONTROL] & 0x80){
				CtrlCount = 1;
			}
			else {
				CtrlCount = 0;
			}

			if (buffer[DIK_RCONTROL] & 0x80){
				CtrlCount++;
			}

			if (buffer[DIK_LMENU] & 0x80){
				AltCount = 1;
			}
			else {
				AltCount = 0;
			}

			if(buffer[DIK_RMENU] & 0x80){
				AltCount++;
			}
		}
	}		
}
