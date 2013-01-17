#include "stdhdr.h"
#include "digi.h"
#include "mesg.h"
#include "simveh.h"
#include "MsgInc\WingmanMsg.h"
#include "find.h"
#include "flight.h"
#include "camp2sim.h"
#include "Aircrft.h"
#include "object.h"
#include "airframe.h"
#include "classtbl.h"
#include "msginc\radiochattermsg.h"
#include "msginc\wingmanmsg.h"
#include "wingorder.h"
#include "otwdrive.h"
/* S.G. 2001-07-30 FOR SimDriver */ #include "Simdrive.h"
/* S.G. 2001-07-30 FOR SimDriver */ #include "FCC.h"
#include "radar.h"		// 2002-02-10 S.G.
#include "campbase.h"	// 2002-02-10 S.G.
#define MANEUVER_DEBUG // MNLOOK
#ifdef MANEUVER_DEBUG
#include "Graphics\include\drawbsp.h"
extern int g_nShowDebugLabels;
extern float g_fAIMinAlt;
#endif
FalconEntity* SpikeCheck (AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL);// 2002-02-10 S.G.

// ----------------------------------------------------
// DigitalBrain::AiPerformManeuver
// ----------------------------------------------------

void DigitalBrain::AiPerformManeuver(void)
{

#ifdef MANEUVER_DEBUG
	char tmpchr[40];
#endif

	switch (mCurrentManeuver)
	{

	case FalconWingmanMsg::WMBreakRight:
	case FalconWingmanMsg::WMBreakLeft:
		AiExecBreakRL();
#ifdef MANEUVER_DEBUG
		sprintf(tmpchr,"%s","AiExecBreakRL");
#endif
		break;

	case FalconWingmanMsg::WMPince:
		AiExecPince();
#ifdef MANEUVER_DEBUG
		sprintf(tmpchr,"%s","AiExecPince");
#endif
		break;

	case FalconWingmanMsg::WMPosthole:
		AiExecPosthole();
#ifdef MANEUVER_DEBUG
		sprintf(tmpchr,"%s","AiExecPosthole");
#endif
		break;

	case FalconWingmanMsg::WMChainsaw:
		AiExecChainsaw();
#ifdef MANEUVER_DEBUG
		sprintf(tmpchr,"%s","AiExecChainsaw");
#endif
		break;

	case FalconWingmanMsg::WMFlex:
		AiExecFlex();
#ifdef MANEUVER_DEBUG
		sprintf(tmpchr,"%s","AiExecFlex");
#endif
		break;

	case FalconWingmanMsg::WMClearSix:
		AiExecClearSix();
#ifdef MANEUVER_DEBUG
		sprintf(tmpchr,"%s","AiExecClearSix");
#endif
		break;
	}
#ifdef MANEUVER_DEBUG
	if ((g_nShowDebugLabels & 0x08) || (g_nShowDebugLabels & 0x400000))
	{
		if (g_nShowDebugLabels & 0x40)
		{
		  RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);
		  if (theRadar) {
			  if (theRadar->digiRadarMode = RadarClass::DigiSTT)
				strcat(tmpchr, " STT");
  			  else if (theRadar->digiRadarMode = RadarClass::DigiSAM)
				  strcat(tmpchr, " SAM");
			  else if (theRadar->digiRadarMode = RadarClass::DigiTWS)
				  strcat(tmpchr, " TWS");
			  else if (theRadar->digiRadarMode = RadarClass::DigiRWS)
				  strcat(tmpchr, " RWS");
			  else if (theRadar->digiRadarMode = RadarClass::DigiOFF)
				strcat(tmpchr, "%s OFF");
			  else strcat(tmpchr, " UNKNOWN");
		  }
		}
   		 
		if (g_nShowDebugLabels & 0x8000)
		{
			 if (((AircraftClass*) self)->af->GetSimpleMode())
				strcat(tmpchr, " SIMP");
			 else
				strcat(tmpchr, " COMP");
		}

		if ( self->drawPointer )
				((DrawableBSP*)self->drawPointer)->SetLabel (tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
	}
#endif 
}



//--------------------------------------------------
// DigitalBrain::AiMonitorTargets
//--------------------------------------------------

void DigitalBrain::AiMonitorTargets()
{
	FalconEntity*	pbaseData;
	int				campX, campY;

// 2000-09-13 MODIFIED BY S.G. HOW STUPID CAN YOU BE! WHO PROGRAMMED THIS HAS NO IDEA HOW ARRAY WORKS! NOT IN RP4
// Cobra - The pot calling the kettle black!
// mpSearchFlags array is AI_TOTAL_SEARCH_TYPES (3) long, thus mpSearchFlags[AI_TOTAL_SEARCH_TYPES] is referencing
// memory outside of the array (mpSearchFlags[3]). AI_FIXATE_ON_TARGET is an index into mpSearchFlags[], not the value (true or false).
// so, whatever the value is in mpSearchFlags[AI_TOTAL_SEARCH_TYPES] must equal AI_FIXATE_ON_TARGET (2)...not likely!
// The same is true for the other 2 if(mpSearchFlags[AI_TOTAL_SEARCH_TYPES] == AI_xxxxxx_TARGET) logic statements.
	//if(mpSearchFlags[AI_TOTAL_SEARCH_TYPES] == AI_FIXATE_ON_TARGET) {
	if(mpSearchFlags[AI_FIXATE_ON_TARGET]) {
		return;
	}
// 2000-09-28 MODIFIED BY S.G. PLUS WE DO THIS ONLY FOR THE PLAYER'S FLIGHT. NOT IN RP4
	// Cobra - see above
//	else if(mpSearchFlags[AI_TOTAL_SEARCH_TYPES] == AI_MONITOR_TARGET) {
	else if(mpSearchFlags[AI_MONITOR_TARGET]) {
//	else if(mpSearchFlags[AI_MONITOR_TARGET] && flightLead->IsSetFlag(MOTION_OWNSHIP)) {
		if(targetPtr && vuxGameTime > mLastReportTime + 120000) {

			mLastReportTime = vuxGameTime;
			pbaseData		= targetPtr->BaseData();
			campY			= SimToGrid(pbaseData->XPos());
			campX			= SimToGrid(pbaseData->YPos());

			// play radio message about target's position
/*// 2000-09-28 ADDED BY S.G. SO THE FUNCTION IS COMPLETE. NOT IN RP4
			short edata[10];
			int numAircraft = 0;

			edata[0]	= self->GetCampaignObject()->GetComponentIndex(self);
			edata[1]	= 0;

//			numAircraft = pbaseData->GetCampaignObject()->NumberOfComponents();
			//for this request we just want the BRA part
			if(numAircraft > 1)
				edata[4] = (short)((pbaseData->Type() - VU_LAST_ENTITY_TYPE)*2 + 1);	//type
			else
				edata[4] = (short)((pbaseData->Type() - VU_LAST_ENTITY_TYPE)*2);	//type
			edata[5] = (short)numAircraft;	//number
			edata[6] = (short) campX;;
			edata[7] = (short) campY;
			edata[8] = (short) -pbaseData->ZPos();
					
			AiMakeRadioResponse( self, rcNEARESTTHREATRSP, edata );
	//END OF ADDED SECTION
*/		}
	}
// 2000-09-13 MODIFIED BY S.G. HOW STUPID CAN YOU BE! WHO PROGRAMMED THIS HAS NO IDEA HOW ARRAY WORKS! NOT IN RP4
	// Cobra - see above
	//else if(mpSearchFlags[AI_TOTAL_SEARCH_TYPES] == AI_SEARCH_FOR_TARGET) {
	else if(mpSearchFlags[AI_SEARCH_FOR_TARGET]) {
		if(targetPtr && targetPtr != mpLastTargetPtr && vuxGameTime > (mLastReportTime + 120000)) {

			mLastReportTime	= vuxGameTime;
			pbaseData			= targetPtr->BaseData();
			mpLastTargetPtr	= targetPtr;
			campY					= SimToGrid(pbaseData->XPos());
			campX					= SimToGrid(pbaseData->YPos());

			// play radio message about target's postion
		}
	}
}



void DigitalBrain::AiSetInPosition(void)
{
	int vehInFlight;
	int flightIdx;

	// Get wingman slot position relative to the leader
	vehInFlight		= ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
	flightIdx		= ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

	if(flightIdx == AiElementLead && vehInFlight == 4)  {
		mInPositionFlag = FALSE;
	}
}


//--------------------------------------------------
// DigitalBrain::AiCheckPlayerInPosition
//
//	Yeah this is an odd place to put this function since it only gets called when the
// AI is a human player.  However, the digi has the necessary data.
//--------------------------------------------------

void DigitalBrain::AiCheckPlayerInPosition(void)
{
	float xdiff, ydiff, zdiff;
	float trkX, trkY, trkZ;
	int vehInFlight;
	int flightIdx;
	ACFormationData::PositionData* curPosition;
	AircraftClass* paircraft;
	float rangeFactor;

	// Get wingman slot position relative to the leader
	vehInFlight		= ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
	flightIdx		= ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

	if(flightIdx == AiElementLead && vehInFlight == 4)  {
		curPosition = &(acFormationData->positionData[mFormation][flightIdx - 1]);
		paircraft	= (AircraftClass*) flightLead;

		ShiAssert(paircraft);
		if (paircraft)
		{
			// Get my leader's position
			trkX	= paircraft->XPos();
			trkY	= paircraft->YPos();
			trkZ	= paircraft->ZPos();

			rangeFactor		= curPosition->range * (2.0F * mFormLateralSpaceFactor);

			// Calculate position relative to the leader
			trkX	+= rangeFactor * (float)cos(curPosition->relAz * mFormSide + paircraft->af->sigma);
			trkY	+= rangeFactor * (float)sin(curPosition->relAz * mFormSide + paircraft->af->sigma);		
		}

		if(curPosition->relEl) {
			trkZ	+= rangeFactor * (float)sin(-curPosition->relEl);
		}
		else {
			trkZ += (flightIdx * -100.0F);
		}

		xdiff = trkX - self->XPos();
		ydiff = trkY - self->YPos();
		zdiff = trkZ - self->ZPos();

		if((xdiff * xdiff + ydiff + ydiff <  2000.0F * 2000.0F) && fabs(zdiff) < 500.0F && mInPositionFlag == FALSE) {	
			mInPositionFlag = TRUE;
			AiMakeCommandMsg( (SimBaseClass*) self, FalconWingmanMsg::WMGlue, AiWingman, FalconNullId);
		}
		else if(((xdiff * xdiff + ydiff + ydiff >  2500.0F * 2500.0F) || fabs(zdiff) > 3000.0F) && mInPositionFlag == TRUE) {	
			mInPositionFlag = FALSE;
			AiMakeCommandMsg( (SimBaseClass*) self, FalconWingmanMsg::WMSplit, AiWingman, FalconNullId);
		}
	}
}

//--------------------------------------------------
// DigitalBrain::AiFollowLead
//--------------------------------------------------

void DigitalBrain::AiFollowLead(void)
{
	ACFormationData::PositionData *curPosition;
	float				rangeFactor;
	float				groundZ;
	int				vehInFlight;
	int				flightIdx;
	AircraftClass* paircraft;

   if (flightLead == self || isWing == 0 || !flightLead) {

      Loiter();
	}
   else {

		// Get wingman slot position relative to the leader
		vehInFlight		= ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
		flightIdx		= ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

		if(flightIdx == AiFirstWing && vehInFlight == 2)  {
			curPosition	= &(acFormationData->twoposData[mFormation]);	// The four ship #2 slot position is copied in to the 2 ship formation array.
			paircraft	= (AircraftClass*) flightLead;
		}
		else if(flightIdx == AiSecondWing && mSplitFlight) {
	      curPosition = &(acFormationData->twoposData[mFormation]);
			paircraft	= (AircraftClass*) ((FlightClass*)self->GetCampaignObject())->GetComponentEntity(AiElementLead);
		}
		else {
			curPosition = &(acFormationData->positionData[mFormation][flightIdx - 1]);
			paircraft	= (AircraftClass*) flightLead;
		}

	   rangeFactor		= curPosition->range * (2.0F * mFormLateralSpaceFactor);

		// Get my leader's position
		ShiAssert(paircraft);
		if (paircraft)
		{
      trackX	= paircraft->XPos();
      trackY	= paircraft->YPos();
      trackZ	= paircraft->ZPos();

			// Calculate position relative to the leader
			trackX	+= rangeFactor * (float)cos(curPosition->relAz * mFormSide + paircraft->af->sigma);
			trackY	+= rangeFactor * (float)sin(curPosition->relAz * mFormSide + paircraft->af->sigma);		

			if(curPosition->relEl) {
				trackZ	+= rangeFactor * (float)sin(-curPosition->relEl);
			}
			else {
				trackZ += (flightIdx * -100.0F);
			}

			AiCheckInPositionCall(trackX, trackY, trackZ);

			// add relative formation altitude - after check in position call !
			if (isWing)	// only wingmen
				trackZ = trackZ + mFormRelativeAltitude;

			// Set track point 1NM ahead of desired location
			trackX	+= 1.0F * NM_TO_FT * (float)cos((paircraft)->af->sigma);
			trackY	+= 1.0F * NM_TO_FT * (float)sin((paircraft)->af->sigma);
		}

		// check for terrain following
    	groundZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
										   		  self->YPos() + self->YDelta() );
		if ( self->ZPos() - groundZ > -1000.0f )
		{
			// Cobra - externalize AI min alt - Default is 500', was 800'
			if ( trackZ - groundZ > -g_fAIMinAlt )
			{
				if ( self->ZPos() - groundZ > -g_fAIMinAlt )
					trackZ = groundZ - g_fAIMinAlt - ( self->ZPos() - groundZ + g_fAIMinAlt ) * 2.0f;
				else
					trackZ = groundZ - g_fAIMinAlt;
			}
		}

		SimpleTrack(SimpleTrackDist, 0.0F);

// 2001-06-06 ADDED BY S.G. WINGY DON'T UPDATE THEIR CURRENT WAYPOINT. BECAUSE OF THIS, THE PLAYER'S WINGY NEVER LOOK AT THEIR TARGET WAYPOINT FOR TARGETS...
		// I'm going to have the AI use the leads waypoint. I'm assuming the lead and the wingmen have the same number of waypoints here...
		WayPointClass* wlistUs   = self->waypoint;
		WayPointClass* wlistLead = NULL;
		if (flightLead)
			wlistLead = ((AircraftClass *)flightLead)->waypoint;

		UnitClass *campUnit = NULL;
		WayPointClass *campCurWP = NULL;
		int waypointIndex = 0;

		// This will set our current waypoint to the leads waypoint
// 2001-10-20 M.N. Added ->GetNextWP() to while (...) to assure a valid WP is chosen
		while (wlistUs->GetNextWP() && wlistLead && wlistLead->GetNextWP() && wlistLead != ((AircraftClass *)flightLead)->curWaypoint) {
			wlistUs   = wlistUs->GetNextWP();
			wlistLead = wlistLead->GetNextWP();
			waypointIndex++;
		}

		// 2001-07-28 S.G. Only do it if it changed and I need to update our ICP wp index as well or it will get screwed up if we change it manually.
		if (self->curWaypoint != wlistUs) {
			self->curWaypoint = wlistUs;

			if (self == SimDriver.GetPlayerEntity())
				self->FCC->SetWaypointNum (waypointIndex);
		}

		// Then this will set the unit's waypoint to wing #1 waypoint (only needed once and only if not current)
		if (isWing == 1) {
			waypointIndex++; // Unit's waypoint number starts at 1, not 0.
			campUnit = (UnitClass *)self->GetCampaignObject();

			if ( campUnit ) // sanity check
				// Only do this if our waypoint has changed
				if (campUnit->GetCurrentWaypoint() != waypointIndex)
					campUnit->SetCurrentWaypoint(waypointIndex);
		}
// END OF ADDED SECTION
	}
}





// ----------------------------------------------------
// DigitalBrain::AiExecBreakRL
// ----------------------------------------------------

void DigitalBrain::AiExecBreakRL(void)
{
	mlTrig				trig;
	short					edata[10];
	VU_ID					pthreat;

	mlSinCos (&trig, mHeadingOrdered);
	SetTrackPoint(self->XPos() + 1000.0F * trig.cos, self->YPos() + 1000.0F * trig.sin, mAltitudeOrdered);

	mSpeedOrdered *= 1.001F;
	TrackPoint(9.0F, mSpeedOrdered);

	mnverTime -= SimLibMajorFrameTime;
	if(mnverTime <= 0.0F) {

		// Anounce that I'm ending the maneuver
		if(mCurrentManeuver == FalconWingmanMsg::WMBreakRight ||
			mCurrentManeuver == FalconWingmanMsg::WMBreakLeft) {
		
			AiSaveSetSearchDomain(DOMAIN_AIR);
			pthreat = AiCheckForThreat(self, DOMAIN_AIR, 0);
			AiRestoreSearchDomain();
			
			if(pthreat == FalconNullId) {
				edata[0]	= self->GetCampaignObject()->GetComponentIndex(self);
				edata[1]	= 0;
   			AiMakeRadioResponse( self, rcGENERALRESPONSEC, edata );
			}
			else {
				AiEngageThreatAtSix(pthreat);
			}
		}

		AiClearManeuver();
	}
}


// ----------------------------------------------------
// DigitalBrain::AiExecPosthole
// ----------------------------------------------------

void DigitalBrain::AiExecPosthole(void)
{
   mpActionFlags[AI_USE_COMPLEX] = TRUE;
   if (mPointCounter == 0)
   {
      TrackPoint(9.0F, mSpeedOrdered);
      if (fabs(self->ZPos() - trackZ) < 1000.0F)
         mPointCounter = 1;
   }
   else
   {
      if (targetPtr)
      {
         if (curMissile)
         {
		      FireControl();
            MissileEngage();
         }
         else
         {
            GunsEngage();
         }
      }
      else
      {
         AiRejoin(NULL);
      }
   }
}

// ----------------------------------------------------
// DigitalBrain::AiExecChainsaw
// ----------------------------------------------------
void DigitalBrain::AiExecChainsaw(void)
{
   if (targetPtr && curMissile)
   {
      mpActionFlags[AI_USE_COMPLEX] = TRUE;
		FireControl();
      MissileEngage();
   }
   else
   {
      AiRejoin(NULL);
   }
}

// ----------------------------------------------------
// DigitalBrain::AiExecPince
// ----------------------------------------------------

void DigitalBrain::AiExecPince(void)
{
	float dx, dy;
	float	deltaSq;

	dx			= trackX - self->XPos();
	dy			= trackY - self->YPos();
	deltaSq	= dx * dx + dy * dy;

// S.G. 1000 feet is too short. I'll make this 5000.0. This will prevent AI from turning around its maneuver point, never to reach it...
//	if(deltaSq <= 1000.0F * 1000.0F) {
	if(deltaSq <= 5000.0F * 5000.0F) {
		mPointCounter++;
	}

	if(mPointCounter >= 2) {
		AiClearManeuver();
		mPointCounter = 0;
	}

	trackX	= mpManeuverPoints[mPointCounter][0];
	trackY	= mpManeuverPoints[mPointCounter][1];
	trackZ	= mAltitudeOrdered;

	mSpeedOrdered *= 1.001F;

	TrackPoint(4.5F, mSpeedOrdered);
}

// ----------------------------------------------------
// DigitalBrain::AiExecFlex
// ----------------------------------------------------

void DigitalBrain::AiExecFlex(void)
{
	float dx, dy;
	float	deltaSq;

	dx			= trackX - self->XPos();
	dy			= trackY - self->YPos();
	deltaSq	= dx * dx + dy * dy;

	if(deltaSq <= 900.0F * 900.0F) {
		mPointCounter++;
	}

	if(mPointCounter >= TOTAL_MANEUVER_PTS) {
		AiClearManeuver();
	}

	trackX	= mpManeuverPoints[mPointCounter][0];
	trackY	= mpManeuverPoints[mPointCounter][1];
	trackZ	= mAltitudeOrdered;

	mSpeedOrdered *= 1.001F;

	TrackPoint(4.5F, mSpeedOrdered);
}





// ----------------------------------------------------
// DigitalBrain::AiExecClearSix
// ----------------------------------------------------

void DigitalBrain::AiExecClearSix(void)
{
	mlTrig				trig;
	short					edata[10];

	mlSinCos (&trig, mHeadingOrdered);
	SetTrackPoint(self->XPos() + 1000.0F * trig.cos, self->YPos() + 1000.0F * trig.sin, mAltitudeOrdered);

	mSpeedOrdered *= 1.001F;
	TrackPoint(9.0F, mSpeedOrdered);

	mnverTime -= SimLibMajorFrameTime;

	if(mnverTime <= 0.0F) {	
		AiRestoreSearchDomain();
		AiClearManeuver();

		edata[0]	= self->GetCampaignObject()->GetComponentIndex(self);
		edata[1]	= 0;
   	AiMakeRadioResponse( self, rcGENERALRESPONSEC, edata );
	}
}

