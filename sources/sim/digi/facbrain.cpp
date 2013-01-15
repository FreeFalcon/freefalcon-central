#include "stdhdr.h"
#include "facbrain.h"
#include "simveh.h"
#include "unit.h"
#include "simdrive.h"
#include "MsgInc\FACMsg.h"
#include "falcsess.h"
#include "Aircrft.h"

FACBrain::FACBrain (AircraftClass *myPlatform, AirframeClass* myAf) : DigitalBrain (myPlatform, myAf)
{
   lastTarget = 0;
   controlledFighter = NULL;
   fighterQ = new TailInsertList;
   fighterQ->Register();
   flags = 0;

   campaignTarget = ((UnitClass*)(self->GetCampaignObject()))->GetUnitMissionTarget();
}

FACBrain::~FACBrain (void)
{
   fighterQ->Unregister();
   delete fighterQ;
}

void FACBrain::PostInsert (void)
{
}

SimBaseClass* FACBrain::AssignTarget (void)
{
SimBaseClass* retval = NULL;

   if (campaignTarget)
   {
      retval = campaignTarget->GetComponentEntity(lastTarget);
      if (retval)
      {
         lastTarget ++;
         lastTarget %= campaignTarget->NumberOfComponents();
      }
      else
      {
         lastTarget = 0;
         retval = campaignTarget->GetComponentEntity(lastTarget);
         lastTarget ++;
         lastTarget %= campaignTarget->NumberOfComponents();
         if (retval == NULL)
         {
            lastTarget = 0;
            campaignTarget = NULL;
         }
      }
   }

   return retval;
}

void FACBrain::RequestMark(void)
{
}

void FACBrain::RequestLocation(void)
{
}

void FACBrain::RequestTACAN(void)
{
}

void FACBrain::RequestBDA(SimVehicleClass* theFighter)
{
int numHit = numInTarget - campaignTarget->NumberOfComponents();

   if (campaignTarget)
   {
      if (numHit > 0)
      {
         // Say hold message
         FalconFACMessage* facMsg = new FalconFACMessage (theFighter->Id(), FalconLocalGame);
         facMsg->dataBlock.caller = self->Id();
         facMsg->dataBlock.type = FalconFACMessage::BDA;
         facMsg->dataBlock.data1 = (float)numHit;
         facMsg->dataBlock.data2 = 0;
         FalconSendMessage (facMsg,FALSE);
         numInTarget -= numHit;
      }
      else
      {
         // Say hold message
         FalconFACMessage* facMsg = new FalconFACMessage (theFighter->Id(), FalconLocalGame);
         facMsg->dataBlock.caller = self->Id();
         facMsg->dataBlock.type = FalconFACMessage::NoBDA;
         facMsg->dataBlock.data1 = facMsg->dataBlock.data2 = 0;
         FalconSendMessage (facMsg,FALSE);
      }
   }
}

void FACBrain::RequestTarget(SimVehicleClass*)
{
SimBaseClass* newTarget;

   newTarget = AssignTarget();

   FalconFACMessage* facMsg = new FalconFACMessage (controlledFighter->Id(), FalconLocalGame);
   facMsg->dataBlock.caller = self->Id();
   if (newTarget)
   {
      // Say situation report
      facMsg->dataBlock.type = FalconFACMessage::FacSit;
      facMsg->dataBlock.data1 = facMsg->dataBlock.data2 = facMsg->dataBlock.data3 = 0;
      facMsg->dataBlock.target = newTarget->Id();
   }
   else
   {
      // Say Hold message
      facMsg->dataBlock.type = FalconFACMessage::HoldAtCP;
      facMsg->dataBlock.data1 = facMsg->dataBlock.data2 = 0;
   }
   FalconSendMessage (facMsg,FALSE);
}

void FACBrain::AddToQ (SimVehicleClass* theFighter)
{
   fighterQ->ForcedInsert(theFighter);
   if (flags & FlightInbound)
   {
      // Say hold message
      FalconFACMessage* facMsg = new FalconFACMessage (theFighter->Id(), FalconLocalGame);
      facMsg->dataBlock.caller = self->Id();
      facMsg->dataBlock.type = FalconFACMessage::HoldAtCP;
      facMsg->dataBlock.data1 = facMsg->dataBlock.data2 = 0;
      FalconSendMessage (facMsg,FALSE);
   }
}

void FACBrain::RemoveFromQ (SimVehicleClass* theFighter)
{
   fighterQ->Remove(theFighter);
}

void FACBrain::FrameExec(SimObjectType* tList, SimObjectType* tPtr){
	SimBaseClass* newTarget;
	if (!controlledFighter){
		VuListIterator fighterWalker (fighterQ);
		controlledFighter = (SimVehicleClass*)fighterWalker.GetFirst();
		if (controlledFighter){
			flags |= FlightInbound;
			newTarget = AssignTarget();

			FalconFACMessage* facMsg = new FalconFACMessage (controlledFighter->Id(), FalconLocalGame);
			facMsg->dataBlock.caller = self->Id();
			if (newTarget){
				// Say situation report
				facMsg->dataBlock.type = FalconFACMessage::FacSit;
				facMsg->dataBlock.data1 = facMsg->dataBlock.data2 = facMsg->dataBlock.data3 = 0;
				facMsg->dataBlock.target = newTarget->Id();
			}
			else {
				// Say Hold message
				facMsg->dataBlock.type = FalconFACMessage::HoldAtCP;
				facMsg->dataBlock.data1 = facMsg->dataBlock.data2 = 0;
			}
			FalconSendMessage (facMsg,FALSE);
		}
	}
	DigitalBrain::FrameExec(tList, tPtr);
}
