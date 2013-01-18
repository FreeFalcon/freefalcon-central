#ifndef _COCKPITMAN_H
#define _COCKPITMAN_H
#include <stdio.h>

#include "stdhdr.h"
#include "otwdrive.h"
#include "Graphics/Include/imagebuf.h"
#include "Graphics/Include/Device.h"
#include "Render2d.h"

#include "cpsurface.h"
#include "cppanel.h"
#include "cpcursor.h"
#include "cpmisc.h"
#include "dispopts.h"
#include "flightdata.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif

#define _ALWAYS_DIRTY 1

// sfr: this will allow changes without losing compatibility
#define CPMANAGER_VERSION 1

#include <vector>

#include "classtbl.h"

//====================================================//
// Default view
//====================================================//

#define COCKPIT_DEFAULT_PANEL 1100
#define PADLOCK_DEFAULT_PANEL 100

//====================================================//
// Special Defines for Special Devices
//====================================================//

#define ICP_INITIAL_SECONDARY_BUTTON 1018
#define ICP_INITIAL_PRIMARY_BUTTON 1013
#define ICP_INITIAL_TERTIARY_BUTTON 1011

//====================================================//
// Mouse and Keyboard Events
//====================================================//

#define CP_MOUSE_MOVE 0
#define CP_MOUSE_DOWN 1
#define CP_MOUSE_UP 2
#define CP_MOUSE_BUTTON0 3
#define CP_MOUSE_BUTTON1 4
#define CP_CHECK_EVENT -1 // fake event, used to query status.

//====================================================//
// BLIT Properties
//====================================================//

#define CPOPAQUE 0
#define CPTRANSPARENT 1

//====================================================//
// Cycle Control Words
//====================================================//

#define BEGIN_CYCLE 0x0001
#define END_CYCLE 0x8000
#define CYCLE_ALL 0xFFFF

//====================================================//
// Persistance Types
//====================================================//

#define NONPERSISTANT 0
#define PERSISTANT 1

//====================================================//
// Location of Cockpit Data and Images
//====================================================//

#define COCKPIT_FILE_6x4 "6_ckpit.dat"
#define COCKPIT_FILE_8x6 "8_ckpit.dat"
#define COCKPIT_FILE_10x7 "10_ckpit.dat"
// OW new
#define COCKPIT_FILE_12x9 "12_ckpit.dat"
#define COCKPIT_FILE_16x12 "16_ckpit.dat"

#define PADLOCK_FILE_6x4 "6_plock.dat"
#define PADLOCK_FILE_8x6 "8_plock.dat"
#define PADLOCK_FILE_10x7 "10_plock.dat"
// OW new
#define PADLOCK_FILE_12x9 "12_plock.dat"
#define PADLOCK_FILE_16x12 "16_plock.dat"

#define COCKPIT_DIR "art\\ckptart\\"

//====================================================//
// Miscellaneous Defines
//====================================================//
#define PANELS_INACTIVE -1
#define NO_ADJ_PANEL -1
#define PADLOCK_PANEL 100

//====================================================//
// Defines for Parsing up Data File
//====================================================//

#define MAX_LINE_BUFFER 512

#define TYPE_MANAGER_STR "MANAGER"
#define TYPE_SURFACE_STR "SURFACE"
#define TYPE_PANEL_STR "PANEL"
#define TYPE_LIGHT_STR "LIGHT"
#define TYPE_BUTTON_STR "BUTTON"
#define TYPE_BUTTONVIEW_STR "BUTTONVIEW"
#define TYPE_MULTI_STR "MULTI"
#define TYPE_INDICATOR_STR "INDICATOR"
#define TYPE_DIAL_STR "DIAL"
#define TYPE_CURSOR_STR "CURSOR"
#define TYPE_DED_STR "DED"
#define TYPE_ADI_STR "ADI"
#define TYPE_MACHASI_STR "MACHASI"
#define TYPE_HSI_STR "HSI"
#define TYPE_DIGITS_STR "DIGITS"
#define TYPE_BUFFER_STR "BUFFER"
#define TYPE_SOUND_STR "SOUND"
#define TYPE_CHEVRON_STR "CHEVRON"
#define TYPE_LIFTLINE_STR "LIFTLINE"
#define TYPE_TEXT_STR "TEXT"
#define TYPE_KNEEVIEW_STR "KNEEVIEW"
#define TYPE_MIRROR_STR "MIRROR"

#define PROP_VERSION_STR "version"
#define PROP_HUDCOLOR_STR "hudcolor"
#define PROP_ORIENTATION_STR "orientation"
#define PROP_PARENTBUTTON_STR "parentbutton"
#define PROP_HORIZONTAL_STR "horizontal"
#define PROP_VERTICAL_STR "vertical"
#define PROP_NUMPANELS_STR "numpanels"
#define PROP_MOUSEBORDER_STR "mouseborder"
#define PROP_NUMOBJECTS_STR "numobjects"
#define PROP_TEMPLATEFILE_STR "templatefile"
#define PROP_FILENAME_STR "filename"
#define PROP_SRCLOC_STR "srcloc"
#define PROP_DESTLOC_STR "destloc"
#define PROP_NUMSURFACES_STR "numsurfaces"
#define PROP_NUMCURSORS_STR "numcursors"
#define PROP_SURFACES_STR "surfaces"
#define PROP_NUMOBJECTS_STR "numobjects"
#define PROP_NUMBUTTONS_STR "numbuttons"
#define PROP_NUMBUTTONVIEWS_STR "numbuttonviews"
#define PROP_OBJECTS_STR "objects"
#define PROP_TRANSPARENT_STR "transparent"
#define PROP_OPAQUE_STR "opaque"
#define PROP_STATES_STR "states"
#define PROP_CALLBACKSLOT_STR "callbackslot"
#define PROP_CURSORID_STR "cursorid"
#define PROP_HOTSPOT_STR "hotspot"
#define PROP_BUTTONS_STR "buttons"
#define PROP_MINVAL_STR "minval"
#define PROP_MAXVAL_STR "maxval"
#define PROP_MINPOS_STR "minpos"
#define PROP_MAXPOS_STR "maxpos"
#define PROP_NUMTAPES_STR "numtapes"
#define PROP_RADIUS0_STR "radius0"
#define PROP_RADIUS1_STR "radius1"
#define PROP_RADIUS2_STR "radius2"
#define PROP_NUMENDPOINTS_STR "numendpoints"
#define PROP_POINTS_STR "points"
#define PROP_VALUES_STR "values"
#define PROP_INITSTATE_STR "initstate"
#define PROP_TYPE_STR "type"
#define PROP_MOMENTARY_STR "momentary"
#define PROP_EXCLUSIVE_STR "exclusive"
#define PROP_TOGGLE_STR "toggle"
#define PROP_INNERARC_STR "innerarc"
#define PROP_OUTERARC_STR "outerarc"
#define PROP_INNERSRCARC_STR "innersrcarc"
#define PROP_OFFSET_STR "offset"
#define PROP_PANTILT_STR "pantilt"
#define PROP_MASKTOP_STR "masktop"
#define PROP_MOUSEBOUNDS_STR "mousebounds"
#define PROP_ADJPANELS_STR "adjpanels"
#define PROP_CYCLEBITS_STR "cyclebits"
#define PROP_PERSISTANT_STR "persistant"
#define PROP_BSURFACE_STR "bsurface"
#define PROP_BSRCLOC_STR "bsrcloc"
#define PROP_BDESTLOC_STR "bdestloc"
#define PROP_COLOR0_STR "color0"
#define PROP_COLOR1_STR "color1"
#define PROP_COLOR2_STR "color2"
#define PROP_COLOR3_STR "color3"
#define PROP_COLOR4_STR "color4"
#define PROP_COLOR5_STR "color5"
#define PROP_COLOR6_STR "color6"
#define PROP_COLOR7_STR "color7"
#define PROP_COLOR8_STR "color8"
#define PROP_COLOR9_STR "color9"
#define PROP_HUD_STR "hud"
#define PROP_MFDLEFT_STR "mfdleft"
#define PROP_MFDRIGHT_STR "mfdright"
#define PROP_MFD3_STR "mfd3" //Wombat778 4-12-04
#define PROP_MFD4_STR "mfd4" //Wombat778 4-12-04
#define PROP_OSBLEFT_STR "osbleft"
#define PROP_OSBRIGHT_STR "osbright"
#define PROP_OSB3_STR "osb3" //Wombat778 4-12-04
#define PROP_OSB4_STR "osb4" //Wombat778 4-12-04
#define PROP_RWR_STR "rwr"
#define PROP_NUMDIGITS_STR "numdigits"
#define PROP_BUTTONVIEWS_STR  "buttonviews"
#define PROP_DELAY_STR "delay"
#define PROP_NUMSOUNDS_STR "numsounds"
#define PROP_ENTRY_STR "entry"
#define PROP_SOUND1_STR "sound1"
#define PROP_SOUND2_STR "sound2"
#define PROP_CALIBRATIONVAL_STR "calibrationval"
#define PROP_ILSLIMITS_STR "ilslimits"
#define PROP_BUFFERSIZE_STR "buffersize"
#define PROP_DOGEOMETRY_STR "dogeometry"
#define PROP_NEEDLERADIUS_STR "needleradius"
#define PROP_STARTANGLE_STR "startangle"
#define PROP_ARCLENGTH_STR "arclength"
#define PROP_MAXMACHVALUE_STR "maxmachvalue"
#define PROP_MINMACHVALUE_STR "minmachvalue"

#define PROP_LIFTCENTERS_STR  "liftcenters"
#define PROP_NUMSTRINGS_STR  "numstrings"
#define PROP_PANTILTLABEL_STR  "pantiltlabel"
#define PROP_BLITBACKGROUND_STR "blitbackground"
#define PROP_BACKDEST_STR "backdest"
#define PROP_BACKSRC_STR "backsrc"
#define PROP_WARNFLAG_STR "warnflag"
#define PROP_ENDLENGTH        "endlength"
#define PROP_ENDANGLE        "endangle"
#define PROP_HUDFONT        "hudfont"
#define PROP_MFDFONT        "mfdfont"
#define PROP_DEDFONT        "dedfont"
#define PROP_GENFONT        "generalfont"
#define PROP_POPFONT        "popupfont"
#define PROP_KNEEFONT       "kneefont"
#define PROP_SAFONT  "saboxfont"
#define PROP_LABELFONT  "labelfont"
#define END_MARKER  "#end"
#define PROP_DED_TYPE     "dedtype"
#define PROP_DED_DED     "ded"
#define PROP_DED_PFL     "pfl"
#define PROP_DO2DPIT_STR    "cockpit2d"
#define PROP_LIFT_LINE_COLOR "liftlinecolor"
#define PROP_RENDER_NEEDLE  "renderneedle" //Wombat778 3-24-04
#define PROP_ALTPANEL  "altpanel" //Wombat778 4-12-04
#define PROP_FLOODLIGHT "floodlight" //sfr
#define PROP_INSTLIGHT "instLight" //sfr


// special 3d cockpit keys
#define PROP_3D_USE_NEW_3DPIT "usenew3dpit" // Cobra
#define PROP_3D_RTTTARGET "rtttarget" // ASSO:
#define PROP_3D_BORESIGHT_Y "boresighty" // ASSO:
#define PROP_3D_PADBACKGROUND "padlockbg"
#define PROP_3D_PADLIFTLINE "padlockliftline"
#define PROP_3D_PADBOXSIDE "padlockvpside"
#define PROP_3D_PADBOXTOP "padlockvptop"
#define PROP_3D_PADTICK "padlocktick"
#define PROP_3D_NEEDLE0 "needlecolor0"
#define PROP_3D_NEEDLE1  "needlecolor1"
#define PROP_3D_DED "dedcolor"
#define PROP_3D_RWR "rwrcolor" // Cobra
#define PROP_3D_HILIGHT "highlight" // Cobra
#define PROP_3D_LOLIGHT "lowlight" // Cobra
#define PROP_3D_COCKPIT "cockpitmodel"
#define PROP_3D_COCKPITDF "cockpitdfmodel"
#define PROP_3D_MAINMODEL "mainmodel"
#define PROP_3D_DAMAGEDMODEL "damagedmodel"

//JAM 10May04
#define PROP_3D_ZBUFFERING "zbuffering"

extern OTWDriverClass OTWDriver;

//====================================================//
// Miscellaneous Utility Functions
//====================================================//

int Compare(const void *, const void *);
char* FindToken(char**lineptr, const char*separators);
void CreateCockpitGeometry(DrawableBSP**, int, int);
void ReadImage(char*, GLubyte**, GLulong**);
void Translate8to16(WORD *, BYTE *, ImageBuffer *);
void Translate8to32(DWORD *, BYTE *, ImageBuffer *);  // OW
/** sfr: added these 2 functions for night lighting */
/** sfr: generates a new palette, given the light factors(RGB) */
void ApplyLightingToPalette(DWORD *in, DWORD *out, float rf, float gf, float bf);
/** sfr: Calculates a color given the light factors */
DWORD CalculateColor(DWORD inColor, float rf, float gf, float bf);
/** sfr: calculates the NVG color of the color, mean of 3 colors shifted to green */
DWORD CalculateNVGColor(DWORD inColor);
/** sfr: inverts RGB components in a color, used by kneemap */
DWORD InvertRGBOrder(DWORD inColor);
// RV - Biker - Define this function here
int FileExists(char *file);
// RV - Biker - Add variable to interface for activating fallback
//int FindCockpit(const char *pCPFile, Vis_Types eCPVisType, const TCHAR* eCPName, const TCHAR* eCPNameNCTR, TCHAR *strCPFile);
int FindCockpit(const char *pCPFile, Vis_Types eCPVisType, const TCHAR* eCPName, const TCHAR* eCPNameNCTR, TCHAR *strCPFile, int fallbackEnable);
int FindCockpitResolution(const char *pCPFile, const char *pCPFile2, const char *pCPFile3, const char *pCPFile4, const char *pCPFile5, Vis_Types eCPVisType, const TCHAR* eCPName, const TCHAR* eCPNameNCTR); //Wombat778 10-12-2003 ;


//====================================================//
// Forward Declarations of Cockpit Objects
// and Outside World
//====================================================//

class OTWDriverClass;
class AircraftClass;

class CPObject;
class CPLight;
class CPDial;
class CPGauge;
class CPButtonObject;
class CPButtonView;
class CPSurface;
class CPPanel;
class CPMulit;
class CPMisc;
class ICPClass;
class KneeBoard;
class CPDed;
class CPHsi;
class CPSoundList;


//Wombat778 3-30-04 class to hold template information
class TemplateInfoClass
{
public:
    int redShift;
    UInt32 dwRBitMask;
    int greenShift;
    UInt32 dwGBitMask;
    int blueShift;
    UInt32 dwBBitMask;
    int pixelsize;
    WORD Pixel32toPixel16(UInt32 ABGR);
    DWORD Pixel32toPixel32(UInt32 ABGR);
};

void Translate8to16(WORD *, BYTE *, ImageBuffer *);

//====================================================//
// CockpitManager Class Definition
//====================================================//
extern BOOL gDoCockpitHack;
#define DO_HIRESCOCK_HACK 1

class CockpitManager
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap pool
    void *operator new(size_t size)
    {
        return MemAllocPtr(gCockMemPool, size, FALSE);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreePtr(mem);
    };
#endif
private:

#if CPMANAGER_VERSION
    struct VersionStruct
    {
        unsigned int major;
        unsigned int minor;
    } mVersion;
    const static struct VersionStruct CURRENT_MANAGER_VERSION;
#endif

    //====================================================//
    // Temporary pointers, used mainly during initialization
    // and cleanup
    //====================================================//

    int mLoadBufferHeight;
    int mLoadBufferWidth;
    int mSurfaceTally;
    int mPanelTally;
    int mObjectTally;
    int mCursorTally;
    int mButtonTally;
    int mButtonViewTally;

    int mNumSurfaces;
    int mNumPanels;
    int mNumObjects;
    int mNumCursors;
    int mNumButtons;
    int mNumButtonViews;
    int mHudFont;
    int mMFDFont;
    int mDEDFont;
    int mGeneralFont;
    int mPopUpFont;
    int mKneeFont;
    int mSABoxFont;
    int mLabelFont;
    int mAltPanel; //Wombat778 4-13-04


    std::vector< CPSurface* > mpSurfaces;
    std::vector< CPPanel* > mpPanels;
    std::vector< CPObject* > mpObjects;
    std::vector< CPCursor* > mpCursors;
    std::vector< CPButtonObject* > mpButtonObjects;
    std::vector< CPButtonView* > mpButtonViews;

    //====================================================//
    // Internal Pointers, Used Mainly for Runtime
    //====================================================//

    CPPanel *mpActivePanel;
    CPPanel *mpNextActivePanel;

    Vis_Types m_eCPVisType; // OW
    _TCHAR m_eCPName[15]; // JB 010711 Name of aircraft for cockpit loading
    _TCHAR m_eCPNameNCTR[5]; // JB 010808 Alternate name of aircraft for cockpit loading

    //====================================================//
    // Control Variables
    //====================================================//

    BOOL mIsInitialized;
    BOOL mIsNextInitialized;
    RECT mMouseBounds;
    int mCycleBit;

    //====================================================//
    // Pointer to buffer for Texture Loading
    //====================================================//

    GLubyte *mpLoadBuffer;

    //====================================================//
    // Initialization Member Functions
    //====================================================//

    void CreateText(int, FILE*);
    void CreateChevron(int, FILE*);
    void CreateLiftLine(int, FILE*);
    void CreateSurface(int, FILE*);
    void CreatePanel(int, FILE*);
    void CreateSwitch(int, FILE*);
    void CreateLight(int, FILE*);
    void CreateButton(int, FILE*);
    void CreateButtonView(int, FILE*);
    void CreateIndicator(int, FILE*);
    void CreateDial(int, FILE*);
    void CreateMulti(int, FILE*);
    void CreateCursor(int, FILE*);
    void CreateDed(int, FILE*);
    void CreateAdi(int, FILE*);
    void CreateMachAsi(int, FILE*);
    void CreateHsiView(int, FILE*);
    void CreateDigits(int, FILE*);
    void CreateSound(int, FILE*);
    void CreateKneeView(int, FILE*);
    void CreateMirror(int, FILE*);
    void LoadBuffer(FILE*);
    void SetupControlTemplate(char*, int, int);
    void ParseManagerInfo(FILE*);
    void ResolveReferences(void);
    static void TimeUpdateCallback(void *self);

public:
    //====================================================//
    // Dimensions specific to the Cockpit
    //====================================================//

    int mTemplateWidth;
    int mTemplateHeight;
    int mMouseBorder;
    //float mScale; // for stretched cockpits //Wombat778 10-06-2003 Changes mScale from int to float
    // sfr separated scales
    float           mHScale;
    float           mVScale;

    //====================================================//
    // Lighting Stuff
    //====================================================//
    // sfr: this 2 lights here are divided into RGB components 1.0 is full brightness, 0.0 is complete darkness
    // made this public so instruments can see it and be drawn correctly
    float mFloodLight[3];
    float mInstLight[3];
    float lightLevel;
    //bool inNVGmode; //sfr: changed this to C++ bool

    //====================================================//
    // Miscellaneous State Information
    //====================================================//
    CPMisc mMiscStates;
    CPSoundList *mpSoundList;
    BOOL mViewChanging;
    F4CSECTIONHANDLE* mpCockpitCritSec;

    //====================================================//
    // Pointer to Device Viewport Boundaries
    //====================================================//

    ViewportBounds *mpViewBounds[BOUNDS_TOTAL];


    //====================================================//
    // Pointers to special avionics devices
    //====================================================//

    ICPClass *mpIcp;
    CPHsi *mpHsi;
    DrawableBSP *mpGeometry; // Pointer to wings and reflections
    KneeBoard *mpKneeBoard;

    float ADIGpDevReading;
    float ADIGsDevReading;
    BOOL mHiddenFlag;

    //====================================================//
    // Pointers to the Outside World
    //====================================================//

    ImageBuffer *mpOTWImage;
    SimBaseClass *mpOwnship;

    ImageBuffer *RatioBuffer;  //Wombat778 10-18-2003 hack for 1.25 ratio screens

    //====================================================//
    // Public Constructors and Destructions
    //====================================================//

    ~CockpitManager();

    // sfr added 2 scaling factors here
#if DO_HIRESCOCK_HACK
    CockpitManager(ImageBuffer*, char*, BOOL, float, float, BOOL, Vis_Types eCPVisType = VIS_F16C, TCHAR* eCPName = NULL, TCHAR* eCPNameNCTR = NULL); //Wombat778 10-06-2003 changed scale from int to float
#else
    CockpitManager(ImageBuffer*, char*, BOOL, float, float); //Wombat778 10-06-2003 changed scale from int to float
#endif

    //====================================================//
    // Public Runtime Functions
    //====================================================//

    void Exec(void); // Called by main sim thread
    void DisplayBlit(void); // Called by display thread
    void DisplayDraw(void); // Called by display thread
    void GeometryDraw(void); // Called by display thread for drawing wings and reflections
    int Dispatch(int, int, int); // Called by input thread
    void Dispatch(int, int);
    int POVDispatch(int, int, int);

    // OW
    void DisplayBlit3D(void); // Called by display thread
    void InitialiseInstruments(void); // JPO - set switches and lights up correctly
    //inline void SetNVGMode(bool state){ inNVGmode = state; };
    //inline boolean GetNVGMode(){ return inNVGmode; };
    //====================================================//
    // Public Auxillary Functions
    //====================================================//

#if CPMANAGER_VERSION
    unsigned int GetMajorVersion() const
    {
        return mVersion.major;
    }
    unsigned int GetMinorVersion() const
    {
        return mVersion.minor;
    }
#endif
    CPButtonObject* GetButtonPointer(int);
    void BoundMouseCursor(int*, int*);
    bool SetActivePanel(int); //Wombat778 4-13-04 changed return to bool
    void SetDefaultPanel(int defaultPanel)
    {
        SetActivePanel(defaultPanel);
    }
    void SetOwnship(SimBaseClass*);
    BOOL ShowHud();
    BOOL ShowMfd();
    BOOL ShowRwr();
    CPPanel* GetActivePanel()
    {
        return mpActivePanel;
    }
    int GetCockpitWidth()
    {
        return DisplayOptions.DispWidth;
    };
    int GetCockpitHeight()
    {
        return DisplayOptions.DispHeight;
    };
    BOOL GetViewportBounds(ViewportBounds*, int);
    float GetCockpitMaskTop(void);
    void SetNextView(void);
    float GetPan(void);
    float GetTilt(void);
    void SetTOD(float);
    void UpdatePalette(); //sfr: updates the 2d cockpit palette(due to inst and flood lights)
    void ImageCopy(GLubyte*, GLubyte*, int, RECT*);
    void SafeImageCopy(GLubyte*, GLubyte*, int, int, RECT*); //Wombat778 3-23-04
    void     LoadCockpitDefaults(void);
    void     SaveCockpitDefaults(void);
    int      HudFont(void);
    int      MFDFont(void);
    int      DEDFont(void);
    int      GeneralFont(void)
    {
        return mGeneralFont;
    };
    int      PopUpFont(void)
    {
        return mPopUpFont;
    };
    int      KneeFont(void)
    {
        return mKneeFont;
    };
    int      SABoxFont(void)
    {
        return mSABoxFont;
    };
    int      LabelFont(void)
    {
        return mLabelFont;
    };
    int      AltPanel(void)
    {
        return mAltPanel;
    }; //Wombat778 4-13-04

    /* sfr: light util, applies cockpit lighting to a color. Will consider NVG, enviroment and flood
    * If useInstrument is true, will also use insturment lighting
    */
    DWORD ApplyLighting(DWORD inColor, bool useInstrument);
    /** sfr: same as above, but uses RGB format for inColor. Kneeboard uses this */
    DWORD ApplyLightingToRGB(DWORD inColor, bool useInstrument);

    /** sfr: the function below computes cockpit lights, filling the values in the arguments
    * enviroment lighting, flood lighting and instrument lighting are considered
    * the parameters must have 3 positions each(RGB)
    */
    void ComputeLightFactors(float *cockpit, float *instrument);

    //Wombat778 3-12-04
    int GetNumPanels(void)
    {
        return mNumPanels;
    };
    CPPanel*  GetPanel(int num)
    {
        return mpPanels[num];
    }
    float GetLightLevel()
    {
        return lightLevel;
    }; //Wombat778 11-15-04
    int Set2DPanelDirection(float pan, float tilt); //Wombat778 11-16-04
    int GetPanelNum(int mIdNum); //Wombat778 11-16-04

    void CockAttachWeapons(void);
    void CockDetachWeapons(void);

    Tpoint PitTurbulence;
    void AddTurbulence(TwoDVertex *pVtx);
    void SetTurbulence(void);
    void AddTurbulenceVp(ViewportBounds *);
};

//====================================================//
// Pointers to template and cockpit palette
//====================================================//

extern ImageBuffer *gpTemplateSurface;
extern GLubyte *gpTemplateImage;
extern GLulong *gpTemplatePalette;
extern TemplateInfoClass *TemplateInfo; //Wombat778 3-30-04
extern FlightData cockpitFlightData;
#endif

