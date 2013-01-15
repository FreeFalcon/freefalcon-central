#include "MsgInc/TankerMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "simdrive.h"
#include "mesg.h"
#include "tankbrn.h"
#include "aircrft.h"
#include "airframe.h"
#include "cpmanager.h"
#include "cpmisc.h"
#include "otwdrive.h"
#include "PlayerOp.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "Campbase.h"
#include "InvalidBufferException.h"


FalconTankerMessage::FalconTankerMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (TankerMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	RequestReliableTransmit ();
	RequestOutOfBandTransmit ();
}

FalconTankerMessage::FalconTankerMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (TankerMsg, FalconEvent::SimThread, senderid, target)
{
	// Your Code Goes Here
	type;
}

FalconTankerMessage::~FalconTankerMessage(void)
{
	// Your Code Goes Here
}

int FalconTankerMessage::Process(uchar autodisp)
{
	AircraftClass* theTanker;
	AircraftClass* thirstyOne;
	FalconRadioChatterMessage* radioMessage;
	AircraftClass *component = NULL;

	int	pos;

	if (autodisp)
		return 0;

	theTanker = (AircraftClass*)(vuDatabase->Find (EntityId()));
	thirstyOne = (AircraftClass*)(vuDatabase->Find (dataBlock.caller));

	switch (dataBlock.type)
	{
			case RequestFuel:
					if(thirstyOne)
					{
						if (SimDriver.GetPlayerEntity() != thirstyOne || PlayerOptions.PlayerRadioVoice)
							SendCallToPlane (theTanker, thirstyOne, rcREQUESTFUEL, FalconLocalSession);

						VuListIterator	cit(thirstyOne->GetCampaignObject()->GetComponents());
						component = (AircraftClass*)cit.GetFirst();
						while(component)
						{
							if(component->IsLocal())
							{
								if(theTanker)
								{
									component->DBrain()->SetATCFlag(DigitalBrain::NeedToRefuel);
									component->DBrain()->SetTanker(theTanker->Id());
									component->DBrain()->StartRefueling();
									theTanker->TBrain()->SetInitial(); // M.N. reset flags to allow a turn & initial direction setup (away from FLOT)
								}
							}
							component = (AircraftClass*)cit.GetNext();
						}
					}
					break;

			case ReadyForGas:
					if(thirstyOne)
					{
						if(theTanker)
							thirstyOne->DBrain()->SetTanker(theTanker->Id());
						if (SimDriver.GetPlayerEntity() != thirstyOne || PlayerOptions.PlayerRadioVoice)
							SendCallToPlane (theTanker, thirstyOne, rcREADYTOFUEL, FalconLocalSession);	
						if(theTanker)
							SendRogerToPlane(thirstyOne, theTanker);	
					}
					break;

			case DoneRefueling:
					if(thirstyOne)
					{
						thirstyOne->DBrain()->DoneRefueling();
						if(dataBlock.data1)
						{
							thirstyOne->DBrain()->ClearATCFlag(DigitalBrain::NeedToRefuel);
							if (SimDriver.GetPlayerEntity() != thirstyOne || PlayerOptions.PlayerRadioVoice)
								SendCallToPlane (theTanker, thirstyOne, rcDONEFUELING, FalconLocalSession);	
							if(theTanker)
								SendRogerToPlane(thirstyOne, theTanker);
						}
						else
						{
							if(theTanker)
								SendCallToPlane (thirstyOne, theTanker, rcDISCONNECT, FalconLocalSession);
						}
					}
					break;

			case Contact:
					if(thirstyOne)
					{
						if(theTanker)
							thirstyOne->af->SetForcedConditions(theTanker->GetVt(), theTanker->Yaw());
						thirstyOne->af->SetFlag(AirframeClass::Refueling);
					}

					if(theTanker && theTanker->IsAirplane())
					{
						theTanker->af->SetFlag(AirframeClass::Refueling);
						theTanker->af->SetForcedConditions(theTanker->GetVt(), theTanker->Yaw());
						if (SimDriver.GetPlayerEntity() != thirstyOne || PlayerOptions.PlayerRadioVoice)
							SendCallToPlane (thirstyOne, theTanker, rcCONTACT, FalconLocalSession);
					}

					if(thirstyOne == SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP) && OTWDriver.pCockpitManager) {
						OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(2);
					}
					break;

			case Breakaway:
					if(theTanker && theTanker->IsAirplane())
						SendCallToPlane (thirstyOne, theTanker, rcBREAKAWAY, FalconLocalSession);

					if(thirstyOne == SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP) && OTWDriver.pCockpitManager) {
						OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(3);
					}
					break;

			case PreContact:
					if(thirstyOne)
					{
						thirstyOne->DBrain()->SetATCFlag(DigitalBrain::Refueling);
						thirstyOne->DBrain()->SetRefuelStatus(DigitalBrain::refRefueling);
					}

					if(theTanker && theTanker->IsAirplane())
						SendCallToPlane (thirstyOne, theTanker, rcPRECONTACT, FalconLocalSession);

					if(thirstyOne == SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP) && OTWDriver.pCockpitManager) {
						OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(0);
					}
					break;

			case ClearToContact:
					if(theTanker && theTanker->IsAirplane())
						SendCallToPlane (thirstyOne, theTanker, rcCLEARTOCONTACT, FalconLocalSession);

					if(thirstyOne == SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP) && OTWDriver.pCockpitManager) {
						OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(1);
					}
					break;

			case Stabalize:
					if(theTanker && theTanker->IsAirplane())
						SendCallToPlane (thirstyOne, theTanker, rcSTABALIZE, FalconLocalSession);
					break;

			case BoomCommand:	
					if(theTanker && theTanker->IsAirplane())
					{
						radioMessage = CreateCallToPlane (thirstyOne, theTanker, rcBOOMCOMMANDS, FalconLocalSession);	
						radioMessage->dataBlock.edata[0] = -1;
						radioMessage->dataBlock.edata[1] = -1;
						radioMessage->dataBlock.edata[2] = (short)FloatToInt32(dataBlock.data1);//direction command
						FalconSendMessage(radioMessage, FALSE);
					}
					break;

			case Disconnect:
					if(thirstyOne)
						thirstyOne->af->ClearFlag(AirframeClass::Refueling);

					if(theTanker && theTanker->IsAirplane())
						SendCallToPlane (thirstyOne, theTanker, rcDISCONNECT, FalconLocalSession);

					if(thirstyOne == SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP) && OTWDriver.pCockpitManager) {
						OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(3);
					}
					break;

			case PositionUpdate:
					if(thirstyOne)
					{
						thirstyOne->DBrain()->SetTnkPosition(FloatToInt32(dataBlock.data1));
					}

					break;

			case TankerTurn:
					SendCallToPlane (thirstyOne, theTanker, rcTANKERTURN, FalconLocalSession);   			
					break;
	}

	if (theTanker && theTanker->IsLocal() && theTanker->IsAirplane() && thirstyOne && thirstyOne->IsAirplane())
	{
		switch (dataBlock.type)
		{
				case RequestFuel:
						{
							VuListIterator	cit(thirstyOne->GetCampaignObject()->GetComponents());
							component = (AircraftClass*)cit.GetFirst();
							if (thirstyOne->OwnerId() != vuLocalSessionEntity->Game()->OwnerId()) component = thirstyOne;
							while(component)
							{
								pos = ((TankerBrain*)theTanker->Brain())->AddToQ(component);
								if (thirstyOne->OwnerId() != vuLocalSessionEntity->Game()->OwnerId() ||
												component->OwnerId() == vuLocalSessionEntity->Game()->OwnerId())
									//me123 dont add other players. they need to request them self for now.
									//otherwice we have a problem getign the wingmen after the players fueled.
								{	  
									if(component == thirstyOne)
									{
										if( pos < 0)
										{
											//all this just so we can confirm what we just said
											Tpoint relPos;
											if(theTanker->IsAwake())
												theTanker->TBrain()->ReceptorRelPosition(&relPos, thirstyOne);
											else
											{
												relPos.x = theTanker->XPos();
												relPos.y = theTanker->YPos();
												relPos.z = theTanker->ZPos();
											}

											float xyRange =(float)sqrt(relPos.x*relPos.x + relPos.y*relPos.y);

											if (xyRange < 500.0F && !theTanker->TBrain()->IsSet(TankerBrain::PrecontactPos) &&
															theTanker->TBrain()->TankingPtr() &&
															fabs(theTanker->TBrain()->TankingPtr()->localData->rangedot) < 100.0F &&
															fabs(theTanker->TBrain()->TankingPtr()->localData->az) < 35.0F * DTR)
											{
												radioMessage = CreateCallToPlane (thirstyOne, theTanker, rcCLEARTOCONTACT, FalconLocalGame);
											}
											else
											{
												radioMessage = CreateCallToPlane (thirstyOne, theTanker, rcPRECONTACT, FalconLocalGame);
											}
											radioMessage->dataBlock.time_to_play = 2000;
											FalconSendMessage(radioMessage, FALSE);
										}
										else if(pos != 0)
											SendRogerToPlane(thirstyOne, theTanker, FalconLocalGame);
									}

									if(pos > 0)
									{
										FalconTankerMessage *TankerMsg	= new FalconTankerMessage (theTanker->Id(), FalconLocalGame);		
										TankerMsg->dataBlock.type	= PositionUpdate;
										TankerMsg->dataBlock.data1  = (float)pos;
										TankerMsg->dataBlock.caller	= component->Id();
										FalconSendMessage(TankerMsg);
									}
								}
								component = (AircraftClass*)cit.GetNext();
								if (thirstyOne->OwnerId() != vuLocalSessionEntity->Game()->OwnerId()) component = 0;
							}				 
						}
						break;

				case ReadyForGas:

						break;

				case DoneRefueling:
						if( ((TankerBrain*)theTanker->Brain())->TankingPosition(thirstyOne) )
							((TankerBrain*)theTanker->Brain())->RemoveFromQ(thirstyOne);
						else
							((TankerBrain*)theTanker->Brain())->DoneRefueling();

						break;

				case Contact:
						break;

				case Breakaway:
				case Disconnect:
						theTanker->af->ClearFlag(AirframeClass::Refueling);

						break;
		}
	}

	return 0;
}

