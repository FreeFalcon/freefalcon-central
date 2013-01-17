#include "stdhdr.h"
#include "Object.h"
#include "simmover.h"
#include "camp2sim.h"
#include "team.h"
#include "MsgInc\TrackMsg.h"
#include "RadarDigi.h"
#include "campbase.h"
#include "simmath.h"
#include "Graphics\Include\drawbsp.h" // 2002-02-26 S.G.
#include "aircrft.h"

extern int g_nShowDebugLabels; // 2002-02-26 S.G.

void CalcRelGeom (SimBaseClass* ownObject, SimObjectType* targetList, TransformMatrix vmat, float elapsedTimeInverse);

RadarDigiClass::RadarDigiClass (int type, SimMoverClass* parentPlatform) : RadarClass(type, parentPlatform)
{
	mode = AA;
	NewRange( 20.0f );
	SetSeekerPos( 0.0f, 0.0f );
	platform->SetRdrRng( 0.0F );
	platform->SetRdrAz( 0.0F );
	platform->SetRdrEl( 0.0F );
	platform->SetRdrCycleTime( 3.0F );
	platform->SetRdrAzCenter( 0.0f );
	platform->SetRdrElCenter( 0.0f );
}

#define SG_NOLOCK 0x00
#define SG_LOCK 0x01
#define SG_JAMMING 0x02
#define SG_FADING 0x04

SimObjectType* RadarDigiClass::Exec (SimObjectType* targetList)
{
	int				sendThisFrame;
#define SAMDEBUG
#ifdef SAMDEBUG
	char label[80] = "               ";
#endif

	// Validate our locked target
	CheckLockedTarget();

	// Don't do anything if no emitting
	if (!isEmitting)
	{
#ifdef SAMDEBUG
		if (g_nShowDebugLabels & 0x200) {
			if (platform->drawPointer)
				((DrawableBSP*)platform->drawPointer)->SetLabel(
					"Not emitting",((DrawableBSP*)platform->drawPointer)->LabelColor()
				);
			else
				((DrawableBSP*)platform->drawPointer)->SetLabel(
					"            ",((DrawableBSP*)platform->drawPointer)->LabelColor()
				);
		}
#endif
		SetDesiredTarget( NULL );
		flag &= ~FirstSweep; // 2002-03-10 ADDED BY S.G. Say we have done our first radar sweep
		return NULL;
	}

	// Weight on wheels inhibit
	if (platform->IsAirplane()){
		if (platform->OnGround()){
			SetSeekerPos( 0.0f, 0.0f );
			platform->SetRdrRng( 0.0F );
			platform->SetRdrAz( 0.0F );
			platform->SetRdrEl( 0.0F );
			platform->SetRdrCycleTime( 9999.0F );//me123
			platform->SetRdrAzCenter( 0.0f );
			platform->SetRdrElCenter( 0.0f );
			SetDesiredTarget( NULL );
			flag &= ~FirstSweep; // 2002-03-10 ADDED BY S.G. Say we have done our first radar sweep
			return NULL;
		}

		// Original code check for isEmiting again, its useless since we return if not emiting above anyway
		platform->SetRdrRng( radarData->NominalRange );
		platform->SetRdrAz( radarData->ScanHalfAngle );
		platform->SetRdrEl( radarData->ScanHalfAngle );
		platform->SetRdrCycleTime( 8.0F );//me123
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}

	// See if it is time to send a "painted" list update
	if (mode == AA) {
		sendThisFrame = (SimLibElapsedTime - lastTargetLockSend > TrackUpdateTime);
	} else {
		sendThisFrame = FALSE;
	}

	// S.G. Now this is where we scan for each target in our target list, a la EyeballClass::Exec
	SimObjectType* tmpPtr = targetList;

	// Just in case we don't have a list but we do have a locked target OR IF THE RADAR IS NOT IN AA MODE
	// I noticed the radar isn't really used in air to ground mode so we'll do just the lockedTarget then
	if (!tmpPtr || mode != AA)
		tmpPtr = lockedTarget;

	while (tmpPtr) {
		// S.G. canSee has THREE meaning now.
		// Bit 0 is when we can see signal, bit 1 is when the signal is jammed and 
		// bit 2 is when the signal is fading
		// By default, we can see the target (bit 0 at value 1)
		int canSee = SG_LOCK;

		if (TeamInfo[platform->GetTeam()]->TStance(tmpPtr->BaseData()->GetTeam()) <= Neutral){
			//me123 don't lock up freindlys
			canSee = SG_NOLOCK;
		}
		// Can't hold a lock if its outside our radar cone
		if (fabs(tmpPtr->localData->ata) > radarData->ScanHalfAngle){
			canSee = SG_NOLOCK;
		}
		else {
			// Only track object in the correct domain (air/land)
			if ( tmpPtr->BaseData()->OnGround() ){
				if (mode == AA){
					// Skip ground objects in AA mode
					canSee = SG_NOLOCK;
				}
			}
			else {
				// Ground mode is more than just GM, 
				// it can be GMT and SEA as well (not sure about SEA though, 
				// but gndAttck.cpp DO put the digital radar in GMT mode)
				// But AFAIK, if NOT in AA, you're in GM or GMT mode (SEA is NOT used by AI after all)
				if (mode != AA) {
					// Skip air objects in AA mode
					canSee = SG_NOLOCK;
				}
			}
		}

		// Now we check the signal strength only if the target is in the radar cone and the right mode
		float ret;
		if (canSee) {
			// Drop lock if the guy has been below the signal strength threshhold too long
			// 2001-05-14 MODIFIED BY S.G. IF ReturnStrength RETURNED -1.0f,	 
			// THEN IT MEANS WE HAVE NO LOS ON THE TARGET. DON'T SAY IT'S JAMMING
			ret = ReturnStrength(tmpPtr);
			if (ret < 1.0f) {
				// Ok so it's too low, but is it jamming? If so, follow anyway...
				if (ret != -1.0f && tmpPtr->BaseData()->IsSPJamming())
					canSee |= SG_JAMMING; // That's our second bit being used
				// So it's too low and were are not jamming. When did we loose the signal?
				else if (SimLibElapsedTime - tmpPtr->localData->rdrLastHit > radarData->CoastTime) {
					// Give up and drop lock
					canSee = SG_NOLOCK;
				}
				// We just lost the signal, but we can still follow it, right?
				else
					canSee |= SG_FADING;
			}
		}

		// Now set that target sensor track according to what we got
		if (canSee) {
			
			//me123 first time we atempt a lock, it requires some time to lock it up
			if (tmpPtr->localData->sensorState[Radar] == NoTrack){
				// we are starting to lock the guy
				tmpPtr->localData->rdrLastHit = SimLibElapsedTime;
				// 2002-02-10 ADDED BY S.G. SensorFusion uses that field
				tmpPtr->localData->sensorLoopCount[Radar] = SimLibElapsedTime;
				// 2002-03-21 ADDED BY S.G. When a radar is doing its first sweep 
				// after creation, don't fade the signal or the SARH missile launched
				// by an aggregated battalion that just deaggregated will lose its sensor lock
				if (!(flag & FirstSweep))
					canSee |= SG_FADING; // this will make the sensor state max set to detection
			}

			// 2002-03-10 ADDED BY S.G. Added the "(flag & FirstSweep) && "
			// so when a radar is doing its first sweep after creation, 
			// don't fade the signal or the SARH missile launched 
			// by an aggregated battalion that just deaggregated will lose its sensor lock
			if ((flag & FirstSweep) && radarDatFile) {
				tmpPtr->localData->rdrLastHit = SimLibElapsedTime - (unsigned)radarDatFile->TimeToLock - 1;
			}
			// END OF ADDED SECTION 2002-03-10

			if (
				radarDatFile && 
				tmpPtr->localData->sensorState[Radar] == Detection &&  
				SimLibElapsedTime - tmpPtr->localData->rdrLastHit < (unsigned)radarDatFile->TimeToLock
			){
				canSee |= SG_FADING;// we are attempting a lock so don't go higher then detection
			}

			// Can we see it (either with a valid lock, a jammed or fading signal?
			if (canSee & (SG_JAMMING | SG_FADING))						// Is it a jammed or fading signal?
				// Yep, say so (weapon can't lock on 'Detection' but digi plane can track it)
				tmpPtr->localData->sensorState[Radar] = Detection;			
			else
				// It's a valid lock, mark it as such. Even when fading, we can launch
				tmpPtr->localData->sensorState[Radar] = SensorTrack;		

			if (!(canSee & SG_FADING)) {									// Is the signal fading?
				// No, so update the last hit field
				tmpPtr->localData->rdrLastHit = SimLibElapsedTime;
				 // 2002-02-10 ADDED BY S.G. SensorFusion uses that field
				tmpPtr->localData->sensorLoopCount[Radar] = SimLibElapsedTime;
			}
		}
		else
			tmpPtr->localData->sensorState[Radar] = NoTrack;				// Sorry, we lost that target...

		// 2000-10-07 S.G. POSSIBLE BUG! 
		// If we are looking at our lockedTarget and we are the only one referencing it, clearing it
		// might invalidate tmpPtr->next. So I'll read it ahead of time
		SimObjectType* tmpPtrNext = tmpPtr->next;

		// Now is it time to do lockedTarget housekeeping stuff?
		// 2000-09-18 S.G. We need to check the base data because 
		// targetList item and lockedData might not be the same
		if (lockedTarget && tmpPtr->BaseData() == lockedTarget->BaseData()) {
#ifdef SAMDEBUG
			if (g_nShowDebugLabels & 0x400)
			{
				sprintf(label, "%04.3f", ret);
				if (canSee & SG_JAMMING)
					strcat(label, "Jamming");
				if (canSee & SG_FADING)
					strcat(label, "Fading");
				if (canSee & SG_NOLOCK)
					strcat(label, "No lock");
				if (platform->drawPointer)
					((DrawableBSP*)platform->drawPointer)->SetLabel(
						label,((DrawableBSP*)platform->drawPointer)->LabelColor()
					);
			}
#endif

			// 2000-09-18 S.G. Update the lockedTarget radar sensor state with what we just calculated.
			lockedTarget->localData->sensorState[Radar] = tmpPtr->localData->sensorState[Radar];

			// If we can see our target, Tell the base class and the rest of the world 
			// where we're looking (if we are looking somewhere)
			if (canSee) {
				SetSeekerPos( TargetAz(platform, tmpPtr), TargetEl(platform, tmpPtr) );
				platform->SetRdrAz( radarData->BeamHalfAngle );
				platform->SetRdrEl( radarData->BeamHalfAngle );
				// 2002-02-10 MODIFIED BY S.G. Different radar cycle timer for different radar mode
				if (digiRadarMode == DigiSTT)
					platform->SetRdrCycleTime( 0.5F ); // Original line
				else if (digiRadarMode == DigiSAM)
					platform->SetRdrCycleTime( 3.0F );
				else if (digiRadarMode == DigiTWS)
					platform->SetRdrCycleTime( 3.0F );
				else
					platform->SetRdrCycleTime( 6.0F );
				platform->SetRdrAzCenter( tmpPtr->localData->az );
				platform->SetRdrElCenter( tmpPtr->localData->el );

				// Tag the target as seen from this frame, unless the target is fading
				if (!(canSee & SG_FADING)) {
					if (sendThisFrame) {
						SendTrackMsg(tmpPtr, Track_Lock, digiRadarMode );
						lastTargetLockSend = SimLibElapsedTime;
					}
				}
			}
			// If we cannot see it, we clear it...
			else if (lastTargetLockSend){
				lockedTarget->localData->sensorState[Radar] = NoTrack;
				SetDesiredTarget( NULL );
			}
		}

		// Now it's time to try our next target... (2000-10-07: We'll use what we saved earlier)
        tmpPtr = tmpPtrNext;
	}

	// If we do not have a locked target, leave the radar centered...
	if (!lockedTarget){
		SetSeekerPos( 0.0f, 0.0f );
		platform->SetRdrAz( radarData->ScanHalfAngle );
		platform->SetRdrEl( radarData->ScanHalfAngle );
		platform->SetRdrCycleTime( 8.0F );
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}

	flag &= ~FirstSweep; // 2002-03-10 ADDED BY S.G. Say we have done our first radar sweep

	VU_ID			lastChaffID = FalconNullId;
	VU_ID			id;
	FalconEntity	*cm;
	float			chance;
	int				dummy = 0;
	SimObjectType	*target =lockedTarget ;
	static const float	cmRangeArray[]		= {0.0F,  1500.0f,  3000.0f,  11250.0f,  18750.0f,  30000.0f};
	static const float	cmBiteChanceArray[]	= {0.0F,     0.1F,     0.5F,      0.5F,      0.2F,      0.1F};
	static const int	cmArrayLength		= sizeof(cmRangeArray) / sizeof(cmRangeArray[0]);

	// No counter measures deployed by campaign things
	// countermeasures only work when tracking (for now)
	if (!lockedTarget || !target || !target->BaseData()->IsSim()) {
		return lockedTarget;
	}

	// Get the ID of the most recently launched counter measure from our target
	id = ((SimBaseClass*)target->BaseData())->NewestChaffID();
	
	// If we have a new chaff bundle to deal with
	if (id != lastChaffID){
		// Stop here if there isn't a counter measure in play
		if (id == FalconNullId) {
			lastChaffID = id;
			return lockedTarget;
		}

		// Try to find the counter measure entity in the database
		cm = (FalconEntity*)vuDatabase->Find( id );

//		MonoPrint ("ConsiderDecoy %08x %f: ", cm, target->localData->range);

		if (!cm) {
			// We'll have to wait until next time
			// (probably because the create event hasn't been processed locally yet)
			return lockedTarget;
		}

		// Start with the suceptability of this seeker to counter measures
		chance = radarData->ChaffChance;

		// Adjust with a range to target based chance of an individual countermeasure working
		chance *= Math.OnedInterp(target->localData->range, cmRangeArray, cmBiteChanceArray, cmArrayLength, &dummy);
		
		// 2000-11-17 REMOVED BY S.G. WHY SHOULD IT?
		// Player countermeasures work better
		//if (target->BaseData()->IsPlayer()) {
		//	chance *= 1.15F;
		//}

		// If we've beaten the missile guidance, countermeasures work two times better
		//if (!canGuide) {
		//	chance *= 2.0f;
		//}

		// Roll the dice
		if (chance > (float)rand()/RAND_MAX){
			// Compute some relative geometry stuff
			const float atx	= platform->dmx[0][0];
			const float aty	= platform->dmx[0][1];
			const float atz	= platform->dmx[0][2];
			const float dx	= cm->XPos() - platform->XPos();
			const float dy	= cm->YPos() - platform->YPos();
			const float dz	= cm->ZPos() - platform->ZPos();
			const float range	= (float)sqrt( dx*dx + dy*dy );
			const float cosATA	= (atx*dx + aty*dy + atz*dz) / (float)sqrt(range*range+dz*dz);
			
			// Only take the bait if we can see the thing
			// TODO:  Should probably use beam width instead of scan angle...
			if (cosATA >= cos(radarData->ScanHalfAngle)){
				if (lastTargetLockSend){
					lastTargetLockSend =0;
					lockedTarget->localData->sensorState[Radar] = NoTrack;
				}
				SetDesiredTarget(new SimObjectType(cm));
			}
			//MonoPrint ("Yes %f %08x\n", chance, target);
		}
		else
		{
			// MonoPrint ("No %f %08x\n", chance, target);
		}

		// Note that we've considered this countermeasure
		lastChaffID = id;
	}

	return lockedTarget;
}

void RadarDigiClass::NewRange( float newRange )
{
	// Round the new range to the nearest integer
	// SCR 9/23/98:  Why bother?
//	newRange = floor( newRange + 0.5f );

	// Keep our various measures of range consistent
	rangeFT = newRange*NM_TO_FT;
	rangeNM = newRange;
	invRangeFT = 1.0f/rangeFT;
}

void RadarDigiClass::SetMode (RadarMode cmd)
{
   mode = cmd;
	if (cmd == GM || cmd == GMT || cmd == SEA)
	{
		// This keeps it from pinging RWRs
		platform->SetRdrRng(0.0F);
		platform->SetRdrAz(0.0F);
		platform->SetRdrEl(0.0F);
		platform->SetRdrCycleTime( 5.0F );//me123
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}
	else
	{
		platform->SetRdrRng( radarData->NominalRange );
		platform->SetRdrAz( radarData->ScanHalfAngle );
		platform->SetRdrEl( radarData->ScanHalfAngle );
		platform->SetRdrCycleTime( 8.0F );
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}
}

int RadarDigiClass::IsAG (void)
{
int retval = FALSE;

// 2000-09-30 MODIFIED BY S.G. AG MODE CAN BE GM, GMT OR SEA, NOT JUST GM
// if (mode == GM)
   if (mode == GM || mode == GMT || mode == SEA)
// 2000-10-04 MODIFIED BY S.G. NEED TO KNOW WHICH MODE WE ARE IN
//     retval = TRUE;
	   retval = mode;

   return retval;
}

void RadarDigiClass::GetAGCenter (float* x, float* y)
{
   if (lockedTarget)
   {
      *x = lockedTarget->BaseData()->XPos();
      *y = lockedTarget->BaseData()->YPos();
   }
   else
   {
      *x = platform->XPos();
      *y = platform->YPos();
   }
}
