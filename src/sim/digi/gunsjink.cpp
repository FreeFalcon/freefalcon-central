#include "stdhdr.h"
#include "simveh.h"
#include "digi.h"
#include "object.h"
#include "simbase.h"
#include "airframe.h"
#include "team.h"
#include "aircrft.h"
#include "sms.h"

#define INIT_GUN_VEL  4500.0f //me123 changed from 3500.0F
extern float SimLibLastMajorFrameTime;


void DigitalBrain::GunsJinkCheck(void)
{
float tgt_time=0.0F,att_time=0.0F,z=0.0F, timeDelta=0.0F;
int twoSeconds=0;
unsigned long lastUpdate=0;
SimObjectType* obj = targetPtr;
SimObjectLocalData* localData=NULL;

   /*-----------------------------------------------*/
   /* Entry conditions-                             */
   /*                                               */
   /* 1. Target range <= INIT_GUN_VEL feet.         */
   /* 2. Target time to fire < ownship time to fire */
   /* 3. Predicted bullet fire <= 2 seconds.        */
   /*-----------------------------------------------*/
//me123 lets check is closure is resonable too before goin into jink mode
   if (curMode != GunsJinkMode)
   {
		if ( obj == NULL )
	  	return;

		if ( obj->BaseData()->IsSim() &&
			(((SimBaseClass*)obj->BaseData())->IsFiring() ||
			TeamInfo[self->GetTeam()]->TStance(obj->BaseData()->GetTeam()) == War))
		{
			localData = obj->localData;

			if ((localData->range > 0.0f) && (localData->range < 6000.0f))//localData->rangedot > -240.0f * FTPSEC_TO_KNOTS )//me123 don't jink if he's got a high closure, he probaly woun't shoot a low Pk shot
			{
				if (localData->range < INIT_GUN_VEL  )
				{
				   /*-----------------------------------*/
				   /* predict time of possible gun fire */
				   /*-----------------------------------*/
				   twoSeconds = FALSE;
				   jinkTime = -1;
				   z = localData->range / INIT_GUN_VEL;
		
				   if ( localData->azFrom > -15.0F * DTR && localData->azFrom < (15.0F * DTR))//me123 status test. changed from 2.0 to 5.0 multible places here becourse we are not always in plane when gunning
				   {
					  twoSeconds = TRUE;
				   }
				   else if (localData->azFrom > (5.0F * DTR) && localData->azFromdot < 0.0F)
				   {
					  if (localData->azFrom + z * localData->azFromdot < (5.0F * DTR))
						 twoSeconds = TRUE;
				   }
				   else if (localData->azFrom < (-5.0F * DTR) && localData->azFromdot > 0.0F)
				   {
					  if (localData->azFrom + z * localData->azFromdot > (-5.0F * DTR))
						 twoSeconds = TRUE;
				   }
		
				   if (twoSeconds)
				   {
					  twoSeconds = FALSE;
					  if (localData->elFrom < (4.0F *DTR) && localData->elFrom > (-10.0F * DTR))//me123 status test. changed all
					  {
						 twoSeconds = TRUE;
					  }
			//		  else if (localData->elFrom > (-2.0F *DTR) && localData->elFromdot < 0.0F)
			//		  {
			//			 if (localData->elFrom + z* localData->elFromdot < (-2.0F *DTR))
			//				twoSeconds = TRUE;
			//		  }
			//		  else if (localData->elFrom < (-13.0F * DTR) && localData->elFromdot > 0.0F)
			//		  {
			//			 if (localData->elFrom + z* localData->elFromdot > (-13.0F * DTR))
			//				twoSeconds = TRUE;
			//		  }
		
					  /*-------------------------------------------------*/
					  /* estimate time to be targeted and time to attack */
					  /*-------------------------------------------------*/
				   lastUpdate = targetPtr->BaseData()->LastUpdateTime();

				   if (lastUpdate == vuxGameTime)
					  timeDelta = SimLibMajorFrameTime;
				   else
					  timeDelta = SimLibLastMajorFrameTime;

				  /*-----------*/
					  /* him -> me */
					  /*-----------*/
					  tgt_time = ((localData->ataFrom / localData->ataFromdot) * timeDelta);
					//  if (tgt_time < 0.0F)//me123 status test,
					//	 tgt_time = 99.0F;//me123 status test,
					  if (localData->ataFrom > -13.0f *DTR && localData->ataFrom < 13.0f *DTR) {tgt_time = 0.0f;}//me123 status test,
					  /*-----------*/
					  /* me -> him */
					  /*-----------*/
					  att_time = (localData->ata / localData->atadot) * timeDelta;
					// if (att_time < 0.0F)//me123 status test,
					//	 att_time = 99.0F;//me123 status test,
					  if (localData->ata > -13.0f *DTR && localData->ata < 13.0f *DTR) {att_time = 0.0f;}//me123 status test,
				   }
		
				   /*--------------*/
				   /* trigger jink */
				   /*--------------*/
				   if (twoSeconds && tgt_time <= att_time)
				   {
					  AddMode(GunsJinkMode);
				   }
				}
		  }
	  }

   } // if not guns jink mode

   /*------------------------------------*/
   /* else already in guns jink          */
   /* this maneuver is timed and removes */
   /* itself, but we must make sure the  */
   /* threat is still around             */
   /*------------------------------------*/
}

void DigitalBrain::GunsJink(void)
{
float aspect, roll_offset, eroll;
SimObjectLocalData* gunsJinkData;
SimObjectType* obj = targetList;
//int randVal;
float maxPull;

   /*------------------------------------*/
   /* this maneuver is timed and removes */
   /* itself, but we must make sure the  */
   /* threat is still around             */
   /*------------------------------------*/
   if ( targetPtr == NULL || targetPtr->BaseData()->IsExploding() || 
	   targetPtr && targetPtr->localData->range > 4000)
   {
	   // bail, no target
      jinkTime = -1;
	   return;
   }
	// Cobra No need to go through all the stuff if we need to avoid the ground
  if(groundAvoidNeeded)
	   return;

   /*-------------------*/
   /* energy management */
   /*-------------------*/
   MachHold(cornerSpeed,self->GetKias(), FALSE);//me123 from TRUE
   /*--------------------*/
   /* find target aspect */
   /*--------------------*/
   gunsJinkData = targetPtr->localData;
   aspect = 180.0F * DTR - gunsJinkData->ata;

   /*-----------------*/
   /* pick roll angle */
   /*-----------------*/
   if (jinkTime == -1)
   {
      // Should I jettison stores?
      if (self->CombatClass() != MnvrClassBomber)
      {
				//Cobra we do this always
				self->Sms->AGJettison();
				SelectGroundWeapon();
      }
      ResetMaxRoll();

      /*--------------------------------*/
      /* aspect >= 60 degrees           */
      /* put plane of wings on attacker */
      /*--------------------------------*/
      if (aspect >= 90.0F * DTR)//me123 changed from 60
      {
         /* offset required to put wings on attacker */
         if (gunsJinkData->droll >= 0.0F) roll_offset = gunsJinkData->droll - 90.0F * DTR;
         else roll_offset = gunsJinkData->droll + 90.0F * DTR;

         /* generate new phi angle */
         newroll = self->Roll() + roll_offset;
      }
      /*---------------------*/
      /* aspect < 60 degrees */
      /* roll +- 90 degrees  */
      /*---------------------*/
      else 
      {
         /* special in-plane crossing case, go the opposite direction */
         if (targetPtr && ((targetPtr->BaseData()->Yaw()   - self->Yaw() < 15.0F * DTR) &&
             (targetPtr->BaseData()->Pitch() - self->Pitch() < 15.0F * DTR) &&
             (targetPtr->BaseData()->Roll()  - self->Roll() < 15.0F * DTR)))
         {
            if (gunsJinkData->droll >= 0.0F && gunsJinkData->az > 0.0F)
            {
               newroll = self->Roll() + 90.0F * DTR; 
            }
            else if (gunsJinkData->droll < 0.0F && gunsJinkData->az < 0.0F)
            {
               newroll = self->Roll() - 90.0F * DTR;
            }
            else      /* fall out, normal case */
            {
               if (gunsJinkData->droll > 0.0F)
                  newroll = self->Roll() - 70.0F * DTR; //me123 status test changed from 90
               else
                  newroll = self->Roll() + 70.0F * DTR;//me123 status test changed from 90
            }
         }
         /* normal jink */
         else
         {
            if (gunsJinkData->droll > 0.0F)
               newroll = self->Roll() - 70.0F * DTR;//me123 status test changed from 90
            else
               newroll = self->Roll() + 70.0F * DTR;//me123 status test changed from 90
         }

         /*--------------------------------------------*/
         /* roll down if speed <= 60% of corner speed  */
         /*--------------------------------------------*/
         if (self->GetKias() <= 0.8F * cornerSpeed)//me123 status test. changed from 0.6
         {
            if (newroll >= 0.0F && newroll <= 45.0F * DTR)
               newroll += 30.0F * DTR;//me123 status test changed from 20
            else if (newroll <= 0.0F && newroll >= -45.0F * DTR)
               newroll -= 30.0F * DTR;//me123 status test changed from 20
         }
      }

      /*------------------------*/
      /* roll angle corrections */
      /*------------------------*/
      if (newroll > 180.0F * DTR)
         newroll -= 360.0F * DTR;
      else if (newroll < -180.0F * DTR)
         newroll += 360.0F * DTR;

      // Clamp roll to limits
      if (newroll > af->MaxRoll())
         newroll = af->MaxRoll();
      else if (newroll < -af->MaxRoll())
         newroll = -af->MaxRoll();

      jinkTime = 0;
   }

   // Allow unlimited rolling
   if (self->CombatClass() != MnvrClassBomber)
      SetMaxRoll (190.0F);

   /*---------------------------*/
   /* roll to the desired angle */
   /*---------------------------*/
   if (jinkTime == 0)
   {
      SetPstick (-2.0F, maxGs, AirframeClass::GCommand);

      /*------------*/
      /* roll error */
      /*------------*/
      eroll = newroll - self->Roll();

      /*-----------------------------*/
      /* roll the shortest direction */
      /*-----------------------------*/
      eroll = SetRstick( eroll * RTD * 4.0F) * DTR;
      SetMaxRollDelta (eroll);
	  //me123 and pull like hell
      maxPull = max (0.8F * af->MaxGs(), maxGs);
      SetPstick ( maxPull, af->MaxGs(), AirframeClass::GCommand);
	  /*-----------------------*/
	   /* stop rolling and pull */
	   /*-----------------------*/
	   if (fabs(eroll) < 5.0F * DTR)//me123 status test, from 5
	   { 
		  jinkTime = 1;
		  SetRstick( 0.5F );//me123 status test, from 0
	   }
   }   

   /*-----------------------*/
   /* pull max gs for 2 sec */
   /*-----------------------*/
   if (jinkTime > 0 || groundAvoidNeeded) 
   {
      maxPull = max (0.8F * af->MaxGs(), maxGs);
      SetPstick ( maxPull, af->MaxGs(), AirframeClass::GCommand);

      if (jinkTime++ > SimLibMajorFrameRate*2.0F + 1.0F)//me123 status test, pull for 5sec instead of 2
      {
         ResetMaxRoll();
         jinkTime = -1;
      }
      else
      {
         // Stay in guns jink
         AddMode(GunsJinkMode);
      }
   }
   else
   {
      // Stay in guns jink
     AddMode(GunsJinkMode);
   }
}
