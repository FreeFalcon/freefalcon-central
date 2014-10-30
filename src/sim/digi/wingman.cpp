#include "stdhdr.h"
#include "digi.h"
#include "mesg.h"
#include "simveh.h"
#include "MsgInc/WingmanMsg.h"
#include "find.h"
#include "flight.h"
#include "wingorder.h"
#include "classtbl.h"
#include "Aircrft.h"

DigitalBrain::BVRProfileType BvrLookup[] =
{
    DigitalBrain::Plevel1a,
    DigitalBrain::Plevel2a,
    DigitalBrain::Plevel3a,
    DigitalBrain::Plevel1b,
    DigitalBrain::Plevel2b,
    DigitalBrain::Plevel3b,
    DigitalBrain::Plevel1c,
    DigitalBrain::Plevel2c,
    DigitalBrain::Plevel3c,
    DigitalBrain::Pbeamdeploy,
    DigitalBrain::Pbeambeam,
    DigitalBrain::Pwall,
    DigitalBrain::Pgrinder,
    DigitalBrain::Pwideazimuth,
    DigitalBrain::Pshortazimuth,
    DigitalBrain::PwideLT,
    DigitalBrain::PShortLT,
    DigitalBrain::PDefensive
};

void DigitalBrain::ReceiveOrders(FalconEvent* theEvent)
{

    FalconWingmanMsg *wingMsg;
    FalconWingmanMsg::WingManCmd command;
    FlightClass *p_flight;
    AircraftClass *p_from;
    int fromIndex;
    short edata[10];

    if ( not self->IsAwake() or self->IsDead())
        return;

    //we can't follow orders about how to fly if we're on the ground still
    if (self->OnGround() or atcstatus >= lOnFinal)
        return;

    wingMsg = (FalconWingmanMsg *)theEvent;
    command = (FalconWingmanMsg::WingManCmd) wingMsg->dataBlock.command;

    p_flight = (FlightClass*) vuDatabase->Find(wingMsg->EntityId());
    p_from = (AircraftClass*) vuDatabase->Find(wingMsg->dataBlock.from);
    fromIndex = p_flight->GetComponentIndex(p_from);

    if (curMode == GunsJinkMode or curMode == MissileDefeatMode or curMode == DefensiveModes)
    {

        switch (command)
        {
                // Commands that change Action and Search States
            case FalconWingmanMsg::WMRTB:
            case FalconWingmanMsg::WMClearSix:
            case FalconWingmanMsg::WMCheckSix:
            case FalconWingmanMsg::WMBreakRight:
            case FalconWingmanMsg::WMBreakLeft:
            case FalconWingmanMsg::WMPosthole:
            case FalconWingmanMsg::WMPince:
            case FalconWingmanMsg::WMChainsaw:
            case FalconWingmanMsg::WMSSOffset:
            case FalconWingmanMsg::WMFlex:
            case FalconWingmanMsg::WMShooterMode:
            case FalconWingmanMsg::WMCoverMode:
            case FalconWingmanMsg::WMSearchGround:
            case FalconWingmanMsg::WMSearchAir:
            case FalconWingmanMsg::WMResumeNormal:
            case FalconWingmanMsg::WMRejoin:
            case FalconWingmanMsg::WMAssignTarget:
            case FalconWingmanMsg::WMAssignGroup:
            case FalconWingmanMsg::WMWeaponsHold:
            case FalconWingmanMsg::WMSpread:
            case FalconWingmanMsg::WMWedge:
            case FalconWingmanMsg::WMTrail:
            case FalconWingmanMsg::WMLadder:
            case FalconWingmanMsg::WMStack:
            case FalconWingmanMsg::WMResCell:
            case FalconWingmanMsg::WMBox:
            case FalconWingmanMsg::WMArrowHead:
            case FalconWingmanMsg::WMFluidFour:
            case FalconWingmanMsg::WMVic:
            case FalconWingmanMsg::WMFinger4:
            case FalconWingmanMsg::WMEchelon:
            case FalconWingmanMsg::WMForm1:
            case FalconWingmanMsg::WMForm2:
            case FalconWingmanMsg::WMForm3:
            case FalconWingmanMsg::WMForm4:
            case FalconWingmanMsg::WMKickout:
            case FalconWingmanMsg::WMCloseup:
            case FalconWingmanMsg::WMToggleSide:
            case FalconWingmanMsg::WMIncreaseRelAlt:
            case FalconWingmanMsg::WMDecreaseRelAlt:
                edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
                edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + self->GetCampaignObject()->GetComponentIndex(self) + 1;
                edata[2] = -1;
                edata[3] = -1;
                AiMakeRadioResponse(self, rcUNABLE, edata);

                // 2001-07-28 ADDED BY S.G. AI NEEDS TO KNOW WHAT DO TO ONCE HE FINISHES HIS DEFENSE...
                switch (command)
                {
                    case FalconWingmanMsg::WMShooterMode:
                        AiGoShooter();
                        break;

                    case FalconWingmanMsg::WMCoverMode:
                        AiGoCover();
                        break;

                    case FalconWingmanMsg::WMRejoin:
                        AiRejoin(wingMsg, AI_REJOIN); // 2001-10-23 ADDED AI_REJOIN BY S.G. So the AI knows it comes from the lead
                        //underOrders = FALSE;
                        break;

                        // Commands that affect mode basis
                    case FalconWingmanMsg::WMAssignTarget:
                    case FalconWingmanMsg::WMAssignGroup:
                        AiDesignateTarget(wingMsg);
                        break;
                }

                // END OF ADDED SECTION
                break;

            case FalconWingmanMsg::WMRaygun:
                AiRaygun(wingMsg);
                break;

            case FalconWingmanMsg::WMBuddySpike:
                AiBuddySpikeReact(wingMsg);
                break;

            case FalconWingmanMsg::WMRadarOn:
                AiSetRadarActive(wingMsg);
                break;

            case FalconWingmanMsg::WMRadarStby:
                AiSetRadarStby(wingMsg);
                break;

                // Transient Commands
            case FalconWingmanMsg::WMGiveBra:
                AiGiveBra(wingMsg);
                break;

            case FalconWingmanMsg::WMGiveStatus:
                AiGiveStatus(wingMsg);
                break;

            case FalconWingmanMsg::WMGiveDamageReport:
                AiGiveDamageReport(wingMsg);
                break;

            case FalconWingmanMsg::WMGiveFuelState:
                AiGiveFuelStatus(wingMsg);
                break;

            case FalconWingmanMsg::WMWeaponsFree:
                AiSetWeaponsAction(wingMsg, AI_WEAPONS_FREE);
                break;

            case FalconWingmanMsg::WMGiveWeaponsCheck:
                AiGiveWeaponsStatus();
                break;

            case FalconWingmanMsg::WMSmokeOn:
                AiSmokeOn(wingMsg);
                break;

            case FalconWingmanMsg::WMSmokeOff:
                AiSmokeOff(wingMsg);
                break;

            case FalconWingmanMsg::WMJokerFuel:
                edata[0] = isWing;
                edata[1] = 0;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidJoker);

                break;

            case FalconWingmanMsg::WMBingoFuel:
                edata[0] = isWing;
                edata[1] = 1;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidBingo);

                break;

            case FalconWingmanMsg::WMFumes:
                edata[0] = isWing;
                edata[1] = 2;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidFumes);

                break;

            case FalconWingmanMsg::WMFlameout:
                edata[0] = isWing;
                edata[1] = 3;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidFlameout);

                break;

            case FalconWingmanMsg::WMGlue:
                AiGlueWing();
                break;

            case FalconWingmanMsg::WMSplit:
                AiSplitWing();
                break;

            case FalconWingmanMsg::WMDropStores:
                AiDropStores(wingMsg);

            case FalconWingmanMsg::WMECMOn:
                AiECMOn(wingMsg);
                break;

            case FalconWingmanMsg::WMECMOff:
                AiECMOff(wingMsg);
                break;

                // 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
            case FalconWingmanMsg::WMPlevel1a:
            case FalconWingmanMsg::WMPlevel2a:
            case FalconWingmanMsg::WMPlevel3a:
            case FalconWingmanMsg::WMPlevel1b:
            case FalconWingmanMsg::WMPlevel2b:
            case FalconWingmanMsg::WMPlevel3b:
            case FalconWingmanMsg::WMPlevel1c:
            case FalconWingmanMsg::WMPlevel2c:
            case FalconWingmanMsg::WMPlevel3c:
            case FalconWingmanMsg::WMPbeamdeploy:
            case FalconWingmanMsg::WMPbeambeam:
            case FalconWingmanMsg::WMPwall:
            case FalconWingmanMsg::WMPgrinder:
            case FalconWingmanMsg::WMPwideazimuth:
            case FalconWingmanMsg::WMPshortazimuth:
            case FalconWingmanMsg::WMPwideLT:
            case FalconWingmanMsg::WMPShortLT:
            case FalconWingmanMsg::WMPDefensive:
                if (flightLead and ((AircraftClass *)flightLead)->DBrain())
                    ((AircraftClass *)flightLead)->DBrain()->SetBvrCurrProfile(BvrLookup[command - FalconWingmanMsg::WMPlevel1a]);

                fromIndex = self->GetCampaignObject()->GetComponentIndex(self);

                if (AiIsFullResponse(fromIndex, wingMsg->dataBlock.to))
                {
                    edata[0] = fromIndex;
                    edata[1] = 1;
                    AiMakeRadioResponse(self, rcROGER, edata);
                }
                else
                    AiRespondShortCallSign(self);

                break;
                // END OF ADDED SECTION 2002-03-15
        }
    }
    else
    {
        switch (command)
        {

                // Other Commands
            case FalconWingmanMsg::WMPromote:
                AiPromote();
                break;

            case FalconWingmanMsg::WMRadarOn:
                AiSetRadarActive(wingMsg);
                break;

            case FalconWingmanMsg::WMRadarStby:
                AiSetRadarStby(wingMsg);
                break;

            case FalconWingmanMsg::WMBuddySpike:
                AiBuddySpikeReact(wingMsg);
                break;

            case FalconWingmanMsg::WMRaygun:
                AiRaygun(wingMsg);
                break;

                // Commands that change Action and Search States
            case FalconWingmanMsg::WMRTB:
                AiRTB(wingMsg);
                break;

            case FalconWingmanMsg::WMClearSix:
                AiClearLeadersSix(wingMsg);
                break;

            case FalconWingmanMsg::WMCheckSix:
                AiCheckOwnSix(wingMsg);
                break;

            case FalconWingmanMsg::WMBreakRight:
                AiBreakRight();
                break;

            case FalconWingmanMsg::WMBreakLeft:
                AiBreakLeft();
                break;

            case FalconWingmanMsg::WMPosthole:
                AiInitPosthole(wingMsg);
                break;


            case FalconWingmanMsg::WMSSOffset:
                AiInitSSOffset(wingMsg);
                break;

            case FalconWingmanMsg::WMPince:
                AiInitPince(wingMsg, TRUE);
                break;

            case FalconWingmanMsg::WMChainsaw:
                AiInitChainsaw(wingMsg);
                break;

            case FalconWingmanMsg::WMFlex:
                AiInitFlex();
                break;


            case FalconWingmanMsg::WMShooterMode:
                AiGoShooter();
                break;

            case FalconWingmanMsg::WMCoverMode:
                AiGoCover();
                break;


            case FalconWingmanMsg::WMSearchGround:
                AiSearchForTargets(DOMAIN_LAND);
                break;


            case FalconWingmanMsg::WMSearchAir:
                AiSearchForTargets(DOMAIN_AIR);
                break;


            case FalconWingmanMsg::WMResumeNormal:
                AiResumeFlightPlan(wingMsg);
                break;


            case FalconWingmanMsg::WMRejoin:
                AiRejoin(wingMsg, AI_REJOIN); // 2001-10-23 ADDED AI_REJOIN BY S.G. So the AI knows it comes from the lead
                //underOrders = FALSE;
                break;

                // Commands that affect mode basis
            case FalconWingmanMsg::WMAssignTarget:
            case FalconWingmanMsg::WMAssignGroup:
                AiDesignateTarget(wingMsg);
                break;


            case FalconWingmanMsg::WMWeaponsHold:
                AiSetWeaponsAction(wingMsg, AI_WEAPONS_HOLD);
                break;


            case FalconWingmanMsg::WMWeaponsFree:
                AiSetWeaponsAction(wingMsg, AI_WEAPONS_FREE);
                break;

                // Commands that modify formation
            case FalconWingmanMsg::WMSpread:
            case FalconWingmanMsg::WMWedge:
            case FalconWingmanMsg::WMTrail:
            case FalconWingmanMsg::WMLadder:
            case FalconWingmanMsg::WMStack:
            case FalconWingmanMsg::WMResCell:
            case FalconWingmanMsg::WMBox:
            case FalconWingmanMsg::WMArrowHead:
            case FalconWingmanMsg::WMFluidFour:
            case FalconWingmanMsg::WMVic:
            case FalconWingmanMsg::WMFinger4:
            case FalconWingmanMsg::WMEchelon:
            case FalconWingmanMsg::WMForm1:
            case FalconWingmanMsg::WMForm2:
            case FalconWingmanMsg::WMForm3:
            case FalconWingmanMsg::WMForm4:
                AiSetFormation(wingMsg);
                break;


            case FalconWingmanMsg::WMKickout:
                AiKickout(wingMsg);
                break;


            case FalconWingmanMsg::WMCloseup:
                AiCloseup(wingMsg);
                break;


            case FalconWingmanMsg::WMToggleSide:
                AiToggleSide();
                break;


            case FalconWingmanMsg::WMIncreaseRelAlt:
                AiIncreaseRelativeAltitude();
                break;


            case FalconWingmanMsg::WMDecreaseRelAlt:
                AiDecreaseRelativeAltitude();
                break;


                // Transient Commands
            case FalconWingmanMsg::WMGiveBra:
                AiGiveBra(wingMsg);
                break;


            case FalconWingmanMsg::WMGiveStatus:
                AiGiveStatus(wingMsg);
                break;


            case FalconWingmanMsg::WMGiveDamageReport:
                AiGiveDamageReport(wingMsg);
                break;


            case FalconWingmanMsg::WMGiveFuelState:
                AiGiveFuelStatus(wingMsg);
                break;


            case FalconWingmanMsg::WMGiveWeaponsCheck:
                AiGiveWeaponsStatus();
                break;

            case FalconWingmanMsg::WMSmokeOn:
                AiSmokeOn(wingMsg);
                break;

            case FalconWingmanMsg::WMSmokeOff:
                AiSmokeOff(wingMsg);
                break;

            case FalconWingmanMsg::WMECMOn:
                AiECMOn(wingMsg);
                break;

            case FalconWingmanMsg::WMECMOff:
                AiECMOff(wingMsg);
                break;


            case FalconWingmanMsg::WMJokerFuel:
                edata[0] = isWing;
                edata[1] = 0;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidJoker);

                break;

            case FalconWingmanMsg::WMBingoFuel:
                edata[0] = isWing;
                edata[1] = 1;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidBingo);

                break;

            case FalconWingmanMsg::WMFumes:
                edata[0] = isWing;
                edata[1] = 2;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidFumes);

                break;

            case FalconWingmanMsg::WMFlameout:
                edata[0] = isWing;
                edata[1] = 3;
                AiMakeRadioResponse(p_from, rcFUELCRITICAL, edata);

                if ( not isWing)
                    FlightMemberWantsFuel(SaidFlameout);

                break;

            case FalconWingmanMsg::WMGlue:
                AiGlueWing();
                break;

            case FalconWingmanMsg::WMSplit:
                AiSplitWing();
                break;

            case FalconWingmanMsg::WMDropStores:
                AiDropStores(wingMsg);

                // 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
            case FalconWingmanMsg::WMPlevel1a:
            case FalconWingmanMsg::WMPlevel2a:
            case FalconWingmanMsg::WMPlevel3a:
            case FalconWingmanMsg::WMPlevel1b:
            case FalconWingmanMsg::WMPlevel2b:
            case FalconWingmanMsg::WMPlevel3b:
            case FalconWingmanMsg::WMPlevel1c:
            case FalconWingmanMsg::WMPlevel2c:
            case FalconWingmanMsg::WMPlevel3c:
            case FalconWingmanMsg::WMPbeamdeploy:
            case FalconWingmanMsg::WMPbeambeam:
            case FalconWingmanMsg::WMPwall:
            case FalconWingmanMsg::WMPgrinder:
            case FalconWingmanMsg::WMPwideazimuth:
            case FalconWingmanMsg::WMPshortazimuth:
            case FalconWingmanMsg::WMPwideLT:
            case FalconWingmanMsg::WMPShortLT:
            case FalconWingmanMsg::WMPDefensive:
                if (flightLead and ((AircraftClass *)flightLead)->DBrain())
                    ((AircraftClass *)flightLead)->DBrain()->SetBvrCurrProfile(BvrLookup[command - FalconWingmanMsg::WMPlevel1a]);

                fromIndex = self->GetCampaignObject()->GetComponentIndex(self);

                if (AiIsFullResponse(fromIndex, wingMsg->dataBlock.to))
                {
                    edata[0] = fromIndex;
                    edata[1] = 1;
                    AiMakeRadioResponse(self, rcROGER, edata);
                }
                else
                    AiRespondShortCallSign(self);

                break;
                // END OF ADDED SECTION 2002-03-15
        } // end switch
    }
}

