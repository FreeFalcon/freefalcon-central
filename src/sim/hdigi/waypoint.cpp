#include "stdhdr.h"
#include "hdigi.h"
#include "otwdrive.h"
#include "pilotinputs.h"
#include "campwp.h"
#include "simveh.h"
#include "fcc.h"
#include "unit.h"
#include "helimm.h"

// Brain Choices
#define GENERIC_BRAIN     0
#define SEAD_BRAIN        1
#define STRIKE_BRAIN      2
#define INTERCEPT_BRAIN   3
#define AIR_CAP_BRAIN     4
#define AIR_SWEEP_BRAIN   5
#define ESCORT_BRAIN      6
#define WAYPOINTER_BRAIN  7

void HeliBrain::FollowWaypoints(void)
{
   // int lastPickle;


   if (self->curWaypoint == NULL)
   {
      AddMode (LoiterMode);
      return;
   }

   GoToCurrentWaypoint();


   /*
   switch (self->curWaypoint->GetWPAction())
   {
      case WP_LAND:
	     if ( onStation == Arrived || onStation == Stabilizing )
		 	LandMe();
		else if ( !(onStation == OnStation) )
         	GoToCurrentWaypoint();
      break;

      default:
         GoToCurrentWaypoint();
      break;
   }
   */
}

void HeliBrain::GoToCurrentWaypoint(void)
{
float rng, desHeading;
float rollLoad;
float rollDir;
float desSpeed;
float wpX, wpY, wpZ;
float dx, dy, time;

   if ( self->curWaypoint->GetWPAction() == WP_PICKUP && onStation == NotThereYet)
   {
		Unit cargo = (Unit) self->curWaypoint->GetWPTarget();
		if ( cargo )
		{
			wpX = cargo->XPos();
			wpY = cargo->YPos();
			wpZ = cargo->ZPos();
		}
		else
		{
   			self->curWaypoint->GetLocation (&wpX, &wpY, &wpZ);
		}
   }
   else
   {
   		self->curWaypoint->GetLocation (&wpX, &wpY, &wpZ);
   }

   // follow terrain at 1000ft
   holdAlt = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
    						   self->YPos() + self->YDelta()) - 1000.0f;

   desSpeed = 1.0f;
   rollDir = 0.0f;
   rollLoad = 0.0f;

   if (curMode != lastMode)
   {
      onStation = NotThereYet;
   }

	/*---------------------------*/
	/* Range to current waypoint */
	/*---------------------------*/
	dx = (wpX - self->XPos());
	dy = (wpY - self->YPos());
	rng = dx * dx + dy * dy;

	/*------------------------------------*/
	/* Heading error for current waypoint */
	/*------------------------------------*/
	desHeading = (float)atan2 (dy, dx) - self->Yaw();
	if (desHeading > 180.0F * DTR)
		desHeading -= 360.0F * DTR;
	else if (desHeading < -180.0F * DTR)
		desHeading += 360.0F * DTR;

	// rollLoad is normalized (0-1) factor of how far off-heading we are
	// to target
	rollLoad = desHeading / (180.0F * DTR);
	if (rollLoad < 0.0F)
		rollLoad = -rollLoad;
	if ( desHeading > 0.0f )
		rollDir = 1.0f;
	else
		rollDir = -1.0f;

	//MonoPrint ("%8.2f %8.2f\n", desHeading * RTD, desLoad);

	/*---------------------------*/
	/* Reached the next waypoint? */
	/*---------------------------*/
	if (rng < (600.0F * 600.0F) || (onStation != NotThereYet) ||
		SimLibElapsedTime > self->curWaypoint->GetWPDepartureTime())
	{
		if (onStation == NotThereYet)
		{
			onStation = Arrived;
		}
		else if ( onStation == OnStation &&
			      SimLibElapsedTime > self->curWaypoint->GetWPDepartureTime())
		{
         	SelectNextWaypoint();
		}
	}

	// landing?
	if ( onStation == Landing ||
		 onStation == DropOff ||
		 onStation == Landed ||
		 onStation == PickUp)
	{
		LandMe();
		return;
	}


	/*--------------*/
	/* On Station ? */
	/*--------------*/
	if (onStation == Arrived )
	{
   		if ( self->curWaypoint->GetWPFlags() & WPF_LAND )
		{
			LandMe();
			return;
		}
		onStation = OnStation;
	}

	// get waypoint speed based on our dist and arrival time
	if ( rng < 600.0f * 600.0f )
	{
		rollLoad = 0.0f;
		desSpeed = 0.0f;
	}
	else
	{
		rng = (float)sqrt( rng );
		if (self->curWaypoint->GetWPArrivalTime() > SimLibElapsedTime) 
			time =  (float)(self->curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC;
		else
			time = -1.0f;
	
		if ( time <= 0.0f )
		{
			// we're late
			desSpeed = 1.0f;
	
		}
		else
		{
			desSpeed = (rng/time)/MAX_HELI_FPS;
			if ( desSpeed > 1.0f )
				desSpeed = 1.0f;
		}
	}

	if ( self->OnGround() )
	{
		self->UnSetFlag( ON_GROUND );
	}

	// if we're close, just point to spot then go
	if ( fabs(rollLoad) > 0.1f && rng < 1000.0f * 1000.0f )
		desSpeed = 0.0f;

	LevelTurn (rollLoad, rollDir, TRUE);
    AltitudeHold(holdAlt);
	MachHold(desSpeed, 0.0F, FALSE);
}

void HeliBrain::SelectNextWaypoint(void)
{
	WayPointClass* tmpWaypoint = self->curWaypoint;
	WayPointClass* wlist = self->waypoint;
	UnitClass *campUnit = NULL;
	WayPointClass *campCurWP = NULL;
	int waypointIndex,i;

	// first get our current waypoint index in the list
	for ( waypointIndex = 0;
		  wlist && wlist != tmpWaypoint;
		  wlist = wlist->GetNextWP(), waypointIndex++ );

	// see if we're running in tactical or campaign.  If so, we want to
	// synch the campaign's waypoints with ours
	// if ( SimDriver.RunningCampaignOrTactical() )
	{
		// get the pointer to our campaign unit
		campUnit = (UnitClass *)self->GetCampaignObject();

		if ( campUnit ) // sanity check
		{
			campCurWP = campUnit->GetFirstUnitWP();

			// now get the camp waypoint that corresponds to our next in the
			// list by index
			for ( i = 0; i <= waypointIndex; i++ )
			{
				if ( campCurWP ) // sanity check
					campCurWP = campCurWP->GetNextWP();
			}
		}
	}

	onStation = NotThereYet;
	self->curWaypoint = self->curWaypoint->GetNextWP();

	// KCK: This isn't working anyway - so I'm commentting it out in order to prevent bugs
	// in the ATC and Campaign
	// edg: this should be OK now that an obsolute waypoint index is used to
	// synch the current WP between sim and camp units.
	if ( campCurWP )
	{
		campUnit->SetCurrentUnitWP( campCurWP );
	}
   	waypointMode = 0;

   	if (!self->curWaypoint)
	{
		// go back to the beginning
      	self->curWaypoint = self->waypoint;
		campUnit->SetCurrentUnitWP( campUnit->GetFirstUnitWP() );
	}

	if ( self->OnGround() )
	{
		self->UnSetFlag( ON_GROUND );
		holdAlt -= 100.0f;
	}

   	ChooseBrain();
}

void HeliBrain::ChooseBrain(void)
{
   if (self->curWaypoint)
   {
      switch (self->curWaypoint->GetWPAction())
      {
         case WP_NOTHING:
         case WP_TAKEOFF:
         case WP_ASSEMBLE:
         case WP_POSTASSEMBLE:
         case WP_REFUEL:
         case WP_REARM:
         case WP_LAND:
         case WP_ELINT:
         case WP_RECON:
         case WP_RESCUE:
         case WP_ASW:
         case WP_TANKER:
         case WP_AIRDROP:
         case WP_JAM:
         case WP_PICKUP:
            // MonoPrint ("Helo Digi Chose Waypoint BRAIN\n");
//            MonoPrint ("Helo Digi Chose WAYPOINT BRAIN\n");
            // modeData = digitalBrains->brainData[AIR_SWEEP_BRAIN];
         break;

         case WP_ESCORT:
//            MonoPrint ("Helo Digi Chose ESCORT BRAIN\n");
         break;

         case WP_CA:
//            MonoPrint ("Helo Digi Chose AIR SWEEP BRAIN\n");
         break;

         case WP_CAP:
//            MonoPrint ("Helo Digi Chose AIR CAP BRAIN\n");
         break;

         case WP_INTERCEPT:
//            MonoPrint ("Helo Digi Chose AIR INTERCEPT BRAIN\n");
         break;

         case WP_GNDSTRIKE:
         case WP_NAVSTRIKE:
         case WP_STRIKE:
         case WP_BOMB:
         case WP_SAD:
//            MonoPrint ("Helo Digi Chose STRIKE BRAIN\n");
         break;

         case WP_SEAD:
//            MonoPrint ("Helo Digi Chose SEAD BRAIN\n");
         break;

         default:
//            MonoPrint ("Why am I here (Helo Digi GetBrain)\n");
  //          MonoPrint ("===>Waypoint action %d\n", self->curWaypoint->GetWPAction());
         break;
      }
   }
   else
   {
   }

}
