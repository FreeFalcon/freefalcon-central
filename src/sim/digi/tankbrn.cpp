#include "stdhdr.h"
#include "tankbrn.h"
#include "simveh.h"
#include "unit.h"
#include "simdrive.h"
#include "object.h"
#include "falcmesg.h"
#include "MsgInc/TankerMsg.h"
#include "falcsess.h"
#include "Aircrft.h"
#include "Graphics/Include/drawbsp.h"
#include "classtbl.h"
#include "Graphics/Include/matrix.h"
#include "airframe.h"
#include "playerop.h"
#include "MsgInc/SimCampMsg.h"//me123
#include "Find.h"
#include "Flight.h"

#ifdef DAVE_DBG
float boomAzTest = 0.0F, boomElTest = -4.0F * DTR, boomExtTest = 0.0F;
extern int MoveBoom;
#endif

extern bool g_bLightsKC135;
extern int g_nShowDebugLabels;

#define FOLLOW_RATE (10.0F * DTR)
// FRB - Moved to tankbrn.h - static const float DrogueExt = 40.0f; // how far the basket goes

// Tpoint tmprack = {0.0F}; // FRB - testing

// 15NOV03 - FRB - Refueling position contants
// F-16 optimim tanking position
#define STDPOSX -39.63939795321F
#define STDPOSZ 25.2530815923F

// ***** 13DEC03 - FRB - Moved to <ac>.dat file
// 15NOV03 - FRB - KC-135 and KC-10 refuelingLocation data fudge  (all boom refuelers)
//#define RFXOFFSET -2.5F  // X refuelingLocation data fudge (+ -> forward)
//#define RFYOFFSET -1.5F  // Y refuelingLocation data fudge (+ -> right)
//#define RFZOFFSET -0.5F  // Z refuelingLocation data fudge (+ -> down)
// 15NOV03 - FRB - IL-78 and KC-130 refuelingLocation data fudge (all drogue refuelers)
//#define IL78POSX -1.5F // Adjustments to <ac>.dat data
//#define IL78POSY -0.5F
//#define IL78POSZ 1.8F
// end ***** 13DEC03 - FRB

#define IL78HACKX 25.0F // Hack when no a/c data is available (???) - 25' to a/c nose

#define DROGUE_SERVICE 10
#define BOOM_SERVICE 20

// end FRB

enum
{
    MOVE_FORWARD,
    MOVE_BACK,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_RIGHT,
    MOVE_LEFT
};

extern short gRackId_Single_Rack;
extern float SimLibLastMajorFrameTime;
extern bool g_bUseTankerTrack;
extern float g_fTankerTrackFactor;
extern float g_fTankerHeadsupDistance;
extern float g_fTankerBackupDistance;
extern float g_fHeadingStabilizeFactor;

TankerBrain::TankerBrain(AircraftClass *myPlatform, AirframeClass* myAf) : DigitalBrain(myPlatform, myAf)
{
    flags = 0;
    stype = 0;
    thirstyQ = new TailInsertList;
    thirstyQ->Register();
    waitQ = new HeadInsertList;
    waitQ->Register();
    curThirsty = NULL;
    tankingPtr = NULL;
    lastStabalize = 0;
    lastBoomCommand = 0;
    memset(&boom, 0, sizeof(boom));
    memset(&rack, 0, sizeof(rack));
    numBooms = 0;
    numDrogues = 0;
    DrogueExt = 40.0f;
    DROGUE = 0;
    BOOM = 0;
    ServiceType = BOOM_SERVICE;
    turnallow = false;
    HeadsUp = false;
    reachedFirstTrackpoint = false;

    // 2002-03-13 MN
    for (int i = 0; i < sizeof(TrackPoints) / sizeof(vector); i++)
    {
        TrackPoints[i].x = 0.0f;
        TrackPoints[i].y = 0.0f;
        TrackPoints[i].z = 0.0f;
    }
}

TankerBrain::~TankerBrain(void)
{
    if (tankingPtr)
    {
        tankingPtr->Release();
    }

    tankingPtr = NULL;
    curThirsty = NULL;

    thirstyQ->Unregister();
    delete thirstyQ;
    thirstyQ = NULL;
    waitQ->Unregister(); // another leak fixed JPO
    delete waitQ;
    waitQ = NULL;
    CleanupBoom();
}

void TankerBrain::CleanupBoom(void)
{
    int i;

    switch (type) // 29NOV03 - FRB - reworked to cleanup the drogues on boom tankers
    {
        case TNKR_KCBOOM:
            if (boom[BOOM].drawPointer)
            {
                ((DrawableBSP*)self->drawPointer)->DetachChild(boom[BOOM].drawPointer, 0);
                delete boom[BOOM].drawPointer;
                boom[BOOM].drawPointer = NULL;

                for (i = 1; i <= numDrogues; i++)
                {
                    if (boom[i].drawPointer)
                    {
                        ((DrawableBSP*)self->drawPointer)->DetachChild(boom[i].drawPointer, i);
                        delete boom[i].drawPointer;
                        boom[i].drawPointer = NULL;
                    }
                }
            }

            break;

        case TNKR_KCDROGUE:
            for (i = 1; i <= numDrogues; i++)
            {
                if (rack[i])
                {
                    if (boom[i].drawPointer)
                    {
                        rack[i]->DetachChild(boom[i].drawPointer, 0);
                        delete boom[i].drawPointer;
                        boom[i].drawPointer = NULL;
                    }

                    if (boom[i].drawPointer)
                        ((DrawableBSP*)self->drawPointer)->DetachChild(rack[i], i - 1);

                    delete rack[i];
                    rack[i] = NULL;
                }
                else
                {
                    if (boom[i].drawPointer)
                    {
                        ((DrawableBSP*)self->drawPointer)->DetachChild(boom[i].drawPointer, i - 1);
                        delete boom[i].drawPointer;
                        boom[i].drawPointer = NULL;
                    }
                }
            }

            break;
    }
}


// 18NOV03 - FRB - Added more boom-equipped and drogue-equipped tankers
// Note: Length of of unextended boom = 33.5' (constant used throughout DriveBoom() code)
void TankerBrain::InitBoom(void)
{
    unsigned long boomModel = 0;
    unsigned long drogueModel = 0;
    Tpoint rackLoc;
    Tpoint simLoc;

    // 26NOV03 - FRB - Variable values from <ac>.dat file

    self->af->GetDrogueRFPos(&DrogueRFPos);
    self->af->GetBoomRFPos(&BoomRFPos);
    DrogueExt = self->af->GetDrogueExt();

    numBooms = self->af->GetnBooms();

    if (numBooms > 1)
        numBooms = 1;

    numDrogues = self->af->GetnDrogues();

    if (numDrogues > 4)
        numDrogues = 4;

    DROGUE = self->af->GetActiveDrogue();

    if ( not numBooms)
        DROGUE ++; // move to correct boom[] index

    BOOM = 0;

    switch (((DrawableBSP*)self->drawPointer)->GetID())
    {
            type = TNKR_KCBOOM;
            ServiceType = BOOM_SERVICE;

        case VIS_KC10:
        case VIS_KC135:
        case VIS_TNKR_BOOM1: // 17NOV03 - FRB - Boom-equipped Parent #2200
        case VIS_TNKR_BOOM2: // 17NOV03 - FRB - Boom-equipped Parent #2201
        case VIS_TNKR_BOOM3: // 17NOV03 - FRB - Boom-equipped Parent #2202
        case VIS_TNKR_BOOM4: // 17NOV03 - FRB - Boom-equipped Parent #2203
        case VIS_TNKR_BOOM5: // 17NOV03 - FRB - Boom-equipped Parent #2204
        {
            if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_KC10)
            {
                stype = TNKR_KC10;
                boomModel = VIS_KC10BOOM;
                drogueModel = 0;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_KC135)
            {
                stype = TNKR_KC135;
                boomModel = VIS_KC135BOOM;
                drogueModel = 0;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_BOOM1)
            {
                // KC-10 with boom and drogue
                stype = TNKR_KC10;
                boomModel = VIS_KCBOOM1;
                drogueModel = VIS_KCDROGUE1;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_BOOM2)
            {
                // KC-135 with boom and drogue
                stype = TNKR_KC135;
                boomModel = VIS_KCBOOM2;
                drogueModel = VIS_KCDROGUE2;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_BOOM3)
            {
                stype = TNKR_UNKNOWN;
                boomModel = VIS_KCBOOM3;
                drogueModel = VIS_KCDROGUE3;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_BOOM4)
            {
                stype = TNKR_UNKNOWN;
                boomModel = VIS_KCBOOM4;
                drogueModel = VIS_KCDROGUE4;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_BOOM5)
            {
                stype = TNKR_UNKNOWN;
                boomModel = VIS_KCBOOM5;
                drogueModel = VIS_KCDROGUE5;
            }
            else
            {
                stype = TNKR_KC135;
                boomModel = VIS_KC135BOOM;
                drogueModel = 0;
            }

            if ( not DrogueExt)
                DrogueExt = 70.0f;

            if ( not numBooms)
                numBooms = 1;

            boom[BOOM].drawPointer = new DrawableBSP(MapVisId(boomModel), &simLoc, &IMatrix);
            ((DrawableBSP*)self->drawPointer)->AttachChild(boom[BOOM].drawPointer, 0);
            ((DrawableBSP*)self->drawPointer)->GetChildOffset(0, &simLoc);
            boom[BOOM].rx = simLoc.x;
            boom[BOOM].ry = simLoc.y;
            boom[BOOM].rz = simLoc.z;
            boom[BOOM].az = 0.0F;
            boom[BOOM].el = 0.0F;
            boom[BOOM].ext = 0.0F;

            if (numDrogues)
            {
                for (int i = 1; i <= numDrogues; i++) // Drogue pod model must have its own rack
                {
                    boom[i].drawPointer = new DrawableBSP(MapVisId(drogueModel), &simLoc, &IMatrix);
                    ((DrawableBSP*)self->drawPointer)->AttachChild(boom[i].drawPointer, i);
                    ((DrawableBSP*)self->drawPointer)->GetChildOffset(i, &simLoc);
                    boom[i].rx = simLoc.x;
                    boom[i].ry = simLoc.y;
                    boom[i].rz = simLoc.z;
                    boom[i].az = 0.0F;
                    boom[i].el = 0.0F;
                    boom[i].ext = 0.0F;
                }
            }

        } // end Boom-equipped a/c
        break;

        case VIS_TNKR_DROGUE1: // 17NOV03 - FRB - Drogue-equipped Parent #2205 (single drogue)
        case VIS_TNKR_DROGUE2: // 17NOV03 - FRB - Drogue-equipped Parent #2206 (single drogue)
        case VIS_TNKR_DROGUE3: // 17NOV03 - FRB - Drogue-equipped Parent #2207 (double drogue)
        case VIS_TNKR_DROGUE4: // 17NOV03 - FRB - Drogue-equipped Parent #2208 (double drogue)
        case VIS_TNKR_DROGUE5: // 17NOV03 - FRB - Drogue-equipped Parent #2209 (double drogue)
        case VIS_KC130: // (double drogue)
        case VIS_IL78: // (triple drogue)
        {
            type = TNKR_KCDROGUE;
            ServiceType = DROGUE_SERVICE;

            if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_IL78)
            {
                boomModel = VIS_RDROGUE;
                numDrogues = 3;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_KC130)
            {
                boomModel = VIS_RDROGUE;
                numDrogues = 2;
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_DROGUE1)
            {
                boomModel = VIS_RDROGUE1;

                if ( not numDrogues)
                    numDrogues = 1;

                // Drogue includes rack
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_DROGUE2)
            {
                boomModel = VIS_RDROGUE2;

                if ( not numDrogues)
                    numDrogues = 1;

                // Drogue includes rack
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_DROGUE3)
            {
                boomModel = VIS_RDROGUE3;

                if ( not numDrogues)
                    numDrogues = 2;

                // Drogue includes rack
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_DROGUE4)
            {
                boomModel = VIS_RDROGUE4;

                if ( not numDrogues)
                    numDrogues = 2;

                // Drogue includes rack
            }
            else if (((DrawableBSP*)self->drawPointer)->GetID() == VIS_TNKR_DROGUE5)
            {
                boomModel = VIS_RDROGUE5;

                if ( not numDrogues)
                    numDrogues = 2;

                // Drogue includes rack
            }
            else
            {
                boomModel = VIS_RDROGUE;

                if ( not numDrogues)
                    numDrogues = 1;

                boomModel = VIS_RDROGUE;
            }


            if ( not DrogueExt)
                DrogueExt = 40.0f;

            numBooms = 0;  // These a/c can't have booms

            if ( not DROGUE)
                DROGUE = 1;

            if (DROGUE > numDrogues)
                DROGUE = numDrogues;

            for (int i = 1; i <= numDrogues; i++)
            {
                if (boomModel == VIS_RDROGUE)
                {
                    rack[i] = new DrawableBSP(MapVisId(VIS_SINGLE_RACK), &simLoc, &IMatrix);
                    ((DrawableBSP*)self->drawPointer)->AttachChild(rack[i], i - 1);
                    ((DrawableBSP*)self->drawPointer)->GetChildOffset(i - 1, &rackLoc);
                    boom[i].drawPointer = new DrawableBSP(MapVisId(boomModel), &simLoc, &IMatrix);
                    rack[i]->AttachChild(boom[i].drawPointer, 0);
                    rack[i]->GetChildOffset(0, &simLoc);
                }
                else // New Drogue pod must include rack
                {
                    boom[i].drawPointer = new DrawableBSP(MapVisId(boomModel), &simLoc, &IMatrix);
                    ((DrawableBSP*)self->drawPointer)->AttachChild(boom[i].drawPointer, i - 1);
                    ((DrawableBSP*)self->drawPointer)->GetChildOffset(i - 1, &simLoc);
                }

                boom[i].rx = simLoc.x + rackLoc.x;
                boom[i].ry = simLoc.y + rackLoc.y;
                boom[i].rz = simLoc.z + rackLoc.z;
                boom[i].az = 0.0F;
                boom[i].el = 0.0F;
                boom[i].ext = 0.0F;

            }

        } // end drogue-equipped a/c
        break;

        default:
            type = TNKR_UNKNOWN;
            break;
    }
}
// end of FRB mod's

void TankerBrain::CallNext(void)
{
    AircraftClass* aircraft;
    FalconTankerMessage* tankMsg;
    int count = 1;

    VuListIterator thirstyWalker(thirstyQ);
    aircraft = (AircraftClass*)thirstyWalker.GetFirst();

    if (aircraft)
    {
        //me123 yea this is crap holdAlt = min ( max (-self->ZPos(), 15000.0F), 25000.0F);
        // holdAlt = -self->ZPos();//me123
        // 2002-02-17 holding current ZPos is crap -
        // A10 can never reach tanker cruising altitude - use defined altitude from airframe class auxaerodata
        holdAlt = aircraft->af->GetRefuelAltitude();
        //it's much easier if the tanker isn't trying to change altitude
        //holdAlt = 20000.0F;
        curThirsty = aircraft;
        flags or_eq IsRefueling;

        // Say precontact message
        tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
        tankMsg->dataBlock.caller = curThirsty->Id();
        tankMsg->dataBlock.type = FalconTankerMessage::PreContact;
        tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = 0;
        FalconSendMessage(tankMsg);

        aircraft = (AircraftClass*)thirstyWalker.GetNext();

        while (aircraft)
        {
            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = aircraft->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = (float)count;
            FalconSendMessage(tankMsg);

            count++;
            aircraft = (AircraftClass*)thirstyWalker.GetNext();
        }

        //me123
        if (vuLocalSessionEntity and 
            vuLocalSessionEntity->Game() and 
            self->OwnerId() not_eq curThirsty->OwnerId()
           )
        {
            // we are hostign a game
            VuGameEntity *game = vuLocalSessionEntity->Game();
            VuSessionsIterator Sessioniter(game);
            VuSessionEntity*   sess;
            sess = Sessioniter.GetFirst();
            int foundone = FALSE;

            while (sess and not foundone)
            {
                if (sess->GetCameraEntity(0) == curThirsty)
                {
                    foundone = TRUE;
                    break;
                }
                else
                {
                    sess = Sessioniter.GetNext();
                }
            }

            if (foundone)
            {
                FalconSimCampMessage *msg = new FalconSimCampMessage(self->GetCampaignObject()->Id(), FalconLocalGame);  // target);
                msg->dataBlock.from = sess->Id();
                msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
                FalconSendMessage(msg);
            }
            else
            {
                // must be an ai plane who's curthirsty so lets give the host the tanker
                FalconSimCampMessage *msg = new FalconSimCampMessage(self->GetCampaignObject()->Id(), FalconLocalGame);  // target);
                msg->dataBlock.from = curThirsty->OwnerId();
                msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
                FalconSendMessage(msg);
            }
        }
    }
}

void TankerBrain::DoneRefueling(void)
{
    /*
    if (tankingPtr)
    {
     tankingPtr->Release(  );
     //if (self->IsInstantAction())
     //InstantAction.SetNoAdd(FALSE);
    }

    tankingPtr = NULL;
    */
    holdAlt += 200.0F; // 15NOV03 - FRB - was 100.0F
    af->ClearFlag(AirframeClass::Refueling);
    flags or_eq ClearingPlane;
    //curThirsty = NULL;
    flags and_eq compl GivingGas;
    flags and_eq compl PatternDefined;
    //flags and_eq compl IsRefueling;

    //me123 transfere ownship back to host when we are done rf
    if (vuLocalSessionEntity and 
        vuLocalSessionEntity->Game() and 
        self->OwnerId() not_eq vuLocalSessionEntity->Game()->OwnerId())
    {
        FalconSimCampMessage *msg = new FalconSimCampMessage(self->GetCampaignObject()->Id(), FalconLocalGame);  // target);
        msg->dataBlock.from = vuLocalSessionEntity->Game()->OwnerId();
        msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
        FalconSendMessage(msg);
    }

    turnallow = false;
    HeadsUp = true;
    reachedFirstTrackpoint = false;
}



void TankerBrain::DriveBoom(void)
{
    float tmpAz, tmpRange, tempEl, rad;

    FalconTankerMessage *tankMsg;

    // 29NOV03 - FRB
    // Type of refueling required by this aircraft?
    if (curThirsty and numBooms >= 1 and numDrogues >= 1) // Tanker has both services?
        if (((AircraftClass*)curThirsty)->af)
            if (((AircraftClass*)curThirsty)->af->GetnDrogues() >= 1.0f)
                ServiceType = DROGUE_SERVICE;
            else
                ServiceType = BOOM_SERVICE;

    // end FRB

    if (ServiceType == DROGUE_SERVICE)
    {
        int range = 0;

        if (tankingPtr and tankingPtr->localData->range < 1500) // FRB - Changed 800' to 1500' to give more time to extend the drogue
        {
            range = static_cast<int>(-DrogueExt);

            // 07DEC03 - FRB - Turn on drogue lights
            if (boom[DROGUE].drawPointer)
                boom[DROGUE].drawPointer->SetSwitchMask(0, 1);
        }

        //     for (int i = 0; i < numDrogues; i++) { // only extend active one
        //    int i = DROGUE;
        if (boom[DROGUE].drawPointer)
        {
            if (boom[DROGUE].ext > range)
            {
                boom[DROGUE].ext -= 5.0F * SimLibLastMajorFrameTime;
            }
            else if (boom[DROGUE].ext < range)
            {
                boom[DROGUE].ext += 5.0F * SimLibLastMajorFrameTime;
            }

            boom[DROGUE].ext = max(min(boom[DROGUE].ext, 1.0F), -DrogueExt);     // 23NOV03 - FRB - changed 40 to DrogueExt
            boom[DROGUE].drawPointer->SetDOFangle(0, fabs(boom[DROGUE].ext / -DrogueExt)); // 04DEC03 - FRB - drogue extension uses Translation DOFs(TDOF)
            //   boom[DROGUE].drawPointer->SetDOFoffset(0, boom[DROGUE].ext);
        }

        //     }
        tmpRange = boom[DROGUE].ext;

        // FRB - No azimuth or elevation control of drogue
        boom[DROGUE].az = 0.0f;
        boom[DROGUE].el = 0.0f;


        tmpAz = 0;
        tempEl = 0;

        if ( not (flags bitand GivingGas) and ( not tankingPtr or tankingPtr->localData->range > 800.0F) or (flags bitand ClearingPlane))
        {
            tmpAz = 0.0F;
            tempEl = 0;
        }
        else if ( not (flags bitand GivingGas) and ( not tankingPtr or tankingPtr->localData->range - DrogueExt > 30.0F))
        {
            tmpAz = 0.0F;
            tempEl = 0;
        }
        else
        {
            tempEl = tankingPtr->localData->el;
            tmpAz = tankingPtr->localData->az;
        }

#if 0 // not done yet  // FRB - No azimuth or elevation control of drogue

        if (boom[DROGUE].az - tmpAz > FOLLOW_RATE * SimLibLastMajorFrameTime)
            boom[DROGUE].az = boom[DROGUE].az - FOLLOW_RATE * SimLibLastMajorFrameTime;
        else if (boom[DROGUE].az - tmpAz < -FOLLOW_RATE * SimLibLastMajorFrameTime)
            boom[DROGUE].az = boom[DROGUE].az + FOLLOW_RATE * SimLibLastMajorFrameTime;
        else
            boom[DROGUE].az = tmpAz;

        if (boom[DROGUE].el - tempEl > FOLLOW_RATE * SimLibLastMajorFrameTime)
            boom[DROGUE].el = boom[DROGUE].el - FOLLOW_RATE * SimLibLastMajorFrameTime;
        else if (boom[DROGUE].el - tempEl < -FOLLOW_RATE * SimLibLastMajorFrameTime)
            boom[DROGUE].el = boom[DROGUE].el + FOLLOW_RATE * SimLibLastMajorFrameTime;
        else
            boom[DROGUE].el = tempEl;

        // FRB - Max drogue hose extension is fixed at -DrogueExt feet
        if (boom[DROGUE].ext - tmpRange > 5.0F * SimLibLastMajorFrameTime)
            boom[DROGUE].ext = boom[DROGUE].ext - 5.0F * SimLibLastMajorFrameTime;
        else if (boom[DROGUE].ext - tmpRange < -5.0F * SimLibLastMajorFrameTime)
            boom[DROGUE].ext = boom[DROGUE].ext + 5.0F * SimLibLastMajorFrameTime;
        else
            boom[DROGUE].ext = tmpRange;

        tmpRange = boom[DROGUE].ext;

        //     Tpoint minB, maxB;
        if (curThirsty and curThirsty->drawPointer)
        {
            // ((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
            // tmpRange -= maxB.x;
            float rad = ((DrawableBSP*)curThirsty->drawPointer)->Radius(); // Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
            // tmpRange -= rad;
        }

        boom[DROGUE].az = max(min(boom[DROGUE].az,  4.0F * DTR), -4.0F * DTR);
        boom[DROGUE].el = max(min(boom[DROGUE].el, 10.0f * DTR), -10.0F * DTR);

        if (boom[DROGUE].drawPointer)
        {
            boom[DROGUE].drawPointer->SetDOFangle(1, -boom[DROGUE].az);
            boom[DROGUE].drawPointer->SetDOFangle(2, -boom[DROGUE].el);
        }

#endif

        int tmpRefuelMode;
        int ScaledRM = 1; // FRB

        // Is it the player here
        if (curThirsty == SimDriver.GetPlayerEntity())
            ScaledRM = tmpRefuelMode = PlayerOptions.GetRefuelingMode();
        // Nope, use 'very' easy refuelling
        else
        {
            tmpRefuelMode = 4;
            ScaledRM = 2;//sfr: why float?? 2.0f;  // FRB - Decrease AI hookup tolerance factor
        }

        if (tmpRefuelMode == 3)
        {
            ScaledRM = 2;//sfr: again 2.0f;  // FRB - Decrease Easy hookup tolerance factor
        }

        // 2002-03-08 MN HACK add in the drawpointer radius - this should fix DROGUE for each aircraft
        // Tested with SU-27, MiG-29, MiG-23, MiG-19, TU-16 and IL-28
        float totalrange = 999.9F;

        // 2002-03-09 MN changed to use aircraft datafile value - safer for fixing IL78 for each aircraft
        if (curThirsty and tankingPtr and curThirsty)
        {
            totalrange = tankingPtr->localData->range + tmpRange; // 23NOV03 - FRB
            //  totalrange = tankingPtr->localData->range;
            // 23NOV03 - FRB   totalrange = tmpRange + ((AircraftClass*)curThirsty)->af->GetIL78Factor() + tankingPtr->localData->range;
        }

        if (curThirsty and g_nShowDebugLabels bitand 0x4000)
        {
            char label[31];
            //  sprintf(label,"%5.1f %5.1f %5.1f",totalrange, tankingPtr->localData->range, boom[DROGUE].rx);
            sprintf(label, "%5.1f %5.1f %5.1f", (boom[DROGUE].ext / -DrogueExt), DrogueRFPos.x, boom[DROGUE].ext);
            ((DrawableBSP*)curThirsty->drawPointer)->SetLabel(label, ((DrawableBSP*)curThirsty->drawPointer)->LabelColor());
        }

        if (curThirsty and g_nShowDebugLabels bitand 0x40000)
        {
            char label[31];
            //  sprintf(label,"%5.1f %5.1f %5.1f",totalrange, tankingPtr->localData->range, boom[DROGUE].rx);
            sprintf(label, "%5.1f %5.1f %d", DrogueExt, DrogueRFPos.x, DROGUE);
            ((DrawableBSP*)curThirsty->drawPointer)->SetLabel(label, ((DrawableBSP*)curThirsty->drawPointer)->LabelColor());
        }

        if ( not (flags bitand (GivingGas bitor ClearingPlane)) and tankingPtr)
        {
            if ((fabs(totalrange) < 6.0F * /*FRB*/ ScaledRM and 
                 fabs(boom[DROGUE].az - tmpAz)*RTD < 6.0F * /*FRB*/ ScaledRM and 
                 fabs(boom[DROGUE].el - tankingPtr->localData->el)*RTD < 2.0F * /*FRB*/ ScaledRM and 
                 boom[DROGUE].el * RTD < 5.0F * /*FRB*/ ScaledRM and 
                 boom[DROGUE].el * RTD > -5.0F * /*FRB*/ ScaledRM and 
                 fabs(boom[DROGUE].az)*RTD < 20.0F * /*FRB*/ ScaledRM and 
                 fabs(tankingPtr->BaseData()->Roll())*RTD < 4.0F * /* S.G.*/ ScaledRM and 
                 fabs(tankingPtr->BaseData()->Pitch())*RTD < 6.0F * /* S.G.*/ ScaledRM)
                or ((flags bitand AIready) and (tmpRefuelMode >= 3))) // 27NOV03 - FRB  AI is in position
            {
                flags or_eq GivingGas;
                tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
                tankMsg->dataBlock.caller = curThirsty->Id();
                tankMsg->dataBlock.type = FalconTankerMessage::Contact;
                tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
                FalconSendMessage(tankMsg);
            }
        }
        else if (tankingPtr and not (flags bitand ClearingPlane))
        {
            tmpAz = tankingPtr->localData->az;

            if (fabs(totalrange) > 8.0F * /*FRB*/ ScaledRM or
                fabs(boom[DROGUE].az - tmpAz)*RTD > 7.0F * /*FRB*/ ScaledRM or
                fabs(boom[DROGUE].el - tankingPtr->localData->el)*RTD > 5.0F * /*FRB*/ ScaledRM or
                fabs(tankingPtr->BaseData()->Roll())*RTD > 5.0F * /* S.G.*/ ScaledRM or
                fabs(tankingPtr->BaseData()->Pitch())*RTD > 8.0F * /* S.G.*/ ScaledRM or
                tankingPtr->localData->el * RTD > 5.0F * /*FRB*/ ScaledRM or
                tankingPtr->localData->el * RTD < -5.0F * /*FRB*/ ScaledRM or
                fabs(tankingPtr->localData->az)*RTD > 23.0F * /*FRB*/ ScaledRM)
            {
                flags and_eq compl GivingGas;
                flags and_eq compl AIready;
                tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
                tankMsg->dataBlock.caller = curThirsty->Id();
                tankMsg->dataBlock.type = FalconTankerMessage::Disconnect;
                tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
                FalconSendMessage(tankMsg);
            }
        }
        else if ((flags bitand ClearingPlane) and tankingPtr and 
                 tankingPtr->localData->range > 0.04F * NM_TO_FT)
        {
            VuEntity *entity = NULL;
            {
                // sfr: @todo is this correct? looks damn weird to me
                VuListIterator myit(thirstyQ);

                if (entity = myit.GetFirst())
                {
                    // MLR 5/13/2004 - Just in case
                    entity = myit.GetNext();
                }
            }

            if (entity and ((SimBaseClass*)entity)->GetCampaignObject() == curThirsty->GetCampaignObject())
            {
                AddToWaitQ(curThirsty);
            }
            else
            {
                PurgeWaitQ();
                FalconTankerMessage *tankMsg;

                tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
                tankMsg->dataBlock.caller = curThirsty->Id();
                tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
                tankMsg->dataBlock.data1 = -1;
                FalconSendMessage(tankMsg);
            }

            RemoveFromQ(curThirsty);
            tankingPtr->Release();
            tankingPtr = NULL;
            flags and_eq compl ClearingPlane;
            flags and_eq compl IsRefueling;
            flags and_eq compl AIready;
        }
        else
        {
            if (tankingPtr)
                tankingPtr->Release();

            tankingPtr = NULL;
            //flags and_eq compl ClearingPlane;
            //flags and_eq compl IsRefueling;
        }

        return;
    } // end Drogue service


    //====================================================
    // Boom service
    if ( not boom[BOOM].drawPointer)
        return;

    // 28NOV03 - FRB - Get nose location to replace F-16 constant
    if (curThirsty and curThirsty->drawPointer)
    {
        rad = ((DrawableBSP*)curThirsty->drawPointer)->Radius(); // Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
    }

    tmpRange = 0.0F;

    if ( not (flags bitand GivingGas) and ( not tankingPtr or tankingPtr->localData->range > 800.0F) or (flags bitand ClearingPlane))
    {
        tmpAz = 0.0F;
        tempEl =  self->af->GetBoomStoredAngle() * DTR; // 12DEC03 - FRB - Use tanker <ac>.dat stored angle
        tmpRange = 0.0F;
    }
    else if ( not (flags bitand GivingGas) and ( not tankingPtr or tankingPtr->localData->range - 33.5F > rad)) // 28NOV03 - FRB - replaced 30.0F w/ rad
    {
        tmpAz = 0.0F;
        tempEl = -15.0F * DTR;
        tmpRange = 6.0F;
    }
    else
    {
        //lower and swivel boom first, then extend
        if (tankingPtr->localData->el < -22.0F * DTR)
            tempEl = tankingPtr->localData->el;
        else
            tempEl = -15.0F;

        // 28NOV03 - FRB -
        if (fabs(tankingPtr->localData->az) > 15.0F * DTR)
            tmpAz = 0.0f;
        else
            tmpAz = tankingPtr->localData->az;

        if (boom[BOOM].el * RTD > -27.2F and not (flags bitand GivingGas))
            tmpRange = 6.0F;
        else
            tmpRange = tankingPtr->localData->range - 33.5F;
    }

    if (boom[BOOM].az - tmpAz > FOLLOW_RATE * SimLibLastMajorFrameTime)
        boom[BOOM].az = boom[BOOM].az - FOLLOW_RATE * SimLibLastMajorFrameTime;
    else if (boom[BOOM].az - tmpAz < -FOLLOW_RATE * SimLibLastMajorFrameTime)
        boom[BOOM].az = boom[BOOM].az + FOLLOW_RATE * SimLibLastMajorFrameTime;
    else
        boom[BOOM].az = tmpAz;

    if (boom[BOOM].el - tempEl > FOLLOW_RATE * SimLibLastMajorFrameTime)
        boom[BOOM].el = boom[BOOM].el - FOLLOW_RATE * SimLibLastMajorFrameTime;
    else if (boom[BOOM].el - tempEl < -FOLLOW_RATE * SimLibLastMajorFrameTime)
        boom[BOOM].el = boom[BOOM].el + FOLLOW_RATE * SimLibLastMajorFrameTime;
    else
        boom[BOOM].el = tempEl;

    if (boom[BOOM].ext - tmpRange > 5.0F * SimLibLastMajorFrameTime)
        boom[BOOM].ext = boom[BOOM].ext - 5.0F * SimLibLastMajorFrameTime;
    else if (boom[BOOM].ext - tmpRange < -5.0F * SimLibLastMajorFrameTime)
        boom[BOOM].ext = boom[BOOM].ext + 5.0F * SimLibLastMajorFrameTime;
    else
        boom[BOOM].ext = tmpRange;


    boom[BOOM].az = max(min(boom[BOOM].az,  23.0F * DTR), -23.0F * DTR);
    boom[BOOM].el = max(min(boom[BOOM].el, self->af->GetBoomStoredAngle() * DTR), -40.0F * DTR);     // <== 12DEC03 - FRB use tanker boom stored angle
    boom[BOOM].ext = max(min(boom[BOOM].ext, 27.0F), 1.0F);

    /*
    Tpoint boomtip, receptor, recWPos;
    BoomTipPosition(&boomtip);

    receptor.x = 0.0F;
    receptor.y = 0.0F;
    receptor.z = -3.0F;

    MatrixMult( &((DrawableBSP*)((SimVehicleClass*)tankingPtr->BaseData())->drawPointer)->orientation, &receptor, &recWPos );
    recWPos.x += tankingPtr->BaseData()->XPos();
    recWPos.y += tankingPtr->BaseData()->YPos();
    recWPos.z += tankingPtr->BaseData()->ZPos();
    */

    boom[BOOM].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boom[BOOM].az);
    boom[BOOM].drawPointer->SetDOFangle(BOOM_ELEVATION, -boom[BOOM].el);
    boom[BOOM].drawPointer->SetDOFoffset(BOOM_EXTENSION, boom[BOOM].ext);

    // OW - sylvains refuelling fix
    // S.G. ADDED SO AI PILOTS HAVE AN EASIER JOB AT REFUELLING THAN THE PLAYERS
    float tmpRefuelMode;
    float ScaledRM = 1.0f; // FRB

    // Is it the player here
    if (curThirsty == SimDriver.GetPlayerEntity())
        ScaledRM = tmpRefuelMode = static_cast<float>(PlayerOptions.GetRefuelingMode());
    // Nope, use 'very' easy refuelling
    else
    {
        tmpRefuelMode = 4.0f;
        ScaledRM = 2.0f;  // FRB - Decrease AI hookup tolerance factor
    }

    if (tmpRefuelMode == 3.0f)
        ScaledRM = 2.0f;  // FRB - Decrease Easy hookup tolerance factor

    // THERE WILL BE FOUR USE OF tmpRefuelMode IN THE NEXT if/else if/else statement. THESE USED TO BE PlayerOptions.GetRefuelingMode()
    // END OF ADDED SECTION

    if (curThirsty and g_nShowDebugLabels bitand 0x4000)
    {
        char label[31];
        //  sprintf(label,"tot*%5.1f tmp*%5.1f R*%5.1f",totalrange, tmpRange, tankingPtr->localData->range);
        sprintf(label, "%5.1f %5.1f %5.1f", BoomRFPos.x, BoomRFPos.y, BoomRFPos.z);
        ((DrawableBSP*)curThirsty->drawPointer)->SetLabel(label, ((DrawableBSP*)curThirsty->drawPointer)->LabelColor());
    }


    if ( not (flags bitand (GivingGas bitor ClearingPlane)) and tankingPtr)
    {
        if ((fabs(boom[BOOM].ext - tankingPtr->localData->range + 33.5F) < 1.0F * /*FRB*/ ScaledRM and 
             fabs(boom[BOOM].az - tmpAz)*RTD < 1.0F * /*FRB*/ ScaledRM and 
             fabs(boom[BOOM].el - tankingPtr->localData->el)*RTD < 1.0F * /*FRB*/ ScaledRM and 
             boom[BOOM].el * RTD < -26.1F and boom[BOOM].el * RTD > -38.9F and fabs(boom[BOOM].az)*RTD < 20.0F /* and 
 fabs(tankingPtr->BaseData()->Roll())*RTD < 4.0F * /* S.G.*/ // ScaledRM and 
             // fabs(tankingPtr->BaseData()->Pitch())*RTD < 4.0F * /* S.G.*/ ScaledRM) */  JPG 14 Jan03 - End of Line 881 thru 883 - pitch and roll isn't needed
             or ((flags bitand AIready) and (tmpRefuelMode >= 3.0f)))) // 27NOV03 - FRB  AI is in position
        {
            flags or_eq GivingGas;
            flags and_eq compl AIready;
            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = curThirsty->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::Contact;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
            FalconSendMessage(tankMsg);
        }
    }
    else if (tankingPtr and not (flags bitand ClearingPlane))
    {
        if (fabs(boom[BOOM].ext - tankingPtr->localData->range + 33.5F) > 2.0F or
            fabs(boom[BOOM].az - tmpAz)*RTD > 2.0F * /*FRB*/ ScaledRM or
            fabs(boom[BOOM].el - tankingPtr->localData->el)*RTD > 2.0F or
            fabs(tankingPtr->BaseData()->Roll())*RTD > 20.0F * /* S.G.*/ ScaledRM or  // JPG 14 Jan 03 - 20 prevents tons of disconnects in the turn
            fabs(tankingPtr->BaseData()->Pitch())*RTD > 20.0F * /* S.G.*/ ScaledRM or
            tankingPtr->localData->el * RTD > -25.0F or
            tankingPtr->localData->el * RTD < -40.0F or
            fabs(tankingPtr->localData->az)*RTD > 23.0F)
        {
            flags and_eq compl GivingGas;
            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = curThirsty->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::Disconnect;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
            FalconSendMessage(tankMsg);
        }
    }
    else if ((flags bitand ClearingPlane) and tankingPtr and tankingPtr->localData->range > 0.04F * NM_TO_FT)
    {
        VuEntity *entity = NULL;
        {
            VuListIterator myit(thirstyQ);

            if (entity = myit.GetFirst()) // MLR 5/13/2004 - Just in case
                entity = myit.GetNext();
        }

        if (entity and ((SimBaseClass*)entity)->GetCampaignObject() == curThirsty->GetCampaignObject())
        {
            AddToWaitQ(curThirsty);
        }
        else
        {
            PurgeWaitQ();
            FalconTankerMessage *tankMsg;

            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = curThirsty->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
            tankMsg->dataBlock.data1 = -1;
            FalconSendMessage(tankMsg);
        }

        RemoveFromQ(curThirsty);
        tankingPtr->Release();
        tankingPtr = NULL;
        flags and_eq compl AIready;
        flags and_eq compl ClearingPlane;
        flags and_eq compl IsRefueling;
    }
    else
    {
        if (tankingPtr)
            tankingPtr->Release();

        tankingPtr = NULL;
        //flags and_eq compl ClearingPlane;
        //flags and_eq compl IsRefueling;
    }
}

void TankerBrain::DriveLights(void)
{

    int lightVal;

    // possible CTD fix
    if ( not self->drawPointer)
        return;

    if (tankingPtr)
    {
        //float elOff = -tankingPtr->localData->el * RTD;
        //float rangeOff = tankingPtr->localData->range - 33.5F;

        // Up/Down light
        // Turn on the Labels
        lightVal = (1 << 6) + 1;

        Tpoint relPos;
        ReceptorRelPosition(&relPos, curThirsty);

        // 2002-03-06 MN calculate relative position based on aircrafts refuel position
        Tpoint boompos;
        boompos.x = boompos.z = 0;

        if (curThirsty and curThirsty->IsAirplane() and ((AircraftClass*)curThirsty)->af)
        {
            ((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);

            // 15NOV03 - FRB - Adjust F-16 rf boom position to match current a/c rf port location
            // if nothing set, use F-16 default
            if (boompos.x == 0 and boompos.y == 0 and boompos.z == 0)
            {
                boompos.x = STDPOSX;
                boompos.y = 0.0f;
                boompos.z = STDPOSZ;
            }
            else
            {
                boompos.x = -boompos.x + STDPOSX;
                boompos.y = -boompos.y;

                if (boompos.z)
                    boompos.z = -(boompos.z + 3.0F) + STDPOSZ;
                else
                    boompos.z = STDPOSZ;
            }
        }

        relPos.x -= boompos.x; // relPos.x += 39.63939795321F;
        relPos.z -= boompos.z; // relPos.z -= 25.2530815923F;
#if 0

        // Set the right arrow
        if (relPos.z < -25.0F)
            lightVal += (1 << 5);
        else if (relPos.z < -15.0F)
            lightVal += (3 << 4);
        else if (relPos.z < -8.0F)
            lightVal += (1 << 4);
        else if (relPos.z < -2.5F)
            lightVal += (3 << 3);
        else if (relPos.z < 2.5F)
            lightVal += (1 << 3);
        else if (relPos.z < 8.0F)
            lightVal += (3 << 2);
        else if (relPos.z < 15.0F)
            lightVal += (1 << 2);
        else if (relPos.z < 25.0F)
            lightVal += (3 << 1);
        else
            lightVal += (1 << 1);

#else // M.N. Turn around lights as IRL does it work this way ??

        if (relPos.z < -25.0F)
            lightVal += (3 << 1);
        else if (relPos.z < -15.0F)
            lightVal += (1 << 2);
        else if (relPos.z < -8.0F)
            lightVal += (3 << 2);
        else if (relPos.z < -2.5F)
            lightVal += (1 << 3);
        else if (relPos.z < 2.5F)
            lightVal += (3 << 3);
        else if (relPos.z < 8.0F)
            lightVal += (1 << 4);
        else if (relPos.z < 15.0F)
            lightVal += (3 << 4);
        else if (relPos.z < 25.0F)
            lightVal += (1 << 5);
        else
            lightVal += (1 << 1);

#endif

        /*
        if(tankingPtr->localData->az > 0.0F)
        {
         // Set the right arrow
         if (elOff < 26.1F)
         lightVal += (0x1 << 5);
         else if (elOff < 27.2F)
         lightVal += (0x11 << 4);
         else if (elOff < 28.4F)
         lightVal += (0x1 << 4);
         else if (elOff < 29.5F)
         lightVal += (0x11 << 3);
         else if (elOff < 35.5F)
         lightVal += (0x1 << 3);
         else if (elOff < 36.6F)
         lightVal += (0x11 << 2);
         else if (elOff < 37.8F)
         lightVal += (0x1 << 2);
         else if (elOff < 38.9F)
         lightVal += (0x11 << 1);
         else
         lightVal += (0x1 << 1);
        }
        else
        {
         lightVal += (0x1 << 1);
        }*/

        ((DrawableBSP*)self->drawPointer)->SetSwitchMask(0, lightVal);

        // Fore/Aft light
        // Turn on the Labels
        lightVal = (1 << 6) + 1;
#if 0

        // Set the right arrow
        if (relPos.x < -25.0F)
            lightVal += (1 << 5);
        else if (relPos.x < -15.0F)
            lightVal += (3 << 4);
        else if (relPos.x < -8.0F)
            lightVal += (1 << 4);
        else if (relPos.x < -2.5F)
            lightVal += (3 << 3);
        else if (relPos.x < 2.5F)
            lightVal += (1 << 3);
        else if (relPos.x < 8.0F)
            lightVal += (3 << 2);
        else if (relPos.x < 15.0F)
            lightVal += (1 << 2);
        else if (relPos.x < 25.0F)
            lightVal += (3 << 1);
        else
            lightVal += (1 << 1);

#else

        // Set the right arrow
        if (relPos.x < -25.0F)
            lightVal += (3 << 1);
        else if (relPos.x < -15.0F)
            lightVal += (1 << 2);
        else if (relPos.x < -8.0F)
            lightVal += (3 << 2);
        else if (relPos.x < -2.5F)
            lightVal += (1 << 3);
        else if (relPos.x < 2.5F)
            lightVal += (3 << 3);
        else if (relPos.x < 8.0F)
            lightVal += (1 << 4);
        else if (relPos.x < 15.0F)
            lightVal += (3 << 4);
        else if (relPos.x < 25.0F)
            lightVal += (1 << 5);
        else
            lightVal += (1 << 1);

#endif
        /*
        // Set the right arrow
        if (rangeOff > 18.5F)
         lightVal += (0x1 << 5);
        else if (rangeOff > 17.25F)
         lightVal += (0x11 << 4);
        else if (rangeOff > 16.0F)
         lightVal += (0x1 << 4);
        else if (rangeOff > 14.75F)
         lightVal += (0x11 << 3);
        else if (rangeOff > 12.25F)
         lightVal += (0x1 << 3);
        else if (rangeOff > 11.0F)
         lightVal += (0x11 << 2);
        else if (rangeOff > 9.75F)
         lightVal += (0x1 << 2);
        else if (rangeOff > 8.55F)
         lightVal += (0x11 << 1);
        else
         lightVal += (0x1 << 1);*/

        ((DrawableBSP*)self->drawPointer)->SetSwitchMask(1, lightVal);
    }
    else
    {
        ((DrawableBSP*)self->drawPointer)->SetSwitchMask(0, 0);
        ((DrawableBSP*)self->drawPointer)->SetSwitchMask(1, 0);
    }
}

void TankerBrain::BreakAway(void)
{
    FalconTankerMessage* tankMsg;

    tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
    tankMsg->dataBlock.caller = curThirsty->Id();
    tankMsg->dataBlock.type = FalconTankerMessage::Breakaway;
    tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
    FalconSendMessage(tankMsg);
    turnallow = false;
    HeadsUp = true;
    reachedFirstTrackpoint = false;
}

void TankerBrain::TurnTo(float newHeading)
{
    // M.N. 2001-11-28
    float dist, dx, dy;

    // Set up new trackpoint
    dist = 200.0F * NM_TO_FT;

    dx = self->XPos() + dist * sin(newHeading);
    dy = self->YPos() + dist * cos(newHeading);

    SetTrackPoint(dx, dy);
}

void TankerBrain::TurnToTrackPoint(int trackPoint)
{
    // Set up new trackpoint
    SetTrackPoint(TrackPoints[trackPoint].x, TrackPoints[trackPoint].y);
}


void TankerBrain::FollowThirsty(void)
{
    float xyRange, dist, dx, dy, heading;
    float oldAz, oldEl, oldRange;
    FalconTankerMessage* tankMsg;

    // Find the thirsty one
    if ( not tankingPtr or tankingPtr->BaseData() not_eq curThirsty)
    {
        if (tankingPtr)
            tankingPtr->Release();

#ifdef DEBUG
        //tankingPtr = new SimObjectType( OBJ_TAG, self, curThirsty );
#else
        tankingPtr = new SimObjectType(curThirsty);
#endif
        tankingPtr->Reference();
        dist = DistanceToFront(SimToGrid(self->YPos()), SimToGrid(self->XPos()));

        if ( not g_bUseTankerTrack and dist > 60.0F and dist < 100.0F)
        {
            turnallow = true; // allow a new turn
            HeadsUp = true;
        }
    }

    if (tankingPtr)
    {
        float inverseTimeDelta = 1.0F / SimLibLastMajorFrameTime;

        Tpoint relPos;
        ReceptorRelPosition(&relPos, curThirsty);

        oldRange = tankingPtr->localData->range;
        tankingPtr->localData->range = (float)sqrt(relPos.x * relPos.x + relPos.y * relPos.y + relPos.z * relPos.z);
        tankingPtr->localData->rangedot = (tankingPtr->localData->range - oldRange) * inverseTimeDelta;
        tankingPtr->localData->range = max(tankingPtr->localData->range, 0.01F);
        xyRange = (float)sqrt(relPos.x * relPos.x + relPos.y * relPos.y);

        /*-------*/
        /* az    */
        /*-------*/
        oldAz = tankingPtr->localData->az;
        tankingPtr->localData->az = (float)atan2(relPos.y, -relPos.x);
        tankingPtr->localData->azFromdot = (tankingPtr->localData->az - oldAz) * inverseTimeDelta;

        /*-------*/
        /* elev  */
        /*-------*/
        oldEl = tankingPtr->localData->el;

        if (xyRange not_eq 0.0)
            tankingPtr->localData->el = (float)atan(-relPos.z / xyRange);
        else
            tankingPtr->localData->el = (relPos.z < 0.0F ? -90.0F * DTR : 90.0F * DTR);

        tankingPtr->localData->elFromdot = (tankingPtr->localData->el - oldEl) * inverseTimeDelta;

        trackZ = -holdAlt;

        if (flags bitand ClearingPlane)
            // desSpeed = af->CalcTASfromCAS(335.0F)*KNOTS_TO_FTPSEC; //JPG 24 Apr 04 - Removed conversions
            desSpeed = af->CalcTASfromCAS(((AircraftClass*)curThirsty)->af->GetRefuelSpeed()) * 1.2f; //*KNOTS_TO_FTPSEC; // 12DEC03 - FRB
        else  // use refuel speed for this aircraft
            desSpeed = af->CalcTASfromCAS(((AircraftClass*)curThirsty)->af->GetRefuelSpeed());//*KNOTS_TO_FTPSEC;

        // 2002-03-13 MN box tanker track
        if (g_bUseTankerTrack)
        {
            /*
             1) Check distance to our current Trackpoint
             2) If distance is < g_fTankerHeadsupDistance, make Headsup call
             3) If distance is < g_fTankerTrackFactor or distance to current trackpoint increases again, head towards the next trackPoint

             If advancedirection is set true, then tanker was outside his max range envelope when
             called for refueling. In this case when switching from Trackpoint 0 to Trackpoint 1,
             tanker would do a 180° turn - so just reverse the order from 0->1->2->3 to 0->3->2->1
            */
            dist = DistSqu(self->XPos(), self->YPos(), TrackPoints[currentTP].x, TrackPoints[currentTP].y);

            // trackPointDistance is always the closest distance to current trackpoint when close to it.
            // If tanker doesn't get "the curve" to catch the trackpoint at g_fTankerTrackFactor distance,
            // which means dist > trackPointDistance again, do the turn to next trackpoint nevertheless
            // (sort of backup function to keep the tanker turning in a track...)
            if (dist < (g_fTankerBackupDistance) * NM_TO_FT * (g_fTankerBackupDistance) * NM_TO_FT)
            {
                if (dist < trackPointDistance)
                    trackPointDistance = dist;
            }
            else
                trackPointDistance = (0.5f + g_fTankerBackupDistance) * NM_TO_FT * (0.5f + g_fTankerBackupDistance) * NM_TO_FT;

            if ((dist < (g_fTankerHeadsupDistance) * NM_TO_FT * (g_fTankerHeadsupDistance) * NM_TO_FT) and HeadsUp)
            {
                reachedFirstTrackpoint = true; // from now on limit rStick, pStick and flight model (afsimple.cpp) until refueling is done
                HeadsUp = false;
                turnallow = true;
                tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
                tankMsg->dataBlock.caller = curThirsty->Id();
                tankMsg->dataBlock.type = FalconTankerMessage::TankerTurn;
                tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
                FalconSendMessage(tankMsg);
            }

            // distance to trackpoint increases again or we are closer than g_fTankerTrackFactor nm ? -> switch to next trackpoint
            if ((dist > trackPointDistance or (dist < (g_fTankerTrackFactor) * NM_TO_FT * (g_fTankerTrackFactor) * NM_TO_FT)) and turnallow)
            {
                HeadsUp = true;
                turnallow = false;

                if (advancedirection)
                    currentTP--;
                else
                    currentTP++;

                if (currentTP > 3)
                    currentTP = 0;

                if (currentTP < 0)
                    currentTP = 3;

                TurnToTrackPoint(currentTP);
            }
        }
        else
        {
            dist = DistanceToFront(SimToGrid(self->YPos()), SimToGrid(self->XPos()));

            if (dist > 45.0F and dist < 140.0F or dist > 200.0F)
            {
                HeadsUp = true; // and a HeadsUp call
            }

            if ((dist <= 36.0F or dist >= 149.0F) and HeadsUp)
            {
                HeadsUp = false;
                turnallow = true;
                tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
                tankMsg->dataBlock.caller = curThirsty->Id();
                tankMsg->dataBlock.type = FalconTankerMessage::TankerTurn;
                tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
                FalconSendMessage(tankMsg);
            }

            if (dist < 35.0F or dist > 150.0F) // refuel between 35 and 150 nm distance to the FLOT
            {
                if (turnallow)
                {
                    turnallow = false;
                    dx = trackX - self->XPos();
                    dy = trackY - self->YPos();
                    heading = (float)atan2(dy, dx);

                    if (heading < 0.0F)
                        heading += PI * 2.0F;

                    heading += PI; // add 180°

                    if (heading > PI * 2.0F)
                        heading -= PI * 2.0F;

                    TurnTo(heading); // set up new trackpoint and make a radio call announcing the turn
                }
            }
        }

        SimpleTrack(SimpleTrackTanker, desSpeed);

        if (g_bUseTankerTrack)
        {
            dy = TrackPoints[currentTP].y - self->YPos();
            dx = TrackPoints[currentTP].x - self->XPos();

            heading = (float)atan2(dy, dx);

            if (fabs(self->Yaw() - heading) < g_fHeadingStabilizeFactor) // when our course is close to trackpoint's direction
                rStick = 0.0F;   // stabilize Tanker's heading
        }

        if (g_nShowDebugLabels bitand 0x800)
        {
            char tmpchr[32];
            dist = (float)sqrt(dist);
            dist *= FT_TO_NM;
            float yaw = self->Yaw();

            if (yaw < 0.0f)
                yaw += 360.0F * DTR;

            yaw *= RTD;

            ReceptorRelPosition(&relPos, curThirsty);

            sprintf(tmpchr, "%3.2f %5.1f   %3.0f %3.2f TP %d", relPos.x, self->XPos(), yaw, dist, currentTP);

            if (self->drawPointer)
                ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
        }

        // Set the lights
        if (stype == TNKR_KC10)
            DriveLights();
        else if (stype == TNKR_KC135 and g_bLightsKC135) // when we have the lights on the KC-135 model
            DriveLights();

        if (xyRange < 500.0F and not (flags bitand PrecontactPos) and 
            fabs(tankingPtr->localData->rangedot) < 100.0F and 
            fabs(tankingPtr->localData->az) < 35.0F * DTR)
        {
            flags or_eq PrecontactPos;
            // Call into contact position
            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = curThirsty->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::ClearToContact;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
            FalconSendMessage(tankMsg);
        }

        if ( not (flags bitand ClearingPlane))
            // 25NOV03 - FRB - Give directions to drogue-refueling a/c
            // if(ServiceType not_eq DROGUE_SERVICE and not (flags bitand ClearingPlane))
        {
            if (xyRange < 200.0F and not (flags bitand GivingGas) and 
                (SimLibElapsedTime - lastBoomCommand) > 10000)
            {
                lastBoomCommand = SimLibElapsedTime;
                // Call boom commands
                tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
                tankMsg->dataBlock.caller = curThirsty->Id();
                tankMsg->dataBlock.type = FalconTankerMessage::BoomCommand;

                // 2002-03-06 MN calculate relative position based on aircrafts refuel position
                Tpoint boompos;
                boompos.x = boompos.z = 0;

                if (curThirsty and curThirsty->IsAirplane() and ((AircraftClass*)curThirsty)->af)
                {
                    ((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);

                    // 15NOV03 - FRB - Adjust F-16 rf boom position to match current a/c rf port location
                    if (ServiceType == BOOM_SERVICE)
                    {
                        // if nothing set, use F-16 default
                        if (boompos.x == 0 and boompos.y == 0 and boompos.z == 0)
                        {
                            boompos.x = STDPOSX;
                            boompos.y = 0.0f;
                            boompos.z = STDPOSZ;
                        }
                        else
                        {
                            boompos.x = -boompos.x + STDPOSX;
                            boompos.y = -boompos.y;

                            if (boompos.z)
                                boompos.z = -(boompos.z + 3.0F) + STDPOSZ;
                            else
                                boompos.z = STDPOSZ;
                        }
                    }
                    else
                    {
                        // 18NOV03 - FRB - Added direction commands to drogue tanker
                        // 15NOV03 - FRB - Adjust droque rf position to match current a/c rf port location
                        // if nothing set, use nose of a/c
                        if (boompos.x == 0 and boompos.y == 0 and boompos.z == 0)
                        {
                            float rad;

                            if (curThirsty and curThirsty->drawPointer)
                            {
                                rad = ((DrawableBSP*)curThirsty->drawPointer)->Radius(); // Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
                                boompos.x = -rad - DrogueExt;
                                boompos.y = 0.0F;
                                boompos.z = 0.0F;
                            }
                            else // no model data available at this time, so fake it.
                            {
                                boompos.x = IL78HACKX - DrogueExt;  // guessimate
                                boompos.y = 0.0F;
                                boompos.z = 0.0F;
                            }
                        }
                        else // use refueling position data
                        {
                            boompos.x = -boompos.x + DrogueRFPos.x;
                            boompos.y = -boompos.y + DrogueRFPos.y;
                            boompos.z = -boompos.z + DrogueRFPos.z;
                        }
                    }
                }

                relPos.x -= boompos.x; // relPos.x += 39.63939795321F;
                relPos.z -= boompos.z; // relPos.z -= 25.2530815923F;

                // pos rx is towards the front of the tanker
                // pos ry is to the right of the tanker
                // pos rz is to the bottom of the tanker
                if (fabs(relPos.x) > fabs(relPos.y) and fabs(relPos.x) > fabs(relPos.z))
                {
                    if (relPos.x < 0.0f)
                        tankMsg->dataBlock.data1 = MOVE_FORWARD;
                    else
                        tankMsg->dataBlock.data1 = MOVE_BACK;
                }
                else if (fabs(relPos.z) > fabs(relPos.y))
                {
                    if (relPos.z < 0.0f)
                        tankMsg->dataBlock.data1 = MOVE_DOWN;
                    else
                        tankMsg->dataBlock.data1 = MOVE_UP;
                }
                else
                {
                    if (relPos.y > 0.0f)
                        tankMsg->dataBlock.data1 = MOVE_LEFT;
                    else
                        tankMsg->dataBlock.data1 = MOVE_RIGHT;
                }

                tankMsg->dataBlock.data2 = 0;
                FalconSendMessage(tankMsg);
            }
        }

        // Too Eratic?
        if ( not (flags bitand ClearingPlane) and (flags bitand GivingGas) and (SimLibElapsedTime - lastStabalize) > 15000 and 
            fabs(tankingPtr->localData->azFromdot) > 10.0F * DTR and fabs(tankingPtr->localData->elFromdot) > 10.0F * DTR)
        {
            lastStabalize = SimLibElapsedTime;
            // Call stablize command
            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = curThirsty->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::Stabalize;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
            FalconSendMessage(tankMsg);
        }
    }
    else
    {
        DoneRefueling();
    }

    // Stick it
    DriveBoom();
}

int TankerBrain::TankingPosition(SimVehicleClass* thirstyOne)
{
    VuEntity *entity = NULL;
    int count = 0;

    VuListIterator myit(thirstyQ);
    entity = myit.GetFirst();

    while (entity)
    {
        if (entity == thirstyOne)
        {
            return count;
        }

        count++;
        entity = myit.GetNext();
    }

    return -1;
}

int TankerBrain::AddToQ(SimVehicleClass* thirstyOne)
{
    VuEntity *entity = NULL;
    int count = 0;

    if (thirstyQ)
    {
        {
            VuListIterator myit(thirstyQ);
            entity = myit.GetFirst();

            while (entity)
            {
                if (entity == thirstyOne)
                {
                    if (count == 0)
                        return -1;
                    else
                        return count;
                }

                count++;
                entity = myit.GetNext();
            }
        }
        thirstyQ->ForcedInsert(thirstyOne);

        if (count not_eq 0)
        {
            FalconTankerMessage *tankMsg;

            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = thirstyOne->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = (float)count;
            FalconSendMessage(tankMsg);
        }
    }

    return count;
}

void TankerBrain::AIReady()
{
    flags or_eq AIready;
}

void TankerBrain::RemoveFromQ(SimVehicleClass* thirstyOne)
{
    if (thirstyOne == curThirsty)
        curThirsty = NULL;

    thirstyQ->Remove(thirstyOne);
}

int TankerBrain::AddToWaitQ(SimVehicleClass* doneOne)
{
    VuEntity *entity = NULL;
    int count = 1;

    {
        VuListIterator myit(waitQ);
        entity = myit.GetFirst();

        while (entity)
        {
            if (entity == doneOne)
            {
                return count;
            }

            count++;
            entity = myit.GetNext();
        }
    }
    waitQ->ForcedInsert(doneOne);

    count = 1;
    {
        VuListIterator myit(waitQ);
        entity = myit.GetFirst();

        while (entity)
        {
            FalconTankerMessage *tankMsg;

            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = doneOne->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = (float)count;
            FalconSendMessage(tankMsg);
            count++;
            entity = myit.GetNext();
        }
    }

    return 1;
}

void TankerBrain::PurgeWaitQ(void)
{
    VuEntity *entity = NULL;
    {
        VuListIterator myit(waitQ);
        entity = myit.GetFirst();

        while (entity)
        {
            FalconTankerMessage *tankMsg;

            tankMsg = new FalconTankerMessage(self->Id(), FalconLocalGame);
            tankMsg->dataBlock.caller = entity->Id();
            tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
            tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = -1;
            FalconSendMessage(tankMsg);
            entity = myit.GetNext();
        }
    }
    waitQ->Purge();
}

void TankerBrain::FrameExec(SimObjectType* tList, SimObjectType* tPtr)
{
    if ( not (flags bitand (IsRefueling bitor ClearingPlane)))
    {
        DigitalBrain::FrameExec(tList, tPtr);

        // Lights Off
        if (stype == TNKR_KC10)
            DriveLights();
        else if (stype == TNKR_KC135 and g_bLightsKC135) // when we have the lights on the KC-135 model
            DriveLights();

#ifdef DAVE_DBG

        if (MoveBoom and ServiceType == BOOM_SERVICE)
        {
            boomAzTest = max(min(boomAzTest,  23.0F * DTR), -23.0F * DTR);
            boomElTest = max(min(boomElTest, 4.0F * DTR), -40.0F * DTR);
            boomExtTest = max(min(boomExtTest, 21.0F), 0.0F);

            boom[BOOM].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boomAzTest);
            boom[BOOM].drawPointer->SetDOFangle(BOOM_ELEVATION, -boomElTest);
            boom[BOOM].drawPointer->SetDOFoffset(BOOM_EXTENSION, boomExtTest);
        }
        else
            DriveBoom();

#else
        // Boom Away
        DriveBoom();
#endif

        // 07DEC03 - FRB - Turn on drogue lights
        if (boom[DROGUE].drawPointer)
            boom[DROGUE].drawPointer->SetSwitchMask(0, 0);

        CallNext();
    }
    else
    {
        if (g_bUseTankerTrack)
        {
            if (directionsetup)
            {
                /*
                 1.) Find heading to FLOT
                 2.) Get distance to FLOT
                 3.) if Distance is too close (MINIMUM_TANKER_DISTANCE, Falcon4.AII),
                 or too far (MAXIMUM_TANKER_DISTANCE, Falcon4.AII), set first tanker trackpoint as the first "Tanker" waypoint
                 4.) In normal case: set up TrackPoints array:
                 TP[].x = XPos
                 TP[].y = YPos
                 TP[].z = Heading (no need to calculate this over and over again...)

                 TP[0] = Current location
                 TP[1] = 90° 25 nm <- this is a 180° heading away from the FLOT
                 TP[2] = 90° 60 nm <- parallel to the FLOT
                 TP[3] = 90° 25 nm <- towards the FLOT

                 boxside decides if we do a left or right box, according to heading to FLOT

                 set currentTP to point 1

                 5.) if one of 3.) is true:
                 TP[0] = farther away/closer to the FLOT
                 TP[1] = 90° 25 nm <- this is a 180° heading away from the FLOT
                 TP[2] = 90° 60 nm <- parallel to the FLOT
                 TP[3] = 90° 25 nm <- towards the FLOT

                 boxside decides if we do a left or right box

                 set currentTP to point 0

                 check if we go from TP 0 to TP 3 or from TP 0 to TP 1
                */
                float dist, dx, dy, longleg, shortleg, heading, distance;
                bool boxside = false;
                int farthercloser = 0;

                reachedFirstTrackpoint = false;

                directionsetup = 0;
                HeadsUp = true;
                advancedirection = false; // means go trackpoint 0->1->2->3

                ShiAssert(curThirsty);

                if (curThirsty)
                {
                    longleg = ((AircraftClass*)curThirsty)->af->GetTankerLongLeg();
                    shortleg = ((AircraftClass*)curThirsty)->af->GetTankerShortLeg();
                }

                distance = DistanceToFront(SimToGrid(self->YPos()), SimToGrid(self->XPos()));

                if (distance < (float)(MINIMUM_TANKER_DISTANCE - 5))
                {
                    farthercloser = 1;
                }
                else if (distance > (float)(MAXIMUM_TANKER_DISTANCE + 5))
                {
                    farthercloser = 2;
                    advancedirection = true; // go trackpoint 0->3->2->1
                }

                if (farthercloser)
                {

                    // Get the first tanker waypoint as first normal trackpoint
                    WayPoint w = NULL;
                    Flight flight = (Flight)self->GetCampaignObject();

                    if (flight)
                        w = flight->GetFirstUnitWP(); // this is takeoff time

                    // use location of first "Tanker" waypoint if we're outside our min/max FLOT range
                    while (w)
                    {
                        if (w->GetWPAction() == WP_TANKER)
                        {
                            break;
                        }

                        w = w->GetNextWP();
                    }

                    float x, y, z;

                    if (w)
                    {
                        w->GetLocation(&x, &y, &z);
                    }
                    else
                    {
                        // we have no waypoints - like in refuel training mission
                        x = self->XPos();
                        y = self->YPos();
                    }

                    // of course from our waypoints location...
                    // sfr: fixing xy order
                    // heading = DirectionToFront(SimToGrid(y),SimToGrid(x));
                    GridIndex gx, gy;
                    ::vector pos = { x, y };
                    ConvertSimToGrid(&pos, &gx, &gy);
                    heading = DirectionToFront(gx, gy);

                    if (heading < 0.0f)
                    {
                        heading += PI * 2;
                    }

                    TrackPoints[0].x = x;
                    TrackPoints[0].y = y;

                    currentTP = 0;
                }
                else
                {
                    // tanker is within min/max range to FLOT, so use current position for trackpoint 0
                    // sfr: fixing xy order
                    //heading = DirectionToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));
                    GridIndex gx, gy;
                    ::vector pos = { self->XPos(), self->YPos()};
                    ConvertSimToGrid(&pos, &gx, &gy);
                    heading = DirectionToFront(gx, gy);
                    //heading = DirectionToFront(SimToGrid(self->XPos()),SimToGrid(self->YPos()));

                    if (heading < 0.0f)
                    {
                        heading += PI * 2;
                    }

                    // TrackPoint[0], current location
                    TrackPoints[0].x = self->XPos();
                    TrackPoints[0].y = self->YPos();

                    currentTP = 1;
                }

                // the closest FLOT point is "east" of us
                if (heading >= 0.0f and heading < PI / 2.0f or heading > PI and heading < PI + PI / 2.0f)
                {
                    boxside = true;
                }// this creates a track box to the right, which should be away from the FLOT, false = to the left

                heading += PI; // turn now 180° away from the FLOT

                if (heading > PI * 2.0F)
                {
                    heading -= PI * 2.0F;
                }

                //TrackPoints[0].z = heading;

                // TrackPoint[1], initial direction
                dist = shortleg * NM_TO_FT;

                dx = TrackPoints[0].x + dist * cos(heading);
                dy = TrackPoints[0].y + dist * sin(heading);

                TrackPoints[1].x = dx;
                TrackPoints[1].y = dy;

                // TrackPoint[2] add 90° heading
                if (boxside)
                {
                    heading -= PI / 2.0f;

                    if (heading < 0.0f)
                    {
                        heading += PI * 2.0f;
                    }
                }
                else
                {
                    heading += PI / 2.0f;

                    if (heading > PI * 2.0F)
                    {
                        heading -= PI * 2.0F;
                    }
                }

                //TrackPoints[1].z = heading;

                dist = longleg * NM_TO_FT;

                dx = TrackPoints[1].x + dist * cos(heading);
                dy = TrackPoints[1].y + dist * sin(heading);

                TrackPoints[2].x = dx;
                TrackPoints[2].y = dy;

                // TrackPoint[3] add another 90° heading
                if (boxside)
                {
                    heading -= PI / 2.0f;

                    if (heading < 0.0f)
                    {
                        heading += PI * 2.0f;
                    }
                }
                else
                {
                    heading += PI / 2.0f;

                    if (heading > PI * 2.0F)
                    {
                        heading -= PI * 2.0F;
                    }
                }

                //TrackPoints[2].z = heading;

                dist = shortleg * NM_TO_FT;

                dx = TrackPoints[2].x + dist * cos(heading);
                dy = TrackPoints[2].y + dist * sin(heading);

                TrackPoints[3].x = dx;
                TrackPoints[3].y = dy;
                /*
                 if (boxside)
                 {
                 heading -= PI/2.0f;
                 if (heading<0.0f)
                 heading += PI * 2.0f;
                 }
                 else
                 {
                 heading += PI/2.0f;
                 if (heading > PI * 2.0F)
                 heading -= PI * 2.0F;
                 }

                 TrackPoints[3].z = heading;*/
                /* MN that was a BS idea... now we do if we are too close to FLOT, continue Trackpoints 0-1-2-3, if we were too far, do TP 0-3-2-1
                 // check our angle from Trackpoint 0 to Trackpoint 1 and 3 - which ever is less, head towards it
                 float x,y,fx,fy,r,a1,a2;
                 fx = TrackPoints[0].y;
                 fy = TrackPoints[0].x;
                 x = self->YPos();
                 y = self->XPos();

                 if (farthercloser)  // angle to first trackpoint
                 r = (float)atan2((float)(fx-x),(float)(fy-y));
                 else
                 r = self->Yaw(); // current heading

                 if (r < 0.0f)
                 r += PI * 2.0f;

                 a1 = r - TrackPoints[0].z;
                 if (a1 < 0.0f)
                 a1 += PI * 2.0f;

                 a2 = r - (TrackPoints[3].z + PI); // +PI as we want the angle from TP0 to TP3 and not TP3 to TP0...
                 if (a2 < 0.0f)
                 a2 += PI * 2.0f;
                 if (a2 > PI * 2.0f)
                 a2 -= PI * 2.0f;

                 if (a2 < a1)
                 advancedirection = true; // go from 0->3->2->1
                */
                // finally head towards our first trackpoint
                TurnToTrackPoint(currentTP);

            }
        }
        else
        {
            // Added by M.N.
            if (directionsetup)
            {
                // at tankercall, do an initial turn away from the FLOT, only reset by "RequestFuel" message
                directionsetup = 0;
                // sfr: fixing xy order
                //float heading = DirectionToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));
                GridIndex gx, gy;
                ::vector pos = { self->XPos(), self->YPos()};
                ConvertSimToGrid(&pos, &gx, &gy);
                //float heading = DirectionToFront(SimToGrid(self->XPos()),SimToGrid(self->YPos()));
                float heading = DirectionToFront(gx, gy);

                if (heading < 0.0f)
                {
                    heading += PI * 2;
                }

                heading += PI; // turn 180° away from the FLOT

                if (heading > PI * 2.0F)
                {
                    heading -= PI * 2.0F;
                }

                TurnTo(heading);
            }
        }

        FollowThirsty();
#ifdef DAVE_DBG

        if (MoveBoom and ServiceType == BOOM_SERVICE)
        {
            boomAzTest = max(min(boomAzTest,  23.0F * DTR), -23.0F * DTR);
            boomElTest = max(min(boomElTest, 4.0F * DTR), -40.0F * DTR);
            boomExtTest = max(min(boomExtTest, 21.0F), 0.0F);    // PJW... was 21 bitand 6

            boom[BOOM].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boomAzTest);
            boom[BOOM].drawPointer->SetDOFangle(BOOM_ELEVATION, -boomElTest);
            boom[BOOM].drawPointer->SetDOFoffset(BOOM_EXTENSION, boomExtTest);
        }

#endif
    }
}

void TankerBrain::BoomWorldPosition(Tpoint *pos)
{
    Trotation *orientation = &((DrawableBSP*)self->drawPointer)->orientation;

    if (ServiceType == BOOM_SERVICE)
    {
        pos->x = orientation->M11 * boom[BOOM].rx + orientation->M12 * boom[BOOM].ry + orientation->M13 * boom[BOOM].rz + self->XPos();
        pos->y = orientation->M21 * boom[BOOM].rx + orientation->M22 * boom[BOOM].ry + orientation->M23 * boom[BOOM].rz + self->YPos();
        pos->z = orientation->M31 * boom[BOOM].rx + orientation->M32 * boom[BOOM].ry + orientation->M33 * boom[BOOM].rz + self->ZPos();
    }
    else
    {
        pos->x = orientation->M11 * boom[DROGUE].rx + orientation->M12 * boom[DROGUE].ry + orientation->M13 * boom[DROGUE].rz + self->XPos();
        pos->y = orientation->M21 * boom[DROGUE].rx + orientation->M22 * boom[DROGUE].ry + orientation->M23 * boom[DROGUE].rz + self->YPos();
        pos->z = orientation->M31 * boom[DROGUE].rx + orientation->M32 * boom[DROGUE].ry + orientation->M33 * boom[DROGUE].rz + self->ZPos();
    }

    return;
}



// 15NOV03 - FRB - Change to use <fm>.dat RefuelingPosition
// 15NOV03 - FRB - Make the IL-78 and KC-130 drogue work with RefuelingPosition
void TankerBrain::ReceptorRelPosition(Tpoint *pos, SimVehicleClass *thirsty)
{
    // Tpoint minB, maxB;
    float rad;

    if ( not thirsty->drawPointer)
    {
        pos->x = thirsty->XPos() - self->XPos();
        pos->y = thirsty->YPos() - self->YPos();
        pos->z = thirsty->ZPos() - self->ZPos();
        return;
    }

    Tpoint recWPos;
    Tpoint recThirstyRelPos;

    // 15NOV03 - FRB - Need to use the <fm>.dat refuelLocation
    ((AircraftClass*)thirsty)->af->GetRefuelPosition(&recThirstyRelPos);

    // Use F-16 position if 0,0,0 refuelingLocation in <fm>.dat
    if (recThirstyRelPos.x == 0.0F and recThirstyRelPos.y == 0.0F and recThirstyRelPos.z == 0.0F)
    {
        if (ServiceType == BOOM_SERVICE)
        {
            recThirstyRelPos.x = 0.0F + BoomRFPos.x; // F-16's refueling port location
            recThirstyRelPos.y = 0.0F + BoomRFPos.y;
            recThirstyRelPos.z = -3.0F + BoomRFPos.z;
        }
        else // Drogue tankers
        {
            // if nothing set, use nose of a/c
            if (thirsty->drawPointer)
            {
                // ((DrawableBSP*)thirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
                rad = ((DrawableBSP*)thirsty->drawPointer)->Radius(); // Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
                recThirstyRelPos.x = rad;
                recThirstyRelPos.y = 0.0F;
                recThirstyRelPos.z = 0.0F;
            }
            else
            {
                recThirstyRelPos.x = IL78HACKX;
                recThirstyRelPos.y = 0.0F;
                recThirstyRelPos.z = 0.0F;
            }
        }
    }
    else if (ServiceType == BOOM_SERVICE) // FRB - Tanker-specific Adjustments
    {
        recThirstyRelPos.x += BoomRFPos.x;
        recThirstyRelPos.y += BoomRFPos.y;
        recThirstyRelPos.z += BoomRFPos.z;
    }

    // 15NOV03 - FRB - end

    MatrixMult(&((DrawableBSP*)thirsty->drawPointer)->orientation, &recThirstyRelPos, &recWPos);
    recWPos.x += thirsty->XPos() - self->XPos();
    recWPos.y += thirsty->YPos() - self->YPos();
    recWPos.z += thirsty->ZPos() - self->ZPos();

    if (thirsty->LastUpdateTime() == vuxGameTime)
    {
        // Correct for different frame rates
        recWPos.x -= thirsty->XDelta() * SimLibMajorFrameTime;
        recWPos.y -= thirsty->YDelta() * SimLibMajorFrameTime;
    }

    MatrixMultTranspose(&((DrawableBSP*)self->drawPointer)->orientation, &recWPos, pos);

    if (ServiceType == BOOM_SERVICE)
    {
        pos->x -= boom[BOOM].rx;
        pos->y -= boom[BOOM].ry;
        pos->z -= boom[BOOM].rz;
    }
    else
    {
        pos->x -= boom[DROGUE].rx;
        pos->y -= boom[DROGUE].ry;
        pos->z -= boom[DROGUE].rz;
    }
}

void TankerBrain::BoomTipPosition(Tpoint *pos)
{
    Trotation *orientation = &((DrawableBSP*)self->drawPointer)->orientation;
    Tpoint boompos;

    if (ServiceType == BOOM_SERVICE)
    {
        mlTrig TrigAz, TrigEl;

        mlSinCos(&TrigAz, boom[BOOM].az);
        mlSinCos(&TrigEl, boom[BOOM].el);

        boompos.x = -(33.5F + boom[BOOM].ext) * TrigAz.cos * TrigEl.cos;
        boompos.y = -(33.5F + boom[BOOM].ext) * TrigAz.sin * TrigEl.cos;
        boompos.z = -(33.5F + boom[BOOM].ext) * TrigEl.sin;

        pos->x = orientation->M11 * (boom[BOOM].rx + boompos.x) + orientation->M12 * (boom[BOOM].ry + boompos.y) + orientation->M13 * (boom[BOOM].rz + boompos.z) + self->XPos();
        pos->y = orientation->M21 * (boom[BOOM].rx + boompos.x) + orientation->M22 * (boom[BOOM].ry + boompos.y) + orientation->M23 * (boom[BOOM].rz + boompos.z) + self->YPos();
        pos->z = orientation->M31 * (boom[BOOM].rx + boompos.x) + orientation->M32 * (boom[BOOM].ry + boompos.y) + orientation->M33 * (boom[BOOM].rz + boompos.z) + self->ZPos();
    }
    else
    {
        // boompos.x = -boom[DROGUE].ext;
        /*
        // Tpoint minB, maxB;
         if (curThirsty and curThirsty->drawPointer)
         {
        //     ((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
        //     boompos.x -= maxB.x;
         float rad = ((DrawableBSP*)curThirsty->drawPointer)->Radius(); // 22NOV03 - FRB - Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
             boompos.x -= rad;
         }
        */
        boompos.x = DrogueRFPos.x;
        boompos.y = DrogueRFPos.y;
        boompos.z = DrogueRFPos.z;

        pos->x = orientation->M11 * (boom[DROGUE].rx + boompos.x) + orientation->M12 * (boom[DROGUE].ry + boompos.y) + orientation->M13 * (boom[DROGUE].rz + boompos.z) + self->XPos();
        pos->y = orientation->M21 * (boom[DROGUE].rx + boompos.x) + orientation->M22 * (boom[DROGUE].ry + boompos.y) + orientation->M23 * (boom[DROGUE].rz + boompos.z) + self->YPos();
        pos->z = orientation->M31 * (boom[DROGUE].rx + boompos.x) + orientation->M32 * (boom[DROGUE].ry + boompos.y) + orientation->M33 * (boom[DROGUE].rz + boompos.z) + self->ZPos();
    }

    return;
}

// 15NOV03 - FRB - Change to reflect <ac>.dat RefuelingPosition
// 15NOV03 - FRB - Make the IL-78 and KC-130 drogue work with RefuelingPosition
void TankerBrain::OptTankingPosition(Tpoint *pos)
{
    Tpoint boompos;

    boompos.x = boompos.y = boompos.z = 0;

    if ( not self->drawPointer)
    {
        pos->x = self->XPos();
        pos->y = self->YPos();
        pos->z = self->ZPos();
        return;
    }

    Trotation *orientation = &((DrawableBSP*)self->drawPointer)->orientation;

    if (ServiceType == BOOM_SERVICE)
    {
        if (curThirsty and curThirsty->IsAirplane() and ((AircraftClass*)curThirsty)->af)
        {
            ((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);

            // 15NOV03 - FRB - Adjust F-16 rf boom position to match current a/c rf port location
            // if nothing set, use F-16 default
            if (boompos.x == 0 and boompos.y == 0 and boompos.z == 0)
            {
                boompos.x = STDPOSX;
                // boompos.y = BoomRFPos.y;
                boompos.z = STDPOSZ;
            }
            else
            {
                boompos.x = -boompos.x + STDPOSX;
                boompos.y = -boompos.y;

                if (boompos.z)
                    boompos.z = -(boompos.z + 3.0F) + STDPOSZ;
                else
                    boompos.z = STDPOSZ;
            }
        }

        pos->x = orientation->M11 * (boom[BOOM].rx + boompos.x) + orientation->M12 * (boom[BOOM].ry + boompos.y) + orientation->M13 * (boom[BOOM].rz + boompos.z) + self->XPos();
        pos->y = orientation->M21 * (boom[BOOM].rx + boompos.x) + orientation->M22 * (boom[BOOM].ry + boompos.y) + orientation->M23 * (boom[BOOM].rz + boompos.z) + self->YPos();
        pos->z = orientation->M31 * (boom[BOOM].rx + boompos.x) + orientation->M32 * (boom[BOOM].ry + boompos.y) + orientation->M33 * (boom[BOOM].rz + boompos.z) + self->ZPos();

        if (self->LastUpdateTime() == vuxGameTime)
        {
            // Correct for different frame rates
            pos->x -= self->XDelta() * SimLibMajorFrameTime;
            pos->y -= self->YDelta() * SimLibMajorFrameTime;
        }
    }
    else
    {
        if (curThirsty and curThirsty->IsAirplane() and ((AircraftClass*)curThirsty)->af)
        {
            ((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);

            // 15NOV03 - FRB - Adjust droque rf position to match current a/c rf port location
            // if nothing set, use nose of a/c
            if (boompos.x == 0 and boompos.y == 0 and boompos.z == 0)
            {
                float rad;

                // Tpoint minB, maxB;
                if (curThirsty and curThirsty->drawPointer)
                {
                    // ((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
                    rad = ((DrawableBSP*)curThirsty->drawPointer)->Radius(); // FRB - Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
                    boompos.x = -rad + DrogueRFPos.x;
                    boompos.y = DrogueRFPos.y;
                    boompos.z = DrogueRFPos.z;
                }
                else // no model data available at this time, so fake it.
                {
                    boompos.x = DrogueRFPos.x - IL78HACKX;  // guessimate
                    boompos.y = DrogueRFPos.y;
                    boompos.z = DrogueRFPos.z;
                }
            }
            else // use refueling position data
            {
                boompos.x = -boompos.x + DrogueRFPos.x;
                boompos.y = -boompos.y + DrogueRFPos.y;
                boompos.z = -boompos.z + DrogueRFPos.z;
            }
        }
        else // not an a/c ?????
        {
            float rad;

            // Tpoint minB, maxB;
            if (curThirsty and curThirsty->drawPointer)
            {
                // ((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
                rad = ((DrawableBSP*)curThirsty->drawPointer)->Radius(); // FRB - Use Parent record radius since BoundingBox +X may be changed to decrease a/c hitbox
                boompos.x = -rad + DrogueRFPos.x;
                boompos.y = DrogueRFPos.y;
                boompos.z = DrogueRFPos.z;
            }
            else
            {
                boompos.x = DrogueRFPos.x - IL78HACKX;  // guessimate
                boompos.y = DrogueRFPos.y;
                boompos.z = DrogueRFPos.z;
            }
        }

        pos->x = orientation->M11 * (boom[DROGUE].rx + boompos.x) + orientation->M12 * (boom[DROGUE].ry + boompos.y) + orientation->M13 * (boom[DROGUE].rz + boompos.z) + self->XPos();
        pos->y = orientation->M21 * (boom[DROGUE].rx + boompos.x) + orientation->M22 * (boom[DROGUE].ry + boompos.y) + orientation->M23 * (boom[DROGUE].rz + boompos.z) + self->YPos();
        pos->z = orientation->M31 * (boom[DROGUE].rx + boompos.x) + orientation->M32 * (boom[DROGUE].ry + boompos.y) + orientation->M33 * (boom[DROGUE].rz + boompos.z) + self->ZPos();

        if (self->LastUpdateTime() == vuxGameTime)
        {
            // Correct for different frame rates
            pos->x -= self->XDelta() * SimLibMajorFrameTime;
            pos->y -= self->YDelta() * SimLibMajorFrameTime;
        }
    }

    return;
}
