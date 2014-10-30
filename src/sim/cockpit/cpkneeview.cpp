#include "stdafx.h"
#include "cpmanager.h"
#include "kneeboard.h"
#include "cpkneeview.h"
#include "dispcfg.h"
#include "otwdrive.h"
#include "simdrive.h" //MI
#include "navsystem.h" //MI
#include "Graphics/Include/renderow.h"
#include "Graphics/Include/TMap.h"
#include "brief.h"
#include "flight.h"
#include "aircrft.h"


// sfr: moved the f***** globals from kneeboard to kneeview
extern bool g_bRealisticAvionics; //MI
extern bool g_bINS; //MI

static const UInt32 WP_COLOR = 0xFF0000FF; // The color of the waypoint marks
static const float WP_SIZE = 0.03f; // The radius of the waypoint marker symbol
static const float  ORIDE_WP_SIZE = 0.08f; // The size of the override waypoint marker
static const UInt32 AC_COLOR = 0xFF00FFFF; // The color of the aircraft location marker
static const float AC_SIZE = 0.06f; // The radius of the aircraft location marker
static const float BORDER_PERCENT = 0.05f; // How much map to display outside the bounding box of the waypoints
static const float KNEEBOARD_SMALLEST_MAP_FRACTION = 4.0f; // What is the smallest fraction of the map we'll zoom to


CPKneeView::CPKneeView(ObjectInitStr *pobjectInitStr, KneeBoard *pboard) : CPObject(pobjectInitStr)
{
    mapImageBuffer = NULL;
    mpKneeBoard = pboard;

    Setup(
        &FalconDisplay.theDisplayDevice, mDestRect.top, mDestRect.left, mDestRect.bottom, mDestRect.right
    );
}

CPKneeView::~CPKneeView()
{
    Cleanup();
}

void CPKneeView::Setup(DisplayDevice *device, int top, int left, int bottom, int right)
{
    mpKneeBoard->Setup();
    dstRect.top = top;
    dstRect.left = left;
    dstRect.bottom = bottom;
    dstRect.right = right;

    srcRect.top = 0;
    srcRect.left = 0;
    srcRect.bottom = bottom - top;
    srcRect.right = right - left;

    // Setup our off screen map buffer and renderer
    MPRSurfaceType front = FalconDisplay.theDisplayDevice.IsHardware() ? VideoMem : SystemMem;
    mapImageBuffer = new ImageBuffer;
    mapImageBuffer->Setup(device, srcRect.right, srcRect.bottom, front, None);
    Render2D::Setup(mapImageBuffer);
}

void CPKneeView::Cleanup()
{
    if (mapImageBuffer)
    {
        mapImageBuffer->Cleanup();
        delete mapImageBuffer;
        mapImageBuffer = NULL;
    }

    Render2D::Cleanup();
    mpKneeBoard->Cleanup();
}

void CPKneeView::Refresh(SimVehicleClass *platform)
{
    RenderMap(platform);
}

void CPKneeView::Exec(SimBaseClass* pOwnship)
{
    mpOwnship = pOwnship;
    RenderMap((SimVehicleClass*)mpOwnship);
}

void CPKneeView::DisplayBlit(void)
{
    if (mpKneeBoard->GetPage() == KneeBoard::MAP)
    {
        // BLT in the map
        mpOTWImage->Compose(mapImageBuffer, &srcRect, &dstRect);
    }
}

void CPKneeView::DisplayDraw(void)
{
    // Set the viewport to the active region of our display
    RenderOTW *renderer = OTWDriver.renderer;
    renderer->SetViewport(
        (float)dstRect.left  / mpOTWImage->targetXres() * (2.0f) - 1.0f,
        (float)dstRect.top   / mpOTWImage->targetYres() * (-2.0f) + 1.0f,
        (float)dstRect.right / mpOTWImage->targetXres() * (2.0f) - 1.0f,
        (float)dstRect.bottom / mpOTWImage->targetYres() * (-2.0f) + 1.0f
    );

    if (mpKneeBoard->GetPage() == KneeBoard::MAP)
    {
        // If we're not in Realistic mode, draw the current position marker
        // M.N. Added Full realism mode
        if (PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
        {
            DrawCurrentPosition(mpOTWImage, renderer, (SimVehicleClass*)mpOwnship);
        }
    }
    else
    {
        DrawMissionText(renderer, (SimVehicleClass*)mpOwnship);
    }
}


// sfr moved functions to here
void CPKneeView::DrawMissionText(Render2D *renderer, SimVehicleClass *platform)
{
    // sfr: check at beginning
    if (SimDriver.RunningDogfight() or SimDriver.RunningInstantAction())
    {
        return;
    }

    float LINE_HEIGHT = renderer->TextHeight();
    int lines;
    float v = 0.80f;
    int oldFont = VirtualDisplay::CurFont();
    DWORD iColor = OTWDriver.pCockpitManager->ApplyLighting(0xFF000000 , false);
    renderer->SetColor(iColor); // Black (ink color)

    VirtualDisplay::SetFont(OTWDriver.pCockpitManager->KneeFont());

    // Display the players call sign and assignment
    char string[1024];

    if (mpKneeBoard->GetPage() == KneeBoard::STEERPOINT)   // JPO new kneeboard page
    {
        v = 0.95f - LINE_HEIGHT;

        if (GetBriefingData(GBD_PACKAGE_STPTHDR, 0, string, sizeof(string)) not_eq -1)
        {
            renderer->TextLeft(-0.95f, v, string);
            v -= LINE_HEIGHT;
        }

        for (lines = 0; GetBriefingData(GBD_PACKAGE_STPT, lines, string, sizeof(string)) not_eq -1; ++lines)
        {
            renderer->TextLeft(-0.95f, v, string);
            v -= LINE_HEIGHT;
        }

        //MI display GPS coords when on ground, for INS alignment stuff
        if (g_bRealisticAvionics and g_bINS)
        {
            v -= 2 * LINE_HEIGHT;

            if (((AircraftClass*)SimDriver.GetPlayerEntity()) and ((AircraftClass*)SimDriver.GetPlayerEntity())->OnGround())
            {
                char latStr[20] = "";
                char longStr[20] = "";
                char tempstr[10] = "";
                float latitude = (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
                float cosLatitude = (float)cos(latitude);
                float longitude = ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + cockpitFlightData.y) / (EARTH_RADIUS_FT * cosLatitude);

                latitude *= RTD;
                longitude *= RTD;

                int longDeg = FloatToInt32(longitude);
                float longMin = (float)fabs(longitude - longDeg) * DEG_TO_MIN;

                int latDeg = FloatToInt32(latitude);
                float latMin = (float)fabs(latitude - latDeg) * DEG_TO_MIN;

                // format lat/long here
                if (latMin < 10.0F)
                {
                    sprintf(latStr, "LAT  N %3d\x03 0%2.2f\'\n", latDeg, latMin);
                }
                else
                {
                    sprintf(latStr, "LAT  N %3d\x03 %2.2f\'\n", latDeg, latMin);
                }

                if (longMin < 10.0F)
                {
                    sprintf(longStr, "LNG  E %3d\x03 0%2.2f\'\n", longDeg, longMin);
                }
                else
                {
                    sprintf(longStr, "LNG  E %3d\x03 %2.2f\'\n", longDeg, longMin);
                }

                renderer->TextLeft(-0.95F, v, latStr);
                v -= LINE_HEIGHT;
                renderer->TextLeft(-0.95F, v, longStr);
                v -= LINE_HEIGHT;
                sprintf(tempstr, "SALT %dFT", (long) - cockpitFlightData.z);
                renderer->TextLeft(-0.95F, v, tempstr);
            }
        }
    }
    else
    {
        if (GetBriefingData(GBD_PLAYER_ELEMENT, 0, string, sizeof(string)) not_eq -1)
        {
            renderer->TextLeft(-0.9f, v, string);
            v -= LINE_HEIGHT;
        }

        if (GetBriefingData(GBD_PLAYER_TASK,    0, string, sizeof(string)) not_eq -1)
        {
            lines = renderer->TextWrap(-0.8f, v, string, LINE_HEIGHT, 1.7f);
            v -= (lines + 1) * LINE_HEIGHT;
        }

        // Display the package info (if we are part of a package)
        if (GetBriefingData(GBD_PACKAGE_LABEL, 0, string, sizeof(string)) not_eq -1)
        {
            renderer->TextLeft(-0.9f, v, string);
            v -= LINE_HEIGHT;

            // Package mission statement
            if (GetBriefingData(GBD_PACKAGE_MISSION, 0, string, sizeof(string)) not_eq -1)
            {
                lines = renderer->TextWrap(-0.8f, v, string, LINE_HEIGHT, 1.7f);
                v -= lines * LINE_HEIGHT;
            }

            // List the flights in the package
            lines = 0;

            while (GetBriefingData(GBD_PACKAGE_ELEMENT_NAME, lines, string, sizeof(string)) not_eq -1)
            {
                renderer->TextLeft(-0.8f, v, string);

                if (GetBriefingData(GBD_PACKAGE_ELEMENT_TASK, lines, string, sizeof(string)) not_eq -1)
                    renderer->TextLeft(-0.1f, v, string);

                lines ++;
                v -= LINE_HEIGHT;
            }
        }
    }

    VirtualDisplay::SetFont(oldFont);
}

void CPKneeView::UpdateMapDimensions(SimVehicleClass *platform)
{

    WayPointClass* wp;
    float x, y, z;
    float left, right, top, bottom;

    m_pixel2nmY = (TheMap.NorthEdge() - TheMap.SouthEdge()) * FT_TO_KM;
    m_pixel2nmX = (TheMap.EastEdge() - TheMap.WestEdge()) * FT_TO_KM;

    CImageFileMemory &mapImageFile = mpKneeBoard->GetImageFile();
    m_pixel2nmY /= mapImageFile.image.height;
    m_pixel2nmX /= mapImageFile.image.width;
    // Start with our current location
    top   = bottom  = platform->XPos();
    right = left    = platform->YPos();

    // Walk the waypoints and get min/max info
    for (wp = platform->waypoint; wp; wp = wp->GetNextWP())
    {
        wp->GetLocation(&x, &y, &z);

        right = max(right, y);
        left = min(left, y);
        top = max(top, x);
        bottom = min(bottom, x);
    }

    // Add the position of the override waypoint (if any)
    ShiAssert(platform->GetCampaignObject());
    ShiAssert(platform->GetCampaignObject()->IsFlight());
    wp = ((FlightClass*)platform->GetCampaignObject())->GetOverrideWP();

    if (wp)
    {
        wp->GetLocation(&x, &y, &z);

        right = max(right, y);
        left = min(left, y);
        top = max(top, x);
        bottom = min(bottom, x);
    }

    // Now get the center of the map we want to display
    wsHcenter = (right + left) * 0.5f;
    wsVcenter = (top + bottom) * 0.5f;

    // Now figure out the minimum width and height we want (as a distance for center for now)
    wsHsize = (right - left) * 0.5f * (1.0f + BORDER_PERCENT);
    wsVsize = (top - bottom) * 0.5f * (1.0f + BORDER_PERCENT);

    if (wsHsize >= TheMap.EastEdge() - TheMap.WestEdge())
    {
        wsHsize = TheMap.EastEdge() - TheMap.WestEdge() - 1.0f; // -1 is for rounding safety...
    }

    if (wsVsize >= TheMap.NorthEdge() - TheMap.SouthEdge())
    {
        wsVsize = TheMap.NorthEdge() - TheMap.SouthEdge() - 1.0f; // -1 is for rounding safety...
    }

    // See how many source pixels we're talking about and round down to an even divisor of the dest pixels
    float hSourcePixels = wsHsize * 2.0f * FT_TO_KM / m_pixel2nmX;
    float vSourcePixels = wsVsize * 2.0f * FT_TO_KM / m_pixel2nmY;

    float hPixelMag = srcRect.right  / hSourcePixels;
    float vPixelMag = srcRect.bottom / vSourcePixels;

    // Cap the pixel magnification at a reasonable level
    float mapPixels = (TheMap.NorthEdge() - TheMap.SouthEdge()) * FT_TO_KM / m_pixel2nmY;
    float drawPixels = (float)srcRect.bottom;
    float maxMag = drawPixels / mapPixels * KNEEBOARD_SMALLEST_MAP_FRACTION;
    pixelMag = FloatToInt32((float)floor(min(min(hPixelMag, vPixelMag), maxMag)));

    // Detect the case where the whole desired image won't fit on screen
    if (pixelMag < 1)
    {
        pixelMag = 1;

        // Recenter on the current position to ensure its visible
        wsHcenter = platform->YPos();
        wsVcenter = platform->XPos();
    }

    // Now readjust our world space dimensions to reflect what we'll actually draw
    wsHsize = 0.5f * srcRect.right  / (float)pixelMag * m_pixel2nmX * KM_TO_FT;
    wsVsize = 0.5f * srcRect.bottom / (float)pixelMag * m_pixel2nmY * KM_TO_FT;

    // Finally shift the center point as necessary to ensure we won't try to draw off the edge
    if (wsHcenter - wsHsize <= TheMap.WestEdge())
    {
        wsHcenter = TheMap.WestEdge() + wsHsize + 0.5f; // +1/2 is for rounding safety...
    }

    if (wsHcenter + wsHsize >= TheMap.EastEdge())
    {
        wsHcenter = TheMap.EastEdge() - wsHsize - 0.5f; // -1/2 is for rounding safety...
    }

    if (wsVcenter - wsVsize <= TheMap.SouthEdge())
    {
        wsVcenter = TheMap.SouthEdge() + wsVsize + 0.5f; // +1/2 is for rounding safety...
    }

    if (wsVcenter + wsVsize >= TheMap.NorthEdge())
    {
        wsVcenter = TheMap.NorthEdge() - wsVsize - 0.5f; // -1/2 is for rounding safety...
    }
}


void CPKneeView::RenderMap(SimVehicleClass *platform)
{
    // Recompute what portion of the world map we need to draw
    UpdateMapDimensions(platform);

    // Copy the map image into the target buffer
    DrawMap();

    // OW FIXME: the following StartFrame() call will result in a call to IDirect3DDevice7::SetRenderTarget. We can't do this on the Voodoo 1 bitand 2 ;(
    DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(DisplayOptions.DispVideoDriver);

    if ( not pDI->SupportsSRT())
        return;

    // Draw in the waypoints
    StartDraw();
    DrawWaypoints(platform);
    EndDraw();
}


void CPKneeView::DrawMap()
{
    //pmvstrm
    bool m_imageloaded = false;
    ShiAssert(m_imageloaded);

    if (mpKneeBoard == NULL)
    {
        return;
    }

    //light factors
    float eLight[3], iLight[3]; //iLight is not used here...
    OTWDriver.pCockpitManager->ComputeLightFactors(eLight, iLight);

    //we load map palette and apply lighting to it
    CImageFileMemory &mapImageFile = mpKneeBoard->GetImageFile();
    DWORD *inColor = mapImageFile.image.palette;
    DWORD outColor[256];
    ApplyLightingToPalette(inColor, outColor, eLight[0], eLight[1], eLight[2]);

    //now we apply lighting to palette and copy only the portion we want from the map
    int w = mapImageFile.image.width;
    int h = mapImageFile.image.height;

    // Decide where to start in the source image
    int srcRowInitOffset = (int)((TheMap.NorthEdge() - (wsVsize + wsVcenter)) * FT_TO_KM / m_pixel2nmX);
    int srcColInitOffset = (int)((wsHcenter - wsHsize)                        * FT_TO_KM / m_pixel2nmY);

    // Lock the back buffer
    DWORD *ptr = (DWORD*)mapImageBuffer->Lock();

    //here we get a pointer to the initial position of the map(0,0)
    UInt8 *mapFirstPointer = mapImageFile.image.image;
    mapFirstPointer += 0;//srcRowInitOffset*w + srcColInitOffset;

    //some auxiliary variables
    int dstWidth = dstRect.right - dstRect.left;
    int dstHeight = dstRect.bottom - dstRect.top;

    // Copy from map to kneeboard
    for (int dstRow = 0; dstRow < (dstRect.bottom - dstRect.top); dstRow++)
    {
        //find the first pointer of that row in the map
        UInt8 *rowFirstPointer = mapFirstPointer + (dstRow + srcRowInitOffset) * w + srcColInitOffset;
        //first destination pointer
        DWORD *dst = (DWORD*)mapImageBuffer->Pixel(ptr, dstRow, 0);

        for (int dstCol = 0; dstCol < (dstRect.right - dstRect.left); dstCol++)
        {
            //this points to the pixel
            UInt8 *pixelPointer = rowFirstPointer + dstCol;

            if (((srcRowInitOffset + dstRow) >= h) or ((srcRowInitOffset + dstRow) < 0) or
                ((srcColInitOffset + dstCol) >= w) or ((srcColInitOffset + dstCol) < 0))
            {
                //we use a transparent pixel...
                dst[0] = 0xff000000;
            }
            else
            {
                //this is destination pixel
                dst[0] = InvertRGBOrder(outColor[*pixelPointer]);
            }

            dst++;
        }
    }

    // Release the source image and unlock the target surface
    mapImageBuffer->Unlock();
    return;
}

void CPKneeView::DrawWaypoints(SimVehicleClass *platform)
{
    WayPointClass* wp = NULL;
    BOOL isFirst = TRUE;
    float x1 = 0.0F, y1 = 0.0F, x2 = 0.0F, y2 = 0.0F;


    DWORD color =  OTWDriver.pCockpitManager->ApplyLighting(WP_COLOR, false);
    //OTWDriver.renderer->SetColor(color);
    SetColor(color);

    // Draw in the waypoints
    for (wp = platform->waypoint; wp; wp = wp->GetNextWP())
    {

        // Get the display coordinates of this waypoint
        MapWaypointToDisplay(wp, &x1, &y1);

        // Draw the waypoint marker and the connecting line if this isn't the first one
        Circle(x1, y1, WP_SIZE);

        if ( not isFirst)
        {
            Line(x1, y1, x2, y2);
        }

        // Step to the next waypoint
        x2 = x1;
        y2 = y1;
        isFirst = FALSE;
    }

    // Draw the override waypoint marker (if any)
    ShiAssert(platform->GetCampaignObject());
    ShiAssert(platform->GetCampaignObject()->IsFlight());
    wp = ((FlightClass*)platform->GetCampaignObject())->GetOverrideWP();

    if (wp)
    {

        // Get the display coordinates of this waypoint
        MapWaypointToDisplay(wp, &x1, &y1);
        x2 = x1 + ORIDE_WP_SIZE;
        y2 = y1 + ORIDE_WP_SIZE;
        x1 = x1 - ORIDE_WP_SIZE;
        y1 = y1 - ORIDE_WP_SIZE;

        // Draw the waypoint marker
        Line(x1, y1, x2, y1);
        Line(x2, y1, x2, y2);
        Line(x2, y2, x1, y2);
        Line(x1, y2, x1, y1);
    }
}


void CPKneeView::DrawCurrentPosition(ImageBuffer *targetBuffer, Render2D *renderer, SimVehicleClass *platform)
{
    const float aspect = (float)srcRect.right / (float)srcRect.bottom;
    float h, v;
    mlTrig trig;
    static const struct
    {
        float x, y;
    } pos[] =
    {
        0.0f,   1.0f, // nose
        0.0f,  -1.0f, // tail
        -1.0f,  -0.4f, // left wing tip
        1.0f,  -0.4f, // right wing tip
        0.0f,   0.3f, // leading edge at fuselage
        -0.5f,  -1.0f, // left stab
        -0.5f,  -1.0f, // right stab
    };
    static const int numPoints = sizeof(pos) / sizeof(pos[0]);
    float x[numPoints];
    float y[numPoints];

    // Convert our position into display space within the destination rect
    v = (platform->XPos() - wsVcenter) / wsVsize;
    h = (platform->YPos() - wsHcenter) / wsHsize;

    if (fabs(v) > 0.95f)
    {
        if (v > 0.95f)
        {
            v =  0.95f;
        }
        else
        {
            v = -0.95f;
        }

        if (vuxRealTime bitand 0x200)
        {
            // Don't draw to implement a flashing icon when the real postion is off screen.
            return;
        }
    }

    if (fabs(h) > 0.95f)
    {
        if (h > 0.95f)
        {
            h =  0.95f;
        }
        else
        {
            h = -0.95f;
        }

        if (vuxRealTime bitand 0x200)
        {
            // Don't draw to implement a flashing icon when the real postion is off screen.
            return;
        }
    }

    // Setup local coordinates for the symbol drawing
    renderer->CenterOriginInViewport();
    renderer->AdjustOriginInViewport(h, v);

    // Transform all our verts
    mlSinCos(&trig, platform->Yaw());
    trig.sin *= AC_SIZE;
    trig.cos *= AC_SIZE;

    for (int i = numPoints - 1; i >= 0; i--)
    {
        x[i] = (pos[i].x * trig.cos + pos[i].y * trig.sin);
        y[i] = (-pos[i].x * trig.sin + pos[i].y * trig.cos) * aspect;
    }

    // Draw the aircraft symbol
    DWORD acColor = OTWDriver.pCockpitManager->ApplyLighting(AC_COLOR, false);
    renderer->SetColor(acColor);
    renderer->Line(x[0], y[0], x[1], y[1]); // Body
    renderer->Line(x[5], y[5], x[6], y[6]); // Tail
    renderer->Tri(x[2], y[2], x[3], y[3], x[4], y[4]); // Wing

    // Draw a ring around the aircraft symbol to highlight it
    DWORD rColor = OTWDriver.pCockpitManager->ApplyLighting(0xFF00FF00, false);
    renderer->SetColor(rColor);
    renderer->Circle(0.0f, 0.0f, 1.5f * AC_SIZE);
}


void CPKneeView::MapWaypointToDisplay(WayPointClass *pwaypoint, float *h, float *v)
{
    float wpX;
    float wpY;
    float wpZ;

    pwaypoint->GetLocation(&wpX, &wpY, &wpZ);

    // Return values are in screen space, while the waypoint location is in FreeFalcon (X North, Y East)
    *v = (wpX - wsVcenter) / wsVsize;
    *h = (wpY - wsHcenter) / wsHsize;
}
