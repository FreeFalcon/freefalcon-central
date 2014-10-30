/***************************************************************************\
    Rwr.cpp

    This provides the basis for all radar dector sensors.  This is the
 class that receives track messages.  It also has some basic helper
 functions to be shared by two or more cousin classes.
\***************************************************************************/
#include "stdhdr.h"
#include "simfile.h"
#include "simmover.h"
#include "ClassTbl.h"
#include "simdrive.h"
#include "Entity.h"
#include "Graphics/Include/render2d.h"
#include "rwr.h"

const int RwrClass::GUIDANCE_CYCLE = 1500; //me123 from 500
const int RwrClass::TRACK_CYCLE = 3500;//me123 from 500
// 2001-02-17 MODIFIED BY S.G. SO 2D (AND DEAD) CONTACTS STAYS FOR 6 SECONDS LIKE SIM CONTACTS
//const int RwrClass::RADIATE_CYCLE = 10000;
const int RwrClass::RADIATE_CYCLE = 6000;
//const int RwrClass::RADIATE_CYCLE = 8000;//me123 from 10000

RwrDataType* RwrDataTable = NULL;
short NumRwrEntries;

//MI
extern bool g_bRealisticAvionics;
#include "aircrft.h"

RwrClass::RwrClass(int type, SimMoverClass* self) : SensorClass(self)
{
    // Store a pointer to our type data to save dereferences later
    typeData = &RwrDataTable[max(min(type, NumRwrEntries - 1), 0)];

    // LRKLUDGE for this data set
    if (typeData->nominalRange > 10.0F)
        typeData->nominalRange /= 40.0F * NM_TO_FT;

    // Initialize some base class data
    sensorType = RWR;
    dataProvided = LooseBearing;
}

RwrClass::~RwrClass(void)
{
}

void RwrClass::DisplayInit(ImageBuffer* image)
{
    DisplayExit();

    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(image);

    privateDisplay->SetColor(0xff00ff00);
}

void RwrClass::GetAGCenter(float* x, float* y)
{
    if (platform)
    {
        *x = platform->XPos();
        *y = platform->YPos();
    }
    else
    {
        *x = 0.0F;
        *y = 0.0F;
    }
}

void RwrClass::DrawEmitterSymbol(int symbolID, int boxed)
{
    DrawSymbol(display, symbolID, boxed);
}

void RwrClass::DrawSymbol(VirtualDisplay *display, int symbolID, int boxed)
{
    static int flash = false;
    // Do we draw flashing things this frame?
    flash = vuxRealTime bitand 0x200;
    static int show = 0;
    static bool oldcount = false;

    if (flash and ((oldcount and show not_eq 0) or ( not oldcount and show == 0)))
    {
        ++show;

        if (show > 3)
            show = oldcount = 0;
    }

    if ( not flash and ((oldcount and show == 0) or ( not oldcount and show not_eq 0)))
    {
        ++show;
        oldcount = show not_eq 0;
    }

    display->ZeroRotationAboutOrigin(); // me123 make sure we are not rotated

    static const float basicAir[][2] =
    {
        { 0.00F,  0.06F},
        { 0.09F, -0.03F},
        { 0.00F,  0.01F},
        { -0.09F, -0.03F},
        { 0.00F,  0.06F}
    };
    static const int NUM_BASIC_AIR_POINTS = sizeof(basicAir) / sizeof(basicAir[0]);

    static const float advAir[][2] =
    {
        { 0.00F,  0.09F},
        { 0.09F, -0.01F},
        { 0.03F,  0.01F},
        { 0.00F, -0.06F},
        { -0.03F,  0.01F},
        { -0.09F, -0.01F},
        { 0.00F,  0.09F}
    };
    static const int NUM_ADV_AIR_POINTS = sizeof(advAir) / sizeof(advAir[0]);

    static const float theBoat[][2] =
    {
        { -0.06F, -0.04F},
        { -0.08F,  0.00F},
        { -0.03F,  0.00F},
        { -0.03F,  0.04F},
        { 0.03F,  0.04F},
        { 0.03F,  0.00F},
        { 0.06F,  0.00F},
        { 0.06F, -0.04F},
        { -0.06F, -0.04F}
    };
    static const int NUM_BOAT_POINTS = sizeof(theBoat) / sizeof(theBoat[0]);

    int i;
    float verticalTextCenter = 0.5f * display->TextHeight();

    switch (symbolID)
    {
        case RWRSYM_ADVANCED_INTERCEPTOR://
            for (i = 1; i < NUM_ADV_AIR_POINTS; i++)
                display->Line(advAir[i][0], advAir[i][1], advAir[i - 1][0], advAir[i - 1][1]);

            break;

        case RWRSYM_BASIC_INTERCEPTOR:
            for (i = 1; i < NUM_BASIC_AIR_POINTS; i++)
                display->Line(basicAir[i][0], basicAir[i][1], basicAir[i - 1][0], basicAir[i - 1][1]);

            break;

        case RWRSYM_NAVAL:
            for (i = 1; i < NUM_BOAT_POINTS; i++)
                display->Line(theBoat[i][0], theBoat[i][1], theBoat[i - 1][0], theBoat[i - 1][1]);

            break;

        case RWRSYM_ACTIVE_MISSILE:
            display->TextCenter(0.0F, verticalTextCenter, "M", boxed);
            break;

        case RWRSYM_CHAPARAL:
        case RWRSYM_KSAM:
            display->TextCenter(0.0F, verticalTextCenter, "C", boxed);
            break;

        case RWRSYM_HAWK:
            display->TextCenter(0.0F, verticalTextCenter, "H", boxed);
            break;

        case RWRSYM_PATRIOT:
            display->TextCenter(0.0F, verticalTextCenter, "P", boxed);
            break;

        case RWRSYM_NIKE:
            display->TextCenter(0.0F, verticalTextCenter, "N", boxed);
            break;

        case RWRSYM_SA2:

            display->TextCenter(0.0F, verticalTextCenter, "2", boxed);
            break;

        case RWRSYM_SA3:

            display->TextCenter(0.0F, verticalTextCenter, "3", boxed);
            break;

        case RWRSYM_SA4:

            display->TextCenter(0.0F, verticalTextCenter, "4", boxed);
            break;

        case RWRSYM_SA5:
            display->TextCenter(0.0F, verticalTextCenter, "5", boxed);
            break;

        case RWRSYM_SA6:

            display->TextCenter(0.0F, verticalTextCenter, "6", boxed);
            break;

        case RWRSYM_SA8:
            display->TextCenter(0.0F, verticalTextCenter, "8", boxed);
            break;

        case RWRSYM_SA9:
            display->TextCenter(0.0F, verticalTextCenter, "9", boxed);
            break;

        case RWRSYM_SA10:
            display->TextCenter(0.0F, verticalTextCenter, "10", boxed);
            break;

        case RWRSYM_SA13:
            display->TextCenter(0.0F, verticalTextCenter, "13", boxed);
            break;

        case RWRSYM_SA15:
            if (show)
                display->TextCenter(0.0F, verticalTextCenter, "15", boxed);
            else
                display->TextCenter(0.0F, verticalTextCenter, "M", boxed);

            break;

        case RWRSYM_AAA:
            if (flash)
                display->TextCenter(0.0F, verticalTextCenter, "A", boxed);
            else
                display->TextCenter(0.0F, verticalTextCenter, "S", boxed);

            break;

        case RWRSYM_SEARCH:
            display->TextCenter(0.0F, verticalTextCenter, "S", boxed);
            break;

        case RWRSYM_UNK1:
            display->TextCenter(0.0F, verticalTextCenter, "U", boxed);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, ".", boxed);
            break;

        case RWRSYM_UNK2:
            display->TextCenter(0.0F, verticalTextCenter, "U", boxed);
            display->TextCenter(0.02F, verticalTextCenter - 0.04F, ".", boxed);
            display->TextCenter(-0.02F, verticalTextCenter - 0.04F, ".", boxed);
            break;

        case RWRSYM_UNK3:
            display->TextCenter(0.0F, verticalTextCenter, "U", boxed);
            display->TextCenter(-0.03F, verticalTextCenter - 0.04F, ".", boxed);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, ".", boxed);
            display->TextCenter(0.03F, verticalTextCenter - 0.04F, ".", boxed);
            break;

        case RWRSYM_PDOT :
            display->TextCenter(0.0F, verticalTextCenter, "P", boxed);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, ".", boxed);
            break;

        case RWRSYM_PSLASH:
            display->TextCenter(0.0F, verticalTextCenter, "P", boxed);
            display->TextCenter(0.01F, verticalTextCenter, "|", boxed);
            break;

        case RWRSYM_A1:
            display->TextCenter(0.0F, verticalTextCenter, "A", boxed);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, ".", boxed);
            break;

        case RWRSYM_A2:
            display->TextCenter(0.0F, verticalTextCenter, "A", boxed);
            display->TextCenter(0.02F, verticalTextCenter - 0.04F, ".", boxed);
            display->TextCenter(-0.02F, verticalTextCenter - 0.04F, ".", boxed);
            break;

        case RWRSYM_V4 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "4", boxed);
            break;

        case RWRSYM_V5 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "5", boxed);
            break;

        case RWRSYM_V14 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "14", boxed);
            break;

        case RWRSYM_V15:
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "15", boxed);
            break;

        case RWRSYM_V16 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "16", boxed);
            break;

        case RWRSYM_V18 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "18", boxed);
            break;

        case RWRSYM_V19 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "19", boxed);
            break;

        case RWRSYM_V20 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "20", boxed);
            break;

        case RWRSYM_V21 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "21", boxed);
            break;

        case RWRSYM_V22 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "22", boxed);
            break;

        case RWRSYM_V23 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "23", boxed);
            break;

        case RWRSYM_V25:
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "25", boxed);
            break;

        case RWRSYM_V27:
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "27", boxed);
            break;

        case RWRSYM_V29 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "29", boxed);
            break;

        case RWRSYM_V30 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "30", boxed);
            break;

        case RWRSYM_V31 :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "31", boxed);
            break;

        case RWRSYM_VA :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "A", boxed);
            break;

        case RWRSYM_VB :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "B", boxed);
            break;

        case RWRSYM_VP :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "P", boxed);
            break;

        case RWRSYM_VPD :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "PD", boxed);
            break;

        case RWRSYM_VS :
            DrawHat(display);
            display->TextCenter(0.0F, verticalTextCenter - 0.04F, "S", boxed);
            break;

        case RWRSYM_Aa :
            display->TextCenter(0.0F, verticalTextCenter, "A", boxed);
            display->TextCenter(0.01F, verticalTextCenter, "|", boxed);
            break;

        case RWRSYM_Ab :
            display->TextCenter(0.01F, verticalTextCenter, "A", boxed);
            display->TextCenter(0.02F, verticalTextCenter, "|", boxed);
            display->TextCenter(0.0F, verticalTextCenter, "|", boxed);
            break;

        case RWRSYM_Ac :
            display->TextCenter(0.1F, verticalTextCenter, "A", boxed);
            display->TextCenter(-0.01F, verticalTextCenter, "|", boxed);
            display->TextCenter(0.1F, verticalTextCenter, "|", boxed);
            display->TextCenter(0.03F, verticalTextCenter, "|", boxed);
            break;

        case RWRSYM_MIB_F_S:
            if ( not show)
                display->TextCenter(0.0F, verticalTextCenter, "F", boxed);
            else
                display->TextCenter(0.0F, verticalTextCenter, "S", boxed);

            break;

        case RWRSYM_MIB_F_A or 66:
            if (show)
                display->TextCenter(0.0F, verticalTextCenter, "F", boxed);
            else
                display->TextCenter(0.0F, verticalTextCenter, "A", boxed);

            break;

        case RWRSYM_MIB_F_M:
            if ( not show)
                display->TextCenter(0.0F, verticalTextCenter, "F", boxed);
            else
                display->TextCenter(0.0F, verticalTextCenter, "M", boxed);

            break;

        case RWRSYM_MIB_F_U:
            if (show)
                display->TextCenter(0.0F, verticalTextCenter, "F", boxed);
            else
                display->TextCenter(0.0F, verticalTextCenter, "U", boxed);

            break;

        case RWRSYM_MIB_F_BW:
            if ( not show)
                display->TextCenter(0.0F, verticalTextCenter, "F", boxed);
            else

                // RWRSYM_BASIC_INTERCEPTOR:
                for (i = 1; i < NUM_BASIC_AIR_POINTS; i++)
                    display->Line(basicAir[i][0], basicAir[i][1], basicAir[i - 1][0], basicAir[i - 1][1]);

            break;

        case RWRSYM_MIB_BW_S:
            if (show)
                display->TextCenter(0.0F, verticalTextCenter, "S", boxed);
            else

                // RWRSYM_BASIC_INTERCEPTOR:
                for (i = 1; i < NUM_BASIC_AIR_POINTS; i++)
                    display->Line(basicAir[i][0], basicAir[i][1], basicAir[i - 1][0], basicAir[i - 1][1]);

            break;

        case RWRSYM_MIB_BW_A:
            if ( not show)
                display->TextCenter(0.0F, verticalTextCenter, "A", boxed);
            else

                // RWRSYM_BASIC_INTERCEPTOR:
                for (i = 1; i < NUM_BASIC_AIR_POINTS; i++)
                    display->Line(basicAir[i][0], basicAir[i][1], basicAir[i - 1][0], basicAir[i - 1][1]);

            break;

        case RWRSYM_MIB_BW_M:
            if ( not show)
                display->TextCenter(0.0F, verticalTextCenter, "M", boxed);
            else

                // RWRSYM_BASIC_INTERCEPTOR:
                for (i = 1; i < NUM_BASIC_AIR_POINTS; i++)
                    display->Line(basicAir[i][0], basicAir[i][1], basicAir[i - 1][0], basicAir[i - 1][1]);

            break;

        default:

            // JB 010727
            if (symbolID >= 100)
            {
                char buffer[4];
                itoa(symbolID - 100, buffer, 10);
                display->TextCenter(0.0F, verticalTextCenter, buffer, boxed);
            }
            else if (symbolID >= 65)
            {
                char buffer[2];
                buffer[0] = symbolID;
                buffer[1] = 0;
                display->TextCenter(0.0F, verticalTextCenter, buffer, boxed);
            }
            else
                display->TextCenter(0.0F, verticalTextCenter, "U", boxed);

            break;
    }
}

//MI
void RwrClass::DrawHat(VirtualDisplay *display)
{
    static const float  HAT_SIZE = 0.06f;

    if ( not display)
        return;

    // FRB - CTD's here
    display->Line(0.0F, HAT_SIZE + 0.04F, HAT_SIZE, 0.04F);
    display->Line(0.0F, HAT_SIZE + 0.04F, -HAT_SIZE, 0.04F);
}
