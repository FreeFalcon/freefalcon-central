/***************************************************************************\
    Cmap.cpp
    Peter Ward
 July 15, 1996

    This code handles drawing the Campaign Map with/without units (user selectable)
\***************************************************************************/
#include <ciso646>
#include <windows.h>
#include <math.h>
#include "unit.h"
#include "team.h"
#include "cmpglobl.h"
#include "campcell.h"
#include "campterr.h"
#include "find.h"
#include "division.h"
#include "flight.h"
#include "campwp.h"
#include "cmpclass.h"
#include "campstr.h"
#include "listadt.h"
#include "objectiv.h"
#include "campaign.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "filters.h"
#include "cmap.h" // header file for this class
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "falcsess.h"
#include "gps.h"
#include "urefresh.h"
#include "battalion.h"
#include "tmap.h" // Artscout - 2026: terrain-derived campaign map
#include "fflog.h" // Artscout - 2026: producer census diagnostic
#include "tlevel.h"
#include "tdskpost.h"
#include "ttypes.h"
#include "camplist.h" // Artscout - 2026: AllObjList, for the campaign overlays
#include "find.h"

enum
{
    PLANNER_RESOURCE = 200110,
};

extern VU_ID gSelectedPackage, gActiveFlightID;
;
extern int gMoveBattalion;

extern GlobalPositioningSystem *gGps;

extern C_Handler *gMainHandler;
void WaypointCB(long ID, short hittype, C_Base *ctrl);
void UnitCB(long ID, short hittype, C_Base *ctrl);
int IsValidWP(WayPointClass *wp, Flight flt);
void Uni_Float(_TCHAR *buffer);

// Artscout - 2026: see s_mapFeetPerPixel below -- this was a fixed FEET_PER_KM / 2.0f,
// baking the shipped bitmap's 2-px-per-km scale into every icon coordinate.
#define FEET_PER_PIXEL (s_mapFeetPerPixel)

// Artscout - 2026: feet of theater per map-image pixel. Was a fixed
// FEET_PER_KM / 2.0f, which silently assumed the shipped 2-px-per-km bitmap --
// generating a finer map means the whole icon coordinate system has to follow it or
// every unit lands at a fraction of its correct position. Set once when the image is
// built; the stock value is the old constant, so nothing moves if the build is off.
static float s_mapFeetPerPixel = FEET_PER_KM / 2.0f;

// Pixels per km, for the few places that need the ratio the other way up (the threat
// rings scale their radii by it).
static float MapPixelsPerKm()
{
    return (s_mapFeetPerPixel > 0.0f) ? (FEET_PER_KM / s_mapFeetPerPixel) : 2.0f;
}

#define ICON_UKN 10126 // 2002-02-21 S.G.
extern int gShowUnknown; // 2002-02-21 S.G.
extern bool g_bAWACSBackground; // 2002-03-10 MN

C_Map::C_Map()
{
    CenterX_ = 0;
    CenterY_ = 0;
    ZoomLevel_ = _MAX_ZOOM_LEVEL_;
    scale_ = 1.0f;
    flags_ = I_NEED_TO_DRAW bitor I_NEED_TO_DRAW_MAP;

    MapID = 0;
    Map_ = NULL;
    memset(&MapRect_, 0, sizeof(UI95_RECT));

    memset(&TeamFlags_[0], 0, sizeof(long) * _MAX_TEAMS_);
    memset(&Team_[0], 0, sizeof(MAPICONS) * _MAX_TEAMS_);
    memset(&TeamColor_[0], 0, sizeof(COLORREF) * _MAX_TEAMS_);
    CurWP_ = NULL;
    CurWPZ_ = NULL;
    WPUnitID_ = FalconNullId;
    CurIcons_ = NULL;
    CurWPArea_.left = -1;
    CurWPArea_.top = -1;
    CurWPArea_.right = -1;
    CurWPArea_.bottom = -1;

    BullsEye_ = NULL;
    Circles_ = 0;

    ObjectiveMask_ = 0;
    UnitMask_ = 0;
    NavalUnitMask_ = 0;
    AirUnitMask_ = 0;
    ThreatMask_ = 0;
    CampOverlay_ = CAMP_OVERLAY_OFF;

    SmallMapCtrl_ = NULL;
    DrawWindow_ = NULL;
    WPZWindow_ = NULL;
    memset(&DrawRect_, 0, sizeof(UI95_RECT));
}

C_Map::~C_Map()
{
    if (Map_ or DrawWindow_ or SmallMapCtrl_)
        Cleanup();
}

void C_Map::Cleanup()
{
    short i, j, k;

    if (DrawWindow_)
    {
        RemoveListsFromWindow();
        DrawWindow_ = NULL;
    }

    if (WPZWindow_)
        WPZWindow_ = NULL;

    if (Map_)
    {
        Map_->Cleanup();
        delete Map_;
        Map_ = NULL;
    }

    if (BullsEye_)
    {
        BullsEye_->Cleanup();
        delete BullsEye_;
        BullsEye_ = NULL;
    }

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (Team_[i].Threats->Type[j])
                {
                    Team_[i].Threats->Type[j]->Cleanup();
                    delete Team_[i].Threats->Type[j];
                }

            delete Team_[i].Threats;
            Team_[i].Threats = NULL;
        }

        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Type[j])
                {
                    Team_[i].Objectives->Type[j]->Cleanup();
                    delete Team_[i].Objectives->Type[j];
                }

            delete Team_[i].Objectives;
            Team_[i].Objectives = NULL;
        }

        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Type[j])
                {
                    Team_[i].NavalUnits->Type[j]->Cleanup();
                    delete Team_[i].NavalUnits->Type[j];
                }

            delete Team_[i].NavalUnits;
            Team_[i].NavalUnits = NULL;
        }

        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                {
                    if (Team_[i].Units->Type[j])
                    {
                        for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                            if (Team_[i].Units->Type[j]->Levels[k])
                            {
                                Team_[i].Units->Type[j]->Levels[k]->Cleanup();
                                delete Team_[i].Units->Type[j]->Levels[k];
                            }

                        delete Team_[i].Units->Type[j];
                    }
                }

            delete Team_[i].Units;
            Team_[i].Units = NULL;
        }

        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Type[j])
                {
                    Team_[i].AirUnits->Type[j]->Cleanup();
                    delete Team_[i].AirUnits->Type[j];
                }

            delete Team_[i].AirUnits;
            Team_[i].AirUnits = NULL;
        }

        if (Team_[i].Waypoints)
        {
            Team_[i].Waypoints->Cleanup();
            delete Team_[i].Waypoints;
            Team_[i].Waypoints = NULL;
        }
    }

    if (CurIcons_)
    {
        CurIcons_->Cleanup();
        delete CurIcons_;
        CurIcons_ = NULL;
    }

    if (CurWP_)
    {
        CurWP_->Cleanup();
        delete CurWP_;
        CurWP_ = NULL;
    }

    if (CurWPZ_)
    {
        CurWPZ_->Cleanup();
        delete CurWPZ_;
        CurWPZ_ = NULL;
    }

    SmallMapCtrl_ = NULL;
}

void C_Map::RemoveAllEntities()
{
    short i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (Team_[i].Threats->Type[j])
                {
                    Team_[i].Threats->Type[j]->Cleanup();
                }
        }

        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Type[j])
                {
                    Team_[i].Objectives->Type[j]->Cleanup();
                }
        }

        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Type[j])
                {
                    Team_[i].NavalUnits->Type[j]->Cleanup();
                }
        }

        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                {
                    if (Team_[i].Units->Type[j])
                    {
                        for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                            if (Team_[i].Units->Type[j]->Levels[k])
                            {
                                Team_[i].Units->Type[j]->Levels[k]->Cleanup();
                            }
                    }
                }
        }

        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Type[j])
                {
                    Team_[i].AirUnits->Type[j]->Cleanup();
                }
        }

        if (Team_[i].Waypoints)
        {
            Team_[i].Waypoints->Cleanup();
        }
    }

    if (CurIcons_)
    {
        CurIcons_->Cleanup();
    }

    if (CurWP_)
    {
        CurWP_->Cleanup();
    }

    if (CurWPZ_)
    {
        CurWPZ_->Cleanup();
    }
}

// Private
void C_Map::CalculateDrawingParams()
{
    float ratio;
    long pixels;

    if (Map_ == NULL or DrawWindow_ == NULL)
        return;

    ratio = (float)(DrawRect_.bottom - DrawRect_.top) /
            (float)(DrawRect_.right - DrawRect_.left);
    pixels = ZoomLevel_;

    MapRect_.left = FloatToInt32(CenterX_) - pixels / 2;

    if (MapRect_.left < 0)
    {
        MapRect_.left = 0;
        CenterX_ = static_cast<float>(MapRect_.left + pixels / 2);
    }

    MapRect_.right = MapRect_.left + pixels;

    if (MapRect_.right > Map_->GetW())
    {
        MapRect_.right = Map_->GetW();
        MapRect_.left = MapRect_.right - pixels;
        CenterX_ = static_cast<float>(MapRect_.right - pixels / 2);
    }

    MapRect_.top = FloatToInt32(CenterY_) - (long)((float)pixels * ratio) / 2;

    if (MapRect_.top < 0)
    {
        MapRect_.top = 0;
        CenterY_ = static_cast<float>(MapRect_.top +
                                      (long)((float)pixels * ratio) / 2);
    }

    MapRect_.bottom = MapRect_.top + (long)((float)pixels * ratio);

    if (MapRect_.bottom > Map_->GetH())
    {
        MapRect_.bottom = Map_->GetH();
        MapRect_.top = MapRect_.bottom - (long)((float)pixels * ratio);
        CenterY_ = static_cast<float>(MapRect_.bottom -
                                      (long)((float)pixels * ratio) / 2);
    }

    Map_->SetSrcRect(&MapRect_);
    Map_->SetDestRect(&DrawRect_);
    Map_->SetScaleInfo(((MapRect_.right - MapRect_.left) * 1000) /
                       (DrawRect_.right - DrawRect_.left));

    scale_ = (float)(DrawRect_.right - DrawRect_.left) /
             ((float)(MapRect_.right - MapRect_.left) * FEET_PER_PIXEL);

    DrawWindow_->VX_[0] =
        -(short)((float)MapRect_.left * FEET_PER_PIXEL * scale_) +
        DrawWindow_->ClientArea_[0].left;
    DrawWindow_->VY_[0] =
        -(short)((float)MapRect_.top * FEET_PER_PIXEL * scale_) +
        DrawWindow_->ClientArea_[0].top;
    SetTeamScales();

    flags_ or_eq I_NEED_TO_DRAW bitor I_NEED_TO_DRAW_MAP;
    short x, y;
    TheCampaign.GetBullseyeLocation(&x, &y);

    if (x not_eq BullsEyeX_ or y not_eq BullsEyeY_)
    {
        SetBullsEye(x * FEET_PER_KM,
                    (TheCampaign.TheaterSizeY - y) * FEET_PER_KM);
        DrawMap();
    }
}

/***************************************************************************\
    Artscout - 2026: build the campaign map image out of the theater's OWN terrain.

    The shipped map is a painted bitmap at 2 pixels per km, and zooming in only
    magnifies those pixels. But the terrain database already holds a colour for every
    post -- TdiskPost::color, an index into TMap::ColorTable, the same pair otw.cpp
    uses to shade untextured ground -- so the map you fly over can be drawn from the
    ground you actually fly over, at whatever post spacing the theater ships.

    Three things make this fit unusually cleanly:

      * The palette is an exact match. This builds an 8-bit paletted IMAGE_RSC, and
        ColorTable is exactly 256 entries, so post colour indices become pixels
        verbatim and the table becomes the palette. No conversion, no quantisation.
      * Paletted is also what the overlay system needs: C_ScaleBitmap::PreparePalette
        derives its 16 blended palettes from the base image's palette, so the
        Logistics layers keep working. A truecolour map would break them.
      * The terrain is already open. TheMap.Setup runs from
        DeviceIndependentGraphicsSetup at startup, long before the campaign UI.

    Read straight from the files rather than through TLevel's streaming loader: this
    wants every block exactly once, not an async working set, and the format is
    simple. Theater.o<lod> is one 32-bit byte-offset per block, row-major over
    BlocksWide x BlocksHigh; at each offset in Theater.l<lod> sit POSTS_PER_BLOCK
    posts, row-major 16x16 (TBlock::Post), sized by g_LargeTerrainFormat.
\***************************************************************************/
extern char FalconTerrainDataDir[];
// Builds an empty 8-bit paletted IMAGE_RSC of any size (cpselect.cpp) -- the same helper
// the occupation map uses, reused here for the terrain image.
extern IMAGE_RSC *CreateOccupationMap(long ID, long w, long h, long palsize);

IMAGE_RSC *BuildTerrainMapImage(long ID, int lod)
{
    extern bool g_bCampMapFlipNS, g_bCampMapFlipEW;

    if (not TheMap.IsReady())
        return NULL;

    if (lod < 0)
        lod = 0;

    if (lod >= TheMap.NumLevels())
        lod = TheMap.NumLevels() - 1;

    TLevel *lv = TheMap.Level(lod);

    if (not lv)
        return NULL;

    const long bw = (long)lv->BlocksWide();
    const long bh = (long)lv->BlocksHigh();

    if (bw < 1 or bh < 1)
        return NULL;

    const long w = bw * POSTS_ACROSS_BLOCK;
    const long h = bh * POSTS_ACROSS_BLOCK;

    // Sanity bound. A theater this size would be ~256 MB of 8-bit image and something
    // has gone wrong with the header rather than us genuinely having that much ground.
    if (w < 16 or h < 16 or (double)w * (double)h > 2.5e8)
        return NULL;

    char base[MAX_PATH], fn[MAX_PATH];
    sprintf(base, "%s/terrain", FalconTerrainDataDir);

    // Block offsets.
    sprintf(fn, "%s/Theater.o%0d", base, lod);
    FILE *fo = fopen(fn, "rb");

    if (not fo)
        return NULL;

    const long nBlocks = bw * bh;
    DWORD *offs = new DWORD[nBlocks];

    if (not offs)
    {
        fclose(fo);
        return NULL;
    }

    const size_t gotOffs = fread(offs, sizeof(DWORD), (size_t)nBlocks, fo);
    fclose(fo);

    if (gotOffs not_eq (size_t)nBlocks)
    {
        delete[] offs;
        return NULL;
    }

    sprintf(fn, "%s/Theater.l%0d", base, lod);
    FILE *fl = fopen(fn, "rb");

    if (not fl)
    {
        delete[] offs;
        return NULL;
    }

    IMAGE_RSC *rsc = CreateOccupationMap(ID, w, h, 256);

    if (not rsc)
    {
        delete[] offs;
        fclose(fl);
        return NULL;
    }

    // Palette straight off the terrain's own table. Entry 0 stays black -- it is what
    // an unread block leaves behind, and black reads as "no data" rather than as some
    // arbitrary terrain colour smeared across a gap.
    WORD *pal = rsc->GetPalette();

    if (pal)
    {
        for (int i = 0; i < 256; i++)
        {
            const Tcolor &c = TheMap.ColorTable[i];
            long r = (long)(c.r * 255.0f), g = (long)(c.g * 255.0f),
                 b = (long)(c.b * 255.0f);
            r = (r < 0) ? 0 : (r > 255) ? 255 : r;
            g = (g < 0) ? 0 : (g > 255) ? 255 : g;
            b = (b < 0) ? 0 : (b > 255) ? 255 : b;
            pal[i] = UI95_RGB24Bit((r << 16) bitor (g << 8) bitor b);
        }
    }

    uchar *img = (uchar *)rsc->GetImage();

    if (not img)
    {
        delete[] offs;
        fclose(fl);
        return NULL;
    }

    const size_t postSize =
        g_LargeTerrainFormat ? sizeof(TNewdiskPost) : sizeof(TdiskPost);
    uchar *blockBuf = new uchar[postSize * POSTS_PER_BLOCK];

    if (not blockBuf)
    {
        delete[] offs;
        fclose(fl);
        return NULL;
    }

    for (long br = 0; br < bh; br++)
    {
        for (long bc = 0; bc < bw; bc++)
        {
            const DWORD off = offs[br * bw + bc];

            if (fseek(fl, (long)off, SEEK_SET) not_eq 0)
                continue;

            if (fread(blockBuf, postSize, POSTS_PER_BLOCK, fl) not_eq
                POSTS_PER_BLOCK)
                continue; // short read: leave the block black rather than guess

            for (int r = 0; r < POSTS_ACROSS_BLOCK; r++)
            {
                for (int c = 0; c < POSTS_ACROSS_BLOCK; c++)
                {
                    const uchar *p =
                        blockBuf + (size_t)(r * POSTS_ACROSS_BLOCK + c) * postSize;
                    // color sits after texID and z in both layouts; the only difference
                    // is texID's width (UInt16 vs UInt32).
                    const uchar col = g_LargeTerrainFormat ?
                                          ((const TNewdiskPost *)p)->color :
                                          ((const TdiskPost *)p)->color;

                    long px = bc * POSTS_ACROSS_BLOCK + c;
                    long py = br * POSTS_ACROSS_BLOCK + r;

                    // Orientation is the one thing here that cannot be settled by
                    // reading: which way the post grid runs against the map's
                    // north-up, east-right convention. Both axes are switchable so a
                    // mirrored theater is a config line, not a rebuild.
                    if (g_bCampMapFlipEW)
                        px = w - 1 - px;

                    if (g_bCampMapFlipNS)
                        py = h - 1 - py;

                    if (px >= 0 and px < w and py >= 0 and py < h)
                        img[py * w + px] = col;
                }
            }
        }
    }

    delete[] blockBuf;
    delete[] offs;
    fclose(fl);

    // The whole coordinate system keys off this. LEVEL_POST_TO_WORLD gives the feet
    // between posts at this LOD, and one post is now one pixel, so that IS the new
    // feet-per-pixel -- derived, never assumed, so a different LOD or a theater with
    // different post spacing stays correctly registered against the icons.
    s_mapFeetPerPixel = LEVEL_POST_TO_WORLD(1, lod);

    return rsc;
}

THREAT_LIST *C_Map::AddThreat(CampEntity ent)
{
    THREAT_LIST *threat;
    long radar_short;
    long radar_long;
    long sam_short;
    long sam_long;
    short i;
    GridIndex x, y;

    threat = NULL;

    if (ent->IsObjective() and ent->IsEmitting())
    {
        ent->GetLocation(&x, &y);
        ShiAssert(Map_Max_Y > 0);
        y = static_cast<short>(Map_Max_Y - y);
        ShiAssert(y >= 0);

        radar_short = ent->GetElectronicDetectionRange(LowAir);
        radar_long = ent->GetElectronicDetectionRange(Air);

        if (radar_short or radar_long)
        {
#ifdef USE_SH_POOLS
            threat = (THREAT_LIST *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL],
                                                sizeof(THREAT_LIST), FALSE);
#else
            threat = new THREAT_LIST;
#endif
            memset(threat, 0, sizeof(THREAT_LIST));

            if (Team_[ent->GetTeam()].Threats)
            {
                if (Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_])
                {
                    Team_[ent->GetTeam()]
                        .Threats->Type[_THREAT_RADAR_HIGH_]
                        ->AddCircle(ent->GetCampID(), C_Threat::THR_CIRCLE, x,
                                    y, radar_long);
                    threat->RadarHigh = Team_[ent->GetTeam()]
                                            .Threats->Type[_THREAT_RADAR_HIGH_]
                                            ->GetThreat(ent->GetCampID());
                }

                if (Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_] and
                    radar_short)
                {
                    Team_[ent->GetTeam()]
                        .Threats->Type[_THREAT_RADAR_LOW_]
                        ->AddCircle(ent->GetCampID(), C_Threat::THR_SLICE, x, y,
                                    radar_short);

                    for (i = 0; i < 8; i++)
                        // 2001-03-14 MODIFIED BY S.G. SO IF THERE IS NO RADAR RANGE DATA, THE radar_short VALUE IS USED INSTEAD
                        // Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->SetRadius(ent->GetCampID(),i,static_cast<long>(min(ent->GetArcRange(i)*FT_TO_KM,radar_short)));
                        Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_RADAR_LOW_]
                            ->SetRadius(
                                ent->GetCampID(), i,
                                static_cast<long>(
                                    ((ObjectiveClass *)ent)->HasRadarRanges() ?
                                        (min(ent->GetArcRange(i) * FT_TO_KM,
                                             radar_short)) :
                                        radar_short));

                    threat->RadarLow = Team_[ent->GetTeam()]
                                           .Threats->Type[_THREAT_RADAR_LOW_]
                                           ->GetThreat(ent->GetCampID());
                }
            }

            delete threat; // JPO - no idea what this threat variable is for
        }
    }

    // 2001-05-08 MODIFIED BY S.G. UNITS CAN STILL FIRE AT YOU, EVEN IF NOT EMITING THEMSELF SO RESERVE THE 'IsEmitting' FOR THREAT_RADAR_* CODE
    // if(ent->IsUnit() and ent->IsEmitting())
    if (ent->IsUnit())
    // THIS IS WHAT I DO IN 1.08i2 BUT NOT REQUIRED IN 1.07 (SEE AT END OF FUNCTION FOR DETAIL)
    // if(ent->IsUnit() and not ((Unit)ent)->Inactive() and (FindUnitType(ent) bitand (_UNIT_AIR_DEFENSE bitor _UNIT_BATTALION)))
    // UI_Refresher *gpsItem=NULL;
    // if(ent->IsUnit() and (gpsItem=(UI_Refresher*)gGps->Find(ent->GetCampID())) and gpsItem->MapItem_ and not (gpsItem->MapItem_->Flags bitand C_BIT_INVISIBLE))
    {
        ent->GetLocation(&x, &y);
        ShiAssert(Map_Max_Y > 0);
        y = static_cast<short>(Map_Max_Y - y);
        ShiAssert(y >= 0);
        radar_short = ent->GetElectronicDetectionRange(LowAir);
        radar_long = ent->GetElectronicDetectionRange(Air);
        sam_short = ent->GetAproxWeaponRange(LowAir);
        sam_long = ent->GetAproxWeaponRange(Air);

        if (radar_short or radar_long or sam_long or sam_short)
        {
            // 2001-06-22 ADDED BY S.G. IF EMITTING, DISPLAY THE RADAR THREATS
            if (((BattalionClass *)ent)->class_data->RadarVehicle < 16)
            {
                if (not radar_short)
                    sam_short /= 128;

                if (not radar_long)
                    sam_long /= 128;
            }

            // END OF ADDED SECTION

#ifdef USE_SH_POOLS
            threat = (THREAT_LIST *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL],
                                                sizeof(THREAT_LIST), FALSE);
#else
            threat = new THREAT_LIST;
#endif
            memset(threat, 0, sizeof(THREAT_LIST));

            if (Team_[ent->GetTeam()].Threats)
            {
                // 2001-05-08 ADDED BY S.G. IF EMITTING, DISPLAY THE RADAR THREATS
                if (ent->IsEmitting())
                {
                    // END OF ADDED SECTION (EXCEPT FOR BLOCK INDENT)
                    if (Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_RADAR_HIGH_] and
                        radar_long)
                    {
                        Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_RADAR_HIGH_]
                            ->AddCircle(ent->GetCampID(), C_Threat::THR_CIRCLE,
                                        x, y, radar_long);
                        threat->RadarHigh =
                            Team_[ent->GetTeam()]
                                .Threats->Type[_THREAT_RADAR_HIGH_]
                                ->GetThreat(ent->GetCampID());
                    }

                    if (Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_RADAR_LOW_] and
                        radar_short)
                    {
                        Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_RADAR_LOW_]
                            ->AddCircle(ent->GetCampID(), C_Threat::THR_SLICE,
                                        x, y, radar_short);

                        for (i = 0; i < 8; i++)
                            Team_[ent->GetTeam()]
                                .Threats->Type[_THREAT_RADAR_LOW_]
                                ->SetRadius(ent->GetCampID(), i,
                                            static_cast<short>(min(
                                                ent->GetArcRange(i) * FT_TO_KM,
                                                radar_short)));

                        threat->RadarLow =
                            Team_[ent->GetTeam()]
                                .Threats->Type[_THREAT_RADAR_LOW_]
                                ->GetThreat(ent->GetCampID());
                    }

                    if (Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_SAM_HIGH_] and
                        sam_long)
                    {
                        Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_SAM_HIGH_]
                            ->AddCircle(ent->GetCampID(), C_Threat::THR_CIRCLE,
                                        x, y, FTOL(sam_long / .539f));
                        threat->SamHigh = Team_[ent->GetTeam()]
                                              .Threats->Type[_THREAT_SAM_HIGH_]
                                              ->GetThreat(ent->GetCampID());
                    }

                    if (Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_SAM_LOW_] and
                        sam_short)
                    {
                        Team_[ent->GetTeam()]
                            .Threats->Type[_THREAT_SAM_LOW_]
                            ->AddCircle(ent->GetCampID(), C_Threat::THR_SLICE,
                                        x, y, sam_short);

                        for (i = 0; i < 8; i++)
                            Team_[ent->GetTeam()]
                                .Threats->Type[_THREAT_SAM_LOW_]
                                ->SetRadius(ent->GetCampID(), i,
                                            static_cast<short>(min(
                                                ent->GetArcRange(i) * FT_TO_KM,
                                                sam_short)));

                        threat->SamLow = Team_[ent->GetTeam()]
                                             .Threats->Type[_THREAT_SAM_LOW_]
                                             ->GetThreat(ent->GetCampID());
                    }
                }
            }

            delete threat; // JPO - no idea what this threat variable is for S.G. I DO BUT I HAVE NO TIME TO FIX IT NOW. I'LL MAKE IT RP5 COMPATIBLE FIRST
        }
    }

    // 2001-05-09 MODIFIED BY S.G. WHY NOT RETURNING THAT STRUCTURE WE FILLED UP? CAN WE SAY 'MEMORY LEAK' HERE? PLUS WITHOUT THIS, THREAT CIRCLES ARE APPEARING FOR ANY UNITS WITH A 'Range' AGAINST 'Air' MOVEMENT TYPE...
    return (NULL);
    // THIS IS DIFFERENT THAN WHAT I DO IN 1.08i2 AND IS THE PREFERED WAY. I CAN'T DO IT IN 1.08i2 BEFORE TOO MUCH CODE WAS OPTOMIZED OUT BY THE COMPILER :-(
    // return (threat);
}


MAPICONLIST *C_Map::AddObjective(Objective Obj)
{
    long ObjType, TypeID;
    short numarcs;
    float radar_short, radar_long;
    ObjClassDataType *ObjPtr;
    _TCHAR Buffer[40];
    DETECTOR *detect;

    ObjType = GetObjectiveType(Obj);
    TypeID = FindTypeIndex(ObjType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        if (ObjType)
        {
            ObjPtr = Obj->GetObjectiveClassData();

            if (ObjPtr)
            {
                Obj->GetName(Buffer, 39, TRUE);
                detect = NULL;
                radar_short = static_cast<float>(
                    Obj->GetElectronicDetectionRange(LowAir));
                radar_long =
                    static_cast<float>(Obj->GetElectronicDetectionRange(Air));

                if (radar_short or radar_long)
                {
#ifdef USE_SH_POOLS
                    detect = (DETECTOR *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL],
                                                     sizeof(DETECTOR), FALSE);
#else
                    detect = new DETECTOR;
#endif
                    detect->HighSam = 0.0f;
                    detect->HighRadar = radar_long * KM_TO_FT;
                    detect->LowSam = 0.0f;

                    if (radar_short)
                    {
                        numarcs = static_cast<short>(Obj->GetNumberOfArcs());
                        numarcs = 1; // TEMP kludge
#ifdef USE_SH_POOLS
                        detect->LowRadar = (ARC_LIST *)MemAllocPtr(
                            UI_Pools[UI_GENERAL_POOL], sizeof(ARC_LIST), FALSE);
#else
                        detect->LowRadar = new ARC_LIST;
#endif
                        detect->LowRadar->numarcs = numarcs;
#ifdef USE_SH_POOLS
                        detect->LowRadar->arcs = (ARC_REC *)MemAllocPtr(
                            UI_Pools[UI_GENERAL_POOL],
                            sizeof(ARC_REC) * numarcs, FALSE);
#else
                        detect->LowRadar->arcs = new ARC_REC[numarcs];
#endif
                        // after getting rid of kludges...
                        // figure out how to do arcs right
                        // kludge
                        detect->LowRadar->arcs[0].arc = 0;
                        detect->LowRadar->arcs[0].range = radar_short;
                    }
                    else
                        detect->LowRadar = NULL;
                }

                return (Team_[Obj->GetTeam()]
                            .Objectives->Type[TypeID]
                            ->AddIconToList(Obj->GetCampID(),
                                            static_cast<short>(ObjType),
                                            ObjPtr->IconIndex, Obj->YPos(),
                                            maxy - Obj->XPos(), FALSE, Buffer,
                                            0, 0, 0, 0, 0, detect));
            }
        }
    }

    return (NULL);
}

MAPICONLIST *C_Map::AddDivision(Division div)
{
    UnitClassDataType *UnitPtr;
    Unit u;
    MAPICONLIST *cur;
    GridIndex x, y;
    long UnitType, Type;
    long totalstr, curstr, perc;
    _TCHAR Buffer[40];

    u = div->GetFirstUnitElement();

    if (u)
    {
        UnitType = FindDivisionType(div->GetDivisionType()) bitand 0xffffff;
        Type = FindTypeIndex(UnitType bitand 0x0fff, GND_TypeList,
                             _MAP_NUM_GND_TYPES_);

        // Figure out Status
        curstr = u->GetTotalVehicles();
        totalstr = u->GetFullstrengthVehicles();

        if (totalstr < 1)
            totalstr = 1;

        perc = (curstr * 100) / totalstr;

        if (perc > 100)
            perc = 100;

        cur = Team_[u->GetTeam()].Units->Type[Type]->Levels[0]->FindID(
            UR_DIVISION bitor div->nid);

        if (cur == NULL)
        {
            div->GetName(Buffer, 39, FALSE);
            UnitPtr = u->GetUnitClassData();
            div->GetLocation(&x, &y);

            return (
                Team_[u->GetTeam()].Units->Type[Type]->Levels[0]->AddIconToList(
                    UR_DIVISION bitor div->nid,
                    static_cast<short>(Type bitor _UNIT_DIVISION),
                    UnitPtr->IconIndex, x * FEET_PER_KM, maxy - y * FEET_PER_KM,
                    FALSE, Buffer, div->nid, 0, 0, (long)perc, 0));
        }
    }

    return (NULL);
}

MAPICONLIST *C_Map::AddUnit(Unit u)
{
    long UnitType, TypeID, LevelID = -1, brigid, batid, numarcs; //
    Unit upar;
    UnitClassDataType *UnitPtr;
    float totalstr, curstr, perc;
    DETECTOR *detect;
    float sam_short;
    float sam_long;
    float radar_short;
    float radar_long;
    _TCHAR Buffer[60];

    UnitType = FindUnitType(u);
    TypeID = -1;

    // Figure out Status
    curstr = (float)u->GetTotalVehicles();
    totalstr = (float)u->GetFullstrengthVehicles();

    if (totalstr < 1)
        totalstr = 1;

    perc = (curstr / totalstr) * 100.0f;

    if (perc > 100.0f)
        perc = 100.0f;

    if (UnitType bitand _UNIT_GROUND_MASK)
    {
        TypeID = FindTypeIndex(UnitType bitand 0x0fff, GND_TypeList,
                               _MAP_NUM_GND_TYPES_);

        if (TypeID not_eq -1)
            LevelID = FindTypeIndex(UnitType bitand _UNIT_GROUND_MASK,
                                    GND_LevelList, _MAP_NUM_GND_LEVELS_);
        else
            LevelID = -1;
    }
    else if (UnitType bitand _UNIT_NAVAL_MASK)
    {
        TypeID = FindTypeIndex(UnitType bitand 0x0fff, NAV_TypeList,
                               _MAP_NUM_NAV_TYPES_);
        LevelID = 1;
    }

    if (TypeID not_eq -1 and LevelID not_eq -1)
    {
        UnitPtr = u->GetUnitClassData();

        if (UnitPtr)
        {
            if (UnitType bitand _UNIT_GROUND_MASK)
            {
                if (LevelID == 1)
                {
                    brigid = u->GetUnitNameID();
                    batid = 0;
                }
                else if (LevelID == 2)
                {
                    upar = u->GetUnitParent();

                    if (upar)
                        brigid = upar->GetUnitNameID();
                    else
                        brigid = 0;

                    batid = u->GetUnitNameID();
                }
                else
                {
                    brigid = 0;
                    batid = 0;
                }

                detect = NULL;

                if (u->IsBattalion())
                {
                    radar_short = static_cast<float>(
                        u->GetElectronicDetectionRange(LowAir));
                    radar_long =
                        static_cast<float>(u->GetElectronicDetectionRange(Air));
                    sam_short =
                        static_cast<float>(u->GetAproxWeaponRange(LowAir));
                    sam_long = static_cast<float>(u->GetAproxWeaponRange(Air));

                    if (radar_short or radar_long or sam_short or sam_long)
                    {
#ifdef USE_SH_POOLS
                        detect = (DETECTOR *)MemAllocPtr(
                            UI_Pools[UI_GENERAL_POOL], sizeof(DETECTOR), FALSE);
#else
                        detect = new DETECTOR;
#endif
                        detect->HighSam = sam_long * KM_TO_FT;
                        detect->HighRadar = radar_long * KM_TO_FT;
                        detect->LowSam = sam_short * KM_TO_FT;

                        if (radar_short)
                        {
                            numarcs = u->GetNumberOfArcs();
                            numarcs = 1; // TEMP kludge

#ifdef USE_SH_POOLS
                            detect->LowRadar = (ARC_LIST *)MemAllocPtr(
                                UI_Pools[UI_GENERAL_POOL], sizeof(ARC_LIST),
                                FALSE);
#else
                            detect->LowRadar = new ARC_LIST;
#endif
                            detect->LowRadar->numarcs =
                                static_cast<short>(numarcs);
#ifdef USE_SH_POOLS
                            detect->LowRadar->arcs = (ARC_REC *)MemAllocPtr(
                                UI_Pools[UI_GENERAL_POOL],
                                sizeof(ARC_REC) * numarcs, FALSE);
#else
                            detect->LowRadar->arcs = new ARC_REC[numarcs];
#endif

                            // after getting rid of kludges...
                            // figure out how to do arcs right
                            // kludge
                            detect->LowRadar->arcs[0].arc = 0;
                            detect->LowRadar->arcs[0].range = radar_short;
                        }
                        else
                            detect->LowRadar = NULL;
                    }
                }

                u->GetName(Buffer, 49, FALSE);
                return (Team_[u->GetTeam()]
                            .Units->Type[TypeID]
                            ->Levels[LevelID]
                            ->AddIconToList(
                                u->GetCampID(), static_cast<short>(UnitType),
                                UnitPtr->IconIndex, u->YPos(), maxy - u->XPos(),
                                static_cast<short>(gMoveBattalion), Buffer,
                                u->GetUnitDivision(), brigid, batid, (long)perc,
                                0, detect));
            }
            else if (UnitType bitand _UNIT_NAVAL_MASK)
            {
                u->GetName(Buffer, 49, FALSE);
                return (
                    Team_[u->GetTeam()].NavalUnits->Type[TypeID]->AddIconToList(
                        u->GetCampID(), static_cast<short>(UnitType),
                        UnitPtr->IconIndex, u->YPos(), maxy - u->XPos(),
                        static_cast<short>(gMoveBattalion), Buffer, (long)perc,
                        0));
            }
        }
    }

    return (NULL);
}

MAPICONLIST *C_Map::AddFlight(Flight flight)
{
    long idx;
    long TypeID;
    UnitClassDataType *UnitPtr;
    _TCHAR Buffer[40];

    UnitPtr = flight->GetUnitClassData();

    if (UnitPtr)
    {
        idx = GetAirIcon(flight->GetSType());
        TypeID = FindTypeIndex(AirIcons[idx].UIType, AIR_TypeList,
                               _MAP_NUM_AIR_TYPES_);

        if (TypeID not_eq -1)
        {
            if (UnitPtr->IconIndex)
            {
                if (gGps->GetTeamNo() == flight->GetTeam())
                {
                    GetCallsign(flight, Buffer);
                }
                else
                {
                    int vid = flight->GetVehicleID(0);
                    VehicleClassDataType *vc = GetVehicleClassData(vid);
                    strcpy(Buffer, vc ? vc->Name : "<unk>");
                }

                return (
                    Team_[flight->GetTeam()]
                        .AirUnits->Type[TypeID]
                        ->AddIconToList(
                            flight->GetCampID(),
                            static_cast<short>(AirIcons[idx].UIType),
                            UnitPtr->IconIndex, flight->YPos(),
                            maxy - flight->XPos(),
                            static_cast<short>(gMoveBattalion), Buffer, 0,
                            ((Flight)flight)->GetLastDirection() bitand 0x7));
            }
        }
    }

    return (NULL);
}

MAPICONLIST *C_Map::AddSquadron(Squadron squadron)
{
    long UnitType, TypeID;
    UnitClassDataType *UnitPtr;
    _TCHAR Buffer[40];

    UnitType = GetObjectiveType(squadron);
    TypeID = FindTypeIndex(UnitType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        if (UnitType)
        {
            UnitPtr = squadron->GetUnitClassData();

            if (UnitPtr)
            {
                squadron->GetName(Buffer, 39, FALSE);
                return (Team_[squadron->GetTeam()]
                            .Objectives->Type[TypeID]
                            ->AddIconToList(squadron->GetCampID(),
                                            static_cast<short>(UnitType), 10117,
                                            squadron->YPos(),
                                            maxy - squadron->XPos(), FALSE,
                                            Buffer, 0, 0, 0, 0, 0, NULL));
            }
        }
    }

    return (NULL);
}

MAPICONLIST *C_Map::AddPackage(Package package)
{
    long UnitType, TypeID;
    _TCHAR Buffer[32];

    UnitType = GetObjectiveType(package);
    TypeID = FindTypeIndex(UnitType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        if (UnitType)
        {
            _stprintf(Buffer, "%s %1d", gStringMgr->GetString(TXT_PACKAGE),
                      package->GetCampID());
            return (Team_[package->GetTeam()]
                        .Objectives->Type[TypeID]
                        ->AddIconToList(package->GetCampID(),
                                        static_cast<short>(UnitType), 10118,
                                        package->YPos(), maxy - package->XPos(),
                                        FALSE, Buffer, 0, 0, 0, 0, 0, NULL));
        }
    }

    return (NULL);
}

MAPICONLIST *C_Map::AddVC(victory_condition *vc)
{
    long TypeID;
    _TCHAR Buffer[32];
    CampEntity ent;

    ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

    if (ent)
    {
        if (not ent->IsUnit() and not ent->IsObjective())
            return (NULL);
    }
    else
        return (NULL);

    TypeID = FindTypeIndex(_VC_CONDITION_, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        _stprintf(Buffer, "%s %1d", gStringMgr->GetString(TXT_VC),
                  vc->get_number());
        return (Team_[vc->get_team()].Objectives->Type[TypeID]->AddIconToList(
            vc->get_number(), _VC_CONDITION_, 10119, ent->YPos(),
            maxy - ent->XPos(), FALSE, Buffer, 0, 0, 0, 0, 0, NULL));
    }

    return (NULL);
}

void C_Map::UpdateVC(victory_condition *vc)
{
    long TypeID;
    CampEntity ent;
    MAPICONLIST *vcicon;

    ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

    if (ent)
    {
        if (not ent->IsUnit() and not ent->IsObjective())
            ent = NULL;
    }

    TypeID = FindTypeIndex(_VC_CONDITION_, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        vcicon = Team_[vc->get_team()].Objectives->Type[TypeID]->FindID(
            vc->get_number());

        if (not ent)
        {
            Team_[vc->get_team()].Objectives->Type[TypeID]->RemoveIcon(
                vc->get_number());
        }
        else if (vcicon)
        {
            vcicon->worldx = ent->YPos();
            vcicon->worldy = maxy - ent->XPos();
            vcicon->x = static_cast<short>(vcicon->worldx * scale_);
            vcicon->y = static_cast<short>(vcicon->worldy * scale_);
        }
    }
}

void C_Map::RemoveVC(long team, long ID)
{
    long TypeID;
    F4CSECTIONHANDLE *Leave;

    TypeID = FindTypeIndex(_VC_CONDITION_, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        Leave = UI_Enter(DrawWindow_);
        Team_[team].Objectives->Type[TypeID]->RemoveIcon(ID);
        UI_Leave(Leave);
    }
}

void C_Map::BuildCurrentWPList(Unit unit)
{
    WayPoint wp = NULL, prevwp = NULL, firstwp = NULL;
    float x, y, z, tempx, tempy, tempz;
    long normID, selID, othrID;
    long i, numwp, xval;
    long starttime, endtime;
    long campID;
    long UseFlag, ZDrag;
    double distance;
    float lx, ly, lz, dx, dy, dz;
    WAYPOINTLIST *wpl = NULL;
    CampEntity target;
    short state;
    _TCHAR buf[40];
    VU_ID *tmpID = NULL;
    UI_Refresher *gpsItem = NULL;
    short airwps, lastwp;

    if (unit == NULL)
        return;

    if (unit->IsFlight() and not unit->Final())
        return;

    if (not CurWP_ or not CurWPZ_)
        return;

    airwps = static_cast<short>(unit->IsFlight());

    if (not airwps)
    {
        firstwp = unit->GetCurrentUnitWP();

        if (firstwp and firstwp->GetPrevWP())
            firstwp = firstwp->GetPrevWP();
    }

    if (not firstwp or airwps)
        firstwp = unit->GetFirstUnitWP();

    if (not firstwp)
        return;

    wp = firstwp;

    starttime = wp->GetWPDepartureTime();
    endtime =
        wp->GetWPDepartureTime(); //so endtime has a value if it doesn't get otherwise initialized
    campID = unit->GetCampID() << 8;

    // set to 0 after Landing WP so we don't connect lines to following waypoints which are not on the agenda (Alt Land cit,Tanker etc)
    UseFlag = C_BIT_USELINE;

    prevwp = NULL;
    numwp = 0;

    while (wp)
    {
        numwp++;
        wp = wp->GetNextWP();
    }

    if (airwps)
    {
        CurWP_->SetMenu(STEERPOINT_POP);
        CurWPZ_->SetMenu(STEERPOINT_POP);
    }
    else
    {
        CurWP_->SetMenu(0);
        CurWPZ_->SetMenu(0);
    }

    CurIcons_->Cleanup();
    CurIcons_->Setup(CurIcons_->GetID(), 0);

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        CurWP_->SetFlagBitOn(C_BIT_DRAGABLE);
        CurWPZ_->SetFlagBitOn(C_BIT_DRAGABLE);
    }
    else
    {
        if (firstwp == unit->GetCurrentUnitWP() or not airwps)
        {
            CurWP_->SetFlagBitOn(C_BIT_DRAGABLE);
            CurWPZ_->SetFlagBitOn(C_BIT_DRAGABLE);
        }
        else
        {
            CurWP_->SetFlagBitOff(C_BIT_DRAGABLE);
            CurWPZ_->SetFlagBitOff(C_BIT_DRAGABLE);
        }
    }

    i = 1;
    wp = firstwp;

    while (wp)
    {
        wp->GetLocation(&x, &y, &z);

        if (x < CurWPArea_.left or CurWPArea_.left < 0)
            CurWPArea_.left = static_cast<long>(x);

        if (x > CurWPArea_.right or CurWPArea_.right < 0)
            CurWPArea_.right = static_cast<long>(x);

        if (y < CurWPArea_.top or CurWPArea_.top < 0)
            CurWPArea_.top = static_cast<long>(y);

        if (y > CurWPArea_.bottom or CurWPArea_.bottom < 0)
            CurWPArea_.bottom = static_cast<long>(y);

        if (wp->GetWPFlags() bitand WPF_TARGET)
        {
            // Set 2d Waypoint
            wpl = CurWP_->AddWaypointToList(
                0x20000000 + campID + i, 0, ASSIGNED_TGT_CUR, ASSIGNED_TGT_CUR,
                ASSIGNED_TGT_CUR, y, maxy - x, FALSE);

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID + i);
                CurWP_->SetState(campID + i, 0);
                wpl->Flags and_eq compl C_BIT_ENABLED;
            }
        }
        else if ((wp->GetWPAction() == WP_TAKEOFF) or
                 (wp->GetWPAction() == WP_LAND))
        {
            // Set 2d Waypoint
            wpl = CurWP_->AddWaypointToList(0x20000000 + campID + i, 0,
                                            HOME_BASE_CUR, HOME_BASE_CUR,
                                            HOME_BASE_CUR, y, maxy - x, FALSE);

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID + i);
                CurWP_->SetState(campID + i, 0);
                wpl->Flags and_eq compl C_BIT_ENABLED;
            }
        }

        target = wp->GetWPTarget();

        if (target)
        {
            // use GPS to make visible
            gpsItem = (UI_Refresher *)gGps->Find(target->GetCampID());

            if (gpsItem and gpsItem->MapItem_)
                CurIcons_->Add(gpsItem->MapItem_);
        }

        i++;
        wp = wp->GetNextWP();
    }

    distance = 0.0f;
    wp = firstwp;
    wp->GetLocation(&lx, &ly, &lz);
    i = 1;
    lastwp = 0;

    while (wp)
    {
        if (not wp->GetNextWP())
            lastwp = 1;

        if (wp->GetWPFlags() bitand WPF_TARGET)
        {
            normID = TGT_CUR;
            selID = TGT_CUR_SEL;
            othrID = TGT_CUR_ERROR;
        }
        else if (wp->GetWPFlags() bitand WPF_IP)
        {
            normID = IP_CUR;
            selID = IP_CUR_SEL;
            othrID = IP_CUR_ERROR;
        }
        else
        {
            normID = STPT_CUR;
            selID = STPT_CUR_SEL;
            othrID = STPT_CUR_ERROR;
        }

        if (unit->IsFlight() and not IsValidWP(wp, (Flight)unit))
            state = 2;
        else if (wp == unit->GetCurrentUnitWP())
            state = 1;
        else
            state = 0;

        wp->GetLocation(
            &x, &y,
            &z); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z

        // Add Nub to insert a waypoint
        if (UseFlag and prevwp and airwps)
        {
            prevwp->GetLocation(
                &tempx, &tempy,
                &tempz); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z
            dx = x - tempx;
            dy = y - tempy;
            _stprintf(buf, "%1.1f", sqrt(dx * dx + dy * dy) * FT_TO_NM);
            Uni_Float(buf);
            dx *= .5;
            dy *= .5;
            wpl = CurWP_->AddWaypointToList(
                0x40000000 + campID + i, 0, ADDLINE_CUR, ADDLINE_CUR_SEL,
                ADDLINE_CUR, tempy + dy, maxy - (tempx + dx), TRUE);

            if (wpl)
            {
                CurWP_->SetWPGroup(0x40000000 + campID + i, campID);
                CurWP_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));
                CurWP_->SetLabel(0x40000000 + campID + i,
                                 gStringMgr->GetText(gStringMgr->AddText(buf)));
                CurWP_->SetTextOffset(0x40000000 + campID + i, 0, -15);
                CurWP_->SetState(0x40000000 + campID + i, state);
                CurWP_->SetLabelColor(0x40000000 + campID + i, 0x00ffffff,
                                      0x0000ffff, 0x000000ff);
                CurWP_->SetLineColor(0x40000000 + campID + i, 0x00ffffff,
                                     0x0000ffff, 0x000000ff);

                if (not(TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
                {
                    if (firstwp not_eq unit->GetCurrentUnitWP())
                    {
                        wpl->Icon->SetText(0, TXT_SPACE);
                        wpl->Icon->SetText(1, TXT_SPACE);
                        wpl->Icon->SetText(2, TXT_SPACE);
                        wpl->Icon->SetFlagBitOff(C_BIT_ENABLED);
                    }
                }

                wpl->Flags or_eq UseFlag;
            }
        }

        // Set 2d Waypoint
        if (UseFlag)
            _stprintf(buf, "%1d", i);
        else
        {
            if (wp->GetWPAction() == WP_LAND)
            {
                _sntprintf(buf, 39, "%s",
                           gStringMgr->GetString(TXT_ALTERNATE_FIELD));
                buf[39] = 0;
            }
            else if (wp->GetWPAction() == WP_REFUEL)
            {
                _sntprintf(buf, 39, "%s", gStringMgr->GetString(TXT_TANKER));
                buf[39] = 0;
            }
            else
                buf[0] = 0;
        }

        if (buf[0])
        {
            if (airwps)
            {
                if (i == 1)
                    wpl =
                        CurWP_->AddWaypointToList(campID + i, 0, normID, selID,
                                                  othrID, y, maxy - x, FALSE);
                else
                    wpl =
                        CurWP_->AddWaypointToList(campID + i, 0, normID, selID,
                                                  othrID, y, maxy - x, TRUE);
            }
            else
            {
                if (lastwp)
                    wpl =
                        CurWP_->AddWaypointToList(campID + i, 0, normID, selID,
                                                  othrID, y, maxy - x, TRUE);
                else
                    wpl = CurWP_->AddWaypointToList(campID + i, 0, 0, 0, 0, y,
                                                    maxy - x, FALSE);
            }

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID);
                CurWP_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));

                if (airwps)
                {
                    CurWP_->SetLabel(campID + i, gStringMgr->GetText(
                                                     gStringMgr->AddText(buf)));
                    CurWP_->SetTextOffset(campID + i, 0, -15);
                }

                CurWP_->SetState(campID + i, state);
                CurWP_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff,
                                      0x000000ff);
                CurWP_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff,
                                     0x000000ff);
                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Flags or_eq UseFlag;
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, i);
            }

            // Set Z Waypoint
            xval = (i - 1) * (650 / numwp) + 60;

            if (airwps)
            {
                if (wp->GetWPAction() == WP_LAND or
                    wp->GetWPAction() == WP_TAKEOFF)
                    ZDrag = FALSE;
                else
                    ZDrag = TRUE;

                wpl = CurWPZ_->AddWaypointToList(
                    static_cast<short>(campID + i), 0, normID, selID, othrID,
                    static_cast<float>(xval), z, static_cast<short>(ZDrag));

                if (wpl)
                {
                    CurWPZ_->SetWPGroup(campID + i, campID);
                    CurWPZ_->SetLabel(
                        campID + i,
                        gStringMgr->GetText(gStringMgr->AddText(buf)));
                    CurWPZ_->SetState(campID + i, state);
                    CurWPZ_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff,
                                           0x000000ff);
                    CurWPZ_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff,
                                          0x000000ff);
                    CurWPZ_->SetTextOffset(campID + i, 0, -15);

                    tmpID = new VU_ID;
                    *tmpID = unit->Id();
                    wpl->Flags or_eq UseFlag;
                    wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                    wpl->Icon->SetUserNumber(C_STATE_1, i);

                    if (not(TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
                        if (firstwp not_eq unit->GetCurrentUnitWP())
                            wpl->Dragable = 0;
                }
            }

            dx = lx - x;
            dy = ly - y;
            dz = lz - z;

            lx = x;
            ly = y;
            lz = z;

            i++;
            distance += sqrt(dx * dx + dy * dy + dz * dz);

            if (wp->GetWPAction() == WP_LAND and UseFlag)
            {
                endtime = wp->GetWPArrivalTime();
                UseFlag = 0;
            }
        }

        prevwp = wp;
        wp = wp->GetNextWP();
    }

    if (airwps)
    {
        wp = ((Flight)unit)->GetOverrideWP();

        if (wp)
        {
            wp->GetLocation(
                &x, &y,
                &z); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z

            _sntprintf(buf, 39, "%s", gStringMgr->GetString(TXT_DIVERT));
            buf[39] = 0;

            wpl = CurWP_->AddWaypointToList(campID + i, 0, TGT_CUR, TGT_CUR_SEL,
                                            TGT_CUR_ERROR, y, maxy - x, FALSE);

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID);
                CurWP_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));
                CurWP_->SetLabel(campID + i,
                                 gStringMgr->GetText(gStringMgr->AddText(buf)));
                CurWP_->SetTextOffset(campID + i, 0, -15);
                CurWP_->SetState(campID + i, 0);
                CurWP_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff,
                                      0x000000ff);
                CurWP_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff,
                                     0x000000ff);
                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, -1);
            }

            // Set Z Waypoint
            xval = (i - 1) * (650 / numwp) + 60;

            wpl = CurWPZ_->AddWaypointToList(
                campID + i, 0, TGT_CUR, TGT_CUR_SEL, TGT_CUR_ERROR,
                static_cast<float>(xval), z, FALSE);

            if (wpl)
            {
                CurWPZ_->SetWPGroup(campID + i, campID);
                CurWPZ_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));
                CurWPZ_->SetLabel(
                    campID + i, gStringMgr->GetText(gStringMgr->AddText(buf)));
                CurWPZ_->SetState(campID + i, 0);
                CurWPZ_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff,
                                       0x000000ff);
                CurWPZ_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff,
                                      0x000000ff);
                CurWPZ_->SetTextOffset(campID + i, 0, -15);

                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, -1);

                if (not(TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
                    if (firstwp not_eq unit->GetCurrentUnitWP())
                        wpl->Dragable = 0;
            }
        }
    }

    CurWPZ_->SetUserNumber(C_STATE_1, endtime - starttime);
    CurWPZ_->SetUserNumber(C_STATE_2, (long)distance);
    CurWPZ_->SetUserNumber(C_STATE_3, 0 /* MPG */);
}

void C_Map::AddToCurIcons(MAPICONLIST *MapItem)
{
    if (MapItem)
        CurIcons_->Add(MapItem);
}

void C_Map::CenterOnIcon(MAPICONLIST *MapItem)
{
    if (MapItem)
    {
        //cx=(CurWPArea_.top/1640 + CurWPArea_.bottom/1640)/2;
        //cy=((maxy - CurWPArea_.left)/1640 + (maxy - CurWPArea_.right)/1640)/2;

        SetMapCenter(static_cast<long>(MapItem->worldx / FEET_PER_PIXEL),
                     static_cast<long>(MapItem->worldy / FEET_PER_PIXEL));

        if (DrawWindow_)
            DrawWindow_->RefreshWindow();
    }
}

void C_Map::BuildWPList(C_Waypoint *wplist, C_Waypoint *, Unit unit)
{
    WayPoint wp = NULL;
    float x, y, z;
    long normID, selID, othrID;
    long i;
    long campID;
    long UseFlag;
    WAYPOINTLIST *wpl = NULL;
    short state;
    _TCHAR buf[39];
    VU_ID *tmpID = NULL;
    short airwps, lastwp;

    if (unit == NULL)
        return;

    if (not unit->Final())
        return;

    airwps = static_cast<short>(unit->IsFlight());

    if (not airwps)
    {
        wp = unit->GetCurrentUnitWP();

        if (wp and wp->GetPrevWP())
            wp = wp->GetPrevWP();
    }

    if (not wp or airwps)
        wp = unit->GetFirstUnitWP();

    if (not wp)
        return;

    campID = unit->GetCampID() << 8;

    // set to 0 after Landing WP so we don't connect lines to following waypoints which are not on the agenda (Alt Land cit,Tanker etc)
    UseFlag = C_BIT_USELINE;

    i = 1;
    lastwp = 0;

    while (wp)
    {
        if (not wp->GetNextWP())
            lastwp = 1;

        if (wp->GetWPFlags() bitand WPF_TARGET)
        {
            normID = TGT_OTR;
            selID = TGT_OTR_SEL;
            othrID = TGT_OTR_OTHER;
        }
        else if (wp->GetWPFlags() bitand WPF_IP)
        {
            normID = IP_OTR;
            selID = IP_OTR_SEL;
            othrID = IP_OTR_OTHER;
        }
        else
        {
            normID = STPT_OTR;
            selID = STPT_OTR_SEL;
            othrID = STPT_OTR_OTHER;
        }

        if (wp == unit->GetCurrentUnitWP())
            state = 1;
        else
            state = 0;

        wp->GetLocation(
            &x, &y,
            &z); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z

        // Set 2d Waypoint
        if (UseFlag)
            _stprintf(buf, "%1d", i);
        else
        {
            if (wp->GetWPAction() == WP_LAND)
            {
                _sntprintf(buf, 39, "%s",
                           gStringMgr->GetString(TXT_ALTERNATE_FIELD));
                buf[39] = 0;
            }
            else if (wp->GetWPAction() == WP_REFUEL)
            {
                _sntprintf(buf, 39, _T("%s"),
                           gStringMgr->GetString(TXT_TANKER));
                buf[39] = 0;
            }
            else
                buf[0] = 0;
        }

        if (buf[0])
        {
            if (airwps)
            {
                if (i == 1)
                    wpl =
                        wplist->AddWaypointToList(campID + i, 0, normID, selID,
                                                  othrID, y, maxy - x, FALSE);
                else
                    wpl =
                        wplist->AddWaypointToList(campID + i, 0, normID, selID,
                                                  othrID, y, maxy - x, FALSE);
            }
            else
            {
                if (lastwp)
                    wpl =
                        wplist->AddWaypointToList(campID + i, 0, normID, selID,
                                                  othrID, y, maxy - x, FALSE);
                else
                    wpl = wplist->AddWaypointToList(campID + i, 0, 0, 0, 0, y,
                                                    maxy - x, FALSE);
            }

            if (wpl)
            {
                wplist->SetWPGroup(campID + i, campID);

                // 2002-03-10 MN fix for black eagle on black ground ;-)
                if (g_bAWACSBackground)
                {
                    wplist->SetLabelColor(campID + i, 0x00999999, 0x00ffffff,
                                          0x00999999);
                    wplist->SetLineColor(campID + i, 0x00999999, 0x00ffffff,
                                         0x00999999);
                }
                else
                {
                    wplist->SetLabelColor(campID + i, 0, 0x00500000, 0);
                    wplist->SetLineColor(campID + i, 0, 0x00500000, 0);
                }

                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Flags or_eq UseFlag;
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, i);
            }
        }

        i++;
        wp = wp->GetNextWP();
    }
}

BOOL C_Map::SetWaypointList(VU_ID unitID)
{
    F4CSECTIONHANDLE *Leave = NULL;
    Unit unit;

    if (unitID == FalconNullId)
        return (FALSE);

    unit = (Unit)FindUnit(unitID);

    if (unit == NULL)
        return (FALSE);

    if (DrawWindow_)
        Leave = UI_Enter(DrawWindow_);

    Team_[unit->GetTeam()].Waypoints->EraseWaypointGroup(unit->GetCampID()
                                                         << 8);
    CampEnterCriticalSection();
    BuildWPList(Team_[unit->GetTeam()].Waypoints, NULL, unit);
    CampLeaveCriticalSection();
    Team_[unit->GetTeam()].Waypoints->Refresh();
    UI_Leave(Leave);
    return (TRUE);
}

BOOL C_Map::SetCurrentWaypointList(VU_ID unitID)
{
    F4CSECTIONHANDLE *Leave = NULL;
    Unit unit;

    if (CurWP_->Dragging())
        return (FALSE);

    if (CurWPZ_->Dragging())
        return (FALSE);

    CampEnterCriticalSection();

    if (DrawWindow_)
        Leave = UI_Enter(DrawWindow_);

    CurWP_->Refresh();
    CurWPZ_->Refresh();

    CurWP_->EraseWaypointList();
    CurWPZ_->EraseWaypointList();

    CurWPArea_.top = -1;
    CurWPArea_.left = -1;
    CurWPArea_.bottom = -1;
    CurWPArea_.right = -1;

    WPUnitID_ = unitID;
    gActiveFlightID = unitID;

    if (unitID == FalconNullId)
    {
        UI_Leave(Leave);
        CampLeaveCriticalSection();
        return (FALSE);
    }

    unit = (Unit)FindUnit(unitID);

    if (unit == NULL)
    {
        UI_Leave(Leave);
        CampLeaveCriticalSection();
        return (FALSE);
    }

    BuildCurrentWPList(unit);
    flags_ or_eq I_NEED_TO_DRAW;
    UI_Leave(Leave);
    CampLeaveCriticalSection();
    return (TRUE);
}

void C_Map::UpdateWaypoint(Flight flt)
{
    WayPoint wp;
    WAYPOINTLIST *nub;
    short i, check;

    if (flt->Id() == WPUnitID_)
    {
        // We have to traverse all our waypoints to check for validity
        if (flt->GetCurrentUnitWP() == flt->GetFirstUnitWP())
            check = 1;
        else
            check = 0;

        i = 1;
        wp = flt->GetFirstUnitWP();

        while (wp)
        {
            if (not IsValidWP(wp, flt) and check)
            {
                CurWP_->SetState((flt->GetCampID() << 8) + i, 2);
                CurWP_->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i,
                                 2);
                CurWPZ_->SetState((flt->GetCampID() << 8) + i, 2);
            }
            else if (wp == flt->GetCurrentUnitWP())
            {
                CurWP_->SetState((flt->GetCampID() << 8) + i, 1);
                CurWP_->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i,
                                 1);
                CurWPZ_->SetState((flt->GetCampID() << 8) + i, 1);
            }
            else
            {
                CurWP_->SetState((flt->GetCampID() << 8) + i, 0);
                CurWP_->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i,
                                 0);
                CurWPZ_->SetState((flt->GetCampID() << 8) + i, 0);
            }

            i++;
            wp = wp->GetNextWP();
        }

        if (not(TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
        {
            if (i > 1)
            {
                nub = CurWP_->GetRoot();

                while (nub)
                {
                    if (nub->ID bitand 0x40000000)
                    {
                        nub->Icon->SetText(0, TXT_SPACE);
                        nub->Icon->SetText(1, TXT_SPACE);
                        nub->Icon->SetText(2, TXT_SPACE);
                        nub->Icon->SetFlagBitOff(C_BIT_ENABLED);
                    }

                    nub = nub->Next;
                }

                CurWP_->SetFlagBitOff(C_BIT_DRAGABLE);
                CurWPZ_->SetFlagBitOff(C_BIT_DRAGABLE);
            }
        }

        CurWP_->Refresh();
        CurWPZ_->Refresh();
    }
    else
    {
        i = 1;
        wp = flt->GetFirstUnitWP();

        while (wp and wp not_eq flt->GetCurrentUnitWP())
        {
            i++;
            wp = wp->GetNextWP();
        }

        Team_[flt->GetTeam()].Waypoints->SetGroupState(flt->GetCampID() << 8,
                                                       0);
        Team_[flt->GetTeam()].Waypoints->SetState((flt->GetCampID() << 8) + i,
                                                  1);
        Team_[flt->GetTeam()].Waypoints->SetState(
            0x40000000 bitor (flt->GetCampID() << 8) + i, 1);
        Team_[flt->GetTeam()].Waypoints->Refresh();
    }
}

void C_Map::RemoveCurWPList()
{
    CurWP_->Refresh();
    CurWP_->EraseWaypointList();
    CurWPZ_->Refresh();
    CurWPZ_->EraseWaypointList();
    CurIcons_->Cleanup();
    CurIcons_->Setup(CurIcons_->GetID(), 0);
    WPUnitID_ = FalconNullId;
}

void C_Map::RemoveWaypoints(short team, long group)
{
    Team_[team].Waypoints->Refresh();
    Team_[team].Waypoints->EraseWaypointGroup(group);
}

// this is called like every 5 or 10 seconds by a callback...
void C_Map::RemoveOldWaypoints()
{
    Unit un;

    // remove current set
    if (WPUnitID_ not_eq FalconNullId)
    {
        un = (Unit)vuDatabase->Find(WPUnitID_);

        if (not un)
            RemoveCurWPList();
    }

    // go through the teams and remove old WP lists
}

void C_Map::RemoveAllWaypoints(short owner)
{
    if ((DrawWindow_) and (Team_[owner].Waypoints))
        Team_[owner].Waypoints->EraseWaypointList();
}

void C_Map::RemoveFromCurIcons(long ID)
{
    if (CurIcons_)
        CurIcons_->Remove(ID);
}

void C_Map::SetBullsEye(float x, float y)
{
    BullsEyeX_ = x;
    BullsEyeY_ = y;

    if (BullsEye_)
        BullsEye_->SetPos(x, y); // Real World XY (where 0,0 is top left corner)
}

void C_Map::TurnOnBullseye()
{
    if (BullsEye_)
    {
        BullsEye_->SetFlagBitOff(C_BIT_INVISIBLE);

        if (DrawWindow_)
            DrawWindow_->RefreshWindow();
    }
}

void C_Map::TurnOffBullseye()
{
    if (BullsEye_)
    {
        BullsEye_->SetFlagBitOn(C_BIT_INVISIBLE);

        if (DrawWindow_)
            DrawWindow_->RefreshWindow();
    }
}

void C_Map::RemapTeamColors(long team)
{
    short j, k;

    if (team >= NUM_TEAMS)
        return;

    if (Team_[team].Objectives)
    {
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
        {
            Team_[team].Objectives->Type[j]->SetMainImage(ObjIconIDs_[team][0],
                                                          ObjIconIDs_[team][1]);
            Team_[team].Objectives->Type[j]->RemapIconImages();
        }
    }

    if (Team_[team].NavalUnits)
    {
        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
        {
            Team_[team].NavalUnits->Type[j]->SetMainImage(
                NavyIconIDs_[team][0], NavyIconIDs_[team][1]);
            Team_[team].NavalUnits->Type[j]->RemapIconImages();
        }
    }

    if (Team_[team].Units)
    {
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
        {
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
            {
                Team_[team].Units->Type[j]->Levels[k]->SetMainImage(
                    ArmyIconIDs_[team][0], ArmyIconIDs_[team][1]);
                Team_[team].Units->Type[j]->Levels[k]->RemapIconImages();
            }
        }
    }

    if (Team_[team].AirUnits)
    {
        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
        {
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_0, AirIconIDs_[team][0][0], AirIconIDs_[team][0][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_1, AirIconIDs_[team][1][0], AirIconIDs_[team][1][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_2, AirIconIDs_[team][2][0], AirIconIDs_[team][2][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_3, AirIconIDs_[team][3][0], AirIconIDs_[team][3][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_4, AirIconIDs_[team][4][0], AirIconIDs_[team][4][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_5, AirIconIDs_[team][5][0], AirIconIDs_[team][5][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_6, AirIconIDs_[team][6][0], AirIconIDs_[team][6][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(
                C_STATE_7, AirIconIDs_[team][7][0], AirIconIDs_[team][7][1]);
            Team_[team].AirUnits->Type[j]->RemapIconImages();
        }
    }
}

void C_Map::FitFlightPlan()
{
    // Based on CurWP_'s x bitand y (ie: CurWPArea_)
    long cx, cy;
    long w, h;

    if (CurWPArea_.left < 0 or CurWPArea_.top < 0 or CurWPArea_.right < 0 or
        CurWPArea_.bottom < 0)
        return;

    w = (CurWPArea_.right - CurWPArea_.left) /
        1000; // 1100 = ft -> 500m * 1.64 (allow for icons to fit on map also)
    h = (CurWPArea_.bottom - CurWPArea_.top) / 1000;

    if (w > h)
        ZoomLevel_ = w;
    else
        ZoomLevel_ = h;

    if (ZoomLevel_ < MaxZoomLevel_)
        ZoomLevel_ = MaxZoomLevel_;

    if (ZoomLevel_ > MinZoomLevel_)
        ZoomLevel_ = MinZoomLevel_;

    cx = (CurWPArea_.top / 1640 + CurWPArea_.bottom / 1640) / 2;
    cy = static_cast<long>(
        ((maxy - CurWPArea_.left) / 1640 + (maxy - CurWPArea_.right) / 1640) /
        2);

    SetMapCenter(cx, cy);
}

// Public
void C_Map::ShowObjectiveType(long mask)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    ObjectiveMask_ or_eq
        (1 << FindTypeIndex(mask, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (not Team_[i].Objectives->Flags[j] and
                    (ObjectiveMask_ bitand (1 << j)))
                {
                    Team_[i].Objectives->Flags[j] = 1;
                    Team_[i].Objectives->Type[j]->SetFlagBitOff(
                        C_BIT_INVISIBLE);
                    Team_[i].Objectives->Type[j]->Refresh();
                }
        }

    UI_Leave(Leave);
}

void C_Map::HideObjectiveType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].Objectives->Type[j]->Refresh();
                    Team_[i].Objectives->Flags[j] = 0;
                    Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    UI_Leave(Leave);
    ObjectiveMask_ and_eq compl offflag;
}

void C_Map::ShowUnitType(long mask)
{
    short i, j, k;
    F4CSECTIONHANDLE *Leave;

    UnitMask_ or_eq
        (1 << FindTypeIndex(mask, GND_TypeList, _MAP_NUM_GND_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            {
                if (not Team_[i].Units->Flags[j] and
                    (UnitMask_ bitand (1 << j)))
                {
                    Team_[i].Units->Flags[j] = 1;

                    for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                    {
                        if (Team_[i].Units->Type[j]->Flags[k] == 1)
                        {
                            Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(
                                C_BIT_INVISIBLE);
                            Team_[i].Units->Type[j]->Levels[k]->Refresh();
                        }
                    }
                }
            }
        }

    UI_Leave(Leave);
}

void C_Map::HideUnitType(long mask)
{
    short i, j, k;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, GND_TypeList, _MAP_NUM_GND_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            {
                if (Team_[i].Units->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].Units->Flags[j] = 0;

                    for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                    {
                        if (Team_[i].Units->Type[j]->Flags[k] == 1)
                        {
                            Team_[i].Units->Type[j]->Levels[k]->Refresh();
                            Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(
                                C_BIT_INVISIBLE);
                        }
                    }
                }
            }
        }

    UI_Leave(Leave);
    UnitMask_ and_eq compl offflag;
}

void C_Map::SetUnitLevel(long level)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                {
                    if (level >= 0 and level < _MAP_NUM_GND_LEVELS_)
                    {
                        Team_[i].Units->Type[j]->Flags[0] = 0;

                        if (level)
                            Team_[i].Units->Type[j]->Levels[0]->Refresh();

                        Team_[i].Units->Type[j]->Levels[0]->SetFlagBitOn(
                            C_BIT_INVISIBLE);
                        Team_[i].Units->Type[j]->Flags[1] = 0;

                        if (level not_eq 1)
                            Team_[i].Units->Type[j]->Levels[1]->Refresh();

                        Team_[i].Units->Type[j]->Levels[1]->SetFlagBitOn(
                            C_BIT_INVISIBLE);
                        Team_[i].Units->Type[j]->Flags[2] = 0;

                        if (level not_eq 2)
                            Team_[i].Units->Type[j]->Levels[2]->Refresh();

                        Team_[i].Units->Type[j]->Levels[2]->SetFlagBitOn(
                            C_BIT_INVISIBLE);
                        Team_[i].Units->Type[j]->Flags[level] = 1;

                        if (Team_[i].Units->Flags[j])
                        {
                            Team_[i]
                                .Units->Type[j]
                                ->Levels[level]
                                ->SetFlagBitOff(C_BIT_INVISIBLE);
                            Team_[i].Units->Type[j]->Levels[level]->Refresh();
                        }
                    }
                }
        }

    UI_Leave(Leave);
}

void C_Map::ShowAirUnitType(long mask)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    AirUnitMask_ or_eq
        (1 << FindTypeIndex(mask, AIR_TypeList, _MAP_NUM_AIR_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (not Team_[i].AirUnits->Flags[j] and
                    (AirUnitMask_ bitand (1 << j)))
                {
                    Team_[i].AirUnits->Flags[j] = 1;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                    Team_[i].AirUnits->Type[j]->Refresh();
                }
        }

    UI_Leave(Leave);
}

void C_Map::HideAirUnitType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, AIR_TypeList, _MAP_NUM_AIR_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].AirUnits->Type[j]->Refresh();
                    Team_[i].AirUnits->Flags[j] = 0;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    UI_Leave(Leave);
    AirUnitMask_ and_eq compl offflag;
}

// 2002-02-21 ADDED BY S.G. Goes throuh all the AIR_TypeList and refresh them
void C_Map::RefreshAllAirUnitType(void)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                Team_[i].AirUnits->Type[j]->Refresh();
        }
    }

    UI_Leave(Leave);
}

void C_Map::ShowNavalUnitType(long mask)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    NavalUnitMask_ or_eq
        (1 << FindTypeIndex(mask, NAV_TypeList, _MAP_NUM_NAV_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (not Team_[i].NavalUnits->Flags[j] and
                    (NavalUnitMask_ bitand (1 << j)))
                {
                    Team_[i].NavalUnits->Flags[j] = 1;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOff(
                        C_BIT_INVISIBLE);
                    Team_[i].NavalUnits->Type[j]->Refresh();
                }
        }

    UI_Leave(Leave);
}

void C_Map::HideNavalUnitType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, NAV_TypeList, _MAP_NUM_NAV_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].NavalUnits->Type[j]->Refresh();
                    Team_[i].NavalUnits->Flags[j] = 0;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    UI_Leave(Leave);
    NavalUnitMask_ and_eq compl offflag;
}

/***************************************************************************\
    Artscout - 2026: campaign overlays -- draw what the supply model is already doing.

    The campaign runs a real economy (campupd/supply.cpp) and none of it was ever shown. Every
    tick ProduceSupplies walks the objective list: factories, army bases, depots and ports make
    supply and replacement vehicles, refineries make fuel, each at DataRate * status/100 -- and
    then, with PowerGrid on, multiplied AGAIN by the status of its nearest non-hostile power
    plant. SupplyUnits then pathfinds from a supply source to each unit and SendSupply deposits a
    running total at every road, intersection, railroad and bridge it crosses, skimming
    (node losses + 2)% off at each one. Those per-objective totals are live, replicated as dirty
    data (objSetSupply), and until now read by exactly one thing: the campaign tool's debug dialog.

    So there is a whole logistics picture sitting in memory with no way to see it. These three
    layers put it on the map, reusing the raster overlay the SAM and radar rings already use.

    Only one can be up at a time, because C_ScaleBitmap keeps a single blended palette -- the same
    reason the threat rings are a radio group and not checkboxes. Selecting a campaign layer takes
    the overlay from the rings and vice versa; the menu code keeps the two groups in step.
\***************************************************************************/

// The overlay is one byte per map pixel used as a 4-bit index into C_ScaleBitmap's palette table,
// but PreparePalette only fills entries 1..9 -- 10 through 15 are allocated and left uninitialised,
// so writing them would read whatever was in that memory. Everything below clamps into 1..9, and 0
// means "leave the map alone".
#define CAMP_TINT_MAX 9

// Stamp a filled disc into the overlay. Brightest contributor wins rather than accumulating, so a
// cluster of overlapping nodes reads as its strongest member instead of saturating to a solid blob
// the moment two of them touch.
static void StampOverlayDisc(BYTE *overlay, long w, long h, long cx, long cy,
                             long r, BYTE tint)
{
    if (not overlay or r < 1 or tint < 1)
        return;

    long y0 = cy - r, y1 = cy + r;

    if (y0 < 0)
        y0 = 0;

    if (y1 > h - 1)
        y1 = h - 1;

    const long r2 = r * r;

    for (long y = y0; y <= y1; y++)
    {
        const long dy = y - cy;
        const long span = static_cast<long>(sqrt(static_cast<double>(r2 - dy * dy)));
        long x0 = cx - span, x1 = cx + span;

        if (x0 < 0)
            x0 = 0;

        if (x1 > w - 1)
            x1 = w - 1;

        BYTE *row = overlay + y * w;

        for (long x = x0; x <= x1; x++)
            if (row[x] < tint)
                row[x] = tint;
    }
}

// Stamp a line into the overlay, thickness in pixels. Plain DDA -- the longer axis is stepped one
// pixel at a time and the other interpolated -- which is all a tint needs, and it avoids pulling in
// a clipper: every write goes through the bounds check.
static void StampOverlayLine(BYTE *overlay, long w, long h, long x0, long y0,
                             long x1, long y1, long thick, BYTE tint)
{
    if (not overlay or tint < 1)
        return;

    const long dx = x1 - x0, dy = y1 - y0;
    long steps = (labs(dx) > labs(dy)) ? labs(dx) : labs(dy);

    if (steps < 1)
        steps = 1;

    if (thick < 1)
        thick = 1;

    const float sx = (float)dx / (float)steps;
    const float sy = (float)dy / (float)steps;
    float fx = (float)x0, fy = (float)y0;
    const long r = thick / 2;

    for (long i = 0; i <= steps; i++)
    {
        const long px = (long)fx, py = (long)fy;

        for (long oy = -r; oy <= r; oy++)
        {
            const long yy = py + oy;

            if (yy < 0 or yy >= h)
                continue;

            BYTE *row = overlay + yy * w;

            for (long ox = -r; ox <= r; ox++)
            {
                const long xx = px + ox;

                if (xx < 0 or xx >= w)
                    continue;

                if (row[xx] < tint)
                    row[xx] = tint;
            }
        }

        fx += sx;
        fy += sy;
    }
}

// Campaign grid -> overlay pixel. AddThreat flips y against Map_Max_Y and BuildOverlay then scales
// by a hardcoded 2 px per grid unit; derive the scale from the actual bitmap instead so a theater
// whose map is not exactly twice its grid still lands correctly, and fall back to the 2 the threat
// rings assume if the extents are not set up yet.
static void CampGridToOverlay(long w, long h, GridIndex gx, GridIndex gy,
                              long *px, long *py)
{
    extern short Map_Max_X;
    extern short Map_Max_Y;

    const float sx = (Map_Max_X > 0) ? (float)w / (float)Map_Max_X : 2.0f;
    const float sy = (Map_Max_Y > 0) ? (float)h / (float)Map_Max_Y : 2.0f;

    *px = static_cast<long>(gx * sx);
    *py = static_cast<long>((Map_Max_Y - gy) * sy);
}

// How many power plants we will consider for the coverage map. A theater has a few dozen; the cap
// only exists so the per-pixel nearest search below stays bounded no matter what gets loaded.
#define CAMP_MAX_PLANTS 128

// Artscout - 2026: the forward line of own troops, as a polyline.
//
// FLOTList is built and kept current by the campaign already -- RebuildFLOTList (camplist.cpp)
// takes the midpoint of every link between two frontline objectives on opposing teams, drops any
// point within 30 km of one it already has, and gamemgr.cpp rebuilds it as the war moves. It has
// only ever been read for distance-to-front arithmetic; nothing has ever drawn it.
//
// The list is sorted along one axis -- FLOTSortDirection picks x or y -- and RebuildFLOTList's own
// comment warns that this "will look very bad in some situations", which is honest: a front that
// doubles back on itself cannot be traced correctly by sorting on a single coordinate, and will
// show a zigzag where the line crosses itself. In Korea the front runs broadly east-west, so
// sorting west-to-east follows it. Nothing here can improve on that without replacing the sort,
// which is campaign code that other things depend on.
static void StampFlotLine(BYTE *overlay, long w, long h)
{
    extern bool g_bLogCampMenu;
    extern void RebuildFLOTList(void);

    if (not overlay or not FLOTList)
        return;

    // Build the list before reading it. RebuildFLOTList has exactly one live caller
    // (gamemgr.cpp, as the player is put into a vehicle), so in the campaign UI the list is empty
    // until you have actually flown -- which is why this drew nothing at all on the first attempt
    // and the blobs being blamed on it were steerpoint markers.
    //
    // Cheap enough to do here: ShowCampaignOverlay is reached only from menu actions, never per
    // frame, and the walk is over FrontList, which StandardRebuild keeps current. Doing it on
    // every overlay build also means the line follows the front as the war moves rather than
    // freezing at whatever it was when it was first drawn.
    RebuildFLOTList();

    ListElementClass *lp = FLOTList->GetFirstElement();
    long lastx = 0, lasty = 0;
    bool have = false;
    long n = 0;

    if (g_bLogCampMenu)
    {
        extern short Map_Max_X;
        extern short Map_Max_Y;
        _TCHAR hd[160];
        sprintf(hd, "[FLOT] overlay %ldx%ld  gridMax %dx%d\n", w, h,
                (int)Map_Max_X, (int)Map_Max_Y);
        FFDebugLog(hd);
    }

    while (lp)
    {
        GridIndex gx = 0, gy = 0;
        UnpackXY(lp->GetUserData(), &gx, &gy);

        long px, py;
        CampGridToOverlay(w, h, gx, gy, &px, &py);

        // The whole question is whether consecutive points are far apart. A polyline that comes
        // out as isolated blobs means each segment is a pixel or two long, which would say the
        // list is not what this assumes -- so print the step, not just the point.
        if (g_bLogCampMenu and n < 16)
        {
            _TCHAR ln[160];
            sprintf(ln, "[FLOT] %2ld grid=(%d,%d) px=(%ld,%ld) step=%ld\n", n,
                    (int)gx, (int)gy, px, py,
                    have ? (labs(px - lastx) + labs(py - lasty)) : -1);
            FFDebugLog(ln);
        }

        if (have)
            StampOverlayLine(overlay, w, h, lastx, lasty, px, py, 3,
                             CAMP_TINT_MAX);

        lastx = px;
        lasty = py;
        have = true;
        n++;
        lp = lp->GetNext();
    }

    if (g_bLogCampMenu)
    {
        _TCHAR tl[80];
        sprintf(tl, "[FLOT] %ld points total\n", n);
        FFDebugLog(tl);
    }
}

void C_Map::ShowCampaignOverlay(long which)
{
    F4CSECTIONHANDLE *Leave;
    extern bool g_bCampFlotLine;
    extern bool g_bLogCampMenu;

    if (not Map_)
        return;

    CampOverlay_ = which;

    // No campaign loaded (dogfight, tactical engagement, the menus before a save is opened)
    // means no objective list to read -- every layer below would iterate a null list.
    if (not AllObjList)
        which = CampOverlay_ = CAMP_OVERLAY_OFF;

    // The FLOT is a toggle, not one of the radio layers, so it has to survive "no layer selected".
    // With every layer off and the FLOT on there is still an overlay to build -- just this one
    // thing in it.
    const bool flot = g_bCampFlotLine and FLOTList and AllObjList;

    if (which == CAMP_OVERLAY_OFF and not flot)
    {
        Map_->NoOverlay();
        flags_ or_eq I_NEED_TO_DRAW_MAP;
        return;
    }

    // The rings and these layers cannot both own the palette. Give it up here so the menu's two
    // radio groups and the map agree about which one is live.
    ThreatMask_ = 0;

    Leave = UI_Enter(DrawWindow_);

    const long w = Map_->GetW();
    const long h = Map_->GetH();

    switch (which)
    {
    case CAMP_OVERLAY_OFF:
        // FLOT only. Its own hue, since no layer is claiming one.
        Map_->PreparePalette(RGB(245, 245, 245));
        break;

    case CAMP_OVERLAY_POWER:
        // One blended palette per overlay, so the links, the hubs and the damage all share a hue and
        // differ only in strength: quiet red is a working feed, bright red is a plant that has
        // stopped delivering and every consumer still tied to it.
        Map_->PreparePalette(RGB(255, 40, 40));
        break;

    case CAMP_OVERLAY_SUPPLY:
        Map_->PreparePalette(RGB(255, 176, 0));
        break;

    case CAMP_OVERLAY_DAMAGE:
        Map_->PreparePalette(RGB(255, 80, 220));
        break;

    default:
        Map_->PreparePalette(RGB(48, 255, 48));
        break;
    }

    Map_->ClearOverlay();
    BYTE *overlay = Map_->GetOverlay();

    if (not overlay or w < 1 or h < 1)
    {
        UI_Leave(Leave);
        return;
    }

    if (which == CAMP_OVERLAY_POWER)
    {
        // Draw the dependency itself -- a line from each thing that needs power to the plant that
        // supplies it -- rather than the Voronoi cells this used to shade.
        //
        // The cells were correct and unreadable. A Voronoi BOUNDARY lies midway between two plants,
        // so the marks always appeared where there was no plant at all, which is the opposite of the
        // question being asked. A link tells you the thing directly: every line leaving a plant is a
        // factory, refinery, depot, port or army base whose output is scaled by that plant's status
        // in ProduceSupplies, and the fan of lines IS the answer to "what does knocking this out
        // cost me".
        //
        // And it is exact now, not an approximation. Iterating producers rather than pixels means
        // each one can be matched using its OWN team relations -- the same GetTTRelations test
        // FindNearestFriendlyPowerStation applies -- instead of the all-plants-at-once nearest
        // neighbour the cell fill had to use. No more hand-waving near the FLOT.
        struct
        {
            long x, y, lost;
            Team team;
        } plants[CAMP_MAX_PLANTS];
        int n = 0;

        {
            VuListIterator it(AllObjList);

            for (Objective o = GetFirstObjective(&it); o and n < CAMP_MAX_PLANTS;
                 o = GetNextObjective(&it))
            {
                const int t = o->GetType();

                if (t not_eq TYPE_NUCLEAR and t not_eq TYPE_POWERPLANT)
                    continue;

                GridIndex gx, gy;
                o->GetLocation(&gx, &gy);
                CampGridToOverlay(w, h, gx, gy, &plants[n].x, &plants[n].y);
                long status = o->GetObjectiveStatus();

                if (status < 0)
                    status = 0;
                else if (status > 100)
                    status = 100;

                plants[n].lost = 100 - status;
                plants[n].team = o->GetTeam();
                n++;
            }
        }

        if (n > 0)
        {
            VuListIterator it(AllObjList);

            for (Objective o = GetFirstObjective(&it); o; o = GetNextObjective(&it))
            {
                const int t = o->GetType();

                if (t not_eq TYPE_FACTORY and t not_eq TYPE_REFINERY and
                    t not_eq TYPE_DEPOT and t not_eq TYPE_PORT and
                    t not_eq TYPE_ARMYBASE)
                    continue;

                GridIndex gx, gy;
                o->GetLocation(&gx, &gy);
                long px, py;
                CampGridToOverlay(w, h, gx, gy, &px, &py);

                // Same choice the sim makes: nearest plant this objective's team is not hostile to.
                const Team mine = o->GetTeam();
                int best = -1;
                long bd = 0;

                for (int i = 0; i < n; i++)
                {
                    if (GetTTRelations(plants[i].team, mine) > Neutral)
                        continue;

                    const long ddx = px - plants[i].x;
                    const long ddy = py - plants[i].y;
                    const long d = ddx * ddx + ddy * ddy;

                    if (best < 0 or d < bd)
                    {
                        bd = d;
                        best = i;
                    }
                }

                if (best < 0)
                    continue;

                // A healthy link is drawn, but quietly; a link whose plant is down is drawn loudly.
                // The point is to read "this one has stopped feeding these" at a glance without the
                // intact grid shouting at you on day one.
                const BYTE tint =
                    static_cast<BYTE>(4 + plants[best].lost * 5 / 100);
                StampOverlayLine(overlay, w, h, px, py, plants[best].x,
                                 plants[best].y, 2, tint);
                StampOverlayDisc(overlay, w, h, px, py, 3, tint);
            }

            // The plants themselves, always at full strength so the hubs are findable even when
            // every link into them is healthy and faint.
            for (int i = 0; i < n; i++)
                StampOverlayDisc(overlay, w, h, plants[i].x, plants[i].y, 7,
                                 CAMP_TINT_MAX);
        }
    }
    else if (which == CAMP_OVERLAY_SUPPLY)
    {
        // What SendSupply left behind on its way through -- drawn as the NETWORK it is, not as a
        // scatter of points.
        //
        // Two things were making it read as unconnected circles. Objectives sit kilometres apart,
        // so discs were never going to merge into a route however large; and the tint was scaled
        // against the uchar's 255 ceiling while real road traffic is single digits, so every
        // conduit came out at the faintest step and only the sources looked like anything.
        //
        // Both fixed here. Objectives carry their own links -- GetNeighbor is the same graph
        // SendSupply walks when it pathfinds -- so an edge between two nodes that both carry
        // traffic IS a stretch of supply route, and drawing it gives the arteries. And the tint
        // scales against the busiest thing actually on the map, conduits and sources ranked
        // separately so the roads are not crushed flat by a depot two orders of magnitude busier.
        struct
        {
            long maxNode, maxSrc;
        } peak = {1, 1};

        {
            VuListIterator it(AllObjList);

            for (Objective o = GetFirstObjective(&it); o; o = GetNextObjective(&it))
            {
                const int t = o->GetType();
                const bool node = (t == TYPE_ROAD or t == TYPE_INTERSECT or
                                   t == TYPE_RAILROAD or t == TYPE_BRIDGE);
                const bool src = (t == TYPE_CITY or t == TYPE_PORT or
                                  t == TYPE_DEPOT or t == TYPE_ARMYBASE);

                if (not node and not src)
                    continue;

                const long traffic =
                    o->GetObjectiveSupply() + o->GetObjectiveFuel();

                if (node and traffic > peak.maxNode)
                    peak.maxNode = traffic;

                if (src and traffic > peak.maxSrc)
                    peak.maxSrc = traffic;
            }
        }

        VuListIterator it(AllObjList);

        for (Objective o = GetFirstObjective(&it); o; o = GetNextObjective(&it))
        {
            const int t = o->GetType();

            // The road network carries the flow, but it does not originate it. SendSupply calls
            // AddSupply on the SOURCE before it walks the path, so the depots, ports, army bases
            // and cities that IsSupplySource names accumulate traffic too. They are where the
            // chain starts.
            const bool isNode = (t == TYPE_ROAD or t == TYPE_INTERSECT or
                                 t == TYPE_RAILROAD or t == TYPE_BRIDGE);
            const bool isSource = (t == TYPE_CITY or t == TYPE_PORT or
                                   t == TYPE_DEPOT or t == TYPE_ARMYBASE);

            if (not isNode and not isSource)
                continue;

            const long traffic = o->GetObjectiveSupply() + o->GetObjectiveFuel();

            if (traffic < 1)
                continue;

            GridIndex gx, gy;
            o->GetLocation(&gx, &gy);
            long px, py;
            CampGridToOverlay(w, h, gx, gy, &px, &py);

            const long peakFor = isSource ? peak.maxSrc : peak.maxNode;
            long step = traffic * (CAMP_TINT_MAX - 1) / (peakFor ? peakFor : 1);

            if (step > CAMP_TINT_MAX - 1)
                step = CAMP_TINT_MAX - 1;

            const BYTE tint = static_cast<BYTE>(1 + step);

            // The edges. A link between two objectives that BOTH carry traffic is a stretch of
            // supply route, so draw it -- that is what turns a scatter of marks into arteries.
            // Tinted by the weaker end, because a route is only carrying what its thinnest
            // stretch carries. Each edge gets drawn from both ends; the disc stamper keeps the
            // brightest value, so doing it twice costs a little time and changes nothing.
            const int nLinks = o->NumLinks();

            for (int li = 0; li < nLinks; li++)
            {
                Objective nb = o->GetNeighbor(li);

                if (not nb)
                    continue;

                const long ntraffic =
                    nb->GetObjectiveSupply() + nb->GetObjectiveFuel();

                if (ntraffic < 1)
                    continue;

                GridIndex nx, ny;
                nb->GetLocation(&nx, &ny);
                long npx, npy;
                CampGridToOverlay(w, h, nx, ny, &npx, &npy);

                const long weaker = (ntraffic < traffic) ? ntraffic : traffic;
                long estep =
                    weaker * (CAMP_TINT_MAX - 1) / (peak.maxNode ? peak.maxNode : 1);

                if (estep > CAMP_TINT_MAX - 1)
                    estep = CAMP_TINT_MAX - 1;

                StampOverlayLine(overlay, w, h, px, py, npx, npy, 3,
                                 static_cast<BYTE>(1 + estep));
            }

            // Bridges larger than plain road, being single points of failure; sources larger
            // still, since everything downstream of one depends on it.
            const long radius = isSource ? 12 : ((t == TYPE_BRIDGE) ? 9 : 6);
            StampOverlayDisc(overlay, w, h, px, py, radius, tint);
        }
    }
    else if (which == CAMP_OVERLAY_DAMAGE)
    {
        // What is left of everything, so you can tell a target still worth the sortie from one you
        // already flattened -- without flying a recon over each to find out.
        //
        // The polarity is deliberate: tint is damage TAKEN, so an untouched theater starts clean and
        // fills in as the campaign goes. Bright means wrecked, i.e. do not bother; anything still dark
        // is still standing. Showing remaining health instead would light the whole map on day one and
        // tell you nothing.
        //
        // GetObjectiveStatus is the same percentage the production maths uses -- rolled up from the
        // per-feature damage states -- so what you see here is exactly what the campaign is scoring,
        // not a separate guess at it.
        VuListIterator it(AllObjList);

        for (Objective o = GetFirstObjective(&it); o; o = GetNextObjective(&it))
        {
            long status = o->GetObjectiveStatus();

            if (status < 0)
                status = 0;
            else if (status >= 100)
                continue; // untouched: leave the map alone

            const long lost = 100 - status;
            GridIndex gx, gy;
            o->GetLocation(&gx, &gy);
            long px, py;
            CampGridToOverlay(w, h, gx, gy, &px, &py);

            // Radius carries the same signal as the tint so a flattened objective reads at a glance
            // from a zoomed-out map, where a 3-pixel dot of any colour does not.
            StampOverlayDisc(overlay, w, h, px, py, 3 + lost * 5 / 100,
                             static_cast<BYTE>(1 + lost * (CAMP_TINT_MAX - 1) / 100));
        }
    }
    else
    {
        // Who actually makes the stuff, sized against the biggest producer in the theater, so one
        // refinery carrying a third of the fuel stands out from a dozen small ones. GetObjectiveDataRate
        // already folds in battle damage, so a half-wrecked factory draws half as bright.
        // Artscout - 2026: one-off census of what the campaign actually treats as a producer.
        // ProduceSupplies names TYPE_FACTORY, TYPE_ARMYBASE, TYPE_DEPOT and TYPE_PORT together,
        // but each one's output is class_data->DataRate from the theater's class table -- so a type
        // the loop mentions still contributes nothing if its data says zero. This reports what the
        // data says rather than what the code implies.
        {
            extern bool g_bLogCampProducers;

            if (g_bLogCampProducers)
            {
                // The network types are here as well as the producers because of a second
                // question the code cannot answer on its own: whether a ROAD objective has any
                // bombable features. Damage only registers through CalcStatus, which walks
                // class_data->Features -- a type with none can never drop below 100 however much
                // ordnance lands on it, and the supply loss now derived from status (NodeSupplyLoss,
                // supply.cpp) would then be a bridges-only mechanic no matter what the knob says.
                // The campaign asks for AMIS_INT against roads with no targetID, which hints that
                // way, but the class table is data and this reports what it actually holds.
                const int types[9] = {TYPE_FACTORY,  TYPE_REFINERY, TYPE_DEPOT,
                                      TYPE_PORT,     TYPE_ARMYBASE, TYPE_ROAD,
                                      TYPE_INTERSECT, TYPE_RAILROAD, TYPE_BRIDGE};
                const char *names[9] = {"FACTORY",  "REFINERY", "DEPOT",
                                        "PORT",     "ARMYBASE", "ROAD",
                                        "INTERSECT", "RAILROAD", "BRIDGE"};

                for (int ti = 0; ti < 9; ti++)
                {
                    long count = 0, rate = 0, status = 0, feats = 0, hurt = 0;
                    VuListIterator cit(AllObjList);

                    for (Objective co = GetFirstObjective(&cit); co;
                         co = GetNextObjective(&cit))
                    {
                        if (co->GetType() not_eq types[ti])
                            continue;

                        count++;
                        rate += co->GetObjectiveDataRate();
                        feats += co->GetTotalFeatures();

                        const long st = co->GetObjectiveStatus();
                        status += st;

                        if (st < 100)
                            hurt++;
                    }

                    char lb[200];
                    sprintf(lb,
                            "[CENSUS] %-9s count=%ld dataRate=%ld "
                            "avgFeatures=%ld avgStatus=%ld damaged=%ld\n",
                            names[ti], count, rate,
                            count ? (feats / count) : 0,
                            count ? (status / count) : 0, hurt);
                    FFDebugLog(lb);
                }
            }
        }

        long maxRate = 0;

        {
            VuListIterator it(AllObjList);

            for (Objective o = GetFirstObjective(&it); o; o = GetNextObjective(&it))
            {
                const int t = o->GetType();

                if (t not_eq TYPE_FACTORY and t not_eq TYPE_REFINERY and
                    t not_eq TYPE_DEPOT and t not_eq TYPE_PORT and
                    t not_eq TYPE_ARMYBASE)
                    continue;

                const long r = o->GetObjectiveDataRate();

                if (r > maxRate)
                    maxRate = r;
            }
        }

        if (maxRate > 0)
        {
            VuListIterator it(AllObjList);

            for (Objective o = GetFirstObjective(&it); o; o = GetNextObjective(&it))
            {
                const int t = o->GetType();

                if (t not_eq TYPE_FACTORY and t not_eq TYPE_REFINERY and
                    t not_eq TYPE_DEPOT and t not_eq TYPE_PORT and
                    t not_eq TYPE_ARMYBASE)
                    continue;

                const long r = o->GetObjectiveDataRate();

                if (r < 1)
                    continue;

                GridIndex gx, gy;
                o->GetLocation(&gx, &gy);
                long px, py;
                CampGridToOverlay(w, h, gx, gy, &px, &py);
                const long share = r * (CAMP_TINT_MAX - 1) / maxRate;
                // 8..24 px rather than the original 4..12: at theater zoom the small end of
                // that range was a dot you had to hunt for, which defeats a layer whose whole
                // job is showing relative weight at a glance.
                StampOverlayDisc(overlay, w, h, px, py, 8 + share * 2,
                                 static_cast<BYTE>(1 + share));
            }
        }
    }

    // Last, so it reads over whatever layer is underneath rather than being buried by it. The
    // front is the one line you want to keep your bearings by while looking at something else.
    if (flot)
    {
        // Which layer is underneath matters to reading the result: Production draws large discs
        // sized by output, and those look nothing like a front line but everything like the
        // "circles instead of a line" this is being blamed for.
        if (g_bLogCampMenu)
        {
            _TCHAR wl[96];
            sprintf(wl, "[FLOT] drawn over layer %ld (0=off)\n", which);
            FFDebugLog(wl);
        }

        StampFlotLine(overlay, w, h);
    }

    Map_->UseOverlay();
    flags_ or_eq I_NEED_TO_DRAW_MAP;
    UI_Leave(Leave);
}

void C_Map::ShowThreatType(long mask)
{
    short i, j;
    long timestamp;
    F4CSECTIONHANDLE *Leave;

    ThreatMask_ =
        (1 << FindTypeIndex(mask, THR_TypeList, _MAP_NUM_THREAT_TYPES_));
    // Artscout - 2026: one blended palette, one owner -- taking it for the rings drops
    // whichever campaign layer had it (MenuSetCirclesCB clears that group's check mark).
    CampOverlay_ = CAMP_OVERLAY_OFF;

    timestamp = GetCurrentTime();
    MonoPrint("Start at %1ld...", timestamp);
    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (not Team_[i].Threats->Flags[j] and
                    (ThreatMask_ bitand (1 << j)))
                {
                    Team_[i].Threats->Flags[j] = 1;
                    Team_[i].Threats->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                    Team_[i].Threats->Type[j]->Refresh();
                }
        }

    if (ThreatMask_ bitand _THR_SAM_LOW)
    {
        Map_->PreparePalette(0x0000ff);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_SAM_LOW_])
                {
                    Team_[i].Threats->Type[_THREAT_SAM_LOW_]->BuildOverlay(
                        Map_->GetOverlay(), Map_->GetW(), Map_->GetH(),
                        MapPixelsPerKm());
                }
            }

        Map_->UseOverlay();
    }
    else if (ThreatMask_ bitand _THR_SAM_HIGH)
    {
        Map_->PreparePalette(0x00ffff);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_SAM_HIGH_])
                {
                    Team_[i].Threats->Type[_THREAT_SAM_HIGH_]->BuildOverlay(
                        Map_->GetOverlay(), Map_->GetW(), Map_->GetH(),
                        MapPixelsPerKm());
                }
            }

        Map_->UseOverlay();
    }
    else if (ThreatMask_ bitand _THR_RADAR_LOW)
    {
        Map_->PreparePalette(0xffff00);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_RADAR_LOW_])
                {
                    Team_[i].Threats->Type[_THREAT_RADAR_LOW_]->BuildOverlay(
                        Map_->GetOverlay(), Map_->GetW(), Map_->GetH(),
                        MapPixelsPerKm());
                }
            }

        Map_->UseOverlay();
    }
    else if (ThreatMask_ bitand _THR_RADAR_HIGH)
    {
        Map_->PreparePalette(0xff0000);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_RADAR_HIGH_])
                {
                    Team_[i].Threats->Type[_THREAT_RADAR_HIGH_]->BuildOverlay(
                        Map_->GetOverlay(), Map_->GetW(), Map_->GetH(),
                        MapPixelsPerKm());
                }
            }

        Map_->UseOverlay();
    }

    timestamp = GetCurrentTime() - timestamp;
    MonoPrint("Total time=%1ld\n", timestamp);
#if 0
    Circles_ = ThreatMask_;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            Team_[i].Units->Type[j]->Levels[2]->ShowCircles(Circles_);

#endif
    flags_ or_eq I_NEED_TO_DRAW_MAP;
    UI_Leave(Leave);
}

void C_Map::HideThreatType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, THR_TypeList, _MAP_NUM_THREAT_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (Team_[i].Threats->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].Threats->Type[j]->Refresh();
                    Team_[i].Threats->Flags[j] = 0;
                    Team_[i].Threats->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    ThreatMask_ and_eq compl offflag;

    Map_->NoOverlay();
#if 0
    Circles_ = ThreatMask_;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            Team_[i].Units->Type[j]->Levels[2]->ShowCircles(Circles_);

#endif
    flags_ or_eq I_NEED_TO_DRAW_MAP;
    UI_Leave(Leave);
}

void C_Map::SetMapImage(long ID)
{
    MapID = ID;

    if (Map_ == NULL)
    {
        Map_ = new C_ScaleBitmap;
        Map_->Setup(5551200, 0, MapID);
    }

    // Artscout - 2026: prefer a map built from the theater's terrain. Built once and kept
    // -- it is the same picture for every map view -- and the painted resource is used
    // unchanged if the terrain files cannot be read, so a missing or odd theater degrades
    // to exactly the old behaviour rather than to a blank map.
    {
        extern bool g_bCampMapFromTerrain;
        extern int g_nCampMapTerrainLod;
        static IMAGE_RSC *s_terrainMap = NULL;
        static bool s_terrainMapTried = false;

        if (g_bCampMapFromTerrain and not s_terrainMapTried)
        {
            s_terrainMapTried = true;
            s_terrainMap =
                BuildTerrainMapImage(5551300, g_nCampMapTerrainLod);
        }

        if (g_bCampMapFromTerrain and s_terrainMap)
            Map_->SetImage(s_terrainMap);
        else
            Map_->SetImage(MapID);
    }

    maxy = (float)(Map_->GetH()) * FEET_PER_PIXEL;
    MinZoomLevel_ = Map_->GetW() / _MIN_ZOOM_LEVEL_;
    MaxZoomLevel_ = Map_->GetW() / _MAX_ZOOM_LEVEL_;

    // Artscout - 2026: let a finer map actually be zoomed into. ZoomLevel_ counts SOURCE pixels
    // across the view, and the closest zoom was Map width / 32 -- which scales with the map, so a
    // map with twice the pixels per km still stopped at the same patch of ground and simply
    // downsampled its extra detail back out. The whole point of building it from terrain was to see
    // that detail, so allow zooming proportionally closer: at 4 posts/km against the painted map's
    // 2 px/km, twice as close. Floored so a very fine future map cannot zoom into a handful of
    // pixels, and left exactly as it was when the painted map is in use (ratio 1).
    {
        const float detail = MapPixelsPerKm() / 2.0f;

        if (detail > 1.0f)
        {
            MaxZoomLevel_ = (long)((float)MaxZoomLevel_ / detail);

            if (MaxZoomLevel_ < 32)
                MaxZoomLevel_ = 32;
        }
    }
    ZoomStep_ = (MinZoomLevel_ - MaxZoomLevel_) / 64;

    if (Map_ and DrawWindow_)
    {
        DrawRect_ = DrawWindow_->ClientArea_[0];
        CalculateDrawingParams();
    }
}

void C_Map::SetWindow(C_Window *win)
{
    if (win)
    {
        if (DrawWindow_)
            RemoveListsFromWindow();

        DrawWindow_ = win;

        if (DrawWindow_)
        {
            DrawRect_ = DrawWindow_->ClientArea_[0];
            AddListsToWindow();
            CalculateDrawingParams();
        }
    }
}

long C_Map::GetMapWidth()
{
    return Map_ ? Map_->GetW() : 1536;
}

long C_Map::GetMapHeight()
{
    return Map_ ? Map_->GetH() : 2048;
}

void C_Map::SetupOverlay()
{
    if (Map_)
        Map_->InitOverlay();
}

void C_Map::SetWPZWindow(C_Window *win)
{
    WPZWindow_ = win;
}

void C_Map::SetZoomLevel(short zoom)
{
    if (zoom >= _MIN_ZOOM_LEVEL_ and zoom <= _MAX_ZOOM_LEVEL_ and
        zoom not_eq ZoomLevel_ and Map_)
    {
        ZoomLevel_ = Map_->GetW() / zoom;
        CalculateDrawingParams();
    }
}

void C_Map::ZoomIn()
{
    long tempzoom;

    tempzoom = ZoomLevel_ - (ZoomStep_ + (ZoomLevel_ >> 6));

    if (tempzoom > MinZoomLevel_)
        tempzoom = MinZoomLevel_;

    if (tempzoom < MaxZoomLevel_)
        tempzoom = MaxZoomLevel_;

    if (tempzoom not_eq ZoomLevel_)
    {
        ZoomLevel_ = tempzoom;
        CalculateDrawingParams();
    }
}

void C_Map::ZoomOut()
{
    long tempzoom;

    tempzoom = ZoomLevel_ + (ZoomStep_ + (ZoomLevel_ >> 6));

    if (tempzoom > MinZoomLevel_)
        tempzoom = MinZoomLevel_;

    if (tempzoom < MaxZoomLevel_)
        tempzoom = MaxZoomLevel_;

    if (tempzoom not_eq ZoomLevel_)
    {
        ZoomLevel_ = tempzoom;
        CalculateDrawingParams();
    }
}

void C_Map::SetMapCenter(long x, long y)
{
    CenterX_ = static_cast<float>(x);
    CenterY_ = static_cast<float>(y);

    float mx = Map_ ? Map_->GetW() : 2048.0F;
    float my = Map_ ? Map_->GetH() : 2048.0F;

    if (CenterX_ < 0)
        CenterX_ = 0;

    if (CenterX_ >= mx)
        CenterX_ = mx - 1;

    if (CenterY_ < 0)
        CenterY_ = 0;

    if (CenterY_ >= my)
        CenterY_ = my - 1;

    CalculateDrawingParams();
}

void C_Map::MoveCenter(long x, long y)
{
    float distance;

    if (Map_ == NULL or DrawWindow_ == NULL)
        return;

    if (not x and not y)
        return;

    distance =
        (float)(MapRect_.right - MapRect_.left) /
        (DrawWindow_->ClientArea_[0].right - DrawWindow_->ClientArea_[0].left);

    CenterX_ += (float)x * distance;
    CenterY_ += (float)y * distance;

    if (CenterX_ < 0)
        CenterX_ = 0;

    if (CenterX_ >= Map_->GetW())
        CenterX_ = Map_->GetW() - 1.0f;

    if (CenterY_ < 0)
        CenterY_ = 0;

    if (CenterY_ >= Map_->GetH())
        CenterY_ = Map_->GetH() - 1.0f;

    CalculateDrawingParams();
}

void C_Map::TurnOnNames()
{
    short i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            Team_[i].Objectives->Type[j]->SetFlagBitOff(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            Team_[i].NavalUnits->Type[j]->SetFlagBitOff(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(
                    C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_NOLABEL);
    }

    flags_ or_eq I_NEED_TO_DRAW_MAP;
}

void C_Map::TurnOnBoundaries()
{
}

void C_Map::TurnOnArrows()
{
}

void C_Map::TurnOffNames()
{
    short i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);
    }

    flags_ or_eq I_NEED_TO_DRAW_MAP;
}

void C_Map::TurnOffBoundaries()
{
}

void C_Map::TurnOffArrows()
{
}

void C_Map::SetObjCallbacks(long type, void (*cb)(long, short, C_Base *))
{
    long i;

    if (type < _MAP_NUM_OBJ_TYPES_)
    {
        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Objectives->Type[type])
                Team_[i].Objectives->Type[type]->SetCallback(cb);
    }
}

void C_Map::SetAllObjCallbacks(void (*cb)(long, short, C_Base *))
{
    long i, j;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            if (Team_[i].Objectives->Type[j])
                Team_[i].Objectives->Type[j]->SetCallback(cb);
}

void C_Map::SetAllAirUnitCallbacks(void (*cb)(long, short, C_Base *))
{
    long i, j;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            if (Team_[i].AirUnits->Type[j])
                Team_[i].AirUnits->Type[j]->SetCallback(cb);
}

void C_Map::SetAllGroundUnitCallbacks(void (*cb)(long, short, C_Base *))
{
    long i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                if (Team_[i].Units->Type[j])
                    if (Team_[i].Units->Type[j]->Levels[k])
                        Team_[i].Units->Type[j]->Levels[k]->SetCallback(cb);
}

void C_Map::SetAllNavalUnitCallbacks(void (*cb)(long, short, C_Base *))
{
    long i, j;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            if (Team_[i].NavalUnits->Type[j])
                Team_[i].NavalUnits->Type[j]->SetCallback(cb);
}

void C_Map::SetAirUnitCallbacks(long type, void (*cb)(long, short, C_Base *))
{
    long i;

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].AirUnits->Type[type])
            Team_[i].AirUnits->Type[type]->SetCallback(cb);
}


void C_Map::SetGroundUnitCallbacks(long level, long type,
                                   void (*cb)(long, short, C_Base *))
{
    long i;

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units->Type[type])
            if (Team_[i].Units->Type[type]->Levels[level])
                Team_[i].Units->Type[type]->Levels[level]->SetCallback(cb);
}

void C_Map::SetNavalUnitCallbacks(long type, void (*cb)(long, short, C_Base *))
{
    long i;

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].NavalUnits->Type[type])
            Team_[i].NavalUnits->Type[type]->SetCallback(cb);
}

C_MapIcon *C_Map::GetObjIconList(long team, long type)
{
    return (Team_[team].Objectives->Type[type]);
}

void C_Map::AddListsToWindow()
{
    short i, j, k;
    long Font = 6;

    if (Map_ == NULL)
    {
        Map_ = new C_ScaleBitmap;
        Map_->Setup(5551000, 0, MapID);
    }

    DrawWindow_->AddControl(Map_);

    // Although The Threats are created here... they don't actually get put into a window

    if (not BullsEye_)
    {
        BullsEye_ = new C_BullsEye;
        BullsEye_->Setup(5550900, 0);
        BullsEye_->SetColor(0x005500);
        BullsEye_->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    DrawWindow_->AddControl(BullsEye_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Threats == NULL)
        {
            Team_[i].Threats = new THR_LIST;

            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
            {
                Team_[i].Threats->Type[j] = new C_Threat;
                Team_[i].Threats->Type[j]->Setup(0, 0);

                if (ThreatMask_ bitand (1 << j))
                {
                    Team_[i].Threats->Flags[j] = 1;
                    Team_[i].Threats->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].Threats->Flags[j] = 0;
                    Team_[i].Threats->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_THREATS_);
        }

        if (Team_[i].Objectives == NULL)
        {
            Team_[i].Objectives = new OBJ_LIST;

            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            {
                Team_[i].Objectives->Type[j] = new C_MapIcon;
                Team_[i].Objectives->Type[j]->Setup(5551000 + i + j * 10, j);
                Team_[i].Objectives->Type[j]->SetFont(Font);
                Team_[i].Objectives->Type[j]->SetTeam(i);
                Team_[i].Objectives->Type[j]->SetMainImage(ObjIconIDs_[i][0],
                                                           ObjIconIDs_[i][1]);

                if (j == FindTypeIndex(_UNIT_PACKAGE, OBJ_TypeList,
                                       _MAP_NUM_OBJ_TYPES_))
                    Team_[i].Objectives->Type[j]->SetMenu(PACKAGE_POP);
                else if (j == FindTypeIndex(_UNIT_SQUADRON, OBJ_TypeList,
                                            _MAP_NUM_OBJ_TYPES_))
                    Team_[i].Objectives->Type[j]->SetMenu(SQUADRON_POP);
                else
                    Team_[i].Objectives->Type[j]->SetMenu(OBJECTIVE_POP);

                Team_[i].Objectives->Type[j]->SetCursorID(CRSR_F16_RM);
                Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

                if (ObjectiveMask_ bitand (1 << j))
                {
                    Team_[i].Objectives->Flags[j] = 1;
                    Team_[i].Objectives->Type[j]->SetFlagBitOff(
                        C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].Objectives->Flags[j] = 0;
                    Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_OBJECTIVES_);
        }

        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            DrawWindow_->AddControl(Team_[i].Objectives->Type[j]);
    }

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].NavalUnits == NULL)
        {
            Team_[i].NavalUnits = new NAV_LIST;

            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            {
                Team_[i].NavalUnits->Type[j] = new C_MapIcon;
                Team_[i].NavalUnits->Type[j]->Setup(5551800 + i + j * 10, j);
                Team_[i].NavalUnits->Type[j]->SetMainImage(NavyIconIDs_[i][0],
                                                           NavyIconIDs_[i][1]);
                Team_[i].NavalUnits->Type[j]->SetFont(Font);
                Team_[i].NavalUnits->Type[j]->SetTeam(i);
                Team_[i].NavalUnits->Type[j]->SetMenu(NAVAL_POP);
                Team_[i].NavalUnits->Type[j]->SetCallback(UnitCB);
                Team_[i].NavalUnits->Type[j]->SetCursorID(CRSR_F16_RM);
                Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_DRAGABLE);
                else
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOff(C_BIT_DRAGABLE);

                if (NavalUnitMask_ bitand (1 << j))
                {
                    Team_[i].NavalUnits->Flags[j] = 1;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOff(
                        C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].NavalUnits->Flags[j] = 0;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_NAVAL_UNITS_);
        }

        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            DrawWindow_->AddControl(Team_[i].NavalUnits->Type[j]);
    }

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Units == NULL)
        {
            Team_[i].Units = new GND_LIST;

            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            {
                Team_[i].Units->Flags[j] = 0;
                Team_[i].Units->Type[j] = new GND_SIZE;

                for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                {
                    Team_[i].Units->Type[j]->Flags[k] = 0;
                    Team_[i].Units->Type[j]->Levels[k] = new C_MapIcon;
                    Team_[i].Units->Type[j]->Levels[k]->Setup(
                        static_cast<short>(5551300 + i + j * 10 + k * 100),
                        static_cast<short>(j + (k << 8)));
                    Team_[i].Units->Type[j]->Levels[k]->SetMainImage(
                        ArmyIconIDs_[i][0], ArmyIconIDs_[i][1]);
                    Team_[i].Units->Type[j]->Levels[k]->SetFont(Font);
                    Team_[i].Units->Type[j]->Levels[k]->SetTeam(i);
                    Team_[i].Units->Type[j]->Levels[k]->SetMenu(UNIT_POP);
                    Team_[i].Units->Type[j]->Levels[k]->SetCursorID(
                        CRSR_F16_RM);
                    Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(
                        C_BIT_NOLABEL);
                    Team_[i].Units->Type[j]->Levels[k]->SetCallback(UnitCB);

                    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(
                            C_BIT_DRAGABLE);
                    else
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(
                            C_BIT_DRAGABLE);

                    if (UnitMask_ bitand (1 << j))
                    {
                        Team_[i].Units->Type[j]->Flags[k] = 1;
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(
                            C_BIT_INVISIBLE);
                    }
                    else
                    {
                        Team_[i].Units->Type[j]->Flags[k] = 0;
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(
                            C_BIT_INVISIBLE);
                    }
                }
            }

            SetUnitLevel(0);
            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_UNITS_);
        }

        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                DrawWindow_->AddControl(Team_[i].Units->Type[j]->Levels[k]);
    }

    if (CurIcons_ == NULL)
    {
        CurIcons_ = new C_DrawList;
        CurIcons_->Setup(5553000, 0);
    }

    DrawWindow_->AddControl(CurIcons_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Waypoints == NULL)
        {
            Team_[i].Waypoints = new C_Waypoint;
            Team_[i].Waypoints->Setup(5551272 + i, C_TYPE_DRAGXY);
            Team_[i].Waypoints->SetCallback(WaypointCB);
            Team_[i].Waypoints->SetFont(Font);
            Team_[i].Waypoints->SetFlagBitOn(C_BIT_TOP);
            Team_[i].Waypoints->SetCursorID(CRSR_STEERPOINT);
            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_WAYPOINTS_);
        }

        DrawWindow_->AddControl(Team_[i].Waypoints);
    }

    // Current Waypoints (After the other waypoints... so they show up infront... (behind the airplanes though))
    if (not CurWP_)
    {
        CurWP_ = new C_Waypoint;
        CurWP_->Setup(5555000, C_TYPE_DRAGXY);
        CurWP_->SetFlagBitOn(C_BIT_DRAGABLE);
        CurWP_->SetCallback(WaypointCB);
        CurWP_->SetFont(Font);
        CurWP_->SetFlagBitOn(C_BIT_TOP);
        CurWP_->SetCursorID(CRSR_STEERPOINT);
    }

    DrawWindow_->AddControl(CurWP_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].AirUnits == NULL)
        {
            Team_[i].AirUnits = new AIR_LIST;

            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            {
                Team_[i].AirUnits->Type[j] = new C_MapIcon;
                Team_[i].AirUnits->Type[j]->Setup(5551201 + i + j * 10, j);
                Team_[i].AirUnits->Type[j]->SetFont(Font);
                Team_[i].AirUnits->Type[j]->SetTeam(i);

                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_0, AirIconIDs_[i][0][0], AirIconIDs_[i][0][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_1, AirIconIDs_[i][1][0], AirIconIDs_[i][1][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_2, AirIconIDs_[i][2][0], AirIconIDs_[i][2][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_3, AirIconIDs_[i][3][0], AirIconIDs_[i][3][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_4, AirIconIDs_[i][4][0], AirIconIDs_[i][4][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_5, AirIconIDs_[i][5][0], AirIconIDs_[i][5][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_6, AirIconIDs_[i][6][0], AirIconIDs_[i][6][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(
                    C_STATE_7, AirIconIDs_[i][7][0], AirIconIDs_[i][7][1]);

                Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                    Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_DRAGABLE);
                else
                    Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_DRAGABLE);

                Team_[i].AirUnits->Type[j]->SetCursorID(CRSR_F16_RM);
                Team_[i].AirUnits->Type[j]->SetMenu(AIRUNIT_MENU);
                Team_[i].AirUnits->Type[j]->SetCallback(UnitCB);

                if (AirUnitMask_ bitand (1 << j))
                {
                    Team_[i].AirUnits->Flags[j] = 1;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].AirUnits->Flags[j] = 0;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_AIR_UNITS_);
        }

        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            DrawWindow_->AddControl(Team_[i].AirUnits->Type[j]);
    }

    if (not CurWPZ_)
    {
        CurWPZ_ = new C_Waypoint;
        CurWPZ_->Setup(5555000, C_TYPE_DRAGY);
        CurWPZ_->SetScaleType(1);
        CurWPZ_->SetWorldRange(LogMinX_, LogMinY_, LogMaxX_, LogMaxY_);
        CurWPZ_->SetFlagBitOn(C_BIT_DRAGABLE);
        CurWPZ_->SetCallback(WaypointCB);
        CurWPZ_->SetFont(Font);
        CurWPZ_->SetClient(1);
        CurWPZ_->SetFlagBitOn(C_BIT_TOP);
        CurWPZ_->SetCursorID(CRSR_STEERPOINT);
    }

    if (WPZWindow_)
        WPZWindow_->AddControl(CurWPZ_);
}

void C_Map::RemoveListsFromWindow()
{
    short i, j, k;

    if (DrawWindow_ == NULL)
        return;

    DrawWindow_->RemoveControl(Map_->GetID());

    if (BullsEye_)
        DrawWindow_->RemoveControl(BullsEye_->GetID());

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Objectives)
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Type[j])
                    DrawWindow_->RemoveControl(
                        Team_[i].Objectives->Type[j]->GetID());

        if (Team_[i].NavalUnits)
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Type[j])
                    DrawWindow_->RemoveControl(
                        Team_[i].NavalUnits->Type[j]->GetID());

        if (Team_[i].Units)
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                    for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                        if (Team_[i].Units->Type[j]->Levels[k])
                            DrawWindow_->RemoveControl(
                                Team_[i].Units->Type[j]->Levels[k]->GetID());

        if (Team_[i].AirUnits)
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Type[j])
                    DrawWindow_->RemoveControl(
                        Team_[i].AirUnits->Type[j]->GetID());

        if (Team_[i].Waypoints)
            DrawWindow_->RemoveControl(Team_[i].Waypoints->GetID());
    }

    if (CurIcons_ and DrawWindow_)
        DrawWindow_->RemoveControl(CurIcons_->GetID());

    if (CurWP_ and DrawWindow_)
        DrawWindow_->RemoveControl(CurWP_->GetID());

    if (CurWPZ_ and WPZWindow_)
        WPZWindow_->RemoveControl(CurWPZ_->GetID());
}

void C_Map::SetTeamScales()
{
    short i, j, k;

    if (BullsEye_)
        BullsEye_->SetScale(scale_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                Team_[i].Objectives->Type[j]->SetScaleFactor(scale_);
        }

        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                Team_[i].NavalUnits->Type[j]->SetScaleFactor(scale_);
        }

        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                    Team_[i].Units->Type[j]->Levels[k]->SetScaleFactor(scale_);
        }

        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                Team_[i].AirUnits->Type[j]->SetScaleFactor(scale_);
        }

        if (Team_[i].Waypoints)
            Team_[i].Waypoints->SetScaleFactor(scale_);
    }

    if (CurWP_)
        CurWP_->SetScaleFactor(scale_);
}

void C_Map::RecalcWaypointZs(long scaletype) // 1=Log, 2=straight
{
    if (scaletype == 1)
    {
        if (CurWPZ_)
        {
            CurWPZ_->SetWorldRange(LogMinX_, LogMinY_, LogMaxX_, LogMaxY_);
            CurWPZ_->SetScaleType(static_cast<short>(scaletype));
        }
    }
    else if (scaletype == 2)
    {
        if (CurWPZ_)
        {
            CurWPZ_->SetWorldRange(StrtMinX_, StrtMinY_, StrtMaxX_, StrtMaxY_);
            CurWPZ_->SetScaleType(static_cast<short>(scaletype));
        }
    }
}

void C_Map::DrawMap()
{
    short i, j, k;
    short x, y;

    if (flags_ bitand (I_NEED_TO_DRAW bitor I_NEED_TO_DRAW_MAP))
    {
        // 2002-04-16 MN update the bullseye location when it changed
        TheCampaign.GetBullseyeLocation(&x, &y);

        if (x not_eq BullsEyeX_ or y not_eq BullsEyeY_)
            SetBullsEye(x * FEET_PER_KM,
                        (TheCampaign.TheaterSizeY - y) * FEET_PER_KM);

        if (flags_ bitand I_NEED_TO_DRAW_MAP)
        {

            if (Map_)
            {
                Map_->Refresh();
            }

            flags_ xor_eq I_NEED_TO_DRAW_MAP;
        }

        if (BullsEye_)
            BullsEye_->Refresh();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_OBJECTIVES_) and Team_[i].Objectives)
            {
                for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                {
                    if (Team_[i].Objectives->Flags[j])
                        Team_[i].Objectives->Type[j]->Refresh();
                }
            }

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_NAVAL_UNITS_) and
                Team_[i].NavalUnits)
            {
                for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                {
                    if (Team_[i].NavalUnits->Flags[j])
                        Team_[i].NavalUnits->Type[j]->Refresh();
                }
            }

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_UNITS_) and Team_[i].Units)
            {
                for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                {
                    if (Team_[i].Units->Flags[j])
                    {
                        for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                        {
                            if (Team_[i].Units->Type[j]->Flags[k])
                                Team_[i].Units->Type[j]->Levels[k]->Refresh();
                        }
                    }
                }
            }

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_WAYPOINTS_) and Team_[i].Waypoints)
                Team_[i].Waypoints->Refresh();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_AIR_UNITS_) and Team_[i].AirUnits)
            {
                for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                {
                    if (Team_[i].AirUnits->Flags[j])
                        Team_[i].AirUnits->Type[j]->Refresh();
                }
            }

        if (CurIcons_)
            CurIcons_->Refresh();

        if (CurWP_)
            CurWP_->Refresh();

        flags_ xor_eq I_NEED_TO_DRAW;
    }
}
