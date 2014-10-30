//****************************************************
// RV - I-Hawk
// Starting HARM avionics systems upgrade 07/2008
//****************************************************


#include "stdhdr.h"
#include "F4Vu.h"
#include "missile.h"
#include "Graphics/Include/display.h"
#include "simveh.h"
#include "airunit.h"
#include "simdrive.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "team.h"
#include "entity.h"
#include "simmover.h"
#include "soundfx.h"
#include "classtbl.h"
#include "rwr.h"
#include "AdvancedHTS.h"
#include "aircrft.h"

const float CURSOR_SIZE =  0.065f;

AdvancedHarmTargetingPod::AdvancedHarmTargetingPod(int idx, SimMoverClass* self) : HarmTargetingPod(idx, self)
{
    curMissile = NULL;
    curTarget =  NULL;
    curTargetWP = prevTargetWP = NULL;
    curTOF = 0.0f;
    timer = 0;
    prevTOF = -1.0f;
    prevTargteSymbol = prevWPNum = 0;
    missileLaunched = false;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    SMSClass* Sms;

    if (playerAC)
    {
        Sms = playerAC->Sms;

        if (Sms)
        {
            curMissile = (MissileClass*)(Sms->GetCurrentWeapon());
        }
    }
}

AdvancedHarmTargetingPod::~AdvancedHarmTargetingPod(void)
{
}

// RV - I-Hawk - HAD display
void AdvancedHarmTargetingPod::HADDisplay(VirtualDisplay* activeDisplay)
{
    float displayX, displayY;
    float x2, y2;
    float wpX, wpY, wpZ;
    float cosAng, sinAng;
    WayPointClass *curWaypoint;
    GroundListElement* tmpElement;
    mlTrig trig;
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

    // Let the base class do the basics
    HarmTargetingPod::HADDisplay(activeDisplay);

    // Set up the trig functions of our current heading
    mlSinCos(&trig, platform->Yaw());
    cosAng = trig.cos;
    sinAng = trig.sin;

    // Draw all known emmitters
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement->BaseObject() == NULL)
        {
            continue;
        }

        // Check if emitter is on priority list
        if ( not IsInPriorityList(tmpElement->symbol))
        {
            continue;
        }

        // Compute the world space oriented, display space scaled, ownship relative postion of the emitter
        y2 = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        x2 = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;

        // Rotate it into heading up space and translate it down to deal with our vertical offset
        displayX = cosAng * x2 - sinAng * y2;
        displayY = sinAng * x2 + cosAng * y2 + HTS_Y_OFFSET;

        // Skip this one if its off screen
        if ((fabs(displayX) > 1.0f) or (fabs(displayY) > 1.0f))
        {
            continue;
        }

        // JB 010726 Clear the designated target if behind the 3/9 line.
        if (displayY - HTS_Y_OFFSET < 0)
        {
            if (lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
            {
                FCC->dropTrackCmd = TRUE;
            }
        }

        DrawEmitter(tmpElement, displayX, displayY, displayY);

        // Mark the locked target
        // Mark the locked target
        if (displayY - HTS_Y_OFFSET > 0 and lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
        {
            display->SetColor(GetMfdColor(MFD_WHITE));
            display->Line(-CURSOR_SIZE, -CURSOR_SIZE, -CURSOR_SIZE, CURSOR_SIZE);
            display->Line(CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
            display->Line(-CURSOR_SIZE, CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
            display->Line(-CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, -CURSOR_SIZE);
            handedoff = true; // Immediate hadnoff in HAD mode
        }

        display->AdjustOriginInViewport(-displayX, -displayY);
    }

    // Adjust our viewport origin
    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    // Draw the waypoints
    display->SetColor(0xFF00FF00);
    display->AdjustRotationAboutOrigin(-platform->Yaw());
    curWaypoint = ((SimVehicleClass*)platform)->waypoint;

    if (curWaypoint)
    {
        curWaypoint->GetLocation(&wpX, &wpY, &wpZ);
        y2 = (wpX - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        x2 = (wpY - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        curWaypoint = curWaypoint->GetNextWP();

        while (curWaypoint)
        {
            curWaypoint->GetLocation(&wpX, &wpY, &wpZ);
            displayY = (wpX - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
            displayX = (wpY - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
            display->Line(x2, y2, displayX, displayY);
            x2 = displayX;
            y2 = displayY;
            curWaypoint = curWaypoint->GetNextWP();
        }
    }

    // Reset our display parameters
    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();
}

// RV - I-Hawk - HAD EXP display
void AdvancedHarmTargetingPod::HADExpDisplay(VirtualDisplay* activeDisplay)
{
    float displayX, displayY;
    float x, y;
    float cosAng, sinAng, origCosAng, origSinAng;
    float EXP1OffsetX, EXP1OffsetY, EXP2OffsetX, EXP2OffsetY;
    float yawOffsetAng;
    float origDisplayX, origDisplayY;
    DWORD tempColor;
    GroundListElement* tmpElement;
    mlTrig trig, trig2;
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

    // Let the base class do the basics
    HarmTargetingPod::HADExpDisplay(activeDisplay);

    // Set up the trig functions of our heading when entered zoom mode
    mlSinCos(&trig, yawBackup);
    cosAng = trig.cos;
    sinAng = trig.sin;

    // Set up the trig functions of our heading
    mlSinCos(&trig2, platform->Yaw());
    origCosAng = trig2.cos;
    origSinAng = trig2.sin;

    // Draw all known emmitters
    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement->BaseObject() == NULL)
        {
            continue;
        }

        // Check if emitter is on priority list
        if ( not IsInPriorityList(tmpElement->symbol))
        {
            continue;
        }

        // Hold original positions to keep track if locked target hadn't past 3-9 o'clock line
        y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;

        origDisplayX = origCosAng * x - origSinAng * y;
        origDisplayY = origSinAng * x + origCosAng * y + HTS_Y_OFFSET;

        // Compute the world space oriented, display space scaled, obacked up ownship relative position of the emitter
        y = (tmpElement->BaseObject()->XPos() - XPosBackup) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
        x = (tmpElement->BaseObject()->YPos() - YPosBackup) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;

        // Rotate it into heading up space and translate it down to deal with our vertical offset
        displayX = cosAng * x - sinAng * y;
        displayY = sinAng * x + cosAng * y;

        // Deal with the EXP1 offset
        EXP1OffsetX = 2.0f * HadOrigCursorX;
        EXP1OffsetY = 2.0f * HadOrigCursorY;

        displayX -= EXP1OffsetX;
        displayY -= EXP1OffsetY;

        // Here also add the EXP2 offset if appropriate
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

        // Skip this one if its off screen
        if ((fabs(displayX) > 1.0f) or (fabs(displayY) > 1.0f))
        {
            continue;
        }

        // JB 010726 Clear the designated target if behind the 3/9 line.
        if (origDisplayY - HTS_Y_OFFSET < 0)
        {
            if (lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
            {
                FCC->dropTrackCmd = TRUE;
            }
        }

        DrawEmitter(tmpElement, displayX, displayY, origDisplayY);

        // Mark the locked target
        if (origDisplayY - HTS_Y_OFFSET > 0 and lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
        {
            tempColor = display->Color();
            display->SetColor(GetMfdColor(MFD_WHITE));
            display->Line(-CURSOR_SIZE, -CURSOR_SIZE, -CURSOR_SIZE, CURSOR_SIZE);
            display->Line(CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
            display->Line(-CURSOR_SIZE, CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
            display->Line(-CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, -CURSOR_SIZE);
            handedoff = true;
        }

        display->AdjustOriginInViewport(-displayX, -displayY);
        display->SetColor(0xFF00FF00);
    }

    // Now draw the ownship symbol in relation to the zoomed area
    y = (platform->XPos() - XPosBackup) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
    x = (platform->YPos() - YPosBackup) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;

    // Rotate it into heading up space and translate it down to deal with our vertical offset
    displayX = cosAng * x - sinAng * y;
    displayY = sinAng * x + cosAng * y;

    // Add the EXP1 offset
    displayX -= EXP1OffsetX;
    displayY -= EXP1OffsetY;

    // Here also add the EXP2 offset if appropriate
    if (HadZoomMode == EXP2)
    {
        displayX -= EXP2OffsetX;
        displayY -= EXP2OffsetY;
    }

    tempColor = display->Color();
    display->AdjustOriginInViewport(displayX, displayY);

    // RV - I-Hawk - Ownship "airplane" marker. Here the display is relatively "static" and the
    // only thing that moves and turn is the ownship AC marker
    display->SetColor(GetMfdColor(MFD_CYAN));
    static const float NOSE = 0.02f;
    static const float TAIL = 0.08f;
    static const float WING = 0.06f;
    static const float TAIL_WING = 0.02f;

    yawOffsetAng = platform->Yaw() - yawBackup;
    float yawOffsetAng2 = yawBackup - platform->Yaw();
    mlTrig offset, offset2;
    mlSinCos(&offset, yawOffsetAng);
    mlSinCos(&offset2, yawOffsetAng2);

    sinAng = offset2.sin;
    cosAng = offset2.cos;

    float r;
    float offsetX, offsetY;

    r = WING * 0.7f;
    x = r * cosAng;
    y = r * sinAng;

    offsetY = 0.05f * cosAng;
    offsetX = 0.05f * sinAng;

    display->Line(x, y, -x, -y);

    r = TAIL_WING;
    x = r * cosAng;
    y = r * sinAng;

    offsetY = 0.04f * cosAng;
    offsetX = 0.04f * sinAng;

    sinAng = offset.sin;
    cosAng = offset.cos;

    r = (NOSE + TAIL) / 1.35f;
    y = r * cosAng;
    x = r * sinAng;
    display->Line(x, y, -x, -y);

    display->SetColor(0xFF00FF00);
    display->AdjustOriginInViewport(-displayX, -displayY);

    // Adjust our viewport origin
    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    // Reset our display parameters
    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();
}

// RV - I-Hawk - HAS mode
void AdvancedHarmTargetingPod::HASDisplay(VirtualDisplay* activeDisplay)
{
    unsigned numOfDrawnTargets, curTime;
    float displayX, displayY, NormDisplayX, NormDisplayY, NormDisplayYNoOffset;
    float x, y, x2, y2;
    float cosAng, sinAng;
    float range, alpha, ex, ey, phi, elevation;
    float rangeX, rangeY, trueRange;
    GroundListElement* tmpElement;
    mlTrig trig;
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();

    // Let the base class do the basics
    HarmTargetingPod::HASDisplay(activeDisplay);

    // Set up the trig functions of our current heading
    mlSinCos(&trig, platform->Yaw());
    cosAng = trig.cos;
    sinAng = trig.sin;

    curTime = (unsigned)((SimLibElapsedTime - HASTimer) * MSEC_TO_SEC);
    // Draw all known emmitters
    ClearDTSB(); // clear DTSB first
    numOfDrawnTargets = 0;

    for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
    {
        if (tmpElement->BaseObject() == NULL)
        {
            continue;
        }

        // Check if emitter is on priority list
        if ( not IsInPriorityList(tmpElement->symbol))
        {
            continue;
        }

        // Check if range is less than 60NM (no reason to get anything beyond...)
        rangeY = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM;
        rangeX = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM;

        rangeY *= rangeY;
        rangeX *= rangeX;
        trueRange = sqrtf(rangeX + rangeY);

        if (trueRange > 60.0f)
        {
            continue;
        }

        // Compute the world space oriented, display space scaled, ownship relative postion of the emitter
        y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
        x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
        float alt = (-platform->ZPos() - (-tmpElement->BaseObject()->ZPos())) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;

        // Now calculate elevation (and not range)
        x2 = x * x;
        y2 = y * y;
        range = sqrt(x2 + y2);   // Get range
        alpha = atan(alt / range);    // Get the elevation angle
        elevation = (1.0f - (alpha / HALF_PI)) * HAS_DISPLAY_RADIUS;

        NormDisplayX = cosAng * x - sinAng * y;
        NormDisplayYNoOffset = sinAng * x + cosAng * y;
        NormDisplayY = sinAng * x + cosAng * y + HTS_Y_OFFSET;

        phi = atan(NormDisplayX / NormDisplayYNoOffset);
        ex = elevation * sin(phi);
        ey = elevation * cos(phi);
        displayX = ex * zoomFactor;
        displayY = ey * zoomFactor + HTS_Y_OFFSET;

        // RV - I-Hawk - Diplay only what's inside the ALIC video
        if ( not IsInsideALIC(displayX, displayY))
        {
            continue;
        }

        // What's behind the 3-9 line, display at ALIC bottom line
        if (NormDisplayY < 0.0f + HTS_Y_OFFSET)
        {
            displayY = 0.0f + HTS_Y_OFFSET;
            displayX = -displayX; // Switch sides when it gets inverted...
        }

        // JB 010726 Clear the designated target if behind the 3/9 line.
        if (NormDisplayY - HTS_Y_OFFSET < 0.0f)
        {
            if (lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
            {
                FCC->dropTrackCmd = TRUE;
            }
        }

        // Draw only emitters which their time had come to be shown according to the HAS timer
        if (numOfDrawnTargets <= curTime)
        {
            DrawEmitter(tmpElement, displayX, displayY, displayY);
            UpdateDTSB(tmpElement->symbol, displayX, displayY);
        }

        numOfDrawnTargets++;

        // Adjust bac display viewport as DrawEmitter function is changing the viewport
        display->AdjustOriginInViewport(-displayX, -displayY);
    }

    HASNumTargets = numOfDrawnTargets;

    // RV - I-Hawk - If we have a locked target, go to handoff mode
    if (lockedTarget and lockedTarget->BaseData())
    {
        SetSubMode(Handoff);
        preHandoffMode = Has;
        handoffRefTime = SimLibElapsedTime;
        handedoff = false;
    }

    display->SetColor(0xFF00FF00);

    // Reset our display parameters
    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();
}

// RV - I-Hawk - Handoff mode (getting here after target is locked in HAS mode)
void AdvancedHarmTargetingPod::HandoffDisplay(VirtualDisplay* activeDisplay)
{
    float displayX, displayY, NormDisplayX, NormDisplayY, NormDisplayYNoOffset;
    float x, y, x2, y2;
    float cosAng, sinAng;
    float range, alpha, ex, ey, phi, elevation;
    mlTrig trig;
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
    GroundListElement* tmpElement = NULL;

    // Let the base class do the basics
    HarmTargetingPod::HandoffDisplay(activeDisplay);

    display = activeDisplay;

    // Set up the trig functions of our current heading
    mlSinCos(&trig, platform->Yaw());
    cosAng = trig.cos;
    sinAng = trig.sin;

    // Draw only the locked target
    ClearDTSB(); // clear DTSB first

    if (preHandoffMode == Has)
    {
        for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
        {
            if ( not lockedTarget or not lockedTarget->BaseData() or preHandoffMode == Pos)
            {
                break;
            }

            if (tmpElement->BaseObject() == NULL)
            {
                continue;
            }

            // Compute the world space oriented, display space scaled, ownship relative postion of the emitter
            y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
            x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;
            float alt = (-platform->ZPos() - (-tmpElement->BaseObject()->ZPos())) * FT_TO_NM / displayRange * HAS_DISPLAY_RADIUS;

            // Displaying azimuth/elevation like HAS mode
            x2 = x * x;
            y2 = y * y;
            range = sqrt(x2 + y2);
            alpha = atan(alt / range);
            elevation = (1.0f - (alpha / HALF_PI)) * HAS_DISPLAY_RADIUS;

            NormDisplayX = cosAng * x - sinAng * y;
            NormDisplayYNoOffset = sinAng * x + cosAng * y;
            NormDisplayY = sinAng * x + cosAng * y + HTS_Y_OFFSET;

            phi = atan(NormDisplayX / NormDisplayYNoOffset);
            ex = elevation * sin(phi);
            ey = elevation * cos(phi);
            displayX = ex;
            displayY = ey + HTS_Y_OFFSET;

            // Skip this one if its off screen
            //if ((fabs(displayX) > 1.0f) or (fabs(displayY) > 1.0f))

            // RV - I-Hawk - Diplay only what's inside the ALIC video
            if ( not IsInsideALIC(displayX, displayY))
            {
                continue;
            }

            // although not drawing none-target emitters on main display, still keep DTSB updated
            display->AdjustOriginInViewport(displayX, displayY);
            UpdateDTSB(tmpElement->symbol, displayX, displayY);
            display->AdjustOriginInViewport(-displayX, -displayY);

            // Only show the locked up target (after we finished updating DTSB list)
            if (tmpElement->BaseObject() not_eq lockedTarget->BaseData())
            {
                continue;
            }

            // JB 010726 Clear the designated target if behind the 3/9 line.
            if (NormDisplayY - HTS_Y_OFFSET < 0.0f)
            {
                if (lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
                {
                    FCC->dropTrackCmd = TRUE;
                }
            }

            DrawEmitter(tmpElement, displayX, displayY, displayY);
            BoxTargetDTSB(tmpElement->symbol, displayX, displayY);

            // Mark the locked target
            if (displayY - HTS_Y_OFFSET > 0 and lockedTarget and tmpElement->BaseObject() == lockedTarget->BaseData())
            {
                display->SetColor(GetMfdColor(MFD_WHITE));
                display->Line(-CURSOR_SIZE, -CURSOR_SIZE, -CURSOR_SIZE, CURSOR_SIZE);
                display->Line(CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
                display->Line(-CURSOR_SIZE, CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
                display->Line(-CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, -CURSOR_SIZE);
            }

            display->AdjustOriginInViewport(-displayX, -displayY);
        }
    }

    if ( not lockedTarget)   // Back to HAS mode
    {
        SetSubMode(HAS);
        ((SimVehicleClass*)platform)->SOIManager(SimVehicleClass::SOI_WEAPON);
        handedoff = false;
    }

    // Check if 3 seconds past since the lock for handoff "delay"
    if (handedoff == false)
    {
        if (SimLibElapsedTime - handoffRefTime > 3 * SEC_TO_MSEC)
        {
            handedoff = true;
        }
    }

    display->SetColor(0xFF00FF00);

    // Adjust our viewport origin
    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    // Reset our display parameters
    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();
}

// RV - I-Hawk - POS display
void AdvancedHarmTargetingPod::POSDisplay(VirtualDisplay* activeDisplay)
{
    int OsbIndex;
    int boxed;
    int days, hours, minutes, seconds;
    DWORD tempColor;
    float OsbPosX, OsbPosY;
    float LDLVerticalPos = (ALICTOP + ALICBOTTOM) / 2.0f + 0.05f;
    float theRange;
    float dx, dy, ETA;
    float wpX, wpY, wpZ;
    float temp;
    GroundListElement* tmpElement = NULL;
    WayPointClass* tempWP = NULL;
    int theWPnum;
    char str[24];
    FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	SMSClass* Sms = NULL;

    if (playerAC)
    {
        Sms = playerAC->Sms;
    }

    if (Sms and not curMissile)
    {
        curMissile = (MissileClass*)(Sms->GetCurrentWeapon());
    }

    display = activeDisplay;
    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();

    // Let the base class do the basics
    HarmTargetingPod::POSDisplay(activeDisplay);

    ClearDTSB();
    ClearPOSTargets();
    BuildPOSTargets();

    OsbIndex = 0;

    for (int i = 0; i < MAX_POS_TARGETS; i++)
    {
        if (POSTargets[i])
        {
            boxed = 0;
            tmpElement = POSTargets[i];
            tempWP = POSTargetsWPs[i];
            GetOsbPos(OsbIndex, OsbPosX, OsbPosY);
            tempColor = display->Color();
            display->SetColor(GetMfdColor(MFD_WHITY_GRAY));  // "whity" gray

            // This one is the current target
            if (i == POSTargetIndex and tmpElement and lockedTarget and 
                tmpElement->BaseObject() == lockedTarget->BaseData())
            {
                curTarget = POSTargets[i];
                curTargetWP = POSTargetsWPs[i];

                // If the target had not been handed off already, flash the box.
                if (handedoff == false)
                {
                    boxed = flash ? 2 : 0;
                }

                else
                {
                    boxed = 2; // Box it
                }

                float zero = 0.0f;
                UpdateDTSB(curTarget->symbol, zero, zero);    // Get it into DTSB
                display->AdjustOriginInViewport(-0.55f, LDLVerticalPos - 0.1f + HTS_Y_OFFSET);
                DrawEmitterSymbol(curTarget->symbol, boxed);
                display->AdjustOriginInViewport(0.55f, -(LDLVerticalPos - 0.1f + HTS_Y_OFFSET));

                // Get the waypoint number where this target is at, we need to display it
                // under the LSDL line
                theWPnum = FindWaypointNum(curTargetWP);
                sprintf(str, "%02d", theWPnum);
                display->TextCenter(-0.54f, LDLVerticalPos - 0.15f + HTS_Y_OFFSET, str);

                // Get missile TOF. Should also be with the under-LSDL pre-launch info
                if (curMissile and curTarget and curTarget->BaseObject())
                {
                    // Get the range to the tmpElement which is the current target "candidate"
                    dx = fabs(playerAC->XPos() - curTarget->BaseObject()->XPos());
                    dx *= dx;
                    dy = fabs(playerAC->YPos() - curTarget->BaseObject()->YPos());
                    dy *= dy;
                    theRange = sqrt(dx + dy);

                    curTOF = curMissile->GetTOF(-playerAC->ZPos(), playerAC->GetVt(), 0.0f, 0.0f, theRange);
                }

                if (curTOF > 0.0f)
                {
                    temp = curTOF;
                    minutes = FloatToInt32(temp / 60.0f);
                    temp -= minutes * 60.0f;
                    seconds = FloatToInt32(temp);
                    sprintf(str, "%d:%02d", abs(minutes), abs(seconds));
                    display->TextCenter(-0.55f, LDLVerticalPos - 0.28f + HTS_Y_OFFSET, str);

                }

                // RV - I-Hawk - Get ETA - BTW I noticed the HUD ETA for waypoints is fucked up
                // and not following the DED ETA which is the accurate one... probably the WP
                // time arrival function is foobard (which is what the HUD is using)

                // Here calculating same way as the DED
                curTargetWP->GetLocation(&wpX, &wpY, &wpZ);
                ETA = (float)(SimLibElapsedTime / SEC_TO_MSEC) + FloatToInt32(Distance(
                            playerAC->XPos(), playerAC->YPos(), wpX, wpY)
                        / playerAC->GetVt());

                // Calculate ETA
                // Get rid of any days first
                days = FloatToInt32(ETA / 86400.0f);     // 86400 seconds in 24 hours
                ETA -= days * 86400.0f;
                hours = FloatToInt32(ETA / 3600.0f);
                ETA -= hours * 3600.0f;
                minutes = FloatToInt32(ETA / 60.0f);
                ETA -= minutes * 60.0f;
                seconds = FloatToInt32(ETA);
                sprintf(str, "%d:%02d:%02d", abs(hours), abs(minutes), abs(seconds));
                display->TextCenter(-0.56f, LDLVerticalPos - 0.37f + HTS_Y_OFFSET, str);
            }

            display->AdjustOriginInViewport(OsbPosX + 0.1f, OsbPosY);
            DrawEmitterSymbol(tmpElement->symbol, boxed);
            display->AdjustOriginInViewport(-(OsbPosX + 0.1f), -OsbPosY);
            display->SetColor(tempColor);
            display->CenterOriginInViewport();
            OsbIndex++;
        }

        else
        {
            break;
        }
    }

    // In case of a launch, save the target's symbol and WP
    if (curMissile and curMissile->launchState not_eq MissileClass::PreLaunch and lockedTarget)
    {
        // Stop tracking the missile
        curMissile = NULL;

        // Backup info on the launched target, and launched waypoint, for TOF
        if (curTarget)
        {
            prevTargteSymbol = curTarget->symbol;
            prevWPNum = FindWaypointNum(curTargetWP);
            prevTOF = curTOF;
            missileLaunched = true; // update the flag
            timer = SimLibElapsedTime;
        }
    }

    if (missileLaunched)   // Handle the last launched missile data
    {
        tempColor = display->Color();
        display->SetColor(GetMfdColor(MFD_WHITY_GRAY));
        // Draw the emitter simbol above the LSDL line at the post-launch info slot is
        display->AdjustOriginInViewport(0.5f, LDLVerticalPos + 0.1f + HTS_Y_OFFSET);
        DrawEmitterSymbol(prevTargteSymbol, 0);
        display->AdjustOriginInViewport(-0.5f, -(LDLVerticalPos + 0.1f + HTS_Y_OFFSET));

        // Get the previous target waypoint number
        sprintf(str, "%02d", prevWPNum);
        display->TextCenter(0.49f, LDLVerticalPos + 0.25f + HTS_Y_OFFSET, str);

        // Get the launched missile TOF, it's just a "dummy" counter...

        if (prevTOF <= 0.0f)
        {
            prevTOF = 0.0f;
        }

        else // Tick down the prevTOF
        {
            if (SimLibElapsedTime - timer > 1 * SEC_TO_MSEC)
            {
                prevTOF -= 1.0f;
                timer += 1 * SEC_TO_MSEC;
            }
        }

        if (prevTOF >= 0.0f)
        {
            temp = prevTOF;
            minutes = FloatToInt32(temp / 60.0f);
            temp -= minutes * 60.0f;
            seconds = FloatToInt32(temp);
            sprintf(str, "%d:%02d", abs(minutes), abs(seconds));
            display->TextCenter(0.48f, LDLVerticalPos + 0.38f + HTS_Y_OFFSET, str);
        }

        display->SetColor(tempColor);
    }

    // RV - I-Hawk - If we have a locked target, go to handoff mode

    if (lockedTarget and lockedTarget->BaseData())
    {
        preHandoffMode = Pos;

        if (handedoff == false)   // Tick the Handoff timer down
        {
            if (SimLibElapsedTime - handoffRefTime > 3 * SEC_TO_MSEC)
            {
                handedoff = true;
            }
        }
    }

    display->SetColor(0xFF00FF00);

    // Adjust our viewport origin
    display->AdjustOriginInViewport(0.0f, HTS_Y_OFFSET);

    // Reset our display parameters
    display->ZeroRotationAboutOrigin();
    display->CenterOriginInViewport();
}

void AdvancedHarmTargetingPod::DrawEmitter(GroundListElement* tmpElement, float &displayX, float &displayY, float &origDisplayY)
{
    DWORD color;
    int   boxed;

    switch (submode)
    {
        case HAS:
            color = GetMfdColor(MFD_WHITY_GRAY);
            boxed = 0;
            break;

        case Handoff:
            color = GetMfdColor(MFD_WHITY_GRAY);
            boxed = 2;
            break;

        case HAD:
        default:

            // Set the symbols draw intensity based on its state
            if (origDisplayY - HTS_Y_OFFSET < 0)
            {
                color = 0x00008000;
                boxed = 0;
            }
            else if (tmpElement->IsSet(GroundListElement::Launch))
            {
                color = GetMfdColor(MFD_RED);
                boxed = flash ? 2 : 0;
            }
            else if (tmpElement->IsSet(GroundListElement::Track))
            {
                color = GetMfdColor(MFD_RED);
                boxed = 2;
            }
            else if (tmpElement->IsSet(GroundListElement::Radiate))
            {
                color = GetMfdColor(MFD_YELLOW);
                boxed = 0;
            }
            else
            {
                color = 0x00008000;
                boxed = 0;
            }

            break;
    }

    // Adjust our display location and draw the emitter symbol
    DWORD tmpColor = display->Color();
    display->AdjustOriginInViewport(displayX, displayY);
    display->SetColor(color);
    DrawEmitterSymbol(tmpElement->symbol, boxed);
    display->SetColor(tmpColor);
}


void AdvancedHarmTargetingPod::GetOsbPos(int &OsbIndex, float &OsbPosX, float &OsbPosY)
{
    switch (OsbIndex)
    {
        case 0:
            GetButtonPos(16, &OsbPosX, &OsbPosY);
            break;

        case 1:
            GetButtonPos(17, &OsbPosX, &OsbPosY);
            break;

        case 2:
            GetButtonPos(18, &OsbPosX, &OsbPosY);
            break;

        case 3:
        default:
            GetButtonPos(19, &OsbPosX, &OsbPosY);
            break;
    }
}
