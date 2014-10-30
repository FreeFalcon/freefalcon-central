#include "stdhdr.h"
#include "camplib.h"
#include "mfd.h"
#include "Graphics/Include/render2d.h"
#include "dispcfg.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "hud.h"
#include "aircrft.h"
#include "fack.h"
#include "otwdrive.h" //MI
#include "cpmanager.h" //MI
#include "icp.h" //MI
#include "aircrft.h" //MI
#include "fcc.h" //MI
#include "radardoppler.h" //MI

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);


struct MfdTestButtons
{
    char *label1, *label2;
    enum { ModeNoop = 0,  // do nothing
           ModeParent, // hand off to parent
           ModeTest1,
           ModeTest2, // two test sub modes
           ModeRaltTest,
           ModeRunTest,
           ModeClear,
         };
    int nextMode;
};
#define NOENTRY { NULL, NULL, MfdTestButtons::ModeNoop}
#define PARENT { NULL, NULL, MfdTestButtons::ModeParent}


static const MfdTestButtons testpage1[20] =
{
    // test page menu
    {"BIT1", NULL, MfdTestButtons::ModeTest2},    // 1
    NOENTRY,
    {"CLR", NULL, MfdTestButtons::ModeClear},
    NOENTRY,
    NOENTRY,    // 5
    {"MFDS", NULL, MfdTestButtons::ModeRunTest},
    {"RALT", "500", MfdTestButtons::ModeRaltTest},
    {"TGP", NULL, MfdTestButtons::ModeRunTest},
    {"FINS", NULL, MfdTestButtons::ModeRunTest},
    {"TFR", NULL, MfdTestButtons::ModeRunTest},    // 10
    PARENT,
    PARENT,
    PARENT,
    PARENT, // current mode
    PARENT,    // 15
    {"RSU", NULL, MfdTestButtons::ModeRunTest},
    {"INS", NULL, MfdTestButtons::ModeRunTest},
    {"SMS", NULL, MfdTestButtons::ModeNoop},
    {"FCR", NULL, MfdTestButtons::ModeRunTest},
    {"DTE", NULL, MfdTestButtons::ModeRunTest},    // 20
};

static const MfdTestButtons testpage2[20] =
{
    // test page menu
    {"BIT2", NULL, MfdTestButtons::ModeTest1},    // 1
    NOENTRY,
    {"CLR", NULL, MfdTestButtons::ModeClear},
    NOENTRY,
    NOENTRY,    // 5
    {"IFF1", NULL, MfdTestButtons::ModeRunTest},
    {"IFF2", NULL, MfdTestButtons::ModeRunTest},
    {"IFF3", NULL, MfdTestButtons::ModeRunTest},
    {"IFFC", NULL, MfdTestButtons::ModeRunTest},
    {"TCN", NULL, MfdTestButtons::ModeRunTest},    // 10
    PARENT,
    PARENT,
    PARENT,
    PARENT,
    PARENT,    // 15
    {NULL, NULL, MfdTestButtons::ModeNoop},
    {NULL, NULL, MfdTestButtons::ModeNoop},
    {NULL, NULL, MfdTestButtons::ModeNoop},
    {"TISL", NULL, MfdTestButtons::ModeRunTest},
    {"UFC", NULL, MfdTestButtons::ModeRunTest},    // 20
};
struct MfdTestPage
{
    const MfdTestButtons *buttons;
};
static const MfdTestPage mfdpages[] =
{
    {testpage1},
    {testpage2},
};
static const int NMFDPAGES = sizeof(mfdpages) / sizeof(mfdpages[0]);

TestMfdDrawable::TestMfdDrawable()
{
    bitpage = 0;
    bittest = -1;
    timer = 0;
}

void TestMfdDrawable::Display(VirtualDisplay* newDisplay)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    //MI
    float cX, cY = 0;

    if (g_bRealisticAvionics)
    {
        RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);

        if ( not theRadar)
        {
            ShiWarning("Oh Oh shouldn't be here without a radar");
            return;
        }
        else
        {
            theRadar->GetCursorPosition(&cX, &cY);
        }
    }

    display = newDisplay;

    ShiAssert(bitpage >= 0 and bitpage < sizeof(mfdpages) / sizeof(mfdpages[0]));
    ShiAssert(display not_eq NULL);

    const MfdTestButtons *mb = mfdpages[bitpage].buttons;
    AircraftClass *self = MfdDisplay[OnMFD()]->GetOwnShip();
    ShiAssert(self not_eq NULL);

    //MI changed
    if (g_bRealisticAvionics)
    {
        if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp and 
            OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
        {
            DrawBullseyeCircle(display, cX, cY);
        }
        else
            DrawReference(self);
    }
    else
        DrawReference(self);

    display->SetColor(GetMfdColor(MFD_LABELS));
    char buf[100];

    for (int i = 0; i < 20; i++)
    {
        int hilite = 0;

        if (i == bittest and timer > SimLibElapsedTime)
            hilite = 1;

        switch (mb[i].nextMode)
        {
            case MfdTestButtons::ModeRaltTest:
                sprintf(buf, "%.0f", hilite ? 300.0f : TheHud->lowAltWarning);
                LabelButton(i, mb[i].label1, buf, hilite);
                break;

            default:
                if (mb[i].label1)
                    LabelButton(i, mb[i].label1, mb[i].label2, hilite);
                else if (mb[i].nextMode == MfdTestButtons::ModeParent)
                    MfdDrawable::DefaultLabel(i);
        }
    }

    if (playerAC and playerAC->mFaults)
    {
        FackClass *fack = playerAC->mFaults;
        float yinc = display->TextHeight();
        const static float namex = -0.6f;
        const static float starty = 0.6f;
        float y = starty;
        float x = namex;
        float xinc = 0.3F;

        for (int i = 0; i < fack->GetMflListCount(); i++)
        {
            const char *fname;
            int subsys;
            int count;
            char timestr[100];

            if (fack->GetMflEntry(i, &fname, &subsys, &count, timestr) == false)
                continue;

            char outstr[100];

            for (int i = 0; i < 5; i++)
            {
                switch (i)
                {
                    case 1:
                        sprintf(outstr, "%-4s", fname);
                        display->TextLeft(x, y, outstr);
                        x += xinc;
                        break;

                    case 2:
                        sprintf(outstr, "%03d", subsys);
                        display->TextLeft(x, y, outstr);
                        x += xinc;
                        break;

                    case 3:
                        x -= 0.1F;
                        sprintf(outstr, "%2d", count);
                        display->TextLeft(x, y, outstr);
                        x += xinc;
                        break;

                    case 4:
                        x -= 0.1F;
                        sprintf(outstr, "%s", timestr);
                        display->TextLeft(x, y, outstr);
                        x += xinc;
                        break;

                    default:
                        break;
                }
            }

            //sprintf (outstr, "%-4s %03d %2d %s", fname, subsys, count, timestr);
            //ShiAssert(strlen(outstr) < sizeof outstr);
            //display->TextLeft(namex, y, outstr);
            y -= yinc;
            x = namex;
        }
    }
}

void TestMfdDrawable::PushButton(int whichButton, int whichMFD)
{
    ShiAssert(bitpage >= 0 and bitpage < sizeof(mfdpages) / sizeof(mfdpages[0]));
    ShiAssert(whichButton >= 0 and whichButton < 20);

    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    switch (mfdpages[bitpage].buttons[whichButton].nextMode)
    {
        case MfdTestButtons::ModeNoop:
            break;

        case MfdTestButtons::ModeRaltTest:
        case MfdTestButtons::ModeRunTest:
            bittest = whichButton;
            timer = SimLibElapsedTime + 5 * CampaignSeconds;
            break;

        case MfdTestButtons::ModeTest2:
            bitpage = 1;
            break;

        case MfdTestButtons::ModeTest1:
            bitpage = 0;
            break;

        case MfdTestButtons::ModeParent:
            MfdDrawable::PushButton(whichButton, whichMFD);
            break;

        case MfdTestButtons::ModeClear: // clear MFL
            if (playerAC and playerAC->mFaults)
                playerAC->mFaults->ClearMfl();

            break;
    }
}
