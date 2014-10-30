#pragma optimize( "", off )
#include <windows.h>
#include <io.h>

#include "FalcLib.h"
#include "F4thread.h"
#include "resource.h"
#include "ui95/chandler.h"
#include "ui95/cthook.h"
#include "Graphics/Include/loader.h"
#include "ACMIUI.h"
#include "ui/include/userids.h"
#include "ui/include/textids.h"
#include "sim/include/misctemp.h"

#include "graphics/include/renderow.h"
#include "graphics/include/drawbsp.h"
#include "graphics/include/drawpole.h"
#include "graphics/include/drawbsp.h"
#include "Acmihash.h"


#include "codelib/tools/lists/lists.h"
#include "AcmiTape.h"
#include "AcmiView.h"

#include "Graphics/DXEngine/DXVBManager.h"
extern bool g_bUse_DX_Engine;

void DelVHSFileCB(long ID, short hittype, C_Base *control);
void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void CloseAllRenderers(long openID);

extern bool g_bHiResUI;
extern int g_nACMIOptionsPopupHiResX;
extern int g_nACMIOptionsPopupHiResY;
extern int g_nACMIOptionsPopupLowResX;
extern int g_nACMIOptionsPopupLowResY;
extern bool g_bEmptyFilenameFix; // 2002-04-18 MN

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#define listBoxBaseID 1
#define MAX_FLT_FILES 50

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// set in acmiui.cpp and referenced in acmitape.cpp
int TESTBUTTONPUSH = 0;

// we need synchronization between ui events and callbacks for exec processing
F4CSECTIONHANDLE* gUICriticalSection = NULL;


typedef struct
{
    char fltName[40];
} fltFiles[MAX_FLT_FILES];


extern int
ACMILoaded;

BOOL
acmiDraw = FALSE,
renderACMI = FALSE;

C_Window
*acmiRenderWin;

ACMIView
*acmiView = NULL;

float
lastHorz = 0.0F,
lastVert = 0.0F;

C_TimerHook
*drawTimer;



C_Slider *gFrameMarker;
int gFrameMarkerMin;
int gFrameMarkerMax;
int gFrameMarkerLen;

C_Text *gCounter;

int gTrailLen = 0;
int gDoWingTrails = 0;
BOOL gDoPoles = TRUE;
BOOL gDoLockLines = FALSE;
BOOL gDoWireFrame = FALSE;
float gObjScale = 1.0f;
int gCameraMode = INTERNAL_CAM;

char gCountText[64];

extern char FalconDataDirectory[_MAX_PATH];

extern C_Handler
*gMainHandler;

extern C_Parser
*gMainParser;

BOOL gAdjustingFrameMarker = FALSE;

///////////////////////////

//BING 4/5/988

char loadedfname[MAX_PATH];
int globalcheck = 0;
int globaloffsetmsecs3 = 0;

///////////////////////////

ACMI_Hash *ACMIIDTable = NULL;

enum
{
    ACMI_LOAD_SCREEN = 200149,
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

extern void ProcessEventArray(C_Window *win, void *events, int count);

void FindACMIFLTFiles();
void HookupACMIControls(long ID);

static void ViewTimerCB(long ID, short hittype, C_Base *control);
void MoveACMIViewTimerCB(long ID, short hittype, C_Base *control);
void ACMIDrawCB(long ID, short hittype, C_Base *control);

void ACMIButtonCB(long ID, short hittype, C_Base *control);
void ACMICloseCB(long ID, short hittype, C_Base *control);
void CloseWindowCB(long ID, short hittype, C_Base *control);

void ACMILoadCB(long ID, short hittype, C_Base *control);
void ACMIFrameMarkerCB(long ID, short hittype, C_Base *control);
void ACMIStopCB(long ID, short hittype, C_Base *control);
void ACMIPlayCB(long ID, short hittype, C_Base *control);
void ACMIPlayBackwardsCB(long ID, short hittype, C_Base *control);
void ACMIStepFowardCB(long ID, short hittype, C_Base *control);
void ACMIStepReverseCB(long ID, short hittype, C_Base *control);
void ACMIRewindCB(long ID, short hittype, C_Base *control);
void ACMIFastForwardCB(long ID, short hittype, C_Base *control);
void ACMIRotateCameraUpCB(long ID, short hittype, C_Base *control);
void ACMIRotateCameraDownCB(long ID, short hittype, C_Base *control);
void ACMIRotateCameraLeftCB(long ID, short hittype, C_Base *control);
void ACMIRotateCameraRightCB(long ID, short hittype, C_Base *control);
void ACMIZoomInCameraCB(long ID, short hittype, C_Base *control);
void ACMIZoomOutCameraCB(long ID, short hittype, C_Base *control);
void ACMITrackingCB(long ID, short hittype, C_Base *control);
//void ACMIWingTrailCB(long ID,short hittype,C_Base *control);
//void ACMIWingTrailIncCB(long ID,short hittype,C_Base *control);
//void ACMIWingTrailDecCB(long ID,short hittype,C_Base *control);
void ACMIPannerCB(long ID, short hittype, C_Base *control);
void ACMIHArrowsCB(long ID, short hittype, C_Base *control);
void ACMIVArrowsCB(long ID, short hittype, C_Base *control);
void ACMICameraCB(long ID, short hittype, C_Base *control);
void ACMICamTrackingCB(long ID, short hittype, C_Base *control);
void ACMICamTrackingPrevCB(long ID, short hittype, C_Base *control);
void ACMICamTrackingNextCB(long ID, short hittype, C_Base *control);
void ACMISubCameraCB(long ID, short hittype, C_Base *control);
void ACMISubCameraPrevCB(long ID, short hittype, C_Base *control);
void ACMISubCameraNextCB(long ID, short hittype, C_Base *control);
void ACMIPickAFileCB(long ID, short hittype, C_Base *control);
void ACMIUpdate(long ID, short hittype, C_Base *control);
void ACMIDraw(long ID, short hittype, C_Base *control);
void ACMIScreenCaptureCB(long ID, short hittype, C_Base *control);
void ACMICutPOVCB(long ID, short hittype, C_Base *control);
void ACMIUpdateModelMenu();
// PJW void ACMITransportButton( int but );
void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename);

void InitACMIIDTable()
{
    if (ACMIIDTable)
    {
        ACMIIDTable->Cleanup();
        delete ACMIIDTable;
        ACMIIDTable = NULL;
    }

    ACMIIDTable = new ACMI_Hash;
    ACMIIDTable->Setup(100);
}

void CleanupACMIIDTable()
{
    if (ACMIIDTable)
    {
        ACMIIDTable->Cleanup();
        delete ACMIIDTable;
        ACMIIDTable = NULL;
    }
}

void InitACMIMenus()
{
    C_PopupList *menu;

    menu = gPopupMgr->GetMenu(ACMI_OPTION_POPUP);

    if (menu)
    {
        menu->SetItemState(OPT_ALT_POLE, (short)DrawablePoled::drawPole);
        menu->SetItemState(OPT_LOCK_LINE, (short)gDoLockLines);
        menu->SetItemState(OPT_WIRE_TERRAIN, (short)gDoWireFrame);

        menu->SetItemState(WING_TRAILS_NONE, 0);
        menu->SetItemState(WING_TRAILS_SHORT, 0);
        menu->SetItemState(WING_TRAILS_MEDIUM, 0);
        menu->SetItemState(WING_TRAILS_LONG, 0);
        menu->SetItemState(WING_TRAILS_MAX, 0);

        if ( not gDoWingTrails)
            menu->SetItemState(WING_TRAILS_NONE, 1);
        else
        {
            if (gTrailLen == ACMI_TRAILS_SHORT)
                menu->SetItemState(WING_TRAILS_SHORT, 1);

            if (gTrailLen == ACMI_TRAILS_MEDIUM)
                menu->SetItemState(WING_TRAILS_MEDIUM, 1);

            if (gTrailLen == ACMI_TRAILS_LONG)
                menu->SetItemState(WING_TRAILS_LONG, 1);

            if (gTrailLen == ACMI_TRAILS_MAX)
                menu->SetItemState(WING_TRAILS_MAX, 1);
        }

        menu->SetItemState(LABEL_NAME, (short)DrawableBSP::drawLabels);
        menu->SetItemState(LABEL_AIRSPEED, (short)DrawablePoled::drawSpeed);
        menu->SetItemState(LABEL_ALTITUDE, (short)DrawablePoled::drawAlt);
        menu->SetItemState(LABEL_HEADING, (short)DrawablePoled::drawHeading);
        menu->SetItemState(LABEL_LOCK_RANGE, (short)DrawablePoled::drawlockrange);
        // menu->SetItemState(LABEL_PITCH,0);
        // menu->SetItemState(LABEL_G,0);
        menu->SetItemState(LABEL_TURN_RATE, (short)DrawablePoled::drawTurnRate);
        menu->SetItemState(LABEL_TURN_RADIUS, (short)DrawablePoled::drawTurnRadius);

        menu->SetItemState(VEH_SIZE_1, 0);
        menu->SetItemState(VEH_SIZE_2, 0);
        menu->SetItemState(VEH_SIZE_3, 0);
        menu->SetItemState(VEH_SIZE_4, 0);
        menu->SetItemState(VEH_SIZE_5, 0);

        if (gObjScale == 1.0f)
            menu->SetItemState(VEH_SIZE_1, 1);

        if (gObjScale == 2.0f)
            menu->SetItemState(VEH_SIZE_2, 1);

        if (gObjScale == 4.0f)
            menu->SetItemState(VEH_SIZE_3, 1);

        if (gObjScale == 8.0f)
            menu->SetItemState(VEH_SIZE_4, 1);

        if (gObjScale == 16.0f)
            menu->SetItemState(VEH_SIZE_5, 1);
    }
}

void CloseACMI()
{
    C_Window *win;
    C_Button *btn;

    if (acmiView)
    {
        // RED - Loader must be released after Objects release
        //TheLoader.Cleanup();
        TheVbManager.Release();

        win = gMainHandler->FindWindow(ACMI_RIGHT_WIN);

        if (win)
        {
            btn = (C_Button*)win->FindControl(ACMI_CLOSE);

            if (btn)
                ACMICloseCB(CLOSE_WINDOW, C_TYPE_LMOUSEUP, btn);
        }
    }
}


// win->SetVirtualY(txt->GetY()-win->ClientArea_[0].top,0);
// win->AdjustScrollbar(0);
// win->RefreshClient(0);

/*
** FindUITextEvent
** Description:
** Returns The Text control in the event list based on the time
** we've passed in.  These times were set up when loading the text
** events.  slot should always be 0 in our case.
*/
C_Base *FindUITextEvent(C_Window *win, long slot, long time)
{
    CONTROLLIST *cur;
    C_Base *found = NULL;

    if ( not win or not time)
        return(NULL);

    cur = win->GetControlList();

    while (cur)
    {
        if (cur->Control_)
        {
            if (cur->Control_->GetUserNumber(slot) >= time)
            {
                return cur->Control_;
                //found=cur->Control_;
            }
        }

        cur = cur->Next;
    }

    return(found);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void TogglePoleCB(long ID, short hittype, C_Base *control)
{
    int temp;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    temp = ((C_PopupList *)control)->GetItemState(ID);

    if (acmiView not_eq NULL)
    {
        acmiView->TogglePoles(temp);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ToggleLockLineCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gDoLockLines = ((C_PopupList *)control)->GetItemState(ID);

    if (acmiView not_eq NULL)
    {
        acmiView->ToggleLockLines(gDoLockLines);
    }

}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ToggleWireFrameCB(long ID, short, C_Base *control)
{
    C_Window *win;
    F4CSECTIONHANDLE *Leave;

    // TOGGLE NAME LABELS HERE
    gDoWireFrame = ((C_PopupList *)control)->GetItemState(ID);

    if (acmiView not_eq NULL)
    {
        win = gMainHandler->FindWindow(ACMI_RENDER_WIN);

        if (win == NULL)
            return;

        // order here is significant
        // we must unload the tape, shut down graphics, restart
        // graphics and reload tape to do the toggle...

        // edg: this is bad.  However we must not do this during an
        // acmiView->Exec() cycle.  Question for Peter: how do we handle
        // the timer callback to do a mutex properly?
        Leave = UI_Enter(win);
        acmiDraw = FALSE;
        //Sleep( 200 );

        // get the tape head position
        int currpos = gFrameMarker->GetSliderPos();
        float pct = (float)(currpos - gFrameMarkerMin) / (float)gFrameMarkerLen;

        // stop the tape
        acmiView->Tape()->Pause();
        // ACMITransportButton( STOP_BUTTON );

        acmiView->UnloadTape(TRUE);
        acmiView->ExitGraphics();
        acmiView->ToggleWireFrame(gDoWireFrame);
        acmiView->InitGraphics(win);

        if ( not acmiView->LoadTape("", TRUE))
        {
            // something's fucked
        }

        // restore setting for wing trails on tape
        // restore tape head positiion
        acmiView->Tape()->SetHeadPosition(pct);
        acmiView->Tape()->SetWingTrails(gDoWingTrails);
        acmiView->Tape()->SetWingTrailLength(gTrailLen);
        acmiView->Tape()->SetObjScale(gObjScale);

        // restore drawing
        acmiDraw = TRUE;
        UI_Leave(Leave);

    }

    control->Parent_->SetGroupState(200001, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ToggleWingTrailsCB(long ID, short, C_Base*)
{


    switch (ID)
    {
        case WING_TRAILS_NONE:
        {
            gDoWingTrails = 0;
            acmiView->Tape()->SetWingTrails(gDoWingTrails);
            break;

        }

        case WING_TRAILS_SHORT:
        {
            if ( not gDoWingTrails)
            {
                gDoWingTrails = TRUE;
                acmiView->Tape()->SetWingTrails(gDoWingTrails);
            }

            gTrailLen = ACMI_TRAILS_SHORT;  // MLR 12/22/2003 - now in seconds
            acmiView->Tape()->SetWingTrailLength(gTrailLen);
            MonoPrint("WingTrails Short \n");
            break;
        }

        case WING_TRAILS_MEDIUM:
        {
            if ( not gDoWingTrails)
            {
                gDoWingTrails = TRUE;
                acmiView->Tape()->SetWingTrails(gDoWingTrails);
            }

            gTrailLen = ACMI_TRAILS_MEDIUM;  // MLR 12/22/2003 - now in seconds
            acmiView->Tape()->SetWingTrailLength(gTrailLen);
            MonoPrint("WingTrails Medium \n");
            break;
        }

        case WING_TRAILS_LONG:
        {
            if ( not gDoWingTrails)
            {
                gDoWingTrails = TRUE;
                acmiView->Tape()->SetWingTrails(gDoWingTrails);
            }

            gTrailLen = ACMI_TRAILS_LONG;  // MLR 12/22/2003 - now in seconds
            acmiView->Tape()->SetWingTrailLength(gTrailLen);
            MonoPrint("WingTrails Long \n");
            break;
        }

        case WING_TRAILS_MAX:
        {
            if ( not gDoWingTrails)
            {
                gDoWingTrails = TRUE;
                acmiView->Tape()->SetWingTrails(gDoWingTrails);
            }

            //gTrailLen = 2000; // MN I want to have them even a bit longer  ;-) (1000 before...)
            gTrailLen = ACMI_TRAILS_MAX;  // MLR 12/22/2003 - now in seconds
            acmiView->Tape()->SetWingTrailLength(gTrailLen);
            MonoPrint("WingTrails Max \n");
            break;
        }


    }


}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////


void ToggleFirstSelectionOfOptionsCB(long ID, short hittype, C_Base *control)
{

    // C_Button *btn;
    C_Window *win;

    win = control->Parent_;
    // int buttonstate=0;
    int temp = 0;


    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    switch (ID)
    {
        case LABEL_NAME:
        {
            // TOGGLE NAME LABELS HERE
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->ToggleLabel(temp);
            }

            break;
        }

        case LABEL_AIRSPEED:
        {
            // TOGGLE AIRSPEED HERE
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->ToggleAirSpeed(temp);
            }

            break;
        }

        case LABEL_ALTITUDE:
        {
            // TOGGLE ALTITUDE HERE
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->ToggleAltitude(temp);
            }

            break;
        }

        case LABEL_HEADING:
        {
            // TOGGLE HEADING HERE
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->ToggleHeading(temp);
            }

            break;
        }

        case LABEL_TURN_RATE:
        {
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->ToggleTurnRate(temp);
            }


            break;
        }


        case LABEL_TURN_RADIUS:
        {
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->ToggleTurnRadius(temp);
            }


            break;
        }

        case LABEL_LOCK_RANGE:
        {
            // TOGGLE HEADING HERE
            temp = ((C_PopupList *)control)->GetItemState(ID);

            if (acmiView not_eq NULL)
            {
                acmiView->Togglelockrange(temp);
            }

            break;
        }
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////


void ToggleObjScaleCB(long ID, short hittype, C_Base*)
{

    if ( not acmiView or not acmiView->Tape() or hittype not_eq C_TYPE_LMOUSEUP)
        return;

    switch (ID)
    {
        case VEH_SIZE_2:
            gObjScale = 2.0f;
            acmiView->Tape()->SetObjScale(2.0f);
            break;

        case VEH_SIZE_3:
            gObjScale = 4.0f;
            acmiView->Tape()->SetObjScale(4.0f);
            break;

        case VEH_SIZE_4:
            gObjScale = 8.0f;
            acmiView->Tape()->SetObjScale(8.0f);
            break;

        case VEH_SIZE_5:
            gObjScale = 16.0f;
            acmiView->Tape()->SetObjScale(16.0f);
            break;

        default:
        case VEH_SIZE_1:
            gObjScale = 1.0f;
            acmiView->Tape()->SetObjScale(1.0f);
            break;
    }

}



///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////

void ACMItoggleLABELSCB(long, short hittype, C_Base *control)
{

    C_PopupList *menu;
    short x, y;
    short w, h;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    menu = gPopupMgr->GetMenu(ACMI_OPTION_POPUP);

    menu->GetWindowSize(&w, &h);

    if (g_bHiResUI)
    {
        x = g_nACMIOptionsPopupHiResX;
        y = g_nACMIOptionsPopupHiResY;
    }
    else
    {
        x = g_nACMIOptionsPopupLowResX;
        y = g_nACMIOptionsPopupLowResY;
    }

    // x = 500; // btn->GetX()-w+20;
    // y = 500; //btn->GetY()-h+5;
    // x = btn->GetX()-w+20;
    // y = btn->GetY()-h+5;

    gPopupMgr->OpenMenu(ACMI_OPTION_POPUP, x, y, control);

    if (menu)
    {
        menu->SetCallback(LABEL_NAME, ToggleFirstSelectionOfOptionsCB);
        menu->SetCallback(LABEL_ALTITUDE, ToggleFirstSelectionOfOptionsCB);
        menu->SetCallback(LABEL_HEADING, ToggleFirstSelectionOfOptionsCB);
        menu->SetCallback(LABEL_AIRSPEED, ToggleFirstSelectionOfOptionsCB);
        menu->SetCallback(LABEL_TURN_RATE, ToggleFirstSelectionOfOptionsCB);
        menu->SetCallback(LABEL_TURN_RADIUS, ToggleFirstSelectionOfOptionsCB);
        menu->SetCallback(LABEL_LOCK_RANGE, ToggleFirstSelectionOfOptionsCB);

        menu->SetCallback(OPT_ALT_POLE, TogglePoleCB);
        menu->SetCallback(OPT_LOCK_LINE, ToggleLockLineCB);
        menu->SetCallback(OPT_WIRE_TERRAIN, ToggleWireFrameCB);
        menu->SetCallback(WING_TRAILS_NONE, ToggleWingTrailsCB);
        menu->SetCallback(WING_TRAILS_SHORT, ToggleWingTrailsCB);
        menu->SetCallback(WING_TRAILS_MEDIUM, ToggleWingTrailsCB);
        menu->SetCallback(WING_TRAILS_LONG, ToggleWingTrailsCB);
        menu->SetCallback(WING_TRAILS_MAX, ToggleWingTrailsCB);

        menu->SetCallback(VEH_SIZE_1, ToggleObjScaleCB);
        menu->SetCallback(VEH_SIZE_2, ToggleObjScaleCB);
        menu->SetCallback(VEH_SIZE_3, ToggleObjScaleCB);
        menu->SetCallback(VEH_SIZE_4, ToggleObjScaleCB);
        menu->SetCallback(VEH_SIZE_5, ToggleObjScaleCB);

        InitACMIMenus();
    }
}


/*
 C_Button *btn;
 C_Window *win;

 win=control->Parent_;
 int buttonstate=0;
 int temp=0;

 btn=(C_Button *)win->FindControl(ACMI_LABELS);
 if(btn not_eq NULL)
 {
 temp = ((C_Button *)btn)->GetState();

 if(temp > 0)
 {
 if(acmiView not_eq NULL)
 {
 acmiView->ToggleLabel(temp);
 }
 }
 else
 {
 if(acmiView not_eq NULL)
 {
 acmiView->ToggleLabel(temp);
 }
 }

 }
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMI_ImportFile(void)
{
    FILE *fp;
    int y;
    char fname[100];
    char fltname[MAX_PATH];
    HANDLE findHand;
    WIN32_FIND_DATA fData;
    BOOL foundAFile = TRUE;

    // look for *.flt files to import
    findHand = FindFirstFile("acmibin\\acmi*.flt", &fData);

    // find anything?
    if (findHand == INVALID_HANDLE_VALUE)
        return;

    while (foundAFile)
    {
        strcpy(fltname, "acmibin\\");
        strcat(fltname, fData.cFileName);

        // find a suitable name to import to
        for (y = 1; y < 10000; y++)
        {
            sprintf(fname, "acmibin\\TAPE%04d.vhs", y);

            fp = fopen(fname, "r");

            if ( not fp)
            {
                ACMITape::Import(fltname, fname);
                break;
            }
            else
            {
                fclose(fp);
            }
        }

        // get next file
        foundAFile = FindNextFile(findHand, &fData);
    }

    FindClose(findHand);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMI_LoadACMICB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_Text   *text;
    C_ListBox *ACMIListBox, *camFilter;
    char *objectName;
    long objectNum, numEntities, listBoxIds = listBoxBaseID;
    C_EditBox * ebox;
    _TCHAR fname[MAX_PATH];
    C_Window *renwin;

    if ( not acmiView)
        return;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    renwin = gMainHandler->FindWindow(ACMI_RENDER_WIN);

    if (renwin == NULL)
        return;

    acmiDraw = FALSE;


    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        for (unsigned int i = 0; i < _tcslen(fname); i++)
        {
            if (fname[i] == '.')
            {
                fname[i] = 0;
            }
        }

        if (fname[0] == 0)
            return;

        _TCHAR buf[MAX_PATH];
        _stprintf(buf, _T("%s.vhs"), fname);

        sprintf(fname, buf);

        // make sure no tape is now loaded
        acmiView->UnloadTape(FALSE);

        // Load the tape.
        if ( not acmiView->LoadTape(fname, FALSE))
        {
            acmiView->UnloadTape(FALSE);
            return;
        }

        acmiView->Tape()->SetWingTrails(gDoWingTrails);
        acmiView->Tape()->SetWingTrailLength(gTrailLen);
        acmiView->Tape()->SetObjScale(gObjScale);

        //save the filename for future reference.
        sprintf(loadedfname, fname);


        win = gMainHandler->FindWindow(ACMI_RIGHT_WIN);

        if (win)
        {
            text = (C_Text *)win->FindControl(ACMI_TAPE_NAME);

            if (text not_eq NULL)
            {
                text->SetText(fname);
                text->Refresh();
            }
        }

        acmiView->InitUIVector();

        win = gMainHandler->FindWindow(ACMI_LEFT_WIN);

        if (win not_eq NULL)
        {
            camFilter = (C_ListBox *)win->FindControl(ACMI_CAMERA);

            if (camFilter not_eq NULL)
            {
                // Init camera view stuff to inside cockpit view
                camFilter->SetValue(gCameraMode);
                ACMICameraCB(camFilter->GetID(), C_TYPE_SELECT, camFilter);
            }

            numEntities = acmiView->Tape()->NumEntities();

            listBoxIds = listBoxBaseID;

            ACMIListBox = (C_ListBox *)win->FindControl(SUBCAMERA_FIELD);

            if (ACMIListBox not_eq NULL)
            {
                ACMIListBox->RemoveAllItems();

                for (objectNum = 0; objectNum < numEntities; objectNum ++)
                {
                    objectName = acmiView->SetListBoxID(objectNum, listBoxIds);

                    if (*objectName)
                    {
                        ACMIListBox = ACMIListBox->AddItem(listBoxIds, C_TYPE_ITEM , objectName);
                        listBoxIds++;
                    }
                    else
                    {
                        acmiView->SetListBoxID(objectNum, -1);
                    }
                }
            }

            listBoxIds = listBoxBaseID;

            ACMIListBox = (C_ListBox *)win->FindControl(TRACKED_OBJECT_FIELD);

            if (ACMIListBox not_eq NULL)
            {
                ACMIListBox->RemoveAllItems();

                for (objectNum = 0; objectNum < numEntities; objectNum ++)
                {
                    objectName = acmiView->SetListBoxID(objectNum, listBoxIds);

                    if (*objectName)
                    {
                        ACMIListBox = ACMIListBox->AddItem(listBoxIds, C_TYPE_ITEM , objectName);
                        listBoxIds++;
                    }
                    else
                    {
                        acmiView->SetListBoxID(objectNum, -1);
                    }

                }
            }

            // edg
            // this call puts the event strings into the
            // event list window -- seems to be broken now...
            void *events;
            int count;
            events = acmiView->Tape()->GetTextEvents(&count);
            ProcessEventArray(win, events, count);

        }// if win not_eq null
    } // end listbox

    gMainHandler->HideWindow(control->Parent_);

    win = gMainHandler->FindWindow(ACMI_LEFT_WIN);

    if (win)
    {
        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    win = gMainHandler->FindWindow(ACMI_RIGHT_WIN);

    if (win)
    {
        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    renwin->UnHideCluster(100);
    renwin->HideCluster(200);
    gMainHandler->ShowWindow(renwin);
    gMainHandler->WindowToFront(renwin);

    acmiDraw = TRUE;
    renderACMI = TRUE;

    renwin->HideCluster(100);
    renwin->UnHideCluster(200);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMI_SaveItCB(long, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    unsigned int i;
    _TCHAR fname[MAX_PATH];
    _TCHAR fnamedir[MAX_PATH];
    _TCHAR oldpath[MAX_PATH];
    C_Window *renwin;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    renwin = gMainHandler->FindWindow(ACMI_RENDER_WIN);

    if (renwin == NULL)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if ( not win)
        return;

    acmiDraw = FALSE;

    sprintf(fnamedir, "acmibin\\");

    gMainHandler->HideWindow(win);
    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)win->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        for (i = 0; i < _tcslen(fname); i++)
            if (fname[i] == '.')
                fname[i] = 0;

        if (fname[0] == 0)
            return;

        _tcscat(fname, ".vhs");

        strcpy(oldpath, "acmibin\\");
        strcat(oldpath, loadedfname);
        _tcscat(fnamedir, fname);

        // get the tape head position
        int currpos = gFrameMarker->GetSliderPos();
        float pct = (float)(currpos - gFrameMarkerMin) / (float)gFrameMarkerLen;

        // stop the tape
        acmiView->Tape()->Pause();

        // rename won't work unless the tape file is close so unload it
        acmiView->UnloadTape(TRUE);

        // rename/copy file to new name
        CopyFile(oldpath, fnamedir, FALSE);

        // reload the tape
        if ( not acmiView->LoadTape(fname , TRUE))
        {
            // something's fucked
        }

        // restore setting for wing trails on tape
        // restore tape head positiion
        acmiView->Tape()->SetHeadPosition(pct);
        acmiView->Tape()->SetWingTrails(gDoWingTrails);
        acmiView->Tape()->SetWingTrailLength(gTrailLen);
        acmiView->Tape()->SetObjScale(gObjScale);

        C_Window *win = gMainHandler->FindWindow(ACMI_RIGHT_WIN);

        if (win)
        {
            C_Text *text = (C_Text *)win->FindControl(ACMI_TAPE_NAME);

            if (text not_eq NULL)
            {
                text->SetText(fname);
                text->Refresh();
            }
        }
    }

    acmiDraw = TRUE;
}

void ACMI_VerifySaveItCB(long ID, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    _TCHAR fname[MAX_PATH];
    FILE *fp;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(ebox->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix

        _stprintf(fname, "acmibin\\%s.vhs", ebox->GetText());
        fp = fopen(fname, "r");

        if (fp)
        {
            fclose(fp);
            AreYouSure(TXT_WARNING, TXT_FILE_EXISTS, ACMI_SaveItCB, CloseWindowCB);
        }
        else
            ACMI_SaveItCB(ID, hittype, control);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

static void LoadACMIFileCB(long, short hittype, C_Base*)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetDeleteCallback(DelVHSFileCB);
    LoadAFile(TXT_LOAD_ACMI, "acmibin\\*.vhs", NULL, ACMI_LoadACMICB, CloseWindowCB);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

static void SaveACMIFileCB(long, short hittype, C_Base*)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetDeleteCallback(DelVHSFileCB);
    SaveAFile(TXT_SAVE_ACMI, "acmibin\\*.vhs", NULL, ACMI_VerifySaveItCB, CloseWindowCB, "");
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

inline BOOL ACMIViewIsReady()
{
    return
        (
            (
                acmiView not_eq NULL and 
                acmiView->Tape() not_eq NULL and 
                acmiView->Tape()->IsLoaded() and 
                acmiView->TapeHasLoaded()
            ) ?
            TRUE :
            FALSE
        );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIButtonCB(long, short hittype, C_Base*)
{
    C_Window
    *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    CloseAllRenderers(ACMI_RIGHT_WIN);
    // ACMI_ImportFile();


    /// make the following call load window shit...


    gTrailLen = ACMI_TRAILS_SHORT; // MLR 12/22/2003 - Was 100
    // sprintf( gTrailLenText, "%d", gTrailLen );

    // create the critical sect
    /*
    if ( gUICriticalSection == NULL )
     gUICriticalSection = F4CreateCriticalSection();
    */

    if ( not ACMILoaded)
    {
        LoadACMIWindows();
    }

    if (acmiView == NULL)
    {
        win = gMainHandler->FindWindow(ACMI_RENDER_WIN);

        if (win == NULL)
            return;

        acmiView = new ACMIView;
        acmiView->ToggleWireFrame(gDoWireFrame);
        acmiView->TogglePoles(gDoPoles);
        acmiView->ToggleLockLines(gDoLockLines);
        acmiView->InitGraphics(win);
    }

    // LoadAFile("acmibin\\*.vhs",NULL,ACMI_LoadACMICB,CloseWindowCB);
    SetDeleteCallback(DelVHSFileCB);
    LoadAFile(TXT_LOAD_ACMI, "acmibin\\*.vhs", NULL, ACMI_LoadACMICB, ACMICloseCB);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void LoadACMIWindows()
{
    long
    id;

    if ( not ACMILoaded)
    {
        gMainParser->LoadImageList("ac_art.lst");
        // gImageMgr->SetAllKeys(UI95_RGB24Bit(0x00ff00ff));
        gMainParser->LoadSoundList("ac_snd.lst");
        gMainParser->LoadWindowList("ac_scf.lst"); // Modified by M.N. - add art/art1024 by LoadWindowList

        id = gMainParser->GetFirstWindowLoaded();

        while (id)
        {
            HookupACMIControls(id);
            id = gMainParser->GetNextWindowLoaded();
        }

        ACMILoaded++;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void FindACMIFLTFiles()
{
    long
    handle;

    int
    retVal = 0;

    _finddata_t
    *fileinfo;

    C_Window
    *win;

    C_Button
    *tbtn;

    int
    y,
    ui_id;

    C_ScrollBar
    *scroll;

    // first check to see if we should import any files.
    // ACMI_ImportFile();

    win = gMainHandler->FindWindow(ACMI_LOAD_WIN);

    if (win == NULL)
        return;


    fileinfo = new _finddata_t;
    handle = _findfirst("acmibin\\*.vhs", fileinfo);
    //handle = _findfirst("Campaign\\save\\fltfiles\\*.vhs", fileinfo );
    // handle = _findfirst("acmibin\\*.vhs", fileinfo );

    if (handle > 0)
    {
        ui_id = 700;
        y = 2;

        tbtn = new C_Button;
        tbtn->Setup(ui_id++, C_TYPE_TOGGLE, 10, y);
        tbtn->SetFlagBitOn(C_BIT_ENABLED);
        tbtn->SetText(C_STATE_0, fileinfo->name);
        tbtn->SetText(C_STATE_1, fileinfo->name);
        tbtn->SetFont(win->Font_);
        tbtn->SetFgColor(C_STATE_0, 0x0f0f0f0);
        tbtn->SetFgColor(C_STATE_1, 0x00fffff);
        tbtn->SetCallback(ACMIPickAFileCB);
        y += gFontList->GetHeight(win->Font_);
        win->AddControl(tbtn);

        while (retVal not_eq -1)
        {
            retVal = _findnext(handle, fileinfo);

            if (retVal not_eq -1)
            {
                tbtn = new C_Button;
                tbtn->Setup(ui_id++, C_TYPE_TOGGLE, 10, y);
                tbtn->SetFlagBitOn(C_BIT_ENABLED);
                tbtn->SetText(C_STATE_0, fileinfo->name);
                tbtn->SetText(C_STATE_1, fileinfo->name);
                tbtn->SetFont(win->Font_);
                tbtn->SetFgColor(C_STATE_0, 0x0f0f0f0);
                tbtn->SetFgColor(C_STATE_1, 0x0ffffff);
                tbtn->SetCallback(ACMIPickAFileCB);
                y += gFontList->GetHeight(win->Font_);
                win->AddControl(tbtn);
            }
        }

        scroll = (C_ScrollBar *)win->FindControl(FILE_SCROLL);

        if (scroll not_eq NULL)
        {
            scroll->SetVirtualH(y);
        }

        if (ui_id > 700)
        {
            win->RefreshWindow();
        }
    }

    delete fileinfo;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void HookupACMIControls(long ID)
{
    C_Panner
    *panner;

    C_Window
    *winme;

    C_Button
    *ctrl;

    C_TimerHook
    *tmr;

    C_ListBox
    *ACMIListBox;

    C_Slider *sctrl;
    C_Text *tctrl;

    ////////////////////////////////////////////////////////////////////////////
    /*
     C_PopupList *menu;


    // caller=gPopupMgr->GetCallingControl();
    // if(menu == NULL)
    // return;

     menu=gPopupMgr->GetMenu(ACMI_OPTION_POPUP);

     if(menu)
     {
     // Legend stuff
     menu->SetCallback(OPT_LABELS,ToggleNamesCB);
     // menu->SetCallback(MID_LEG_CIRCLES,MenuToggleCirclesCB);
     // menu->SetCallback(MID_LEG_BOUND,MenuToggleTroupBoundariesCB);
     // menu->SetCallback(MID_LEG_MOVE,MenuToggleMovementArrowsCB);
     }
    */

    ///////////////////////////////////////////////////////////////////////////////////


    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Hook up IDs here...Hook up Main Buttons...
    ctrl = (C_Button *)winme->FindControl(ACMI_CLOSE);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMICloseCB);
    }

    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(CloseWindowCB);
    }

    ctrl = (C_Button *)winme->FindControl(FILE_LOAD_BUTTON);

    if (ctrl not_eq NULL)
    {
        //ctrl->SetCallback(ACMILoadCB);
        ctrl->SetCallback(LoadACMIFileCB);

    }

    ctrl = (C_Button *)winme->FindControl(STOP);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIStopCB);
        //gTransport[ STOP_BUTTON ] = ctrl;
    }

    ctrl = (C_Button *)winme->FindControl(PLAY);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIPlayCB);
        //gTransport[ PLAY_BUTTON ] = ctrl;
    }

    ctrl = (C_Button *)winme->FindControl(REVERSE);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIPlayBackwardsCB);
        //gTransport[ REV_BUTTON ] = ctrl;
    }

    ctrl = (C_Button *)winme->FindControl(FRAME_STEP_FORWARD);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIStepFowardCB);
        //gTransport[ SPLAY_BUTTON ] = ctrl;
    }

    ctrl = (C_Button *)winme->FindControl(FRAME_STEP_BACK);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIStepReverseCB);
        //gTransport[ SREV_BUTTON ] = ctrl;
    }

    ctrl = (C_Button *)winme->FindControl(FASTREVERSE);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIRewindCB);
        //gTransport[ FF_BUTTON ] = ctrl;
    }

    ctrl = (C_Button *)winme->FindControl(FASTFORWARD);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIFastForwardCB);
    }

    ctrl = (C_Button *)winme->FindControl(TRACKING_CHECK_CTRL);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMITrackingCB);
    }

    // ctrl = (C_Button *)winme->FindControl(TRAIL_CTRL);
    // if(ctrl not_eq NULL)
    // {
    // ctrl->SetCallback(ACMIWingTrailCB);
    // }
    //
    // ctrl = (C_Button *)winme->FindControl(TRAIL_INC);
    // if(ctrl not_eq NULL)
    // {
    // ctrl->SetCallback(ACMIWingTrailIncCB);
    // }

    // ctrl = (C_Button *)winme->FindControl(TRAIL_DEC);
    // if(ctrl not_eq NULL)
    // {
    // ctrl->SetCallback(ACMIWingTrailDecCB);
    // }

    panner = (C_Panner *)winme->FindControl(YAW_PITCH_ARROWS);

    if (panner not_eq NULL)
    {
        panner->SetCallback(ACMIPannerCB);
    }

    panner = (C_Panner *)winme->FindControl(OTHER_YAW_PITCH_ARROWS);

    if (panner not_eq NULL)
    {
        panner->SetCallback(ACMIPannerCB);
    }

    panner = (C_Panner *)winme->FindControl(H_ARROWS);

    if (panner not_eq NULL)
    {
        panner->SetCallback(ACMIHArrowsCB);
    }

    panner = (C_Panner *)winme->FindControl(ACMI_ZOOMER);

    if (panner not_eq NULL)
    {
        panner->SetCallback(ACMIHArrowsCB);
    }

    panner = (C_Panner *)winme->FindControl(V_ARROWS);

    if (panner not_eq NULL)
    {
        panner->SetCallback(ACMIVArrowsCB);
    }

    ACMIListBox = (C_ListBox *)winme->FindControl(ACMI_CAMERA);

    if (ACMIListBox not_eq NULL)
    {
        ACMIListBox->SetCallback(ACMICameraCB);
    }

    ACMIListBox = (C_ListBox *)winme->FindControl(SUBCAMERA_FIELD);

    if (ACMIListBox not_eq NULL)
    {
        ACMIListBox->SetCallback(ACMISubCameraCB);
    }

    ctrl = (C_Button *)winme->FindControl(PREV_SUBCAMERA_CTRL);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMISubCameraPrevCB);
    }

    ctrl = (C_Button *)winme->FindControl(NEXT_SUBCAMERA_CTRL);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMISubCameraNextCB);
    }

    ACMIListBox = (C_ListBox *)winme->FindControl(TRACKED_OBJECT_FIELD);

    if (ACMIListBox not_eq NULL)
    {
        ACMIListBox->SetCallback(ACMICamTrackingCB);
    }

    ctrl = (C_Button *)winme->FindControl(PREV_TRACK_CTRL);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMICamTrackingPrevCB);
    }

    ctrl = (C_Button *)winme->FindControl(NEXT_TRACK_CTRL);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMICamTrackingNextCB);
    }

    ACMIListBox = (C_ListBox *)winme->FindControl(SUBCAMERA_FIELD);

    if (ACMIListBox not_eq NULL)
    {
        ACMIListBox->SetCallback(ACMISubCameraCB);
    }

    ctrl = (C_Button *)winme->FindControl(ADD_POV);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMIScreenCaptureCB);
    }

    ctrl = (C_Button *)winme->FindControl(CUT_POV);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMICutPOVCB);
    }

    sctrl = (C_Slider *)winme->FindControl(FRAME_MARKER);

    if (sctrl not_eq NULL)
    {
        gFrameMarker = sctrl;
        gFrameMarker->SetCallback(ACMIFrameMarkerCB);
        gFrameMarkerMin = gFrameMarker->GetSliderMin();
        gFrameMarkerMax = gFrameMarker->GetSliderMax();
        gFrameMarkerLen = gFrameMarkerMax - gFrameMarkerMin;
    }

    tctrl = (C_Text *)winme->FindControl(COUNTER);

    if (tctrl not_eq NULL)
    {
        gCounter = tctrl;
    }

    /*
    tctrl = (C_Text *)winme->FindControl(TRAIL_FIELD);
    if(tctrl not_eq NULL)
    {
     gTrailLenCtrl = tctrl;
    }
    */


    /// ACMI LABEL TOGGLE - GNU OPTIONS BUTTONS... LABELS ARE INSIDE THIS..
    ctrl = (C_Button *)winme->FindControl(ACMI_LABELS);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(ACMItoggleLABELSCB);
        ctrl->SetState(0);
    }


    /// ACMI SAVE AS...
    ctrl = (C_Button *)winme->FindControl(FILE_SAVE_BUTTON);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(SaveACMIFileCB);
        ctrl->SetState(0);
    }


    if (ID == ACMI_RENDER_WIN)
    {
        drawTimer = new C_TimerHook;
        drawTimer->Setup(C_DONT_CARE, C_TYPE_NORMAL);
        drawTimer->SetUpdateCallback(ViewTimerCB);
        drawTimer->SetDrawCallback(ACMIDrawCB);
        drawTimer->SetFlagBitOn(C_BIT_ABSOLUTE);
        drawTimer->SetFlagBitOff(C_BIT_TIMER);
        drawTimer->SetFlagBitOn(C_BIT_INVISIBLE);
        drawTimer->SetCluster(200);
        drawTimer->SetReady(1);
        winme->AddControlTop(drawTimer);

        tmr = new C_TimerHook;
        tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
        tmr->SetUpdateCallback(MoveACMIViewTimerCB);
        tmr->SetFlagBitOn(C_BIT_INVISIBLE);
        tmr->SetCluster(200);
        tmr->SetUserNumber(_UI95_TIMER_DELAY_, 10);
        tmr->SetFlagBitOn(C_BIT_ABSOLUTE);
        winme->AddControlTop(tmr);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

static void ViewTimerCB(long, short, C_Base *control)
{
    // F4EnterCriticalSection( gUICriticalSection );
    control->SetReady(1);
    control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
    control->Parent_->RefreshWindow();
    // F4LeaveCriticalSection( gUICriticalSection );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void MoveACMIViewTimerCB(long, short, C_Base *control)
{
    C_Window *win;
    float pct;
    int newpos;
    int mins, secs, msecs, hrs;

    float Simseconds = 0.0f;

    // F4EnterCriticalSection( gUICriticalSection );

    if (control->GetUserNumber(_UI95_TIMER_COUNTER_) < 100)
    {

        // should probably check to see if the tape is loaded...

        if (ACMIViewIsReady())
        {

            Simseconds = acmiView->Tape()->SimTime();

            acmiView->Exec();
            TheLoader.WaitForLoader();

            pct = acmiView->Tape()->SimTime() - acmiView->Tape()->GetTodOffset();
            secs = (int)(pct);
            msecs = (int)(pct * 100) - secs * 100;

            // edg: yuck  I assume Bing did this....
            // SimTime() (as now time stamped on tape) includes the
            // time of day in seconds.  Assuming the Sim Starts at Noon gives
            // 12 hrs * 60 secs/hr = 720 which is why the following subtraction is in.
            // A bad assumption.  We'll need to record the todOffset in the raw data
            // file so we can get it into tthe tape header
            // mins = (int)(pct/60.0f)-720;

            mins = (int)((pct) / 60.0f);
            hrs = (int)((mins) / 60.0f);
            mins = (int)(fmod((float)mins, 60.0f));

            secs = (int)(fmod(pct, 60.0f));

            if (gAdjustingFrameMarker == FALSE)
            {
                sprintf(gCountText,
                        "%02d:%02d:%02d:%02d",
                        hrs,
                        mins,
                        secs,
                        msecs);

                gCounter->Refresh();
                gCounter->SetText(gCountText);
                gCounter->Refresh();


                pct = acmiView->Tape()->GetTapePercent();
                newpos = gFrameMarkerMin + (int)((float)gFrameMarkerLen * pct);
                gFrameMarker->Refresh();
                gFrameMarker->SetSliderPos((short)newpos);
                gFrameMarker->Refresh();

                if (newpos == gFrameMarkerMax)
                {
                    win = gMainHandler->FindWindow(ACMI_LEFT_WIN);

                    if (win)
                        win->SetGroupState(200001, 0); // Turn off all VCR buttons

                }
            }

            // handle moving the events list when tape running
            if ( not acmiView->Tape()->IsPaused())
            {
                int intTime = (int)(acmiView->Tape()->SimTime() - acmiView->Tape()->GetTodOffset()) * 1000;
                win = gMainHandler->FindWindow(ACMI_LEFT_WIN);

                if (win)
                {
                    C_Text *txt = (C_Text *)FindUITextEvent(win, 0, intTime);

                    if (txt)
                    {
                        win->SetVirtualY(txt->GetY() - win->ClientArea_[0].top, 0);
                        win->AdjustScrollbar(0);
                        win->RefreshClient(0);
                    }
                }
            }
        }

        control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_DELAY_));
        control->Parent_->update_ or_eq C_DRAW_REFRESHALL;
        control->Parent_->RefreshWindow();
    }

    control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_COUNTER_) - 1);
    // F4LeaveCriticalSection( gUICriticalSection );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIDrawCB(long, short, C_Base*)
{
    /*
    ** EDG NOTE: There's really no reason (that I could descern) to have
    ** AcmiView->Exec() and ->Draw() done in 2 different callbacks.
    ** Therefore this callback is now commented out and Draw() is called
    ** directly from Exec().  This callback can be removed completely
    */

    if (ACMIViewIsReady())
    {
        acmiView->Draw();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMILoadCB(long, short hittype, C_Base *control)
{
    C_Window
    *win;

    // F4EnterCriticalSection( gUICriticalSection );
    if (hittype == C_TYPE_LMOUSEUP)
    {
        acmiDraw = FALSE;
        renderACMI = FALSE;

        win = gMainHandler->FindWindow(ACMI_RENDER_WIN);
        win->HideCluster(200);
        win->UnHideCluster(100);

        FindACMIFLTFiles();

        gMainHandler->EnableWindowGroup(control->GetGroup());
    }

    // F4LeaveCriticalSection( gUICriticalSection );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICloseCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;
    C_Window *renwin;


    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    renwin = gMainHandler->FindWindow(ACMI_RENDER_WIN);

    if (renwin == NULL)
        return;

    // critical section for drawing
    Leave = UI_Enter(renwin);
    acmiDraw = FALSE;
    renderACMI = FALSE;

    gMainHandler->HideWindow(control->Parent_);


    drawTimer->SetFlagBitOn(C_BIT_INVISIBLE);

    if (acmiView not_eq NULL)
    {

        if (acmiView->TapeHasLoaded())
        {
            acmiView->UnloadTape(FALSE);
        }

        CloseACMI();
        acmiView->ExitGraphics();
        delete acmiView;
        acmiView = NULL;


    }

    if (control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(control->GetGroup());
    }

    UI_Leave(Leave);

    // RV - RED - We r leaving ACMI... why still true?
    // acmiDraw = TRUE;
    // renderACMI = TRUE;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIStopCB(long, short hittype, C_Base *control)
{
    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->Tape()->Pause();
        // ACMITransportButton( STOP_BUTTON );
    }

    control->Parent_->SetGroupState(200001, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIPlayCB(long, short hittype, C_Base *control)
{
    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->Tape()->Pause();
        acmiView->Tape()->SetPlayVelocity(1.0);
        acmiView->Tape()->SetPlayAcceleration(0.0);
        acmiView->Tape()->SetMaxPlaySpeed(1.0);
        acmiView->Tape()->Play();
        // ACMITransportButton( PLAY_BUTTON );
    }

    control->Parent_->SetGroupState(200001, 0);
    control->SetState(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIPlayBackwardsCB(long, short hittype, C_Base *control)
{
    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->Tape()->Pause();
        acmiView->Tape()->SetPlayVelocity(-1.0);
        acmiView->Tape()->SetPlayAcceleration(0.0);
        acmiView->Tape()->SetMaxPlaySpeed(1.0);
        acmiView->Tape()->Play();
        // ACMITransportButton( REV_BUTTON );
    }

    control->Parent_->SetGroupState(200001, 0);
    control->SetState(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIStepFowardCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;

    if ((hittype == C_TYPE_LMOUSEUP or hittype == C_TYPE_REPEAT) and ACMIViewIsReady())
    {
        Leave = UI_Enter(control->Parent_);
        acmiView->Tape()->Pause();
        acmiView->Tape()->StepTime(0.1F);
        UI_Leave(Leave);
    }

    control->Parent_->SetGroupState(200001, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIStepReverseCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;

    if ((hittype == C_TYPE_LMOUSEUP or hittype == C_TYPE_REPEAT) and ACMIViewIsReady())
    {
        Leave = UI_Enter(control->Parent_);
        acmiView->Tape()->Pause();
        acmiView->Tape()->StepTime(-0.1F);
        UI_Leave(Leave);
    }

    control->Parent_->SetGroupState(200001, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIRewindCB(long, short hittype, C_Base *control)
{
    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->Tape()->Pause();
        acmiView->Tape()->SetPlayVelocity(-1.0);
        acmiView->Tape()->SetPlayAcceleration(-1.0);
        acmiView->Tape()->SetMaxPlaySpeed(8.0);
        acmiView->Tape()->Play();
        // ACMITransportButton( REV_BUTTON );
    }

    control->Parent_->SetGroupState(200001, 0);
    control->SetState(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIFastForwardCB(long, short hittype, C_Base *control)
{
    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->Tape()->Pause();
        acmiView->Tape()->SetPlayVelocity(1.0);
        acmiView->Tape()->SetPlayAcceleration(1.0);
        acmiView->Tape()->SetMaxPlaySpeed(8.0);
        acmiView->Tape()->Play();
        // ACMITransportButton( FF_BUTTON );
    }

    control->Parent_->SetGroupState(200001, 0);
    control->SetState(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIRotateCameraUpCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIRotateCameraDownCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIRotateCameraLeftCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIRotateCameraRightCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////


void ACMIZoomInCameraCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIZoomOutCameraCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMITrackingCB(long, short hittype, C_Base*)
{
    if
    (
        hittype == C_TYPE_LMOUSEUP and 
        ACMIViewIsReady()
    )
    {
        acmiView->ToggleTracking();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////
void ACMIPannerCB(long, short hittype, C_Base *control)
{
    C_Panner
    *panner;

    float
    horz = 0.0F,
    vert = 0.0F;


    if ((hittype == C_TYPE_LMOUSEDOWN or hittype == C_TYPE_REPEAT) and ACMIViewIsReady())
    {
        panner = (C_Panner *) control;
        horz = (float)panner->GetHRange();
        vert = (float)panner->GetVRange();

        //scale them so's we don't rotate too fast....
        acmiView->SetPannerAzEl(horz * 0.003F, -vert * 0.003F);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIHArrowsCB(long, short hittype, C_Base *control)
{
    C_Panner
    *panner;

    float
    horz = 0.0F,
    vert = 0.0F;


    if ((hittype == C_TYPE_LMOUSEDOWN or hittype == C_TYPE_REPEAT) and ACMIViewIsReady())
    {
        panner = (C_Panner *) control;
        horz = (float)panner->GetHRange();
        vert = (float)panner->GetVRange();

        // note vert = -X and horz = Y
        acmiView->SetPannerXYZ(-vert, horz, 0.0f);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIVArrowsCB(long, short hittype, C_Base *control)
{
    C_Panner
    *panner;

    float
    horz = 0.0F,
    vert = 0.0F;


    if ((hittype == C_TYPE_LMOUSEDOWN or hittype == C_TYPE_REPEAT) and ACMIViewIsReady())
    {
        panner = (C_Panner *) control;
        horz = (float)panner->GetHRange();
        vert = (float)panner->GetVRange();
        acmiView->SetPannerXYZ(0.0F, 0.0f, vert);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICameraCB(long, short hittype, C_Base *control)
{
    C_ListBox *ACMIListBox;
    C_Button *item;
    C_Window *win;
    long i, cluster;

    if (hittype == C_TYPE_SELECT and ACMIViewIsReady())
    {
        ACMIListBox = (C_ListBox *)control;
        gCameraMode = ACMIListBox->GetTextID();

        acmiView->SelectCamera(gCameraMode);

        item = ACMIListBox->GetItem(gCameraMode);

        if (item)
        {
            win = gMainHandler->FindWindow(ACMI_LEFT_WIN);

            if (win)
            {
                i = 10; // Joe's starting ID for stuff to hide
                cluster = item->GetUserNumber(i);

                while (cluster and i < 20)
                {
                    win->HideCluster(cluster);
                    i++;
                    cluster = item->GetUserNumber(i);
                }

                i = 0; // Joe's starting ID for stuff to enable
                cluster = item->GetUserNumber(i);

                while (cluster and i < 10)
                {
                    win->UnHideCluster(cluster);
                    i++;
                    cluster = item->GetUserNumber(i);
                }
            }
        }

        // ACMIUpdateModelMenu();
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICamTrackingCB(long, int hittype, C_Control *control)
{
    C_ListBox
    *ACMIListBox;

    long
    itemSel;

    ACMIListBox = (C_ListBox *)control;

    if
    (
        hittype == C_TYPE_SELECT and 
        ACMIViewIsReady() and 
        ACMIListBox not_eq NULL
    )
    {
        itemSel = ACMIListBox->GetTextID();

        acmiView->SwitchTrackingObject(itemSel);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICamTrackingCB(long, short hittype, C_Base *control)
{
    C_ListBox
    *ACMIListBox;

    long
    itemSel;

    ACMIListBox = (C_ListBox *)control;

    if
    (
        hittype == C_TYPE_SELECT and 
        ACMIViewIsReady() and 
        ACMIListBox not_eq NULL
    )
    {
        itemSel = ACMIListBox->GetTextID();

        acmiView->SwitchTrackingObject(itemSel);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICamTrackingPrevCB(long, short hittype, C_Base *control)
{
    // C_Window *winme = gMainHandler->FindWindow(ACMI_LEFT_WIN);
    C_ListBox *lbox;
    C_Window *winme = control->Parent_;

    lbox = (C_ListBox *)winme->FindControl(TRACKED_OBJECT_FIELD);

    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->IncrementTrackingObject(-1);

        if (lbox)
        {
            lbox->Refresh();
            lbox->SetValue(acmiView->ListBoxID(acmiView->TrackingObject(), INTERNAL_CAM));
            lbox->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICamTrackingNextCB(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    C_Window *winme = control->Parent_;

    lbox = (C_ListBox *)winme->FindControl(TRACKED_OBJECT_FIELD);

    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->IncrementTrackingObject(1);

        if (lbox)
        {
            lbox->Refresh();
            lbox->SetValue(acmiView->ListBoxID(acmiView->TrackingObject(), INTERNAL_CAM));
            lbox->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMISubCameraCB(long, short hittype, C_Base *control)
{
    C_ListBox
    *ACMIListBox;

    long
    itemSel;

    ACMIListBox = (C_ListBox *)control;

    if
    (
        hittype == C_TYPE_SELECT and 
        ACMIViewIsReady() and 
        ACMIListBox not_eq NULL
    )
    {
        itemSel = ACMIListBox->GetTextID();

        acmiView->SwitchCameraObject(itemSel);
        // ACMIUpdateModelMenu();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMISubCameraPrevCB(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    C_Window *winme = control->Parent_;

    lbox = (C_ListBox *)winme->FindControl(SUBCAMERA_FIELD);

    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->IncrementCameraObject(-1);

        if (lbox)
        {
            lbox->Refresh();
            lbox->SetValue(acmiView->ListBoxID(acmiView->CameraObject(), INTERNAL_CAM));
            lbox->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMISubCameraNextCB(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    C_Window *winme = control->Parent_;

    lbox = (C_ListBox *)winme->FindControl(SUBCAMERA_FIELD);

    if (hittype == C_TYPE_LMOUSEUP and ACMIViewIsReady())
    {
        acmiView->IncrementCameraObject(1);

        if (lbox)
        {
            lbox->Refresh();
            lbox->SetValue(acmiView->ListBoxID(acmiView->CameraObject(), INTERNAL_CAM));
            lbox->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIPickAFileCB(long, short hittype, C_Base *control)
{
    C_Button
    *tbtn;

    // edg: big hack to note: this function doesn't actually
    // seem to set fname to anything that's been loaded in
    // the UI list.  So, for now, the only legal file will
    // be default.  This file will be created automatically
    // from acmi.flt
    _TCHAR
    *fname = "lastflt.vhs";

    C_Window
    *winme;

    C_ListBox
    *ACMIListBox,
    *camFilter;


    C_Text *text;


    char
    *objectName;

    int
    objectNum,
    numEntities,
    listBoxIds = listBoxBaseID;

    long
    camSel;

    F4CSECTIONHANDLE *Leave;
    C_Window *renwin;


    if (hittype not_eq C_TYPE_LMOUSEUP or acmiView == NULL)
        return;

    renwin = gMainHandler->FindWindow(ACMI_RENDER_WIN);

    if (renwin == NULL)
        return;

    // critical section for drawing
    Leave = UI_Enter(renwin);
    acmiDraw = FALSE;



    tbtn = (C_Button *)control;
    tbtn->SetState(1);
    fname = tbtn->GetText(0);
    gMainHandler->HideWindow(control->GetParent());


    gMainHandler->EnableWindowGroup(200149);

    winme = gMainHandler->FindWindow(ACMI_LOAD_SCREEN);


    // make sure no tape is now loaded
    acmiView->UnloadTape(FALSE);

    // Load the tape.
    if ( not acmiView->LoadTape(fname, FALSE))
    {
        acmiView->UnloadTape(FALSE);
        //F4LeaveCriticalSection( gUICriticalSection );
        return;
    }

    acmiView->Tape()->SetWingTrails(gDoWingTrails);
    acmiView->Tape()->SetWingTrailLength(gTrailLen);
    acmiView->Tape()->SetObjScale(gObjScale);


    //save the filename for future reference.
    sprintf(loadedfname, fname);

    acmiView->InitUIVector();

    winme = gMainHandler->FindWindow(ACMI_LEFT_WIN);

    if (winme not_eq NULL)
    {
        camFilter = (C_ListBox *)winme->FindControl(ACMI_CAMERA);

        if (camFilter not_eq NULL)
        {
            camSel = camFilter->GetTextID();
            acmiView->SelectCamera(camSel);
        }

        numEntities = acmiView->Tape()->NumEntities();

        ACMIListBox = (C_ListBox *)winme->FindControl(SUBCAMERA_FIELD);

        if (ACMIListBox not_eq NULL)
        {
            ACMIListBox->RemoveAllItems();

            for (objectNum = 0; objectNum < numEntities; objectNum ++)
            {
                listBoxIds = acmiView->ListBoxID(objectNum, EXTERNAL_CAM);

                if (listBoxIds > -1)
                {
                    objectName = acmiView->SetListBoxID(objectNum, listBoxIds);
                    ACMIListBox = ACMIListBox->AddItem(listBoxIds, C_TYPE_ITEM , objectName);
                }
            }
        }

        listBoxIds = listBoxBaseID;

        ACMIListBox = (C_ListBox *)winme->FindControl(TRACKED_OBJECT_FIELD);

        if (ACMIListBox not_eq NULL)
        {
            ACMIListBox->RemoveAllItems();

            for (objectNum = 0; objectNum < numEntities; objectNum ++)
            {
                objectName = acmiView->SetListBoxID(objectNum, listBoxIds);

                ACMIListBox = ACMIListBox->AddItem(listBoxIds, C_TYPE_ITEM , objectName);
                listBoxIds++;
            }
        }

        // edg
        // this call puts the event strings into the
        // event list window -- seems to be broken now...
        void *events;
        int count;
        events = acmiView->Tape()->GetTextEvents(&count);
        ProcessEventArray(winme, events, count);
    }


    gMainHandler->EnableWindowGroup(200000);
    winme = gMainHandler->FindWindow(ACMI_RENDER_WIN);
    winme->HideCluster(100);
    winme->UnHideCluster(200);



    // put the name of the vhs file into the window top.
    winme = gMainHandler->FindWindow(ACMI_RIGHT_WIN);

    if (winme not_eq NULL)
    {

        text = (C_Text *)winme->FindControl(ACMI_TAPE_NAME);

        if (text not_eq NULL)
        {
            text->SetText(fname);
            text->Refresh();
        }

    }



    acmiDraw = TRUE;
    renderACMI = TRUE;
    UI_Leave(Leave);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIUpdateModelMenu()
{
    C_Window
    *winme;

    C_ListBox
    *ACMIListBox,
    *camFilter;

    char
    *objectName;

    int
    objectNum,
    numEntities,
    listBoxIds;

    long
    camSel = EXTERNAL_CAM;

    winme = gMainHandler->FindWindow(ACMI_LEFT_WIN);

    if (winme not_eq NULL)
    {
        camFilter = (C_ListBox *)winme->FindControl(ACMI_CAMERA);

        if (camFilter not_eq NULL)
        {
            camSel = camFilter->GetTextID();
        }

        ACMIListBox = (C_ListBox *)winme->FindControl(SUBCAMERA_FIELD);

        if (ACMIListBox not_eq NULL)
        {
            ACMIListBox->RemoveAllItems();

            if (ACMIViewIsReady())
            {
                numEntities = acmiView->Tape()->NumEntities();

                for (objectNum = 0; objectNum < numEntities; objectNum ++)
                {
                    listBoxIds = acmiView->ListBoxID(objectNum, camSel);

                    if (listBoxIds > -1)
                    {
                        objectName = acmiView->SetListBoxID(objectNum, listBoxIds);
                        ACMIListBox = ACMIListBox->AddItem(listBoxIds, C_TYPE_ITEM , objectName);
                    }
                }
            }
        }

        ACMIListBox = (C_ListBox *)winme->FindControl(TRACKED_OBJECT_FIELD);

        if (ACMIListBox not_eq NULL)
        {
            ACMIListBox->RemoveAllItems();

            if (ACMIViewIsReady())
            {
                numEntities = acmiView->Tape()->NumEntities();

                for (objectNum = 0; objectNum < numEntities; objectNum ++)
                {
                    listBoxIds = acmiView->ListBoxID(objectNum, camSel);

                    if (listBoxIds > -1)
                    {
                        objectName = acmiView->SetListBoxID(objectNum, listBoxIds);
                        ACMIListBox = ACMIListBox->AddItem(listBoxIds, C_TYPE_ITEM , objectName);
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIUpdate(long, short, C_Base *control)
{
    if (acmiDraw)
    {
        control->SetReady(1);

        if (ACMIViewIsReady())
        {
            acmiDraw = FALSE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIScreenCaptureCB(long, short hittype, C_Base*)
{
    if
    (
        hittype == C_TYPE_LMOUSEUP and 
        ACMIViewIsReady()
    )
    {
        acmiView->ToggleScreenShot();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMICutPOVCB(long, short, C_Base*)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void ACMIFrameMarkerCB(long, short hittype, C_Base *control)
{
    float pct;
    int currpos;
    float t;
    int secs, msecs, hrs, mins;


    /*
    ** edg: leaving this here -- if we want to be able to update the
    ** tape while we're moving the slider, then I think we're going to need
    ** a critical section.
    */
    F4CSECTIONHANDLE *Leave;



    if ( not ACMIViewIsReady())
        return;

    if (hittype == C_TYPE_MOUSEMOVE)
    {
        Leave = UI_Enter(control->Parent_);
        // tell main draw function not to do frame marker update
        gAdjustingFrameMarker = TRUE;

        // edg: no tape updating while moving slider....
        // set the tape head position
        currpos = gFrameMarker->GetSliderPos();
        t = (float)(currpos - gFrameMarkerMin) / (float)gFrameMarkerLen;
        // acmiView->Tape()->SetHeadPosition( pct );

        pct = acmiView->Tape()->GetNewSimTime(t) - acmiView->Tape()->GetTodOffset();
        secs = (int)(pct);
        msecs = (int)(pct * 100) - secs * 100;

        // edg: yuck!  I assume Bing did this....
        // SimTime() (as now time stamped on tape) includes the
        // time of day in seconds.  Assuming the Sim Starts at Noon gives
        // 12 hrs * 60 secs/hr = 720 which is why the following subtraction is in.
        // A bad assumption.  We'll need to record the todOffset in the raw data
        // file so we can get it into tthe tape header
        // mins = (int)(pct/60.0f)-720;

        mins = (int)((pct) / 60.0f);
        hrs = (int)((mins) / 60.0f);
        mins = (int)(fmod((float)mins, 60.0f));

        secs = (int)(fmod(pct, 60.0f));

        sprintf(gCountText,
                "%02d:%02d:%02d:%02d",
                hrs,
                mins,
                secs,
                msecs);

        gCounter->Refresh();
        gCounter->SetText(gCountText);
        gCounter->Refresh();
        UI_Leave(Leave);
    }
    else if (hittype == C_TYPE_LMOUSEDOWN)
    {
        Leave = UI_Enter(control->Parent_);
        acmiDraw = FALSE;

        acmiView->Tape()->Pause();
        acmiView->Tape()->SetWingTrails(0);

        // restore drawing
        acmiDraw = TRUE;
        UI_Leave(Leave);

        control->Parent_->SetGroupState(200001, 0);
    }
    else if (hittype == C_TYPE_LDROP)
    {
        Leave = UI_Enter(control->Parent_);

        currpos = gFrameMarker->GetSliderPos();
        pct = (float)(currpos - gFrameMarkerMin) / (float)gFrameMarkerLen;
        acmiView->Tape()->SetHeadPosition(pct);
        acmiView->Tape()->SetWingTrails(gDoWingTrails);

        // restore drawing
        acmiDraw = TRUE;
        UI_Leave(Leave);

        // tell main draw function not to do frame marker update
        gAdjustingFrameMarker = FALSE;
    }
}

#if 0 // Burrito?
void ACMITransportButton(int but)
{
    int i;

    for (i = 0; i < NUM_TRANSPORT_BUTTONS; i++)
    {
        // only 1 transport button can be on
        if (i == but)
            gTransport[i]->SetState(1);
        else
            gTransport[i]->SetState(0);
    }


}
#endif
