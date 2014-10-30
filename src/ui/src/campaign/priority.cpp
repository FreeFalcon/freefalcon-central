// Priorities window stuff here

#include <windows.h>
#include "vu2.h"
#include "gtmobj.h"
#include "objectiv.h"
#include "falcsess.h"
#include "campmap.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "userids.h"
#include "textids.h"
#include "ClassTbl.h"
#include "Cmpclass.h"

#pragma warning(disable : 4127)

extern C_Handler *gMainHandler;

enum
{
    MAX_PAKS = 50,
};

void CloseWindowCB(long, short, C_Base *);
IMAGE_RSC *CreateOccupationMap(long ID, long w, long h, long palsize);
void SendPrimaryObjectiveList(uchar teammask);

// Temporary storage for PAK priorities (so we can undo) [0]=value,[1]=TRUE if we changed the value
static short PAKPriorities[MAX_PAKS][NUM_TEAMS][2];

// Calculated Palette for PAKMap settings
static WORD PAKPalette[101];

IMAGE_RSC *PAKMap = NULL;

static long CurrentPAK = 0;
static char BlinkPAK = 0;

void InitPAKNames()
{
    C_Window *win;
    C_ListBox *lbox;
    long idx;
    _TCHAR buffer[70];
    VuListIterator poit(POList);
    Objective o;

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(PAK_TITLE);

        if (lbox)
        {
            lbox->RemoveAllItems();

            // search list...
            idx = 1;
            o = (Objective) poit.GetFirst();

            while (o)
            {
                _stprintf(buffer, "%s %1ld:", gStringMgr->GetString(TXT_PAK), idx);
                o->GetName(&buffer[_tcsclen(buffer)], 50, TRUE);
                lbox->AddItem(idx, C_TYPE_ITEM, buffer);
                o = (Objective) poit.GetNext();
                idx++;
            }
        }
    }
}

void MakePAKPalette()
{
    short i;

    for (i = 0; i < 101; i++)
        PAKPalette[100 - i] = UI95_RGB24Bit(0x000000ff bitor (((i * 255) / 100) << 8) bitor (((i * 255) / 100) << 16));
}

void InitPAKMap()
{
    C_Window *win;
    C_Bitmap *bmp;

    if ( not PAKMap)
    {
        SetCursor(gCursors[CRSR_WAIT]);
        MakePAKPalette();
        PAKMap = CreateOccupationMap(1, TheCampaign.TheaterSizeX / PAK_MAP_RATIO, TheCampaign.TheaterSizeY / PAK_MAP_RATIO, MAX_PAKS);

        if (PAKMap)
        {
            PAKMap->Header->flags or_eq _RSC_USECOLORKEY_;
            MakeCampMap(MAP_PAK, (uchar*)PAKMap->Owner->GetData(), TheCampaign.TheaterSizeX / PAK_MAP_RATIO * TheCampaign.TheaterSizeY / PAK_MAP_RATIO);

            win = gMainHandler->FindWindow(STRAT_WIN);

            if (win)
            {
                bmp = (C_Bitmap*)win->FindControl(PAK_REGION);

                if (bmp)
                    bmp->SetImage(PAKMap);
            }
        }

        SetCursor(gCursors[CRSR_F16]);
    }
}

void PositionSlider(long value, C_Slider *slider)
{
    long range, pos, maxval;

    if (slider)
    {
        range = slider->GetSliderMax() - slider->GetSliderMin();
        maxval = max(1, slider->GetSteps());

        pos  = range * value / maxval;
        pos += slider->GetSliderMin();

        slider->Refresh();
        slider->SetSliderPos(pos);
        slider->Refresh();
    }
}

long SliderValue(C_Slider *slider)
{
    long range, pos, maxval, value;

    if (slider)
    {
        range = slider->GetSliderMax() - slider->GetSliderMin();
        maxval = max(1, slider->GetSteps());

        pos = slider->GetSliderPos() - slider->GetSliderMin();
        value = pos * maxval / range;

        return(value);
    }

    return(0);
}

void ResetToDefaults()
{
    POData POD;
    long i;
    VuListIterator poit(POList);
    Objective o;

    o = (Objective) poit.GetFirst();

    while (o)
    {
        POD = GetPOData(o);

        if (POD)
        {
            for (i = 0; i < NUM_TEAMS; i++)
                // POD->player_priority[i]=POD->air_priority[i];
                POD->player_priority[i] = -1;

            POD->flags and_eq compl GTMOBJ_PLAYER_SET_PRIORITY;
        }

        o = (Objective) poit.GetNext();
    }

    for (i = 0; i < NUM_TEAMS; i++)
    {
        memcpy(TeamInfo[i]->SetAllObjTypePriority(), DefaultObjtypePriority[TAT_INTERDICT - 1], sizeof(uchar)*MAX_TGTTYPE);
        memcpy(TeamInfo[i]->SetAllUnitTypePriority(), DefaultUnittypePriority[TAT_INTERDICT - 1], sizeof(uchar)*MAX_UNITTYPE);
        memcpy(TeamInfo[i]->SetAllMissionPriority(), DefaultMissionPriority[TAT_INTERDICT - 1], sizeof(uchar)*AMIS_OTHER);
    }
}

void TurnOffHQButton()
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(HQ_FLAG);

        if (btn)
        {
            btn->SetState(0);
            btn->Refresh();
        }
    }
}

void TurnOnHQButton()
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(HQ_FLAG);

        if (btn)
        {
            btn->SetState(1);
            btn->Refresh();
        }
    }
}

// Just turn off HQ Flag if slider moved
void PriSliderCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEDOWN)
        return;

    TurnOffHQButton();
}

// Copies Kevin's priority stuff into the Window's controls
void LoadTargetPriorities()
{
    C_Window *win;
    C_Slider *sldr;
    long value;
    int team = FalconLocalSession->GetTeam();

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        // Target Type priorities
        // Aircraft
        sldr = (C_Slider*)win->FindControl(TARGET_1);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            // Aircraft is based solely on ESCORT Priority.. For lack of a better variable
            // TJL 10/26/03 Aircraft will be targeted by offensive SWEEP mission, not Escorts
            value = TeamInfo[team]->GetMissionPriority(AMIS_SWEEP);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Air fields
        sldr = (C_Slider*)win->FindControl(TARGET_2);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetObjTypePriority(TYPE_AIRBASE);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Air defense
        // Cobra - 2/06 JG - Changed from Air defense to Airstrip
        sldr = (C_Slider*)win->FindControl(TARGET_3);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetObjTypePriority(TYPE_AIRSTRIP);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Radar -to have CCC added and name changed to Radar/CCC
        // Cobra - 2/06 JG - moved CCC to radar and combined
        sldr = (C_Slider*)win->FindControl(TARGET_4);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_RADAR) + TeamInfo[team]->GetObjTypePriority(TYPE_COM_CONTROL)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Army
        sldr = (C_Slider*)win->FindControl(TARGET_5);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_ARMYBASE) + TeamInfo[team]->GetObjTypePriority(TYPE_FORTIFICATION)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // CCC  to be changed to Power plant and refinery
        // Cobra - 2/06 JG - Moved CCC to radar and moved Power plant bitand refinery here
        sldr = (C_Slider*)win->FindControl(TARGET_6);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_POWERPLANT) + TeamInfo[team]->GetObjTypePriority(TYPE_REFINERY)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Infrastructure
        sldr = (C_Slider*)win->FindControl(TARGET_7);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_ROAD) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_BRIDGE) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_RAIL_TERMINAL)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Logistics
        sldr = (C_Slider*)win->FindControl(TARGET_8);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetObjTypePriority(TYPE_DEPOT);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // War production
        sldr = (C_Slider*)win->FindControl(TARGET_9);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_CHEMICAL) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_FACTORY) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_NUCLEAR)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Air Defense Units
        // 2/06 Moved Air defense units to this slot
        sldr = (C_Slider*)win->FindControl(TARGET_10);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_AIR_DEFENSE);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Armored Units
        sldr = (C_Slider*)win->FindControl(TARGET_11);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ARMOR) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ARMORED_CAV)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Infantry Units
        sldr = (C_Slider*)win->FindControl(TARGET_12);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_INFANTRY) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_MARINE) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_MECHANIZED) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_AIRMOBILE) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_RESERVE)) / 5;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Artillery Units
        sldr = (C_Slider*)win->FindControl(TARGET_13);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ROCKET) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_SP_ARTILLERY) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_SS_MISSILE) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_TOWED_ARTILLERY)) / 4;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Support units
        sldr = (C_Slider*)win->FindControl(TARGET_14);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ENGINEER) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_HQ)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Naval Units
        sldr = (C_Slider*)win->FindControl(TARGET_15);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetUnitTypePriority(MAX_UNITTYPE - 1);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }
    }
}

// Copies Kevin's priority stuff into the Window's controls
void LoadMissionPriorities()
{
    C_Window *win;
    C_Slider *sldr;
    long value;
    int team = FalconLocalSession->GetTeam();

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        // Mission Types
        // OCA
        sldr = (C_Slider*)win->FindControl(MISSION_1);

        if (sldr)
            // TJL 10/26/03 Changed AMIS_SWEEP to AMIS_ESCORT.
            // Cobra - 2/06 JG - Moved AMIS_ESCORT to Recon and changed to Escort.
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetMissionPriority(AMIS_OCASTRIKE) ;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // SAM Suppression
        sldr = (C_Slider*)win->FindControl(MISSION_2);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetMissionPriority(AMIS_SEADSTRIKE);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Interdiction
        sldr = (C_Slider*)win->FindControl(MISSION_3);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_SAD) +
                     TeamInfo[team]->GetMissionPriority(AMIS_INT) +
                     TeamInfo[team]->GetMissionPriority(AMIS_BAI)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // CAS
        sldr = (C_Slider*)win->FindControl(MISSION_4);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_ONCALLCAS) +
                     TeamInfo[team]->GetMissionPriority(AMIS_PRPLANCAS) +
                     TeamInfo[team]->GetMissionPriority(AMIS_CAS)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Strategic Strikes
        sldr = (C_Slider*)win->FindControl(MISSION_5);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_INTSTRIKE) +
                     TeamInfo[team]->GetMissionPriority(AMIS_STRIKE) +
                     TeamInfo[team]->GetMissionPriority(AMIS_DEEPSTRIKE)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Anti shipping
        sldr = (C_Slider*)win->FindControl(MISSION_6);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_ASW) +
                     TeamInfo[team]->GetMissionPriority(AMIS_ASHIP)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // DCA
        sldr = (C_Slider*)win->FindControl(MISSION_7);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_BARCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_BARCAP2) +
                     TeamInfo[team]->GetMissionPriority(AMIS_HAVCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_TARCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_RESCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_AMBUSHCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_INTERCEPT)) / 7;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Recon   to become Escort
        // Cobra - 2/06 JG - Moved  AMIS_ESCORT bitand AMIS_SEADESCORT to Recon and changed to Recon to Escort
        sldr = (C_Slider*)win->FindControl(MISSION_8);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_ESCORT) +
                     TeamInfo[team]->GetMissionPriority(AMIS_SEADESCORT)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }
    }
}

void LoadPAKPriorities()
{
    long i, idx;
    POData POD;
    short teamid;
    WORD *Pal;
    short CampControl = 1;

    if ( not PAKMap)
        return;

    teamid = FalconLocalSession->GetTeam();
    Pal = (WORD*)((char *)(PAKMap->Owner->GetData() + PAKMap->Header->paletteoffset));

    memset(PAKPriorities, 0, sizeof(PAKPriorities));

    VuListIterator poit(POList);
    Objective o;

    idx = 1;
    o = (Objective) poit.GetFirst();

    while (o)
    {
        POD = GetPOData(o);

        if (POD)
        {
            for (i = 0; i < NUM_TEAMS; i++)
            {
                if (POD->player_priority[i] >= 0)
                {
                    PAKPriorities[idx][i][0] = (uchar)POD->player_priority[i];
                    CampControl = 0;
                }
                else
                    PAKPriorities[idx][i][0] = (uchar)POD->air_priority[i];
            }

            Pal[idx] = PAKPalette[PAKPriorities[idx][teamid][0]];

            if (CampControl and POD->flags bitand GTMOBJ_PLAYER_SET_PRIORITY)
                CampControl = 0;
        }

        o = (Objective) poit.GetNext();
        idx++;
    }

    if (CampControl)
        TurnOnHQButton();
    else
        TurnOffHQButton();
}

////////////////////////////
// Copies Kevin's priority stuff into the Window's controls
void LoadDefaultTargetPriorities()
{
    C_Window *win;
    C_Slider *sldr;
    long value;
    int team = FalconLocalSession->GetTeam();

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        // Target Type priorities
        // Aircraft
        sldr = (C_Slider*)win->FindControl(TARGET_1);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetMissionPriority(AMIS_SWEEP); // TJL 10/26/03 Changed from Escort to Sweep
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Air fields
        sldr = (C_Slider*)win->FindControl(TARGET_2);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetObjTypePriority(TYPE_AIRBASE);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Air defense to be changed to airstrip
        // Cobra - 2/06 JG - Changed from Air defense to Airstrip
        sldr = (C_Slider*)win->FindControl(TARGET_3);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetObjTypePriority(TYPE_AIRSTRIP);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Radar -to have CCC added and name changed to Radar/CCC
        // Cobra - 2/06 JG - moved CCC to radar and combined
        sldr = (C_Slider*)win->FindControl(TARGET_4);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_RADAR) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_COM_CONTROL)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Army
        sldr = (C_Slider*)win->FindControl(TARGET_5);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_ARMYBASE) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_FORTIFICATION)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // CCC  to be changed to Power plant and refinery
        // Cobra - 2/06 JG - Moved CCC to radar and moved Power plant bitand refinery here
        sldr = (C_Slider*)win->FindControl(TARGET_6);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_POWERPLANT) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_REFINERY)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Infrastructure
        sldr = (C_Slider*)win->FindControl(TARGET_7);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_ROAD) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_BRIDGE) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_RAIL_TERMINAL)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Logistics
        sldr = (C_Slider*)win->FindControl(TARGET_8);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetObjTypePriority(TYPE_DEPOT);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // War production
        sldr = (C_Slider*)win->FindControl(TARGET_9);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetObjTypePriority(TYPE_CHEMICAL) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_FACTORY) +
                     TeamInfo[team]->GetObjTypePriority(TYPE_NUCLEAR)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Air Defense Units
        // 2/06 Moved Air defense units to this slot
        sldr = (C_Slider*)win->FindControl(TARGET_10);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_AIR_DEFENSE);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Armored Units
        sldr = (C_Slider*)win->FindControl(TARGET_11);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ARMOR) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ARMORED_CAV)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Infantry Units
        sldr = (C_Slider*)win->FindControl(TARGET_12);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_INFANTRY) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_MARINE) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_MECHANIZED) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_AIRMOBILE) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_RESERVE)) / 5;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Artillery Units
        sldr = (C_Slider*)win->FindControl(TARGET_13);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ROCKET) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_SP_ARTILLERY) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_SS_MISSILE) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_TOWED_ARTILLERY)) / 4;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Support units
        sldr = (C_Slider*)win->FindControl(TARGET_14);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_ENGINEER) +
                     TeamInfo[team]->GetUnitTypePriority(STYPE_UNIT_HQ)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Naval Units
        sldr = (C_Slider*)win->FindControl(TARGET_15);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetUnitTypePriority(MAX_UNITTYPE - 1);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }
    }
}

// Copies Kevin's priority stuff into the Window's controls
void LoadDefaultMissionPriorities()
{
    C_Window *win;
    C_Slider *sldr;
    long value;
    int team = FalconLocalSession->GetTeam();

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        // Mission Types
        // OCA
        sldr = (C_Slider*)win->FindControl(MISSION_1);

        if (sldr)
            // TJL 10/26/03 Changed AMIS_SWEEP to AMIS_ESCORT.
            // Cobra - 2/06 JG - Moved AMIS_ESCORT to Recon and changed to Escort.
        {
            sldr->SetCallback(PriSliderCB);
            value = TeamInfo[team]->GetMissionPriority(AMIS_OCASTRIKE);
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // SAM Suppression
        sldr = (C_Slider*)win->FindControl(MISSION_2);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_SEADSTRIKE) +
                     TeamInfo[team]->GetMissionPriority(AMIS_SEADESCORT)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Interdiction
        sldr = (C_Slider*)win->FindControl(MISSION_3);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_SAD) +
                     TeamInfo[team]->GetMissionPriority(AMIS_INT) +
                     TeamInfo[team]->GetMissionPriority(AMIS_BAI)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // CAS
        sldr = (C_Slider*)win->FindControl(MISSION_4);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_ONCALLCAS) +
                     TeamInfo[team]->GetMissionPriority(AMIS_PRPLANCAS) +
                     TeamInfo[team]->GetMissionPriority(AMIS_CAS)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Strategic Strikes
        sldr = (C_Slider*)win->FindControl(MISSION_5);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_INTSTRIKE) +
                     TeamInfo[team]->GetMissionPriority(AMIS_STRIKE) +
                     TeamInfo[team]->GetMissionPriority(AMIS_DEEPSTRIKE)) / 3;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Anti shipping
        sldr = (C_Slider*)win->FindControl(MISSION_6);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_ASW) +
                     TeamInfo[team]->GetMissionPriority(AMIS_ASHIP)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // DCA
        sldr = (C_Slider*)win->FindControl(MISSION_7);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_BARCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_BARCAP2) +
                     TeamInfo[team]->GetMissionPriority(AMIS_HAVCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_TARCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_RESCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_AMBUSHCAP) +
                     TeamInfo[team]->GetMissionPriority(AMIS_INTERCEPT)) / 7;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }

        // Recon   to become Escort
        // Cobra - 2/06 JG - Moved  AMIS_ESCORT bitand AMIS_SEADESCORT to Recon and changed to Recon to Escort
        sldr = (C_Slider*)win->FindControl(MISSION_8);

        if (sldr)
        {
            sldr->SetCallback(PriSliderCB);
            value = (TeamInfo[team]->GetMissionPriority(AMIS_ESCORT) +
                     TeamInfo[team]->GetMissionPriority(AMIS_SEADESCORT)) / 2;
            PositionSlider(value, sldr); // Value is in range of 0 -> 100
        }
    }
}

void LoadDefaultPAKPriorities()
{
    long i, idx;
    POData POD;
    short teamid;
    WORD *Pal;

    if ( not PAKMap)
        return;

    teamid = FalconLocalSession->GetTeam();
    Pal = (WORD*)((char *)(PAKMap->Owner->GetData() + PAKMap->Header->paletteoffset));

    memset(PAKPriorities, 0, sizeof(PAKPriorities));

    VuListIterator poit(POList);
    Objective o;

    idx = 1;
    o = (Objective) poit.GetFirst();

    while (o)
    {
        POD = GetPOData(o);

        if (POD)
        {
            for (i = 0; i < NUM_TEAMS; i++)
                PAKPriorities[idx][i][0] = (uchar)POD->air_priority[i];

            Pal[idx] = PAKPalette[PAKPriorities[idx][teamid][0]];
        }

        o = (Objective) poit.GetNext();
        idx++;
    }
}

// Retrieves the Window's control values, and put's them in Kevin's structures
void SaveTargetPriorities()
{
    C_Window *win;
    C_Slider *sldr;
    uchar value;
    int team = FalconLocalSession->GetTeam();

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        // Target Type priorities
        // Aircraft
        sldr = (C_Slider*)win->FindControl(TARGET_1);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_SWEEP, value); //TJL 10/26/03 Changed form Escort to Sweep
        }

        // Air fields
        sldr = (C_Slider*)win->FindControl(TARGET_2);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_AIRBASE, value);
        }

        // Air defense to be changed to airstrip
        // Cobra - 2/06 JG - Changed from Air defense to Airstrip
        sldr = (C_Slider*)win->FindControl(TARGET_3);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_AIRSTRIP, value);
        }

        // Radar -to have CCC added and name changed to Radar/CCC
        // Cobra - 2/06 JG - moved CCC to radar and combined
        sldr = (C_Slider*)win->FindControl(TARGET_4);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_RADAR, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_COM_CONTROL, value);
        }

        // Army
        sldr = (C_Slider*)win->FindControl(TARGET_5);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_ARMYBASE, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_FORTIFICATION, value);
        }

        // CCC  to be changed to Power plant and refinery
        // Cobra - 2/06 JG - Moved CCC to radar and moved Power plant bitand refinery here
        sldr = (C_Slider*)win->FindControl(TARGET_6);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_POWERPLANT, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_REFINERY, value);
        }

        // Infrastructure
        sldr = (C_Slider*)win->FindControl(TARGET_7);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_ROAD, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_BRIDGE, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_RAIL_TERMINAL, value);
        }

        // Logistics
        sldr = (C_Slider*)win->FindControl(TARGET_8);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_DEPOT, value);
        }

        // War production
        sldr = (C_Slider*)win->FindControl(TARGET_9);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetObjTypePriority(TYPE_CHEMICAL, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_FACTORY, value);
            TeamInfo[team]->SetObjTypePriority(TYPE_NUCLEAR, value);
        }

        // Air Defense Units
        // 2/06 Moved Air defense units to this slot
        sldr = (C_Slider*)win->FindControl(TARGET_10);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_AIR_DEFENSE, value);
        }

        // Armored Units
        sldr = (C_Slider*)win->FindControl(TARGET_11);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_ARMOR, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_ARMORED_CAV, value);
        }

        // Infantry Units
        sldr = (C_Slider*)win->FindControl(TARGET_12);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_INFANTRY, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_MARINE, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_MECHANIZED, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_AIRMOBILE, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_RESERVE, value);
        }

        // Artillery Units
        sldr = (C_Slider*)win->FindControl(TARGET_13);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_ROCKET, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_SP_ARTILLERY, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_SS_MISSILE, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_TOWED_ARTILLERY, value);
        }

        // Support units
        sldr = (C_Slider*)win->FindControl(TARGET_14);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_ENGINEER, value);
            TeamInfo[team]->SetUnitTypePriority(STYPE_UNIT_HQ, value);
        }

        // Naval Units
        sldr = (C_Slider*)win->FindControl(TARGET_15);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetUnitTypePriority(MAX_UNITTYPE - 1, value);
            // TeamInfo[team]->SetMissionPriority(AMIS_ASHIP, value);
        }
    }
}

void SaveMissionPriorities()
{
    C_Window *win;
    C_Slider *sldr;
    uchar value;
    int team = FalconLocalSession->GetTeam();

    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        // Mission Types
        // OCA
        sldr = (C_Slider*)win->FindControl(MISSION_1);

        if (sldr)
            // TJL 10/26/03 Changed AMIS_SWEEP to AMIS_ESCORT.
            // Cobra - 2/06 JG - Moved AMIS_ESCORT to Recon and changed to Escort.
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_OCASTRIKE, value);
        }

        // SAM Suppression
        sldr = (C_Slider*)win->FindControl(MISSION_2);

        if (sldr)
        {
            value = static_cast<char>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_SEADSTRIKE, value);
            TeamInfo[team]->SetMissionPriority(AMIS_SEADESCORT, value);
        }

        // Interdiction
        sldr = (C_Slider*)win->FindControl(MISSION_3);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_SAD, value);
            TeamInfo[team]->SetMissionPriority(AMIS_INT, value);
            TeamInfo[team]->SetMissionPriority(AMIS_BAI, value);
        }

        // CAS
        sldr = (C_Slider*)win->FindControl(MISSION_4);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_ONCALLCAS, value);
            TeamInfo[team]->SetMissionPriority(AMIS_PRPLANCAS, value);
            TeamInfo[team]->SetMissionPriority(AMIS_CAS, value);
        }

        // Strategic Strikes
        sldr = (C_Slider*)win->FindControl(MISSION_5);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_INTSTRIKE, value);
            TeamInfo[team]->SetMissionPriority(AMIS_STRIKE, value);
            TeamInfo[team]->SetMissionPriority(AMIS_DEEPSTRIKE, value);
        }

        // Anti shipping
        sldr = (C_Slider*)win->FindControl(MISSION_6);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_ASW, value);
            TeamInfo[team]->SetMissionPriority(AMIS_ASHIP, value);
        }

        // DCA
        sldr = (C_Slider*)win->FindControl(MISSION_7);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_BARCAP, value);
            TeamInfo[team]->SetMissionPriority(AMIS_BARCAP2, value);
            TeamInfo[team]->SetMissionPriority(AMIS_HAVCAP, value);
            TeamInfo[team]->SetMissionPriority(AMIS_TARCAP, value);
            TeamInfo[team]->SetMissionPriority(AMIS_RESCAP, value);
            TeamInfo[team]->SetMissionPriority(AMIS_AMBUSHCAP, value);
            TeamInfo[team]->SetMissionPriority(AMIS_INTERCEPT, value);
        }

        // Recon   to become Escort
        // Cobra - 2/06 JG - Moved  AMIS_ESCORT bitand AMIS_SEADESCORT to Recon and changed to Recon to Escort
        sldr = (C_Slider*)win->FindControl(MISSION_8);

        if (sldr)
        {
            value = static_cast<uchar>(SliderValue(sldr));
            TeamInfo[team]->SetMissionPriority(AMIS_ESCORT, value);
            TeamInfo[team]->SetMissionPriority(AMIS_SEADESCORT, value);
        }
    }
}

void SavePAKPriorities()
{
    long i, idx;
    POData POD;

    VuListIterator poit(POList);
    Objective o;

    idx = 1;
    o = (Objective) poit.GetFirst();

    while (o)
    {
        POD = GetPOData(o);

        if (POD)
        {
            // KCK: Technically, the player should only be able to modify priorities for his team
            i = FalconLocalSession->GetTeam();

            if (i > 0 and i < NUM_TEAMS and TeamInfo[i])
                // for(i=0;i<NUM_TEAMS;i++)
            {
                POD->player_priority[i] = PAKPriorities[idx][i][0];
                POD->flags or_eq GTMOBJ_PLAYER_SET_PRIORITY;
                PAKPriorities[idx][i][1] = 0;
            }
        }

        o = (Objective) poit.GetNext();
        idx++;
    }

    // For multiplayer - send our new priorities
    if (TRUE) // isOnline()
    {
        uchar teammask;
        teammask = static_cast<uchar>((1 << FalconLocalSession->GetTeam()));
        SendPrimaryObjectiveList(teammask);
    }
}

// Init the PAK Slider to the campaign's current value
void SelectPAK(long PAKID, long TeamID)
{
    C_Window *win;
    C_Slider *sldr;
    WORD *Pal;
    F4CSECTIONHANDLE *Leave;

    if (PAKID >= MAX_PAKS or TeamID >= NUM_TEAMS)
        return;


    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        Leave = UI_Enter(win);

        if (BlinkPAK < 0) // restore before changing
        {
            Pal = (WORD*)(PAKMap->Owner->GetData() + PAKMap->Header->paletteoffset);
            Pal[CurrentPAK] = PAKPalette[PAKPriorities[CurrentPAK][FalconLocalSession->GetTeam()][0]];
        }

        CurrentPAK = PAKID;
        BlinkPAK = static_cast<char>(CurrentPAK);
        sldr = (C_Slider*)win->FindControl(PAK_SLIDER);

        if (sldr)
            PositionSlider(100 - PAKPriorities[PAKID][TeamID][0], sldr); // Value is in range of 0 -> 100

        UI_Leave(Leave);
    }
}

// UI Callbacks

void PriorityTabsCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    control->Parent_->HideCluster(control->GetUserNumber(1));
    control->Parent_->HideCluster(control->GetUserNumber(2));
    control->Parent_->UnHideCluster(control->GetUserNumber(0));

    control->Parent_->RefreshWindow();
}

void UsePriotityCB(long ID, short hittype, C_Base *control)
{
    C_Button *btn;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    btn = (C_Button*)control->Parent_->FindControl(HQ_FLAG);

    if (btn and not btn->GetState())
    {
        SaveTargetPriorities();
        SaveMissionPriorities();
        SavePAKPriorities();
    }
    else
        ResetToDefaults();

    CloseWindowCB(ID, hittype, control);
}

void ResetPriorityCB(long, short hittype, C_Base *control)
{
    long PAKID;
    C_ListBox *lbox;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // Kevin TODO: Reset priorities to those set by the campaign

    LoadTargetPriorities();
    LoadMissionPriorities();
    LoadPAKPriorities();

    lbox = (C_ListBox*)control->Parent_->FindControl(PAK_TITLE);

    if (lbox)
    {
        PAKID = ((C_ListBox*)lbox)->GetTextID();

        if ( not PAKID)
            PAKID = 1;

        if (PAKID)
            SelectPAK(PAKID, FalconLocalSession->GetTeam());
    }
}

void CancelPriorityCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    CloseWindowCB(ID, hittype, control);
}

void OpenPriorityCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_Button *btn;
    C_ListBox *lbox;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    InitPAKMap();

    LoadTargetPriorities();
    LoadMissionPriorities();
    LoadPAKPriorities();

    SelectPAK(1, FalconLocalSession->GetTeam());

    // Init window... make sure proper stuff is visible or hidden
    win = gMainHandler->FindWindow(STRAT_WIN);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(PAK_TITLE);

        if (lbox)
        {
            lbox->Refresh();
            lbox->SetValue(1);
            lbox->Refresh();
        }

        btn = (C_Button*)win->FindControl(TARGET_PRIORITIES);

        if (btn)
        {
            btn->SetState(1);
            PriorityTabsCB(btn->GetID(), C_TYPE_LMOUSEUP, btn);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(MISSION_PRIORITIES);

        if (btn)
        {
            btn->SetState(0);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(PAK_PRIORITIES);

        if (btn)
        {
            btn->SetState(0);
            btn->Refresh();
        }
    }

    // Display window
    gMainHandler->EnableWindowGroup(control->GetGroup());
}

// Callback from MAP x,y checker
void MapSelectPAKCB(long, short hittype, C_Base *control)
{
    C_Button *btn;
    C_ListBox *lbox;
    long x, y, PAKID;
    char *overlay;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not control or not PAKMap)
        return;

    btn = (C_Button*)control;
    x = btn->GetRelX();
    y = btn->GetRelY();

    if (x < 0 or x >= TheCampaign.TheaterSizeX / PAK_MAP_RATIO or y < 0 or y >= TheCampaign.TheaterSizeY / PAK_MAP_RATIO)
        return;

    overlay = PAKMap->Owner->GetData();

    PAKID = overlay[y * TheCampaign.TheaterSizeX / PAK_MAP_RATIO + x];

    if (PAKID)
    {
        SelectPAK(PAKID, FalconLocalSession->GetTeam());
        lbox = (C_ListBox*)control->Parent_->FindControl(PAK_TITLE);

        if (lbox)
        {
            lbox->Refresh();
            lbox->SetValue(PAKID);
            lbox->Refresh();
        }
    }
}

void SelectPAKCB(long, short hittype, C_Base *control)
{
    long PAKID;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    PAKID = ((C_ListBox*)control)->GetTextID();

    if (PAKID)
        SelectPAK(PAKID, FalconLocalSession->GetTeam());
}

void SetPAKPriorityCB(long, short hittype, C_Base *control)
{
    long value;

    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    value = 100 - SliderValue((C_Slider*)control);

    PAKPriorities[CurrentPAK][FalconLocalSession->GetTeam()][0] = static_cast<short>(value);
    PAKPriorities[CurrentPAK][FalconLocalSession->GetTeam()][1] = 1; // Mark as changed

    TurnOffHQButton();
}

void SetCampaignPrioritiesCB(long, short hittype, C_Base *base)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (base->GetState())
    {
        LoadDefaultTargetPriorities();
        LoadDefaultMissionPriorities();
        LoadDefaultPAKPriorities();

        SelectPAK(CurrentPAK, FalconLocalSession->GetTeam());
    }
    else
    {

    }

    base->Refresh();
}

BOOL PAKMapTimerCB(C_Base *me)
{
    WORD *Pal;

    if (CurrentPAK)
    {
        me->SetUserNumber(0, (me->GetUserNumber(0) + 1) bitand 0x03);

        if ( not me->GetUserNumber(0))
        {
            Pal = (WORD*)(PAKMap->Owner->GetData() + PAKMap->Header->paletteoffset);
            BlinkPAK = static_cast<char>(-BlinkPAK);

            if (BlinkPAK < 0) // Flash BLACK
            {
                Pal[CurrentPAK] = 0;
            }
            else // Show Actual color
            {
                Pal[CurrentPAK] = PAKPalette[PAKPriorities[CurrentPAK][FalconLocalSession->GetTeam()][0]];
            }

            return(TRUE);
        }
    }

    return(FALSE);
}
