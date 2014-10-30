///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Tactical Engagement - Robin Heydon
//
// Implements the user interface for the tactical engagement section
// of FreeFalcon
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "falclib.h"
#include "vu2.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "cmpclass.h"
#include "division.h"
#include "cmap.h"
#include "tac_class.h"
#include "te_defs.h"
#include "find.h"
#include "camplist.h"
#include "gps.h"
#include "campmap.h"

extern GlobalPositioningSystem *gGps;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    SMFLGS_USA  = 52012,
    SMFLGS_NKOREA  = 52013,
    SMFLGS_CHINA  = 52014,
    SMFLGS_FRANCE  = 52015,
    SMFLGS_CIS  = 52016,
    SMFLGS_SKOREA   = 52017,
    SMFLGS_JAPAN  = 52018,
    SMFLGS_GERMAN  = 52019,
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern IMAGE_RSC *gOccupationMap;
extern int MRX;
extern int MRY;
extern uchar gSelectedTeam;
extern long gDrawTeam;
extern void RebuildFLOTList(void); // 2001-10-31 M.N.
extern int RebuildFrontList(void);  // 2001-10-31 M.N.

long OwnershipChanged = 0;

uchar *te_restore_map = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_territory_editor_clear(long ID, short hittype, C_Base *control);
void tactical_territory_editor_restore(long ID, short hittype, C_Base *control);
void tactical_territory_map_edit(long ID, short hittype, C_Base *control);
void UpdateOwners();

// Run the list of campaign things and change ownership as required
void UpdateObjectiveOwnership(void);
void UpdateUnitOwnership(void);

void tactical_team_selection(long ID, short hittype, C_Base *control);

void MakeOccupationMap(IMAGE_RSC *Map);
IMAGE_RSC *CreateOccupationMap(long ID, long w, long h, long palsize);
static void SetupMapWindow(void);
void CampaignListCB(void);
void UpdateOccupationMap(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_territory_editor_clear(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (TheCampaign.CampMapData)
    {
        memset(TheCampaign.CampMapData, 0, TheCampaign.CampMapSize);
        UpdateOccupationMap();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_territory_editor_restore(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ((TheCampaign.CampMapData) and (te_restore_map))
    {
        memcpy(TheCampaign.CampMapData, te_restore_map, TheCampaign.CampMapSize);
        UpdateOccupationMap();
    }
}

void tactical_set_drawmode(long, short hittype, C_Base *)
{
    if (hittype == C_TYPE_LMOUSEUP)
        return;

    gDrawTeam = gSelectedTeam;
}

void tactical_set_erasemode(long, short hittype, C_Base *)
{
    if (hittype == C_TYPE_LMOUSEUP)
        return;

    gDrawTeam = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int get_group_cover(int x, int y)
{
    int
    dx,
    dy,
    cover;

    for (dx = 0; dx < MAP_RATIO; dx ++)
    {
        for (dy = 0; dy < MAP_RATIO; dy ++)
        {
            cover = GetCover(static_cast<short>(x + dx), static_cast<short>(y + dy));

            if (cover not_eq Water)
            {
                return cover;
            }
        }
    }

    return Water;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int
fill_width,
fill_height,
fill_team;

static unsigned char
*fill_src;

static void flood_fill_team(int x, int y)
{
    int
    old_team;

    unsigned char
    cover;

    if (x < 0 or y < 0 or x >= fill_width or y >= fill_height)
        return;

    if (x bitand 1)
    {
        old_team = (fill_src[(y * fill_width + x) / 2] bitand 0xf0) >> 4;
    }
    else
    {
        old_team = fill_src[(y * fill_width + x) / 2] bitand 0x0f;
    }

    if ((old_team == gDrawTeam) or (old_team not_eq fill_team))
    {
        return;
    }

    cover = static_cast<uchar>(get_group_cover(x * MAP_RATIO, y * MAP_RATIO));

    if (cover not_eq Water)
    {

        if (x bitand 1)
        {
            fill_src[(y * fill_width + x) / 2] and_eq 0x0f;
            fill_src[(y * fill_width + x) / 2] or_eq gDrawTeam  << 4;

            flood_fill_team(x + 0, y - 1);
            flood_fill_team(x - 1, y + 0);
            flood_fill_team(x + 1, y + 0);
            flood_fill_team(x + 0, y + 1);
        }
        else
        {
            fill_src[(y * fill_width + x) / 2] and_eq 0xf0;
            fill_src[(y * fill_width + x) / 2] or_eq gDrawTeam ;

            flood_fill_team(x + 0, y - 1);
            flood_fill_team(x - 1, y + 0);
            flood_fill_team(x + 1, y + 0);
            flood_fill_team(x + 0, y + 1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_territory_map_edit(long, short hittype, C_Base *control)
{
    long
    width,
    height,
    px,
    py,
    lx,
    ly,
    x,
    y;

    uchar
    cover,
    *src;

    if (hittype == C_TYPE_RMOUSEDOWN)
    {
        save_territory_editor();
        OwnershipChanged = 1;

        x = control->GetRelX();
        y = control->GetRelY();

        y = TheCampaign.TheaterSizeY / MAP_RATIO - y;

        x -= 3;
        y -= 4;

        width = TheCampaign.TheaterSizeX / MAP_RATIO;
        height = TheCampaign.TheaterSizeY / MAP_RATIO;

        src = TheCampaign.CampMapData;

        if (src)
        {
            fill_width = width;
            fill_height = height;
            fill_src = src;

            if (x bitand 1)
            {
                fill_team = (fill_src[(y * fill_width + x) / 2] bitand 0xf0) >> 4;
            }
            else
            {
                fill_team = fill_src[(y * fill_width + x) / 2] bitand 0x0f;
            }

            flood_fill_team(x, y);

            MakeOccupationMap(gOccupationMap);
            control->Refresh();
        }
    }
    else if ((hittype == C_TYPE_LMOUSEDOWN) or (hittype == C_TYPE_DRAGXY))
    {
        if (hittype == C_TYPE_LMOUSEDOWN)
            save_territory_editor();

        OwnershipChanged = 1;

        x = control->GetRelX();
        y = control->GetRelY();

        y = TheCampaign.TheaterSizeY / MAP_RATIO - y;

        x -= 3;
        y -= 4;

        width = TheCampaign.TheaterSizeX / MAP_RATIO;
        height = TheCampaign.TheaterSizeY / MAP_RATIO;

        src = TheCampaign.CampMapData;

        if (src)
        {
            for (lx = 0; lx < MAP_RATIO; lx ++)
            {
                px = x + lx;

                for (ly = 0; ly < MAP_RATIO; ly ++)
                {
                    py = y + ly;

                    if ((px >= 0) and (px < width) and (py >= 0) and (py < height))
                    {
                        cover = static_cast<uchar>(get_group_cover(px * MAP_RATIO, py * MAP_RATIO));

                        if (cover not_eq Water)
                        {
                            // Update the ownership for all objectives/squadrons/battalions in the area


                            if (px bitand 1)
                            {
                                src[(py * width + px) / 2] and_eq 0x0f;
                                src[(py * width + px) / 2] or_eq gDrawTeam << 4;
                            }
                            else
                            {
                                src[(py * width + px) / 2] and_eq 0xf0;
                                src[(py * width + px) / 2] or_eq gDrawTeam;
                            }
                        }
                    }
                }
            }

            MakeOccupationMap(gOccupationMap);
            control->Refresh();
        }
    }
    else if (hittype == C_TYPE_LMOUSEUP)
    {
        OwnershipChanged = 1;
        MakeOccupationMap(gOccupationMap);
        control->Refresh();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetupOccupationMap(void)
{
    C_Window *win;
    C_Button *but;
    C_Bitmap *bmp;

    if (gOccupationMap == NULL and (TheCampaign.TheaterSizeX and TheCampaign.TheaterSizeY and TheCampaign.CampMapData))
    {
        // Create Occupation map...
        gOccupationMap = CreateOccupationMap(1, TheCampaign.TheaterSizeX / MAP_RATIO, TheCampaign.TheaterSizeY / MAP_RATIO, 16);
    }

    if (gOccupationMap)
        MakeOccupationMap(gOccupationMap);

    win = gMainHandler->FindWindow(TAC_TEAM_WIN);

    if (win not_eq NULL)
    {
        but = (C_Button *)win->FindControl(TAC_MAP_TE);

        if (but)
        {
            but->SetImage(0, gOccupationMap);
            but->SetSubParents(win);
            but->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_MISSION_SUA);

    if (win)
    {
        bmp = (C_Bitmap*)win->FindControl(TAC_OVERLAY);

        if (bmp)
        {
            bmp->SetImage(gOccupationMap);
            bmp->SetSubParents(win);
            bmp->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_SUA_WIN);

    if (win)
    {
        bmp = (C_Bitmap*)win->FindControl(TAC_OVERLAY);

        if (bmp)
        {
            bmp->SetImage(gOccupationMap);
            bmp->SetSubParents(win);
            bmp->Refresh();
        }
    }
}

void UpdateOccupationMap(void)
{
    C_Window *win;
    C_Button *but;
    C_Bitmap *bmp;

    if ( not gOccupationMap)
        return;

    MakeOccupationMap(gOccupationMap);

    win = gMainHandler->FindWindow(TAC_TEAM_WIN);

    if (win not_eq NULL)
    {
        but = (C_Button *)win->FindControl(TAC_MAP_TE);

        if (but)
        {
            but->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_MISSION_SUA);

    if (win)
    {
        bmp = (C_Bitmap*)win->FindControl(TAC_OVERLAY);

        if (bmp)
        {
            bmp->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void save_territory_editor(void)
{
    if (TheCampaign.CampMapData)
    {
        if ( not te_restore_map)
            te_restore_map = new uchar[TheCampaign.CampMapSize];

        if (te_restore_map)
        {
            memcpy(te_restore_map, TheCampaign.CampMapData, TheCampaign.CampMapSize);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tryx[20] = { 1, -1, 0, 0, 1, 1, -1, -1, 2, -2, 0, 0, 2, 2, -2, -2, 1, -1, 1, -1 };
int tryy[20] = { 0, 0, 1, -1, 1, -1, 1, -1, 0, 0, 2, -2, 1, -1, 1, -1, 2, 2, -2, -2 };

short GetMapTeam(short x, short y)
{
    int bit, indx;
    uchar pixel;
    int width;

    if ( not TheCampaign.CampMapData)
        return(0);

    width = MRX >> 1;
    x = static_cast<short>(x / MAP_RATIO);
    y = static_cast<short>(y / MAP_RATIO);
    bit = x bitand 1;
    x >>= 1;
    pixel = TheCampaign.CampMapData[y * width + x];
    pixel = static_cast<uchar>((pixel >>(bit * 4)) bitand 0x0f);

    if (pixel > 0 and pixel < NUM_TEAMS)
        return(pixel);

    // KCK: Added this to try and avoid accidental painting of team 0 stuff
    // Basically, I'm going to look around until I find a non-water area
    for (int i = 0; i < 20; i++)
    {
        indx = (y + tryy[i]) * width + x + tryx[i];

        if (indx >= 0 and indx < TheCampaign.CampMapSize)
        {
            pixel = TheCampaign.CampMapData[indx];
            pixel = static_cast<uchar>((pixel >>(bit * 4)) bitand 0x0f);

            if (pixel > 0 and pixel < NUM_TEAMS)
                return(pixel);
        }
    }

    return(0);
}

void UpdateObjectiveOwnership()
{
    VuListIterator myit(AllObjList);
    Objective obj;
    GridIndex   x, y;
    Team team;

    obj = (Objective) myit.GetFirst();

    while (obj not_eq NULL)
    {
        obj->GetLocation(&x, &y);

        team = static_cast<uchar>(GetMapTeam(x, y));

        if ((team < NUM_TEAMS) and (TeamInfo[team]))
        {
            if (obj->GetOwner() not_eq team)
                obj->SetOwner(team);

            if (obj->GetObjectiveOldown() not_eq team)
                obj->SetObjectiveOldown(team);
        }
        else
        {
            if (obj->GetOwner() not_eq 0)
                obj->SetOwner(0);

            if (obj->GetObjectiveOldown() not_eq 0)
                obj->SetObjectiveOldown(0);
        }

        obj = (Objective) myit.GetNext();
    }

    // 2001-10-31 M.N. rebuild FLOTlist for Distance calculation
    RebuildFrontList(TRUE, FALSE);
    RebuildFLOTList();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UpdateUnitOwnership(void)
{
#if 0
    int team;
    short unitX, unitY;
    VuListIterator myit(AllUnitList);
    UnitClass* theUnit;


    theUnit = GetFirstUnit(&myit);

    while (theUnit not_eq NULL)
    {
        theUnit->GetLocation(&unitX, &unitY);
        team = GetOwner(TheCampaign.CampMapData, unitX, unitY);
        ShiAssert(team < NUM_TEAMS and TeamInfo[team]);
        theUnit->SetOwner(team);

        theUnit = GetNextUnit(&myit);
    }

#endif
}

void UpdateOwners()
{
    if (OwnershipChanged)
    {
        UpdateObjectiveOwnership();
        UpdateUnitOwnership();
        gGps->Update();
        OwnershipChanged = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
