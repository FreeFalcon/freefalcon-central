#include "stdafx.h"
#include <sstream>
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#include "stdhdr.h"
#include "simdrive.h"
#include "cpmirror.h"
#include "cpres.h"
#include "cpchev.h"
#include "cpmanager.h"
#include "cpobject.h"
#include "cpsurface.h"
#include "cppanel.h"
#include "cplight.h"
#include "button.h"
#include "cpindicator.h"
#include "cpdial.h"
#include "icp.h"
#include "KneeBoard.h"
#include "cpded.h"
#include "cpadi.h"
#include "cpmachasi.h"
#include "dispcfg.h"
#include "cphsi.h"
#include "sinput.h"
#include "cpdigits.h"
#include "inpfunc.h"
#include "Graphics/Include/filemem.h"
#include "Graphics/Include/image.h"
#include "Graphics/Include/TimeMgr.h"
#include "Graphics/Include/TOD.h"
#include "fsound.h"
#include "soundfx.h"
#include "cplift.h"
#include "cpsound.h"
#include "mfd.h"
#include "f4thread.h"
#include "cptext.h"
#include "dispopts.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/renderow.h"
#include "sms.h"
#include "simweapn.h"
#include "cpkneeview.h"
#include "flightData.h"
#include "datadir.h"
#include "hud.h"
#include "commands.h"
#include "ui/include/logbook.h"
#include "falcsnd/voicemanager.h"
#include "aircrft.h"
#include "FalcLib/include/playerop.h"
#include "Dofsnswitches.h"
#include "weather.h"
#include "falcsess.h"

using namespace std;

// argh globals
extern bool cockpit_verifier;
extern bool g_bRealisticAvinoics;
extern bool g_bStartIn3Dpit;  // Cobra
extern bool g_bCockpitAutoScale; //Wombat778 10-24-2003
extern bool g_bRatioHack; //Wombat778 11-03-2003
extern bool g_b2DPitWingFOVFix; //Wombat778 2-25-2004
extern int g_nShow2DPitErrors; //Wombat778 4-11-04
extern bool g_bResizeUsesResMgr; //Wombat778 4-14-04
extern bool g_bUse_DX_Engine; // COBRA - RED
extern float g_fHUDonlySize;
// RV - Biker - We need this for theater switching
extern char FalconCockpitThrDirectory[];

//Cobra TJL 11/08/04
//char cockpitFolder[50];//Cobra
// RV - Biker
char cockpitFolder[_MAX_PATH];

#define menudat "\\menu.dat"

// This controls how often the 2D cockpit lighting is recomputed.
static const float COCKPIT_LIGHT_CHANGE_TOLERANCE = 0.1f;
static int gDebugLineNum;

ImageBuffer *gpTemplateSurface = NULL;
GLubyte *gpTemplateImage = NULL;
GLulong *gpTemplatePalette = NULL;
TemplateInfoClass *TemplateInfo = NULL; //Wombat778 3-30-04 load template info so we can close the actual template file
FlightData cockpitFlightData;

#if DO_HIRESCOCK_HACK
BOOL gDoCockpitHack;
#endif

#ifdef USE_SH_POOLS
MEM_POOL gCockMemPool;
#endif

#ifdef _DEBUG

// Special assert for Snqqpy
#define CockpitMessage(badString, area, lineNum ) \
{                                                                 \
 char buffer[80];    \
 int choice;    \
 \
 sprintf( buffer, "Cockpit Error in %s at line %0d - bad string is %s", area, lineNum, badString);\
 choice = MessageBox(NULL, buffer, "Problem:  ",   \
 MB_ICONERROR bitor MB_ABORTRETRYIGNORE bitor MB_TASKMODAL); \
 if (choice == IDABORT) { \
 exit(-1); \
 } \
 if (choice == IDRETRY) { \
 __asm int 3 \
 } \
}
#else
#define CockpitMessage(A,B,C)
#endif

//Wombat778 4-11-04 Helper function to put up an error message in non-debug

#if CPMANAGER_VERSION
//const struct CockpitManager::VersionStruct CURRENT_MANAGER_VERSION = { 1, 0 };
#endif

void CockpitError(int line, int errorpriority)
{
    char temp[150]; //Wombat778 4-22-04 changed to 150

    if (FalconDisplay.displayFullScreen) //we dont want this to run in fullscreen mode because that causes problems
        return;

    //Wombat778 4-11-04 rewrote to be better
    if (errorpriority == 1 and g_nShow2DPitErrors >= errorpriority)
    {
        sprintf(temp, "Critical 2D Cockpit Error Detected in Block Before Line %d.  System Stability May Be Compromised.", line);
        MessageBox(NULL, temp, "Critical Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
    }
    else if (errorpriority == 2 and g_nShow2DPitErrors >= errorpriority)
    {
        sprintf(temp, "2D Cockpit Error Detected in Block Before Line %d.", line);
        MessageBox(NULL, temp, "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
    }



}

void ReadImage(char* pfilename, GLubyte** image, GLulong** palette)
{

    int result;
    int totalWidth;
    CImageFileMemory  texFile;

    // Make sure we recognize this file type
    result = texFile.imageType = CheckImageType(pfilename);
    ShiAssert(texFile.imageType not_eq IMAGE_TYPE_UNKNOWN);

    // Open the input file
    result = texFile.glOpenFileMem(pfilename);
    ShiAssert(result == 1);

    // Read the image data (note that ReadTextureImage will close texFile for us)
    texFile.glReadFileMem();
    result = ReadTextureImage(&texFile);

    if (result not_eq GOOD_READ)
    {
        //sfr: changed message
        std::ostringstream msg;
        msg << "Failed to read image: " << pfilename;
        ShiError(msg.str().c_str());
    }

    // Store the image size (check it in debug mode)
    totalWidth = texFile.image.width;

    *image = texFile.image.image;

    if (palette)
    {
        *palette = texFile.image.palette;
    }
    else
    {
        glReleaseMemory((char*)texFile.image.palette);
    }
}



// This is a helper function for SetTOD which uses the lit 16 bit
// palette to convert from 8 bit data to 16 bit lit data for a
// single image buffer element of the cockpit.
// SCR 3/10/98

void Translate8to16(WORD *pal, BYTE *src, ImageBuffer *image)
{
    int row;
    WORD *tgt;
    WORD *end;
    void *imgPtr;

    ShiAssert(FALSE == F4IsBadReadPtr(pal, 256 * sizeof(*pal)));
    ShiAssert(src);
    ShiAssert(image);

    // OW FIXME
    // ShiAssert( image->PixelSize() == sizeof(*tgt) );

    imgPtr = image->Lock();

    for (row = 0; row < image->targetYres(); row++)
    {
        tgt = (WORD*)image->Pixel(imgPtr, row, 0);
        end = tgt + image->targetXres();

        while (tgt < end)
        {
            *tgt = pal[*src];
            tgt++;
            src++;
        }
    }

    image->Unlock();
}

void Translate8to32(DWORD *pal, BYTE *src, ImageBuffer *image)
{
    int row;
    DWORD *tgt;
    DWORD *end;
    void *imgPtr;

    ShiAssert(FALSE == F4IsBadReadPtr(pal, 256 * sizeof(*pal)));
    ShiAssert(src);
    ShiAssert(image);

    // OW FIXME
    // ShiAssert( image->PixelSize() == sizeof(*tgt) );

    imgPtr = image->Lock();

    for (row = 0; row < image->targetYres(); row++)
    {
        tgt = (DWORD*)image->Pixel(imgPtr, row, 0);
        end = tgt + image->targetXres();

        while (tgt < end)
        {
            *tgt = pal[*src];
            tgt++;
            src++;
        }
    }

    image->Unlock();
}


//====================================================//
// FindToken()
//====================================================//

char* FindToken(char** string, const char* separators)
{

    char* result;
    char* token;

    // find first occurance of something other than separator
    token = _tcsspnp(*string, separators);

    // if there are none, we are probably at the end of string, nothing left to parse
    if (token == NULL)
    {
        // return the position of the terminator as the string
        *string = strchr(*string, '\0');
    }
    else   // we still have tokens to parse
    {

        // starting with the first character other than a separator...
        // find the first separator.
        result = strpbrk(token, separators);

        if (result == NULL)  // found a string no separators
        {
            // return the position of the terminator as the string
            *string = strchr(*string, '\0');
        }
        else   // still have characters to parse
        {

            *result = NULL;
            *string = result + 1;
        }
    }

    return(token);
}

//====================================================//
// CockpitManager::CockpitManager
//====================================================//
CockpitManager::CockpitManager(
#if DO_HIRESCOCK_HACK
    ImageBuffer *pOTWImage,
    char *pCPFile,
    BOOL mainCockpit,
    // sfr separated scales
    float hScale,
    float vScale,
    BOOL doHack,
    Vis_Types eCPVisType,
    TCHAR* eCPName,
    TCHAR* eCPNameNCTR
#else
    ImageBuffer *pOTWImage,
    char *pCPFile,
    BOOL mainCockpit,
    // sfr separated scales
    float hScale,
    float vScale,
#endif
)
{
    //Wombat778 10-06-2003 Changes scale from int to float

    CP_HANDLE* pcockpitDataFile;
    const int lineLen = MAX_LINE_BUFFER - 1;
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* presult;
    BOOL quitFlag  = FALSE;
    int idNum;
    char ptype[16] = "";

#if DO_HIRESCOCK_HACK
    gDoCockpitHack = doHack;
    m_eCPVisType = eCPVisType;

    if (eCPName)
    {
        strcpy(m_eCPName, eCPName);

        for (int i = 0; i < (int)strlen(m_eCPName); i++)
        {
            if (m_eCPName[i] == '/' or m_eCPName[i] == '\\')
            {
                if (i < (int)strlen(m_eCPName) - 1)
                {
                    strcpy(m_eCPName + i, m_eCPName + i + 1);
                    i--;
                }
                else
                    m_eCPName[i] = 0;
            }
        }
    }

    if (eCPNameNCTR)
    {
        strcpy(m_eCPNameNCTR, eCPNameNCTR);

        for (int i = 0; i < (int)strlen(m_eCPNameNCTR); i++)
        {
            if (m_eCPNameNCTR[i] == '/' or m_eCPNameNCTR[i] == '\\')
            {
                if (i < (int)strlen(m_eCPNameNCTR) - 1)
                {
                    strcpy(m_eCPName + i, m_eCPName + i + 1);
                    i--;
                }
                else
                    m_eCPNameNCTR[i] = 0;
            }
        }
    }

#endif

    mpOTWImage = pOTWImage;
    mpOwnship = NULL;

    mpSoundList = NULL;

    ADIGpDevReading = -HORIZONTAL_SCALE;
    ADIGsDevReading = -VERTICAL_SCALE;
    mHiddenFlag = FALSE;

    mHScale = hScale;
    mVScale = vScale;

    if (mainCockpit)
    {
        mpIcp = new ICPClass;
        mpHsi = new CPHsi;
        mpKneeBoard = new KneeBoard;
    }
    else
    {
        mpIcp = NULL;
        mpHsi = NULL;
        mpKneeBoard = NULL;
    }

    mSurfaceTally = 0;
    mPanelTally = 0;
    mObjectTally = 0;
    mButtonTally = 0;
    mCursorTally = 0;
    mButtonViewTally = 0;
    mNumSurfaces      = 0;
    mNumPanels        = 0;
    mNumObjects       = 0;
    mNumCursors       = 0;
    mNumButtons       = 0;
    mNumButtonViews   = 0;

    lightLevel = 1.0F;
    //sfr: added flood and instrumentation lights
    mFloodLight[0] = 0.35F;
    mFloodLight[1] = 0.15F;
    mFloodLight[2] = 0.15F;
    mInstLight[0] = 1.0F;
    mInstLight[1] = 0.15F;
    mInstLight[2] = 0.15F;

    mCycleBit = BEGIN_CYCLE;
    mIsInitialized = FALSE;
    mIsNextInitialized = FALSE;
    mpGeometry = FALSE;
    mHudFont           = 0;
    mMFDFont           = 0;
    mDEDFont  = 0;
    mGeneralFont  = 0;
    mPopUpFont  = 0;
    mKneeFont  = 0;
    mSABoxFont  = 0;
    mLabelFont         = 0;
    mAltPanel = 0; //Wombat778 4-12-04
    memset(&PitTurbulence, 0x00, sizeof(PitTurbulence));


    char strCPFile[MAX_PATH];

    // RV - Biker - Use fallback
    //m_eCPVisType = (Vis_Types)FindCockpit(pCPFile, eCPVisType, eCPName, eCPNameNCTR, strCPFile);
    m_eCPVisType = (Vis_Types)FindCockpit(pCPFile, eCPVisType, eCPName, eCPNameNCTR, strCPFile, TRUE);

    pCPFile = strCPFile;

    pcockpitDataFile = CP_OPEN(pCPFile, "r");

    if ( not pcockpitDataFile)
    {
        if (mainCockpit)
        {
            // Try to use the 8x6 and doHack instead
            pcockpitDataFile = CP_OPEN(COCKPIT_FILE_8x6, "r");
        }
        else
        {
            pcockpitDataFile = CP_OPEN(PADLOCK_FILE_8x6, "r");
        }

        gDoCockpitHack = TRUE;
    }

    F4Assert(pcockpitDataFile); //Error: Couldn't open file
    gDebugLineNum = 0;

    // Load Buffer creation for DEMO release
    mpLoadBuffer = NULL;

    while ( not quitFlag)
    {
        presult = fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        quitFlag = (presult == NULL);

        if ((*plineBuffer == '#') and ( not quitFlag))
        {
            sscanf((plineBuffer + 1), "%d %s", &idNum, ptype);

            if ( not strcmpi(ptype, TYPE_MANAGER_STR))
            {
                ParseManagerInfo(pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_TEXT_STR))
            {
                CreateText(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_CHEVRON_STR))
            {
                CreateChevron(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_LIFTLINE_STR))
            {
                CreateLiftLine(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_SURFACE_STR))
            {
                CreateSurface(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_PANEL_STR))
            {
                CreatePanel(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_LIGHT_STR))
            {
                CreateLight(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_BUTTON_STR))
            {
                CreateButton(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_BUTTONVIEW_STR))
            {
                CreateButtonView(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_INDICATOR_STR))
            {
                CreateIndicator(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_DIGITS_STR))
            {
                CreateDigits(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_ADI_STR))
            {
                CreateAdi(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_DIAL_STR))
            {
                CreateDial(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_CURSOR_STR))
            {
                CreateCursor(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_DED_STR))
            {
                CreateDed(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_MACHASI_STR))
            {
                CreateMachAsi(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_HSI_STR))
            {
                CreateHsiView(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_SOUND_STR))
            {
                CreateSound(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_KNEEVIEW_STR))
            {
                CreateKneeView(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_MIRROR_STR))
            {
                CreateMirror(idNum, pcockpitDataFile);
            }
            else if ( not strcmpi(ptype, TYPE_BUFFER_STR))
            {
                LoadBuffer(pcockpitDataFile);
            }
            else
            {
                CockpitMessage(ptype, "Top Level", gDebugLineNum);
            }
        }
    }

    CP_CLOSE(pcockpitDataFile);

    glReleaseMemory((char*) mpLoadBuffer);
    mpLoadBuffer = NULL;

    ResolveReferences();

    if (mpSoundList)
    {
        delete mpSoundList;
    }

    mpActivePanel = NULL; // first panel in list
    mpNextActivePanel = NULL;
    mViewChanging = TRUE;

    // Initialize the lighting conditions and register for future time of day updates
    TimeUpdateCallback(this);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);

    mpCockpitCritSec = F4CreateCriticalSection("mpCockpitCritSec");

    if (cockpit_verifier)
    {
        if (mSurfaceTally not_eq mNumSurfaces or
            mObjectTally not_eq mNumObjects or
            mPanelTally not_eq mNumPanels or
            mButtonTally not_eq mNumButtons or
            mButtonViewTally not_eq mNumButtonViews)
        {
            char buf[0x400];
            sprintf(buf, "Verify error detected\n\nNumSurfaces:\t%.3d\t\tSurfaceTally:\t%.3d\nNumObjects:\t%.3d\t\tObjectTally:\t%.3d\nNumPanels:\t%.3d\t\tPanelTally:\t%.3d\nNumButtons:\t%.3d\t\tButtonTally:\t%.3d\nNumButtonViews:\t%.3d\t\tButtonViewTally:\t%.3d\t\n",
                    mNumSurfaces, mSurfaceTally, mNumObjects, mObjectTally, mNumPanels, mPanelTally, mNumButtons, mButtonTally, mNumButtonViews, mButtonViewTally);
            ::MessageBox(NULL, buf, "FreeFalcon Cockpit Verifier", MB_OK bitor MB_SETFOREGROUND);
        }

        else ::MessageBox(NULL, "No errors.", "FreeFalcon Cockpit Verifier", MB_OK bitor MB_SETFOREGROUND);
    }

    //Wombat778 10-18-2003 Hack for 1.25 resolutions
    //setup a black imagebuffer the width of the screen
    //Only do this when not rendering
    /*
    if ( not DisplayOptions.bRender2DCockpit and g_bCockpitAutoScale and g_bRatioHack and ((float) DisplayOptions.DispWidth / (float) DisplayOptions.DispHeight) == 1.25) //Wombat778 11-04-2003 added g_bRatiohack in case user has an actual 1280x1024 pit 10-24-2003 added g_bCockpitAutoScale //so we are in a 1.25 ratio
    {
     RatioBuffer = new ImageBuffer;
     RatioBuffer->Setup(&FalconDisplay.theDisplayDevice,DisplayOptions.DispWidth,FloatToInt32((DisplayOptions.DispHeight-(float)DisplayOptions.DispHeight*0.9375f)+0.5f),SystemMem,None,FALSE);  //Wombat778 10-06 2003 Setup a new imagebuffer.  Should begin as black  //Wombat778 10-24-2003 make the calc the same as later for rounding accuracy
     RatioBuffer->Clear(0x00000000);
    }*/

    //Wombat778 3-30-04 get rid of the template buffers if we dont need them anymore
    if (DisplayOptions.bRender2DCockpit)
    {
        if (gpTemplateSurface)
        {
            gpTemplateSurface->Cleanup();
            delete gpTemplateSurface;
            gpTemplateSurface = NULL;

            glReleaseMemory((char*) gpTemplateImage);
            gpTemplateImage = NULL;
        }
    }


}


//====================================================//
// CockpitManager::~CockpitManager
//====================================================//

CockpitManager::~CockpitManager()
{
    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);

    if (gpTemplateSurface)
    {
        gpTemplateSurface->Cleanup();
        delete gpTemplateSurface;
        gpTemplateSurface = NULL;

        glReleaseMemory((char*) gpTemplateImage);
        gpTemplateImage = NULL;
    }

    //Wombat778 3-30-04 Broke the palette out of above

    if (gpTemplatePalette)
    {
        glReleaseMemory((char*) gpTemplatePalette);
        gpTemplatePalette = NULL;
    }

    //Wombat778 3-30-04 get rid of the templateinfo class
    if (TemplateInfo)
    {
        delete TemplateInfo;
        TemplateInfo = NULL;
    }

    for (int i = 0; i < (int)mpSurfaces.size(); i++)
    {
        if (mpSurfaces[i]) delete mpSurfaces[i];
    }

    mpSurfaces.clear();

    for (int i = 0; i < (int)mpPanels.size(); i++)
    {
        if (mpPanels[i]) delete mpPanels[i];
    }

    mpPanels.clear();

    for (int i = 0; i < (int)mpObjects.size(); i++)
    {
        if (mpObjects[i])
        {
            CPObject *o = mpObjects[i];
            delete o;
        }
    }

    mpObjects.clear();

    for (int i = 0; i < (int)mpButtonObjects.size(); i++)
    {
        if (mpButtonObjects[i]) delete mpButtonObjects[i];
    }

    mpButtonObjects.clear();

    for (int i = 0; i < (int)mpButtonViews.size(); i++)
    {
        if (mpButtonViews[i]) delete mpButtonViews[i];
    }

    mpButtonViews.clear();

    for (int i = 0; i < (int)mpCursors.size(); i++)
    {
        if (mpCursors[i]) delete mpCursors[i];
    }

    mpCursors.clear();

    if (mpIcp)
    {
        delete mpIcp;
        mpIcp = NULL;
    }

    if (mpHsi)
    {
        delete mpHsi;
        mpHsi = NULL;
    }

    if (mpKneeBoard)
    {
        delete mpKneeBoard;
        mpKneeBoard = NULL;
    }

    for (int i = 0; i < BOUNDS_TOTAL; i++)
    {
        if (mpViewBounds[i])
        {
            delete mpViewBounds[i];
        }
    }

    if (mpGeometry)
    {
        DrawableBSP::Unlock(mpGeometry->GetID());
        delete mpGeometry;
        mpGeometry = NULL;
    }

    F4DestroyCriticalSection(mpCockpitCritSec);
    mpCockpitCritSec = NULL; // JB 010108


    //Wombat778 10-18-2003 Kill the temp ratiobuffer
    //Only do this when not rendering

    /*if ( not DisplayOptions.bRender2DCockpit and g_bCockpitAutoScale and g_bRatioHack and ((float) DisplayOptions.DispWidth / (float) DisplayOptions.DispHeight) == 1.25) //Wombat778 10-24-2003 added g_bCockpitAutoScale //so we are in a 1.25 ratio
     delete RatioBuffer;*/

}



//====================================================//
// CockpitManager::SetupControlTemplate
//====================================================//
void CockpitManager::SetupControlTemplate(char* pfileName, int width, int height)
{

    char ptemplateFile[MAX_LINE_BUFFER];

    if ( not gpTemplateSurface)
    {

        gpTemplateSurface = new ImageBuffer;

        // RV - Biker - Use fallback
        //FindCockpit(pfileName, m_eCPVisType, m_eCPName, m_eCPNameNCTR, ptemplateFile);
        FindCockpit(pfileName, m_eCPVisType, m_eCPName, m_eCPNameNCTR, ptemplateFile, TRUE);

        gpTemplateSurface->Setup(&FalconDisplay.theDisplayDevice,
                                 width,
                                 height,
                                 SystemMem,
                                 None);
        gpTemplateSurface->SetChromaKey(0xFFFF0000);

        if (gpTemplatePalette)
            ReadImage(ptemplateFile, &gpTemplateImage, NULL);
        else
            ReadImage(ptemplateFile, &gpTemplateImage, &gpTemplatePalette);

        //Wombat778 3-30-04 Save the template information for later use if the cockpit is rendered. This will save a lot of memory because we can free the imagebuffer itself
        if ( not TemplateInfo)
        {
            TemplateInfo = new TemplateInfoClass;
            TemplateInfo->redShift = gpTemplateSurface->RedShift();
            TemplateInfo->blueShift = gpTemplateSurface->BlueShift();
            TemplateInfo->greenShift = gpTemplateSurface->GreenShift();
            TemplateInfo->dwRBitMask = gpTemplateSurface->RedMask();
            TemplateInfo->dwBBitMask = gpTemplateSurface->BlueMask();
            TemplateInfo->dwGBitMask = gpTemplateSurface->GreenMask();
            TemplateInfo->pixelsize = gpTemplateSurface->PixelSize();
        }

    }
}

//====================================================//
// CockpitManager::ParseManagerInfo
//====================================================//
void CockpitManager::ParseManagerInfo(FILE* pcockpitDataFile)
{

    const int lineLen = MAX_LINE_BUFFER - 1;
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char* ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    char ptemplateFileName[32] = "";
    int i;
    RECT viewBounds;
    int numSounds;
    int     twodpit[2] = { 1196, 1197 };
    bool     dogeometry = false;


    for (i = 0; i < BOUNDS_TOTAL; i++)
    {
        mpViewBounds[i] = NULL;
    }

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

#if CPMANAGER_VERSION

    // sfr: parse version string. Must be first in manager because even manager can have new strings now
    if ( not strcmpi(ptoken, PROP_VERSION_STR))
    {
        ptoken = FindToken(&plinePtr, "=;\n");
        int ret = sscanf(ptoken, "%d.%d", &mVersion.major, &mVersion.minor);

        if (ret <= 1)
        {
            mVersion.minor = 0;
        }

        if (ret == 0)
        {
            // this shouldnt happen, parse error
            mVersion.major = 0;
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }
    else
    {
        mVersion.major = 0;
        mVersion.minor = 0;
    }

#endif

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_NUMSURFACES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mNumSurfaces);
        }
        else if ( not strcmpi(ptoken, PROP_MFDLEFT_STR))
        {
#ifdef USE_SH_POOLS
            mpViewBounds[BOUNDS_MFDLEFT] = (ViewportBounds *)MemAllocPtr(gCockMemPool, sizeof(ViewportBounds), FALSE);
#else
            mpViewBounds[BOUNDS_MFDLEFT] = new ViewportBounds;
#endif

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &viewBounds.top,
                   &viewBounds.left,
                   &viewBounds.bottom,
                   &viewBounds.right);
            ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_MFDLEFT], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mHScale, mVScale);
        }
        else if ( not strcmpi(ptoken, PROP_MFDRIGHT_STR))
        {
#ifdef USE_SH_POOLS
            mpViewBounds[BOUNDS_MFDRIGHT] = (ViewportBounds *)MemAllocPtr(gCockMemPool, sizeof(ViewportBounds), FALSE);
#else
            mpViewBounds[BOUNDS_MFDRIGHT] = new ViewportBounds;
#endif

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &viewBounds.top,
                   &viewBounds.left,
                   &viewBounds.bottom,
                   &viewBounds.right);
            ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_MFDRIGHT], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mHScale, mVScale);
        }
        //Wombat778 4-12-04 Add support for extra in cockpit MFDs
        else if ( not strcmpi(ptoken, PROP_MFD3_STR))
        {
#ifdef USE_SH_POOLS
            mpViewBounds[BOUNDS_MFD3] = (ViewportBounds *)MemAllocPtr(gCockMemPool, sizeof(ViewportBounds), FALSE);
#else
            mpViewBounds[BOUNDS_MFD3] = new ViewportBounds;
#endif

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &viewBounds.top,
                   &viewBounds.left,
                   &viewBounds.bottom,
                   &viewBounds.right);
            ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_MFD3], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mHScale, mVScale);
        }
        //Wombat778 4-12-04 Add support for extra in cockpit MFDs
        else if ( not strcmpi(ptoken, PROP_MFD4_STR))
        {
#ifdef USE_SH_POOLS
            mpViewBounds[BOUNDS_MFD4] = (ViewportBounds *)MemAllocPtr(gCockMemPool, sizeof(ViewportBounds), FALSE);
#else
            mpViewBounds[BOUNDS_MFD4] = new ViewportBounds;
#endif

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &viewBounds.top,
                   &viewBounds.left,
                   &viewBounds.bottom,
                   &viewBounds.right);
            ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_MFD4], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mHScale, mVScale);
        }


        else if ( not strcmpi(ptoken, PROP_HUD_STR))
        {

#ifdef USE_SH_POOLS
            mpViewBounds[BOUNDS_HUD] = (ViewportBounds *)MemAllocPtr(gCockMemPool, sizeof(ViewportBounds), FALSE);
#else
            mpViewBounds[BOUNDS_HUD] = new ViewportBounds;
#endif

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &viewBounds.top,
                   &viewBounds.left,
                   &viewBounds.bottom,
                   &viewBounds.right);

            if (g_fHUDonlySize)
            {
                viewBounds.top -= (viewBounds.bottom - viewBounds.top) * (LONG) g_fHUDonlySize;
                viewBounds.bottom += (viewBounds.bottom - viewBounds.top) * (LONG) g_fHUDonlySize;
                viewBounds.left -= (viewBounds.right - viewBounds.left) * (LONG) g_fHUDonlySize;
                viewBounds.right += (viewBounds.right - viewBounds.left) * (LONG) g_fHUDonlySize;
            }

            ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_HUD], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mHScale, mVScale);
        }
        else if ( not strcmpi(ptoken, PROP_RWR_STR))
        {

#ifdef USE_SH_POOLS
            mpViewBounds[BOUNDS_RWR] = (ViewportBounds *)MemAllocPtr(gCockMemPool, sizeof(ViewportBounds), FALSE);
#else
            mpViewBounds[BOUNDS_RWR] = new ViewportBounds;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(viewBounds.top),
                   &(viewBounds.left),
                   &(viewBounds.bottom),
                   &(viewBounds.right));
            ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_RWR], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mHScale, mVScale);
        }
        else if ( not strcmpi(ptoken, PROP_NUMPANELS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mNumPanels);
        }
        else if ( not strcmpi(ptoken, PROP_NUMOBJECTS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mNumObjects);
        }
        else if ( not strcmpi(ptoken, PROP_NUMSOUNDS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &numSounds);

            if (numSounds > 0)
            {
                mpSoundList = new CPSoundList(numSounds);
            }
            else
            {
                mpSoundList = NULL;
            }
        }
        else if ( not strcmpi(ptoken, PROP_NUMBUTTONS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mNumButtons);
        }
        else if ( not strcmpi(ptoken, PROP_NUMBUTTONVIEWS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mNumButtonViews);
        }
        else if ( not strcmpi(ptoken, PROP_NUMCURSORS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mNumCursors);
        }
        else if ( not strcmpi(ptoken, PROP_MOUSEBORDER_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mMouseBorder);
        }
        else if ( not strcmpi(ptoken, PROP_HUDFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mHudFont);
        }
        else if ( not strcmpi(ptoken, PROP_MFDFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mMFDFont);
        }
        else if ( not strcmpi(ptoken, PROP_DEDFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mDEDFont);
        }
        else if ( not strcmpi(ptoken, PROP_POPFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mPopUpFont);
        }
        else if ( not strcmpi(ptoken, PROP_KNEEFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mKneeFont);
        }
        else if ( not strcmpi(ptoken, PROP_GENFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mGeneralFont);
        }
        else if ( not strcmpi(ptoken, PROP_SAFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mSABoxFont);
        }
        else if ( not strcmpi(ptoken, PROP_LABELFONT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mLabelFont);
        }
        else if ( not strcmpi(ptoken, PROP_TEMPLATEFILE_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mTemplateWidth);

            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mTemplateHeight);

            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%s", ptemplateFileName);

            SetupControlTemplate(ptemplateFileName, mTemplateWidth, mTemplateHeight);
        }
        else if ( not strcmpi(ptoken, PROP_DOGEOMETRY_STR))
        {
            dogeometry = true;
        }
        else if ( not strcmpi(ptoken, PROP_DO2DPIT_STR))
        {
            dogeometry = true;
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d", &twodpit[0], &twodpit[1]);
        }
        //Wombat778 4-13-04 Read in a panel number to use for a special keystroke
        else if ( not strcmpi(ptoken, PROP_ALTPANEL))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &mAltPanel);
        }
        //sfr added for lights. Remember, FreeFalcon uses 0xAABBGGRR format
        else if ( not strcmpi(ptoken, PROP_FLOODLIGHT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            unsigned int tempFlood;
            sscanf(ptoken, "%x", &tempFlood);
            mFloodLight[2] = (float)((tempFlood bitand 0xff0000) >> 16) / 0xff;
            mFloodLight[1] = (float)((tempFlood bitand 0x00ff00) >> 8) / 0xff;
            mFloodLight[0] = (float)((tempFlood bitand 0x0000ff) >> 0) / 0xff;
        }
        else if ( not strcmpi(ptoken, PROP_INSTLIGHT))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            unsigned int tempInst;
            sscanf(ptoken, "%x", &tempInst);
            mInstLight[2] = (float)((tempInst bitand 0xff0000) >> 16) / 0xff;
            mInstLight[1] = (float)((tempInst bitand 0x00ff00) >> 8) / 0xff;
            mInstLight[0] = (float)((tempInst bitand 0x0000ff) >> 0) / 0xff;
        }
        else if ( not strcmpi(ptoken, PROP_HUDCOLOR_STR))
        {
            unsigned int HudColor;
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%x", &HudColor);
            // TheHud->SetHudColor(HudColor);
        }

        // 2000-11-12 ADDED BY S.G. SO COMMENTED LINE DON'T TRIGGER AN ASSERT
        else if ( not strncmp(ptoken, "//", 2)) //Wombat778 4-19-04 converted from strcmpi to strncmp so that comments dont need spaces after
            ;
        // END OF ADDED SECTION
        else
        {
            // OW <- S.G. NO NEED ANYMORE?
            CockpitMessage(ptoken, "Manager", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    if (dogeometry)
    {
        CreateCockpitGeometry(&mpGeometry, twodpit[0], twodpit[1]);
    }
}

//====================================================//
// CockpitManager::CreateSound
//====================================================//
void CockpitManager::CreateSound(int idNum, FILE* pcockpitDataFile)
{

    int entry = 0;
    const int lineLen = MAX_LINE_BUFFER - 1;
    char* plinePtr = NULL;
    char* ptoken = NULL;
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_ENTRY_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d", &entry);
        }
        else
        {
            CockpitMessage(ptoken, "Sound", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    mpSoundList->AddSound(idNum, entry);
}

// sfr: removes invalid characters from folder names and spaces
string RemoveInvalidChars(const string &instr)
{
    string outstr = instr + '\0';
    string invalid_chars = "*/\\";

    std::string::iterator pos = outstr.begin();
    int len = strlen(outstr.c_str());

    for (int i = 0; i < len; ++i)
        if (isspace(*pos) or (invalid_chars.find(*pos) not_eq string::npos))
            outstr.erase(pos); // this increments pos
        else ++pos;

    return outstr;
}

//====================================================//
// CockpitManager::LoadBuffer
//====================================================//
void CockpitManager::LoadBuffer(FILE* pcockpitDataFile)
{
    char psurfaceFile[MAX_LINE_BUFFER];
    char pfileName[20] = "";
    char* plinePtr;
    char* ptoken;
    char plineBuffer[MAX_LINE_BUFFER] = "";
    const int lineLen = MAX_LINE_BUFFER - 1;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};


    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_FILENAME_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%s", pfileName);

            // RV - Biker - Check for valid naming here also
            std::string tmp_eCPName     = RemoveInvalidChars(string(m_eCPName, 15));
            std::string tmp_eCPNameNCTR = RemoveInvalidChars(string(m_eCPNameNCTR, 5));

            if (m_eCPVisType == MapVisId(VIS_F16C))
            {
                //sprintf(psurfaceFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pfileName);
                sprintf(psurfaceFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pfileName);
            }
            else
            {
                sprintf(psurfaceFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(m_eCPVisType), pfileName);

                //if( not ResExistFile(psurfaceFile))
                if ( not FileExists(psurfaceFile))
                {
                    //sprintf(psurfaceFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, m_eCPName, pfileName);
                    sprintf(psurfaceFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, tmp_eCPName.c_str(), pfileName);

                    //if( not ResExistFile(psurfaceFile))
                    if ( not FileExists(psurfaceFile))
                    {
                        //sprintf(psurfaceFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, m_eCPNameNCTR, pfileName);
                        sprintf(psurfaceFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, tmp_eCPNameNCTR.c_str(), pfileName);

                        //if( not ResExistFile(psurfaceFile))
                        if ( not FileExists(psurfaceFile))
                        {
                            // F16C fallback
                            //sprintf(psurfaceFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pfileName);
                            sprintf(psurfaceFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pfileName);
                        }
                    }
                }
            }
        }
        else if ( not strcmpi(ptoken, PROP_BUFFERSIZE_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d", &mLoadBufferWidth, &mLoadBufferHeight);
        }
        else
        {
            CockpitMessage(ptoken, "Template", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    if (mpLoadBuffer)
    {
        glReleaseMemory((char*) mpLoadBuffer);
    }

    ReadImage(psurfaceFile, &mpLoadBuffer, NULL);
}

//====================================================//
// CockpitManager::CreateText
//====================================================//
void CockpitManager::CreateText(int idNum, FILE* pcockpitDataFile)
{
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    int numStrings = 0;


    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_NUMSTRINGS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &numStrings);
        }
        else
        {
            CockpitMessage(ptoken, "Text", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPObject *p = new CPText(&objectInitStr, numStrings);
    ShiAssert(p);

    if (p == NULL) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateChevron
//====================================================//
void CockpitManager::CreateChevron(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    ChevronInitStr chevInitStr;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_PANTILT_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%f %f", &(chevInitStr.pan), &(chevInitStr.tilt));
        }
        else if ( not strcmpi(ptoken, PROP_PANTILTLABEL_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d", &(chevInitStr.panLabel), &(chevInitStr.tiltLabel));
        }
        else
        {
            CockpitMessage(ptoken, "Chevron", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.destRect.top = 0;
    objectInitStr.destRect.left = 0;
    objectInitStr.destRect.bottom = 0;
    objectInitStr.destRect.right = 0;

    objectInitStr.callbackSlot = 8;
    objectInitStr.cycleBits = 0xFFFF;

    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPChevron *p = new CPChevron(&objectInitStr, &chevInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}


//====================================================//
// CockpitManager::CreateLiftLine
//====================================================//
void CockpitManager::CreateLiftLine(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER];
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    LiftInitStr liftInitStr;

    liftInitStr.doLabel = FALSE;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_PANTILT_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%f %f", &(liftInitStr.pan), &(liftInitStr.tilt));
        }
        else if ( not strcmpi(ptoken, PROP_PANTILTLABEL_STR))
        {
            liftInitStr.doLabel = TRUE;

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d", &(liftInitStr.panLabel), &(liftInitStr.tiltLabel));
        }
        else
        {
            CockpitMessage(ptoken, "Lift Line", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.destRect.top = 0;
    objectInitStr.destRect.left = 0;
    objectInitStr.destRect.bottom = 0;
    objectInitStr.destRect.right = 0;

    objectInitStr.callbackSlot = 8;
    objectInitStr.cycleBits = 0xFFFF;

    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPLiftLine *p = new CPLiftLine(&objectInitStr, &liftInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}


//====================================================//
// CockpitManager::CreateDed
//====================================================//

void CockpitManager::CreateDed(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    DedInitStr  dedInitStr;

    //MI fixup
    if ( not g_bRealisticAvionics)
        dedInitStr.color0 = 0xff38e0f8; // default JPO
    else
        dedInitStr.color0 = 0xFF00FF9C;

    dedInitStr.dedtype = DEDT_DED;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &dedInitStr.color0);
        }
        else if ( not strcmpi(ptoken, PROP_DED_TYPE))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            if ( not strcmpi(ptoken, PROP_DED_DED))
            {
                dedInitStr.dedtype = DEDT_DED;
            }
            else if ( not strcmpi(ptoken, PROP_DED_PFL))
            {
                dedInitStr.dedtype = DEDT_PFL;
            }
            else
                CockpitMessage(ptoken, "DED", gDebugLineNum);
        }
        else
        {
            CockpitMessage(ptoken, "DED", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPDed *p = new CPDed(&objectInitStr, &dedInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateCursor
//====================================================//

void CockpitManager::CreateCursor(int idNum, FILE* pcockpitDataFile)
{
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    CursorInitStr cursorInitStruct;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(cursorInitStruct.srcRect.top),
                   &(cursorInitStruct.srcRect.left),
                   &(cursorInitStruct.srcRect.bottom),
                   &(cursorInitStruct.srcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_HOTSPOT_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d", &cursorInitStruct.xhotSpot,
                   &cursorInitStruct.yhotSpot);
        }
        else
        {
            CockpitMessage(ptoken, "Cursor", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    cursorInitStruct.idNum = idNum;
    cursorInitStruct.pOtwImage = mpOTWImage;
    cursorInitStruct.pTemplate = gpTemplateSurface;

    CPCursor *p = new CPCursor(&cursorInitStruct);
    ShiAssert(p);

    if ( not p) return;

    mpCursors.push_back(p);
    mCursorTally++;
}

void CockpitManager::CreateDigits(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;

    DigitsInitStr digitsInitStr = {0};
    ObjectInitStr objectInitStr;
    int destIndex = 0;
    RECT *pdestRects = NULL;
    int srcIndex = 0;
    RECT *psrcRects;

#ifdef USE_SH_POOLS
    psrcRects = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT) * 10, FALSE);
#else
    psrcRects = new RECT[10];
#endif

    objectInitStr.bsrcRect.top = 0;
    objectInitStr.bsrcRect.left = 0;
    objectInitStr.bsrcRect.bottom = 0;
    objectInitStr.bsrcRect.right = 0;
    objectInitStr.bdestRect = objectInitStr.bsrcRect;
    objectInitStr.bsurface = -1;
    digitsInitStr.numDestDigits   = 0;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_NUMDIGITS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &digitsInitStr.numDestDigits);

#ifdef USE_SH_POOLS

            if (digitsInitStr.numDestDigits > 0)
            {
                pdestRects = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT) * digitsInitStr.numDestDigits, FALSE);
            }

#else
            pdestRects = new RECT[digitsInitStr.numDestDigits];
#endif

        }
        else if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            F4Assert(srcIndex < 10);
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(psrcRects[srcIndex].top),
                   &(psrcRects[srcIndex].left),
                   &(psrcRects[srcIndex].bottom),
                   &(psrcRects[srcIndex].right));
            srcIndex++;
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            F4Assert(destIndex < digitsInitStr.numDestDigits);

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(pdestRects[destIndex].top),
                   &(pdestRects[destIndex].left),
                   &(pdestRects[destIndex].bottom),
                   &(pdestRects[destIndex].right));

            destIndex++;
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);
        }
        else
        {
            CockpitMessage(ptoken, "Digit", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    digitsInitStr.psrcRects = psrcRects;
    digitsInitStr.pdestRects = pdestRects;

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;

    //Wombat778 3-22-04 Code for rendering the digits instead of blitting
    //Wombat778 4-11-04 added some error reporting

    if (DisplayOptions.bRender2DCockpit)
    {
        digitsInitStr.sourcedigits = new SourceDigitType[10];

        if (digitsInitStr.sourcedigits)
            for (int i = 0; i < 10; i++)
            {
                //Wombat778 4-13-04 prevent a heap error with pit errors
                if ((psrcRects[i].bottom - psrcRects[i].top) * (psrcRects[i].right - psrcRects[i].left) > 0) //Wombat778 4-22-04 changed from >=  to >
                    digitsInitStr.sourcedigits[i].digit = new BYTE[(psrcRects[i].bottom - psrcRects[i].top) * (psrcRects[i].right - psrcRects[i].left)];
                else
                    digitsInitStr.sourcedigits[i].digit = NULL;

                SafeImageCopy(gpTemplateImage, digitsInitStr.sourcedigits[i].digit, mTemplateHeight, mTemplateWidth, &psrcRects[i]); //wombat778 4-11-04 make safeimagecopy always used
            }
        else
            CockpitError(gDebugLineNum, 1); //Wombat778 4-11-04
    }

    //Wombat778 end

    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;
    objectInitStr.destRect = digitsInitStr.pdestRects[0];
    digitsInitStr.psrcRects = psrcRects;
    digitsInitStr.pdestRects = pdestRects;

    CPDigits *p = new CPDigits(&objectInitStr, &digitsInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

void CockpitManager::CreateIndicator(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char porientationStr[15] = "";
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    IndicatorInitStr indicatorInitStr = {0};
    ObjectInitStr objectInitStr;
    int destIndex = 0;


    objectInitStr.bsrcRect.top = 0;
    objectInitStr.bsrcRect.left = 0;
    objectInitStr.bsrcRect.bottom = 0;
    objectInitStr.bsrcRect.right = 0;
    objectInitStr.bdestRect = objectInitStr.bsrcRect;
    objectInitStr.bsurface = -1;
    indicatorInitStr.calibrationVal = 0;
    indicatorInitStr.pdestRect       = NULL;
    indicatorInitStr.psrcRect        = NULL;
    indicatorInitStr.minPos          = NULL;
    indicatorInitStr.maxPos          = NULL;
    indicatorInitStr.numTapes        = 0;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {
        if ( not strcmpi(ptoken, PROP_MINVAL_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &indicatorInitStr.minVal);
        }
        else if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_MAXVAL_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &indicatorInitStr.maxVal);
        }
        else if ( not strcmpi(ptoken, PROP_MINPOS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &indicatorInitStr.minPos[destIndex]);
        }
        else if ( not strcmpi(ptoken, PROP_MAXPOS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &indicatorInitStr.maxPos[destIndex]);
        }
        else if ( not strcmpi(ptoken, PROP_NUMTAPES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &indicatorInitStr.numTapes);
#ifdef USE_SH_POOLS

            if (indicatorInitStr.numTapes > 0)
            {
                indicatorInitStr.pdestRect = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT) * indicatorInitStr.numTapes, FALSE);
                indicatorInitStr.psrcRect = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT) * indicatorInitStr.numTapes, FALSE);
                indicatorInitStr.minPos = (int *)MemAllocPtr(gCockMemPool, sizeof(int) * indicatorInitStr.numTapes, FALSE);
                indicatorInitStr.maxPos = (int *)MemAllocPtr(gCockMemPool, sizeof(int) * indicatorInitStr.numTapes, FALSE);
            }

#else
            indicatorInitStr.pdestRect = new RECT[indicatorInitStr.numTapes];
            indicatorInitStr.psrcRect = new RECT[indicatorInitStr.numTapes];
            indicatorInitStr.minPos = new int[indicatorInitStr.numTapes];
            indicatorInitStr.maxPos = new int[indicatorInitStr.numTapes];
#endif
        }
        else if ( not strcmpi(ptoken, PROP_CALIBRATIONVAL_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &indicatorInitStr.calibrationVal);
        }
        else if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(indicatorInitStr.psrcRect[destIndex].top),
                   &(indicatorInitStr.psrcRect[destIndex].left),
                   &(indicatorInitStr.psrcRect[destIndex].bottom),
                   &(indicatorInitStr.psrcRect[destIndex].right));
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            F4Assert(destIndex < indicatorInitStr.numTapes);

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(indicatorInitStr.pdestRect[destIndex].top),
                   &(indicatorInitStr.pdestRect[destIndex].left),
                   &(indicatorInitStr.pdestRect[destIndex].bottom),
                   &(indicatorInitStr.pdestRect[destIndex].right));

            objectInitStr.transparencyType = CPOPAQUE;

            if (destIndex == 0)
            {
                for (int tmpVar = 1; tmpVar < indicatorInitStr.numTapes; tmpVar++)
                {
                    indicatorInitStr.maxPos[tmpVar] = indicatorInitStr.maxPos[0];
                    indicatorInitStr.minPos[tmpVar] = indicatorInitStr.minPos[0];
                    indicatorInitStr.psrcRect[tmpVar] = indicatorInitStr.psrcRect[0];
                }
            }

            destIndex++;
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);
        }
        else if ( not strcmpi(ptoken, PROP_ORIENTATION_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%s", porientationStr);

            if ( not strcmpi(porientationStr, PROP_HORIZONTAL_STR))
            {
                indicatorInitStr.orientation = IND_HORIZONTAL;
            }
            else if ( not strcmpi(porientationStr, PROP_VERTICAL_STR))
            {
                indicatorInitStr.orientation = IND_VERTICAL;
            }
            else
            {
                ShiWarning("Bad Orientation String");
            }
        }
        else
        {
            CockpitMessage(ptoken, "Indicator", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    //Wombat778 3-22-04 Code for rendering the indicators instead of blitting
    //Wombat778 4-11-04 added some error reporting

    if (DisplayOptions.bRender2DCockpit)
    {
        //Wombat778 4-13-04 prevent a heap error with pit errors
        if (indicatorInitStr.numTapes >= 0)
            indicatorInitStr.sourceindicator = new SourceIndicatorType[indicatorInitStr.numTapes];
        else
            indicatorInitStr.sourceindicator = NULL;

        if (indicatorInitStr.sourceindicator)
            for (int i = 0; i < indicatorInitStr.numTapes; i++)
            {
                //Wombat778 4-13-04 prevent a heap error with pit errors
                if ((indicatorInitStr.psrcRect[i].bottom - indicatorInitStr.psrcRect[i].top) * (indicatorInitStr.psrcRect[i].right - indicatorInitStr.psrcRect[i].left) > 0) //Wombat778 4-22-04 changed from >=  to >
                    indicatorInitStr.sourceindicator[i].indicator = new BYTE[(indicatorInitStr.psrcRect[i].bottom - indicatorInitStr.psrcRect[i].top) * (indicatorInitStr.psrcRect[i].right - indicatorInitStr.psrcRect[i].left)];
                else
                    indicatorInitStr.sourceindicator[i].indicator = NULL;

                SafeImageCopy(gpTemplateImage, indicatorInitStr.sourceindicator[i].indicator, mTemplateHeight, mTemplateWidth, &indicatorInitStr.psrcRect[i]); //wombat778 4-11-04 make safeimagecopy always used
            }
        else
            CockpitError(gDebugLineNum, 1); //Wombat778 4-11-04
    }

    //Wombat778 end

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;
    objectInitStr.destRect = *indicatorInitStr.pdestRect;

    CPIndicator *p = new CPIndicator(&objectInitStr, &indicatorInitStr);
    ShiAssert(p);

    if ( not p) return;

    delete [] indicatorInitStr.pdestRect;
    delete [] indicatorInitStr.psrcRect;
    delete [] indicatorInitStr.minPos;
    delete [] indicatorInitStr.maxPos;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateDial
//====================================================//

void CockpitManager::CreateDial(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    DialInitStr dialInitStr = {0};
    ObjectInitStr objectInitStr;
    int valuesIndex = 0;
    int pointsIndex = 0;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);
    dialInitStr.ppoints = NULL;
    dialInitStr.pvalues = NULL;
    dialInitStr.IsRendered = false; //Wombat778 3-26-04 the needle isnt rendered unless explicitly stated in the dat file

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_NUMENDPOINTS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &dialInitStr.endPoints);
#ifdef USE_SH_POOLS

            if (dialInitStr.endPoints > 0)
            {
                dialInitStr.ppoints = (float *)MemAllocPtr(gCockMemPool, sizeof(float) * dialInitStr.endPoints, FALSE);
                dialInitStr.pvalues = (float *)MemAllocPtr(gCockMemPool, sizeof(float) * dialInitStr.endPoints, FALSE);
            }

#else
            dialInitStr.ppoints = new float[dialInitStr.endPoints];
            dialInitStr.pvalues = new float[dialInitStr.endPoints];
#endif
        }
        else if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_POINTS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            while (ptoken)
            {
                sscanf(ptoken, "%f", &dialInitStr.ppoints[pointsIndex]);
                ptoken = FindToken(&plinePtr, pseparators);
                pointsIndex++;
            }
        }
        else if ( not strcmpi(ptoken, PROP_VALUES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            while (ptoken)
            {
                sscanf(ptoken, "%f", &dialInitStr.pvalues[valuesIndex]);
                ptoken = FindToken(&plinePtr, pseparators);
                valuesIndex++;
            }
        }
        else if ( not strcmpi(ptoken, PROP_RADIUS0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &dialInitStr.radius0);
        }
        else if ( not strcmpi(ptoken, PROP_RADIUS1_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &dialInitStr.radius1);
        }
        else if ( not strcmpi(ptoken, PROP_RADIUS2_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &dialInitStr.radius2);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &dialInitStr.color0);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR1_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &dialInitStr.color1);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR2_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &dialInitStr.color2);
        }

        else if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(dialInitStr.srcRect.top),
                   &(dialInitStr.srcRect.left),
                   &(dialInitStr.srcRect.bottom),
                   &(dialInitStr.srcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));

            objectInitStr.transparencyType = CPOPAQUE;
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }

        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);

            if ( not objectInitStr.persistant)
            {
                objectInitStr.bsrcRect.top = 0;
                objectInitStr.bsrcRect.left = 0;
                objectInitStr.bsrcRect.bottom = 0;
                objectInitStr.bsrcRect.right = 0;

                objectInitStr.bdestRect = objectInitStr.bsrcRect;
                objectInitStr.bsurface = -1;
            }
        }
        else if ( not strcmpi(ptoken, PROP_BSRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top),
                   &(objectInitStr.bsrcRect.left),
                   &(objectInitStr.bsrcRect.bottom),
                   &(objectInitStr.bsrcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BDESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top),
                   &(objectInitStr.bdestRect.left),
                   &(objectInitStr.bdestRect.bottom),
                   &(objectInitStr.bdestRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BSURFACE_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d", &(objectInitStr.bsurface));
        }
        else if ( not strcmpi(ptoken, PROP_RENDER_NEEDLE))   //Wombat778 3-26-04 support for rendered textured needles
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d", &(dialInitStr.IsRendered));
        }

        else
        {
            CockpitMessage(ptoken, "Dial", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    //Wombat778 3-26-04 Code for rendering the dials instead of line drawing
    //Wombat778 4-11-04 added some error reporting

    if (DisplayOptions.bRender2DCockpit and dialInitStr.IsRendered)
    {
        //Wombat778 4-13-04 prevent a heap error with pit errors
        if ((dialInitStr.srcRect.bottom - dialInitStr.srcRect.top) * (dialInitStr.srcRect.right - dialInitStr.srcRect.left) > 0) //Wombat778 4-22-04 changed from >=  to >
            dialInitStr.sourcedial = new BYTE[(dialInitStr.srcRect.bottom - dialInitStr.srcRect.top) * (dialInitStr.srcRect.right - dialInitStr.srcRect.left)];
        else
            dialInitStr.sourcedial = NULL;

        SafeImageCopy(gpTemplateImage, dialInitStr.sourcedial, mTemplateHeight, mTemplateWidth, &dialInitStr.srcRect); //wombat778 4-11-04 make safeimagecopy always used
    }

    //Wombat778 end

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPDial *p = new CPDial(&objectInitStr, &dialInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}


//====================================================//
// CockpitManager::CreateSurface
//====================================================//

void CockpitManager::CreateSurface(int idNum, FILE* pcockpitDataFile)
{
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    char pfileName[32] = "";
    char psurfaceFile[MAX_LINE_BUFFER];
    const int lineLen = MAX_LINE_BUFFER - 1;
    SurfaceInitStr surfaceInitStruct = {0};
    RECT destRect;


    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);
    surfaceInitStruct.srcRect.top = 0;
    surfaceInitStruct.srcRect.bottom = 0;
    surfaceInitStruct.srcRect.left = 0;
    surfaceInitStruct.srcRect.right = 0;

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_FILENAME_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%s", pfileName);

            if (m_eCPVisType == MapVisId(VIS_F16C))
                sprintf(psurfaceFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pfileName);
            else
            {
                sprintf(psurfaceFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(m_eCPVisType), pfileName);

                // RV - Biker - No more res manager
                //if( not ResExistFile(psurfaceFile))
                if ( not FileExists(psurfaceFile))
                {
                    sprintf(psurfaceFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, m_eCPName, pfileName);

                    // RV - Biker - No more res manager
                    //if( not ResExistFile(psurfaceFile))
                    if ( not FileExists(psurfaceFile))
                    {
                        sprintf(psurfaceFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, m_eCPNameNCTR, pfileName);

                        // RV - Biker - No more res manager
                        //if( not ResExistFile(psurfaceFile))
                        if ( not FileExists(psurfaceFile))
                        {
                            // F16C fallback
                            sprintf(psurfaceFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pfileName);
                        }
                    }
                }
            }
        }
        else if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(surfaceInitStruct.srcRect.top),
                   &(surfaceInitStruct.srcRect.left),
                   &(surfaceInitStruct.srcRect.bottom),
                   &(surfaceInitStruct.srcRect.right));
        }
        else
        {
            CockpitMessage(ptoken, "Surface", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    destRect.top = 0;
    destRect.left = 0;
    destRect.bottom = surfaceInitStruct.srcRect.bottom - surfaceInitStruct.srcRect.top + 1;
    destRect.right = surfaceInitStruct.srcRect.right - surfaceInitStruct.srcRect.left + 1;

#ifdef USE_SH_POOLS
    surfaceInitStruct.psrcBuffer = (BYTE *)MemAllocPtr(gCockMemPool, sizeof(BYTE) * destRect.bottom * destRect.right, FALSE);
#else
    surfaceInitStruct.psrcBuffer = new BYTE[destRect.bottom * destRect.right];
#endif

    surfaceInitStruct.srcRect.right += 1;
    surfaceInitStruct.srcRect.bottom += 1;

    SafeImageCopy(mpLoadBuffer, surfaceInitStruct.psrcBuffer, mLoadBufferHeight, mLoadBufferWidth, &surfaceInitStruct.srcRect); //wombat778 4-11-04 make safeimagecopy always used

    surfaceInitStruct.idNum = idNum;

    surfaceInitStruct.pOtwImage = OTWDriver.OTWImage;

    CPSurface *p = new CPSurface(&surfaceInitStruct);
    ShiAssert(p);

    if ( not p) return;

    mpSurfaces.push_back(p);
    mSurfaceTally++;
}

//====================================================//
// CockpitManager::ImageCopy
//====================================================//

void CockpitManager::ImageCopy(GLubyte* ploadBuffer,
                               GLubyte* psrcBuffer,
                               int width,
                               RECT* psrcRect)
{
    int i, j, n;
    int rowIndex;

    for (i = psrcRect->top, n = 0; i < psrcRect->bottom; i++)
    {
        rowIndex = i * width;

        for (j = psrcRect->left; j < psrcRect->right; j++, n++)
        {
            psrcBuffer[n] = ploadBuffer[rowIndex + j];
        }
    }
}

//Wombat778 3-23-04 added to protect against source rectangles that are too big
//====================================================//
// CockpitManager::SafeImageCopy. Adds a check for whether the src rectangle exceeds the source buffer
//====================================================//

void CockpitManager::SafeImageCopy(GLubyte* ploadBuffer,
                                   GLubyte* psrcBuffer,
                                   int height,
                                   int width,
                                   RECT* psrcRect)
{
    bool clearbuffer = false;

    //Wombat778 4-13-04 move the check for a bad pointer here.
    if ( not psrcBuffer)
    {
        CockpitError(gDebugLineNum, 1);
        return;
    }

    if (psrcRect->top < 0 or psrcRect->left < 0 or psrcRect->bottom > height or psrcRect->right > width) //Wombat778 4-11-04 added another check for src rectangles below 0
    {
        CockpitError(gDebugLineNum, 2); //Wombat778 4-11-04 throw up an error
        clearbuffer = true; //Wombat778 4-13-04 since this is just a buffer size error, we still want to clear the texture with chroma blue
    }

    int i, j, n;
    int rowIndex;

    for (i = psrcRect->top, n = 0; i < psrcRect->bottom; i++)
    {
        rowIndex = i * width;

        for (j = psrcRect->left; j < psrcRect->right; j++, n++)
        {
            if ( not clearbuffer)
                psrcBuffer[n] = ploadBuffer[rowIndex + j];
            else
                psrcBuffer[n] = 0; //Wombat778 4-13-04 fill the texture with the chroma value
        }
    }
}




//====================================================//
// CockpitManager::CreatePanel
//====================================================//

void CockpitManager::CreatePanel(int idNum, FILE* pcockpitDataFile)
{
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    PanelInitStr *ppanelInitStr;
    int surfaceIndex = 0;
    int objectIndex = 0;
    int buttonViewIndex = 0;
    int            i;
    char ptransparencyStr[32] = "";

    bool osb3exists = false; //Wombat778 4-12-04
    bool osb4exists = false; //Wombat778 4-12-04

#ifdef USE_SH_POOLS
    ppanelInitStr = (PanelInitStr *)MemAllocPtr(gCockMemPool, sizeof(PanelInitStr), FALSE);
#else
    ppanelInitStr = new PanelInitStr;
#endif

    memset(ppanelInitStr->pviewRects, 0, sizeof(RECT*) * BOUNDS_TOTAL);
    ppanelInitStr->psurfaceData = NULL;
    ppanelInitStr->pobjectIDs = NULL;
    ppanelInitStr->pbuttonViewIDs = NULL;
    ppanelInitStr->numButtonViews = NULL;
    ppanelInitStr->doGeometry       = FALSE;
    ppanelInitStr->mfdFont           = mMFDFont;
    ppanelInitStr->hudFont           = mHudFont;
    ppanelInitStr->dedFont           = mDEDFont;
    ppanelInitStr->pan = 0; // JPO more inits
    ppanelInitStr->tilt = 0;
    ppanelInitStr->maskTop = 0;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_NUMSURFACES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &ppanelInitStr->numSurfaces);
#ifdef USE_SH_POOLS

            if (ppanelInitStr->numSurfaces > 0)
            {
                ppanelInitStr->psurfaceData = (PanelSurfaceStr *)MemAllocPtr(gCockMemPool, sizeof(PanelSurfaceStr) * ppanelInitStr->numSurfaces, FALSE);
            }

#else
            ppanelInitStr->psurfaceData = new PanelSurfaceStr[ppanelInitStr->numSurfaces];
#endif
        }
        else if ( not strcmpi(ptoken, PROP_DOGEOMETRY_STR))
        {
            ppanelInitStr->doGeometry = TRUE;
        }
        else if ( not strcmpi(ptoken, PROP_MFDLEFT_STR))
        {

#ifdef USE_SH_POOLS
            ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]   = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT), FALSE);
#else
            ppanelInitStr->pviewRects[BOUNDS_MFDLEFT] = new RECT;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->top),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->left),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->bottom),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->right));
        }
        else if ( not strcmpi(ptoken, PROP_MFDRIGHT_STR))
        {

#ifdef USE_SH_POOLS
            ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]   = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT), FALSE);
#else
            ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT] = new RECT;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->top),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->left),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->bottom),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->right));
        }
        //Wombat778 4-12-04 add support for additional in-cockpit mfds
        else if ( not strcmpi(ptoken, PROP_MFD3_STR))
        {

#ifdef USE_SH_POOLS
            ppanelInitStr->pviewRects[BOUNDS_MFD3]   = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT), FALSE);
#else
            ppanelInitStr->pviewRects[BOUNDS_MFD3] = new RECT;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_MFD3]->top),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFD3]->left),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFD3]->bottom),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFD3]->right));
        }
        //Wombat778 4-12-04 add support for additional in-cockpit mfds
        else if ( not strcmpi(ptoken, PROP_MFD4_STR))
        {

#ifdef USE_SH_POOLS
            ppanelInitStr->pviewRects[BOUNDS_MFD4]   = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT), FALSE);
#else
            ppanelInitStr->pviewRects[BOUNDS_MFD4] = new RECT;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_MFD4]->top),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFD4]->left),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFD4]->bottom),
                   &(ppanelInitStr->pviewRects[BOUNDS_MFD4]->right));
        }

        else if ( not strcmpi(ptoken, PROP_HUD_STR))
        {

#ifdef USE_SH_POOLS
            ppanelInitStr->pviewRects[BOUNDS_HUD]   = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT), FALSE);
#else
            ppanelInitStr->pviewRects[BOUNDS_HUD] = new RECT;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_HUD]->top),
                   &(ppanelInitStr->pviewRects[BOUNDS_HUD]->left),
                   &(ppanelInitStr->pviewRects[BOUNDS_HUD]->bottom),
                   &(ppanelInitStr->pviewRects[BOUNDS_HUD]->right));
        }
        else if ( not strcmpi(ptoken, PROP_RWR_STR))
        {

#ifdef USE_SH_POOLS
            ppanelInitStr->pviewRects[BOUNDS_RWR]   = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT), FALSE);
#else
            ppanelInitStr->pviewRects[BOUNDS_RWR] = new RECT;
#endif
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_RWR]->top),
                   &(ppanelInitStr->pviewRects[BOUNDS_RWR]->left),
                   &(ppanelInitStr->pviewRects[BOUNDS_RWR]->bottom),
                   &(ppanelInitStr->pviewRects[BOUNDS_RWR]->right));

        }
        else if ( not strcmpi(ptoken, PROP_MOUSEBOUNDS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->mouseBounds.top),
                   &(ppanelInitStr->mouseBounds.left),
                   &(ppanelInitStr->mouseBounds.bottom),
                   &(ppanelInitStr->mouseBounds.right));
        }
        else if ( not strcmpi(ptoken, PROP_ADJPANELS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d %d %d %d %d", &(ppanelInitStr->adjacentPanels.N),
                   &(ppanelInitStr->adjacentPanels.NE),
                   &(ppanelInitStr->adjacentPanels.E),
                   &(ppanelInitStr->adjacentPanels.SE),
                   &(ppanelInitStr->adjacentPanels.S),
                   &(ppanelInitStr->adjacentPanels.SW),
                   &(ppanelInitStr->adjacentPanels.W),
                   &(ppanelInitStr->adjacentPanels.NW));
        }
        else if ( not strcmpi(ptoken, PROP_SURFACES_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d %d %s %d %d %d %d", &ppanelInitStr->psurfaceData[surfaceIndex].surfaceNum,
                   &ppanelInitStr->psurfaceData[surfaceIndex].persistant,
                   ptransparencyStr,
                   &ppanelInitStr->psurfaceData[surfaceIndex].destRect.top,
                   &ppanelInitStr->psurfaceData[surfaceIndex].destRect.left,
                   &ppanelInitStr->psurfaceData[surfaceIndex].destRect.bottom,
                   &ppanelInitStr->psurfaceData[surfaceIndex].destRect.right);

            ppanelInitStr->psurfaceData[surfaceIndex].psurface = NULL;

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                ppanelInitStr->psurfaceData[surfaceIndex].transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                ppanelInitStr->psurfaceData[surfaceIndex].transparencyType = CPOPAQUE;
            }
            else
            {
                ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
            }

            surfaceIndex++;
        }
        else if ( not strcmpi(ptoken, PROP_NUMOBJECTS_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->numObjects);
#ifdef USE_SH_POOLS

            if (ppanelInitStr->numObjects > 0)
            {
                ppanelInitStr->pobjectIDs   = (int *)MemAllocPtr(gCockMemPool, sizeof(int) * ppanelInitStr->numObjects, FALSE);
            }

#else
            ppanelInitStr->pobjectIDs = new int[ppanelInitStr->numObjects];
#endif
        }
        else if ( not strcmpi(ptoken, PROP_OBJECTS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            while (ptoken)
            {
                sscanf(ptoken, "%d", &ppanelInitStr->pobjectIDs[objectIndex]);
                ptoken = FindToken(&plinePtr, pseparators);
                objectIndex++;
            }
        }
        else if ( not strcmpi(ptoken, PROP_OFFSET_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d %d", &ppanelInitStr->xOffset, &ppanelInitStr->yOffset);
        }
        else if ( not strcmpi(ptoken, PROP_NUMBUTTONVIEWS_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->numButtonViews);

            if (ppanelInitStr->numButtonViews > 0)
            {
#ifdef USE_SH_POOLS
                ppanelInitStr->pbuttonViewIDs   = (int *)MemAllocPtr(gCockMemPool, sizeof(int) * ppanelInitStr->numButtonViews, FALSE);
#else
                ppanelInitStr->pbuttonViewIDs = new int[ppanelInitStr->numButtonViews];
#endif
            }
        }
        else if ( not strcmpi(ptoken, PROP_BUTTONVIEWS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);

            while (ptoken)
            {
                sscanf(ptoken, "%d", &ppanelInitStr->pbuttonViewIDs[buttonViewIndex]);
                ptoken = FindToken(&plinePtr, pseparators);
                buttonViewIndex++;
            }
        }
        else if ( not strcmpi(ptoken, PROP_PANTILT_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%f %f", &ppanelInitStr->pan, &ppanelInitStr->tilt);
            // RV - RED - Tilt must be conformed to vertical Scaling
            ppanelInitStr->tilt *= (mVScale / mHScale);
        }
        else if ( not strcmpi(ptoken, PROP_HUDFONT))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->hudFont);
        }
        else if ( not strcmpi(ptoken, PROP_MFDFONT))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->mfdFont);
        }
        else if ( not strcmpi(ptoken, PROP_DEDFONT))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->dedFont);
        }
        else if ( not strcmpi(ptoken, PROP_MASKTOP_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->maskTop);
        }
        else if ( not strcmpi(ptoken, PROP_CURSORID_STR))
        {
            ptoken = FindToken(&plinePtr, "\n=;");
            sscanf(ptoken, "%d", &ppanelInitStr->defaultCursor);
        }
        else if ( not strcmpi(ptoken, PROP_OSBLEFT_STR))
        {
            for (i = 0; i < 20; i++)
            {
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[0][i][0]);
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[0][i][1]);
            }
        }
        else if ( not strcmpi(ptoken, PROP_OSBRIGHT_STR))
        {
            for (i = 0; i < 20; i++)
            {
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[1][i][0]);
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[1][i][1]);
            }
        }

        //Wombat778 4-12-04 add support for separate osb labelling for MFD 3 and 4

        else if ( not strcmpi(ptoken, PROP_OSB3_STR))
        {
            osb3exists = true;

            for (i = 0; i < 20; i++)
            {
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[2][i][0]);
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[2][i][1]);
            }
        }
        else if ( not strcmpi(ptoken, PROP_OSB4_STR))
        {
            osb4exists = true;

            for (i = 0; i < 20; i++)
            {
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[3][i][0]);
                ptoken = FindToken(&plinePtr, pseparators);
                sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[3][i][1]);
            }
        }

        //Wombat778 end;


        // 2000-11-12 ADDED BY S.G. SO COMMENTED LINE DON'T TRIGGER AN ASSERT
        else if ( not strncmp(ptoken, "//", 2)) //Wombat778 4-19-04 converted from strcmpi to strncmp so that comments dont need spaces after
            ;
        // END OF ADDED SECTION
        else
        {
            CockpitMessage(ptoken, "Panel", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    F4Assert(surfaceIndex == ppanelInitStr->numSurfaces); // should have as many surfaces as specified in file
    F4Assert(objectIndex == ppanelInitStr->numObjects); // should have as many objects as specified in file

    //Wombat778 4-12-04  If no osb3 or osb4 lines exist, then fill that section with the info from osbleft and osbright
    if ( not osb3exists)
        for (i = 0; i < 20; i++)
        {
            ppanelInitStr->osbLocation[2][i][0] = ppanelInitStr->osbLocation[0][i][0];
            ppanelInitStr->osbLocation[2][i][1] = ppanelInitStr->osbLocation[0][i][1];
        }

    if ( not osb4exists)
        for (i = 0; i < 20; i++)
        {
            ppanelInitStr->osbLocation[3][i][0] = ppanelInitStr->osbLocation[1][i][0];
            ppanelInitStr->osbLocation[3][i][1] = ppanelInitStr->osbLocation[1][i][1];
        }

    //Wombat778 end

    ppanelInitStr->hScale = mHScale;
    ppanelInitStr->vScale = mVScale;
    ppanelInitStr->cockpitWidth = DisplayOptions.DispWidth;
    ppanelInitStr->cockpitHeight = DisplayOptions.DispHeight;
    ppanelInitStr->idNum = idNum;
    ppanelInitStr->pOtwImage = OTWDriver.OTWImage;

    CPPanel *p = new CPPanel(ppanelInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpPanels.push_back(p);
    mPanelTally++;
}


//====================================================//
// CockpitManager::ResolveReferences
//====================================================//

void CockpitManager::ResolveReferences(void)
{

    int i, j, k;
    int panelSurfaces;
    int panelObjects;
    int panelButtonViews;
    int surfaceId;
    int objectId;
    int buttonId;
    int buttonViewId;
    BOOL found;

    // loop thru all panels
    for (i = 0; i < mPanelTally; i++)
    {
        panelSurfaces = mpPanels[i]->mNumSurfaces;

        // loop thru each panel's list of surfaces
        for (j = 0; j < panelSurfaces; j++)
        {
            surfaceId = mpPanels[i]->mpSurfaceData[j].surfaceNum;
            found = FALSE;
            k = 0;

            // search cpmanager's list of surface pointers
            while (( not found) and (k < mSurfaceTally))
            {
                if (mpSurfaces[k]->mIdNum == surfaceId)
                {
                    mpPanels[i]->mpSurfaceData[j].psurface = mpSurfaces[k];
                    found = TRUE;
                }
                else
                {
                    k++;
                }
            }

            F4Assert(found);    //couldn't find the surface in our list
        }
    }

    // loop thru all panels
    for (i = 0; i < mPanelTally; i++)
    {
        panelObjects = mpPanels[i]->mNumObjects;

        // loop thru each of the panel's objects
        for (j = 0; j < panelObjects; j++)
        {
            objectId = mpPanels[i]->mpObjectIDs[j];
            found = FALSE;
            k = 0;

            // search cpmanager's list of object pointers
            while (( not found) and (k < mObjectTally))
            {
                if (mpObjects[k]->mIdNum == objectId)
                {
                    mpPanels[i]->mpObjects[j] = mpObjects[k];
                    found = TRUE;
                }
                else
                {
                    k++;
                }
            }

            F4Assert(found); //couldn't find the object in our list
        }
    }

    for (i = 0; i < mButtonViewTally; i++)
    {

        found = FALSE;
        j = 0;
        buttonId = mpButtonViews[i]->GetParentButton();

        while ( not found and j < mButtonTally)
        {

            if (mpButtonObjects[j]->GetId() == buttonId)
            {
                mpButtonObjects[j]->AddView(mpButtonViews[i]);
                found = TRUE;
            }
            else
            {
                j++;
            }
        }

        F4Assert(found);
    }


    for (i = 0; i < mButtonTally; i++)
    {
        if (mpButtonObjects[i]->GetSound(1) >= 0)
        {
            mpButtonObjects[i]->SetSound(1, mpSoundList->GetSoundIndex(mpButtonObjects[i]->GetSound(1)));

            if (mpButtonObjects[i]->GetSound(2) >= 0)
            {
                mpButtonObjects[i]->SetSound(2, mpSoundList->GetSoundIndex(mpButtonObjects[i]->GetSound(2)));
            }
        }
    }


    for (i = 0; i < mPanelTally; i++)
    {
        panelButtonViews = mpPanels[i]->mNumButtonViews;

        // loop thru each of the panel's objects
        for (j = 0; j < panelButtonViews; j++)
        {
            buttonViewId = mpPanels[i]->mpButtonViewIDs[j];
            found = FALSE;
            k = 0;

            // search cpmanager's list of object pointers
            while (( not found) and (k < mButtonViewTally))
            {
                if (mpButtonViews[k]->GetId() == buttonViewId)
                {
                    mpPanels[i]->mpButtonViews[j] = mpButtonViews[k];
                    found = TRUE;
                }
                else
                {
                    k++;
                }
            }

            F4Assert(found); //couldn't find the object in our list
        }
    }


    if (mpIcp)
    {
        // loop thru each of the panel's buttons
        buttonId = ICP_INITIAL_PRIMARY_BUTTON;
        found = FALSE;
        i = 0;

        // search cpmanager's list of button pointers
        while (( not found) and (i < mButtonTally))
        {
            if (mpButtonObjects[i]->GetId() == buttonId)
            {
                mpIcp->InitPrimaryExclusiveButton(mpButtonObjects[i]);
                found = TRUE;
            }
            else
            {
                i++;
            }
        }

        F4Assert(found); //couldn't find the button in our list


        buttonId = ICP_INITIAL_TERTIARY_BUTTON;
        found = FALSE;
        i = 0;

        // search cpmanager's list of button pointers
        while (( not found) and (i < mButtonTally))
        {
            if (mpButtonObjects[i]->GetId() == buttonId)
            {
                mpIcp->InitTertiaryExclusiveButton(mpButtonObjects[i]);
                found = TRUE;
            }
            else
            {
                i++;
            }
        }

        F4Assert(found); //couldn't find the button in our list

    }
}


//====================================================//
// CockpitManager::CreateKneeView
//====================================================//
void CockpitManager::CreateKneeView(int idNum, FILE* pcockpitDataFile)
{
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;

    objectInitStr.cycleBits = 0x0000;

    objectInitStr.bsrcRect.top = 0;
    objectInitStr.bsrcRect.left = 0;
    objectInitStr.bsrcRect.bottom = 0;
    objectInitStr.bsrcRect.right = 0;
    objectInitStr.bdestRect = objectInitStr.bsrcRect;
    objectInitStr.bsurface = -1;
    objectInitStr.callbackSlot = -1;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));

        }
        else
        {
            CockpitMessage(ptoken, "Kneeboard", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPKneeView *p = new CPKneeView(&objectInitStr, mpKneeBoard);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateMirror
//====================================================//
void CockpitManager::CreateMirror(int idNum, FILE* pcockpitDataFile)
{
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char *plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    char ptransparencyStr[32] = "";

    ObjectInitStr objectInitStr;
    memset(&objectInitStr, 0, sizeof(objectInitStr));
    objectInitStr.bsurface = -1;
    objectInitStr.callbackSlot = -1;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    ++gDebugLineNum;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {
        if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%s %d %d %d %d",
                   ptransparencyStr,
                   &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right)
                  );

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                objectInitStr.transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                objectInitStr.transparencyType = CPOPAQUE;
            }
        }
        else if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d",
                   &(objectInitStr.bsrcRect.top),
                   &(objectInitStr.bsrcRect.left),
                   &(objectInitStr.bsrcRect.bottom),
                   &(objectInitStr.bsrcRect.right)
                  );
            objectInitStr.bsrcRect.bottom++;
            objectInitStr.bsrcRect.right++;
        }
        else
        {
            CockpitMessage(ptoken, "Mirror", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        ++gDebugLineNum;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum  = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;
    // @TODO copy the mirror image to constructor
    /*if ((source.bottom-source.top)* (source.right-source.left) >=0){
     hsiInitStr.sourcehsi = new BYTE[(source.bottom-source.top)* (source.right-source.left)];
    }
    else {
     hsiInitStr.sourcehsi = NULL;
    }
    SafeImageCopy(gpTemplateImage, hsiInitStr.sourcehsi, mTemplateHeight, mTemplateWidth, &source);*/

    CPMirror *p = new CPMirror(objectInitStr);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateLight
//====================================================//

void CockpitManager::CreateLight(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    LightButtonInitStr lightButtonInitStr = {0};
    ObjectInitStr objectInitStr;
    int state = 0;
    char ptransparencyStr[32] = "";

    lightButtonInitStr.cursorId = -1; // just so that we pass something nice to the light class
    objectInitStr.cycleBits = 0x0000;

    objectInitStr.bsrcRect.top = 0;
    objectInitStr.bsrcRect.left = 0;
    objectInitStr.bsrcRect.bottom = 0;
    objectInitStr.bsrcRect.right = 0;
    objectInitStr.bdestRect = objectInitStr.bsrcRect;
    objectInitStr.bsurface = -1;
    lightButtonInitStr.psrcRect   = NULL;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);
    lightButtonInitStr.states = 0;

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_STATES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &lightButtonInitStr.states);
#ifdef USE_SH_POOLS

            if (lightButtonInitStr.states > 0)
            {
                lightButtonInitStr.psrcRect = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT) * lightButtonInitStr.states, FALSE);
            }

#else
            lightButtonInitStr.psrcRect = new RECT[lightButtonInitStr.states];
#endif
        }
        else if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_CURSORID_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &lightButtonInitStr.cursorId);
        }
        else if ( not strcmpi(ptoken, PROP_INITSTATE_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &lightButtonInitStr.initialState);
        }
        else if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            F4Assert(state < lightButtonInitStr.states);

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(lightButtonInitStr.psrcRect[state].top),
                   &(lightButtonInitStr.psrcRect[state].left),
                   &(lightButtonInitStr.psrcRect[state].bottom),
                   &(lightButtonInitStr.psrcRect[state].right));
            lightButtonInitStr.psrcRect[state].bottom++;
            lightButtonInitStr.psrcRect[state].right++;
            state++;
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
                   &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                objectInitStr.transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                objectInitStr.transparencyType = CPOPAQUE;
            }
            else
            {
                ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
            }
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);
        }
        else
        {
            CockpitMessage(ptoken, "Light", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }


    //Wombat778 3-22-04 Code for rendering the lights instead of blitting
    //Wombat778 4-11-04 Added some error reporting

    if (DisplayOptions.bRender2DCockpit)
    {
        if (lightButtonInitStr.states >= 0) //Wombat778 4-13-04 prevent a heap error with pit errors
            lightButtonInitStr.sourcelights = new SourceLightType[lightButtonInitStr.states];
        else
            lightButtonInitStr.sourcelights = NULL;

        if (lightButtonInitStr.sourcelights)
            for (int i = 0; i < lightButtonInitStr.states; i++)
            {
                //Wombat778 4-13-04 prevent a heap error with pit errors
                if ((lightButtonInitStr.psrcRect[i].bottom - lightButtonInitStr.psrcRect[i].top) * (lightButtonInitStr.psrcRect[i].right - lightButtonInitStr.psrcRect[i].left) > 0) //Wombat778 4-22-04 changed from >=  to >
                    lightButtonInitStr.sourcelights[i].light = new BYTE[(lightButtonInitStr.psrcRect[i].bottom - lightButtonInitStr.psrcRect[i].top) * (lightButtonInitStr.psrcRect[i].right - lightButtonInitStr.psrcRect[i].left)];
                else
                    lightButtonInitStr.sourcelights[i].light = NULL;

                SafeImageCopy(gpTemplateImage, lightButtonInitStr.sourcelights[i].light, mTemplateHeight, mTemplateWidth, &lightButtonInitStr.psrcRect[i]); //wombat778 4-11-04 make safeimagecopy always used
            }
        else
            CockpitError(gDebugLineNum, 1); //Wombat778 4-11-04

    }

    //Wombat778 end
    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPLight *p = new CPLight(&objectInitStr, &lightButtonInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}


//====================================================//
// CockpitManager::CreateButtonView
//====================================================//

void CockpitManager::CreateButtonView(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    char ptransparencyStr[32] = "";
    ButtonViewInitStr buttonViewInitStr = {0};
    int state = 0;

    buttonViewInitStr.objectId = idNum;
    buttonViewInitStr.pSrcRect = NULL;
    buttonViewInitStr.states = 0;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {


        if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            F4Assert(state < buttonViewInitStr.states);

            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(buttonViewInitStr.pSrcRect[state].top),
                   &(buttonViewInitStr.pSrcRect[state].left),
                   &(buttonViewInitStr.pSrcRect[state].bottom),
                   &(buttonViewInitStr.pSrcRect[state].right));
            buttonViewInitStr.pSrcRect[state].bottom++;
            buttonViewInitStr.pSrcRect[state].right++;
            state++;
        }

        else if ( not strcmpi(ptoken, PROP_STATES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonViewInitStr.states);
#ifdef USE_SH_POOLS

            if (buttonViewInitStr.states > 0)
            {
                buttonViewInitStr.pSrcRect = (RECT *)MemAllocPtr(gCockMemPool, sizeof(RECT) * buttonViewInitStr.states, FALSE);
            }

#else
            buttonViewInitStr.pSrcRect = new RECT[buttonViewInitStr.states];
#endif
        }
        else if ( not strcmpi(ptoken, PROP_PARENTBUTTON_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonViewInitStr.parentButton);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
                   &(buttonViewInitStr.destRect.top),
                   &(buttonViewInitStr.destRect.left),
                   &(buttonViewInitStr.destRect.bottom),
                   &(buttonViewInitStr.destRect.right));

            buttonViewInitStr.destRect.bottom++;
            buttonViewInitStr.destRect.right++;

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                buttonViewInitStr.transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                buttonViewInitStr.transparencyType = CPOPAQUE;
            }
            else
            {
                ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
            }
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonViewInitStr.persistant);
        }
        else
        {
            CockpitMessage(ptoken, "ButtonView", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    //Wombat778 3-22-04 Code for rendering the buttonviews instead of blitting
    //Wombat778 4-11-04 Added some error reporting

    if (DisplayOptions.bRender2DCockpit)
    {
        //Wombat778 4-13-04 prevent a heap error with pit errors
        if (buttonViewInitStr.states) //Wombat778 4-18-04 dont create an error if the states = 0, because cockpits use this.
        {
            if (buttonViewInitStr.states > 0)
                buttonViewInitStr.sourcebuttonview = new SourceButtonViewType[buttonViewInitStr.states];
            else
                buttonViewInitStr.sourcebuttonview = NULL;

            if (buttonViewInitStr.sourcebuttonview)
                for (int i = 0; i < buttonViewInitStr.states; i++)
                {
                    //Wombat778 4-13-04 prevent a heap error with pit errors
                    if ((buttonViewInitStr.pSrcRect[i].bottom - buttonViewInitStr.pSrcRect[i].top) * (buttonViewInitStr.pSrcRect[i].right - buttonViewInitStr.pSrcRect[i].left) > 0) //Wombat778 4-22-04 changed from >=  to >
                        buttonViewInitStr.sourcebuttonview[i].buttonview = new BYTE[(buttonViewInitStr.pSrcRect[i].bottom - buttonViewInitStr.pSrcRect[i].top) * (buttonViewInitStr.pSrcRect[i].right - buttonViewInitStr.pSrcRect[i].left)];
                    else
                        buttonViewInitStr.sourcebuttonview[i].buttonview = NULL;

                    SafeImageCopy(gpTemplateImage, buttonViewInitStr.sourcebuttonview[i].buttonview, mTemplateHeight, mTemplateWidth, &buttonViewInitStr.pSrcRect[i]); //wombat778 4-11-04 make safeimagecopy always used
                }
            else
                CockpitError(gDebugLineNum, 1); //Wombat778 4-11-04
        }
        else
            buttonViewInitStr.sourcebuttonview = NULL;
    }

    //Wombat778 end
    buttonViewInitStr.hScale = mHScale;
    buttonViewInitStr.vScale = mVScale;
    buttonViewInitStr.objectId = idNum;
    buttonViewInitStr.pOTWImage = mpOTWImage;
    buttonViewInitStr.pTemplate = gpTemplateSurface;

    CPButtonView *p = new CPButtonView(&buttonViewInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpButtonViews.push_back(p);
    mButtonViewTally++;
}

//====================================================//
// CockpitManager::CreateButton
//====================================================//

void CockpitManager::CreateButton(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ButtonObjectInitStr buttonObjectInitStr;


    buttonObjectInitStr.objectId = idNum;

    buttonObjectInitStr.sound1 = -1;
    buttonObjectInitStr.sound2 = -1;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_STATES_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.totalStates);
        }
        else if ( not strcmpi(ptoken, PROP_DELAY_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.delay);
        }
        else if ( not strcmpi(ptoken, PROP_CURSORID_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.cursorIndex);
        }
        else if ( not strcmpi(ptoken, PROP_INITSTATE_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.normalState);
        }
        else if ( not strcmpi(ptoken, PROP_SOUND1_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.sound1);
        }
        else if ( not strcmpi(ptoken, PROP_SOUND2_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.sound2);
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_NUMBUTTONVIEWS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &buttonObjectInitStr.totalViews);
        }
        else
        {
            CockpitMessage(ptoken, "Button", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    CPButtonObject *p = new CPButtonObject(&buttonObjectInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpButtonObjects.push_back(p);
    mButtonTally++;
}


//====================================================//
// CockpitManager::CreateAdi
//====================================================//

void CockpitManager::CreateAdi(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    char ptransparencyStr[32] = "";
    ADIInitStr adiInitStr = {0};

    adiInitStr.doBackRect = FALSE;
    adiInitStr.backSrc.top = 0;
    adiInitStr.backSrc.left = 0;
    adiInitStr.backSrc.bottom = 0;
    adiInitStr.backSrc.right = 0;
    adiInitStr.color0 = 0xFF20A2C8;
    adiInitStr.color1 = 0xff808080;
    adiInitStr.color2 = 0xffffffff;
    adiInitStr.color3 = 0xFF6CF3F3;
    adiInitStr.color4 = 0xFF6CF3F3;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(adiInitStr.srcRect.top),
                   &(adiInitStr.srcRect.left),
                   &(adiInitStr.srcRect.bottom),
                   &(adiInitStr.srcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_ILSLIMITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(adiInitStr.ilsLimits.top),
                   &(adiInitStr.ilsLimits.left),
                   &(adiInitStr.ilsLimits.bottom),
                   &(adiInitStr.ilsLimits.right));

        }
        else if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
                   &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                objectInitStr.transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                objectInitStr.transparencyType = CPOPAQUE;
            }
            else
            {
                ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
            }
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);

            if ( not objectInitStr.persistant)
            {
                objectInitStr.bsrcRect.top = 0;
                objectInitStr.bsrcRect.left = 0;
                objectInitStr.bsrcRect.bottom = 0;
                objectInitStr.bsrcRect.right = 0;

                objectInitStr.bdestRect = objectInitStr.bsrcRect;
                objectInitStr.bsurface = -1;
            }
        }
        else if ( not strcmpi(ptoken, PROP_BSRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top),
                   &(objectInitStr.bsrcRect.left),
                   &(objectInitStr.bsrcRect.bottom),
                   &(objectInitStr.bsrcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BDESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top),
                   &(objectInitStr.bdestRect.left),
                   &(objectInitStr.bdestRect.bottom),
                   &(objectInitStr.bdestRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BSURFACE_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d", &(objectInitStr.bsurface));
        }

        else if ( not strcmpi(ptoken, PROP_BLITBACKGROUND_STR))
        {
            adiInitStr.doBackRect = TRUE;
            LoadBuffer(pcockpitDataFile);
        }
        else if ( not strcmpi(ptoken, PROP_BACKDEST_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");

            sscanf(ptoken, "%d %d %d %d", &(adiInitStr.backDest.top),
                   &(adiInitStr.backDest.left),
                   &(adiInitStr.backDest.bottom),
                   &(adiInitStr.backDest.right));
        }
        else if ( not strcmpi(ptoken, PROP_BACKSRC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");

            sscanf(ptoken, "%d %d %d %d", &(adiInitStr.backSrc.top),
                   &(adiInitStr.backSrc.left),
                   &(adiInitStr.backSrc.bottom),
                   &(adiInitStr.backSrc.right));
        }
        else if ( not strcmpi(ptoken, PROP_COLOR0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &adiInitStr.color0);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR1_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &adiInitStr.color1);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR2_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &adiInitStr.color2);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR3_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &adiInitStr.color3);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR4_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &adiInitStr.color4);
        }
        else
        {
            CockpitMessage(ptoken, "ADI", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }


    if (adiInitStr.doBackRect)
    {
        RECT destRect;


        destRect.top = 0;
        destRect.left = 0;
        destRect.bottom = adiInitStr.backSrc.bottom - adiInitStr.backSrc.top + 1;
        destRect.right = adiInitStr.backSrc.right - adiInitStr.backSrc.left + 1;

#ifdef USE_SH_POOLS
        adiInitStr.pBackground = (BYTE *)MemAllocPtr(gCockMemPool, sizeof(BYTE) * destRect.bottom * destRect.right, FALSE);
#else
        adiInitStr.pBackground = new BYTE[destRect.bottom * destRect.right];
#endif

        adiInitStr.backSrc.right += 1;
        adiInitStr.backSrc.bottom += 1;

        SafeImageCopy(mpLoadBuffer, adiInitStr.pBackground, mLoadBufferHeight, mLoadBufferWidth, &adiInitStr.backSrc); //wombat778 4-11-04 make safeimagecopy always used

        adiInitStr.backSrc.top = destRect.top;
        adiInitStr.backSrc.left = destRect.left;
        adiInitStr.backSrc.bottom = destRect.bottom;
        adiInitStr.backSrc.right = destRect.right;
    }


    //Wombat778 3-24-04 Even though there already appears to be code above for rendering, it doesnt appear to get used,
    //and I dont know if it works properly, so I will add my own.

    //Wombat778 4-11-04 Added some error reporting

    if (DisplayOptions.bRender2DCockpit)
    {
        //Wombat778 4-13-04 prevent a heap error with pit errors
        if ((adiInitStr.srcRect.bottom - adiInitStr.srcRect.top) * (adiInitStr.srcRect.right - adiInitStr.srcRect.left) > 0) //Wombat778 4-22-04 changed from >=  to >
            adiInitStr.sourceadi = new BYTE[(adiInitStr.srcRect.bottom - adiInitStr.srcRect.top) * (adiInitStr.srcRect.right - adiInitStr.srcRect.left)];
        else
            adiInitStr.sourceadi = NULL;

        SafeImageCopy(gpTemplateImage, adiInitStr.sourceadi, mTemplateHeight, mTemplateWidth, &adiInitStr.srcRect); //wombat778 4-11-04 make safeimagecopy always used
    }

    //Wombat778 end


    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPAdi *p = new CPAdi(&objectInitStr, &adiInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateHsiView
//====================================================//

void CockpitManager::CreateHsiView(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    char ptransparencyStr[32] = "";
    int destcount = 0;
    int srccount = 0;
    RECT destination;
    RECT source;
    int transparencyType = 0;
    HsiInitStr hsiInitStr;


    //MI
    if ( not g_bRealisticAvionics)
    {
        hsiInitStr.colors[HSI_COLOR_ARROWS] = 0xff1e6cff;
        hsiInitStr.colors[HSI_COLOR_ARROWGHOST] = 0xff1e6cff;
        hsiInitStr.colors[HSI_COLOR_COURSEWARN] = 0xff0000ff;
        hsiInitStr.colors[HSI_COLOR_HEADINGMARK] = 0xff469646;
        hsiInitStr.colors[HSI_COLOR_STATIONBEARING] = 0xff323cff;
        hsiInitStr.colors[HSI_COLOR_COURSE] = 0xff73dcf0;
        hsiInitStr.colors[HSI_COLOR_AIRCRAFT] = 0xffe0e0e0;
        hsiInitStr.colors[HSI_COLOR_CIRCLES] = 0xffe0e0e0;
        hsiInitStr.colors[HSI_COLOR_DEVBAR] = 0xff73dcf0;
        hsiInitStr.colors[HSI_COLOR_ILSDEVWARN] = 0xff0000fd;
    }
    else
    {
        hsiInitStr.colors[HSI_COLOR_ARROWS] = 0xff0000ff;
        hsiInitStr.colors[HSI_COLOR_ARROWGHOST] = 0xff0000ff;
        hsiInitStr.colors[HSI_COLOR_COURSEWARN] = 0xff0000ff;
        hsiInitStr.colors[HSI_COLOR_HEADINGMARK] = 0xffffffff;
        hsiInitStr.colors[HSI_COLOR_STATIONBEARING] = 0xffffffff;
        hsiInitStr.colors[HSI_COLOR_COURSE] = 0xffffffff;
        hsiInitStr.colors[HSI_COLOR_AIRCRAFT] = 0xffffffff;
        hsiInitStr.colors[HSI_COLOR_CIRCLES] = 0xffe0e0e0;
        hsiInitStr.colors[HSI_COLOR_DEVBAR] = 0xffffffff;
        hsiInitStr.colors[HSI_COLOR_ILSDEVWARN] = 0xff0000fd;
    }

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {

        if ( not strcmpi(ptoken, PROP_SRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(source.top),
                   &(source.left),
                   &(source.bottom),
                   &(source.right));

            if (srccount == 0)
            {
                hsiInitStr.compassSrc = source;
            }
            else if (srccount == 1)
            {
                hsiInitStr.devSrc = source; // course deviation ring
            }
            else
            {
                ShiWarning("Bad HSI Source Count"); //couldn't read in transparency type
            }

            srccount++;
        }
        else if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
                   &(destination.top),
                   &(destination.left),
                   &(destination.bottom),
                   &(destination.right));

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                objectInitStr.transparencyType = CPOPAQUE;
            }
            else
            {
                ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
            }

            if (destcount == 0)
            {
                objectInitStr.destRect = destination;
                objectInitStr.transparencyType = transparencyType;
            }
            else if (destcount == 1)
            {
                hsiInitStr.compassDest = destination;;
                hsiInitStr.compassTransparencyType = transparencyType;
            }
            else if (destcount == 2)
            {
                hsiInitStr.devDest = destination;;
            }
            else
            {
                ShiWarning("Bad dest count");
            }

            destcount++;
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);

            if ( not objectInitStr.persistant)
            {
                objectInitStr.bsrcRect.top = 0;
                objectInitStr.bsrcRect.left = 0;
                objectInitStr.bsrcRect.bottom = 0;
                objectInitStr.bsrcRect.right = 0;

                objectInitStr.bdestRect = objectInitStr.bsrcRect;
                objectInitStr.bsurface = -1;
            }
        }
        else if ( not strcmpi(ptoken, PROP_WARNFLAG_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(hsiInitStr.warnFlag.top),
                   &(hsiInitStr.warnFlag.left),
                   &(hsiInitStr.warnFlag.bottom),
                   &(hsiInitStr.warnFlag.right));
        }
        else if ( not strcmpi(ptoken, PROP_BSRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top),
                   &(objectInitStr.bsrcRect.left),
                   &(objectInitStr.bsrcRect.bottom),
                   &(objectInitStr.bsrcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BDESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top),
                   &(objectInitStr.bdestRect.left),
                   &(objectInitStr.bdestRect.bottom),
                   &(objectInitStr.bdestRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BSURFACE_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d", &(objectInitStr.bsurface));
        }
        else if ( not strcmpi(ptoken, PROP_COLOR0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[0]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR1_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[1]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR2_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[2]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR3_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[3]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR4_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[4]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR5_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[5]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR6_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[6]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR7_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[7]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR8_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[8]);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR9_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &hsiInitStr.colors[9]);
        }
        else
        {
            CockpitMessage(ptoken, "HSI", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    //Wombat778 3-24-04 Code for rendering the HSI instead of blitting
    //WOmbat778 4-11-04 added some error reporting

    if (DisplayOptions.bRender2DCockpit)
    {
        //Wombat778 4-13-04 prevent a heap error with pit errors
        if ((source.bottom - source.top) * (source.right - source.left) >= 0)
        {
            hsiInitStr.sourcehsi = new BYTE[(source.bottom - source.top) * (source.right - source.left)];
        }
        else
        {
            hsiInitStr.sourcehsi = NULL;
        }

        //wombat778 4-11-04 make safeimagecopy always used
        SafeImageCopy(gpTemplateImage, hsiInitStr.sourcehsi, mTemplateHeight, mTemplateWidth, &source);
    }

    //Wombat778 end
    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;

    objectInitStr.pCPManager = this;

    hsiInitStr.pHsi = mpHsi;

    CPHsiView *p = new CPHsiView(&objectInitStr, &hsiInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::CreateMachAsi
//====================================================//

void CockpitManager::CreateMachAsi(int idNum, FILE* pcockpitDataFile)
{

    char plineBuffer[MAX_LINE_BUFFER] = "";
    char* plinePtr;
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
    const int lineLen = MAX_LINE_BUFFER - 1;
    ObjectInitStr objectInitStr;
    MachAsiInitStr machAsiInitStr;
    char ptransparencyStr[32] = "";

    machAsiInitStr.color0 =  0xFF181842;
    machAsiInitStr.color1 =  0xFF0C0C7A;
    machAsiInitStr.end_angle = 1.0F;
    machAsiInitStr.end_radius = 0.21F;

    fgets(plineBuffer, lineLen, pcockpitDataFile);
    gDebugLineNum ++;
    plinePtr = plineBuffer;
    ptoken = FindToken(&plinePtr, pseparators);

    while (strcmpi(ptoken, END_MARKER))
    {


        if ( not strcmpi(ptoken, PROP_CYCLEBITS_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%x", &objectInitStr.cycleBits);
        }
        else if ( not strcmpi(ptoken, PROP_DESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
                   &(objectInitStr.destRect.top),
                   &(objectInitStr.destRect.left),
                   &(objectInitStr.destRect.bottom),
                   &(objectInitStr.destRect.right));

            if ( not strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR))
            {
                objectInitStr.transparencyType = CPTRANSPARENT;
            }
            else if ( not strcmpi(ptransparencyStr, PROP_OPAQUE_STR))
            {
                objectInitStr.transparencyType = CPOPAQUE;
            }
            else
            {
                ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
            }
        }
        else if ( not strcmpi(ptoken, PROP_COLOR0_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &machAsiInitStr.color0);
        }
        else if ( not strcmpi(ptoken, PROP_COLOR1_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%lx", &machAsiInitStr.color1);
        }
        else if ( not strcmpi(ptoken, PROP_ENDANGLE))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &machAsiInitStr.end_angle);
        }
        else if ( not strcmpi(ptoken, PROP_ENDLENGTH))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &machAsiInitStr.end_radius);
        }
        else if ( not strcmpi(ptoken, PROP_CALLBACKSLOT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
        }
        else if ( not strcmpi(ptoken, PROP_MINVAL_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &machAsiInitStr.min_dial_value);
        }
        else if ( not strcmpi(ptoken, PROP_MAXVAL_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &machAsiInitStr.max_dial_value);
        }
        else if ( not strcmpi(ptoken, PROP_STARTANGLE_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &machAsiInitStr.dial_start_angle);
        }
        else if ( not strcmpi(ptoken, PROP_ARCLENGTH_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%f", &machAsiInitStr.dial_arc_length);
        }
        else if ( not strcmpi(ptoken, PROP_NEEDLERADIUS_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &machAsiInitStr.needle_radius);
        }
        else if ( not strcmpi(ptoken, PROP_PERSISTANT_STR))
        {
            ptoken = FindToken(&plinePtr, pseparators);
            sscanf(ptoken, "%d", &objectInitStr.persistant);

            if ( not objectInitStr.persistant)
            {
                objectInitStr.bsrcRect.top = 0;
                objectInitStr.bsrcRect.left = 0;
                objectInitStr.bsrcRect.bottom = 0;
                objectInitStr.bsrcRect.right = 0;

                objectInitStr.bdestRect = objectInitStr.bsrcRect;
                objectInitStr.bsurface = -1;
            }
        }
        else if ( not strcmpi(ptoken, PROP_BSRCLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top),
                   &(objectInitStr.bsrcRect.left),
                   &(objectInitStr.bsrcRect.bottom),
                   &(objectInitStr.bsrcRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BDESTLOC_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top),
                   &(objectInitStr.bdestRect.left),
                   &(objectInitStr.bdestRect.bottom),
                   &(objectInitStr.bdestRect.right));
        }
        else if ( not strcmpi(ptoken, PROP_BSURFACE_STR))
        {
            ptoken = FindToken(&plinePtr, "=;\n");
            sscanf(ptoken, "%d", &(objectInitStr.bsurface));
        }
        else
        {
            CockpitMessage(ptoken, "MachAsi", gDebugLineNum);
        }

        fgets(plineBuffer, lineLen, pcockpitDataFile);
        gDebugLineNum ++;
        plinePtr = plineBuffer;
        ptoken = FindToken(&plinePtr, pseparators);
    }

    objectInitStr.hScale = mHScale;
    objectInitStr.vScale = mVScale;
    objectInitStr.idNum = idNum;
    objectInitStr.pOTWImage = mpOTWImage;
    objectInitStr.pTemplate = gpTemplateSurface;
    objectInitStr.pCPManager = this;

    CPMachAsi *p = new CPMachAsi(&objectInitStr, &machAsiInitStr);
    ShiAssert(p);

    if ( not p) return;

    mpObjects.push_back(p);
    mObjectTally++;
}

//====================================================//
// CockpitManager::Exec
//====================================================//

void CockpitManager::Exec()
{

    mViewChanging = FALSE;

    if ( not mpActivePanel)
    {
        return;
    }

    if ( not mpOwnship)
    {
        return;
    }

    if (mIsInitialized == FALSE)
    {
        mpActivePanel->Exec(mpOwnship, CYCLE_ALL); //RUN ALL
        mIsInitialized = TRUE;
        mIsNextInitialized = TRUE;
    }
    else
    {
        if (mCycleBit == END_CYCLE)
        {
            mCycleBit = BEGIN_CYCLE;
        }
        else
        {
            mCycleBit = mCycleBit << 1;
        }

        mpActivePanel->Exec(mpOwnship, mCycleBit);
    }
}


void CockpitManager::CockAttachWeapons(void)
{
    int stationNum;
    SMSClass *sms = SimDriver.GetPlayerAircraft()->Sms;
    DrawableBSP* child;

    for (stationNum = 1; stationNum < sms->NumHardpoints(); stationNum++)
    {
        // MLR 2/20/2004 - new rack code, compatible with SP3 still
        child = sms->hardPoint[stationNum]->GetTopDrawable();

        if (child) mpGeometry->AttachChild(child, stationNum - 1);
    }
}


void CockpitManager::CockDetachWeapons(void)
{
    int stationNum;
    SMSClass *sms = SimDriver.GetPlayerAircraft()->Sms;
    DrawableBSP* child;

    for (stationNum = 1; stationNum < sms->NumHardpoints(); stationNum++)
    {
        // MLR 2/20/2004 - new rack code, compatible with SP3 still
        child = sms->hardPoint[stationNum]->GetTopDrawable();

        if (child) mpGeometry->DetachChild(child, stationNum - 1);
    }
}



//====================================================//
// CockpitManager::GeometryDraw
//====================================================//

void CockpitManager::GeometryDraw(void)
{
    Tpoint tempLight, worldLight;
    BOOL drawWing = TRUE;
    BOOL drawReflection = TRUE;
    BOOL drawOrdinance = TRUE;
    AircraftClass *pac = SimDriver.GetPlayerAircraft();

    if (mpActivePanel and mpGeometry)
    {
        {
            // MLR 2003-10-05 Get Texture set
            // this needs to be placed somewhere where it only runs once
            if (pac->drawPointer->GetClass() == DrawableObject::BSP)
            {
                DrawableBSP *bsp = (DrawableBSP*)SimDriver.GetPlayerEntity()->drawPointer;
                int t = bsp->GetTextureSet();

                if (mpGeometry->GetNTextureSet() not_eq 0)
                    mpGeometry->SetTextureSet(t % mpGeometry->GetNTextureSet());
                else
                    mpGeometry->SetTextureSet(0);
            }
        }

        if ( not mpActivePanel->DoGeometry() or PlayerOptions.ObjectDetailLevel() < 1.5F)
        {
            drawOrdinance = FALSE;
        }

        // MLR 2003-10-05 Commented this out.  Who wants to fly a plane with no wings?
        // This switch seems to be unused by the 3d pits anyhow. 7 is put to better use
        // a few lines down from here.
        // MLR 2003-10-12 Uncommented this out, we'll just work around it later.
        if (mpActivePanel->DoGeometry() and PlayerOptions.ObjectDetailLevel() >= 1.0F)
        {
            mpGeometry->SetSwitchMask(7, 1);
        }
        else
        {
            mpGeometry->SetSwitchMask(7, 0);
            drawWing = FALSE;
        }


        if (PlayerOptions.SimVisualCueMode == VCReflection or PlayerOptions.SimVisualCueMode == VCBoth)
        {
            mpGeometry->SetSwitchMask(3, 1);
        }
        else
        {
            mpGeometry->SetSwitchMask(3, 0);
            drawReflection = FALSE;
        }

        if (drawWing or drawReflection)
        {

            // Wombat778 2-24-04 Set the FOV back to 60 for drawing the wings and ordinance.
            // Fixes the disembodied wings phenomenon
            // 2-25-04 Since there are a few very minor side effects, decided to make this optional.
            float tempfov;

            if (g_b2DPitWingFOVFix)
            {
                tempfov = OTWDriver.GetFOV();
                OTWDriver.SetFOV(60.0f * DTR);
            }

            // Force object texture on for this part regardless of player settings
            BOOL oldState = OTWDriver.renderer->GetObjectTextureState();
            OTWDriver.renderer->SetObjectTextureState(TRUE);

            // Setup the local lighting and transform environment (body relative)
            OTWDriver.renderer->GetLightDirection(&worldLight);
            MatrixMultTranspose(&(OTWDriver.ownshipRot), &worldLight, &tempLight);
            OTWDriver.renderer->SetLightDirection(&tempLight);
            // COBRA - RED - Pit Vibrations
            Tpoint HeadOrigin = Origin;
            Tpoint Turb = pac->GetTurbulence();
            HeadOrigin.y = Turb.x;
            HeadOrigin.z -= Turb.z;

            OTWDriver.renderer->SetCamera(&HeadOrigin, &(OTWDriver.headMatrix));
            SMSClass *sms = pac->Sms;

            // COBRA - DX - if using DX Engine, Canopy has to be Oriented
            mpGeometry->orientation = OTWDriver.ownshipRot;
            // Draw the cockpit object
            mpGeometry->Draw(OTWDriver.renderer);

            // Put the light back into world space
            OTWDriver.renderer->SetLightDirection(&worldLight);

            // Restore camera
            OTWDriver.renderer->SetCamera(&(OTWDriver.cameraPos), &(OTWDriver.cameraRot));

            // Restore the texture settings
            OTWDriver.renderer->SetObjectTextureState(oldState);

            // MLR 2003-10-12
            // I moved all my previous animation code to the AircraftClass,
            // it's more readily (more likely) to get updated there as new
            // DOFs and Switches are added.
            //
            // Also both 2d and 3d pit had the exact same duplicate code.
            pac->CopyAnimationsToPit(mpGeometry);

            //Wombat778 2-25-04 Since there are a few very minor side effects, decided to make this optional.
            if (g_b2DPitWingFOVFix)
            {
                OTWDriver.SetFOV(tempfov); //Wombat778 2-24-04 Set the FOV back to what it was
            }
        }

    }
}


//====================================================//
// CockpitManager::DisplayBlit
//====================================================//

void CockpitManager::DisplayBlit()
{
#if DO_HIRESCOCK_HACK

    if (gDoCockpitHack)
    {
        return;
    }

    if (mIsInitialized and mpActivePanel)
    {
        mpActivePanel->DisplayBlit();
    }

#else

    if (mIsInitialized and mpActivePanel)
    {
        mpActivePanel->DisplayBlit();
    }

#endif
}

// OW
void CockpitManager::DisplayBlit3D()
{
#if DO_HIRESCOCK_HACK

    if (gDoCockpitHack)
    {
        return;
    }

    if (mIsInitialized and mpActivePanel)
    {
        mpActivePanel->DisplayBlit3D();
    }

#else

    if (mIsInitialized and mpActivePanel)
    {
        mpActivePanel->DisplayBlit3D();
    }

#endif
}

//====================================================//
// CockpitManager::DisplayDraw
//====================================================//

void CockpitManager::DisplayDraw()
{

#if DO_HIRESCOCK_HACK

    if (gDoCockpitHack)
    {
        return;
    }

    if (mIsInitialized and mpActivePanel)
    {
        OTWDriver.renderer->StartDraw();
        OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
        OTWDriver.renderer->CenterOriginInViewport();
        OTWDriver.renderer->ZeroRotationAboutOrigin();

        mpActivePanel->DisplayDraw();
        OTWDriver.renderer->EndDraw();
    }

#else

    if (mIsInitialized and mpActivePanel)
    {
        OTWDriver.renderer->StartDraw();
        OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
        OTWDriver.renderer->CenterOriginInViewport();
        OTWDriver.renderer->ZeroRotationAboutOrigin();

        mpActivePanel->DisplayDraw();
        OTWDriver.renderer->EndDraw();
    }

#endif
}

CPButtonObject* CockpitManager::GetButtonPointer(int buttonId)
{

    BOOL found = FALSE;
    int i = 0;
    CPButtonObject* preturnValue = NULL;

    while ( not found and i < mNumButtons)
    {
        if (mpButtonObjects[i]->GetId() == buttonId)
        {
            found = TRUE;
            preturnValue = mpButtonObjects[i];
        }
        else
        {
            i++;
        }
    }

    F4Assert(found);
    return(preturnValue);
}


//====================================================//
// CockpitManager::Dispatch
//====================================================//

void CockpitManager::Dispatch(int buttonId, int mouseSide)
{

    BOOL found = FALSE;
    int i = 0;
    int event;

    while ( not found and i < mButtonTally)
    {
        if (mpButtonObjects[i]->GetId() == buttonId)
        {
            found = TRUE;

            if (mouseSide == STARTDAT_MOUSE_LEFT)
            {
                event = CP_MOUSE_BUTTON0;
            }
            else
            {
                event = CP_MOUSE_BUTTON1;
            }

            mpButtonObjects[i]->HandleEvent(event);
        }
        else
        {
            i++;
        }
    }

    // F4Assert(found);
}



int CockpitManager::POVDispatch(int direction, int curXPos, int curYPos)
{
    int cursorIndex = -1;

    if (SimDriver.InSim())
    {
        if (mpActivePanel)
        {
            if (mpActivePanel->POVDispatch(direction))
            {
                if (mpNextActivePanel)
                {
                    mpNextActivePanel->Dispatch(&cursorIndex, CP_MOUSE_MOVE, curXPos, curYPos);
                }
                else
                {
                    MonoPrint("Cannot Change to Next Active Panel\n");
                }

                F4SoundFXSetDist(SFX_CP_CHNGVIEW, TRUE, 0.0f, 1.0f);
            }
        }
        else
        {
            cursorIndex = -1;
        }
    }
    else
    {
        cursorIndex = -1;
    }

    return cursorIndex;
}


int CockpitManager::Dispatch(int event, int xpos, int ypos)
{

    int cursorIndex;

    if (SimDriver.InSim())
    {
        if (mpActivePanel)
        {
            if (mpActivePanel->Dispatch(&cursorIndex, event, xpos, ypos))
            {
                if (mpNextActivePanel)
                {
                    mpNextActivePanel->Dispatch(&cursorIndex, CP_MOUSE_MOVE, xpos, ypos);
                }
                else
                {
                    MonoPrint("Cannot Change to Next Active Panel\n");
                }

                F4SoundFXSetDist(SFX_CP_CHNGVIEW, TRUE, 0.0f, 1.0f);
            }
        }
        else
        {
            cursorIndex = -1;
        }
    }
    else
    {
        cursorIndex = -1;
    }

    return cursorIndex;
}


//====================================================//
// CockpitManager::SetOwnship
//====================================================//

void CockpitManager::SetOwnship(SimBaseClass* paircraftClass)
{

    mpOwnship = paircraftClass;

    if (mpIcp)
    {
        mpIcp->SetOwnship(); // necessary for now, may not need when done
    }
}

//====================================================//
// CockpitManager::GetCockpitMaskTop
//====================================================//

float CockpitManager::GetCockpitMaskTop()
{

    float returnValue;

#if DO_HIRESCOCK_HACK

    if (mpActivePanel and not gDoCockpitHack)
    {
        returnValue = mpActivePanel->mMaskTop;
    }
    else
    {
        returnValue = -1.0F;
    }

#else

    if (mpActivePanel)
    {
        returnValue = mpActivePanel->mMaskTop;
    }
    else
    {
        returnValue = -1.0F;
    }

#endif
    return(returnValue);
}


//====================================================//
// CockpitManager::SetActivePanel
//====================================================//

bool CockpitManager::SetActivePanel(int panelId)   //Wombat778 changed return to bool
{

    int i = 0;
    bool found = FALSE; //Wombat778 4-13-04 changed to bool

    mCycleBit = BEGIN_CYCLE;

    if (panelId == PANELS_INACTIVE)
    {

        F4EnterCriticalSection(mpCockpitCritSec);

        if (mpActivePanel)
        {
            mpActivePanel->DiscardLitSurfaces();
        }

        mpActivePanel = NULL;
        mpNextActivePanel = NULL;

        F4LeaveCriticalSection(mpCockpitCritSec);

        mIsNextInitialized = FALSE;
    }
    else if (mpActivePanel == NULL or mpActivePanel->mIdNum not_eq panelId)
    {

        // loop thru all the panels
        while (( not found) and (i < mPanelTally))
        {
            // if we find the panel with our id, make it active
            if (mpPanels[i]->mIdNum == panelId)
            {

                F4EnterCriticalSection(mpCockpitCritSec);
                mpNextActivePanel = mpPanels[i];
                F4LeaveCriticalSection(mpCockpitCritSec);
                mIsNextInitialized = FALSE;
                found = TRUE;
            }
            else
            {
                i++;
            }
        }

        F4Assert(found);
    }

    // MonoPrint("panelID == %d\n", panelId);
    return found; //Wombat778 4-13-04 return the status
}

//====================================================//
// CockpitManager::ShowRwr
//====================================================//
BOOL CockpitManager::ShowRwr(void)
{

    if (mpViewBounds[BOUNDS_RWR])
    {
        return(TRUE);
    }
    else if (mpActivePanel and mpActivePanel->mpViewBounds[BOUNDS_RWR])
    {
        return(TRUE);
    }

    return (FALSE);
}


//====================================================//
// CockpitManager::ShowHud
//====================================================//
BOOL CockpitManager::ShowHud(void)
{

    if (mpViewBounds[BOUNDS_HUD])
    {
        return(TRUE);
    }
    else if (mpActivePanel and mpActivePanel->mpViewBounds[BOUNDS_HUD])
    {
        return(TRUE);
    }

    return (FALSE);
}

//====================================================//
// CockpitManager::ShowMfd
//====================================================//
BOOL CockpitManager::ShowMfd(void)
{

    if (mpViewBounds[BOUNDS_MFDLEFT] and mpViewBounds[BOUNDS_MFDRIGHT])
    {
        return(TRUE);
    }
    else if (mpActivePanel and mpActivePanel->mpViewBounds[BOUNDS_MFDLEFT] and mpActivePanel->mpViewBounds[BOUNDS_MFDRIGHT])
    {
        return(TRUE);
    }

    return (FALSE);
}

int CockpitManager::HudFont(void)
{
    int retval;

    if (mpActivePanel)
    {
        retval = mpActivePanel->HudFont();
    }
    else
    {
        retval = mHudFont;
    }

    return retval;
}

int CockpitManager::MFDFont(void)
{
    int retval;

    if (mpActivePanel)
    {
        retval = mpActivePanel->MFDFont();
    }
    else
    {
        retval = mMFDFont;
    }

    return retval;
}

int CockpitManager::DEDFont(void)
{
    int retval;

    if (mpActivePanel)
    {
        retval = mpActivePanel->DEDFont();
    }
    else
    {
        retval = mDEDFont;
    }

    return retval;
}
//====================================================//
// CockpitManager::GetViewportBounds
//====================================================//
BOOL CockpitManager::GetViewportBounds(ViewportBounds* bounds, int viewPort)
{

    BOOL returnValue = FALSE;
#if DO_HIRESCOCK_HACK

    if (mpActivePanel and not gDoCockpitHack)
    {
        returnValue = mpActivePanel->GetViewportBounds(bounds, viewPort);
    }
    else if (mpViewBounds[viewPort])
    {
        *bounds = *mpViewBounds[viewPort];
        returnValue = TRUE;
    }

#else

    if (mpActivePanel)
    {
        returnValue = mpActivePanel->GetViewportBounds(bounds, viewPort);
    }
    else if (mpViewBounds[viewPort])
    {
        *bounds = *mpViewBounds[viewPort];
        returnValue = TRUE;
    }

#endif
    return(returnValue);
}

void CockpitManager::SetNextView(void)
{
    F4EnterCriticalSection(mpCockpitCritSec);

    if ( not mIsNextInitialized and mpNextActivePanel)
    {
        if (mpNextActivePanel not_eq mpActivePanel)
        {

            // OW
            // keep them all in video memory, otherwise the texture manger eats up all the video memory and we have to create them in system memory
            // if( not PlayerOptions.bFast2DCockpit)
            {
                if (mpActivePanel)
                    mpActivePanel->DiscardLitSurfaces();
            }

            mpNextActivePanel->CreateLitSurfaces(lightLevel);
            mpActivePanel = mpNextActivePanel;

            mMouseBounds = mpActivePanel->mMouseBounds;
            mpActivePanel->SetDirtyFlags();

            mpNextActivePanel = NULL;
            mIsInitialized = FALSE;
            mViewChanging = TRUE;
        }
    }

    F4LeaveCriticalSection(mpCockpitCritSec);
}


#if DO_HIRESCOCK_HACK
float CockpitManager::GetPan(void)
{

    if (mpActivePanel and not gDoCockpitHack)
    {
        return (mpActivePanel->mPan);
    }

    return (0.0F);
}

float CockpitManager::GetTilt(void)
{

    if (mpActivePanel and not gDoCockpitHack)
    {
        return (mpActivePanel->mTilt);
    }

    return (0.0F);
}
#else
float CockpitManager::GetPan(void)
{

    if (mpActivePanel)
    {
        return (mpActivePanel->mPan);
    }

    return (0.0F);
}

float CockpitManager::GetTilt(void)
{

    if (mpActivePanel)
    {
        return (mpActivePanel->mTilt);
    }

    return (0.0F);
}
#endif

void CockpitManager::SetTOD(float newLightLevel)
{

    /* if ((fabs(lightLevel - newLightLevel) <= COCKPIT_LIGHT_CHANGE_TOLERANCE) and 
     (OTWDriver.renderer->GetGreenMode() not_eq inNVGmode)) {
     return;
     }*/

    lightLevel = newLightLevel;

    // Apply lighting effects to palette here
    if (mpActivePanel)
    {
        mpActivePanel->SetPalette();
    }
}

void CockpitManager::UpdatePalette()
{
    // Apply lighting effects to palette here
    if (mpActivePanel)
    {
        mpActivePanel->SetPalette();
    }
}


/***************************************************************************\
 Update the light level on the cockpit.
 \***************************************************************************/
void CockpitManager::TimeUpdateCallback(void *self)
{
    ((CockpitManager*)self)->SetTOD(TheTimeOfDay.GetLightLevel());
}


// Save the current cockpit state
void CockpitManager::SaveCockpitDefaults(void)
{
    char dataFileName[_MAX_PATH];
    char tmpStr[_MAX_PATH];
    char tmpStr1[_MAX_PATH];
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    sprintf(dataFileName, "%s\\config\\%s.ini", FalconDataDirectory, LogBook.Callsign());

    // Save HUD Data  COBRA - RED - No more used
    sprintf(tmpStr, "%d", TheHud->GetHudColor());
    WritePrivateProfileString("Hud", "Color", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", TheHud->GetScalesSwitch());
    WritePrivateProfileString("Hud", "Scales", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", TheHud->GetBrightnessSwitch());
    WritePrivateProfileString("Hud", "Brightness", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", TheHud->GetFPMSwitch());
    WritePrivateProfileString("Hud", "FPM", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", TheHud->GetDEDSwitch());
    WritePrivateProfileString("Hud", "DED", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", TheHud->GetVelocitySwitch());
    WritePrivateProfileString("Hud", "Velocity", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", TheHud->GetRadarSwitch());
    WritePrivateProfileString("Hud", "Alt", tmpStr, dataFileName);

    //MI
    sprintf(tmpStr, "%d", (int)(TheHud->SymWheelPos * 1000));
    WritePrivateProfileString("Hud", "SymWheelPos", tmpStr, dataFileName);

    // Save the ICP Master Mode
    if (g_bRealisticAvionics)
    {
        if (mpIcp)
        {
            if (mpIcp->IsICPSet(ICPClass::MODE_A_G))
            {
                sprintf(tmpStr, "%d", 1);
                WritePrivateProfileString("ICP", "MasterMode", tmpStr, dataFileName);
            }
            else if (mpIcp->IsICPSet(ICPClass::MODE_A_A))
            {
                sprintf(tmpStr, "%d", 2);
                WritePrivateProfileString("ICP", "MasterMode", tmpStr, dataFileName);
            }
            else
            {
                sprintf(tmpStr, "%d", 0);
                WritePrivateProfileString("ICP", "MasterMode", tmpStr, dataFileName);
            }
        }
    }
    else
    {
        if (mpIcp->GetPrimaryExclusiveButton())
        {
            sprintf(tmpStr, "%d", mpIcp->GetPrimaryExclusiveButton()->GetId());
            sprintf(tmpStr1, "%d", mpIcp->GetICPPrimaryMode());
        }
        else
        {
            sprintf(tmpStr, "-1");
            sprintf(tmpStr1, "-1");
        }

        WritePrivateProfileString("ICP", "PrimaryId", tmpStr, dataFileName);
        WritePrivateProfileString("ICP", "PrimaryMode", tmpStr1, dataFileName);

        if (mpIcp->GetSecondaryExclusiveButton())
        {
            sprintf(tmpStr, "%d", mpIcp->GetSecondaryExclusiveButton()->GetId());
            sprintf(tmpStr1, "%d", mpIcp->GetICPSecondaryMode());
        }
        else
        {
            sprintf(tmpStr, "-1");
            sprintf(tmpStr1, "-1");
        }

        WritePrivateProfileString("ICP", "SecondaryId", tmpStr, dataFileName);
        WritePrivateProfileString("ICP", "SecondaryMode", tmpStr1, dataFileName);

        if (mpIcp->GetTertiaryExclusiveButton())
        {
            sprintf(tmpStr, "%d", mpIcp->GetTertiaryExclusiveButton()->GetId());
            sprintf(tmpStr1, "%d", mpIcp->GetICPTertiaryMode());
        }
        else
        {
            sprintf(tmpStr, "-1");
            sprintf(tmpStr1, "-1");
        }

        WritePrivateProfileString("ICP", "TertiaryId", tmpStr, dataFileName);
        WritePrivateProfileString("ICP", "TertiaryMode", tmpStr1, dataFileName);
    }

    // Save the MFD States
    for (int i = 0; i < NUM_MFDS; i++)
    {
        sprintf(tmpStr1, "Display%d", i);
        sprintf(tmpStr, "%d", MfdDisplay[i]->mode);
        WritePrivateProfileString("MFD", tmpStr1, tmpStr, dataFileName);

        // JPO - having written old format, dump the new format
        for (int mm = 0; mm < MFDClass::MAXMM; mm++)
        {
            for (int j = 0; j < 3; j++)
            {
                sprintf(tmpStr1, "Display%d-%d-%d", i, mm, j);
                sprintf(tmpStr, "%d", MfdDisplay[i]->primarySecondary[mm][j]);
                WritePrivateProfileString("MFD", tmpStr1, tmpStr, dataFileName);
            }

            sprintf(tmpStr1, "Display%d-%d-csel", i, mm);
            sprintf(tmpStr, "%d", MfdDisplay[i]->cursel[mm]);
            WritePrivateProfileString("MFD", tmpStr1, tmpStr, dataFileName);
        }

    }

    //MI save EWS stuff
    if (mpIcp)
    {
        //Chaff and Flare Bingo
        sprintf(tmpStr1, "Flare Bingo");
        sprintf(tmpStr, "%d", mpIcp->FlareBingo);
        WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

        sprintf(tmpStr1, "Chaff Bingo");
        sprintf(tmpStr, "%d", mpIcp->ChaffBingo);
        WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

        sprintf(tmpStr1, "Jammer");
        sprintf(tmpStr, "%d", mpIcp->EWS_JAMMER_ON ? 1 : 0);
        WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

        sprintf(tmpStr1, "Bingo");
        sprintf(tmpStr, "%d", mpIcp->EWS_BINGO_ON ? 1 : 0);
        WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

    }

    int i = 0;

    for (i = 0; i < MAX_PGMS - 1; i++)
    {
        if (mpIcp)
        {
            //Chaff Burst quantity
            sprintf(tmpStr1, "PGM %d Chaff BQ" , i);
            sprintf(tmpStr, "%d", mpIcp->iCHAFF_BQ[i]);
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Chaff Burst Interval
            sprintf(tmpStr1, "PGM %d Chaff BI" , i);
            sprintf(tmpStr, "%d", (int)(mpIcp->fCHAFF_BI[i] * 1000));
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Chaff Salvo quantity
            sprintf(tmpStr1, "PGM %d Chaff SQ", i);
            sprintf(tmpStr, "%d", mpIcp->iCHAFF_SQ[i]);
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Chaff Salvo Interval
            sprintf(tmpStr1, "PGM %d Chaff SI" , i);
            sprintf(tmpStr, "%d", (int)(mpIcp->fCHAFF_SI[i] * 1000));
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Flare Burst quantity
            sprintf(tmpStr1, "PGM %d Flare BQ" , i);
            sprintf(tmpStr, "%d", mpIcp->iFLARE_BQ[i]);
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Flare Burst Interval
            sprintf(tmpStr1, "PGM %d Flare BI" , i);
            sprintf(tmpStr, "%d", (int)(mpIcp->fFLARE_BI[i] * 1000));
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Flare Salvo quantity
            sprintf(tmpStr1, "PGM %d Flare SQ", i);
            sprintf(tmpStr, "%d", mpIcp->iFLARE_SQ[i]);
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

            //Flare Salvo Interval
            sprintf(tmpStr1, "PGM %d Flare SI" , i);
            sprintf(tmpStr, "%d", (int)(mpIcp->fFLARE_SI[i] * 1000));
            WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);
        }
    }

    //Save the current PGM and number selection
    if (playerAC)
    {
        sprintf(tmpStr1, "Mode Selection");
        sprintf(tmpStr, "%d", playerAC->EWSPGM());
        WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

        sprintf(tmpStr1, "Number Selection");
        sprintf(tmpStr, "%d", playerAC->EWSProgNum);
        WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);
    }

    //MI save Bullseye show option
    if (mpIcp)
    {
        //Chaff and Flare Bingo
        sprintf(tmpStr1, "BullseyeInfoOnMFD");
        sprintf(tmpStr, "%d", mpIcp->ShowBullseyeInfo ? 1 : 0);
        WritePrivateProfileString("Bullseye", tmpStr1, tmpStr, dataFileName);
    }

    //MI load the laser starting time
    if (mpIcp)
    {
        sprintf(tmpStr1, "LaserST");
        sprintf(tmpStr, "%d", mpIcp->LaserTime);
        WritePrivateProfileString("Laser", tmpStr1, tmpStr, dataFileName);
    }

    //MI save Cockpit selection
    if (playerAC)
    {
        sprintf(tmpStr1, "WideView");
        sprintf(tmpStr, "%d", playerAC->WideView ? 1 : 0);
        WritePrivateProfileString("Cockpit View", tmpStr1, tmpStr, dataFileName);
    }


    // Save the current OTW View
    sprintf(tmpStr, "%d", OTWDriver.GetOTWDisplayMode());
    WritePrivateProfileString("OTW", "Mode", tmpStr, dataFileName);

    sprintf(tmpStr, "%d", VM->GetRadioFreq(0));
    WritePrivateProfileString("COMMS", "Comm1", tmpStr, dataFileName);
    sprintf(tmpStr, "%d",  VM->GetRadioFreq(1));
    WritePrivateProfileString("COMMS", "Comm2", tmpStr, dataFileName);

    // Master Arm switch
    if (playerAC and playerAC->Sms) // JB 010628 CTD // JPO CTD fix
    {
        sprintf(tmpStr, "%d", playerAC->Sms->MasterArm());
        WritePrivateProfileString("Weapons", "MasterArm", tmpStr, dataFileName);
    }
}


// Save the current cockpit state
void CockpitManager::LoadCockpitDefaults(void)
{
    char dataFileName[_MAX_PATH], tmpStr[512];
    int pMode, sMode, tMode, i;
    int pButton, sButton, tButton, buttonId;
    CPButtonObject* theButton;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    sprintf(dataFileName, "%s\\config\\%s.ini", FalconDataDirectory, LogBook.Callsign());

    // Load HUD Data
    TheHud->SetHudColor(GetPrivateProfileInt("Hud", "Color", TheHud->GetHudColor(), dataFileName)); // COBRA - RED - NO MORE USED
    TheHud->SetScalesSwitch((HudClass::ScalesSwitch)GetPrivateProfileInt("Hud", "Scales", TheHud->GetScalesSwitch(), dataFileName));
    OTWDriver.pCockpitManager->Dispatch(1066, 0);
    TheHud->SetFPMSwitch((HudClass::FPMSwitch)GetPrivateProfileInt("Hud", "FPM", TheHud->GetFPMSwitch(), dataFileName));
    OTWDriver.pCockpitManager->Dispatch(1067, 0);
    TheHud->SetDEDSwitch((HudClass::DEDSwitch)GetPrivateProfileInt("Hud", "DED", TheHud->GetDEDSwitch(), dataFileName));
    OTWDriver.pCockpitManager->Dispatch(1068, 0);
    TheHud->SetVelocitySwitch((HudClass::VelocitySwitch)GetPrivateProfileInt("Hud", "Velocity", TheHud->GetVelocitySwitch(), dataFileName));
    OTWDriver.pCockpitManager->Dispatch(1069, 0);
    TheHud->SetRadarSwitch((HudClass::RadarSwitch)GetPrivateProfileInt("Hud", "Alt", TheHud->GetRadarSwitch(), dataFileName));
    OTWDriver.pCockpitManager->Dispatch(1070, 0);
    TheHud->SetBrightnessSwitch((HudClass::BrightnessSwitch)GetPrivateProfileInt("Hud", "Brightness", TheHud->GetBrightnessSwitch(), dataFileName));
    OTWDriver.pCockpitManager->Dispatch(1071, 0);

    //MI
    //ATARIBABY/WOMBAT Hud sym wheel ramp start fix
    if (TheHud->SymWheelPos > 0.5F)
    {
        TheHud->SymWheelPos = (float)GetPrivateProfileInt("Hud", "SymWheelPos", 1000, dataFileName);
        TheHud->SymWheelPos /= 1000.0F;
        TheHud->SymWheelPos = max(0.5F, min(TheHud->SymWheelPos, 1.0F));
        TheHud->SetLightLevel();
    }

    // Reload the ICP Master Mode
    //MI
    if (g_bRealisticAvionics)
    {
        int Mode = GetPrivateProfileInt("ICP", "MasterMode", -1, dataFileName);

        if (Mode >= 0 and Mode < 3)
        {
            if (mpIcp)
            {
                switch (Mode)
                {
                    case 0: //NAV
                        if (mpIcp->IsICPSet(ICPClass::MODE_A_G))
                        {
                            SimICPAG(0, KEY_DOWN, NULL);
                        }
                        else if (mpIcp->IsICPSet(ICPClass::MODE_A_A))
                        {
                            SimICPAA(0, KEY_DOWN, NULL);
                        }

                        break;

                    case 1: //AG
                        if ( not mpIcp->IsICPSet(ICPClass::MODE_A_G))
                        {
                            SimICPAG(0, KEY_DOWN, NULL);
                        }

                        break;

                    case 2: //AA
                        if ( not mpIcp->IsICPSet(ICPClass::MODE_A_A))
                        {
                            SimICPAA(0, KEY_DOWN, NULL);
                        }

                        break;

                    default:
                        break;
                }

                mpIcp->ChangeToCNI();
                mpIcp->LastMode = CNI_MODE;
                mpIcp->SetICPTertiaryMode(CNI_MODE);
            }
        }
    }
    else
    {
        pButton = GetPrivateProfileInt("ICP", "PrimaryId", -1, dataFileName);
        sButton = GetPrivateProfileInt("ICP", "SecondaryId", -1, dataFileName);
        tButton = GetPrivateProfileInt("ICP", "TertiaryId", -1, dataFileName);
        pMode = GetPrivateProfileInt("ICP", "PrimaryMode", mpIcp->GetICPPrimaryMode(), dataFileName);
        sMode = GetPrivateProfileInt("ICP", "SecondaryMode", mpIcp->GetICPSecondaryMode(), dataFileName);
        tMode = GetPrivateProfileInt("ICP", "TertiaryMode", mpIcp->GetICPTertiaryMode(), dataFileName);



        if (VM)
        {
            VM->ChangeRadioFreq(GetPrivateProfileInt("COMMS", "Comm1", rcfFlight1, dataFileName), 0);
            VM->ChangeRadioFreq(GetPrivateProfileInt("COMMS", "Comm2", rcfPackage1, dataFileName), 1);
        }

        // Find the primary mode button
        buttonId = pButton;
        i = 0;
        theButton = NULL;

        // search cpmanager's list of button pointers
        while (i < mButtonTally)
        {
            if (mpButtonObjects[i]->GetId() == buttonId)
            {
                theButton = mpButtonObjects[i];
                break;
            }
            else
            {
                i++;
            }
        }

        // If found, push it
        if (theButton)
        {
            theButton->HandleMouseEvent(3);  //Button 0 down
            theButton->HandleEvent(buttonId);
        }

        // Find the secondary mode button
        theButton = NULL;
        buttonId = sButton;
        i = 0;

        // search cpmanager's list of button pointers
        while (i < mButtonTally)
        {
            if (mpButtonObjects[i]->GetId() == buttonId)
            {
                theButton = mpButtonObjects[i];
                break;
            }
            else
            {
                i++;
            }
        }

        // If found, push it
        if (theButton)
        {
            theButton->HandleMouseEvent(3);  //Button 0 down
            theButton->HandleEvent(buttonId);
        }


        // Find the tertiary mode button
        theButton = NULL;
        buttonId = tButton;
        i = 0;

        // search cpmanager's list of button pointers
        while (i < mButtonTally)
        {
            if (mpButtonObjects[i]->GetId() == buttonId)
            {
                theButton = mpButtonObjects[i];
                break;
            }
            else
            {
                i++;
            }
        }

        // If found, push it
        if (theButton)
        {
            theButton->HandleMouseEvent(3);  //Button 0 down
            theButton->HandleEvent(buttonId);
        }
    }

    // load the OTW View - Cobra - override OTW mode with cobra.cfg setting
    if (g_bStartIn3Dpit)
    {
        OTWDriver.SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
    }
    else
    {
        OTWDriver.SetOTWDisplayMode(
            (OTWDriverClass::OTWDisplayMode)GetPrivateProfileInt(
                "OTW", "Mode", OTWDriver.GetOTWDisplayMode(), dataFileName));
    }

    // Load the MFD States
    for (int i = 0; i < NUM_MFDS; i++)
    {
        // JPO ignore the old format in favour of newer stuff
        //THW 2003-11-14 reactivated to make it work
        sprintf(tmpStr, "Display%d", i);
        MfdDisplay[i]->SetNewMode((MFDClass::MfdMode)GetPrivateProfileInt("MFD", tmpStr, MfdDisplay[i]->mode, dataFileName));

        //THW end
        for (int mm = 0; mm < MFDClass::MAXMM; mm++)
        {
            for (int j = 0; j < 3; j++)
            {
                sprintf(tmpStr, "Display%d-%d-%d", i, mm, j);
                int val = GetPrivateProfileInt("MFD", tmpStr, MfdDisplay[i]->primarySecondary[mm][j], dataFileName);
                MfdDisplay[i]->primarySecondary[mm][j] = (MFDClass::MfdMode)val;
            }

            //sprintf (tmpStr, "Display%d-%d-curmm", i, mm);
            sprintf(tmpStr, "Display%d-%d-csel", i, mm); //THW 2003-11-14 fixed typo
            MfdDisplay[i]->cursel[mm] = GetPrivateProfileInt("MFD", tmpStr, MfdDisplay[i]->cursel[mm], dataFileName);
        }
    }

    //MI EWS stuff
    if (mpIcp and SimDriver.GetPlayerEntity() and not F4IsBadReadPtr(SimDriver.GetPlayerEntity(), sizeof(AircraftClass)))
    {
        //Chaff and Flare Bingo
        mpIcp->FlareBingo = GetPrivateProfileInt("EWS", "Flare Bingo",
                            mpIcp->FlareBingo, dataFileName);

        mpIcp->ChaffBingo = GetPrivateProfileInt("EWS", "Chaff Bingo",
                            mpIcp->FlareBingo, dataFileName);

        //Jammer and Bingo
        int temp = GetPrivateProfileInt("EWS", "Jammer", playerAC->EWSProgNum, dataFileName);

        if (temp == 1)
        {
            mpIcp->EWS_JAMMER_ON = TRUE;
        }
        else
        {
            mpIcp->EWS_JAMMER_ON = FALSE;
        }

        temp = GetPrivateProfileInt("EWS", "Bingo", playerAC->EWSProgNum, dataFileName);

        if (temp == 1)
        {
            mpIcp->EWS_BINGO_ON = TRUE;
        }
        else
        {
            mpIcp->EWS_BINGO_ON = FALSE;
        }
    }

    for (int i = 0; i < MAX_PGMS - 1; i++)
    {
        if (mpIcp)
        {
            sprintf(tmpStr, "PGM %d Chaff BQ", i);
            mpIcp->iCHAFF_BQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iCHAFF_BQ[i],
                                  dataFileName);

            sprintf(tmpStr, "PGM %d Chaff BI", i);
            mpIcp->fCHAFF_BI[i] = (float)GetPrivateProfileInt("EWS", tmpStr, (int)(mpIcp->fCHAFF_BI[i] * 1000),
                                  dataFileName);
            mpIcp->fCHAFF_BI[i] /= 1000;

            sprintf(tmpStr, "PGM %d Chaff SQ", i);
            mpIcp->iCHAFF_SQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iCHAFF_SQ[i],
                                  dataFileName);

            sprintf(tmpStr, "PGM %d Chaff SI", i);
            mpIcp->fCHAFF_SI[i] = (float)GetPrivateProfileInt("EWS", tmpStr, (int)(mpIcp->fCHAFF_SI[i] * 1000),
                                  dataFileName);
            mpIcp->fCHAFF_SI[i] /= 1000;


            sprintf(tmpStr, "PGM %d Flare BQ", i);
            mpIcp->iFLARE_BQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iFLARE_BQ[i],
                                  dataFileName);

            sprintf(tmpStr, "PGM %d Flare BI", i);
            mpIcp->fFLARE_BI[i] = (float)GetPrivateProfileInt("EWS", tmpStr, (int)(mpIcp->fFLARE_BI[i] * 1000),
                                  dataFileName);
            mpIcp->fFLARE_BI[i] /= 1000;

            sprintf(tmpStr, "PGM %d Flare SQ", i);
            mpIcp->iFLARE_SQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iFLARE_SQ[i],
                                  dataFileName);

            sprintf(tmpStr, "PGM %d Flare SI", i);
            mpIcp->fFLARE_SI[i] = (float)GetPrivateProfileInt("EWS", tmpStr, (int)(mpIcp->fFLARE_SI[i] * 1000),
                                  dataFileName);
            mpIcp->fFLARE_SI[i] /= 1000;
        }
    }

    //MI save Bullseye show option
    if (mpIcp)
    {
        //Chaff and Flare Bingo
        sprintf(tmpStr, "BullseyeInfoOnMFD");
        int temp = GetPrivateProfileInt("Bullseye", tmpStr, 0, dataFileName);

        if (temp <= 0)
            mpIcp->ShowBullseyeInfo = FALSE;
        else
            mpIcp->ShowBullseyeInfo = TRUE;
    }

    //MI load the laser starting time
    if (mpIcp)
    {
        sprintf(tmpStr, "LaserST");
        int temp = GetPrivateProfileInt("Laser", tmpStr, 8, dataFileName);

        if (temp < 0)
            mpIcp->LaserTime = 8; //standard
        else if (temp > 176)
            mpIcp->LaserTime = 176; //maximum
        else
            mpIcp->LaserTime = temp;
    }

    //MI save Cockpit selection
    if (SimDriver.GetPlayerEntity() and not F4IsBadReadPtr(SimDriver.GetPlayerEntity(), sizeof(AircraftClass)))
    {
        sprintf(tmpStr, "WideView");
        int temp = GetPrivateProfileInt("Cockpit View", tmpStr, 0, dataFileName);

        if (temp <= 0)
        {
            playerAC->WideView = FALSE;
        }
        else
        {
            playerAC->WideView = TRUE;
        }

        if (OTWDriver.pCockpitManager)
        {
            if (playerAC->WideView)
            {
                OTWDriver.pCockpitManager->SetActivePanel(91100);
            }
            else
            {
                OTWDriver.pCockpitManager->SetActivePanel(1100);
            }
        }
    }

    // Master Arm
    //if (SimDriver.GetPlayerEntity()) // JB 010220 CTD
    // sfr: TODO take this JB hack out
    if (playerAC and 
        playerAC->Sms and 
 not F4IsBadReadPtr(playerAC, sizeof(AircraftClass)) and 
 not F4IsBadCodePtr((FARPROC) playerAC->Sms))
    {
        // JB 010220 CTD
        playerAC->Sms->SetMasterArm(
            (SMSBaseClass::MasterArmState)GetPrivateProfileInt(
                "Weapons", "MasterArm", playerAC->Sms->MasterArm(), dataFileName
            )
        );

        //MI EWS stuff
        playerAC->SetPGM((AircraftClass::EWSPGMSwitch)GetPrivateProfileInt("EWS", "Mode Selection",
                         playerAC->EWSPGM(), dataFileName));

        playerAC->EWSProgNum = GetPrivateProfileInt("EWS", "Number Selection",
                               playerAC->EWSProgNum, dataFileName);
    }

    OTWDriver.pCockpitManager->Dispatch(1097, 0);
}

/** Sfr: added these for cockpit lighting */
// applies cockpit lighting to a color, of a given type
DWORD CockpitManager::ApplyLighting(DWORD inColor, bool useInst)
{
    float cLight[3], iLight[3];
    ComputeLightFactors(cLight, iLight);

    float *light = (useInst) ? iLight : cLight;

    return (OTWDriver.renderer->GetGreenMode()) ?
           CalculateNVGColor(CalculateColor(inColor, light[0], light[1], light[2])) :
           CalculateColor(inColor, light[0], light[1], light[2])
           ;
}

//same as above, but uses RGB instead of BGR, returning BGR
DWORD CockpitManager::ApplyLightingToRGB(DWORD inColor, bool useInst)
{
    float cLight[3], iLight[3];
    ComputeLightFactors(cLight, iLight);

    float *light = (useInst) ? iLight : cLight;

    //invert color components
    DWORD red = inColor bitand 0xff0000;
    DWORD green = inColor bitand 0x00ff00;
    DWORD blue = inColor bitand 0x0000ff;
    DWORD colorBGR = (red) bitor (green << 8) bitor (blue << 16);

    return (OTWDriver.renderer->GetGreenMode()) ?
           CalculateNVGColor(CalculateColor(inColor, light[0], light[1], light[2])) :
           CalculateColor(inColor, light[0], light[1], light[2])
           ;
}


//compute cockpit lights
void CockpitManager::ComputeLightFactors(float *cLight, float *iLight)
{
    // minimum cockpit lighting, from enviroment
    float eLight = (lightLevel < 0.01f) ? 0.01f : lightLevel;

    // use one light for each color
    for (int i = 0; i < 3; i++)
    {
        cLight[i] = eLight;
        iLight[i] = eLight;
    }

    float floodFactor = 1.0f, instFactor = 1.0f;

    // COBRA - RED - Do it if still a Player Entity available
    AircraftClass *pAircraft = SimDriver.GetPlayerAircraft();

    if (pAircraft)
    {
        //flood
        int floodAux = pAircraft->GetInteriorLight();
        // sfr: this is totally out of place
        pAircraft->SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, (floodAux) ? 1 : 0);

        if (floodAux) pAircraft->SetAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);
        else pAircraft->ClearAcStatusBits(AircraftClass::ACSTATUS_PITLIGHT);

        bool floodOn = (floodAux > 0) ? true : false;
        floodFactor = (float)(floodAux) / 2.0f;

        // add lightLevel
        if (floodOn)
        {
            for (int i = 0; i < 3; i++)
            {
                cLight[i] = mFloodLight[i] * floodFactor + eLight;
                cLight[i] = (cLight[i] > 1.0f) ? 1.0f : cLight[i];
                iLight[i] = cLight[i];
            }
        }

        //instrument
        int instAux = pAircraft->GetInstrumentLight();
        bool instOn = (instAux > 0) ? true : false;
        instFactor = (float)(instAux) / 2.0f;

        //instrument lights with own illumination, but they can be less than flood
        //instrument light is added to existing flood light
        if (instOn)
        {
            for (int i = 0; i < 3; i++)
            {
                iLight[i] = mInstLight[i] * instFactor + cLight[i] + eLight;
                iLight[i] = (iLight[i] > 1.0f) ? 1.0f : iLight[i];
            }
        }
    }
}


//====================================================//
// CreateCockpitGeometry
//====================================================//

void CreateCockpitGeometry(DrawableBSP** ppGeometry, int normalType, int dogType)
{
    int model = 0;
    int team = 0;
    int texture = 1;

    BOOL isDogfight;

    isDogfight = FalconLocalGame->GetGameType() == game_Dogfight;

    if (isDogfight)
    {
        model = dogType; // change this to proper vis type when it gets into classtable
        team = FalconLocalSession->GetTeam();

        switch (team)
        {
            case 1:
                texture = 3;
                break;

            case 2:
                texture = 4;
                break;

            case 3:
                texture = 1;
                break;

            case 4:
            default:
                texture = 2;
                break;
        }
    }
    else
    {
        model = normalType; // change this to proper vis type when it gets into classtable
    }

    // Wings and reflection
    DrawableBSP::LockAndLoad(model);
    *ppGeometry = new DrawableBSP(model, &Origin, &IMatrix, 1.0f);

    // COBRA - DX - If Using the DX Engine everithing must have RL Scaling...
    if (g_bUse_DX_Engine)
        (*ppGeometry)->SetScale(1.0f); // COBRA - RED
    else
        (*ppGeometry)->SetScale(10.0f); // Keep the geometry away from the clipping plane


    if (isDogfight)
    {
        (*ppGeometry)->SetTextureSet(texture);
    }
}

/** sfr: applies lighting factors to all colors of the palette, generating the result palette on out */
void ApplyLightingToPalette(DWORD *in, DWORD *out, float rf, float gf, float bf)
{
    for (int i = 0; i < 256; i++)
    {
        out[i] = CalculateColor(in[i], rf, gf, bf);
    }
}

/** sfr: Added for night light in cockpit
 * calculates a color, affected by the 3 light components
 */
DWORD CalculateColor(DWORD inColor, float rf, float gf, float bf)
{
    //transparent case
    if (inColor == 0xffff0000)
    {
        return inColor;
    }

    //for now
    int red, green, blue;
    DWORD alpha;
    DWORD outColor;

    //we get RGB and preserve alpha
    blue = (int)(((inColor bitand 0x00ff0000) >> 16) *  bf);
    green = (int)(((inColor bitand 0x0000ff00) >>  8) *  gf);
    red = (int)(((inColor bitand 0x000000ff) >>  0) *  rf);
    alpha = inColor bitand 0xff000000;
    outColor = (alpha) bitor (red << 0) bitor (green << 8) bitor (blue << 16);

    return outColor;
}

/** srf: this is used to invert a pixel from RGB to BGR, and vice-versa */
DWORD InvertRGBOrder(DWORD inColor)
{
    int c1, c2, c3;

    c1 = (inColor bitand 0xff0000);
    c2 = (inColor bitand 0x00ff00);
    c3 = (inColor bitand 0x0000ff);

    DWORD outColor = (c1 >> 16) bitor (c2) bitor (c3 << 16);
    return outColor;
}

//we keep the alpha value
DWORD CalculateNVGColor(DWORD inColor)
{
    //transparent case
    if (inColor == 0xffff0000)
    {
        return inColor;
    }

    DWORD nvgColor = 0;
    DWORD alpha;
    int red;
    int green;
    int blue;

    red = (inColor bitand 0x000000ff);
    green = (inColor bitand 0x0000ff00) >> 8;
    blue = (inColor bitand 0x00ff0000) >> 16;
    alpha = inColor bitand 0xff000000;

    nvgColor = (red + green + blue) / 3;
    nvgColor = (nvgColor << 8) bitor alpha;
    return nvgColor;
}

// JPO
// used to set all the lights buttons and switches to their corerct states.
void CockpitManager::InitialiseInstruments(void)
{
    // actually for now - its jsut buttons that need to be up to date.
    for (int i = 0; i < mButtonTally; i++)
    {
        mpButtonObjects[i]->UpdateStatus();
    }
}


//Wombat778 4-14-04 New function that can bypasses the resource manager.
int FileExists(char *file)
{
    FILE *fp;
    int retval = false;


    if (g_bResizeUsesResMgr)
        return ResExistFile(file);

#undef fopen
#undef fclose

    fp = fopen(file, "r");

    if (fp)
    {
        fclose(fp);
        retval = true;
    }

#define fopen       ResFOpen
#define fclose      ResFClose

    return retval;
}





//Wombat778 10-12-2003
// This function looks for the highest cockpit resolution available for a given aircraft, and then defaults to
//the highest cockpit resolution available for the f16.
// The three pCPFile inputs are the cockpit dat files in order of priority.
// The return value is 1, 2 or 3 corresponding to which file was best
//Wombat778 4-14-04  changed all the ResExistFile to FileExists which can bypass the resource manager.
//sfr: rewrote this function. Added trimming, took night switcher out
int FindCockpit(
    const char *pCPFile,
    Vis_Types eCPVisType,
    const TCHAR* eCPName,
    const TCHAR* eCPNameNCTR,
    TCHAR *strCPFile,
    int fallback
)
{

    // FO sfr: plane number is no more
    // try plane number
    // FRB - Make cockpits switchable with theater
    sprintf(cockpitFolder, "%s", FalconCockpitThrDirectory);
    sprintf(strCPFile, "%s\\%d\\%s", cockpitFolder, MapVisId(eCPVisType), pCPFile);

    if (FileExists(strCPFile))
    {
        return (eCPVisType);
    }

    // try plane name (VCD name from DB, skips slashes and trim)
    if (strlen(eCPName) > 0)
    {
        std::string name = RemoveInvalidChars(string(eCPName, 15));
        // RV - Biker - Make cockpits switchable with theater
        sprintf(cockpitFolder, "%s", FalconCockpitThrDirectory);
        sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder, name.c_str(), pCPFile);

        if (FileExists(strCPFile))
        {
            return (eCPVisType);
        }
    }

    // try NCTR if it didnt work (skips slashes and trim)
    if (strlen(eCPNameNCTR) > 0)
    {
        std::string nameNCTR = RemoveInvalidChars(string(eCPNameNCTR, 5));
        // RV - Biker - Make cockpits switchable with theater
        sprintf(cockpitFolder, "%s", FalconCockpitThrDirectory);
        sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder, nameNCTR.c_str(), pCPFile);

        if (FileExists(strCPFile))
        {
            return (eCPVisType);
        }
    }

    // FRB - try the theater standard cockpit
    if (fallback)
    {
        // FRB - Make cockpits switchable with theater
        sprintf(cockpitFolder, "%s", FalconCockpitThrDirectory);
        sprintf(strCPFile, "%s\\%s", cockpitFolder, pCPFile);

        if (FileExists(strCPFile))
        {
            return (eCPVisType);
        }
    }

    // RV - Biker - For fallback use standard cockpit directory
    if (fallback)
    {
        sprintf(cockpitFolder, "%s", COCKPIT_DIR);
        sprintf(strCPFile, "%s%s", cockpitFolder, pCPFile);
        return MapVisId(VIS_F16C);
    }
    else
        return 0;
}

int FindCockpitResolution(
    const char *pCPFile1,
    const char *pCPFile2,
    const char *pCPFile3,
    const char *pCPFile4,
    const char *pCPFile5,
    Vis_Types eCPVisType,
    const TCHAR* eCPNameOrig,
    const TCHAR* eCPNameNCTROrig
)
{
    // RV - Biker - This does not work always
    //const char *eCPName = RemoveInvalidChars(string(eCPNameOrig, 15)).c_str();
    //const char *eCPNameNCTR = RemoveInvalidChars(string(eCPNameNCTROrig, 5)).c_str();
    std::string tmp_eCPName     = RemoveInvalidChars(string(eCPNameOrig, 15));
    std::string tmp_eCPNameNCTR = RemoveInvalidChars(string(eCPNameNCTROrig, 5));

    const char *eCPName = tmp_eCPName.c_str();
    const char *eCPNameNCTR = tmp_eCPNameNCTR.c_str();

    TCHAR strCPFile[MAX_PATH];

    // sprintf(strCPFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(eCPVisType), pCPFile1);
    // if(FileExists(strCPFile)) return 1;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile1);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile1);

    if (FileExists(strCPFile)) return 1;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile1);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile1);

    if (FileExists(strCPFile)) return 1;

    // sprintf(strCPFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(eCPVisType), pCPFile2);
    // if(FileExists(strCPFile)) return 2;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile2);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile2);

    if (FileExists(strCPFile)) return 2;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile2);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile2);

    if (FileExists(strCPFile)) return 2;

    // sprintf(strCPFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(eCPVisType), pCPFile3);
    // if(FileExists(strCPFile)) return 3;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile3);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile3);

    if (FileExists(strCPFile)) return 3;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile3);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile3);

    if (FileExists(strCPFile)) return 3;

    //Wombat778 4-15-04  Added 4-5

    // sprintf(strCPFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(eCPVisType), pCPFile4);
    // if(FileExists(strCPFile)) return 4;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile4);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile4);

    if (FileExists(strCPFile)) return 4;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile4);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile4);

    if (FileExists(strCPFile)) return 4;

    // sprintf(strCPFile, "%s%d\\%s", cockpitFolder /*COCKPIT_DIR*/, MapVisId(eCPVisType), pCPFile5);
    // if(FileExists(strCPFile)) return 5;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile5);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPName, pCPFile5);

    if (FileExists(strCPFile)) return 5;

    //sprintf(strCPFile, "%s%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile5);
    sprintf(strCPFile, "%s\\%s\\%s", cockpitFolder /*COCKPIT_DIR*/, eCPNameNCTR, pCPFile5);

    if (FileExists(strCPFile)) return 5;

    //We have gotten here, no custom cockpit available, so do f-16

    //sprintf(strCPFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile1);
    sprintf(strCPFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile1);

    if (FileExists(strCPFile)) return 1;

    //sprintf(strCPFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile2); //Wombat778 10-14-2003  Fixed stupid typo which caused 1600 pit not to scale down (pCPFile1 was set instead of pCPFile2)
    sprintf(strCPFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile2);

    if (FileExists(strCPFile)) return 2;

    //sprintf(strCPFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile3); //Wombat778 4-03-04
    sprintf(strCPFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile3);

    if (FileExists(strCPFile)) return 3;

    //sprintf(strCPFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile4); //Wombat778 4-15-04
    sprintf(strCPFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile4);

    if (FileExists(strCPFile)) return 4;

    //sprintf(strCPFile, "%s%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile5); //Wombat778 4-15-04
    sprintf(strCPFile, "%s\\%s", cockpitFolder /*COCKPIT_DIR*/, pCPFile5);

    if (FileExists(strCPFile)) return 5;

    return 0;


}


//Wombat778 3-30-04 Copies of the pixeltopixel imagebuffer functions but designed to work with our templateinfo struct.
// The point of this is to be able to use the existing settod function without the huge template imagebuffer actually being in memory

WORD TemplateInfoClass::Pixel32toPixel16(UInt32 ABGR)
{
    UInt32 color;

    // RED
    if (redShift >= 0)
    {
        color = (ABGR >>  redShift) bitand dwRBitMask;
    }
    else
    {
        color = (ABGR << -redShift) bitand dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq (ABGR >>  greenShift) bitand dwGBitMask;
    }
    else
    {
        color or_eq (ABGR << -greenShift) bitand dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq (ABGR >>  blueShift) bitand dwBBitMask;
    }
    else
    {
        color or_eq (ABGR << -blueShift) bitand dwBBitMask;
    }

    return (WORD)color;
}

DWORD TemplateInfoClass::Pixel32toPixel32(UInt32 ABGR)
{
    UInt32 color;

    // RED
    if (redShift >= 0)
    {
        color = (ABGR >>  redShift) bitand dwRBitMask;
    }
    else
    {
        color = (ABGR << -redShift) bitand dwRBitMask;
    }

    // GREEN
    if (greenShift >= 0)
    {
        color or_eq (ABGR >>  greenShift) bitand dwGBitMask;
    }
    else
    {
        color or_eq (ABGR << -greenShift) bitand dwGBitMask;
    }

    // BLUE
    if (blueShift >= 0)
    {
        color or_eq (ABGR >>  blueShift) bitand dwBBitMask;
    }
    else
    {
        color or_eq (ABGR << -blueShift) bitand dwBBitMask;
    }

    color or_eq ABGR bitand 0xff000000;

    return color;
}


//Wombat778 11-15-04 added for new method of 2d pit trackir movement based on the pantilt values of a panel

int CockpitManager::GetPanelNum(int mIdNum)
{
    int numpanels = GetNumPanels();

    for (int i = 0; i < numpanels; i++)
        if (GetPanel(i)->mIdNum == mIdNum)
            return i;

    return -1;
}


//Wombat778 11-16-04 Function to find the best panel based on where you are looking.  Uses a pathfinding approach
//  so that only "compatible" panels are chosen.  Ie. so that when you are in wideview, you only
//  get wideview panels.  The code first moves vertically from the current panel until the best
//  tilt value is found, then moves horizontally until the best pan value is found.

int CockpitManager::Set2DPanelDirection(float pan, float tilt)
{

    CPPanel *currentpanel;

    if (mpActivePanel) //Wombat778 11-17-04 added for security against changing views
        currentpanel = mpActivePanel;
    else if (mpNextActivePanel)
        currentpanel = mpNextActivePanel;
    else
        return false; //this is a bad place to be in because we have no starting panels.

    int newpanel = currentpanel->mIdNum;
    int newpanelnum = GetPanelNum(newpanel);
    int lastpanel = newpanel;
    int lastpanelnum = newpanelnum;

    bool bestpan = false;
    bool besttilt = false;

    while ( not besttilt)
    {
        if (tilt > GetPanel(lastpanelnum)->mTilt)
            newpanel = GetPanel(lastpanelnum)->mAdjacentPanels.S;
        else if (tilt < GetPanel(lastpanelnum)->mTilt)
            newpanel = GetPanel(lastpanelnum)->mAdjacentPanels.N;
        else besttilt = true;

        if (newpanel >= 0)
            newpanelnum = GetPanelNum(newpanel);
        else
        {
            newpanel = lastpanel;
            besttilt = true;
        }

        if (fabs(GetPanel(newpanelnum)->mTilt - tilt) >= fabs(GetPanel(lastpanelnum)->mTilt - tilt))
        {
            newpanel = lastpanel;
            newpanelnum = lastpanelnum;
            besttilt = true;
        }

        lastpanel = newpanel;
        lastpanelnum = newpanelnum;
    }

    while ( not bestpan)
    {

        if (pan > GetPanel(lastpanelnum)->mPan)
            newpanel = GetPanel(lastpanelnum)->mAdjacentPanels.E;
        else if (pan < GetPanel(lastpanelnum)->mPan)
            newpanel = GetPanel(lastpanelnum)->mAdjacentPanels.W;
        else bestpan = true;

        if (newpanel >= 0)
            newpanelnum = GetPanelNum(newpanel);
        else
        {
            newpanel = lastpanel;
            bestpan = true;
        }

        if (fabs(GetPanel(newpanelnum)->mPan - pan) >= fabs(GetPanel(lastpanelnum)->mPan - pan))
        {
            newpanel = lastpanel;
            newpanelnum = lastpanelnum;
            bestpan = true;
        }

        lastpanel = newpanel;
        lastpanelnum = newpanelnum;
    }

    if (newpanel not_eq currentpanel->mIdNum)
    {
        SetActivePanel(newpanel);
        return true;
    }

    return false;
}



// Cobra - RED - Setup the 2D turbulence for the pit
void CockpitManager::SetTurbulence(void)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    PitTurbulence = playerAC->GetTurbulence();
    // Scale to pixels
    PitTurbulence.y = PitTurbulence.z * DisplayOptions.DispHeight;
    PitTurbulence.x = PitTurbulence.x * DisplayOptions.DispWidth;
    // Being 2D Pit with no depth, differently from 3D Pith, shaking may be too much violent
    // so, we r going to apply a Square mantaining to shake values sign to it and then multiply by 2
    PitTurbulence.y = (float)((int)(PitTurbulence.y * 2.0f / sqrtf(PitTurbulence.y)));
    PitTurbulence.x = (float)((int)(PitTurbulence.x * 2.0f / sqrtf(PitTurbulence.x)));
}


// COBAR - RED -This function adds a turbulence pixel offset to a Vertex[4]
void CockpitManager::AddTurbulence(TwoDVertex *pVtx)
{
    for (DWORD a = 0; a < 4; a++)
    {
        pVtx[a].x += PitTurbulence.x;
        pVtx[a].y += PitTurbulence.y;
    };
}

// Cobra - RED - Apply the 2D turbulence to a view port
void CockpitManager::AddTurbulenceVp(ViewportBounds *Vp)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    Tpoint Turbulence = playerAC->GetTurbulence();
    // to avoid shift of viewports coming from decimals, use it only if integer offsets available
    float OffsetX = PitTurbulence.x * 2.0f / DisplayOptions.DispWidth;
    float OffsetY = -PitTurbulence.y * 2.0f / DisplayOptions.DispHeight;
    Vp->left += OffsetX;
    Vp->right += OffsetX;
    Vp->top += OffsetY;
    Vp->bottom += OffsetY;
}
