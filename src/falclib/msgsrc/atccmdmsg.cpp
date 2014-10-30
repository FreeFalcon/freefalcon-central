/*
 * Machine Generated source file for message "ATC Command".
 * NOTE: The functions here must be completed by hand.
 * Generated on 27-March-1998 at 00:04:13
 * Generated from file EVENTS.XLS by Microprose
 */

#include "MsgInc/ATCCmdMsg.h"
#include "mesg.h"
#include "digi.h"
#include "aircrft.h"
#include "ptdata.h"
#include "MsgInc/RadioChatterMsg.h"
#include "otwdrive.h"

#include "classtbl.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


#if 0 // Retro 15Jan2004
// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"
#else
#ifndef USE_DINPUT_8 // Retro 15Jan2004
#define DIRECTINPUT_VERSION 0x0700
#else
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#endif

#include "tacan.h"
#include "f4vu.h"
#include "find.h"

extern int g_nMinTacanChannel;

Objective FindAlternateLandingStrip(Flight flight);

FalconATCCmdMessage::FalconATCCmdMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) :
    FalconEvent(ATCCmdMsg, FalconEvent::SimThread, entityId, target, loopback)
{
    // Your Code Goes Here
}

FalconATCCmdMessage::FalconATCCmdMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
    : FalconEvent(ATCCmdMsg, FalconEvent::SimThread, senderid, target)
{
    // Your Code Goes Here
}

FalconATCCmdMessage::~FalconATCCmdMessage(void)
{
    // Your Code Goes Here
}

int FalconATCCmdMessage::Process(uchar autodisp)
{
    if (autodisp)
    {
        return 0;
    }

    float cosAngle = 0.0F, desAlt = 0.0F, finalHdg = 0.0F;
    float
    finalX = 0.0F, finalY = 0.0F, baseX = 0.0F, baseY = 0.0F,
    x = 0.0F , y = 0.0F, z = 0.0F, dx = 0.0F, dy = 0.0F, distD = 0.0F, distA = 0.0F;
    GridIndex X = 0, Y = 0;
    int taxiPoint = 0, value = 0, rwIndex = 0;
    FalconRadioChatterMessage *radioMessage = NULL;
    AtcStatusEnum status;

    //WayPointClass* curWaypoint;
    AircraftClass *aircraft = (AircraftClass*)vuDatabase->Find(EntityId());
    ObjectiveClass *atc = (ObjectiveClass*)vuDatabase->Find(dataBlock.from);
    ObjectiveClass *divertBase = NULL;
    ObjectiveClass *altBase = NULL;

    if (dataBlock.type == Release and aircraft and aircraft->DBrain())
    {
        if ( not aircraft->IsPlayer() or aircraft->AutopilotType() not_eq AircraftClass::CombatAP)
        {
            aircraft->DBrain()->ResetATC();
        }

        aircraft->DBrain()->ClearATCFlag(DigitalBrain::Landed);
        aircraft->DBrain()->SetATCFlag(DigitalBrain::PermitTakeoff);
    }
    else if (dataBlock.type == Landed and aircraft and aircraft->DBrain())
    {
        aircraft->DBrain()->ClearATCFlag(DigitalBrain::PermitTakeoff);
        aircraft->DBrain()->SetATCFlag(DigitalBrain::Landed);
    }

    if (aircraft and atc and aircraft->IsAirplane() and atc->IsObjective())
    {
        DigitalBrain *acBrain = aircraft->DBrain();
        ATCBrain* atcBrain = atc->brain;

        //curWaypoint = aircraft->curWaypoint;

        if (acBrain and atcBrain)
        {
            //I am sending an actual time instead of a delta, because of the huge time differences between different
            //machines at startup
            /*
               if(dataBlock.rwtimeDelta)
               acBrain->SetRunwayInfo(dataBlock.from, dataBlock.rwindex, dataBlock.rwtimeDelta + SimLibElapsedTime);
               else if(acBrain->RwTime())
               acBrain->SetRunwayInfo(dataBlock.from, dataBlock.rwindex, acBrain->RwTime());
               else
               acBrain->SetRunwayInfo(dataBlock.from, dataBlock.rwindex, SimLibElapsedTime);
             */
            acBrain->SetRunwayInfo(dataBlock.from, dataBlock.rwindex, dataBlock.rwtime);

            switch (dataBlock.type)
            {
                case TakePosition:
                    if (aircraft->OnGround())
                    {
                        break;
                    }

                    rwIndex = atcBrain->FindBestLandingRunway(aircraft, TRUE);
                    atcBrain->FindFinalPt(aircraft, rwIndex, &x, &y);
                    desAlt = atcBrain->GetAltitude(aircraft, lTakingPosition);
                    acBrain->SetATCStatus(lTakingPosition);
                    acBrain->SetTrackPoint(x, y, desAlt);
                    acBrain->CalculateNextTurnDistance();
                    break;

                case EmergencyHold:
                case Hold:
                    if (aircraft->OnGround())
                    {
                        break;
                    }

                    acBrain->SetATCStatus(lHolding);
                    radioMessage = CreateCallFromATC(atc, aircraft, rcATCORBIT2, FalconLocalSession);
                    radioMessage->dataBlock.edata[2] = -1; //altitude in thousands
                    radioMessage->dataBlock.edata[3] = -1; //altitude in thousands
                    FalconSendMessage(radioMessage, FALSE);
                    break;

                case ToFirstLeg:
                    if (aircraft->OnGround())
                        break;

                    if (acBrain->ATCStatus() < lFirstLeg)
                    {
                        acBrain->SetTaxiPoint(GetFirstPt(dataBlock.rwindex));
                        atcBrain->FindFinalPt(aircraft, dataBlock.rwindex, &finalX, &finalY);
                        cosAngle = atcBrain->DetermineAngle(aircraft, dataBlock.rwindex, lFirstLeg);

                        if (cosAngle < 0.0F)
                        {
                            atcBrain->FindBasePt(aircraft, dataBlock.rwindex, finalX, finalY, &baseX, &baseY);
                            status = atcBrain->FindFirstLegPt(aircraft, dataBlock.rwindex, acBrain->RwTime(), baseX, baseY, TRUE, &x, &y);
                        }
                        else
                        {
                            status = atcBrain->FindFirstLegPt(aircraft, dataBlock.rwindex, acBrain->RwTime(), finalX, finalY, FALSE, &x, &y);
                        }

                        if (status not_eq lFirstLeg)
                            acBrain->SendATCMsg(status);

                        desAlt = atcBrain->GetAltitude(aircraft, status);
                        acBrain->SetATCStatus(status);
                        acBrain->SetTrackPoint(x, y, desAlt);
                        acBrain->CalculateNextTurnDistance();
                    }

                    if (aircraft->IsPlayer())
                    {
                        atcBrain->MakeVectorCall(aircraft, FalconLocalSession);
                    }

                    break;

                case ToBase:
                    if (aircraft->OnGround())
                        break;

                    if (acBrain->ATCStatus() < lToBase)
                    {
                        atcBrain->FindFinalPt(aircraft, dataBlock.rwindex, &finalX, &finalY);
                        atcBrain->FindBasePt(aircraft, dataBlock.rwindex, finalX, finalY, &baseX, &baseY);
                        acBrain->SetATCStatus(lToBase);
                        acBrain->SetTrackPoint(baseX, baseY, atcBrain->GetAltitude(aircraft, lToBase));
                        acBrain->CalculateNextTurnDistance();
                    }

                    if (aircraft->IsPlayer())
                    {
                        atcBrain->MakeVectorCall(aircraft, FalconLocalSession);
                    }

                    break;

                case ToFinal:
                    if (aircraft->OnGround())
                        break;

                    if (acBrain->ATCStatus() < lToFinal)
                    {
                        atcBrain->FindFinalPt(aircraft, dataBlock.rwindex, &finalX, &finalY);
                        acBrain->SetATCStatus(lToFinal);
                        acBrain->SetTrackPoint(finalX, finalY, atcBrain->GetAltitude(aircraft, lToFinal));
                        acBrain->CalculateNextTurnDistance();
                    }

                    if (aircraft->IsPlayer())
                    {
                        atcBrain->MakeVectorCall(aircraft, FalconLocalSession);
                    }

                    break;

                case OnFinal:
                    if (aircraft->OnGround())
                        break;

                    acBrain->SetTaxiPoint(GetFirstPt(dataBlock.rwindex));
                    acBrain->SetATCStatus(lOnFinal);
                    TranslatePointData(atc, GetFirstPt(dataBlock.rwindex), &x, &y);
                    acBrain->SetTrackPoint(x, y, atcBrain->GetAltitude(aircraft, lOnFinal));

                    if (aircraft->IsPlayer())
                    {
                        //acBrain->GetTrackPoint(&x, &y, &desAlt);
                        radioMessage = CreateCallFromATC(atc, aircraft, rcTURNTOFINAL, FalconLocalSession);

                        float rnwyHdg, planeHdg;

                        rnwyHdg = PtHeaderDataTable[dataBlock.rwindex].data + 180.0F;

                        if (rnwyHdg > 360.0F)
                            rnwyHdg -= 360.0F;

                        planeHdg = aircraft->Yaw() * RTD;

                        if (planeHdg < 0.0F)
                            planeHdg += 360.0F;

                        float delta =  rnwyHdg - planeHdg;

                        if (delta > 180.0F)
                            delta -= 360.0F;
                        else if (delta < -180.0F)
                            delta += 360.0F;

                        if (fabs(delta) < 5.0F)
                            radioMessage->dataBlock.edata[2] = 2;

                        if (delta < 0.0F)
                            radioMessage->dataBlock.edata[2] = 1;
                        else
                            radioMessage->dataBlock.edata[2] = 0;

                        //radioMessage->dataBlock.edata[2] = atcBrain->CalculateStandRateTurnToPt(aircraft, x, y, &finalHdg);
                        finalHdg = PtHeaderDataTable[dataBlock.rwindex].data + 180.0F;

                        if (finalHdg > 360)
                            finalHdg -= 360;

                        radioMessage->dataBlock.edata[3] = (short)FloatToInt32(finalHdg);
                        //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
                        radioMessage->dataBlock.edata[4] = 32767; //vector type
                        FalconSendMessage(radioMessage, FALSE);
                    }

                    break;

                case Landed:
                {
                    AtcStatusEnum atcs = acBrain->ATCStatus();

                    if (atcs < lCrashed)
                    {
                        acBrain->ClearATCFlag(DigitalBrain::RequestTakeoff);

                        // sfr: only track point if ATC is on
                        // this can happen in MP when a player touches down a destroyed runway
                        if (atcs not_eq noATC)
                        {
                            taxiPoint = GetFirstPt(dataBlock.rwindex);
                            TranslatePointData(atc, GetNextPt(taxiPoint) , &x, &y);
                            acBrain->SetTrackPoint(x, y, atcBrain->GetAltitude(aircraft, lLanded));
                        }

                        acBrain->SetATCStatus(lLanded);
                    }

                    break;
                }

                case TaxiOff:
                    if ( not aircraft->OnGround())
                    {
                        break;
                    }

                    if (acBrain->ATCStatus() < lTaxiOff)
                    {
                        acBrain->SetATCStatus(lTaxiOff);
                    }

                    break;

                case EmerToBase:
                    if (aircraft->OnGround())
                        break;

                    if (acBrain->ATCStatus() not_eq lEmergencyToBase)
                    {
                        atcBrain->FindFinalPt(aircraft, dataBlock.rwindex, &finalX, &finalY);
                        atcBrain->FindBasePt(aircraft, dataBlock.rwindex, finalX, finalY, &baseX, &baseY);
                        acBrain->SetTrackPoint(baseX, baseY, atcBrain->GetAltitude(aircraft, lEmergencyToBase));
                        acBrain->SetATCStatus(lEmergencyToBase);
                        acBrain->CalculateNextTurnDistance();
                    }

                    break;

                case EmerToFinal:
                    if (aircraft->OnGround())
                        break;

                    if (acBrain->ATCStatus() not_eq lEmergencyToFinal)
                    {
                        atcBrain->FindFinalPt(aircraft, dataBlock.rwindex, &finalX, &finalY);
                        acBrain->SetTrackPoint(finalX, finalY, atcBrain->GetAltitude(aircraft, lEmergencyToFinal));
                        acBrain->SetATCStatus(lEmergencyToFinal);
                        acBrain->CalculateNextTurnDistance();
                    }

                    break;

                case EmerOnFinal:
                    if (aircraft->OnGround())
                        break;

                    if (acBrain->ATCStatus() not_eq lEmergencyOnFinal)
                    {
                        acBrain->SetTaxiPoint(GetFirstPt(dataBlock.rwindex));
                        TranslatePointData(atc, GetFirstPt(dataBlock.rwindex), &x, &y);
                        acBrain->SetATCStatus(lEmergencyOnFinal);
                        acBrain->SetTrackPoint(x, y, atcBrain->GetAltitude(aircraft, lEmergencyOnFinal));
                    }

                    break;

                case Abort:
                    if (aircraft->OnGround())
                        break;

                    atcBrain->FindAbortPt(aircraft, &x, &y, &z);

                    if (acBrain->ATCStatus() not_eq lAborted)
                    {
                        acBrain->SetATCStatus(lAborted);
                        acBrain->SetTrackPoint(x, y, z);
                        acBrain->CalculateNextTurnDistance();
                    }

                    if ( not aircraft->IsPlayer() and rand() % 3)
                    {
                        radioMessage = CreateCallFromATC(atc, aircraft, rcATCGOAROUND2, FalconLocalSession);
                        //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
                        radioMessage->dataBlock.edata[3] = 32767;
                    }
                    else
                    {
                        radioMessage = CreateCallFromATC(atc, aircraft, rcATCGOAROUND, FalconLocalSession);

                        atcBrain->CalculateStandRateTurnToPt(aircraft, x, y, &finalHdg);
                        radioMessage->dataBlock.edata[3] = (short)FloatToInt32(finalHdg);
                        radioMessage->dataBlock.edata[4] = 32767;
                    }

                    value = rand() % 4;

                    switch (value)
                    {
                        case 0:
                        case 1:
                        case 2:
                            radioMessage->dataBlock.edata[2] = (short)value;
                            break;

                        case 3:
                            radioMessage->dataBlock.edata[2] = 11;
                            break;
                    };

                    FalconSendMessage(radioMessage, FALSE);

                    if (atc->IsLocal())
                    {
                        atcBrain->RemoveTraffic(aircraft->Id(), PtHeaderDataTable[dataBlock.rwindex].runwayNum);
                    }

                    break;

                case ClearToLand:
                    if (aircraft->OnGround())
                        break;

                    radioMessage = CreateCallFromATC(atc, aircraft, rcCLEAREDLAND, FalconLocalSession);
                    radioMessage->dataBlock.edata[4] = (short)atcBrain->GetRunwayName(atcBrain->GetOppositeRunway(dataBlock.rwindex));
                    FalconSendMessage(radioMessage, FALSE);
                    acBrain->SetATCFlag(DigitalBrain::ClearToLand);
                    break;

                case Taxi:
                    if ( not aircraft->OnGround())
                        break;

                    switch (PtDataTable[acBrain->GetTaxiPoint()].type)
                    {
                        case RunwayPt:
                            atcBrain->FindRunwayPt((Flight)aircraft->GetCampaignObject(), aircraft->vehicleInUnit, dataBlock.rwindex, &x, &y);
                            break;

                        case TakeoffPt:
                            atcBrain->FindTakeoffPt((Flight)aircraft->GetCampaignObject(), aircraft->vehicleInUnit, dataBlock.rwindex, &x, &y);
                            break;

                        default:
                        case TakeRunwayPt:
                        case TaxiPt:
                            TranslatePointData(atc, acBrain->GetTaxiPoint() , &x, &y);
                    }

                    acBrain->SetTrackPoint(x, y, atcBrain->GetAltitude(aircraft, tTaxi));

                    acBrain->ClearATCFlag(DigitalBrain::PermitRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeoff);
                    //aircraft->af->ClearFlag(AirframeClass::EngineOff);
                    //aircraft->af->ClearFlag(AirframeClass::ThrottleCheck);

                    acBrain->SetATCStatus(tTaxi);
                    acBrain->CalculateTaxiSpeed(10.0F);
                    acBrain->SetWaitTimer(acBrain->CalcWaitTime(atcBrain));
                    //CalcTaxiTime not working yet, it broke AI taxi
                    // acBrain->SetWaitTimer(acBrain->CalcTaxiTime(atcBrain)); //RAS - 11Oct04 - calc new taxi time
                    break;

                case Wait:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->ClearATCFlag(DigitalBrain::PermitRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeoff);
                    acBrain->SetATCStatus(tWait);
                    acBrain->SetWaitTimer(acBrain->CalcWaitTime(atcBrain));
                    break;

                case EmergencyStop:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->ClearATCFlag(DigitalBrain::PermitRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeoff);
                    acBrain->SetATCStatus(tEmerStop);
                    break;

                case HoldShort:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->ClearATCFlag(DigitalBrain::PermitRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeoff);
                    acBrain->SetATCStatus(tTaxi);
                    //RAS - not sure why 10.0?
                    acBrain->CalculateTaxiSpeed(10.0F);
                    acBrain->SetWaitTimer(acBrain->CalcWaitTime(atcBrain));
                    break;

                case PrepToTakeRunway:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->ClearATCFlag(DigitalBrain::PermitRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeoff);
                    acBrain->SetATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->SetATCStatus(tTaxi);
                    acBrain->SetWaitTimer(acBrain->CalcWaitTime(atcBrain));
                    break;

                case TakeRunway:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->SetATCStatus(tTakeRunway);
                    acBrain->SetATCFlag(DigitalBrain::PermitRunway);
                    acBrain->SetATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->ClearATCFlag(DigitalBrain::PermitTakeoff);
                    acBrain->SetWaitTimer(acBrain->CalcWaitTime(atcBrain));

                    if ( not acBrain->isWing or aircraft->IsPlayer() or aircraft->vehicleInUnit == 2 or
 not atcBrain->UseSectionTakeoff((Flight)aircraft->GetCampaignObject(), dataBlock.rwindex))
                    {
                        radioMessage = CreateCallFromATC(atc, aircraft, rcPOSITIONANDHOLD, FalconLocalSession);
                        radioMessage->dataBlock.edata[3] = (short)atcBrain->GetRunwayName(dataBlock.rwindex);
                        FalconSendMessage(radioMessage, FALSE);
                    }

                    break;

                case Takeoff:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->SetATCStatus(tTakeRunway);
                    acBrain->SetATCFlag(DigitalBrain::PermitRunway);
                    acBrain->SetATCFlag(DigitalBrain::PermitTakeRunway);
                    acBrain->SetATCFlag(DigitalBrain::PermitTakeoff);
                    acBrain->SetWaitTimer(acBrain->CalcWaitTime(atcBrain));

                    if ( not acBrain->isWing or aircraft->IsPlayer() or aircraft->vehicleInUnit == 2 or
 not atcBrain->UseSectionTakeoff((Flight)aircraft->GetCampaignObject(), dataBlock.rwindex))
                    {
                        if (rand() % 2 or aircraft->IsPlayer())
                        {
                            radioMessage = CreateCallFromATC(atc, aircraft, rcCLEAREDONRUNWAY, FalconLocalSession);
                            radioMessage->dataBlock.edata[3] = (short)atcBrain->GetRunwayName(dataBlock.rwindex);
                            FalconSendMessage(radioMessage, FALSE);
                        }
                        else
                        {
                            SendCallFromATC(atc, aircraft, rcCLEAREDDEPARTURE, FalconLocalSession);
                        }
                    }

                    break;

                case Divert:
                    if (aircraft->OnGround())
                        break;

                    {
                        vector pos;
                        pos.x = aircraft->XPos();
                        pos.y = aircraft->YPos();
                        ConvertSimToGrid(&pos, &X, &Y);
                        divertBase = FindNearestFriendlyAirbase(aircraft->GetTeam(), X, Y);
                        //altBase = FindAlternateLandingStrip ((Flight)aircraft->GetCampaignObject());
                        //find closest one
                        /*if( not divertBase)
                          {
                          divertBase = altBase;
                          }
                          else if(altBase)
                          {
                          dx = altBase->XPos() - aircraft->XPos();
                          dy = altBase->XPos() - aircraft->XPos();
                          distA = dx*dx + dy*dy;

                          dx = divertBase->XPos() - aircraft->XPos();
                          dy = divertBase->XPos() - aircraft->XPos();
                          distD = dx*dx + dy*dy;

                          if(distD > distA)
                          divertBase = altBase;
                          }*/

                        if ( not divertBase)
                        {
                            divertBase = FindNearestFriendlyRunway(aircraft->GetTeam(), X, Y);
                        }


                        if (divertBase)
                        {
                            if (divertBase->GetType() == TYPE_AIRBASE)
                            {
                                int channel;
                                TacanList::StationSet station;
                                TacanList::Domain domain;
                                int range, ttype;
                                float ilsfreq;

                                //Cobra We need to get bearing/range for the calls
                                float az;
                                float rangediv;
                                float xtest = aircraft->XPos();
                                float ytest = aircraft->YPos();
                                float divx = divertBase->XPos();
                                float divy = divertBase->YPos();
                                float diffx = (xtest - divx);
                                float diffy = (ytest - divy);
                                //ATC takes the NM and does some conversion somewhere... so we have to
                                //Add in the NMTOKM to compensate
                                rangediv = NM_TO_KM * ((float)sqrt(diffx * diffx + diffy * diffy) * FT_TO_NM);

                                az = RTD * (float)atan2(divy - ytest, divx - xtest);

                                if (az < -0.6f)
                                    az += 360.0f;

                                //End

                                radioMessage = CreateCallFromATC(atc, aircraft, rcATCDIVERT, FalconLocalSession);
                                radioMessage->dataBlock.edata[3] = (short)divertBase->brain->Name();
                                radioMessage->dataBlock.edata[4] = (short)az/*SimToGrid(divertBase->YPos())*/;
                                radioMessage->dataBlock.edata[5] = (short)rangediv/*SimToGrid(divertBase->XPos())*/;
                                gTacanList->GetChannelFromVUID(divertBase->Id(), &channel, &station, &domain, &range, &ttype, &ilsfreq);

                                radioMessage->dataBlock.edata[6] = (short)(channel - g_nMinTacanChannel); //right now Joe has this using the eBearing eval
                                //but this will be replaced by a channel eval
                                radioMessage->dataBlock.edata[7] = (short)(station + 23); //this uses the eAlfabet eval and stations are
                                //either X or Y
                            }
                            else
                            {
                                //radioMessage = CreateCallFromATC( atc, aircraft, rcDIVERTSTRIPBRA, FalconLocalSession);
                                //radioMessage->dataBlock.edata[0] = 0;
                                //radioMessage->dataBlock.edata[1] = SimToGrid(divertBase->YPos());
                                //radioMessage->dataBlock.edata[2] = SimToGrid(divertBase->XPos());
                            }

                            FalconSendMessage(radioMessage, FALSE);
                            acBrain->SetRunwayInfo(divertBase->Id(), 0, SimLibElapsedTime);
                            acBrain->SetTrackPoint(divertBase->XPos(), divertBase->YPos(), -5000.0F);
                            acBrain->CalculateNextTurnDistance();
                        }
                        else
                        {
                            SendCallFromATC(atc, aircraft, rcUSEALTFIELD, FalconLocalSession);
                        }

                        acBrain->SetATCStatus(lIngressing);
                        break;
                    }

                case TaxiBack:
                    if ( not aircraft->OnGround())
                        break;

                    acBrain->SetATCStatus(tTaxiBack);
                    break;

                case Release:
                    if (aircraft->OnGround())
                        break;

                    if ( not aircraft->IsPlayer() or aircraft->AutopilotType() not_eq AircraftClass::CombatAP)
                        acBrain->ResetATC();

                    //Cobra Let's try a new comm here
                    //rcDEPARTHEADING (7)
                    //rcRESUMEOWNNAV (5)
                    radioMessage = CreateCallFromATC(atc, aircraft, rcDEPARTHEADING, FalconLocalSession);
                    radioMessage->dataBlock.edata[3] = atcBrain->FindBestLandingRunway(aircraft, TRUE);
                    FalconSendMessage(radioMessage, FALSE);
                    break;

                case ExitSim:
                    if (aircraft->IsLocal())
                    {
                        OTWDriver.ExitMenu(DIK_E);
                    }

                    break;

                default:
                    break;
            }
        }
    }

    return 1;
}

