#include "stdhdr.h"
#include "commands.h"
#include "inpFunc.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "falclib\include\f4find.h"
#include "simfile.h"
#include "f4find.h"
#include "PlayerOp.h"
#include "aircrft.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "ascii.h"
#include "popmenu.h"

#include "sinput.h"	// Retro 15Jan2004

InputFunctionHashTable UserFunctionTable;

//Wombat778 10-07-2003 Scroll wheel variables
extern char g_strScrollUpFunction[0x40];
extern char g_strScrollDownFunction[0x40];
extern char g_strMiddleButtonFunction[0x40];
extern InputFunctionType scrollupfunc;
extern InputFunctionType scrolldownfunc;
extern InputFunctionType middlebuttonfunc;
//Wombat778 10-07-2003 end of new variables

#ifdef USE_SH_POOLS
MEM_POOL gInputMemPool = NULL;
#endif

int CommandsKeyCombo = 0;
int CommandsKeyComboMod = 0;

// Variables for use with Input functions
unsigned int chatterCount=0;
char chatterStr[256];
short AsciiAllowed=0;
unsigned int MaxInputLength=60;
void (*UseInputFn)()=NULL;
void (*DiscardInputFn)()=NULL;

// Input function DEFS
void StandardAsciiInput(unsigned long key,int state);
void ExtendedKeyInput(unsigned long key,int state);

void InputBuildString (unsigned long i);
int insertMode = 0;

InputFunctionHashTable::InputFunctionHashTable(void)
{
int i;

	#ifdef USE_SH_POOLS
	if ( gInputMemPool == NULL )
	{
      	gInputMemPool = MemPoolInitFS( sizeof(struct FunctionPtrListEntry), 20, 0 );
	}
	#endif

   for (i=0; i<NumHashEntries; i++)
   {
      functionTable[i] = NULL;
   }

   for (i=0; i<NumButtons; i++)
   {
      buttonTable[i].func = NULL;
	  buttonTable[i].cpButtonID = -1;
   }

   for (i=0; i<NumPOVs; i++)
   {
	   for(int j = 0; j < 8; j++)
	   {
		   POVTable[i].func[j] = NULL;
		   POVTable[i].cpButtonID[j] = -1;
	   }
   }


}

InputFunctionHashTable::~InputFunctionHashTable(void)
{
   ClearTable();

	#ifdef USE_SH_POOLS
	if ( gInputMemPool != NULL )
	{
      	MemPoolFree (gInputMemPool);
      	gInputMemPool = NULL;
	}
	#endif
}

void InputFunctionHashTable::ClearTable(void)
{
int i;
struct FunctionPtrListEntry* tmpEntry;

   for (i=0; i<NumHashEntries; i++){
      while (functionTable[i]){
         tmpEntry = functionTable[i];
         functionTable[i] = functionTable[i]->next;
		 #ifdef USE_SH_POOLS
         MemFreeFS( tmpEntry );
		 #else
         delete tmpEntry;
		 #endif
      }
   }

   for (i=0; i<NumButtons; i++)
   {
	   buttonTable[i].func = NULL;
	   buttonTable[i].cpButtonID = -1;
   }

   for (i=0; i<NumPOVs; i++)
   {
	   for(int j = 0; j < 8; j++)
	   {
		   POVTable[i].func[j] = NULL;
		   POVTable[i].cpButtonID[j] = -1;
	   }
   }
}

void InputFunctionHashTable::AddFunction (int key, int flags, int buttonId, int mouseSide, InputFunctionType funcPtr)
{
struct FunctionPtrListEntry* tmpEntry;

   if (key < 0 || key >= NumHashEntries)
	   return;

   // Check for duplicate
   tmpEntry = functionTable[key];
   while (tmpEntry)
   {
      if (tmpEntry->flags == flags)
         break;

      tmpEntry = tmpEntry->next;
   }

   // F4Assert (tmpEntry == NULL);

	#ifdef USE_SH_POOLS
    tmpEntry = (FunctionPtrListEntry *)MemAllocFS( gInputMemPool );
	#else
    tmpEntry = new struct FunctionPtrListEntry;
	#endif
	tmpEntry->mouseSide	= mouseSide;
	tmpEntry->buttonId		= buttonId;
   tmpEntry->flags		= flags;
   tmpEntry->theFunc		= funcPtr;
	tmpEntry->controlID		= 0;
   tmpEntry->next			= functionTable[key];
   functionTable[key]	= tmpEntry;
}

void InputFunctionHashTable::RemoveFunction (int key, int flags)
{
struct FunctionPtrListEntry* tmpEntry;
struct FunctionPtrListEntry* lastEntry = NULL;

   if(key == -1)
	   return;

   tmpEntry = functionTable[key];
   while (tmpEntry)
   {
      if (tmpEntry->flags == flags)
         break;

      lastEntry = tmpEntry;
      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry && tmpEntry->flags == flags)
   {
      if (lastEntry)
         lastEntry->next = tmpEntry->next;
      else
         functionTable[key] = tmpEntry->next;

	  #ifdef USE_SH_POOLS
        MemFreeFS( tmpEntry );
	  #else
        delete tmpEntry;
	  #endif
   }
}

InputFunctionType InputFunctionHashTable::GetFunction (int key, int flags, int* pbuttonId, int* pmouseSide)
{
struct FunctionPtrListEntry* tmpEntry;
InputFunctionType retval = NULL;

   if(key == -1)
	   return NULL;

   tmpEntry = functionTable[key];
   while (tmpEntry)
   {
      if (tmpEntry->flags == flags)
         break;

      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry) {
		*pbuttonId = tmpEntry->buttonId;
		*pmouseSide = tmpEntry->mouseSide;
      retval = tmpEntry->theFunc;
	}

   return retval;
}


//Wombat778 2-05-04	 Find a function's buttonid from a pointer

int InputFunctionHashTable::GetButtonId (InputFunctionType funcPtr)
{
	struct FunctionPtrListEntry* tmpEntry;

	for (int i=0; i < NumHashEntries; i++)			//Wombat778 2-05-04 I will burn in hell for doing this to a hash table
	{
		tmpEntry = functionTable[i];
		while (tmpEntry)
		{
			if (tmpEntry->theFunc == funcPtr)
			   return tmpEntry->buttonId;      
			tmpEntry = tmpEntry->next;
		}

	}
   
	return 0;
}


long InputFunctionHashTable::GetControl(int key, int flags)
{
	struct FunctionPtrListEntry* tmpEntry;
	long retval = 0;
   
   if(key == -1)
	   return retval;

   tmpEntry = functionTable[key];
   while (tmpEntry)
   {
      if (tmpEntry->flags == flags)
         break;

      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry) {
      retval = tmpEntry->controlID;
	}

   return retval;
}

BOOL InputFunctionHashTable::SetControl(int key, int flags, long control)
{
	struct FunctionPtrListEntry* tmpEntry;
	BOOL retval = FALSE;

   if(key == -1)
	   return retval;

   tmpEntry = functionTable[key];
   while (tmpEntry)
   {
      if (tmpEntry->flags == flags)
         break;

      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry) {
      tmpEntry->controlID = control;
	  retval = TRUE;
	}

   return retval;
}

BOOL InputFunctionHashTable::SetButtonFunction(int buttonID,InputFunctionType theFunc, int CPbuttonId)
{
	if(buttonID < 0 || buttonID >= NumButtons) 
		return FALSE;
	
	buttonTable[buttonID].func = theFunc;
	buttonTable[buttonID].cpButtonID = CPbuttonId;
	return TRUE;
}

InputFunctionType InputFunctionHashTable::GetButtonFunction(int buttonID, int *cpButtonID)
{
	if(buttonID < 0 || buttonID >= NumButtons)
	{
		if(cpButtonID)
			*cpButtonID = -1;
		return NULL; 
	}

	if(cpButtonID)
		*cpButtonID = buttonTable[buttonID].cpButtonID;

	return buttonTable[buttonID].func;
}

BOOL InputFunctionHashTable::SetPOVFunction(int POV, int dir, InputFunctionType theFunc, int cpButtonID )
{
	if(POV < 0 || POV >= NumPOVs) 
		return FALSE;
	if (dir < 0 || dir >= MAX_POV_DIR)
	    return FALSE;
	
	POVTable[POV].func[dir] = theFunc;
	POVTable[POV].cpButtonID[dir] = cpButtonID;
	return TRUE;
}

InputFunctionType InputFunctionHashTable::GetPOVFunction(int POV, int dir, int* cpButtonID)
{
	if(POV < 0 || POV >= NumPOVs || dir < 0 || dir >= MAX_POV_DIR)
	{
		if(cpButtonID)
			*cpButtonID = -1;
		return NULL; 
	}

	if(cpButtonID)
		*cpButtonID = POVTable[POV].cpButtonID[dir];

	return POVTable[POV].func[dir];
}

void SetupInputFunctions (void)
{
   chatterCount = 0;
   memset (chatterStr, 0, sizeof(chatterStr));
   //LoadFunctionTables();
}

void CleanupInputFunctions (void)
{
   //UserFunctionTable.ClearTable();
}

//Wombat778 03-06-04 Call this instead of the input function directly. It allows capturing/blocking of keystrokes
void
CallFunc(InputFunctionType theFunc, unsigned long val, int state, void* pButton) 
{
//	if (!TrainingScript->IsBlocked(theFunc,NULL))		//Wombat778 3-09-04 Check if this function is being blocked by the training script
//	{
//		if (TrainingScript->IsCapturing())
//			TrainingScript->CaptureCommand(theFunc, NULL);
		theFunc(val, state, pButton);
//	}
}


void CallInputFunction (unsigned long val, int state){
	int keyDown = (state & KEY_DOWN ? 1 : 0);
	InputFunctionType theFunc;
	int flags, buttonId, mouseSide;

	// Special String builder
	if (CommandsKeyCombo == -1 && CommandsKeyComboMod == -1){
		if (keyDown){
			InputBuildString (val);
		}
	}
	else if (CommandsKeyCombo == -2 && CommandsKeyComboMod == -2){
		if (keyDown) { 
			//dangling else - JPO
			if (!(state & 0x6) && DIK_IsAscii(val,state)){
				StandardAsciiInput(val,state);
			}
			else {
				ExtendedKeyInput(val,state);
			}
		}
	}
	else if (OTWDriver.InExitMenu() && keyDown){
		if (keyDown){
			OTWDriver.ExitMenu (val);
		}
	}
	else {
		flags = 
			(state & MODS_MASK) + 
			(CommandsKeyCombo << SECOND_KEY_SHIFT) + 
			(CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT)
		;
		theFunc = UserFunctionTable.GetFunction(val, flags, &buttonId, &mouseSide);

		/* // ASSOCIATOR: Commented this out so that Comms menu will not deactivate while pressing other keys
		// Cancel the combo, whether it is handled or not
		if (
			CommandsKeyCombo && keyDown && theFunc != ScreenShot && 
			theFunc != RadioMessageSend && theFunc != OTWRadioMenuStep && theFunc != OTWRadioMenuStepBack
		){
			CommandsKeyCombo = 0;
			CommandsKeyComboMod = 0;

				OTWDriver.pMenuManager->DeActivate();
		}*/


		// ASSOCIATOR: Added so that other keys can be pressed while in Comms menus 
		if (
			CommandsKeyComboMod && CommandsKeyCombo && keyDown && 
			theFunc != ScreenShot && theFunc != RadioMessageSend && theFunc != OTWRadioMenuStep && 
			theFunc != OTWRadioMenuStepBack
		){
			CommandsKeyCombo = 0;
			CommandsKeyComboMod = 0;
			OTWDriver.pMenuManager->DeActivate();
		}
			
		// ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
		if( OTWDriver.pMenuManager->IsActive() && val >= DIK_1 && val <= DIK_9 ){
			theFunc = RadioMessageSend;
		}
		
		// ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
		if( OTWDriver.pMenuManager->IsActive() && val == DIK_ESCAPE ){
			CommandsKeyCombo    = 0;	  
			CommandsKeyComboMod = 0;
			OTWDriver.pMenuManager->DeActivate();
		} 
			
		// ASSOCIATOR: Added so temp variables
		int tempCombo = 0;
		int tempComboMod = 0;

		// ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
		if (
			CommandsKeyCombo && theFunc != ScreenShot && 
			theFunc != RadioMessageSend && 
			theFunc != OTWRadioMenuStep && 
			theFunc != OTWRadioMenuStepBack 
		){
			tempCombo = CommandsKeyCombo;
			tempComboMod = CommandsKeyComboMod;		
			CommandsKeyCombo = 0;
			CommandsKeyComboMod = 0;
			flags = 
				(state & MODS_MASK) + 
				(CommandsKeyCombo << SECOND_KEY_SHIFT) + 
				(CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT)
			;
			theFunc = UserFunctionTable.GetFunction(val, flags, &buttonId, &mouseSide);
			CommandsKeyCombo    = tempCombo;	  
			CommandsKeyComboMod = tempComboMod;				
		}

		
		// Call the Function
		if (theFunc) {
			if(buttonId < 0) {
				//theFunc(val, state, NULL);
				CallFunc(theFunc,val, state, NULL);
			}
 			else {
				//theFunc(val, state, OTWDriver.pCockpitManager->GetButtonPointer(buttonId));
				CallFunc(theFunc,val, state, OTWDriver.pCockpitManager->GetButtonPointer(buttonId));
				if(SimDriver.GetPlayerAircraft() && 
				SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) &&
				!((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
					OTWDriver.pCockpitManager->Dispatch(buttonId, mouseSide);
				}
			}
		}
	}
}

void LoadFunctionTables(_TCHAR *fname)
{
	SimlibFileClass* funcFile;
	char tmpStr[_MAX_PATH];
	char funcName[_MAX_PATH];
	char fileName[_MAX_PATH];
	//char pilotName[_MAX_PATH] = {0};
	int key1, mod1;
	int key2, mod2;
	int flags, buttonId, mouseSide;
	InputFunctionType theFunc;

	sprintf(fileName,"%s\\config\\%s.key",FalconDataDirectory, fname);

	funcFile = SimlibFileClass::Open(fileName, SIMLIB_READ);

	if (funcFile == NULL)
	{
		sprintf(fileName,"%s\\config\\keystrokes.key",FalconDataDirectory);
		funcFile = SimlibFileClass::Open(fileName, SIMLIB_READ);
		if (funcFile == NULL)
		{
			SimLibPrintError ("No Function Table File\n");
			return;
		}
	}

	while (funcFile->ReadLine (tmpStr, sizeof(tmpStr)) == SIMLIB_OK)
	{
		// Skip Comments
		if (tmpStr[0] == '#')
			continue;

		sscanf (tmpStr, "%s %d %d %x %x %x %x %*[^\n]", funcName, &buttonId, &mouseSide, &key2, &mod2, &key1, &mod1);
	      

		theFunc = FindFunctionFromString (funcName);

		if (theFunc)
		{
			if (key1 == -1)
			{
				flags = mod1 + (key2 << SECOND_KEY_SHIFT) + (mod2 << SECOND_KEY_MOD_SHIFT);
				//for (int i=0; i<UserFunctionTable.NumHashEntries; i++)
				for (int i=DIK_1; i<=DIK_0; i++)
				
				{
				//Find this key combo
				UserFunctionTable.AddFunction(i, flags, buttonId, mouseSide, theFunc);
				}
				UserFunctionTable.AddFunction(DIK_ESCAPE, flags, buttonId, mouseSide, theFunc);
 				UserFunctionTable.AddFunction(DIK_SYSRQ, flags, buttonId, mouseSide, theFunc); // screen shot
			}
			else
			{
				//this function has no key combo assigned
				if(key2 == -1)
					continue;
				
				if(key2 == -2)
				{
					UserFunctionTable.SetButtonFunction(buttonId, theFunc, mouseSide);
					//int this case mouseside contains the cockpit button ID while
					//buttonID is the corresponding joystick button
				}
				else if(key2 == -3)
				{
					UserFunctionTable.SetPOVFunction(buttonId, mod2, theFunc, mouseSide);
					//int this case mouseside contains the cockpit button ID while
					//buttonID is the corresponding hat
					//mod2 is the direction the hat is pressed
				}
				else
				{
					//Find this key combo
					flags = mod2 + (key1 << SECOND_KEY_SHIFT) + (mod1 << SECOND_KEY_MOD_SHIFT);
					UserFunctionTable.AddFunction(key2, flags, buttonId, mouseSide, theFunc);
				}
			}
		}
		else
		{
			// MonoPrint ("ERROR !!!!! %s not found\n", funcName);
	#ifdef DEBUG
			//sprintf (tmpStr, "ERROR !!!!! %s not found\n", funcName);
			//OutputDebugString (tmpStr);
	#endif
		}
	}
	funcFile->Close();
	delete funcFile;

	//Wombat778 10-07-2003 Load scroll wheel functions. Added these here because I need to be 100% sure that keys have been loaded.
	scrollupfunc=FindFunctionFromString(g_strScrollUpFunction);		
	scrolldownfunc=FindFunctionFromString(g_strScrollDownFunction);
	middlebuttonfunc=FindFunctionFromString(g_strMiddleButtonFunction);
}

void InputBuildString (unsigned long i)
{
   switch (i)
   {
      case DIK_RETURN: // Enter
	   case DIK_NUMPADENTER:
         CommandsKeyCombo = 0;
         CommandsKeyComboMod = 0;
      break;

      case DIK_BACK:
         memmove (&chatterStr[chatterCount-1], &chatterStr[chatterCount],
            MAX_CHAT_LENGTH - chatterCount);
         chatterCount --;
      break;

      case DIK_DELETE:
         memmove (&chatterStr[chatterCount], &chatterStr[chatterCount+1],
            MAX_CHAT_LENGTH - chatterCount - 1);
         chatterCount --;
      break;

      case DIK_LEFT:
         if (chatterCount)
            chatterCount --;
      break;

      case DIK_RIGHT:
         if (chatterCount < strlen (chatterStr))
            chatterCount ++;
      break;

      case DIK_INSERT:
         insertMode = 1 - insertMode;
      break;

      case DIK_1:
      case DIK_2:
      case DIK_3:
      case DIK_4:
      case DIK_5:
      case DIK_6:
      case DIK_7:
      case DIK_8:
      case DIK_9:
         if (chatterCount < MAX_CHAT_LENGTH)
         {
            if (insertMode)
            {
               memmove (&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
                  MAX_CHAT_LENGTH - chatterCount - 1);
            }
			   chatterStr[chatterCount] = (char)(i - DIK_1 + '1');
            chatterCount ++;
         }
      break;

      case DIK_0:
         if (chatterCount < MAX_CHAT_LENGTH)
         {
            if (insertMode)
            {
               memmove (&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
                  MAX_CHAT_LENGTH - chatterCount - 1);
            }
			   chatterStr[chatterCount] = '0';
            chatterCount ++;
         }
      break;

      case DIK_PERIOD:
         if (chatterCount < MAX_CHAT_LENGTH)
         {
            if (insertMode)
            {
               memmove (&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
                  MAX_CHAT_LENGTH - chatterCount - 1);
            }
			   chatterStr[chatterCount] = '.';
            chatterCount ++;
         }
      break;

      case DIK_MINUS:
         if (chatterCount < MAX_CHAT_LENGTH)
         {
            if (insertMode)
            {
               memmove (&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
                  MAX_CHAT_LENGTH - chatterCount - 1);
            }
			   chatterStr[chatterCount] = '-';
            chatterCount ++;
         }
      break;
   }
}

// Handles any Printable character
void StandardAsciiInput(unsigned long key,int state)
{
	unsigned long i;
	char asciival;
		
	asciival=AsciiChar(key,state);

	switch(AsciiAllowed)
	{
		case 1: // Integers
			if(!DIK_IsDigit(key,state) && asciival != '-')
				return;
			break;
		case 2: // Floats
			if(!DIK_IsDigit(key,state) && asciival != '-' && asciival != '.')
				return;
			break;
	}
	if(chatterCount < MaxInputLength)
	{
		if(insertMode)
		{	// move rest of chars to the right
			for(i=MaxInputLength-2;i >= chatterCount;i--)
				chatterStr[i+1]=chatterStr[i];
			chatterStr[MaxInputLength]=0;
		}
		if(!chatterStr[chatterCount])
			chatterStr[chatterCount+1]=0;
		chatterStr[chatterCount]=asciival;
		chatterCount++;
	}
}

// Handles NON ascii stuff like END, CURSOR keys, HOME etc.
// Can handle ALT,SHIFT,CTRL flags... if you implement it
void ExtendedKeyInput(unsigned long key,int)
{
	static unsigned long i;
	switch(key)
	{
		case DIK_NUMPAD6:
			if(chatterCount < MaxInputLength && chatterStr[chatterCount])
			{
				chatterCount++;
			}
			break;
		case DIK_NUMPAD4:
			if(chatterCount > 0)
			{
				chatterCount--;
			}
			break;
		case DIK_NUMPAD0:
			// insert mode
			break;
		case DIK_NUMPAD7:
			if(chatterCount > 0)
			{
				chatterCount=0;
			}
			break;
		case DIK_INSERT:
			insertMode ^= 1;
			break;
		case DIK_NUMPAD1:
			while(chatterStr[chatterCount] && chatterCount < (MaxInputLength-1))
				chatterCount++;
			break;
		case DIK_DECIMAL:
			if(chatterCount >= (MaxInputLength))
				break;
			if(chatterStr[chatterCount])
			{
				i=chatterCount+1;
				if(i < MaxInputLength)
				{
					while(chatterStr[i] && i < MaxInputLength)
					{
						chatterStr[i-1]=chatterStr[i];
						i++;
					}
					chatterStr[i-1]=0;
				}
			}
			break;
		case DIK_BACK:
			if(chatterCount > 0)
			{				
				chatterCount--;
				for(i=chatterCount;i < MaxInputLength;i++)
					chatterStr[i]=chatterStr[i+1];
				chatterStr[MaxInputLength]=0;
			}
			break;
		case DIK_RETURN:
			// Use it
			if(UseInputFn)
				(*UseInputFn)();
			chatterCount=0;
			CommandsKeyCombo = 0;
			CommandsKeyComboMod = 0;
			memset(chatterStr,0,sizeof(chatterStr));
			UseInputFn=NULL;
			DiscardInputFn=NULL;
			OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() & ~SHOW_CHATBOX);
			break;
		case DIK_ESCAPE:
			// Discard it
			if(DiscardInputFn)
				(*DiscardInputFn)();
			chatterCount=0;
			CommandsKeyCombo = 0;
			CommandsKeyComboMod = 0;
			memset(chatterStr,0,sizeof(chatterStr));
			UseInputFn=NULL;
			DiscardInputFn=NULL;
			OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() & ~SHOW_CHATBOX);
			break;
	}
}

#ifdef HASH_TEST

int main (void)
{
   InitDebug (DEBUGGER_TEXT_MODE);
   LoadFunctionTables();
   return 0;
}
#endif
