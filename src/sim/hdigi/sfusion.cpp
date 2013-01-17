#include "stdhdr.h"
#include "classtbl.h"
#include "hdigi.h"
#include "sensors.h"
#include "radar.h"
#include "irst.h"
#include "rwr.h"
#include "visual.h"
#include "simveh.h"
#include "missile.h"
#include "object.h"
#include "Entity.h"

#define NO_ID 0
#define EID   1
#define VID   2

void HeliBrain::SensorFusion(void)
{
SimObjectType* obj = targetList;
float turnTime,timeToRmax,rmax,tof,totV;
Falcon4EntityClassType *classPtr;
SimObjectLocalData* localData;
int pcId;

   /*--------------------*/
   /* do for all objects */
   /*--------------------*/
   while (obj)
   {
      localData = obj->localData;
		classPtr = (Falcon4EntityClassType*) obj->BaseData()->EntityType();

	  if (!obj->BaseData()->IsSim() || ((SimBaseClass*)obj->BaseData())->IsExploding() )
	  {
		  obj = obj->next;
		  continue;
	  }

	  /* using truth data */
      localData->sensorState[SensorClass::Visual]   = SensorClass::SensorTrack;

      /*--------------------------------------------------*/
      /* Sensor id state                                  */
      /* RWR ids coming form RWR_INTERP can be incorrect. */ 
      /* Visual identification is 100% correct.           */
      /*--------------------------------------------------*/
      if (localData->sensorState[SensorClass::Visual] || localData->sensorState[SensorClass::RWR] >= SensorClass::SensorTrack)
      {
			if (obj->BaseData()->IsMissile())
         {
            pcId = ID_MISSILE;
         }
			else if (obj->BaseData()->IsBomb())
         {
            pcId = ID_NEUTRAL;
         }
         else if (((SimBaseClass*)obj->BaseData())->GetTeam() == side)
				pcId = ID_FRIENDLY;
         else if (((SimBaseClass*)obj->BaseData())->GetTeam() == ID_NEUTRAL)
				pcId = ID_NEUTRAL;
         else
            pcId = ID_HOSTILE;
      }
      else 
      {
         pcId = ID_NONE;
      }

      /*----------------------------------------------------*/
      /* Threat determination                               */
      /* Assume threat has your own longest range missile.  */
      /* Hypothetical time before we're in the mort locker. */
      /* If its a missile calculate time to impact.         */
      /*---------------------------------------------------*/
      /*---------*/
      /* missile */
      /*---------*/
      if (obj->BaseData()->IsMissile())
      {
         /*--------------------*/
         /* known missile type */
         /*--------------------*/
         if (pcId == ID_MISSILE)
         {
            /*---------------------------------*/
            /* ignore your own side's missiles */
            /*---------------------------------*/
            if (((SimBaseClass*)obj->BaseData())->GetTeam() == side)
            {
               localData->threatTime = 2.0F * MAX_THREAT_TIME;
            }
            else
            {
               if (localData->sensorState[SensorClass::RWR] >= SensorClass::SensorTrack) 
                  localData->threatTime = localData->range / AVE_AIM120_VEL;
               else
                  localData->threatTime = localData->range / AVE_AIM9L_VEL;
            }
         }
         else
            localData->threatTime = MAX_THREAT_TIME;
      }
      /*----------*/
      /* aircraft */
      /*----------*/
      else if ((pcId != ID_NONE) && (pcId < ID_NEUTRAL))
      {
         /*-------------------------*/
         /* time to turn to ownship */
         /*-------------------------*/
         turnTime = localData->ataFrom / FIVE_G_TURN_RATE;

         /*------------------*/
         /* closing velocity */
         /*------------------*/
         totV = ((SimBaseClass*)obj->BaseData())->Vt() + self->Vt()*(float)cos(localData->ata*DTR);

         /*------------*/
         /* 10 NM rmax */
         /*------------*/
         rmax = 60762.11F;   /* 10 NM */

         /*-------------------------------------------*/
         /* calculate time to rmax and time of flight */
         /*-------------------------------------------*/
         if (localData->range > rmax) 
         {
            timeToRmax = (rmax - localData->range) / totV;
            tof = rmax / AVE_AIM120_VEL;
         }
         else 
         {
            timeToRmax = 0.0F;
            tof = localData->range / AVE_AIM120_VEL;
         }

         /*-------------*/
         /* threat time */
         /*-------------*/
         localData->threatTime = turnTime + timeToRmax + tof;
      }
      else
         localData->threatTime = 2.0F * MAX_THREAT_TIME;

      /*----------------------------------------------------*/
      /* Targetability determination                        */
      /* Use the longest range missile currently on board   */
      /* Hypothetical time before the tgt ac can be morted  */
      /*                                                    */
      /* Aircraft on own team are returned SENSOR_UNK       */
      /*----------------------------------------------------*/
      localData->targetTime = 2.0F * MAX_TARGET_TIME;
      if (!obj->BaseData()->IsMissile() && pcId < ID_NEUTRAL)
      {
         /*------------------*/
         /* closing velocity */
         /*------------------*/
         totV     = ((SimBaseClass*)obj->BaseData())->Vt()*(float)cos(localData->ataFrom*DTR) + self->Vt();

         /*------------------------*/
         /* time to turn on target */
         /*------------------------*/
         turnTime = localData->ata / FIVE_G_TURN_RATE;

         /*-------------------*/
         /* digi has missiles */
         /*-------------------*/
         rmax = 60762.11F;

         if (localData->range > rmax)
         {
            timeToRmax = (rmax - localData->range) / totV;
            tof = rmax / AVE_AIM120_VEL;
         }
         else 
         {
            timeToRmax = 0.0F;
            tof = localData->range / AVE_AIM120_VEL;
         }

         localData->targetTime = turnTime + timeToRmax + tof;
      }

      obj = obj->next;
   }
}
