#include "stdhdr.h"
#include "commands.h"
#include "inpFunc.h"
#include "controlsxml.h"   // #53: XML function catalog (name -> BMS label)
#include "otwdrive.h"
#include "cpmanager.h"
#include "falclib/include/f4find.h"
#include "simfile.h"
#include "f4find.h"
#include "PlayerOp.h"
#include "aircrft.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "ascii.h"
#include "popmenu.h"

#include "sinput.h" // Retro 15Jan2004

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
unsigned int chatterCount = 0;
char chatterStr[256];
short AsciiAllowed = 0;
unsigned int MaxInputLength = 60;
void (*UseInputFn)() = NULL;
void (*DiscardInputFn)() = NULL;

// Input function DEFS
void StandardAsciiInput(unsigned long key, int state);
void ExtendedKeyInput(unsigned long key, int state);

void InputBuildString(unsigned long i);
int insertMode = 0;

InputFunctionHashTable::InputFunctionHashTable(void)
{
    int i;

#ifdef USE_SH_POOLS

    if (gInputMemPool == NULL)
    {
        gInputMemPool = MemPoolInitFS(sizeof(struct FunctionPtrListEntry), 20, 0);
    }

#endif

    for (i = 0; i < NumHashEntries; i++)
    {
        functionTable[i] = NULL;
    }

    for (i = 0; i < NumButtons; i++)
    {
        buttonTable[i].func = NULL;
        buttonTable[i].cpButtonID = -1;
    }

    for (i = 0; i < NumPOVs; i++)
    {
        for (int j = 0; j < 8; j++)
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

    if (gInputMemPool not_eq NULL)
    {
        MemPoolFree(gInputMemPool);
        gInputMemPool = NULL;
    }

#endif
}

void InputFunctionHashTable::ClearTable(void)
{
    int i;
    struct FunctionPtrListEntry* tmpEntry;

    for (i = 0; i < NumHashEntries; i++)
    {
        while (functionTable[i])
        {
            tmpEntry = functionTable[i];
            functionTable[i] = functionTable[i]->next;
#ifdef USE_SH_POOLS
            MemFreeFS(tmpEntry);
#else
            delete tmpEntry;
#endif
        }
    }

    for (i = 0; i < NumButtons; i++)
    {
        buttonTable[i].func = NULL;
        buttonTable[i].cpButtonID = -1;
    }

    for (i = 0; i < NumPOVs; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            POVTable[i].func[j] = NULL;
            POVTable[i].cpButtonID[j] = -1;
        }
    }
}

void InputFunctionHashTable::AddFunction(int key, int flags, int buttonId, int mouseSide, InputFunctionType funcPtr)
{
    struct FunctionPtrListEntry* tmpEntry;

    if (key < 0 or key >= NumHashEntries)
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
    tmpEntry = (FunctionPtrListEntry *)MemAllocFS(gInputMemPool);
#else
    tmpEntry = new struct FunctionPtrListEntry;
#endif
    tmpEntry->mouseSide = mouseSide;
    tmpEntry->buttonId = buttonId;
    tmpEntry->flags = flags;
    tmpEntry->theFunc = funcPtr;
    tmpEntry->controlID = 0;
    tmpEntry->next = functionTable[key];
    functionTable[key] = tmpEntry;
}

void InputFunctionHashTable::RemoveFunction(int key, int flags)
{
    struct FunctionPtrListEntry* tmpEntry;
    struct FunctionPtrListEntry* lastEntry = NULL;

    if (key == -1)
        return;

    tmpEntry = functionTable[key];

    while (tmpEntry)
    {
        if (tmpEntry->flags == flags)
            break;

        lastEntry = tmpEntry;
        tmpEntry = tmpEntry->next;
    }

    if (tmpEntry and tmpEntry->flags == flags)
    {
        if (lastEntry)
            lastEntry->next = tmpEntry->next;
        else
            functionTable[key] = tmpEntry->next;

#ifdef USE_SH_POOLS
        MemFreeFS(tmpEntry);
#else
        delete tmpEntry;
#endif
    }
}

InputFunctionType InputFunctionHashTable::GetFunction(int key, int flags, int* pbuttonId, int* pmouseSide)
{
    struct FunctionPtrListEntry* tmpEntry;
    InputFunctionType retval = NULL;

    if (key == -1)
        return NULL;

    tmpEntry = functionTable[key];

    while (tmpEntry)
    {
        if (tmpEntry->flags == flags)
            break;

        tmpEntry = tmpEntry->next;
    }

    if (tmpEntry)
    {
        *pbuttonId = tmpEntry->buttonId;
        *pmouseSide = tmpEntry->mouseSide;
        retval = tmpEntry->theFunc;
    }

    return retval;
}


//Wombat778 2-05-04  Find a function's buttonid from a pointer

int InputFunctionHashTable::GetButtonId(InputFunctionType funcPtr)
{
    struct FunctionPtrListEntry* tmpEntry;

    for (int i = 0; i < NumHashEntries; i++) //Wombat778 2-05-04 I will burn in hell for doing this to a hash table
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

    if (key == -1)
        return retval;

    tmpEntry = functionTable[key];

    while (tmpEntry)
    {
        if (tmpEntry->flags == flags)
            break;

        tmpEntry = tmpEntry->next;
    }

    if (tmpEntry)
    {
        retval = tmpEntry->controlID;
    }

    return retval;
}

BOOL InputFunctionHashTable::SetControl(int key, int flags, long control)
{
    struct FunctionPtrListEntry* tmpEntry;
    BOOL retval = FALSE;

    if (key == -1)
        return retval;

    tmpEntry = functionTable[key];

    while (tmpEntry)
    {
        if (tmpEntry->flags == flags)
            break;

        tmpEntry = tmpEntry->next;
    }

    if (tmpEntry)
    {
        tmpEntry->controlID = control;
        retval = TRUE;
    }

    return retval;
}

BOOL InputFunctionHashTable::SetButtonFunction(int buttonID, InputFunctionType theFunc, int CPbuttonId)
{
    if (buttonID < 0 or buttonID >= NumButtons)
        return FALSE;

    buttonTable[buttonID].func = theFunc;
    buttonTable[buttonID].cpButtonID = CPbuttonId;
    return TRUE;
}

InputFunctionType InputFunctionHashTable::GetButtonFunction(int buttonID, int *cpButtonID)
{
    if (buttonID < 0 or buttonID >= NumButtons)
    {
        if (cpButtonID)
            *cpButtonID = -1;

        return NULL;
    }

    if (cpButtonID)
        *cpButtonID = buttonTable[buttonID].cpButtonID;

    return buttonTable[buttonID].func;
}

BOOL InputFunctionHashTable::SetPOVFunction(int POV, int dir, InputFunctionType theFunc, int cpButtonID)
{
    if (POV < 0 or POV >= NumPOVs)
        return FALSE;

    if (dir < 0 or dir >= MAX_POV_DIR)
        return FALSE;

    POVTable[POV].func[dir] = theFunc;
    POVTable[POV].cpButtonID[dir] = cpButtonID;
    return TRUE;
}

InputFunctionType InputFunctionHashTable::GetPOVFunction(int POV, int dir, int* cpButtonID)
{
    if (POV < 0 or POV >= NumPOVs or dir < 0 or dir >= MAX_POV_DIR)
    {
        if (cpButtonID)
            *cpButtonID = -1;

        return NULL;
    }

    if (cpButtonID)
        *cpButtonID = POVTable[POV].cpButtonID[dir];

    return POVTable[POV].func[dir];
}

void SetupInputFunctions(void)
{
    chatterCount = 0;
    memset(chatterStr, 0, sizeof(chatterStr));
    //LoadFunctionTables();
}

void CleanupInputFunctions(void)
{
    //UserFunctionTable.ClearTable();
}

//Wombat778 03-06-04 Call this instead of the input function directly. It allows capturing/blocking of keystrokes
void
CallFunc(InputFunctionType theFunc, unsigned long val, int state, void* pButton)
{
    // if ( not TrainingScript->IsBlocked(theFunc,NULL)) //Wombat778 3-09-04 Check if this function is being blocked by the training script
    // {
    // if (TrainingScript->IsCapturing())
    // TrainingScript->CaptureCommand(theFunc, NULL);
    theFunc(val, state, pButton);
    // }
}


void CallInputFunction(unsigned long val, int state)
{
    int keyDown = (state bitand KEY_DOWN ? 1 : 0);
    InputFunctionType theFunc;
    int flags, buttonId, mouseSide;

    // Special String builder
    if (CommandsKeyCombo == -1 and CommandsKeyComboMod == -1)
    {
        if (keyDown)
        {
            InputBuildString(val);
        }
    }
    else if (CommandsKeyCombo == -2 and CommandsKeyComboMod == -2)
    {
        if (keyDown)
        {
            //dangling else - JPO
            if ( not (state bitand 0x6) and DIK_IsAscii(val, state))
            {
                StandardAsciiInput(val, state);
            }
            else
            {
                ExtendedKeyInput(val, state);
            }
        }
    }
    else if (OTWDriver.InExitMenu() and keyDown)
    {
        if (keyDown)
        {
            OTWDriver.ExitMenu(val);
        }
    }
    else
    {
        flags =
            (state bitand MODS_MASK) +
            (CommandsKeyCombo << SECOND_KEY_SHIFT) +
            (CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT)
            ;
        theFunc = UserFunctionTable.GetFunction(val, flags, &buttonId, &mouseSide);

        /* // ASSOCIATOR: Commented this out so that Comms menu will not deactivate while pressing other keys
        // Cancel the combo, whether it is handled or not
        if (
         CommandsKeyCombo and keyDown and theFunc not_eq ScreenShot and 
         theFunc not_eq RadioMessageSend and theFunc not_eq OTWRadioMenuStep and theFunc not_eq OTWRadioMenuStepBack
        ){
         CommandsKeyCombo = 0;
         CommandsKeyComboMod = 0;

         OTWDriver.pMenuManager->DeActivate();
        }*/


        // ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
        if (
            CommandsKeyComboMod and CommandsKeyCombo and keyDown and 
            theFunc not_eq ScreenShot and theFunc not_eq RadioMessageSend and theFunc not_eq OTWRadioMenuStep and 
            theFunc not_eq OTWRadioMenuStepBack
        )
        {
            CommandsKeyCombo = 0;
            CommandsKeyComboMod = 0;
            OTWDriver.pMenuManager->DeActivate();
        }

        // ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
        if (OTWDriver.pMenuManager->IsActive() and val >= DIK_1 and val <= DIK_9)
        {
            theFunc = RadioMessageSend;
        }

        // ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
        if (OTWDriver.pMenuManager->IsActive() and val == DIK_ESCAPE)
        {
            CommandsKeyCombo    = 0;
            CommandsKeyComboMod = 0;
            OTWDriver.pMenuManager->DeActivate();
        }

        // ASSOCIATOR: Added so temp variables
        int tempCombo = 0;
        int tempComboMod = 0;

        // ASSOCIATOR: Added so that other keys can be pressed while in Comms menus
        if (
            CommandsKeyCombo and theFunc not_eq ScreenShot and 
            theFunc not_eq RadioMessageSend and 
            theFunc not_eq OTWRadioMenuStep and 
            theFunc not_eq OTWRadioMenuStepBack
        )
        {
            tempCombo = CommandsKeyCombo;
            tempComboMod = CommandsKeyComboMod;
            CommandsKeyCombo = 0;
            CommandsKeyComboMod = 0;
            flags =
                (state bitand MODS_MASK) +
                (CommandsKeyCombo << SECOND_KEY_SHIFT) +
                (CommandsKeyComboMod << SECOND_KEY_MOD_SHIFT)
                ;
            theFunc = UserFunctionTable.GetFunction(val, flags, &buttonId, &mouseSide);
            CommandsKeyCombo    = tempCombo;
            CommandsKeyComboMod = tempComboMod;
        }


        // Call the Function
        if (theFunc)
        {
            if (buttonId < 0)
            {
                //theFunc(val, state, NULL);
                CallFunc(theFunc, val, state, NULL);
            }
            else
            {
                //theFunc(val, state, OTWDriver.pCockpitManager->GetButtonPointer(buttonId));
                CallFunc(theFunc, val, state, OTWDriver.pCockpitManager->GetButtonPointer(buttonId));

                if (SimDriver.GetPlayerAircraft() and 
                    SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and 
 not ((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
                {
                    OTWDriver.pCockpitManager->Dispatch(buttonId, mouseSide);
                }
            }
        }
    }
}

// #20: remap a joystick buttonId by the saved device GUID to the current enumeration
// DirectInput. buttonId = joyIndex*128 + localBtn (see sijoy.cpp:307). If a device
// with this GUID is connected now -- recompute joyIndex; otherwise keep as is
// (device absent -- no harm done). Analogous to RemapAxisMappingByGUID for axes.
static int RemapJoyButtonIdByGUID(int savedButtonId, const GUID& savedGUID)
{
    static const GUID zero = {0};

    if (memcmp(&savedGUID, &zero, sizeof(GUID)) == 0)
        return savedButtonId;

    int local = savedButtonId % SIMLIB_MAX_DIGITAL;

    for (int i = SIM_JOYSTICK1; i < SIM_NUMDEVICES; ++i)
    {
        if (memcmp(&gDIDevGUIDs[i], &savedGUID, sizeof(GUID)) == 0)
            return (i - SIM_JOYSTICK1) * SIMLIB_MAX_DIGITAL + local;
    }

    return savedButtonId;   // device is not connected right now
}

// #20: extract "GUID=<32 hex>" from a .key line. Returns true and writes 16 bytes to out.
static bool ParseLineDeviceGUID(const char* line, GUID* out)
{
    const char* gp = strstr(line, "GUID=");

    if ( not gp)
        return false;

    gp += 5;
    unsigned char* b = (unsigned char*)out;

    for (int i = 0; i < (int)sizeof(GUID); ++i)
    {
        unsigned int v;

        if (sscanf(gp + i * 2, "%2x", &v) not_eq 1)
            return false;

        b[i] = (unsigned char)v;
    }

    return true;
}

void LoadFunctionTables(_TCHAR *fname)
{
    // #53: bindings are loaded from the active profile's XML (config\profiles\<active>\...)
    // instead of keystrokes.key. fname is ignored (the profile comes from profiles.xml).
    (void)fname;

    // Artscout - 2026: make sure the active profile reflects the persisted last-selected pilot
    // (profiles.xml <active>) before we read any bindings - otherwise startup/options would load
    // the "default" profile until the logbook UI is opened.
    ControlsXml_RestoreActiveProfile();

    // --- keyboard: keyboard.xml ---
    static CxKbBind kb[1200];
    int nkb = ControlsXml_ReadKeyboard(kb, 1200);

    for (int i = 0; i < nkb; i++)
    {
        if (kb[i].k2 < 0)
            continue;   // entry without a key

        InputFunctionType f = FindFunctionFromString(kb[i].func);

        if (not f)
            continue;

        int flags = kb[i].m2 + (kb[i].k1 << SECOND_KEY_SHIFT) + (kb[i].m1 << SECOND_KEY_MOD_SHIFT);
        UserFunctionTable.AddFunction(kb[i].k2, flags, kb[i].cpbtn, kb[i].mouse, f);
    }

    // --- devices: <GUID>.xml ONLY for those found during enumeration (gDIDevButtons>0) ---
    static const GUID zeroGuid = {0};
    static CxBtnBind bb[512];
    char guidStr[2 * sizeof(GUID) + 1];

    for (int dev = SIM_JOYSTICK1; dev < SIM_NUMDEVICES; dev++)
    {
        if (gDIDevButtons[dev] <= 0)
            continue;

        if (memcmp(&gDIDevGUIDs[dev], &zeroGuid, sizeof(GUID)) == 0)
            continue;

        const unsigned char *gbytes = (const unsigned char *)&gDIDevGUIDs[dev];

        for (int k = 0; k < (int)sizeof(GUID); k++)
            sprintf(guidStr + k * 2, "%02X", gbytes[k]);

        guidStr[2 * sizeof(GUID)] = 0;

        int nb = ControlsXml_ReadDevice(guidStr, bb, 512);
        int slotOff = dev - SIM_JOYSTICK1;

        for (int i = 0; i < nb; i++)
        {
            InputFunctionType f = FindFunctionFromString(bb[i].func);

            if (not f)
                continue;

            if (bb[i].isPov)
            {
                UserFunctionTable.SetPOVFunction(bb[i].id, bb[i].dir, f, bb[i].cpbtn);
            }
            else
            {
                // per-GUID file: remap to the current device slot (local button + slot offset)
                int newId = slotOff * SIMLIB_MAX_DIGITAL + (bb[i].id % SIMLIB_MAX_DIGITAL);
                UserFunctionTable.SetButtonFunction(newId, f, bb[i].cpbtn);
            }
        }
    }

    //Wombat778 10-07-2003 Load scroll wheel functions. Added these here because I need to be 100% sure that keys have been loaded.
    scrollupfunc = FindFunctionFromString(g_strScrollUpFunction);
    scrolldownfunc = FindFunctionFromString(g_strScrollDownFunction);
    middlebuttonfunc = FindFunctionFromString(g_strMiddleButtonFunction);

    // #53: load the prebuilt XML function catalog (C++ name -> BMS label [+category]).
    // The file config\controls.xml is generated OFFLINE (tools\gen_controls_xml.py); the sim only reads.
    ControlsXml_LoadCatalog();
}

void InputBuildString(unsigned long i)
{
    switch (i)
    {
        case DIK_RETURN: // Enter
        case DIK_NUMPADENTER:
            CommandsKeyCombo = 0;
            CommandsKeyComboMod = 0;
            break;

        case DIK_BACK:
            memmove(&chatterStr[chatterCount - 1], &chatterStr[chatterCount],
                    MAX_CHAT_LENGTH - chatterCount);
            chatterCount --;
            break;

        case DIK_DELETE:
            memmove(&chatterStr[chatterCount], &chatterStr[chatterCount + 1],
                    MAX_CHAT_LENGTH - chatterCount - 1);
            chatterCount --;
            break;

        case DIK_LEFT:
            if (chatterCount)
                chatterCount --;

            break;

        case DIK_RIGHT:
            if (chatterCount < strlen(chatterStr))
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
                    memmove(&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
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
                    memmove(&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
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
                    memmove(&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
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
                    memmove(&chatterStr[chatterCount + 1], &chatterStr[chatterCount],
                            MAX_CHAT_LENGTH - chatterCount - 1);
                }

                chatterStr[chatterCount] = '-';
                chatterCount ++;
            }

            break;
    }
}

// Handles any Printable character
void StandardAsciiInput(unsigned long key, int state)
{
    unsigned long i;
    char asciival;

    asciival = AsciiChar(key, state);

    switch (AsciiAllowed)
    {
        case 1: // Integers
            if ( not DIK_IsDigit(key, state) and asciival not_eq '-')
                return;

            break;

        case 2: // Floats
            if ( not DIK_IsDigit(key, state) and asciival not_eq '-' and asciival not_eq '.')
                return;

            break;
    }

    if (chatterCount < MaxInputLength)
    {
        if (insertMode)
        {
            // move rest of chars to the right
            for (i = MaxInputLength - 2; i >= chatterCount; i--)
                chatterStr[i + 1] = chatterStr[i];

            chatterStr[MaxInputLength] = 0;
        }

        if ( not chatterStr[chatterCount])
            chatterStr[chatterCount + 1] = 0;

        chatterStr[chatterCount] = asciival;
        chatterCount++;
    }
}

// Handles NON ascii stuff like END, CURSOR keys, HOME etc.
// Can handle ALT,SHIFT,CTRL flags... if you implement it
void ExtendedKeyInput(unsigned long key, int)
{
    static unsigned long i;

    switch (key)
    {
        case DIK_NUMPAD6:
            if (chatterCount < MaxInputLength and chatterStr[chatterCount])
            {
                chatterCount++;
            }

            break;

        case DIK_NUMPAD4:
            if (chatterCount > 0)
            {
                chatterCount--;
            }

            break;

        case DIK_NUMPAD0:
            // insert mode
            break;

        case DIK_NUMPAD7:
            if (chatterCount > 0)
            {
                chatterCount = 0;
            }

            break;

        case DIK_INSERT:
            insertMode xor_eq 1;
            break;

        case DIK_NUMPAD1:
            while (chatterStr[chatterCount] and chatterCount < (MaxInputLength - 1))
                chatterCount++;

            break;

        case DIK_DECIMAL:
            if (chatterCount >= (MaxInputLength))
                break;

            if (chatterStr[chatterCount])
            {
                i = chatterCount + 1;

                if (i < MaxInputLength)
                {
                    while (chatterStr[i] and i < MaxInputLength)
                    {
                        chatterStr[i - 1] = chatterStr[i];
                        i++;
                    }

                    chatterStr[i - 1] = 0;
                }
            }

            break;

        case DIK_BACK:
            if (chatterCount > 0)
            {
                chatterCount--;

                for (i = chatterCount; i < MaxInputLength; i++)
                    chatterStr[i] = chatterStr[i + 1];

                chatterStr[MaxInputLength] = 0;
            }

            break;

        case DIK_RETURN:

            // Use it
            if (UseInputFn)
                (*UseInputFn)();

            chatterCount = 0;
            CommandsKeyCombo = 0;
            CommandsKeyComboMod = 0;
            memset(chatterStr, 0, sizeof(chatterStr));
            UseInputFn = NULL;
            DiscardInputFn = NULL;
            OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitand compl SHOW_CHATBOX);
            break;

        case DIK_ESCAPE:

            // Discard it
            if (DiscardInputFn)
                (*DiscardInputFn)();

            chatterCount = 0;
            CommandsKeyCombo = 0;
            CommandsKeyComboMod = 0;
            memset(chatterStr, 0, sizeof(chatterStr));
            UseInputFn = NULL;
            DiscardInputFn = NULL;
            OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitand compl SHOW_CHATBOX);
            break;
    }
}

#ifdef HASH_TEST

int main(void)
{
    InitDebug(DEBUGGER_TEXT_MODE);
    LoadFunctionTables();
    return 0;
}
#endif
