#include "stdhdr.h"
#include "classtbl.h"
#include "digi.h"
#include "object.h"
#include "simveh.h"
#include "find.h"
#include "msginc/radiochattermsg.h"
#include "msginc/wingmanmsg.h"
#include "wingorder.h"
#include "aircrft.h"
#include "fack.h"
#include "campbase.h"
#include "flight.h"
#include "airframe.h"
#include "navsystem.h"
#include "radar.h"
#include <stdlib.h>
#include "playerop.h"
#include "msginc/trackmsg.h"
#include "msginc/tankermsg.h"
#include "otwdrive.h"
#include "Sms.h"
/* S.G. 2001-06-16 AI SWITCH TO NAV ON GO COVER */ #include "fcc.h"

extern bool g_bPitchLimiterForAI; // 2002-02-12 S.G.

extern float g_fSSoffsetManeuverPoints1a; // 2002-04-07 S.G.
extern float g_fSSoffsetManeuverPoints1b; // 2002-04-07 S.G.
extern float g_fSSoffsetManeuverPoints2a; // 2002-04-07 S.G.
extern float g_fSSoffsetManeuverPoints2b; // 2002-04-07 S.G.
extern float g_fPinceManeuverPoints1a; // 2002-04-07 S.G.
extern float g_fPinceManeuverPoints1b; // 2002-04-07 S.G.
extern float g_fPinceManeuverPoints2a; // 2002-04-07 S.G.
extern float g_fPinceManeuverPoints2b; // 2002-04-07 S.G.

// ----------------------------------------------------
// DigitalBrain::AiSaveWeaponState
// ----------------------------------------------------

void DigitalBrain::AiSaveWeaponState()
{
    mSavedWeapons = mWeaponsAction;
}


// ----------------------------------------------------
// DigitalBrain::AiRestoreWeaponState
// ----------------------------------------------------

void DigitalBrain::AiRestoreWeaponState()
{
    mWeaponsAction = mSavedWeapons;
}

// ----------------------------------------------------
// DigitalBrain::AiSaveSetSearchDomain
// ----------------------------------------------------

void DigitalBrain::AiSaveSetSearchDomain(char domain)
{

    mSaveSearchDomain = mSearchDomain;
    mSearchDomain = domain;
}



// ----------------------------------------------------
// DigitalBrain::AiRestoreSearchDomain
// ----------------------------------------------------

void DigitalBrain::AiRestoreSearchDomain()
{
    mSearchDomain = mSaveSearchDomain;
}



// ----------------------------------------------------
// DigitalBrain::AiSetManeuver
// ----------------------------------------------------

void DigitalBrain::AiSetManeuver(int maneuver)
{
    mpActionFlags[AI_EXECUTE_MANEUVER] = TRUE;
    mpActionFlags[AI_RTB]              = FALSE;
    mCurrentManeuver = maneuver;
    mnverTime = 10.0F;
}



// ----------------------------------------------------
// DigitalBrain::AiClearManeuver
// ----------------------------------------------------

void DigitalBrain::AiClearManeuver(void)
{
    mpActionFlags[AI_USE_COMPLEX]      = FALSE;
    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mpActionFlags[AI_RTB]              = FALSE;
    mCurrentManeuver = FalconWingmanMsg::WMTotalMsg;
}


// ----------------------------------------------------
// ----------------------------------------------------
//
// Commands that modify Action and Search States
//
// ----------------------------------------------------
// ----------------------------------------------------



// ----------------------------------------------------
// DigitalBrain::AiSmokeOn
// ----------------------------------------------------

void DigitalBrain::AiSmokeOn(FalconWingmanMsg* msg)
{
    short edata[10];
    int flightIdx;

    if (SimDriver.GetPlayerEntity())
    {
        FalconTrackMessage* trackMsg = new FalconTrackMessage(1, SimDriver.GetPlayerEntity()->Id(), FalconLocalGame);
        ShiAssert(trackMsg);
        trackMsg->dataBlock.trackType = Track_SmokeOn;
        trackMsg->dataBlock.id = self->Id();

        FalconSendMessage(trackMsg, TRUE);
    }


    // set a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}


// ----------------------------------------------------
// DigitalBrain::AiSmokeOff
// ----------------------------------------------------

void DigitalBrain::AiSmokeOff(FalconWingmanMsg* msg)
{
    short edata[10];
    int flightIdx;

    if (SimDriver.GetPlayerEntity())
    {
        FalconTrackMessage* trackMsg = new FalconTrackMessage(1, SimDriver.GetPlayerEntity()->Id(), FalconLocalGame);
        ShiAssert(trackMsg);
        trackMsg->dataBlock.trackType = Track_SmokeOff;
        trackMsg->dataBlock.id = self->Id();

        FalconSendMessage(trackMsg, TRUE);
    }

    // set a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}

// ----------------------------------------------------
// DigitalBrain::AiECMOn
// ----------------------------------------------------

void DigitalBrain::AiECMOn(FalconWingmanMsg* msg)
{
    short edata[10];
    int flightIdx;


    // set a radar flag here
    self->SetFlag(ECM_ON);

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 0;
        AiMakeRadioResponse(self, rcECMON, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}


// ----------------------------------------------------
// DigitalBrain::AiECMOff
// ----------------------------------------------------

void DigitalBrain::AiECMOff(FalconWingmanMsg* msg)
{
    short edata[10];
    int flightIdx;

    // turn ECM off
    self->UnSetFlag(ECM_ON);
    // set a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 0;
        AiMakeRadioResponse(self, rcECMOFF, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}

// ----------------------------------------------------
// DigitalBrain::AiClearLeadersSix
// ----------------------------------------------------

void  DigitalBrain::AiClearLeadersSix(FalconWingmanMsg* msg)
{

    int flightIdx;
    short edata[10];
    AircraftClass* pfrom;
    AircraftClass* ptgt;
    mlTrig trig;
    float xpos;
    float ypos;
    float rz;
    int navangle;
    float angle;
    float xdiff, ydiff;
    int random;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    pfrom = (AircraftClass*) vuDatabase->Find(msg->dataBlock.from);

    if (msg->dataBlock.newTarget == FalconNullId)
    {

        // angles of the aircraft we are clearing
        mlSinCos(&trig, pfrom->Yaw());

        xpos = pfrom->XPos() - trig.cos * 1000.0F; // 1000 feet behind aircraft we are clearing
        ypos = pfrom->YPos() - trig.sin * 1000.0F;

        xpos = xpos - self->XPos();
        ypos = ypos - self->YPos();

        mHeadingOrdered = (float)atan2(xpos, ypos);

        mSpeedOrdered = self->GetVt() * FTPSEC_TO_KNOTS;
        mAltitudeOrdered = self->ZPos();
        mnverTime = 15.0F;

        AiSetManeuver(FalconWingmanMsg::WMClearSix);

        if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
        {
            edata[0] = flightIdx;
            edata[1] = 2;
            AiMakeRadioResponse(self, rcROGER, edata);
            AiCheckFormStrip();
        }
        else
        {
            edata[0] = -1;
            edata[1] = -1;
            AiMakeRadioResponse(self, rcROGER, edata);
            AiCheckFormStrip();
        }

    }
    else
    {

        AiCheckFormStrip();

        mDesignatedObject = msg->dataBlock.newTarget;
        ptgt = (AircraftClass*) vuDatabase->Find(mDesignatedObject);

        if (ptgt and pfrom and not F4IsBadReadPtr(ptgt, sizeof(AircraftClass)) and not F4IsBadReadPtr(pfrom, sizeof(AircraftClass))) // JB 010318 CTD
        {
            if (ptgt->ZPos() - pfrom->ZPos() < -500.0F)
            {
                edata[0] = 7; // break low
            }
            else if (ptgt->ZPos() - pfrom->ZPos() > 500.0F)
            {
                edata[0] = 6; // break hi
            }
            else
            {
                random = 2 * FloatToInt32((float)rand() / (float)RAND_MAX);

                if (random)
                {
                    edata[0] = 0; // break hi
                }
                else
                {
                    edata[0] = 3; // break hi
                }
            }

            AiMakeRadioResponse(self, rcBREAK, edata);


            mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
            AiSaveWeaponState();
            mWeaponsAction = AI_WEAPONS_FREE;


            edata[0] = 2 * (ptgt->Type() - VU_LAST_ENTITY_TYPE);

            // convert to compass angle

            xdiff = ptgt->XPos() - pfrom->XPos();
            ydiff = ptgt->YPos() - pfrom->YPos();

            angle = (float)atan2(ydiff, xdiff);
            angle = angle - pfrom->Yaw();
            navangle =  FloatToInt32(RTD * angle);

            if (navangle < 0)
            {
                navangle = 360 + navangle;
            }

            edata[1] = navangle / 30; // scale compass angle for radio eData

            if (edata[1] >= 12)
            {
                edata[1] = 0;
            }

            rz = ptgt->ZPos() - pfrom->ZPos();

            if (rz < 300.0F and rz > -300.0F)   // check relative alt and select correct frag
            {
                edata[2] = 1;
            }
            else if (rz < -300.0F and rz > -1000.0F)
            {
                edata[2] = 2;
            }
            else if (rz < -1000.0F)
            {
                edata[2] = 3;
            }
            else
            {
                edata[2] = 0;
            }


            AiMakeRadioResponse(self, rcENGAGINGC, edata);
        }
    }
}


void DigitalBrain::AiEngageThreatAtSix(VU_ID threat)
{

    short edata[10];
    AircraftClass* ptgt;
    int navangle;
    float angle;
    float xdiff, ydiff;
    float rz;

    mDesignatedObject = threat;
    mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
    AiSaveWeaponState();
    mWeaponsAction = AI_WEAPONS_FREE;

    AiCheckFormStrip();


    ptgt = (AircraftClass*) vuDatabase->Find(mDesignatedObject);

    if (ptgt)
    {
        edata[0] = 2 * (ptgt->Type() - VU_LAST_ENTITY_TYPE);


        xdiff = ptgt->XPos() - self->XPos();
        ydiff = ptgt->YPos() - self->YPos();

        angle = (float)atan2(ydiff, xdiff);
        angle = angle - self->Yaw();
        navangle =  FloatToInt32(RTD * angle);

        if (navangle < 0)
        {
            navangle = 360 + navangle;
        }

        edata[1] = navangle / 30; // scale compass angle for radio eData

        if (edata[1] >= 12)
        {
            edata[1] = 0;
        }

        rz = ptgt->ZPos() - self->ZPos();

        if (rz < 300.0F and rz > -300.0F)   // check relative alt and select correct frag
        {
            edata[2] = 1;
        }
        else if (rz < -300.0F and rz > -1000.0F)
        {
            edata[2] = 2;
        }
        else if (rz < -1000.0F)
        {
            edata[2] = 3;
        }
        else
        {
            edata[2] = 0;
        }

        AiMakeRadioResponse(self, rcENGAGINGC, edata);
    }
}




// ----------------------------------------------------
// DigitalBrain::AiCheckOwnSix
// ----------------------------------------------------

void DigitalBrain::AiCheckOwnSix(FalconWingmanMsg* msg)
{

    int flightIdx;
    short edata[10];
    float az;
    VU_ID threat;
    int direction;
    AircraftClass* pfrom;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    pfrom = (AircraftClass*) vuDatabase->Find(msg->dataBlock.from);

    AiSaveSetSearchDomain(DOMAIN_AIR);
    threat = AiCheckForThreat(self, DOMAIN_AIR, 1, &az);

    if (threat == FalconNullId)
    {

        direction = 2 * FloatToInt32((float)rand() / (float)RAND_MAX);

        if (direction)
        {
            AiSetManeuver(FalconWingmanMsg::WMBreakRight);
        }
        else
        {
            AiSetManeuver(FalconWingmanMsg::WMBreakLeft);
        }


        if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
        {
            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + self->GetCampaignObject()->GetComponentIndex(self) + 1;
            edata[2] = -1;
            edata[3] = -1;
            AiMakeRadioResponse(self, rcCOPY, edata);
            AiCheckFormStrip();
        }
        else
        {
            AiRespondShortCallSign(self);
        }
    }
    else
    {
        AiEngageThreatAtSix(threat);
    }

    AiRestoreSearchDomain();
}


// ----------------------------------------------------
// DigitalBrain::AiBreakLeft
// ----------------------------------------------------

void DigitalBrain::AiBreakLeft(void)
{

    MonoPrint("\tin AiBreakLeft\n");

    AiSetManeuver(FalconWingmanMsg::WMBreakLeft);
    mHeadingOrdered = self->Yaw() - 90.0F * DTR;

    if (mHeadingOrdered <= -180.0F * DTR)
    {
        mHeadingOrdered += 360.0F * DTR;
    }

    mSpeedOrdered = self->GetVt() * FTPSEC_TO_KNOTS;
    mAltitudeOrdered = self->ZPos();
    mnverTime = 15.0F;

}


// ----------------------------------------------------
// DigitalBrain::AiBreakRight
// ----------------------------------------------------

void DigitalBrain::AiBreakRight(void)
{
    MonoPrint("\tin AiBreakRight\n");

    AiSetManeuver(FalconWingmanMsg::WMBreakRight);
    mHeadingOrdered = self->Yaw() + 90.0F * DTR;

    if (mHeadingOrdered > 180.0F * DTR)
    {
        mHeadingOrdered -= 360.0F * DTR;
    }

    mSpeedOrdered = self->GetVt() * FTPSEC_TO_KNOTS;
    mAltitudeOrdered = self->ZPos();
    mnverTime = 15.0F;

}

// ----------------------------------------------------
// DigitalBrain::AiInitSSOffset
// ----------------------------------------------------

void DigitalBrain::AiInitSSOffset(FalconWingmanMsg* msg)
{
    float trigYaw;
    float XSelf;
    float YSelf;
    float ZSelf;
    mlTrig firstTrig;
    int flightIdx;
    short edata[10];
    float side;

    AiSetManeuver(FalconWingmanMsg::WMPince);

    XSelf = self->XPos();
    YSelf = self->YPos();
    ZSelf = self->ZPos();

    // 2002-04-07 ADDED BY S.G. Since SSOffset is available on the menu while we have something bugged, why not use it?
    if (vuDatabase->Find(msg->dataBlock.newTarget))
    {
        mDesignatedObject = msg->dataBlock.newTarget;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        mWeaponsAction                      = AI_WEAPONS_FREE;
        mpSearchFlags[AI_FIXATE_ON_TARGET] = TRUE;
        AiRunTargetSelection();
        // mpActionFlags[AI_USE_COMPLEX]       = TRUE;
        // if (mpActionFlags[AI_EXECUTE_MANEUVER])
        // mpActionFlags[AI_EXECUTE_MANEUVER] = TRUE + 1;
    }

    // END OF ADDED SECTION
    // Find maneuver Axis
    // Start w/ bearing to target/threat,
    // if none use then lead's heading, else
    // use ownship heading
    //LRKLUDGE Need to check passed target here as well
    if (targetPtr)
    {
        trigYaw = self->Yaw() + TargetAz(self, targetPtr);
        mSpeedOrdered    = self->GetVt() * FTPSEC_TO_KNOTS; // Cobra - convert to kts.
        mAltitudeOrdered = self->ZPos();
    }
    else if (flightLead)
    {
        trigYaw = flightLead->Yaw();
        mSpeedOrdered    = flightLead->GetVt() * FTPSEC_TO_KNOTS; // Cobra - convert to kts.
        mAltitudeOrdered = flightLead->ZPos();
    }
    else
    {
        trigYaw = self->Yaw();
        mSpeedOrdered    = self->GetVt() * FTPSEC_TO_KNOTS;
        mAltitudeOrdered = self->ZPos();
    }

    // Get the angles
    mlSinCos(&firstTrig, trigYaw);

    // S.G. isWing has the index position of the plane in the flight. It is ALWAYS less than the number of planes
    // if (isWing > self->GetCampaignObject()->NumberOfComponents())
    // Instead, odd plane number (wingmen) have the 1.0F side. Leaders (flight and element) have the -1.0F side
    // if (isWing bitand 1)
    // 2001-8-03 BUT INSTEAD, I'LL REVERSE IT SO THE WINGS GO TO THE LEFT
    if ( not (isWing bitand 1))
        side = 1.0F;
    else
        side = -1.0F;

    // 2002-04-07 MODIFIED BY S.G. Replaced the constant by FalconSP.cfg vars
    mpManeuverPoints[0][0] = XSelf + firstTrig.cos *  g_fSSoffsetManeuverPoints1a * NM_TO_FT - firstTrig.sin * g_fSSoffsetManeuverPoints1b * NM_TO_FT * side;
    mpManeuverPoints[0][1] = YSelf + firstTrig.sin *  g_fSSoffsetManeuverPoints1a * NM_TO_FT + firstTrig.cos * g_fSSoffsetManeuverPoints1b * NM_TO_FT * side;
    // S.G. SECOND LEG IS JUST 4 NM, NOT 20 NM
    // mpManeuverPoints[1][0] = XSelf + firstTrig.cos * 20.0F * NM_TO_FT - firstTrig.sin * 5.0F * NM_TO_FT * side;
    // mpManeuverPoints[1][1] = YSelf + firstTrig.sin * 20.0F * NM_TO_FT + firstTrig.cos * 5.0F * NM_TO_FT * side;
    mpManeuverPoints[1][0] = XSelf + firstTrig.cos *  g_fSSoffsetManeuverPoints2a * NM_TO_FT - firstTrig.sin * g_fSSoffsetManeuverPoints2b * NM_TO_FT * side;
    mpManeuverPoints[1][1] = YSelf + firstTrig.sin *  g_fSSoffsetManeuverPoints2a * NM_TO_FT + firstTrig.cos * g_fSSoffsetManeuverPoints2b * NM_TO_FT * side;

    mPointCounter = 0;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (flightIdx and msg)
    {
        AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx); // this has nothing to do witht he doSplit variable
    }


    if (msg == NULL)
    {
        AiRespondShortCallSign(self);
    }
    else
    {
        if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
        {
            edata[0] = flightIdx;
            edata[1] = 9; // fixed from "bracketing"
            AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
            AiCheckFormStrip();
        }
        else
        {
            AiRespondShortCallSign(self);
        }
    }
}


// ----------------------------------------------------
// DigitalBrain::AiInitPosthole
// ----------------------------------------------------

void DigitalBrain::AiInitPosthole(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];

    // Find the target given
    if (vuDatabase->Find(msg->dataBlock.newTarget))
    {
        mDesignatedObject = msg->dataBlock.newTarget;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        mWeaponsAction                      = AI_WEAPONS_FREE;
        mpSearchFlags[AI_FIXATE_ON_TARGET] = TRUE;
        AiSetManeuver(FalconWingmanMsg::WMPosthole);
        AiRunTargetSelection();
        mpActionFlags[AI_USE_COMPLEX]       = TRUE;
        mSpeedOrdered = cornerSpeed;
        SetTrackPoint(self->XPos(), self->YPos(), OTWDriver.GetGroundLevel(trackX, trackY) - 4000.0F);
        mPointCounter = 0;

        flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
        AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);

        if (msg == NULL)
        {
            AiRespondShortCallSign(self);
        }
        else
        {
            if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
            {
                edata[0] = flightIdx;
                edata[1] = 5;
                AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
                AiCheckFormStrip();
            }
            else
            {
                AiCheckFormStrip();
                AiRespondShortCallSign(self);
            }
        }

        // 2002-03-15 ADDED BY S.G.
        if (mpActionFlags[AI_EXECUTE_MANEUVER])
            mpActionFlags[AI_EXECUTE_MANEUVER] = TRUE + 1;
    }
    else
    {
        mDesignatedObject = FalconNullId;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        mWeaponsAction                      = AI_WEAPONS_HOLD;
        mpSearchFlags[AI_FIXATE_ON_TARGET] = FALSE;
        AiClearManeuver();
        edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
        edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 +
                      self->GetCampaignObject()->GetComponentIndex(self) + 1;
        edata[2] = -1;
        edata[3] = -1;
        edata[4] = 1;
        AiMakeRadioResponse(self, rcUNABLE, edata);
    }
}


// ----------------------------------------------------
// DigitalBrain::AiInitPince
// ----------------------------------------------------
void DigitalBrain::AiInitPince(FalconWingmanMsg* msg, int doSplit)
{
    float trigYaw;
    float XSelf;
    float YSelf;
    float ZSelf;
    mlTrig firstTrig;
    int flightIdx;
    short edata[10];
    float side;

    AiSetManeuver(FalconWingmanMsg::WMPince);

    XSelf = self->XPos();
    YSelf = self->YPos();
    ZSelf = self->ZPos();

    // 2002-04-07 ADDED BY S.G. Since Pince is available on the menu while we have something bugged, why not use it?
    if (vuDatabase->Find(msg->dataBlock.newTarget))
    {
        mDesignatedObject = msg->dataBlock.newTarget;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        mWeaponsAction                      = AI_WEAPONS_FREE;
        mpSearchFlags[AI_FIXATE_ON_TARGET] = TRUE;
        AiRunTargetSelection();
        // mpActionFlags[AI_USE_COMPLEX]       = TRUE;
        // if (mpActionFlags[AI_EXECUTE_MANEUVER])
        // mpActionFlags[AI_EXECUTE_MANEUVER] = TRUE + 1;
    }

    // END OF ADDED SECTION

    // Find maneuver Axis
    // Start w/ bearing to target/threat,
    // if none use then lead's heading, else
    // use ownship heading
    //LRKLUDGE Need to check passed target here as well
    if (targetPtr)
    {
        trigYaw = self->Yaw() + TargetAz(self, targetPtr);
        mSpeedOrdered    = self->GetVt() * FTPSEC_TO_KNOTS; // Cobra - convert to kts.
        mAltitudeOrdered = self->ZPos();
    }
    else if (flightLead)
    {
        trigYaw = flightLead->Yaw();
        mSpeedOrdered    = flightLead->GetVt() * FTPSEC_TO_KNOTS; // Cobra - convert to kts.
        mAltitudeOrdered = flightLead->ZPos();
    }
    else
    {
        trigYaw = self->Yaw();
        mSpeedOrdered    = self->GetVt() * FTPSEC_TO_KNOTS;
        mAltitudeOrdered = self->ZPos();
    }

    // Get the angles
    mlSinCos(&firstTrig, trigYaw);

    // S.G. isWing has the index position of the plane in the flight. It is ALWAYS less than the number of planes
    // if (doSplit and isWing > self->GetCampaignObject()->NumberOfComponents())
    // Instead, odd plane number (wingmen) have the 1.0F side. Leaders (flight and element) have the -1.0F side
    if (doSplit and (isWing bitand 1))
        side = 1.0F;
    else
        side = -1.0F;

    // 2002-04-07 MODIFIED BY S.G. Replaced the constant by FalconSP.cfg vars
    mpManeuverPoints[0][0] = XSelf + firstTrig.cos *  g_fPinceManeuverPoints1a * NM_TO_FT - firstTrig.sin * g_fPinceManeuverPoints1b * NM_TO_FT * side;
    mpManeuverPoints[0][1] = YSelf + firstTrig.sin *  g_fPinceManeuverPoints1a * NM_TO_FT + firstTrig.cos * g_fPinceManeuverPoints1b * NM_TO_FT * side;
    // S.G. SECOND LEG IS JUST 4 NM, NOT 20 NM
    // mpManeuverPoints[1][0] = XSelf + firstTrig.cos * 20.0F * NM_TO_FT - firstTrig.sin * 5.0F * NM_TO_FT * side;
    // mpManeuverPoints[1][1] = YSelf + firstTrig.sin * 20.0F * NM_TO_FT + firstTrig.cos * 5.0F * NM_TO_FT * side;
    mpManeuverPoints[1][0] = XSelf + firstTrig.cos *  g_fPinceManeuverPoints2a * NM_TO_FT - firstTrig.sin * g_fPinceManeuverPoints2b * NM_TO_FT * side;
    mpManeuverPoints[1][1] = YSelf + firstTrig.sin *  g_fPinceManeuverPoints2a * NM_TO_FT + firstTrig.cos * g_fPinceManeuverPoints2b * NM_TO_FT * side;

    mPointCounter = 0;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (flightIdx and msg)
    {
        AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx); // this has nothing to do witht he doSplit variable
    }


    if (msg == NULL)
    {
        AiRespondShortCallSign(self);
    }
    else
    {
        if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
        {
            edata[0] = flightIdx;
            edata[1] = 4;
            AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
            AiCheckFormStrip();
        }
        else
        {
            AiRespondShortCallSign(self);
        }
    }
}



// ----------------------------------------------------
// DigitalBrain::AiInitFlex
// ----------------------------------------------------

void DigitalBrain::AiInitFlex(void)
{
    mlTrig firstTrig;
    mlTrig secondTrig;
    float XSelf;
    float YSelf;
    float ZSelf;
    float spacing = NM_TO_FT;

    AiSetManeuver(FalconWingmanMsg::WMPince);

    XSelf = self->XPos();
    YSelf = self->YPos();
    ZSelf = self->ZPos();

    AiInitTrig(&firstTrig, &secondTrig);

    mpManeuverPoints[0][0] = XSelf + secondTrig.cos * spacing;
    mpManeuverPoints[0][1] = YSelf + firstTrig.sin * spacing;
    mpManeuverPoints[1][0] = XSelf - secondTrig.cos * 2.0F * spacing;
    mpManeuverPoints[1][1] = YSelf - secondTrig.sin * 2.0F * spacing;
    mpManeuverPoints[2][0] = XSelf - secondTrig.cos * 2.1F * spacing;
    mpManeuverPoints[2][1] = YSelf - secondTrig.sin * 2.1F * spacing;

    mAltitudeOrdered = self->ZPos();
    mSpeedOrdered = self->GetVt() * FTPSEC_TO_KNOTS;
    mPointCounter = 0;
}


// ----------------------------------------------------
// DigitalBrain::AiInitTrig
// ----------------------------------------------------

void DigitalBrain::AiInitTrig(mlTrig* firstTrig, mlTrig* secondTrig)
{
    float trigYaw;

    // Find maneuver Axis
    // Start w/ bearing to target/threat,
    // if none use then lead's heading, else
    // use ownship heading
    if (targetPtr)
    {
        trigYaw = self->Yaw() + TargetAz(self, targetPtr);
    }
    else if (flightLead)
    {
        trigYaw = flightLead->Yaw();
    }
    else
    {
        trigYaw = self->Yaw();
    }

    // Get the angles
    mlSinCos(firstTrig, trigYaw);

    trigYaw += 90.0F * DTR;

    if (trigYaw > 180.0F * DTR)
    {
        trigYaw -= (360.0F * DTR);
    }

    mlSinCos(secondTrig, trigYaw);

    // Go left/right based on position in flight and
    // number of people in flight.
    // For a 2 ship 0 goes right, 1 goes left
    // In a 4 ship 0 bitand 1 go right, 2 bitand 3 go left

    if (isWing >= 2 or (isWing == 1 and self->GetCampaignObject()->NumberOfComponents() < 3))
    {
        firstTrig->cos = -firstTrig->cos;
        firstTrig->sin = -firstTrig->sin;
    }

#if 0
    float dx, dy, dz;
    float ry;
    float YawSelf;

    dx = flightLead->XPos() - XSelf;
    dy = flightLead->YPos() - YSelf;
    dz = flightLead->ZPos() - ZSelf;

    ry = self->dmx[1][0] * dx + self->dmx[1][1] * dy + self->dmx[1][2] * dz;

    YawSelf = self->Yaw();

    mlSinCos(firstTrig, YawSelf);

    YawSelf += 90.0F * DTR;

    if (YawSelf > 180.0F * DTR)
    {
        YawSelf -= (360.0F * DTR);
    }

    mlSinCos(secondTrig, YawSelf);

    if (ry > 0.0F)   // Break left
    {
        firstTrig->cos = -firstTrig->cos;
        firstTrig->sin = -firstTrig->sin;
    }

#endif
}

// ----------------------------------------------------
// DigitalBrain::AiInitChainsaw
// ----------------------------------------------------

void DigitalBrain::AiInitChainsaw(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];

    // Find the target given

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (vuDatabase->Find(msg->dataBlock.newTarget))
    {
        mDesignatedObject = msg->dataBlock.newTarget;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        mWeaponsAction                      = AI_WEAPONS_FREE;
        mpSearchFlags[AI_FIXATE_ON_TARGET] = TRUE;
        AiSetManeuver(FalconWingmanMsg::WMChainsaw);
        AiRunTargetSelection();
        mpActionFlags[AI_USE_COMPLEX]       = TRUE;
        AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);

        // 2002-03-15 ADDED BY S.G.
        if (mpActionFlags[AI_EXECUTE_MANEUVER])
            mpActionFlags[AI_EXECUTE_MANEUVER] = TRUE + 1;
    }
    else
    {
        mDesignatedObject = FalconNullId;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        mWeaponsAction                      = AI_WEAPONS_HOLD;
        mpSearchFlags[AI_FIXATE_ON_TARGET] = FALSE;
        AiClearManeuver();
    }


    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 6;
        AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
        AiCheckFormStrip();
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}


// ----------------------------------------------------
// DigitalBrain::AiGoShooter
// ----------------------------------------------------

void DigitalBrain::AiGoShooter(void)
{
    if (mpActionFlags[AI_ENGAGE_TARGET] == AI_NONE) // 2002-03-04 ADDED BY S.G. Change it if not already set, assume an air target (can't tell)
        mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type

    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mWeaponsAction = AI_WEAPONS_FREE;

    mpSearchFlags[AI_FIXATE_ON_TARGET] = TRUE;
    mpSearchFlags[AI_MONITOR_TARGET] = FALSE;
    mpActionFlags[AI_RTB]               = FALSE;

    AiClearManeuver();

    //MonoPrint("Going Shooter\n");
}

// ----------------------------------------------------
// DigitalBrain::AiGoCover
// ----------------------------------------------------

void DigitalBrain::AiGoCover(void)
{

    mpActionFlags[AI_ENGAGE_TARGET]   = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mpActionFlags[AI_RTB]              = FALSE;
    mWeaponsAction                     = AI_WEAPONS_HOLD;

    mpSearchFlags[AI_FIXATE_ON_TARGET] = FALSE;
    mpSearchFlags[AI_MONITOR_TARGET]   = TRUE;

    AiClearManeuver();

    // 2001-06-16 ADDED BY S.G. NEED TO GO BACK IN NAV MODE.
    if (self->AutopilotType() == AircraftClass::CombatAP or self->IsDigital()) // 2002-01-28 ADDED BY S.G But only if in CombatAP
        self->FCC->SetMasterMode(FireControlComputer::Nav);

    // END OF ADDED SECTION

    //MonoPrint("Going Cover\n");
}

// ----------------------------------------------------
// DigitalBrain::AiSearchForTargets
// ----------------------------------------------------

void DigitalBrain::AiSearchForTargets(char domain)
{
    mSearchDomain = domain;
    // 2000-09-13 MODIFIED BY S.G. PRETTY USELESS LINE IF YOU ASK ME... NOT IN RP4
    // mpSearchFlags[AI_SEARCH_FOR_TARGET];
    mpSearchFlags[AI_SEARCH_FOR_TARGET] = TRUE; // Cobra - try turning them loose
    mLastReportTime = 0;
}

// ----------------------------------------------------
// DigitalBrain::AiResumeFlightPlan
// ----------------------------------------------------

void DigitalBrain::AiResumeFlightPlan(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];

    mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mpActionFlags[AI_RTB]               = FALSE;

    mpSearchFlags[AI_FIXATE_ON_TARGET] = FALSE;
    mpSearchFlags[AI_MONITOR_TARGET] = FALSE;

    mDesignatedType = AI_NO_DESIGNATED;
    mWeaponsAction = AI_WEAPONS_HOLD;

    AiClearManeuver();

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    AiGlueFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}

// ----------------------------------------------------
// DigitalBrain::AiRejoin
// ----------------------------------------------------

void DigitalBrain::AiRejoin(FalconWingmanMsg* msg, AiHint hint)
{
    short edata[10];
    int flightIdx;

    //we can't rejoin if we're on the ground still
    if (self->OnGround() or atcstatus >= lOnFinal)
        return;

    AiCheckPosition();
    // mInPositionFlag = FALSE;

    mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mpActionFlags[AI_FOLLOW_FORMATION] = TRUE;
    mpActionFlags[AI_RTB] = FALSE;
    mpActionFlags[AI_LANDING] = FALSE;

    SendATCMsg(noATC);
    atcstatus = noATC;
    // cancel atc here

    // 2001-07-11 ADDED BY S.G. NEED TO SET THE SAME WAYPOINT AS THE LEAD ONCE WE REJOIN...
    WayPointClass* wlistUs   = self->waypoint;
    WayPointClass* wlistLead = NULL;

    if (flightLead)
        wlistLead = ((AircraftClass *)flightLead)->waypoint;

    // This will set our current waypoint to the leads waypoint
    // 2001-10-20 Modified by M.N. Added ->GetNextWP() to assure that we get a valid waypoint
    while (wlistUs->GetNextWP() and wlistLead and wlistLead->GetNextWP() and wlistLead not_eq ((AircraftClass *)flightLead)->curWaypoint)
    {
        wlistUs   = wlistUs->GetNextWP();
        wlistLead = wlistLead->GetNextWP();
    }

    self->curWaypoint = wlistUs;
    // END OF ADDED SECTION

    rwIndex = 0;
    self->af->gearHandle = -1.0F; //up

    mpActionFlags[AI_USE_COMPLEX]       = FALSE;

    mpSearchFlags[AI_FIXATE_ON_TARGET] = FALSE;

    AiClearManeuver();

    mFormLateralSpaceFactor = 1.0F;
    mFormSide = 1;
    mFormRelativeAltitude = 0.0F;
    mDesignatedObject = FalconNullId;

    // 2001-05-22 ADDED BY S.G. NEED TO TELL AI TO STOP THEIR GROUND ATTACK
    agDoctrine = AGD_NONE;
    SetGroundTarget(NULL);

    // 2001-06-30 ADDED BY S.G. WHEN REJOINING, THE HasAGWeapon IS REFLECTED INTO HasCanUseAGWeapon SO IF THE LEAD WITH ONLY HARMS FIRED THEM AT A ENROUTE TARGET, WHEN SWITCHING TO THE ATTACK TARGET, HE DOESN'T ABORT THINKING NO ONE CAN FIRE ANYTHING...
    // 2001-10-23 MODIFIED BY S.G. Only if the rejoin comes from the lead and not from yourself rejoining on your own so the lead doesn't think your HARMS can be used on the target you were bombing
    if (hint == AI_REJOIN and IsSetATC(HasAGWeapon))
        SetATCFlag(HasCanUseAGWeapon);

    // END OF ADDED SECTION

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    // 2002-03-10 MN when ordered AI to rejoin, rejoin immediately, not only when connected to the boom
    // if(self->af->IsSet(Refueling)) {
    if (refuelstatus not_eq refNoTanker and refuelstatus not_eq refDone)
    {
        VuEntity* theTanker = vuDatabase->Find(tankerId);
        FalconTankerMessage* TankerMsg;

        if (theTanker)
            TankerMsg = new FalconTankerMessage(theTanker->Id(), FalconLocalGame);
        else
            TankerMsg = new FalconTankerMessage(FalconNullId, FalconLocalGame);

        TankerMsg->dataBlock.type = FalconTankerMessage::DoneRefueling;
        TankerMsg->dataBlock.data1 = 1;
        TankerMsg->dataBlock.caller = self->Id();
        FalconSendMessage(TankerMsg);
    }


    // edg: can't msg be NULL?  It looks like some calls use it and I
    // want to use this function to have the wingies return to formation
    // after their AG attack is done.
    if (msg)
        AiGlueFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);

    if (msg and AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = -1;
        edata[1] = -1;
        edata[2] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
        edata[3] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
        AiMakeRadioResponse(self, rcONMYWAY, edata);
    }
    else if (hint == AI_TAKEOFF)   // JPO take the hint
    {
        short edata[10];

        edata[0] = ((FlightClass*) self->GetCampaignObject())->GetComponentIndex(self);
        edata[1] = -1;
        edata[2] = -1;
        edata[3] = -1;
        edata[4] = -1;
        edata[5] = -1;
        edata[6] = -1;
        edata[7] = -1;
        edata[8] = -1;
        edata[9] = -1;
        AiMakeRadioResponse(self, rcLIFTOFF, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }

    // 2002-02-23 MN when ordered to rejoin, AI must make weapons safe
    mWeaponsAction = AI_WEAPONS_HOLD;
}


// ----------------------------------------------------
// ----------------------------------------------------
//
// Commands that modify the state basis
//
// ----------------------------------------------------
// ----------------------------------------------------

// ----------------------------------------------------
// DigitalBrain::AiDesignateTarget
// ----------------------------------------------------

void DigitalBrain::AiDesignateTarget(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];
    FalconEntity* newTarg = (FalconEntity*)vuDatabase->Find(msg->dataBlock.newTarget);

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (newTarg)
    {
        // 2001-07-17 ADDED BY S.G. WHEN TOLD TO DESIGNATE AND IT'S A PLAYER'S WING, LOOSE YOUR HISTORY BECAUSE YOU MIGHT HAVE CHOSEN SOMETHING TO HIT BY YOURSELF ALREADY
        //            THE TARGET HERE WILL BE A CAMPAIGN OBJECT BUT ONCE THE HISTORY IS REMOVED, THE SAME SIM OBJECT CAN BE CHOSEN.
        if (flightLead and flightLead->IsSetFlag(MOTION_OWNSHIP))
            gndTargetHistory[0] = NULL;

        // END OF ADDED SECTION

        // Try not to attack friendlies
        if (newTarg->GetTeam() not_eq self->GetTeam() or (SkillLevel() < 2 and rand() % 10 > SkillLevel() + 8))
        {
            mWeaponsAction = AI_WEAPONS_FREE;

            // 2000-09-26 ADDED BY S.G. SO ASSIGN GROUP WORKS BY ASSIGNING TARGETS ACCORDING TO THEIR POSITION IN FLIGHT (LIKE FOR THE AI)
            if ((FalconWingmanMsg::WingManCmd) msg->dataBlock.command == FalconWingmanMsg::WMAssignGroup)
            {
                // If it's a sim object, get the corresponding campaign object and assign it
                if (((FalconEntity*)newTarg)->IsSim())
                    AiSearchTargetList(((SimBaseClass *)newTarg)->GetCampaignObject());
                else
                    AiSearchTargetList(newTarg);

                if (targetPtr)
                    mDesignatedObject = targetPtr->BaseData()->Id();
            }
            else
                // END OF ADDED SECTION (EXCEPT FOR INDENT OF THE NEXT LINE)
                mDesignatedObject = msg->dataBlock.newTarget;

            // mpActionFlags[AI_ENGAGE_TARGET] = TRUE; // 2002-03-04 REMOVED BY S.G. Done within the "if (newTarg->OnGround())" test below now

            mpActionFlags[AI_RTB]               = FALSE;
            mCurrentManeuver = FalconWingmanMsg::WMTotalMsg;


            if (newTarg->OnGround())
            {
                mpActionFlags[AI_ENGAGE_TARGET] = AI_GROUND_TARGET; // 2002-03-04 ADDED BY S.G. It's a ground target, say that's what we're engaging
                // 2001-06-20 ADDED BY S.G. NEED TO TELL AI THERE AG MISSION IS NOT COMPLETE ANYMORE
                missionComplete = FALSE;
                // END OF ADDED SECTION

                SetGroundTarget(newTarg);

                if (self->AutopilotType() == AircraftClass::CombatAP)
                {
                    SetupAGMode(NULL, NULL);

                    if (groundTargetPtr == NULL)
                    {
                        mDesignatedObject = FalconNullId;
                        mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type

                        edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
                        edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + self->GetCampaignObject()->GetComponentIndex(self) + 1;
                        edata[2] = -1;
                        edata[3] = -1;
                        edata[4] = 0;
                        AiMakeRadioResponse(self, rcUNABLE, edata);
                    }
                    else
                    {
                        edata[0] = flightIdx;
                        edata[1] = 2;
                        AiMakeRadioResponse(self, rcROGER, edata);
                        AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);
                    }
                }
                else
                {
                    agDoctrine = AGD_NEED_SETUP;
                }
            }
            else
            {
                // 2000-10-11 ADDED BY S.G. NEED TO SET agDoctrine TO AGD_NONE AND CLEAR OUT THE GROUND TARGET SO IT WON'T ATTACK GROUND TARGET INSTEAD OF THE DESIGNATED AIR ONE
                // 2002-03-04 ADDED BY S.G. Only reset the agDoctrine and groundTargetPtr if it's comming from the player. That way, AI will not react too quickly to target change
                // 2002-03-07 MODIFIED BY S.G. Make sure flightlead and myLead are non NULL before doing a ->IsPlayer on them...
                int leadIsPlayer = FALSE;

                if (flightLead and flightLead->IsPlayer())
                    leadIsPlayer = TRUE;
                else
                {
                    if (isWing == AiSecondWing)
                    {
                        AircraftClass *myLead;
                        myLead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);

                        if (myLead and myLead->IsPlayer())
                            leadIsPlayer = TRUE;
                    }
                }

                if (leadIsPlayer)
                    // END OF ADDED SECTION 2002-03-04
                {
                    agDoctrine = AGD_NONE;
                    SetGroundTarget(NULL);
                }

                // END OF ADDED SECTION 2000-10-11
                // 2002-03-04 ADDED BY S.G. Prioritize ground target over air target but if needed be, it will attack anyway
                if (mpActionFlags[AI_ENGAGE_TARGET] == AI_NONE)
                    mpActionFlags[AI_ENGAGE_TARGET] = AI_AIR_TARGET;

                // END OF ADDED SECTION 2002-03-04
                edata[0] = flightIdx;
                edata[1] = 2;
                AiMakeRadioResponse(self, rcROGER, edata);

                AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);
            }
        }
        else
        {
            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 +
                          self->GetCampaignObject()->GetComponentIndex(self) + 1;
            edata[2] = -1;
            edata[3] = -1;
            edata[4] = 1;
            AiMakeRadioResponse(self, rcUNABLE, edata);
            return;
        }
    }
    else
    {
        mDesignatedObject = FalconNullId;
        mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
    }
}

// ----------------------------------------------------
// DigitalBrain::AiDesignateGroup
// ----------------------------------------------------

void DigitalBrain::AiDesignateGroup(FalconWingmanMsg* msg)
{
#if 0
    SimBaseClass* psimBase;
    SimBaseClass* ptarget;

    mDesignatedObject = FalconNullId;


    psimBase = (SimBaseClass*) vuDatabase->Find(msg->dataBlock.newTarget);

    // VWF caution what about things that are not vehicles?
    // if it is a vehicle
    if (psimBase and psimBase->campaignObject->components)
    {
        VuListIterator elementWalker(psimBase->campaignObject->components);
        // pick the closest to my side of formation
        ptarget = (SimBaseClass*)elementWalker.GetFirst();
        //ptarget = (SimBaseClass*)elementWalker.GetNext();

        SetTarget(ptarget);
    }
    else
    {
        SetTarget(NULL);
    }

#endif
}

// ----------------------------------------------------
// DigitalBrain::AiSetWeaponsAction
// ----------------------------------------------------

void DigitalBrain::AiSetWeaponsAction(FalconWingmanMsg* msg, DigitalBrain::AiWeaponsAction action)
{
    WayPointClass* tmpWaypoint = self->waypoint;
    int flightIdx;
    short edata[10];

    mWeaponsAction = action;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    // 2000-09-28 ADDED BY S.G. IF ASKED TO GO WEAPON HOLD, SET 'missileShotTimer' AT 2 HOURS FROM NOW
    if (action == AI_WEAPONS_HOLD)
    {
        missileShotTimer = SimLibElapsedTime + 2 * 60 * 60 * SEC_TO_MSEC;
        AiRejoin(NULL);

    }

    //Cobra TJL let's remove the WaitingPermission.  If I give weaponsfree I'm expecting the AI
    //to get its game on and find targets
    if (action == AI_WEAPONS_FREE and missionClass == AGMission /* and IsSetATC(WaitingPermission)*/)
    {
        missileShotTimer = 0;

        // Find the IP waypoint
        while (tmpWaypoint)
        {
            // 2000-09-28 MODIFIED BY S.G. WHAT IF WE DON'T HAVE AN IP?
            // IN THAT CASE, CHECK IF TARGET, IF SUCH, GO BACK ONE WAYPOINT AND BREAK
            if (tmpWaypoint->GetWPFlags() bitand WPF_TARGET)
            {
                tmpWaypoint = tmpWaypoint->GetPrevWP();
                break;
            }

            if (tmpWaypoint->GetWPFlags() bitand WPF_IP)
                break;

            tmpWaypoint = tmpWaypoint->GetNextWP();
        }

        // Have an IP
        if (tmpWaypoint)
            self->curWaypoint = tmpWaypoint->GetNextWP();

        // 2000-09-28 MODIFIED BY S.G. WE SAVE THE CURRENT GROUND TARGET, CLEAR IT, CALL THE ROUTINE AND IF IT CAN'T FIND ONE, RESTORE IT
        // SelectGroundTarget (TARGET_ANYTHING);
        //Cobra... this gets called when  you give weapons free and retargets??? why?
        //Will this allow a retargeting?
        /*SimObjectType* tmpGroundTargetPtr = groundTargetPtr;
        groundTargetPtr = 0;
        SelectGroundTarget (TARGET_ANYTHING);
        if (groundTargetPtr == NULL)
         groundTargetPtr = tmpGroundTargetPtr;*/
        groundTargetPtr = NULL;//Ok, let's force a reevaluation each command.

        if (groundTargetPtr == NULL)//cobra
            SelectGroundTarget(TARGET_ANYTHING);

        SetupAGMode(NULL, NULL);

        if (groundTargetPtr == NULL)
        {
            mDesignatedObject = FalconNullId;
            mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type

            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 +
                       self->GetCampaignObject()->GetComponentIndex(self) + 1;
            edata[2] = -1;
            edata[3] = -1;
            edata[4] = 0;
            AiMakeRadioResponse(self, rcUNABLE, edata);
        }
        else
        {
            // 2000-09-27 ADDED BY S.G. WE'LL CLEAR THE FLAG *ONLY* IF WE HAVE A TARGET
            ClearATCFlag(WaitingPermission);

            mDesignatedObject = groundTargetPtr->BaseData()->Id();
            mpActionFlags[AI_ENGAGE_TARGET] = AI_GROUND_TARGET; // 2002-03-04 MODIFIED BY S.G. Use new enum type
            mpActionFlags[AI_RTB]               = FALSE;
            edata[0] = flightIdx;
            edata[1] = 2;
            AiMakeRadioResponse(self, rcROGER, edata);
        }
    }
    else
    {
        // RV - Biker - Allow them to fire missiles
        if (action == AI_WEAPONS_FREE)
            missileShotTimer = 0;

        mpActionFlags[AI_RTB] = FALSE;

        if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
        {
            edata[0] = flightIdx;
            edata[1] = 2;
            AiMakeRadioResponse(self, rcROGER, edata);
        }
        else
        {
            AiRespondShortCallSign(self);
        }
    }
}




// ----------------------------------------------------
// ----------------------------------------------------
//
// Commands that modify formation
//
// ----------------------------------------------------
// ----------------------------------------------------

void DigitalBrain::AiSetFormation(FalconWingmanMsg* msg)
{

    short edata[10];
    int flightIdx;

    //we can't fly in formation if we're on the ground still
    if (self->OnGround() or atcstatus >= lOnFinal)
        return;

    // mInPositionFlag = FALSE;
    AiCheckPosition();

    // mFormLateralSpaceFactor = 1.0F;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    mFormation = acFormationData->FindFormation(msg->dataBlock.command);
    mpActionFlags[AI_FOLLOW_FORMATION] = TRUE;
    SendATCMsg(noATC);
    atcstatus = noATC;
    // cancel atc here

    AiGlueFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {

        edata[1] = -1;
        edata[2] = -1;
        edata[3] = -1;
        edata[4] = -1;
        edata[5] = -1;
        edata[6] = -1;
        edata[7] = -1;
        edata[8] = -1;
        edata[9] = -1;

        edata[0] = flightIdx;


        int radioform = mFormation;

        // M.N. hack the next formation message indexes (currently 59,60,61)
        if (radioform > 8)
            radioform = mFormation + 50;

        switch (radioform)
        {
            case FalconWingmanMsg::WMSpread:
                edata[1] = 1;
                break;

            case FalconWingmanMsg::WMWedge:
                edata[1] = 2;
                break;

            case FalconWingmanMsg::WMTrail:
                edata[1] = 3;
                break;

            case FalconWingmanMsg::WMLadder:
                edata[1] = 4;
                break;

            case FalconWingmanMsg::WMStack:
                edata[1] = 5;
                break;

            case FalconWingmanMsg::WMResCell:
                edata[1] = 6;
                break;

            case FalconWingmanMsg::WMBox:
                edata[1] = 7;
                break;

            case FalconWingmanMsg::WMArrowHead:
                edata[1] = 8;
                break;

            case FalconWingmanMsg::WMFluidFour:
                edata[1] = 14;
                break;

            case FalconWingmanMsg::WMVic:
                edata[1] = 10;
                break;

            case FalconWingmanMsg::WMEchelon:
                edata[1] = 11;
                break;

            case FalconWingmanMsg::WMFinger4:
                edata[1] = 13;
                break;

            case FalconWingmanMsg::WMForm1:
                edata[1] = 15;
                break;

            case FalconWingmanMsg::WMForm2:
                edata[1] = 16;
                break;

            case FalconWingmanMsg::WMForm3:
                edata[1] = 17;
                break;

            case FalconWingmanMsg::WMForm4:
                edata[1] = 18;
                break;
        }

        AiMakeRadioResponse(self, rcFORMRESPONSEB, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}


void DigitalBrain::AiKickout(FalconWingmanMsg* msg)
{

    short edata[10];
    int flightIdx;

    // mInPositionFlag = FALSE;
    AiCheckPosition();

    mFormLateralSpaceFactor *= 2.0F;
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {

        edata[1] = -1;
        edata[2] = -1;
        edata[3] = -1;
        edata[4] = -1;
        edata[5] = -1;
        edata[6] = -1;
        edata[7] = -1;
        edata[8] = -1;
        edata[9] = -1;

        edata[0] = flightIdx;
        edata[1] = 2;

        AiMakeRadioResponse(self, rcFORMRESPONSEA, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}


void DigitalBrain::AiCloseup(FalconWingmanMsg* msg)
{

    short edata[10];
    int flightIdx;

    // mInPositionFlag = FALSE;
    AiCheckPosition();

    mFormLateralSpaceFactor *= 0.5F;
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    //MI give us a negative if we can't get closer
    if (mFormLateralSpaceFactor < 0.0625F)
    {
        edata[2] = -1;
        edata[3] = -1;
        edata[4] = -1;
        edata[5] = -1;
        edata[6] = -1;
        edata[7] = -1;
        edata[8] = -1;
        edata[9] = -1;

        edata[0] = flightIdx;
        edata[1] = 5;

        AiMakeRadioResponse(self, rcFORMRESPONSEA, edata);
    }
    else
    {
        if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
        {

            edata[1] = -1;
            edata[2] = -1;
            edata[3] = -1;
            edata[4] = -1;
            edata[5] = -1;
            edata[6] = -1;
            edata[7] = -1;
            edata[8] = -1;
            edata[9] = -1;

            edata[0] = flightIdx;
            edata[1] = 3;

            AiMakeRadioResponse(self, rcFORMRESPONSEA, edata);
        }
        else
        {
            AiRespondShortCallSign(self);
        }
    }
}


void DigitalBrain::AiToggleSide(void)
{
    mFormSide *= -1; // +1 normal formation, -1 is mirrored formation
    AiRespondShortCallSign(self);
}

void DigitalBrain::AiIncreaseRelativeAltitude(void)
{
    mFormRelativeAltitude -= 1000.0F;
    // do radio response here
    //MI making reply
    short edata[10];
    int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    edata[1] = -1;
    edata[2] = -1;
    edata[3] = -1;
    edata[4] = -1;
    edata[5] = -1;
    edata[6] = -1;
    edata[7] = -1;
    edata[8] = -1;
    edata[9] = -1;

    edata[0] = flightIdx;
    edata[1] = 0;

    AiMakeRadioResponse(self, rcFORMRESPONSEA, edata);

}

void DigitalBrain::AiDecreaseRelativeAltitude(void)
{
    mFormRelativeAltitude += 1000.0F;
    // do radio response here
    //MI making reply
    short edata[10];
    int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    edata[1] = -1;
    edata[2] = -1;
    edata[3] = -1;
    edata[4] = -1;
    edata[5] = -1;
    edata[6] = -1;
    edata[7] = -1;
    edata[8] = -1;
    edata[9] = -1;

    edata[0] = flightIdx;
    edata[1] = 1;

    AiMakeRadioResponse(self, rcFORMRESPONSEA, edata);
}



// ----------------------------------------------------
// ----------------------------------------------------
//
// Transient Commands
//
// ----------------------------------------------------
// ----------------------------------------------------

// ----------------------------------------------------
// AIWingClass::AiGiveBra
// ----------------------------------------------------

void DigitalBrain::AiGiveBra(FalconWingmanMsg* msg)
{
    short edata[10];
    float rx, ry, rz;
    float dsq;
    int response;
    float xdiff, ydiff;
    float angle;
    int navangle;

    AircraftClass* psender;

    psender = (AircraftClass*) vuDatabase->Find(msg->dataBlock.from);


    rx = self->XPos() - psender->XPos();
    ry = self->YPos() - psender->YPos();
    rz = self->ZPos() - psender->ZPos();

    navangle = FloatToInt32(ConvertRadtoNav((float)atan2(ry, rx))); // convert to compass angle

    dsq = rx * rx + ry * ry;

    if (dsq < NM_TO_FT * NM_TO_FT)
    {

        edata[0] = self->GetCampaignObject()->GetComponentIndex(self);

        xdiff = self->XPos() - psender->XPos();
        ydiff = self->YPos() - psender->YPos();

        angle = (float)atan2(ydiff, xdiff);
        angle = angle - psender->Yaw();
        navangle =  FloatToInt32(RTD * angle);

        if (navangle < 0)
        {
            navangle = 360 + navangle;
        }

        edata[1] = navangle / 30; // scale compass angle for radio eData

        if (edata[1] >= 12)
        {
            edata[1] = 0;
        }

        /*
         edata[1] = navangle / 30; // scale compass angle for radio eData
         if(edata[1] >= 12) {
         edata[1] = 0;
         }
        */
        if (rz < 300.0F and rz > -300.0F)   // check relative alt and select correct frag
        {
            edata[2] = 1;
        }
        else if (rz < -300.0F and rz > -1000.0F)
        {
            edata[2] = 2;
        }
        else if (rz < -1000.0F)
        {
            edata[2] = 3;
        }
        else
        {
            edata[2] = 0;
        }

        response = rcPOSITIONRESPONSEB;
    }
    else
    {
        edata[0] = ((FlightClass*)psender->GetCampaignObject())->callsign_id;
        edata[1] = (((FlightClass*)psender->GetCampaignObject())->callsign_num - 1) * 4 + psender->GetCampaignObject()->GetComponentIndex(psender) + 1;
        edata[2] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
        edata[3] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + self->GetCampaignObject()->GetComponentIndex(self) + 1;
        edata[4] = (short) SimToGrid(self->YPos());
        edata[5] = (short) SimToGrid(self->XPos());
        edata[6] = (short) - self->ZPos() ;

        response = rcPOSITIONRESPONSEA;

    }

    AiMakeRadioResponse(self, response, edata);
}

// ----------------------------------------------------
// AIWingClass::AiGiveStatus
// ----------------------------------------------------

void DigitalBrain::AiGiveStatus(FalconWingmanMsg* msg)
{
    short edata[10];
    int response;
    int random;
    float xdiff;
    float ydiff;
    float rz;
    int flightIdx;
    FalconEntity* pmytarget = NULL;
    AircraftClass* pfrom;
    float angle;
    int navangle;

    if (targetPtr)
    {
        pmytarget = targetPtr->BaseData();
    }

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if ((curMode == GunsJinkMode or curMode == MissileDefeatMode) and pmytarget and (pmytarget->IsAirplane() or pmytarget->IsHelicopter()))
    {

        xdiff = self->XPos() - pmytarget->XPos();
        ydiff = self->YPos() - pmytarget->YPos();

        if (xdiff * xdiff + ydiff * ydiff > NM_TO_FT * NM_TO_FT)
        {
            if (PlayerOptions.BullseyeOn())
            {
                response = rcENGDEFENSIVEA;
            }
            else
            {
                response = rcENGDEFENSIVEB;
            }

            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
            edata[2] = (short) SimToGrid(pmytarget->YPos());
            edata[3] = (short) SimToGrid(pmytarget->XPos());
            edata[4] = (short) pmytarget->ZPos();
        }
        else
        {
            edata[0] = flightIdx;
            response = rcENGDEFENSIVEC;
        }
    }
    else if (pmytarget and (pmytarget->IsAirplane() or pmytarget->IsHelicopter()))
    {

        edata[0] = -1;
        edata[1] = -1;
        edata[2] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
        edata[3] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
        edata[4] = (short) SimToGrid(pmytarget->YPos());
        edata[5] = (short) SimToGrid(pmytarget->XPos());
        edata[6] = (short) pmytarget->ZPos();

        response = rcAIRTARGETBRA;

        // rcBANDIT
    }
    else if (mpActionFlags[AI_EXECUTE_MANEUVER]/* == TRUE *//* 2002-03-15 REMOVED BY S.G. Can be TRUE or TRUE+1 now */)
    {

        edata[0] = self->GetCampaignObject()->GetComponentIndex(self);

        switch (mCurrentManeuver)
        {

            case FalconWingmanMsg::WMChainsaw:
                edata[1] = 6;
                break;

            case FalconWingmanMsg::WMPince:
                edata[1] = 4;
                break;

            case FalconWingmanMsg::WMPosthole:
                edata[1] = 5;
                break;

            case FalconWingmanMsg::WMFlex:
            case FalconWingmanMsg::WMSkate:
            case FalconWingmanMsg::WMSSOffset:
                edata[1] = 8;
                break;
        }

        // status = performing maneuver
        response = rcEXECUTERESPONSE;
    }
    else if ((curMode == GunsEngageMode or
              curMode == MissileEngageMode or
              curMode == WVREngageMode or
              curMode == BVREngageMode) and pmytarget and (pmytarget->IsAirplane() or pmytarget->IsHelicopter()))
    {
        xdiff = self->XPos() - pmytarget->XPos();
        ydiff = self->YPos() - pmytarget->YPos();

        if (xdiff * xdiff + ydiff * ydiff > NM_TO_FT * NM_TO_FT)
        {
            if (PlayerOptions.BullseyeOn())
            {
                response = rcENGAGINGA;
            }
            else
            {
                response = rcENGAGINGB;
            }

            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
            edata[2] = 2 * (pmytarget->Type() - VU_LAST_ENTITY_TYPE);
            edata[3] = (short) SimToGrid(pmytarget->YPos());
            edata[4] = (short) SimToGrid(pmytarget->XPos());
            edata[5] = (short) pmytarget->ZPos();
        }
        else
        {
            pfrom = (AircraftClass*) vuDatabase->Find(msg->dataBlock.from);
            edata[0] = 2 * (pmytarget->Type() - VU_LAST_ENTITY_TYPE);

            xdiff = pmytarget->XPos() - pfrom->XPos();
            ydiff = pmytarget->YPos() - pfrom->YPos();

            angle = (float)atan2(ydiff, xdiff);
            angle = angle - pfrom->Yaw();
            navangle =  FloatToInt32(RTD * angle);

            if (navangle < 0)
            {
                navangle = 360 + navangle;
            }

            edata[1] = navangle / 30; // scale compass angle for radio eData

            if (edata[1] >= 12)
            {
                edata[1] = 0;
            }

            rz = pmytarget->ZPos() - pfrom->ZPos();

            if (rz < 300.0F and rz > -300.0F)   // check relative alt and select correct frag
            {
                edata[2] = 1;
            }
            else if (rz < -300.0F and rz > -1000.0F)
            {
                edata[2] = 2;
            }
            else if (rz < -1000.0F)
            {
                edata[2] = 3;
            }
            else
            {
                edata[2] = 0;
            }

            response = rcENGAGINGC;
        }
    }
    else if (pmytarget and (pmytarget->IsAirplane() or pmytarget->IsHelicopter()))
    {
        // and i am spiked
        response =  rcSPIKE;
    }
    // else if(have stuff on radar) {
    // response = rcPICTUREBRA;
    // }
    else
    {
        // status = clean, clear bitand naked
        random = 4 * (FloatToInt32((float) rand() / (float) RAND_MAX));

        edata[0] = flightIdx;
        edata[1] = random;

        response = rcGENERALRESPONSEC;
    }

    AiMakeRadioResponse(self, response, edata);
}

// ----------------------------------------------------
// AIWingClass::AiGiveDamageReport
// ----------------------------------------------------

#define DAMAGELIST 9

void DigitalBrain::AiGiveDamageReport(FalconWingmanMsg* msg)
{
    short edata[10];
    int lastFault = 0;
    int count = 0;
    int i;
    typedef struct
    {
        FaultClass::type_FSubSystem subSystem;
        int status;
    } DamageEntry;

    DamageEntry pFaultList[DAMAGELIST] = {{FaultClass::eng_fault, FALSE},
        {FaultClass::fcr_fault, FALSE},
        {FaultClass::flcs_fault, FALSE},
        {FaultClass::sms_fault, FALSE},
        {FaultClass::ins_fault, FALSE},
        {FaultClass::rwr_fault, FALSE},
        {FaultClass::tcn_fault, FALSE},
        {FaultClass::ufc_fault, FALSE},
        {FaultClass::amux_fault, FALSE}
    };

    edata[0] = self->GetCampaignObject()->GetComponentIndex(self); // Get my slot in the flight
    count = ((AircraftClass*) self)->mFaults->GetFFaultCount(); // Check how many faults are set

    if (count == 0)   // If is no damage, say a-okay
    {
        edata[1] = 3;
        AiMakeRadioResponse(self, rcGENERALRESPONSEC, edata);
        return;
    }

    i = count;

    while (lastFault < DAMAGELIST)
    {

        if (((AircraftClass*) self)->mFaults->GetFault(pFaultList[lastFault].subSystem) == TRUE)
        {

            pFaultList[lastFault].status = TRUE;

            if (pFaultList[lastFault].subSystem == FaultClass::eng_fault)   // Evaluate each system
            {
                edata[1] = 0;
                edata[2] = 4;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::fcr_fault)
            {
                edata[1] = 1;
                edata[2] = 1;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::flcs_fault)
            {
                edata[1] = 2;
                edata[2] = 2;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::sms_fault)
            {
                edata[1] = 3;
                edata[2] = 2;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::ins_fault)
            {
                edata[1] = 4;
                edata[2] = 1;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::rwr_fault)
            {
                edata[1] = 5;
                edata[2] = 2;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::tcn_fault)
            {
                edata[1] = 6;
                edata[2] = 1;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::ufc_fault)
            {
                edata[1] = 7;
                edata[2] = 2;
            }
            else if (pFaultList[lastFault].subSystem == FaultClass::amux_fault)
            {
                edata[1] = 8;
                edata[2] = 1;
            }

            if (i not_eq count)
            {
                edata[0] = -1;
            }

            AiMakeRadioResponse(self, rcDAMREPORT, edata);
            i--;
        }

        lastFault++;
    }

    edata[0] = -1;

    if (i > 0)
    {
        for (lastFault = 0; lastFault < DAMAGELIST; lastFault++, i--)
        {

            if (pFaultList[lastFault].status == FALSE)
            {

                if (pFaultList[lastFault].subSystem == FaultClass::eng_fault)   // Evaluate each system
                {
                    edata[1] = 0;
                    edata[2] = 4;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::fcr_fault)
                {
                    edata[1] = 1;
                    edata[2] = 1;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::flcs_fault)
                {
                    edata[1] = 2;
                    edata[2] = 2;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::sms_fault)
                {
                    edata[1] = 3;
                    edata[2] = 2;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::ins_fault)
                {
                    edata[1] = 4;
                    edata[2] = 1;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::rwr_fault)
                {
                    edata[1] = 5;
                    edata[2] = 2;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::tcn_fault)
                {
                    edata[1] = 6;
                    edata[2] = 1;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::ufc_fault)
                {
                    edata[1] = 7;
                    edata[2] = 2;
                }
                else if (pFaultList[lastFault].subSystem == FaultClass::amux_fault)
                {
                    edata[1] = 8;
                    edata[2] = 1;
                }
            }

            if (i > 0)
            {
                AiMakeRadioResponse(self, rcDAMREPORT, edata);
            }
        }
    }
}

// ----------------------------------------------------
// AIWingClass::AiGiveFuelStatus
// ----------------------------------------------------

void DigitalBrain::AiGiveFuelStatus(FalconWingmanMsg* msg)
{
    short edata[10];
    int response;

    edata[0] = self->GetCampaignObject()->GetComponentIndex(self);

    if (self->af->Fuel() <= self->bingoFuel)
    {
        response = rcGENERALRESPONSEC;
        edata[1] = 4;
    }
    else
    {
        response = rcFUELCHECKRSP;
        edata[1] = (FloatToInt32(self->af->Fuel() + self->af->ExternalFuel()));// / 1000;
    }

    AiMakeRadioResponse(self, response, edata);
}

// ----------------------------------------------------
// AIWingClass::AiGiveWeaponsStatus
// ----------------------------------------------------

void DigitalBrain::AiGiveWeaponsStatus(void)
{
    // what happens if i have multiple weapon types?
    short edata[10];
    int hp;
    int flightIdx;

    int hasRadar = 0;
    int hasHeat = 0;
    int hasBomb = 0;
    int hasHARM = 0;
    int hasAGM = 0;
    int hasLGB = 0;
    int hasRockets = 0;
    int hasWeapons = 0;
    int response;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    short e0 = ((FlightClass*)self->GetCampaignObject())->callsign_id;
    short e1 = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;


    // 2001-10-16 M.N. only one time the full callsign
    edata[0] = e0;
    edata[1] = e1;
    edata[2] = -1;
    AiMakeRadioResponse(self, rcEXECUTE, edata);


    // Do a search for Heaters and Radars
    SMSClass *sms = (SMSClass*) self->GetSMS();

    if (sms)
    {
        for (hp = 1; hp < sms->NumHardpoints(); hp++)
        {
            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtAim120)
                hasRadar += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtAim9)
                hasHeat += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtAgm88)
                hasHARM += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtAgm65)
                hasAGM += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtMk82)
                hasBomb += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtMk84)
                hasBomb += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtGBU)
                hasLGB += sms->hardPoint[hp]->weaponCount;

            if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtLAU and sms->hardPoint[hp]->GetWeaponClass() == wcRocketWpn)
                // hasRockets += sms->hardPoint[hp]->weaponCount;
                hasRockets++;

            //if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->GetWeaponType() == wtGuns)
            // hasGuns = 1;
        }
    }

    if (hasRadar > 24)
        hasRadar = 24;

    if (hasHARM > 24)
        hasHARM = 24;

    if (hasHeat > 24)
        hasHeat = 24;

    if (hasAGM > 24)
        hasAGM = 24;

    if (hasBomb > 24)
        hasBomb = 24;

    if (hasLGB > 24)
        hasLGB = 24;

    if (hasRockets > 24)
        hasRockets = 24;

    hasWeapons = hasRadar + hasHARM + hasHeat + hasAGM + hasBomb + hasLGB + hasRockets;

    if (hasAGM)
    {
        edata[0] = (short)hasAGM--;
        edata[1] = 242;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata); // Commsequence changed 01-11-15
    }

    if (hasHARM)
    {
        edata[0] = (short)hasHARM--;
        edata[1] = 245;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata);
    }

    if (hasLGB)
    {
        edata[0] = (short)hasLGB--;
        edata[1] = 838;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata);
    }

    if (hasBomb)
    {
        edata[0] = (short)hasBomb--;
        edata[1] = 914;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata);
    }

    if (hasRockets)
    {
        edata[0] = (short)hasRockets--;
        edata[1] = 887;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata);
    }

    if (hasRadar)
    {
        edata[0] = (short)hasRadar--;
        edata[1] = 231;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata);
    }

    if (hasHeat)
    {
        edata[0] = (short)hasHeat--;
        edata[1] = 233;
        AiMakeRadioResponse(self, rcWEAPONSCHECKRSP, edata);
    }

    if ( not hasWeapons)
    {
        edata[0] = -1;
        edata[1] = 0; // Winchester
        AiMakeRadioResponse(self, rcWEAPONSLOW, edata);
    }

    // Finally say fuel

    edata[0] = -1; // Turn off flight position

    if (self->af->Fuel() <= self->bingoFuel)
    {
        response = rcGENERALRESPONSEC;
        edata[1] = 4;
    }
    else
    {
        response = rcFUELCHECKRSP;
        edata[1] = (FloatToInt32(self->af->Fuel() + self->af->ExternalFuel()));// / 1000;
    }

    AiMakeRadioResponse(self, response, edata);

}





// ----------------------------------------------------
// DigitalBrain::AiGiveWeaponsStatus
// ----------------------------------------------------

void DigitalBrain::AiPromote(void)
{
    //MI for radio response
    short edata[10];
    int response;
    int flightIdx;

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (isWing > 0)
    {
        isWing --;

        if ( not isWing)
        {
            SetLead(TRUE);
        }
        else
        {
            SetLead(FALSE);
        }

        //MI for radio response
        response = rcEXECUTERESPONSE;
        edata[0] = flightIdx;
        edata[1] = 11;
        AiMakeRadioResponse(self, response, edata);
    }
}


// ----------------------------------------------------
// DigitalBrain::AiRaygun
// ----------------------------------------------------

void DigitalBrain::AiRaygun(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];

    // set a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (msg->dataBlock.newTarget == self->Id())
    {
        edata[0] = -1;
        edata[1] = flightIdx;
        AiMakeRadioResponse(self, rcBUDDYSPIKE, edata);
    }
    else
    {
        edata[0] = -1;
        edata[1] = CALLSIGN_NUM_OFFSET + flightIdx + 1;
        edata[2] = -1;
        edata[3] = -1;
        edata[4] = 1;
        AiMakeRadioResponse(self, rcUNABLE, edata);
    }
}


// ----------------------------------------------------
// DigitalBrain::AiBuddySpikeReact
// ----------------------------------------------------

void DigitalBrain::AiBuddySpikeReact(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];

    // set a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}



// ----------------------------------------------------
// DigitalBrain::AiSetRadarActive
// ----------------------------------------------------
void DigitalBrain::AiSetRadarActive(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];
    RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

    // Make sure the radar is on
    if (theRadar)
    {
        theRadar->SetEmitting(TRUE);
    }

    // set a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}

// ----------------------------------------------------
// DigitalBrain::AiSetRadarStby
// ----------------------------------------------------
void DigitalBrain::AiSetRadarStby(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];
    RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

    // Make sure the radar is off
    if (theRadar)
    {
        theRadar->SetEmitting(FALSE);
    }

    // clear a radar flag here
    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}

// ----------------------------------------------------
// DigitalBrain::AiRTB
// ----------------------------------------------------
void DigitalBrain::AiRTB(FalconWingmanMsg* msg)
{
    int flightIdx;
    short edata[10];
    WayPointClass* pWaypoint = self->waypoint;
    BOOL done = FALSE;

    mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
    mpActionFlags[AI_RTB] = TRUE;

    while ( not done)
    {
        if (pWaypoint)
        {
            if (pWaypoint->GetWPAction() == WP_LAND)
            {
                // RV - Biker - When RTB go to WP before home base
                self->curWaypoint = pWaypoint->GetPrevWP();
                done = TRUE;
            }
            else
            {
                pWaypoint = pWaypoint->GetNextWP();
            }
        }
        else
        {
            // unable
            done = TRUE;
        }
    }

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
    AiSplitFlight(msg->dataBlock.to, msg->dataBlock.from, flightIdx);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;

        if ( not IsSetATC(SaidRTB))
        {
            SetATCFlag(SaidRTB);
            AiMakeRadioResponse(self, rcIMADOT, edata);
        }

        AiCheckFormStrip();
    }
    else
    {
        AiCheckFormStrip();
        AiRespondShortCallSign(self);
    }
}

// ----------------------------------------------------
// DigitalBrain::AiCheckInPositionCall
// ----------------------------------------------------
void DigitalBrain::AiCheckInPositionCall(float trX, float trY, float trZ)
{
    short edata[10];
    float xdiff, ydiff, zdiff;
    int vehInFlight;
    int flightIdx;


    if (mInPositionFlag == FALSE)
    {
        // 2002-02-12 ADDED BY S.G. If the lead is climbing (or is the player since I can't tell what altitude he wants), be more relax about z
        float maxZDiff;

        if (g_bPitchLimiterForAI and flightLead and (flightLead->IsSetFlag(MOTION_OWNSHIP) or (((AircraftClass*)flightLead)->DBrain() and fabs(flightLead->ZPos() - ((AircraftClass*)flightLead)->DBrain()->trackZ) > 2000.0f)))
            maxZDiff = 2000.0f;
        else
            maxZDiff = 250.0f;

        // END OF ADDED SECTION 2002-02-12

        xdiff = trX - self->XPos();
        ydiff = trY - self->YPos();
        zdiff = trZ - self->ZPos();

        if ((xdiff * xdiff + ydiff * ydiff <  250.0F * 250.0F) and fabs(zdiff) < maxZDiff)  // 2002-02-12 MODIFIED BY S.G. It's "ydiff * ydiff" not "ydiff + ydiff" plus replaced 250.0f for maxZDiff
        {
            mInPositionFlag = TRUE;
            edata[0] = self->GetCampaignObject()->GetComponentIndex(self);
            AiMakeRadioResponse(self, rcINPOSITION, edata);

            vehInFlight = ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
            flightIdx = ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

            if (flightIdx == AiElementLead and vehInFlight == 4)
            {
                AiMakeCommandMsg((SimBaseClass*) self, FalconWingmanMsg::WMGlue, AiWingman, FalconNullId);
            }
        }
    }
}

// Check if we are in position, and set the flag as needed. Make NO radio calls
void DigitalBrain::AiCheckPosition(void)
{
    float xdiff, ydiff, zdiff;
    float trX, trY, trZ, rangeFactor;
    ACFormationData::PositionData *curPosition;
    AircraftClass* paircraft;
    int vehInFlight, flightIdx;

    if (flightLead and flightLead not_eq self)
    {
        // Get wingman slot position relative to the leader
        vehInFlight = ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
        flightIdx = ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

        if (flightIdx == AiFirstWing and vehInFlight == 2)
        {
            curPosition = &(acFormationData->twoposData[mFormation]); // The four ship #2 slot position is copied in to the 2 ship formation array.
            paircraft = (AircraftClass*) flightLead;
        }
        else if (flightIdx == AiSecondWing and mSplitFlight)
        {
            curPosition = &(acFormationData->twoposData[mFormation]);
            paircraft = (AircraftClass*)((FlightClass*)self->GetCampaignObject())->GetComponentEntity(AiElementLead);
        }
        else
        {
            curPosition = &(acFormationData->positionData[mFormation][flightIdx - 1]);
            paircraft = (AircraftClass*) flightLead;
        }

        rangeFactor = curPosition->range * (2.0F * mFormLateralSpaceFactor);

        // Get my leader's position
        ShiAssert(paircraft)

        if (paircraft)
        {
            trX = paircraft->XPos();
            trY = paircraft->YPos();
            trZ = paircraft->ZPos();

            // Calculate position relative to the leader
            trX += rangeFactor * (float)cos(curPosition->relAz * mFormSide + paircraft->af->sigma);
            trY += rangeFactor * (float)sin(curPosition->relAz * mFormSide + paircraft->af->sigma);
        }

        if (curPosition->relEl)
        {
            trZ += rangeFactor * (float)sin(-curPosition->relEl);
        }
        else
        {
            trZ += (flightIdx * -100.0F);
        }

        xdiff = trX - self->XPos();
        ydiff = trY - self->YPos();
        zdiff = trZ - self->ZPos();

        if ((xdiff * xdiff + ydiff + ydiff >  250.0F * 250.0F) or fabs(zdiff) < 250.0F)
        {
            mInPositionFlag = FALSE;
        }
        else
        {
            mInPositionFlag = TRUE;
        }
    }
}

// ----------------------------------------------------
// DigitalBrain::AiCheckFormStrip
// ----------------------------------------------------

void DigitalBrain::AiCheckFormStrip(void)
{
    short edata[10];

    if (mpActionFlags[AI_FOLLOW_FORMATION] == TRUE and mInPositionFlag)
    {
        mInPositionFlag = FALSE;
        edata[0] = self->GetCampaignObject()->GetComponentIndex(self);
        AiMakeRadioResponse(self, rcSTRIPING, edata);
    }
}



// ----------------------------------------------------
// DigitalBrain::AiGlueWing
// ----------------------------------------------------
void DigitalBrain::AiGlueWing(void)
{
    mSplitFlight = FALSE;
}


// ----------------------------------------------------
// DigitalBrain::AiSplitWing
// ----------------------------------------------------

void DigitalBrain::AiSplitWing(void)
{
    mSplitFlight = TRUE;
}


// ----------------------------------------------------
// DigitalBrain::AiDropStores
// ----------------------------------------------------

void DigitalBrain::AiDropStores(FalconWingmanMsg *msg)
{
    short edata[10];
    int flightIdx;

    self->Sms->EmergencyJettison();

    flightIdx = self->GetCampaignObject()->GetComponentIndex(self);

    if (AiIsFullResponse(flightIdx, msg->dataBlock.to))
    {
        edata[0] = flightIdx;
        edata[1] = 1;
        AiMakeRadioResponse(self, rcROGER, edata);
    }
    else
    {
        AiRespondShortCallSign(self);
    }
}
