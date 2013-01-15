#include "stdhdr.h"
#include "hdigi.h"
#include "mesg.h"
#include "simbase.h"
#include "MsgInc\WingmanMsg.h"
#include "simveh.h"
#include "campBase.h"
#include "otwdrive.h"

void HeliBrain::ReceiveOrders (FalconEvent* theEvent)
{
//VUKLUDGE
FalconWingmanMsg *wingCommand = (FalconWingmanMsg *)theEvent;
int goLead = FALSE;

   if (!self->IsAwake())
	   return;

   switch (wingCommand->dataBlock.command)
   {
		case FalconWingmanMsg::WMSpread:
		case FalconWingmanMsg::WMWedge:
		case FalconWingmanMsg::WMTrail:
		case FalconWingmanMsg::WMLadder:
		case FalconWingmanMsg::WMStack:
		case FalconWingmanMsg::WMResCell:
		case FalconWingmanMsg::WMBox:
		case FalconWingmanMsg::WMArrowHead:
		case FalconWingmanMsg::WMFluidFour:
	        curFormation = wingCommand->dataBlock.command;
      case FalconWingmanMsg::WMRejoin:
         underOrders = FALSE;
      break;

      case FalconWingmanMsg::WMBreakRight:
         underOrders = TRUE;
         headingOrdered = self->Yaw() + 90.0F * DTR;
         altitudeOrdered = self->ZPos();
         curOrder = wingCommand->dataBlock.command;
      break;

      case FalconWingmanMsg::WMBreakLeft:
         underOrders = TRUE;
         headingOrdered = self->Yaw() - 90.0F * DTR;
         altitudeOrdered = self->ZPos();
         curOrder = wingCommand->dataBlock.command;
      break;

      case FalconWingmanMsg::WMAssignTarget:
      break;

      case FalconWingmanMsg::WMPosthole:
      case FalconWingmanMsg::WMPince:
      case FalconWingmanMsg::WMChainsaw:
      break;

      case FalconWingmanMsg::WMFree:
         goLead = TRUE;
         underOrders = FALSE;
      break;

      case FalconWingmanMsg::WMPromote:
         isWing --;
         if (!isWing)
         {
            SetLead(TRUE);
            self->flightLead = self;
		    }
         else
            self->flightLead = (HelicopterClass *)self->GetCampaignObject()->GetComponentLead();
      break;

      default:
         curFormation = FalconWingmanMsg::WMWedge;
         MonoPrint ("Digi %d received bad order %d at\n", self->Id().num_,
            wingCommand->dataBlock.command, SimLibElapsedTime);
      break;
   }

   if (goLead && isWing)
   {
      SetLead (TRUE);
   }
   else if (isWing)
   {
      SetLead(FALSE);
   }

   MonoPrint ("Message received %d at %.2f\n",
      self->Id().num_, SimLibElapsedTime);
}

void HeliBrain::CheckOrders (void)
{
   if (underOrders)
   {
      AddMode (FollowOrdersMode);
   }
   else
   {
   }
}

void HeliBrain::FollowOrders (void)
{
float desSpeed;
int turnType;

   if (self->flightLead == self)
      Loiter();

   switch (curOrder)
   {
      case FalconWingmanMsg::WMBreakRight:
      case FalconWingmanMsg::WMBreakLeft:
         trackX = self->XPos() + 5000.0F * (float)cos(headingOrdered);
         trackY = self->YPos() + 5000.0F * (float)sin(headingOrdered);
         desSpeed = CORNER_SPEED;
         turnType = 1;
      break;

      case FalconWingmanMsg::WMPosthole:
         trackX = self->XPos();
         trackY = self->YPos();
         trackZ = 0.0F;
         desSpeed = CORNER_SPEED;
         turnType = 1;
      break;

      case FalconWingmanMsg::WMChainsaw:
      break;

      case FalconWingmanMsg::WMPince:
      break;
   }

   AutoTrack( 100.0f );
}

void HeliBrain::FollowLead (void)
{
	Tpoint newpos={0.0f};
	float  groundZ=0.0F, rng=0.0F;
	HeliBrain *leadBrain=NULL;
	float dx=0.0F, dy=0.0F;

	onStation = NotThereYet;

	if ( self->flightLead )
		leadBrain = (HeliBrain *)self->flightLead->Brain();

   if (self->flightLead == self )
   {
      Loiter();
   }
   else
   {
	  self->GetFormationPos( &newpos.x, &newpos.y, &newpos.z );

      trackX = newpos.x;
      trackY = newpos.y;
      trackZ = newpos.z;
	  dx = newpos.x - self->XPos();
	  dy = newpos.y - self->YPos();
	  rng = dx * dx + dy * dy;

	  if ( self->flightLead &&
		   leadBrain &&
	  	   (leadBrain->onStation == Landing ||
	  	    leadBrain->onStation == PickUp ||
	  	    leadBrain->onStation == DropOff ||
	  	    leadBrain->onStation == Landed ) )
	  {
		   if ( rng < 300.0f * 300.0f )
		   {
			    if (onStation != Landing )
				{
					onStation = Landing;
   					groundZ = OTWDriver.GetGroundLevel(self->XPos(), self->YPos());
				}

	  			if ( self->ZPos() >= groundZ - 5.0f )
				{
					rStick = 0.0f;
					pStick = 0.0f;
					throtl = 0.50f;
				}
				else
				{
					LevelTurn (0.0f, 0.0f, TRUE);
    				throtl = 0.00;
					MachHold(0.0f, 0.0F, FALSE);
				}

		   }
		   else
		   {
	  	   		onStation = NotThereYet;
  	       		AutoTrack( 0.0f );
		   }
	  }
	  else
	  {
			if ( self->OnGround() )
			{
				self->UnSetFlag( ON_GROUND );
			}

	  		onStation = NotThereYet;
  	  		AutoTrack( 0.0f );
	  }
   }
}
