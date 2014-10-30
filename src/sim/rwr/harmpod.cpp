//****************************************************
// RV - I-Hawk
// Starting HARM avionics systems upgrade 07/2008
//****************************************************

#include "stdhdr.h"
#include "F4Vu.h"
#include "missile.h"
#include "MsgInc/TrackMsg.h"
#include "Graphics/Include/display.h"
#include "simveh.h"
#include "airunit.h"
#include "simdrive.h"
#include "aircrft.h"
#include "sms.h"
#include "object.h"
#include "team.h"
#include "entity.h"
#include "simmover.h"
#include "soundfx.h"
#include "classtbl.h"
#include "radar.h"
#include "handoff.h"
#include "HarmPod.h"
#include "otwdrive.h"

#include "simio.h"  // MD -- 20040111: added for analog cursor support

const float RANGE_POSITION = -0.8f; // offset from LHS of MFD
const float RANGE_MIDPOINT =  0.45F; // where the text should appear
const float CURSOR_SIZE =  0.05f;
extern float g_fCursorSpeed;

// RV - I-Hawk - Some helping functions
void GetCurWezAngle(int i, float &angleX, float &angleY, float &offsetX, float &offsetY);
void GetCurWezValue(int i, float &curX, float &curY, float &nextX, float &nextY);

int HarmTargetingPod::flash = FALSE; // Controls flashing display elements


HarmTargetingPod::HarmTargetingPod(int idx, SimMoverClass* self) : RwrClass(idx, self)
{
    sensorType = HTS;
    cursorX = 0.0F;
    cursorY = 0.0F;
    displayRange = 60;
    trueDisplayRange = 60;
    zoomFactor = 1.0f;
    HadZoomFactor = 1.0f;
    zoomMode = Wide;
    HadZoomMode = NORM;
    preHandoffMode = Has;
    filterMode = ALL;
    curTarget = NULL;
    POSTargetIndex = -1;
    handedoff = false;
    handoffRefTime = 0x0;
    HASTimer = 0x0;
    HASNumTargets = 0;
    HadOrigCursorX = 0.0f;
    HadOrigCursorY = 0.0f;
    HadOrigCursorX2 = 0.0f;
    HadOrigCursorY2 = 0.0f;
    yawBackup = 0.0f;

    ClearDTSB();
    ClearPOSTargets();
}

HarmTargetingPod::~HarmTargetingPod(void)
{
}

void HarmTargetingPod::LockTargetUnderCursor(void)
{
    FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();
    GroundListElement *choice;

    // RV - I-Hawk - Seperating lock modes by system submode, as HAS using elevation for position
    // calculation, and HAD using range (and has some special expended modes)
    if (submode == HAS)
    {
        choice = FindHASTargetUnderCursor();
    }

    else // submode = HAD
    {
        choice = FindTargetUnderCursor();
    }

    // Will lock the sensor on the related entity (no change if NULL)
    LockListElement(choice);
}

// RV - I-Hawk - Special function to lock targets from the POS mode PB targets list
void HarmTargetingPod::LockPOSTarget(void)
{
    GroundListElement *choice = FindPOSTarget();

    handedoff = false; // Reset the handed off flag
    handoffRefTime = SimLibElapsedTime; // assign the current time to the Handoff timer

    // Will lock the sensor on the related entity (no change if NULL)
    if (choice)
    {
        LockListElement(choice);
    }

    else
    {
        FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();

        if (FCC)
        {
            FCC->dropTrackCmd = TRUE;
        }
    }
}

void HarmTargetingPod::BoresightTarget(void)
{
    GroundListElement *tmpElement;
    GroundListElement *choice = NULL;
    float bestSoFar = 0.5f;
    float dx, dy, dz;
    float cosATA;
    float displayX, displayY;
    mlTrig trig;


    // Convienience synonym for the "At" vector of the platform...
    const float atx = platform->dmx[0][0];
    const float aty = platform->dmx[0][1];
    const float atz = platform->dmx[0][2];

    // Set up the trig functions of our current heading
    mlSinCos(&trig, platform->Yaw());

    FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();

    // Walk our list looking for the thing in range and nearest our nose
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement -> BaseObject() == NULL) continue;

        // Figure the relative geometry we need
        dx = tmpElement->BaseObject()->XPos() - platform->XPos();
        dy = tmpElement->BaseObject()->YPos() - platform->YPos();
        dz = tmpElement->BaseObject()->ZPos() - platform->ZPos();
        cosATA = (atx * dx + aty * dy + atz * dz) / (float)sqrt(dx * dx + dy * dy + dz * dz);

        // Rotate it into heading up space
        displayX = trig.cos * dy - trig.sin * dy;
        displayY = trig.sin * dx + trig.cos * dx;

        // Scale and shift for display range
        displayX *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        displayY *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        displayY += HTS_Y_OFFSET;

        //if ((fabs(displayX) > 1.0f) or (fabs(displayY) > 1.0f))
        // RV - I-Hawk - Diplay only what's inside the ALIC video
        if ( not IsInsideALIC(displayX, displayY))
        {
            continue;
        }

        // Is it our best candidate so far?
        if (cosATA > bestSoFar)
        {
            bestSoFar = cosATA;
            choice = tmpElement;
        }
    }

    // Will lock the sensor on the related entity (no change if NULL)
    LockListElement(choice);
}

void HarmTargetingPod::NextTarget(void)
{
    GroundListElement *tmpElement;
    GroundListElement *choice = NULL;
    float bestSoFar = 1e6f;
    float dx, dy;
    float displayX, displayY;
    mlTrig trig;
    float range;
    float currentRange;
    GroundListElement *currentElement;


    // Get data on our current target (if any)
    if (lockedTarget)
    {
        currentElement = FindEmmitter(lockedTarget->BaseData());
        dx = currentElement->BaseObject()->XPos() - platform->XPos();
        dy = currentElement->BaseObject()->YPos() - platform->YPos();
        currentRange = (float)sqrt(dx * dx + dy * dy);
    }
    else
    {
        currentElement = NULL;
        currentRange = -1.0f;
    }

    // Set up the trig functions of our current heading
    mlSinCos(&trig, platform->Yaw());

    FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();

    // Walk our list looking for the nearest thing in range but farther than our current target
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement -> BaseObject() == NULL) continue;

        // Figure the relative geometry we need
        dx = tmpElement->BaseObject()->XPos() - platform->XPos();
        dy = tmpElement->BaseObject()->YPos() - platform->YPos();

        // Rotate it into heading up space
        displayX = trig.cos * dy - trig.sin * dy;
        displayY = trig.sin * dx + trig.cos * dx;

        // Scale and shift for display range
        displayX *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        displayY *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        displayY += HTS_Y_OFFSET;

        //if ((fabs(displayX) > 1.0f) or (fabs(displayY) > 1.0f))
        // RV - I-Hawk - Diplay only what's inside the ALIC video
        if ( not IsInsideALIC(displayX, displayY))
        {
            continue;
        }

        range = (float)sqrt(dx * dx + dy * dy);

        // Skip it if its too close
        if (range < currentRange)
        {
            continue;
        }

        // Is it our best candidate so far?
        if (range < bestSoFar)
        {
            // Don't choose the same one we've already got
            if (tmpElement not_eq currentElement)
            {
                bestSoFar = range;
                choice = tmpElement;
            }
        }
    }

    // Will lock the sensor on the related entity (no change if NULL)
    LockListElement(choice);
}

void HarmTargetingPod::PrevTarget(void)
{
    GroundListElement *tmpElement;
    GroundListElement *choice = NULL;
    float bestSoFar = -1.0f;
    float dx, dy;
    float displayX, displayY;
    mlTrig trig;
    float range;
    float currentRange;
    GroundListElement *currentElement;

    // Get data on our current target (if any)
    if (lockedTarget)
    {
        currentElement = FindEmmitter(lockedTarget->BaseData());
        dx = currentElement->BaseObject()->XPos() - platform->XPos();
        dy = currentElement->BaseObject()->YPos() - platform->YPos();
        currentRange = (float)sqrt(dx * dx + dy * dy);
    }
    else
    {
        currentElement = NULL;
        currentRange = 1e6f;
    }

    // Set up the trig functions of our current heading
    mlSinCos(&trig, platform->Yaw());

    FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();

    // Walk our list looking for the farthest thing in range but nearer than our current target
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement -> BaseObject() == NULL) continue;

        // Figure the relative geometry we need
        dx = tmpElement->BaseObject()->XPos() - platform->XPos();
        dy = tmpElement->BaseObject()->YPos() - platform->YPos();

        // Rotate it into heading up space
        displayX = trig.cos * dy - trig.sin * dy;
        displayY = trig.sin * dx + trig.cos * dx;

        // Scale and shift for display range
        displayX *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        displayY *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        displayY += HTS_Y_OFFSET;

        //if ((fabs(displayX) > 1.0f) or (fabs(displayY) > 1.0f))
        // RV - I-Hawk - Diplay only what's inside the ALIC video
        if ( not IsInsideALIC(displayX, displayY))
        {
            continue;
        }

        range = (float)sqrt(dx * dx + dy * dy);

        // Skip it if its too far
        if (range > currentRange)
        {
            continue;
        }

        // Is it our best candidate so far?
        if (range > bestSoFar)
        {
            // Don't choose the same one we've already got
            if (tmpElement not_eq currentElement)
            {
                bestSoFar = range;
                choice = tmpElement;
            }
        }
    }

    // Will lock the sensor on the related entity (no change if NULL)
    LockListElement(choice);
}

void HarmTargetingPod::SetDesiredTarget(SimObjectType* newTarget)
{
    FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();
    GroundListElement* tmpElement = FCC->GetFirstGroundElement();

    // Before we tell the sensor about the target, make sure we see it
    while (tmpElement)
    {
        if (tmpElement->BaseObject() == newTarget->BaseData())
        {
            // Okay, we found it, so take the lock
            SetSensorTarget(newTarget);
            break;
        }

        tmpElement = tmpElement->GetNext();
    }

    // NOTE: when called from the AI ground attack routine this will create the element if not found 
    if ( not tmpElement)
    {
        tmpElement = new GroundListElement(newTarget->BaseData());
        tmpElement->next = FCC->GetFirstGroundElement();
        FCC->grndlist = tmpElement;
        SetSensorTarget(newTarget);
    }
}

// RV - I-Hawk - HAD display
void HarmTargetingPod::HADDisplay(VirtualDisplay* activeDisplay)
{
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;

    display = activeDisplay;

    // Do we draw flashing things this frame ?
    flash = vuxRealTime bitand 0x200;

    display->SetColor(0xFF00FF00);

    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    // Ownship "airplane" marker
    DWORD tempColor = display->Color();
    display->SetColor(GetMfdColor(MFD_CYAN));
    static const float NOSE = 0.02f;
    static const float TAIL = 0.08f;
    static const float WING = 0.06f;
    static const float TAIL_WING = 0.02f;
    display->Line(-WING,   0.00f,  WING,  0.00f);
    display->Line(-TAIL_WING,   -0.08f,  TAIL_WING,  -0.08f);
    display->Line(0.00f, -TAIL,   0.00f,  NOSE);

    // JB Draw 180 degree lines
    //display->SetColor( 0xFF004000 );
    //display->Line((float) -.95, 0.0, -WING - WING, 0.0);
    //display->Line(WING + WING, 0.0, (float) .95, 0.0);
    //display->Arc(0.0, 0.0, WING + WING, 180 * DTR, 0);
    //display->SetColor( 0xFF00FF00 );

    // Diplay the HAD mode blue circles at 1/3s of current range
    float largeArcRad = 1.0f * HTS_DISPLAY_RADIUS ;
    float secondArcRad = 0.66f * HTS_DISPLAY_RADIUS;
    float firstArcRad =  0.33f * HTS_DISPLAY_RADIUS;

    display->SetColor(GetMfdColor(MFD_BLUE));
    display->Circle(0.0f, 0.0f, largeArcRad);
    display->Circle(0.0f, 0.0f, secondArcRad);
    display->Circle(0.0f, 0.0f, firstArcRad);

    display->Line(0.0f, firstArcRad, 0.0f, firstArcRad + 0.06f);
    display->Line(firstArcRad, 0.0f, firstArcRad + 0.06f, 0.0f);
    display->Line(0.0f, -firstArcRad, 0.0f, -(firstArcRad + 0.06f));
    display->Line(-firstArcRad, 0.0f, -(firstArcRad + 0.06f), 0.0f);

    display->SetColor(tempColor);
    display->AdjustOriginInViewport(0.0f, -HTS_Y_OFFSET);

    // Display the missile effective footprint
    ShiAssert(platform->IsAirplane());

    if (((AircraftClass*)platform)->Sms->curWeapon and 
        ((AircraftClass*)platform)->Sms->curWeaponType == wtAgm88)
    {
        ShiAssert(((AircraftClass*)platform)->Sms->curWeapon->IsMissile());
        DrawWEZ((MissileClass*)((AircraftClass*)platform)->Sms->GetCurrentWeapon());
    }

    // Draw the cursors
    tempColor = display->Color();
    display->SetColor(GetMfdColor(MFD_WHITE));
    display->AdjustOriginInViewport((cursorX * zoomFactor), (cursorY + HTS_Y_OFFSET) * zoomFactor);
    display->Line(-CURSOR_SIZE, CURSOR_SIZE, CURSOR_SIZE, CURSOR_SIZE);
    display->Line(-CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, -CURSOR_SIZE);
    display->CenterOriginInViewport();
    display->SetColor(tempColor);

    float x18, y18;
    float x19, y19;
    GetButtonPos(18, &x18, &y18);
    GetButtonPos(19, &x19, &y19);
    float ymid = y18 + (y19 - y18) / 2;

    // Add range and arrows
    char str[8];
    sprintf(str, "%3d", trueDisplayRange);
    ShiAssert(strlen(str) < sizeof(str));
    display->TextLeftVertical(x18, ymid, str);

    // up arrow
    display->AdjustOriginInViewport(x19 + arrowW, y19 + arrowH / 2);
    display->Tri(0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);

    // down arrow
    display->CenterOriginInViewport();
    display->AdjustOriginInViewport(x18 + arrowW, y18 - arrowH / 2);
    display->Tri(0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);

    display->CenterOriginInViewport();

    //MI
    if (g_bRealisticAvionics)
    {
        if (((AircraftClass*)platform)->Sms->curWeapon and ((AircraftClass*)platform)->Sms->curWeapon->IsMissile())
        {
            if (((AircraftClass*)SimDriver.GetPlayerAircraft())->GetSOI() == SimVehicleClass::SOI_WEAPON)
            {
                display->SetColor(GetMfdColor(MFD_GREEN));
                DrawBorder(); // JPO SOI
            }
        }
    }
}

// RV - I-Hawk - HAD special EXP zoom modes
void HarmTargetingPod::HADExpDisplay(VirtualDisplay* activeDisplay)
{
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;

    display = activeDisplay;

    // Do we draw flashing things this frame ?
    flash = vuxRealTime bitand 0x200;

    display->SetColor(0xFF00FF00);

    DWORD tempColor = display->Color();

    // Draw the cursors
    display->SetColor(GetMfdColor(MFD_WHITE));
    display->AdjustOriginInViewport((cursorX * zoomFactor), (cursorY + HTS_Y_OFFSET) * zoomFactor);
    display->Line(-CURSOR_SIZE, CURSOR_SIZE, CURSOR_SIZE, CURSOR_SIZE);
    display->Line(-CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, -CURSOR_SIZE);
    display->CenterOriginInViewport();
    display->SetColor(tempColor);

    display->CenterOriginInViewport();

    //MI
    if (g_bRealisticAvionics)
    {
        if (((AircraftClass*)platform)->Sms->curWeapon and ((AircraftClass*)platform)->Sms->curWeapon->IsMissile())
        {
            if (((AircraftClass*)SimDriver.GetPlayerAircraft())->GetSOI() == SimVehicleClass::SOI_WEAPON)
            {
                display->SetColor(GetMfdColor(MFD_GREEN));
                DrawBorder(); // JPO SOI
            }
        }
    }
}

// RV - I-Hawk - HAS video display. Showing symbols azimuth/elevation (not range)
void HarmTargetingPod::HASDisplay(VirtualDisplay* activeDisplay)
{
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;
    int minutes, seconds;

    display = activeDisplay;

    // Do we draw flashing things this frame ?
    flash = vuxRealTime bitand 0x200;

    display->SetColor(0xFF00FF00);

    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    DrawDTSBBox(); // RV - I-Hawk - Draw the DTSB box

    // RV - I-Hawk - Draw the ALIC video box
    DWORD tempColor = display->Color();
    display->SetColor(GetMfdColor(MFD_RED));

    float ALICSide   = ALICSIDE * zoomFactor;
    float ALICTop    = ALICTOP * zoomFactor;
    float ALICBottom = ALICBOTTOM * zoomFactor;

    // Lines are dashed
    float ALICSpace = ALICSide / 8.0f;

    for (float pos = -(ALICSide); pos < ALICSide ; pos += 2.0f * ALICSpace)
    {
        display->Line(pos,   ALICTop,  pos + ALICSpace,  ALICTop);
        display->Line(pos,   ALICBottom,  pos + ALICSpace,  ALICBottom);
    }

    ALICSpace = (ALICTop - ALICBottom) / 12.0f;

    for (float pos = ALICBottom; pos < (ALICTop - (0.0875f * zoomFactor)); pos += 2.0f * ALICSpace)
    {
        display->Line(-(ALICSide),   pos,  -(ALICSide) ,  pos + ALICSpace);
        display->Line(ALICSide,   pos,  ALICSide,  pos + ALICSpace);
    }

    // Draw the ALIC range lines
    display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
    static const float HorizontalRangeLine = (ALICTop / 11.0f) * 9.5f;
    static const float VerticalRangeLineTOP = (ALICTop + (0.05f * zoomFactor));
    static const float VerticalRangeLineBottom = (ALICBottom + (0.1f * zoomFactor));
    display->Line(ALICSide, HorizontalRangeLine, -ALICSide, HorizontalRangeLine);
    display->Line(0.0f, VerticalRangeLineTOP, 0.0f, VerticalRangeLineBottom);

    static const float horizontalLinesSpace = ALICSide / 3.0f;
    static const float verticalLinesSpace = (HorizontalRangeLine - VerticalRangeLineBottom) / 4.0f;

    for (float pos = -ALICSide; pos <= ALICSide; pos += horizontalLinesSpace)
    {
        if (fabs(pos) < (0.05f * zoomFactor)) continue;  // no need to draw at the middle

        display->Line(pos,   HorizontalRangeLine + (0.04f * zoomFactor),  pos ,  HorizontalRangeLine - (0.04f * zoomFactor));
    }

    for (float pos = VerticalRangeLineBottom + verticalLinesSpace; pos <= VerticalRangeLineTOP - (0.2f * zoomFactor); pos += verticalLinesSpace)
    {
        display->Line((0.04f * zoomFactor) ,   pos ,  -(0.04f * zoomFactor) ,  pos);
    }

    // Write SCT-1 near the top range line
    display->TextCenter(0.15f, VerticalRangeLineTOP + 0.05f, "SCT-1", 0);

    // Now we will draw the system worst scan time. each target takes 1 second until drawn, so
    // according to updated number of targets currently to be drawn we can decide search time
    int temp = HASNumTargets;
    minutes = temp / 60;
    temp -= minutes * 60;
    seconds = temp;
    char str[8];
    sprintf(str, "%d:%02d", minutes, seconds);
    display->TextCenter(-0.25f, VerticalRangeLineTOP + 0.05f, str, 0);

    display->SetColor(tempColor);
    display->AdjustOriginInViewport(0.0f, -HTS_Y_OFFSET);

    // Draw the cursors
    tempColor = display->Color();
    display->SetColor(GetMfdColor(MFD_WHITE));
    display->AdjustOriginInViewport(cursorX, cursorY + HTS_Y_OFFSET);
    display->Line(-CURSOR_SIZE, CURSOR_SIZE, CURSOR_SIZE, CURSOR_SIZE);
    display->Line(-CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, -CURSOR_SIZE);
    display->CenterOriginInViewport();
    display->SetColor(tempColor);

    display->CenterOriginInViewport();

    //MI
    if (g_bRealisticAvionics)
    {
        if (((AircraftClass*)platform)->Sms->curWeapon and ((AircraftClass*)platform)->Sms->curWeapon->IsMissile())
        {
            if (((AircraftClass*)SimDriver.GetPlayerAircraft())->GetSOI() == SimVehicleClass::SOI_WEAPON)
            {
                DrawBorder(); // JPO SOI
            }
        }
    }
}

// RV - I-Hawk - Handoff display. Moving here afetr locking a target in HAS mode
void HarmTargetingPod::HandoffDisplay(VirtualDisplay* activeDisplay)
{
    display = activeDisplay;

    // Do we draw flashing things this frame ?
    flash = vuxRealTime bitand 0x200;

    display->SetColor(0xFF00FF00);

    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    DrawDTSBBox(); // RV - I-Hawk - Draw the DTSB box

    DWORD tempColor = display->Color();

    // Draw the Handoff display lines

    float ALICSide   = ALICSIDE * zoomFactor;
    float ALICTop    = ALICTOP * zoomFactor;
    float ALICBottom = ALICBOTTOM * zoomFactor;

    display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
    static const float verticalHandoffLineTop = ALICTop + 0.05f;
    static const float verticalHandoffLineBottom = (ALICBottom + (0.1f * zoomFactor));
    static const float horizontalHandoffLine = (verticalHandoffLineTop + verticalHandoffLineBottom) / 2.0f;
    static const float horizontalEdges = ALICSide * 0.66f;
    float topLineoffset = horizontalHandoffLine + (0.05f * zoomFactor);
    float bottomLineoffset = horizontalHandoffLine - (0.05f * zoomFactor);

    // The lines
    display->Line(-(horizontalEdges), horizontalHandoffLine, -(0.05f * zoomFactor), horizontalHandoffLine);
    display->Line((horizontalEdges), horizontalHandoffLine, (0.05f * zoomFactor), horizontalHandoffLine);
    display->Line(0.0f, verticalHandoffLineTop, 0.0f, topLineoffset);
    display->Line(0.0f, verticalHandoffLineBottom, 0.0f, bottomLineoffset);

    // The small lines at the edges
    display->Line(-(0.05f * zoomFactor), verticalHandoffLineTop, (0.05f * zoomFactor), verticalHandoffLineTop);
    display->Line(-(0.05f * zoomFactor), verticalHandoffLineBottom, (0.05f * zoomFactor), verticalHandoffLineBottom);
    display->Line(-(horizontalEdges), topLineoffset, -(horizontalEdges), bottomLineoffset);
    display->Line((horizontalEdges), topLineoffset, (horizontalEdges), bottomLineoffset);

    if (handedoff == true)   // Write RDY only if info been handed off to the missile
    {
        // Write "READY" at bottom of display
        display->SetColor(GetMfdColor(MFD_WHITY_GRAY)); // "whity" gray...
        display->TextCenter(0.0F, -0.2F, "RDY");
    }

    display->SetColor(tempColor);
    display->AdjustOriginInViewport(0.0f, -HTS_Y_OFFSET);
    display->CenterOriginInViewport();

    //MI
    if (g_bRealisticAvionics)
    {
        if (((AircraftClass*)platform)->Sms->curWeapon and ((AircraftClass*)platform)->Sms->curWeapon->IsMissile())
        {
            if (((AircraftClass*)SimDriver.GetPlayerAircraft())->GetSOI() == SimVehicleClass::SOI_WEAPON)
            {
                DrawBorder(); // JPO SOI
            }
        }
    }
}

// RV - I-Hawk - POS mode display
void HarmTargetingPod::POSDisplay(VirtualDisplay* activeDisplay)
{
    display = activeDisplay;

    // Do we draw flashing things this frame ?
    flash = vuxRealTime bitand 0x200;

    display->SetColor(0xFF00FF00);

    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    DrawDTSBBox(); // RV - I-Hawk - Draw the DTSB box

    float ALICSide   = ALICSIDE * zoomFactor;
    float ALICTop    = ALICTOP * zoomFactor;
    float ALICBottom = ALICBOTTOM * zoomFactor;

    // Draw the LSDL line
    DWORD tempColor = display->Color();

    display->SetColor(GetMfdColor(MFD_BRIGHT_GREEN)); // Bright green
    float LDLVerticalPos = (ALICTop + ALICBottom) / 2.0f + (0.05f * zoomFactor);
    display->Line(-(0.85f * zoomFactor), LDLVerticalPos, (0.85f * zoomFactor), LDLVerticalPos);

    display->SetColor(tempColor);

    display->CenterOriginInViewport();

    //MI
    if (g_bRealisticAvionics)
    {
        if (((AircraftClass*)platform)->Sms->curWeapon and ((AircraftClass*)platform)->Sms->curWeapon->IsMissile())
        {
            if (((AircraftClass*)SimDriver.GetPlayerAircraft())->GetSOI() == SimVehicleClass::SOI_WEAPON)
            {
                DrawBorder(); // JPO SOI
            }
        }
    }
}

// RV - I-Hawk - Draw missile footprint (only in HAD mode)
void HarmTargetingPod::DrawWEZ(MissileClass *theMissile)
{
    float curX, curY, nextX, nextY, angleX, angleY, offsetX, offsetY;
    float cur2X, cur2Y, next2X, next2Y, stepX, stepY;

    // If we don't have a missile, quit now
    if ( not theMissile)
    {
        return;
    }

    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    // This would be very nice, but would require correct data (and a missile)
    float mxRng  =  theMissile->GetRMax(-platform->ZPos(), platform->GetVt(),   0.0f,     0.0f, 0.0f) * FT_TO_NM;
    float mnRng  = -theMissile->GetRMax(-platform->ZPos(), platform->GetVt(), 180.0f * DTR, 0.0f, 180.0f * DTR) * FT_TO_NM;
    float latRng =  theMissile->GetRMax(-platform->ZPos(), platform->GetVt(),  90.0f * DTR, 0.0f,  90.0f * DTR) * FT_TO_NM;
    float footprintRatio = 2.0f * latRng / (mxRng - mnRng);

    // Shrink the displayed WEZ a little to represent a good launch zone instead of strictly Rmax
    //mnRng *= 0.8f;

    // RV - I-Hawk - Make it a little bigger... it's too small
    mxRng *= 1.15f;

    // float footprintCtr =  (mxRng * ( 1.8f ))  / (2.0f * displayRange) ;
    float footprintRad = mxRng  / displayRange;

    // RV - I-Hawk - White color
    display->SetColor(GetMfdColor(MFD_WHITY_GRAY));   // MFD_WHITE

    // RV - I-Hawk - Changing the way the WEZ is draw. No more circles or arcs but interpolated
    // curves of a circle, but with offsets at some point...
    // If the WEZ is too big for the current display scale, draw at MFD edges and use dashes
    if (footprintRad > 0.7f)
    {
        footprintRad = 0.75f;
        display->SetColor(GetMfdColor(MFD_YELLOW));   // MFD_WHITE

        float scale = (float)(displayRange / 60);

        curX = 0.0f;
        curY = 0.0f;
        nextX = 0.0625f;
        nextY = -0.0625f;
        display->Line(curX, curY, nextX, nextY);
        display->Line(curX, curY, -nextX, nextY);

        for (int i = 0; i < 8; i++)
        {
            // get some preset values, as this footprint is constant
            GetCurWezValue(i, curX, curY, nextX, nextY);

            // Cutting the line into 4 pieces, to make it look dashed...
            stepX = (nextX - curX) / 4.0f;
            stepY = (nextY - curY) / 4.0f;
            cur2X = curX;
            cur2Y = curY;
            next2X = cur2X + stepX;
            next2Y = cur2Y + stepY;

            for (int j = 0; j < 4; j++)
            {
                int draw = j bitand 1;

                if (draw)
                {
                    display->Line(cur2X, cur2Y, next2X, next2Y);
                    display->Line(-cur2X, cur2Y, -next2X, next2Y);
                }

                cur2X = next2X;
                cur2Y = next2Y;
                next2X += stepX;
                next2Y += stepY;
            }
        }
    }
    else
    {
        //display->Circle (  0.0f, footprintCtr, /*footprintRatio*footprintRad,*/ footprintRad);

        // Some scaling values...
        float scaleMaxRange = mxRng / 25.5f; // Scale for missiles range compared to AGM-88 as a reference
        float scale = (float)((float)displayRange / 60.0f); // Scale for display range compared to 60NM

        curX = 0.0f;
        curY = 0.0f;
        nextX = (footprintRad / 8.0f);
        nextY = -(footprintRad / 8.0f);
        display->Line(curX, curY, nextX, nextY);
        display->Line(curX, curY, -nextX, nextY);
        curX = nextX;
        curY = nextY;

        float curAngle = 275.0f * DTR;

        for (int i = 0; i < 8; i++)
        {
            // Get preset trig values (already calculated by hand in such function), as the angles are always the same
            GetCurWezAngle(i, angleX, angleY, offsetX, offsetY);
            nextX = footprintRad * angleX + offsetX / scale * scaleMaxRange ;
            nextY = scaleMaxRange * (footprintRad * angleY + offsetY / scale - (HTS_Y_OFFSET / 1.25f / scale));

            display->Line(curX, curY, nextX, nextY);
            display->Line(-curX, curY, -nextX, nextY);
            curX = nextX;
            curY = nextY;
        }
    }

    // Restore the default full intensity green color
    display->SetColor(0xFF00FF00);
    display->AdjustOriginInViewport(0.0f, -HTS_Y_OFFSET);
}

// Make sure we have the latest data for each track, and age them if they get old
//SimObjectType* HarmTargetingPod::Exec (SimObjectType* targetList)
SimObjectType* HarmTargetingPod::Exec(SimObjectType*)
{
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
    GroundListElement* tmpElement;
    GroundListElement* prevElement = NULL;
    SimBaseClass* curSimObj;
    SimObjectLocalData* localData;
    VuListIterator emitters(EmitterList);
    int trackType;

    float xMove = 0.0F, yMove = 0.0F;

    if ((FCC->cursorXCmd not_eq 0) or (FCC->cursorYCmd not_eq 0))
        if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) and (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
        {
            yMove = (float)FCC->cursorYCmd / 10000.0F;
            xMove = (float)FCC->cursorXCmd / 10000.0F;
        }
        else
        {
            yMove = (float)FCC->cursorYCmd;
            xMove = (float)FCC->cursorXCmd;
        }

    // Validate our locked target
    CheckLockedTarget();

    // Cursor Control
    //MI
    if ( not g_bRealisticAvionics)
    {
        cursorX += xMove * g_fCursorSpeed * HTS_CURSOR_RATE * zoomFactor * SimLibMajorFrameTime;
        cursorY += yMove * g_fCursorSpeed * HTS_CURSOR_RATE * zoomFactor * SimLibMajorFrameTime;
    }

    else
    {
        if (((AircraftClass*)SimDriver.GetPlayerAircraft()) and ((AircraftClass*)SimDriver.GetPlayerAircraft())->GetSOI() == SimVehicleClass::SOI_WEAPON)
        {
            cursorX += xMove * g_fCursorSpeed * HTS_CURSOR_RATE * SimLibMajorFrameTime;
            cursorY += yMove * g_fCursorSpeed * HTS_CURSOR_RATE * SimLibMajorFrameTime;
        }
    }

    if (submode == HAS)
    {
        cursorX = min(max(cursorX, -(0.75f) * zoomFactor), (0.75f * zoomFactor));
        cursorY = min(max(cursorY, 0.0f), (1.05f * zoomFactor));
    }

    else if (submode == HAD)
    {
        cursorX = min(max(cursorX, -(0.95f)), 0.95f);
        cursorY = min(max(cursorY, -0.5f), 1.5f);
    }

    FCC->UpdatePlanned();
    tmpElement = FCC->GetFirstGroundElement();

    // Go through the list of emmitters and update the data
    while (tmpElement)
    {
        // Handle sim/camp hand off
        tmpElement->HandoffBaseObject();

        // Removed?  (We really shouldn't do this -- once detected, things shouldn't disappear, just never emit)
        if ( not tmpElement->BaseObject())
        {
            tmpElement = tmpElement->next;
            continue;
        }

        // Time out our guidance flag
        if (SimLibElapsedTime > tmpElement->lastHit + RadarClass::TrackUpdateTime * 2.5f)
        {
            tmpElement->ClearFlag(GroundListElement::Launch);
        }

        // Time out our track flag
        if (SimLibElapsedTime > tmpElement->lastHit + TRACK_CYCLE)
        {
            tmpElement->ClearFlag(GroundListElement::Track);
        }

        // Time out our radiate flag
        if (SimLibElapsedTime > tmpElement->lastHit + RADIATE_CYCLE)
        {
            tmpElement->ClearFlag(GroundListElement::Radiate);
        }

        prevElement = tmpElement;
        tmpElement = tmpElement->next;
    }

    // Walk our list marking things as unchecked
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        tmpElement->SetFlag(GroundListElement::UnChecked);
    }

    // Check the target list for 'pings'
    for (SimObjectType *curObj = platform->targetList, *next; curObj not_eq NULL; curObj = next)
    {
        next = curObj->next;
        // sfr: taking my chances
        /*if (F4IsBadReadPtr(curObj, sizeof(SimObjectType)) or F4IsBadCodePtr((FARPROC) curObj->BaseData()))
        {
         // JB 010318 CTD
         curObj = curObj->next;
         continue;
        } */

        if (curObj->BaseData()->IsSim() and curObj->BaseData()->GetRadarType())
        {
            // Localize the info
            curSimObj = (SimBaseClass*)curObj->BaseData();
            localData = curObj->localData;

            // Is it time to hear this one again?
            if (SimLibElapsedTime > localData->sensorLoopCount[HTS] + curSimObj->RdrCycleTime() * SEC_TO_MSEC)
            {
                // Can we hear it?
                if (BeingPainted(curObj) and CanDetectObject(curObj))
                {
                    ObjectDetected(curObj->BaseData(), Track_Ping);
                    localData->sensorLoopCount[HTS] = SimLibElapsedTime;
                }
            }
            else
            {
                tmpElement = FindEmmitter(curSimObj);

                if (tmpElement)
                {
                    tmpElement->ClearFlag(GroundListElement::UnChecked);
                }
            }
        }
    }

    // Check the emitter list
    for (CampBaseClass *curEmitter = (CampBaseClass*)emitters.GetFirst(), *next;
         curEmitter;
         curEmitter = next)
    {
        next = static_cast<CampBaseClass*>(emitters.GetNext());

        // Check if aggregated unit can detect
        if (
            curEmitter->IsAggregate() and // A campaign thing
            curEmitter->CanDetect(platform) and // That has us spotted
            curEmitter->GetRadarMode() not_eq FEC_RADAR_OFF and // And is emmitting
            // JB 011016 CanDetectObject (platform))          // And there is line of sight
            CanDetectObject(curEmitter)                       // And there is line of sight // JB 011016
        )
        {
            // What type of hit is this?
            switch (curEmitter->GetRadarMode())
            {
                case FEC_RADAR_SEARCH_1:
                case FEC_RADAR_SEARCH_2:
                case FEC_RADAR_SEARCH_3:
                case FEC_RADAR_SEARCH_100:
                    trackType = Track_Ping;
                    break;

                case FEC_RADAR_AQUIRE:
                    trackType = Track_Lock;
                    break;

                case FEC_RADAR_GUIDE:
                    trackType = Track_Launch;
                    break;

                default:
                    // Probably means its off...
                    trackType = Track_Unlock;
                    break;
            }

            // Add it to the list (if the list isn't full)
            ObjectDetected(curEmitter, trackType);
        }
    }

    // Walk our list looking for unchecked Sim things
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement->BaseObject() and 
            tmpElement->BaseObject()->IsSim() and 
            (tmpElement->IsSet(GroundListElement::UnChecked)) and 
            SimLibElapsedTime >
            tmpElement->lastHit + ((SimBaseClass*)(tmpElement->BaseObject()))->RdrCycleTime() * SEC_TO_MSEC
           )
        {
            curSimObj = (SimBaseClass*)(tmpElement->BaseObject());

            if (curSimObj->RdrRng())
            {
                float top, bottom;

                // Check LOS
                // See if the target is near the ground
                OTWDriver.GetAreaFloorAndCeiling(&bottom, &top);

                if (curSimObj->ZPos() < top or OTWDriver.CheckLOS(platform, curSimObj))
                {
                    ObjectDetected(curSimObj, Track_Ping);
                }
            }
        }
    }

    return NULL;
}

VU_ID HarmTargetingPod::FindIDUnderCursor(void)
{
    GroundListElement *choice = FindTargetUnderCursor();
    VU_ID tgtId = FalconNullId;

    if (choice and choice->BaseObject())
    {
        tgtId = choice->BaseObject()->Id();
    }

    return tgtId;
}

GroundListElement* HarmTargetingPod::FindTargetUnderCursor(void)
{
    GroundListElement *tmpElement;
    GroundListElement *choice = NULL;
    float bestSoFar = 10.0f;
    float x, y;
    float displayX, displayY;
    mlTrig trig;
    float delta;
    float EXP1OffsetX, EXP1OffsetY, EXP2OffsetX, EXP2OffsetY;

    if (HadZoomMode == NORM)
    {
        mlSinCos(&trig, platform->Yaw());
    }

    else if (HadZoomMode == EXP1 or HadZoomMode == EXP2)
    {
        mlSinCos(&trig, yawBackup);
    }

    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

    // Walk our list looking for the thing in range and nearest the center of the cursors
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement->BaseObject() == NULL) continue;

        if ( not IsInPriorityList(tmpElement->symbol))
        {
            continue;
        }

        switch (HadZoomMode)
        {
            case NORM:
            default:
                // Convert to normalized display space with heading up
                y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
                x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
                displayX = trig.cos * x - trig.sin * y;
                displayY = trig.sin * x + trig.cos * y;
                break;

                // RV - I-Hawk - if in zoom modes, do the right offsets
            case EXP1:
            case EXP2:
                // Compute the world space oriented, display space scaled, obacked up ownship relative position of the emitter
                y = (tmpElement->BaseObject()->XPos() - XPosBackup) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
                x = (tmpElement->BaseObject()->YPos() - YPosBackup) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;

                // Rotate it into heading up space and translate it down to deal with our vertical offset
                displayX = trig.cos * x - trig.sin * y;
                displayY = trig.sin * x + trig.cos * y;

                // Deal with the EXP1 offset
                EXP1OffsetX = 2.0f * HadOrigCursorX;
                EXP1OffsetY = 2.0f * HadOrigCursorY;

                displayX -= EXP1OffsetX;
                displayY -= EXP1OffsetY;

                displayY -= HTS_Y_OFFSET;

                // Here also add the EXP2 offset
                if (HadZoomMode == EXP2)
                {
                    EXP2OffsetX = 2.0f * HadOrigCursorX + 2.0f * HadOrigCursorX2;
                    EXP2OffsetY = 2.0f * HadOrigCursorY + 2.0f * HadOrigCursorY2;

                    EXP2OffsetY += 2.0f * HTS_Y_OFFSET;

                    if (displayRange < 10)
                    {
                        EXP2OffsetY -= HTS_Y_OFFSET / 2.0f;

                        if (displayRange < 5)
                        {
                            EXP2OffsetY -= HTS_Y_OFFSET / 2.0f;
                        }
                    }

                    displayX -= EXP2OffsetX;
                    displayY -= EXP2OffsetY;
                }

                break;
        }

        // See if this is the closest to the cursor point so far
        delta = (float)max(fabs(displayX - cursorX), fabs(displayY - cursorY));

        if (delta < bestSoFar)
        {
            bestSoFar = delta;
            choice = tmpElement;
        }
    }

    // See if this is inside the cursor region
    if (bestSoFar < (CURSOR_SIZE))
    {
        return choice;
    }
    else
    {
        return NULL;
    }
}

GroundListElement* HarmTargetingPod::FindHASTargetUnderCursor(void)
{
    GroundListElement *tmpElement;
    GroundListElement *choice = NULL;
    float bestSoFar = 10.0f;
    float x, y, x2, y2;
    float displayX, displayY;
    float alt, range, elevation, phi, alpha, ex, ey;
    mlTrig trig;
    float delta;

    mlSinCos(&trig, platform->Yaw());

    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

    // Walk our list looking for the thing in range and nearest the center of the cursors
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement->BaseObject() == NULL) continue;

        if ( not IsInPriorityList(tmpElement->symbol))
        {
            continue;
        }

        // Convert to normalized display space with heading up
        y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
        x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
        alt = (-platform->ZPos() - (-tmpElement->BaseObject()->ZPos())) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
        x2 = x * x;
        y2 = y * y;
        range = sqrt(x2 + y2);
        alpha = atan(alt / range);
        elevation = (1.0f - (alpha / (PI / 2.0f))) * HAS_DISPLAY_RADIUS;

        displayX = trig.cos * x - trig.sin * y;
        displayY = trig.sin * x + trig.cos * y;

        phi = atan(displayX / displayY);
        ex = elevation * sin(phi);
        ey = elevation * cos(phi);
        displayX = ex * zoomFactor;
        displayY = ey * zoomFactor;

        // See if this is the closest to the cursor point so far
        delta = (float)max(fabs(displayX - cursorX), fabs(displayY - cursorY));

        if (delta < bestSoFar)
        {
            bestSoFar = delta;
            choice = tmpElement;
        }
    }

    // See if this is inside the cursor region
    if (bestSoFar < (CURSOR_SIZE * zoomFactor))
    {
        return choice;
    }
    else
    {
        return NULL;
    }
}

GroundListElement* HarmTargetingPod::FindPOSTarget()
{
    if (POSTargetIndex >= 0 and POSTargetIndex <= 3)
    {
        return POSTargets[POSTargetIndex];
    }

    else
    {
        return NULL;
    }
}

// RV - I-Hawk - Build the POS targets list according to waypoints position. If an emitter is
// close enough to one of the valid waypoints (not using T-O, landing and pre-landing waypoints)
// then it'll get into the list. The list can hold up to 4 targets
void HarmTargetingPod::BuildPOSTargets(void)
{
    GroundListElement *tmpElement;
    GroundListElement *choice = NULL;
    float bestSoFar; // very small value from waypoint
    mlTrig trig;
    float wpX, wpY, wpZ;
    float x, y;
    float displayX, displayY;
    float delta;
    float waypointX, waypointY;
    int firstFreeTargetIndex;
    WayPointClass *curWaypoint;
    WayPointClass *tempWP, *firstValidWP, *lastValidWP;
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
    bool curTargetFound = false;

    curWaypoint = ((SimVehicleClass*)platform)->curWaypoint; // Get the current waypoint

    if ( not curWaypoint) return;   // Don't do anything if waypoint is NULL

    if (POSTargets[MAX_POS_TARGETS - 1]) return;  // Don't bother if POS targets list already full

    // Now the purpose is to get to first valid waypoint and last valid waypoint, then
    // start to build a PB list in between
    if (curWaypoint->GetWPAction() == WP_TAKEOFF)   // check if the current is the Take-off WP
    {
        if (curWaypoint->GetNextWP())   // Validate next one isn't NULL
        {
            firstValidWP = curWaypoint->GetNextWP(); // Mark next one as the first valid WP
        }

        else
        {
            return;
        }
    }

    else // Find the first waypoint
    {
        while (curWaypoint->GetPrevWP() and (curWaypoint->GetPrevWP())->GetWPAction() not_eq WP_TAKEOFF)
        {
            curWaypoint = curWaypoint->GetPrevWP();
        }

        firstValidWP = curWaypoint;
    }

    tempWP = firstValidWP;

    // Now move on to find the last valid WP
    while (tempWP->GetNextWP() and (tempWP->GetNextWP())->GetWPAction() not_eq WP_LAND)
    {
        tempWP = tempWP->GetNextWP();
    }

    lastValidWP = tempWP;

    firstFreeTargetIndex = 0;

    int tempIndex = firstFreeTargetIndex;

    for (tempWP = firstValidWP; tempWP not_eq lastValidWP->GetNextWP(); tempWP = tempWP->GetNextWP())
    {
        mlSinCos(&trig, platform->Yaw());

        tempWP->GetLocation(&wpX, &wpY, &wpZ);

        // Walk our list looking for the thing in range and nearest the waypoint
        bestSoFar = 0.005f;
        choice = NULL;

        for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
        {
            if (tmpElement->BaseObject() == NULL) continue;

            // Although not displaying anything, still using the display variables...
            // Convert to normalized display space with heading up
            y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / 30.0f * HTS_DISPLAY_RADIUS;
            x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / 30.0f * HTS_DISPLAY_RADIUS;
            displayX = trig.cos * x - trig.sin * y;
            displayY = trig.sin * x + trig.cos * y;

            // Now get the waypoint's position
            y = (wpX - platform->XPos()) * FT_TO_NM / 30.0f * HTS_DISPLAY_RADIUS;
            x = (wpY - platform->YPos()) * FT_TO_NM / 30.0f * HTS_DISPLAY_RADIUS;
            waypointX = trig.cos * x - trig.sin * y;
            waypointY = trig.sin * x + trig.cos * y;

            // See if this is the closest to the waypoint so far (must be smaller than 0.0005f anyway)
            delta = (float)max(fabs(displayX - waypointX), fabs(displayY - waypointY));

            if (delta < bestSoFar)
            {
                bestSoFar = delta;
                choice = tmpElement;
            }

            if ( not curTargetFound and curTarget and curTarget->BaseObject())
            {
                if (tmpElement == curTarget or tmpElement->BaseObject() == curTarget->BaseObject())
                {
                    curTargetFound = true;
                }
            }

            if (lockedTarget and lockedTarget->BaseData() == tmpElement->BaseObject())
            {
                if (displayY < 0.0f)
                {
                    FCC->dropTrackCmd = TRUE;
                }
            }
        }

        // If choice is not NULL, we got a matching element
        if (choice)
        {
            POSTargets[tempIndex] = choice;
            POSTargetsWPs[tempIndex] = tempWP;
            tempIndex++;

            if (tempIndex > MAX_POS_TARGETS - 1)   // list is full, get out...
            {
                break;
            }
        }

        if ( not curTargetFound)   // If the cur target no longer alive, NULL the curTarget pointer
        {
            curTarget = NULL;
        }
    }
}

void HarmTargetingPod::GetAGCenter(float* x, float* y)
{
    mlTrig yawTrig;
    float range = displayRange * NM_TO_FT / HTS_DISPLAY_RADIUS;

    mlSinCos(&yawTrig, platform->Yaw());

    *x = (cursorY * yawTrig.cos - cursorX * yawTrig.sin) * range + platform->XPos();
    *y = (cursorY * yawTrig.sin + cursorX * yawTrig.cos) * range + platform->YPos();
}

// RV - I-Hawk - Code isn't used anyway

//#if 0
//void HarmTargetingPod::BuildPreplannedTargetList (void)
//{
// FlightClass* theFlight = (FlightClass*)(platform->GetCampaignObject());
// FalconPrivateList* knownEmmitters = NULL;
// ListElement* tmpElement;
// ListElement* curElement = NULL;
// FalconEntity* eHeader;
//
// if (SimDriver.RunningCampaignOrTactical() and theFlight)
// knownEmmitters = theFlight->GetKnownEmitters();
//
// if (knownEmmitters)
// {
// VuListIterator elementWalker (knownEmmitters);
//
// eHeader = (FalconEntity*)elementWalker.GetFirst();
// while (eHeader)
// {
// tmpElement = new ListElement (eHeader);
//
// if (emmitterList == NULL)
// emmitterList = tmpElement;
// else
// curElement->next = tmpElement;
// curElement = tmpElement;
// eHeader = (FalconEntity*)elementWalker.GetNext();
// }
//
// knownEmmitters->Unregister();
// delete knownEmmitters;
// }
//}
//#endif

GroundListElement* HarmTargetingPod::FindEmmitter(FalconEntity *entity)
{
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
    GroundListElement* tmpElement = FCC->GetFirstGroundElement();

    while (tmpElement)
    {
        if (tmpElement->BaseObject() == entity)
        {
            break;
        }

        tmpElement = tmpElement->GetNext();
    }

    return (tmpElement);
}


int HarmTargetingPod::ObjectDetected(FalconEntity* newEmmitter, int trackType, int dummy)  // 2002-02-09 MODIFIED BY S.G. Added the unused dummy var
{
    int retval = FALSE;
    GroundListElement *tmpElement;

    // We're only tracking ground things right now...
    if (newEmmitter->OnGround())
    {
        // See if this one is already in our list
        tmpElement = FindEmmitter(newEmmitter);

        if ( not tmpElement)
        {
            FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

            // Add the new one at the head;
            tmpElement = new GroundListElement(newEmmitter);
            FCC->AddGroundElement(tmpElement);

            retval = TRUE;
        }

        // Make sure it is marked as checked
        tmpElement->ClearFlag(GroundListElement::UnChecked);

        switch (trackType)
        {
                // Note:  It is intentional that these cases fall through...
            case Track_Unlock:
                tmpElement->ClearFlag(GroundListElement::Track);

            case Track_LaunchEnd:
                tmpElement->ClearFlag(GroundListElement::Launch);

                // Break here....
                break;

                // Note:  It is intentional that these cases fall through...
            case Track_Launch:
                tmpElement->SetFlag(GroundListElement::Launch);

            case Track_Lock:
                tmpElement->SetFlag(GroundListElement::Track);

            default:
                tmpElement->SetFlag(GroundListElement::Radiate);
        }

        // Update the hit time;
        tmpElement->lastHit = SimLibElapsedTime;
    }

    return retval;
}


// Will lock the sensor on the related entity (no change if NULL)
void HarmTargetingPod::LockListElement(GroundListElement *choice)
{
    SimObjectType *obj;

    // If we have a candidate, look for it in the platform's target list
    if (choice)
    {
        for (obj = platform->targetList; obj; obj = obj->next)
        {
            if (obj->BaseData() == choice->BaseObject())
            {
                // We found a match
                SetSensorTarget(obj);
                break;
            }
        }

        // If we didn't find it in the target list, force the issue anyway...
        if ( not obj)
        {
            SetSensorTargetHack(choice->BaseObject());
        }
    }
}

// RV - I-Hawk - for the advance HAS display, spearated HTS range from HSD range
void HarmTargetingPod::IncreaseRange()
{
    if (trueDisplayRange >= 115)   // Don't go over 120NM
    {
        return;
    }

    // The range is integer, so when getting down from 15 it becomes 8, so if 8 get it back to 15
    // instead of multiplying
    if (trueDisplayRange == 8)
    {
        trueDisplayRange = 15;
        displayRange = trueDisplayRange;
        zoomMode = Wide;
        zoomFactor = 1.0f;
        return;
    }

    trueDisplayRange *= 2;
    displayRange = trueDisplayRange;
    zoomMode = Wide;
    zoomFactor = 1.0f;
}

void HarmTargetingPod::DecreaseRange()
{
    if (trueDisplayRange <= 10)
    {
        return;
    }

    if (trueDisplayRange == 15)
    {
        trueDisplayRange = 8;
        displayRange = trueDisplayRange;
        zoomMode = Wide;
        zoomFactor = 1.0f;
        return;
    }

    trueDisplayRange /= 2;
    displayRange = trueDisplayRange;
    zoomMode = Wide;
    zoomFactor = 1.0f;
}

// Check if emitter is inside the ALIC video
bool HarmTargetingPod::IsInsideALIC(float &displayX, float &displayY)
{
    if (fabs(displayX) > (0.75f * zoomFactor) or displayY > ((1.05f + HTS_Y_OFFSET) * zoomFactor) or
        displayY < ((-0.3f + HTS_Y_OFFSET) * zoomFactor))
    {
        return false;
    }

    else
    {
        return true;
    }
}

// For now only toggle between Wide and Center zoom
void HarmTargetingPod::ToggleZoomMode()
{
    switch (zoomMode)
    {
        case Wide:
            zoomMode = Center;
            zoomFactor = 2.0f;
            displayRange /= 2;
            break;

        case Center:
        default:
            zoomMode = Wide;
            zoomFactor = 1.0f;
            displayRange *= 2;
            break;
    }

    // TODO: Implement Right/Left zooming ability for HTS HAS mode
}

// HAD zoom mode togglling
void HarmTargetingPod::ToggleHADZoomMode()
{
    switch (HadZoomMode)
    {
        case NORM:
            HadZoomMode = EXP1;
            SaveHadCursorPos();
            displayRange /= 2;
            break;

        case EXP1:
            HadZoomMode = EXP2;
            SaveHadCursorPos();
            displayRange /= 2;
            break;

        case EXP2:
        default:
            HadZoomMode = NORM;
            ResetHadCursorPos();
            displayRange = trueDisplayRange;
            break;
    }
}

// RV - I-Hawk - Draw the DTSB box
void HarmTargetingPod::DrawDTSBBox()
{
    if (zoomFactor > 1.0f)   // No do in a zoom mode
    {
        return;
    }

    DWORD tempColor = display->Color();

    display->SetColor(GetMfdColor(MFD_BRIGHT_GREEN)); // Bright green
    static const float DTSBSide = 0.65f;
    static const float DTSBTop = 1.3f;
    static const float DTSBBottom = 1.15f;
    display->Line(-DTSBSide,   DTSBTop,  DTSBSide,  DTSBTop);
    display->Line(-DTSBSide,   DTSBTop,  -DTSBSide,  DTSBBottom);
    display->Line(-DTSBSide,   DTSBBottom,  DTSBSide,  DTSBBottom);
    display->Line(DTSBSide,   DTSBTop,  DTSBSide,  DTSBBottom);

    display->SetColor(tempColor);
}

void HarmTargetingPod::UpdateDTSB(int symbol, float &displayX, float &displayY)
{
    float verticalPos = 1.225f; // vertical center of DTSB box
    float horizontalPos = -0.5f; // first slot to write into (starting from left to right)
    float horizontalSpace = 0.2f; // space between symbols in the box
    int i;

    if (zoomFactor > 1.0f)   // no DTSB in zoomed mode
    {
        return;
    }

    if (DTSBList[MAX_DTSB_TARGETS - 1])   // list is already full, no place for you, sorry...
    {
        return;
    }

    // Get into list only a "worthy" symbol, SAM or search radar
    if ( not IsInPriorityList(symbol))
    {
        return;
    }

    // Do not draw unknown threats into DTSB
    if (symbol == RWRSYM_UNKNOWN or
        symbol == RWRSYM_UNK1 or
        symbol == RWRSYM_UNK2 or
        symbol == RWRSYM_UNK3 or
        symbol == RWRSYM_MIB_F_U or
        symbol == RWRSYM_VS or
        symbol == RWRSYM_MIB_F_S or
        symbol == RWRSYM_MIB_BW_S
       )
    {
        return;
    }

    // Start filling the DTSB list
    // Look for first open slot
    for (i = 0; i < MAX_DTSB_TARGETS; i++)
    {
        if (DTSBList[i])   // this position is already taken...
        {
            if (DTSBList[i] == symbol)   // symbol is already on the list... return
            {
                return;
            }

            horizontalPos += horizontalSpace; // jump one slot to the right
            continue;
        }

        else
        {
            break;
        }
    }

    // Adjust the display according to the correct slot position on the box
    display->AdjustOriginInViewport(-displayX + horizontalPos, -displayY + verticalPos + HTS_Y_OFFSET);
    DWORD tempColor = display->Color();
    display->SetColor(GetMfdColor(MFD_WHITY_GRAY));  // "whity" gray

    DrawEmitterSymbol(symbol, 0);   // Draw the symbol in the box
    DTSBList[i] = symbol; // Add the symbol to the list

    // Reset the display back
    display->SetColor(tempColor);
    display->AdjustOriginInViewport(displayX - horizontalPos, +displayY - verticalPos - HTS_Y_OFFSET);
}

void HarmTargetingPod::BoxTargetDTSB(int symbol, float &displayX, float &displayY)
{
    float verticalPos = 1.225f; // vertical center of DTSB box
    float horizontalPos = -0.5f; // first slot to write into (starting from left to right)
    float horizontalSpace = 0.2f; // space between symbols in the box
    int i;
    bool found = false;

    // Get into list only a "worthy" symbol, SAM or search radar
    if ( not IsInPriorityList(symbol))
    {
        return;
    }

    // Do not draw unknown threats into DTSB
    if (symbol == RWRSYM_UNKNOWN or
        symbol == RWRSYM_UNK1 or
        symbol == RWRSYM_UNK2 or
        symbol == RWRSYM_UNK3 or
        symbol == RWRSYM_MIB_F_U or
        symbol == RWRSYM_VS or
        symbol == RWRSYM_MIB_F_S or
        symbol == RWRSYM_MIB_BW_S
       )
    {
        return;
    }

    for (i = 0; i < 5; i++)
    {
        if (DTSBList[i] == symbol)
        {
            found = true;
            break;
        }

        horizontalPos += horizontalSpace;
    }

    if (found)
    {
        // Adjust the display according to the correct slot position on the box
        display->AdjustOriginInViewport(-displayX + horizontalPos, -displayY + verticalPos + HTS_Y_OFFSET);
        DWORD tempColor = display->Color();
        display->SetColor(GetMfdColor(MFD_WHITY_GRAY));  // "whity" gray

        DrawEmitterSymbol(symbol, 2);   // Draw the symbol in a box

        // Reset the display back
        display->SetColor(tempColor);
        display->AdjustOriginInViewport(displayX - horizontalPos, +displayY - verticalPos - HTS_Y_OFFSET);
    }
}

// RV - I-Hawk - Clear the POS PB targets list
void HarmTargetingPod::ClearPOSTargets(void)
{
    for (int i = 0; i < MAX_POS_TARGETS; i++)
    {
        POSTargets[i] = NULL;
        POSTargetsWPs[i] = NULL;
    }
}

void HarmTargetingPod::SetPOSTargetIndex(int index)
{
    if (index < 0 or index > 3)
    {
        POSTargetIndex = -1;
        return;
    }

    POSTargetIndex = index;
}

// RV - I-Hawk - Find waypoint number in the list of waypoints
int HarmTargetingPod::FindWaypointNum(WayPointClass* theWP)
{
    int WPnum;
    WayPointClass* tempWaypoint;

    tempWaypoint = ((SimVehicleClass*)platform)->waypoint;

    if ( not tempWaypoint) return -1;

    WPnum = 1;

    while (tempWaypoint and tempWaypoint not_eq theWP)
    {
        tempWaypoint = tempWaypoint->GetNextWP();
        WPnum++;
    }

    return WPnum;
}

// RV - I-Hawk - Check if the emitter is in the current priority mode
bool HarmTargetingPod::IsInPriorityList(int symbol)
{
    HASFilterMode curMode = GetFilterMode();

    switch (curMode)
    {
        case ALL: // Return always true here
        default:
            return true;
            break;

        case HP: // Get only high priority threats, basically only SAMs and AAA radars
            if (symbol < RWRSYM_HAWK or
                (symbol > 23 and symbol not_eq 111 and symbol not_eq 112 and 
                 symbol not_eq 117) and 
                symbol not_eq RWRSYM_KSAM or
                symbol == RWRSYM_SEARCH)
            {
                return false;
            }

            else
            {
                return true;
            }

            break;

        case HA: // Get only high altitude threats, basically all large SAMs

            if ((symbol >= RWRSYM_HAWK and symbol <= RWRSYM_SA6) or
                symbol == RWRSYM_SA10 or
                symbol == RWRSYM_NIKE or
                symbol == 111 or
                symbol == 112 or
                symbol == 117)
            {
                return true;
            }

            else
            {
                return false;
            }

            break;

        case LA: // Get only low altitude threats, basically all small SAMs and AAA

            if (symbol == RWRSYM_SA8 or
                symbol == RWRSYM_SA9 or
                symbol == RWRSYM_SA13 or
                symbol == RWRSYM_AAA or
                symbol == RWRSYM_CHAPARAL or
                symbol == RWRSYM_CHAPARAL or
                symbol == RWRSYM_SA15 or
                (symbol >= 21 and symbol <= 23) or
                symbol == RWRSYM_KSAM
               )
            {
                return true;
            }

            else
            {
                return false;
            }

            break;

    }
}

void HarmTargetingPod::SaveHadCursorPos()
{
    // We switched to EXP1 zoom mode, save yaw... if EXP2 no need as orientation of AC already saved from EXP1
    if (HadZoomMode == EXP1)
    {
        HadOrigCursorX = cursorX;
        HadOrigCursorY = cursorY;
        yawBackup = platform->Yaw();
        XPosBackup = platform->XPos();
        YPosBackup = platform->YPos();
    }

    if (HadZoomMode == EXP2)
    {
        HadOrigCursorX2 = cursorX;
        HadOrigCursorY2 = cursorY;
    }
}

void HarmTargetingPod::ResetHadCursorPos()
{
    HadOrigCursorX = HadOrigCursorY = yawBackup = XPosBackup = YPosBackup = 0.0f;
    HadOrigCursorX2 = HadOrigCursorY2 = 0.0f;
}

void GetCurWezAngle(int i, float &angleX, float &angleY, float &offsetX, float &offsetY)
{
    switch (i)
    {
        case 0: // angle is 310
            angleX = 0.642f;
            offsetX = 0.065f;
            angleY = -0.766f;
            offsetY = -0.01f;
            break;

        case 1: // angle is 330
            angleX = 0.866f;
            offsetX = 0.085f;
            angleY = -0.5f;
            offsetY = -0.01f;
            break;

        case 2: // angle is 350
            angleX = 0.984f;
            offsetX = 0.08f;
            angleY = -0.173f;
            offsetY = 0.0f;
            break;

        case 3: // angle is 015
            angleX = 0.965f;
            offsetX = 0.125f;
            angleY = 0.258f;
            offsetY = 0.0f;
            break;

        case 4: // angle is 040
            angleX = 0.766f;
            offsetX = 0.085f;
            angleY = 0.642f;
            offsetY = 0.02f;
            break;

        case 5: // angle is 065
            angleX = 0.422f;
            offsetX = 0.0f;
            angleY = 0.906f;
            offsetY = -0.06f;
            break;

        case 6: // angle is 080
            angleX = 0.173f;
            offsetX = 0.0f;
            angleY = 0.984f;
            offsetY = -0.075f;
            break;

        case 7: // angle is 090
            angleX = 0.0f;
            offsetX = 0.0f;
            angleY = 1.0f;
            offsetY = -0.085f;
            break;
    }
}

void GetCurWezValue(int i, float &curX, float &curY, float &nextX, float &nextY)
{
    float scale = 1.22f;

    switch (i)
    {
        case 0:
            curX = nextX;
            curY = nextY;
            nextX = 0.35f * scale;
            nextY = 0.1f * scale;
            break;

        case 1:
            curX = nextX;
            curY = nextY;
            nextX = 0.55f * scale;
            nextY = 0.18f * scale;
            break;

        case 2:
            curX = nextX;
            curY = nextY;
            nextX = 0.65f * scale;
            nextY = 0.3f * scale;
            break;

        case 3:
            curX = nextX;
            curY = nextY;
            nextX = 0.72f * scale;
            nextY = 0.65f * scale;
            break;

        case 4:
            curX = nextX;
            curY = nextY;
            nextX = 0.6f * scale;
            nextY = 0.85f * scale;
            break;

        case 5:
            curX = nextX;
            curY = nextY;
            nextX = 0.2f * scale;
            nextY = 0.95f * scale;
            break;

        case 6:
            curX = nextX;
            curY = nextY;
            nextX = 0.05f * scale;
            nextY = 0.955f * scale;
            break;

        case 7:
            curX = nextX;
            curY = nextY;
            nextX = 0.0f * scale;
            nextY = 0.96f * scale;
            break;
    }
}

// Stuff isn't used anyway

//#if 0
//// List elements of the HarmTargetingPod
//HarmTargetingPod::ListElement::ListElement (FalconEntity* newEntity)
//{
// F4Assert (newEntity);
//
// baseObject = newEntity;
// symbol = RadarDataTable[newEntity->GetRadarType()].RWRsymbol;
// flags = 0;
// next = NULL;
// lastHit = SimLibElapsedTime;
// VuReferenceEntity(newEntity);
//}
//
//
//HarmTargetingPod::ListElement::~ListElement ()
//{
// VuDeReferenceEntity(baseObject);
//}
//
//
//void HarmTargetingPod::ListElement::HandoffBaseObject(void)
//{
// FalconEntity *newBase;
//
// ShiAssert( baseObject );
//
// newBase = SimCampHandoff( baseObject, HANDOFF_RADAR );
//
// if (newBase not_eq baseObject) {
// VuDeReferenceEntity(baseObject);
//
// baseObject = newBase;
//
// if (baseObject) {
// VuReferenceEntity(baseObject);
// }
// }
//}
//#endif
