#include "stdhdr.h"
#include "simveh.h"
#include "digi.h"
#include "object.h"
#include "simbase.h"
#include "aircrft.h"
#include "guns.h"
#include "airframe.h"
#include "simdrive.h"

#define MANEUVER_DEBUG
#ifdef MANEUVER_DEBUG
#include "Graphics\include\drawbsp.h"
extern int g_nShowDebugLabels;
#endif

#define CONTROL_POINT_DISTANCE    1600.0f; //me123 from  2750.0F
#define CONTROL_POINT_ELEVATION         25.0F
#define MAGIC_NUMBER                     0.5F
#define ALT_RATE_DEADBAND               1000.0F
//#define DEBUG_BFM 
#define VERTICAL_MAGIC   0.025f
void DigitalBrain::RollAndPull(void)
{ //MonoPrint ("RollAndPull");
	// me123 at the moment this is basicly THE' bfm rutine
	// we basicly put the lift vector on the bandit and pull
	// it's a agreesive 
#ifdef MANEUVER_DEBUG
char tmpchr[40];
strcpy(tmpchr, "R&P nothing");
#endif
   /*-------------------------------------------*/
   /* roll and pull logic is geometry dependent */
   /*-------------------------------------------*/
	if (groundAvoidNeeded == TRUE) 
	{
		GroundCheck();
		return;
	}

//	SLOW FLYING COMPETITION  ??  Rollign sizzors, flat sizzors or stack...some kinda 3/9 line fight
	if (targetData->range <= 500.0f && 
		(targetPtr->BaseData()->Yaw() - self->Yaw() < 30.0F * DTR)&&
		targetData->ata >= 55.0F * DTR &&
		targetData->ata <= 125.0F * DTR &&
		targetData->ataFrom >= 55.0F * DTR &&
		targetData->ataFrom <= 125.0F * DTR
	)
	
	{// This is a slow flyign competition
 
	 	if (targetData->ataFrom <= 140.0f * DTR)//me123 we are deffinatly not behind he's 3/9
		{
			SetTrackPoint(targetPtr);
#ifdef MANEUVER_DEBUG
			strcpy(tmpchr, "R&P slow fly comp not beh.3/9");
#endif
		}

		else //me123 we are behind him
		{
			SetTrackPoint(targetPtr);
#ifdef MANEUVER_DEBUG
			strcpy(tmpchr, "R&P slow fly comp behind 3/9");
#endif
		}
		AutoTrack(maxGs);
		
		MachHold(0.36F * cornerSpeed, self->GetKias(), FALSE);
#ifdef DEBUG_BFM 
		MonoPrint ("Slow flying competition  yeaa haaa let's hit the brakes");
#endif
	}




	// OFFENSIVE
	
else if (targetData->ata <= targetData->ataFrom || targetData->ata <= 90 *DTR)//me123 from 45
	{   
#ifdef DEBUG_BFM  		
	MonoPrint ("OFFENSIVE");
#endif
      /*--------------*/
      /* me -> <- him */
      /*--------------*/
		if (targetData->ataFrom <= 55.0F * DTR)
		{  
#ifdef DEBUG_BFM
			MonoPrint ("head on");   
#endif
			SetTrackPoint(
				targetPtr->BaseData()->XPos(), 
				targetPtr->BaseData()->YPos(), 
				targetPtr->BaseData()->ZPos()
			);
			
			if (targetPtr->localData->range > 6.0F * NM_TO_FT)
			{
//				MonoPrint ("pre mearge outside 6nm so let's fly fast");
				trackZ -=targetPtr->BaseData()->ZDelta()*0.5f ;
				// 2002-03-13 ADDED BY S.G. Lets not waste fuel for nothing IMHO
				if (targetPtr->localData->range > 15.0F * NM_TO_FT) {
					MachHold(cornerSpeed,self->GetKias(), TRUE);
#ifdef MANEUVER_DEBUG
strcpy(tmpchr, "R&P head on range > 15.0");
#endif
				}
				// Ok so we're within 6 to 15 NM, do we need to go real fast all the time or only when facing one another
				if (targetData->ata > 15.0f * DTR || targetData->ataFrom > 15.0f * DTR)
					MachHold(cornerSpeed,self->GetKias(), TRUE);
				else {
				// END OF ADDED SECTION 2002-03-14
					MachHold(2.0f * cornerSpeed,self->GetKias(), TRUE);
#ifdef MANEUVER_DEBUG
strcpy(tmpchr, "R&P head on range > 6.0");
#endif
				}
				AutoTrack(maxGs);
			}

			else if (targetPtr->localData->range  < 6.0F * NM_TO_FT && targetPtr->localData->range >= 1.5F * NM_TO_FT)
			{		
		//	MonoPrint ("between 6 and 1.5nm trying to force a merge nose up");
			trackZ += 4000.0f;       
			MachHold(1.05f * cornerSpeed, self->GetKias(), TRUE); // 2002-03-14 MODIFIED BY S.G. from 2 * cornerSpeed to 1.05f * cornerSpeed. Don't over do it
			AutoTrack(maxGs);
#ifdef MANEUVER_DEBUG
strcpy(tmpchr, "R&P head on range < 6.0");
#endif
			}

			else 
			{//MonoPrint ("inside 1.5nm ");
#ifdef MANEUVER_DEBUG
strcpy(tmpchr, "R&P head on range < 1.5");
#endif
			trackZ = targetPtr->BaseData()->ZPos();
			AutoTrack(maxGs);
			 /*-------------------*/
			 /* energy management */
			 /*-------------------*/
			EnergyManagement();
			}
		}

		  /*--------------*/
		  /* me -> him -> */
		  /*--------------*/
		else  
		{     
#ifdef DEBUG_BFM
			MonoPrint ("me -> him ->");
#endif
			SetTrackPoint(
				targetPtr->BaseData()->XPos(), 
				targetPtr->BaseData()->YPos(), 
				targetPtr->BaseData()->ZPos()
			);		
			AutoTrack(maxGs);
			 /*-------------------*/
			 /* energy management */
			 /*-------------------*/
#ifdef MANEUVER_DEBUG
strcpy(tmpchr, "R&P me -> him ->");
#endif
			EnergyManagement();

		 }
   }


   // HERE THE TARGET IS ON THE BEAM
	/*--------------------------------*/
    /* me ->   or  me ->  or  me->    */
    /* him ->      him ^     him v    */
    /*--------------------------------*/

	//NEUTRAL (none of us is pointing)  (when we get here ata is >35)
else if (targetData->ataFrom >= 45.0F * DTR)
{            
#ifdef DEBUG_BFM
		MonoPrint ("Neutral");
#endif
		SetTrackPoint(targetPtr->BaseData()->XPos(), targetPtr->BaseData()->YPos(), targetPtr->BaseData()->ZPos());
		
		AutoTrack(maxGs);
			 /*-------------------*/
			 /* energy management */
			 /*-------------------*/
		EnergyManagement();
#ifdef MANEUVER_DEBUG
		strcpy(tmpchr, "R&P Neutral");
#endif
}


   // DEFENSIVE
else if (targetData->ataFrom <45.0F * DTR)
   {
#ifdef DEBUG_BFM
MonoPrint ("Defensive ");
#endif	
		// OVERSHOOT CHECK
		if (-self->ZPos() > 3000.0f && targetPtr->localData->ata >= 150.0F * DTR &&  targetPtr->localData->range <= 2000.0f && -targetPtr->localData->rangedot * FTPSEC_TO_KNOTS > 70)
		{
			SetTrackPoint(targetPtr);
			AutoTrack(maxGs);
#ifdef MANEUVER_DEBUG
			strcpy(tmpchr, "R&P Defensive OVERSHOOT");
#endif
			MachHold(0.36F * cornerSpeed, self->GetKias(), FALSE);
#ifdef DEBUG_BFM
			MonoPrint ("Overshoot your basted ");
#endif	
			         
		}
		//NOT EMIDIATLY THREATENED
		else if  (	-targetPtr->BaseData()->ZPos() > 5000.0f &&
					targetData->range > 1000 &&
					targetData->ataFrom >= 15.0F * DTR &&
					self->GetKias() <= cornerSpeed * 0.9f && 
					self->Pitch() < -5*DTR)
		{
			SetTrackPoint(targetPtr);
			AutoTrack(maxGs);
#ifdef MANEUVER_DEBUG
			strcpy(tmpchr, "R&P Defens. NOT IMMED. THREATND");
#endif
			if (af->alpha > 2.0F  ) {
			SetPstick (-1.0F, maxGs, AirframeClass::GCommand);}//let's unload the jet
			// 2002-03-14 MODIFIED BY S.G. Little energy? cornerSpeed * 2 is 840 knots for the F16, drop this to 
			MachHold(1.05f * cornerSpeed, self->GetKias(), TRUE); 
//          MonoPrint ("NOT EMIDIATLY THREATENED Let's get alittle energy");
		}
		// EMIDIATLY THREATENED
		else 
		{
#ifdef MANEUVER_DEBUG
			strcpy(tmpchr, "R&P Defensive IMMED. THREATENED");
#endif
			SetTrackPoint(
				targetPtr->BaseData()->XPos(), 
				targetPtr->BaseData()->YPos(), 
				targetPtr->BaseData()->ZPos()
			);
			AutoTrack(maxGs);
			EnergyManagement();
		}

   }
#ifdef MANEUVER_DEBUG
if (g_nShowDebugLabels & 0x20)
{
	if ( self->drawPointer )
   			((DrawableBSP*)self->drawPointer)->SetLabel (tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
}
#endif 
}

void DigitalBrain::EnergyManagement(void)
{
if (targetData->range <= 1800.0f || 
		(
		targetData->range <= 2500.0f && 
		targetData->ata <= 45.0F * DTR && 
		targetData->ataFrom >= 90.0F * DTR 
		)
	)
	{
	MaintainClosure();
	return;
	}
//is the target maneuvering ?
			// 2002-03-14 MODIFIED BY S.G.Only SimBaseClass have deltas plus campaign objects don't actually fight
			if  (targetPtr->BaseData()->IsSim() && fabs(targetPtr->BaseData()->PitchDelta()) > 0.10F)
			// if  (fabs (targetPtr->BaseData()->PitchDelta()) > 0.10F)
			{
// is this a vertical fight ?
				if (fabs (targetPtr->BaseData()->YawDelta()) < VERTICAL_MAGIC &&
					fabs (self->YawDelta()) < VERTICAL_MAGIC)
	//both fighter and target vertival
				{
				 EagManage();
#ifdef DEBUG_BFM
MonoPrint ("both vertical , eag");
#endif	
				}

				else if (fabs (targetPtr->BaseData()->YawDelta())<VERTICAL_MAGIC ||
						 fabs (self->YawDelta())<VERTICAL_MAGIC)
	//only one of the jets are vertical
				{
					//fighter is vertical
					if (fabs (self->YawDelta())<VERTICAL_MAGIC)
					{ 
					 EagManage();
#ifdef DEBUG_BFM
MonoPrint ("fighter vertical Eag ");
#endif	
					}
					else
					//fighter is horizontal
					{
						if (self->ZPos() > (targetPtr->BaseData()->ZPos()))
						{//fighter is below
						MachHold(cornerSpeed, self->GetKias(), TRUE);
#ifdef DEBUG_BFM
MonoPrint ("fighter is below ");
#endif	
						}
						//fighter is above
						else 
						{
						MachHold(cornerSpeed, self->GetKias(), TRUE);
						
#ifdef DEBUG_BFM
MonoPrint ("fighter is above ");
#endif	
						}
					}
				}
	//horizontal fight
				else 
				{
					// nose/nose or nose/tail
					if (self->YawDelta() > 0 && targetPtr->BaseData()->YawDelta() < 0 ||
						self->YawDelta() < 0 && targetPtr->BaseData()->YawDelta() > 0)
					{//nose/nose
					MachHold(0.6f * cornerSpeed, self->GetKias(), FALSE);
#ifdef DEBUG_BFM
MonoPrint ("NOSE/NOSE ");
#endif	
					}
					else
					{//nose/tail
					MachHold(cornerSpeed, self->GetKias(), TRUE);
#ifdef DEBUG_BFM
MonoPrint ("nose/tail ");
#endif	
					}
				}

			}


//target is non maneuvering.
			else
			{

				if (targetData->ataFrom <90.0F * DTR)
				MachHold(cornerSpeed, self->GetKias(), FALSE);
				else
				MaintainClosure();
#ifdef DEBUG_BFM
MonoPrint ("target non maneuvering");
#endif	
			}
		
}
void DigitalBrain::PullToControlPoint(void)
{
	//me123 this rutine does our nose to nose tactic. The only one at the moment :-(
	if(!targetPtr){	return; }

	SetTrackPoint(targetPtr);
	AutoTrack(maxGs);
}

void DigitalBrain::EagManage(void)
{
		MachHold(1.05f * cornerSpeed,  self->GetKias(), FALSE); // 2002-03-14 MODIFIED BY S.G. Went from 1.1 to 1.05 (465 to 440 for the F16)

	//do the eag.
	if (self->PitchDelta() > 0)
	{// we are on the way up
		if (self->Pitch() < -45.0F * DTR)//were at the buttom of the eag
		{//always go for speed here
		MachHold(1.05f * cornerSpeed,  self->GetKias(), TRUE); // 2002-03-14 MODIFIED BY S.G. Went from 1.1 to 1.05 (465 to 440 for the F16)
#ifdef DEBUG_BFM
MonoPrint ("we are on the way up, always go for speed here");
#endif	
		}
		else if (self->Pitch() > 85.0F * DTR)//were goign over the top
		{//max aft stick awaileble and burner
		MachHold(cornerSpeed, self->GetKias(), FALSE);
#ifdef DEBUG_BFM
MonoPrint ("max aft stick awaileble and burner");
#endif	
		}
	}
	else
	{//we are on the way down
		if (self->Pitch() > -30.0F * DTR)//we are more nose up then xx degree sose down
		{
		MachHold(cornerSpeed, self->GetKias(), FALSE);
#ifdef DEBUG_BFM
MonoPrint ("we are on the way down...max pull");
#endif	
		}
		else //we are more the xx degree nose down 
		{//limit stick to hit corner +
		MachHold(cornerSpeed, self->GetKias(), TRUE);
#ifdef DEBUG_BFM
MonoPrint ("limit stick to hit corner +");
#endif	
		}
	}
	
}

void DigitalBrain::PullToCollisionPoint(void)
{
float tc;
float newX, newY, newZ;

   /*------------------------*/
   /* find time to collision */
   /*------------------------*/
   tc = CollisionTime();

   /*------------------------------*/
   /* If collision time is defined */
   /* extrapolate targets position */
   /*------------------------------*/
   if (lastMode != curMode)
   {
	  if (tc > 0.0)
      {
			float tx = targetPtr->BaseData()->XPos();
			float ty = targetPtr->BaseData()->YPos();
			float tz = targetPtr->BaseData()->ZPos();
			tx += targetPtr->BaseData()->XDelta() * tc;
			ty += targetPtr->BaseData()->YDelta() * tc;
			if (targetPtr->BaseData()->ZDelta() > ALT_RATE_DEADBAND ){
				tz += (targetPtr->BaseData()->ZDelta() - ALT_RATE_DEADBAND) * tc;
			}
			else if (targetPtr->BaseData()->ZDelta() < -ALT_RATE_DEADBAND ){
				tz += (targetPtr->BaseData()->ZDelta() + ALT_RATE_DEADBAND) * tc;
			}
			SetTrackPoint(tx, ty, tz);
      }
      /*-----------------------------*/
      /* Collision point not defined */
      /* track a point X seconds     */
      /* ahead of the target.        */
      /*-----------------------------*/
      else {
		  float 
			  tx = targetPtr->BaseData()->XPos(), 
			  ty = targetPtr->BaseData()->YPos(), 
			  tz = targetPtr->BaseData()->ZPos();

		tx += targetPtr->BaseData()->XDelta() * MAGIC_NUMBER;
		ty += targetPtr->BaseData()->YDelta() * MAGIC_NUMBER;
		if (targetPtr->BaseData()->ZDelta() > ALT_RATE_DEADBAND ){
			tz += (targetPtr->BaseData()->ZDelta() - ALT_RATE_DEADBAND) * MAGIC_NUMBER;
		}
		else if (targetPtr->BaseData()->ZDelta() < -ALT_RATE_DEADBAND ){
			tz += (targetPtr->BaseData()->ZDelta() + ALT_RATE_DEADBAND) * MAGIC_NUMBER;
		}
		SetTrackPoint(tx, ty, tz);
      }

      if (targetPtr->localData->range > 5.0F * NM_TO_FT && targetPtr->BaseData()->ZPos() > self->ZPos())
         trackZ = self->ZPos();
   }
   else
   {
      if (tc > 0.0)
      {
         newX = targetPtr->BaseData()->XPos();
         newY = targetPtr->BaseData()->YPos();
         newZ = targetPtr->BaseData()->ZPos();
         newX += targetPtr->BaseData()->XDelta() * tc;
         newY += targetPtr->BaseData()->YDelta() * tc;
         if (targetPtr->BaseData()->ZDelta() > ALT_RATE_DEADBAND )
            newZ += (targetPtr->BaseData()->ZDelta() - ALT_RATE_DEADBAND) * tc;
         else if (targetPtr->BaseData()->ZDelta() < -ALT_RATE_DEADBAND )
            newZ += (targetPtr->BaseData()->ZDelta() + ALT_RATE_DEADBAND) * tc;
      }
      /*-----------------------------*/
      /* Collision point not defined */
      /* new a point X seconds     */
      /* ahead of the target.        */
      /*-----------------------------*/
      else
      {
         newX = targetPtr->BaseData()->XPos();
         newY = targetPtr->BaseData()->YPos();
         newZ = targetPtr->BaseData()->ZPos();
         newX += targetPtr->BaseData()->XDelta() * MAGIC_NUMBER;
         newY += targetPtr->BaseData()->YDelta() * MAGIC_NUMBER;
         if (targetPtr->BaseData()->ZDelta() > ALT_RATE_DEADBAND )
            newZ += (targetPtr->BaseData()->ZDelta() - ALT_RATE_DEADBAND) * MAGIC_NUMBER;
         else if (targetPtr->BaseData()->ZDelta() < -ALT_RATE_DEADBAND )
            newZ += (targetPtr->BaseData()->ZDelta() + ALT_RATE_DEADBAND) * MAGIC_NUMBER;
      }

      if (targetPtr->localData->range > 5.0F * NM_TO_FT && targetPtr->BaseData()->ZPos() < self->ZPos())
         newZ = self->ZPos();

      SetTrackPoint(0.1F * newX + 0.9F * trackX, 0.1F * newY + 0.9F * trackY, 0.1F * newZ + 0.9F * trackZ);
   }
   AutoTrack(maxGs);
}

void DigitalBrain::MaintainClosure(void)
{
float rng,closure,rngdot;

   /*------------------------*/
   /* range to control point */
   /*------------------------*/
	if (targetData->ataFrom >= 90.0f *DTR)
	   {rng = targetData->range - CONTROL_POINT_DISTANCE;}

	else {
		// 2002-03-14 MODIFIED BY S.G. Uh? Range is NEVER negative, why do this? I think this is what RIK meant
		//rng = -targetData->range + 6000.0f - CONTROL_POINT_DISTANCE;
		rng = targetData->range - 6000.0f - CONTROL_POINT_DISTANCE;
	}

   /*------------------------*/
   /* current closure in kts */
   /*------------------------*/
   rngdot = -targetData->rangedot * FTPSEC_TO_KNOTS;

   /*---------------------------------------*/
   /* desired in kts closure based on range */
   /*---------------------------------------*/
   closure = (((rng - rngdot * 5.0f) / 1000.0F) * 50.0F); /* farmer range*closure function */ //me123
   closure = min (max (closure, -350.0F), 1000.0F);
   if (targetData->range < 2500.0f)
	   closure = min (closure, 30.0f);


   /*-------------------*/
   /* mach hold command */
   /*-------------------*/
   if (closure - rngdot > 0.0f) {
	   // 2002-03-14 MODIFIED BY S.G. If we're already getting closer and asking to gove above cornerSpeed, top at cornerSpeed if we're further than 2 NM
	   // MachHold ( (self->GetKias() + (closure - rngdot)), self->GetKias(), FALSE);  //me123 from MachHold(max (cornerSpeed, (self->GetKias() + (closure - rngdot))), self->GetKias(), TRUE);
	   MachHold(targetData->range > 2.0f * NM_TO_FT && rngdot > 0 && self->GetKias() + (closure - rngdot) > cornerSpeed ? cornerSpeed : self->GetKias() + (closure - rngdot), self->GetKias(), FALSE);  //me123 from MachHold(max (cornerSpeed, (self->GetKias() + (closure - rngdot))), self->GetKias(), TRUE);
   }
   else if (targetData->range < 5000.0f)
	   MachHold ( (min( cornerSpeed /* *1.2f S.G. */ , self->GetKias() + (closure - rngdot)) ), self->GetKias(), FALSE) ; //me123 from MachHold(max (cornerSpeed, (self->GetKias() + (closure - rngdot))), self->GetKias(), FALSE);
   else
	   MachHold ( (self->GetKias() + (closure - rngdot)), self->GetKias(), FALSE); //me123 from MachHold(max (cornerSpeed, (self->GetKias() + (closure - rngdot))), self->GetKias(), FALSE);

}

float DigitalBrain::CollisionTime(void)
{
float tc;

//   tc = min (targetData->range / -targetData->rangedot, 5.0F);
   tc = min (targetData->range / self->GetVt(), 0.5F);//me123 from 1.5

   return tc;
}

