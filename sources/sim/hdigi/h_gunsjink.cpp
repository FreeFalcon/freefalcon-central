#include "stdhdr.h"
#include "hdigi.h"
#include "object.h"
#include "simbase.h"

#define INIT_GUN_VEL   3500.0F

void HeliBrain::GunsJinkCheck(void)
{

//edg: lets get rid of this for now
// seems to be some crash in release mode in this area
#if 0
float tgt_time,att_time,z;
int twoSeconds;
SimObjectType* obj = self->targetList;
SimObjectLocalData* localData;

   /*-----------------------------------------------*/
   /* Entry conditions-                             */
   /*                                               */
   /* 1. Target range <= INIT_GUN_VEL feet.         */
   /* 2. Target time to fire < ownship time to fire */
   /* 3. Predicted bullet fire <= 2 seconds.        */
   /*-----------------------------------------------*/
   if (curMode != GunsJinkMode)
   {
      gunsJinkPtr = NULL;

      /*--------------------*/
      /* do for all objects */
      /*--------------------*/
      while (obj)
      {
         if (obj->BaseData()->IsSim() &&
            (((SimBaseClass*)obj->BaseData())->IsFiring() || ((SimBaseClass*)obj->BaseData())->GetTeam() != side))
         {
            localData = obj->localData;

            if (localData->range < INIT_GUN_VEL )
            {
               /*-----------------------------------*/
               /* predict time of possible gun fire */
               /*-----------------------------------*/
               twoSeconds = FALSE;
               jinkTime = -1;
               z = localData->range / INIT_GUN_VEL;
               if (fabs (localData->azFrom) < (4.0F * DTR))
               {
                  twoSeconds = TRUE;
               }
               else if (localData->azFrom > (4.0F * DTR) && localData->azFromdot < 0.0F)
               {
                  if (localData->azFrom + z * localData->azFromdot < (4.0F * DTR))
                     twoSeconds = TRUE;
               }
               else if (localData->azFrom < (-4.0F * DTR) && localData->azFromdot > 0.0F)
               {
                  if (localData->azFrom + z * localData->azFromdot > (-4.0F * DTR))
                     twoSeconds = TRUE;
               }

               if (twoSeconds)
               {
                  twoSeconds = FALSE;
                  if (fabs (localData->elFrom) < (2.0F * DTR) && localData->elFrom > (-10.0F * DTR))
                  {
                     twoSeconds = TRUE;
                  }
                  else if (localData->elFrom > (2.0F * DTR) && localData->elFromdot < 0.0F)
                  {
                     if (localData->elFrom + z * localData->elFromdot < (2.0F * DTR))
                        twoSeconds = TRUE;
                  }
                  else if (localData->elFrom < (-10.0F * DTR) && localData->elFromdot > 0.0F)
                  {
                     if (localData->elFrom + z * localData->elFromdot > (-10.0F * DTR))
                        twoSeconds = TRUE;
                  }

                  /*-------------------------------------------------*/
                  /* estimate time to be targeted and time to attack */
                  /*-------------------------------------------------*/
                  /*-----------*/
                  /* him -> me */
                  /*-----------*/
                  tgt_time = ((localData->ataFrom / localData->ataFromdot) * SimLibMajorFrameTime);
                  if (tgt_time < 0.0F)
                     tgt_time = 99.0F;

                  /*-----------*/
                  /* me -> him */
                  /*-----------*/
                  att_time = (localData->ata / localData->atadot) * SimLibMajorFrameTime;
                  if (att_time < 0.0F)
                     att_time = 99.0F;
               }

               /*--------------*/
               /* trigger jink */
               /*--------------*/
               if (twoSeconds && tgt_time < att_time)
               {
                  gunsJinkPtr = obj;
                  AddMode(GunsJinkMode);
                  break;
               }
            }
         }

         obj = obj->next;
      }
   }

   /*------------------------------------*/
   /* else already in guns jink          */
   /* this maneuver is timed and removes */
   /* itself, but we must make sure the  */
   /* threat is still around             */
   /*------------------------------------*/
#endif
}

void HeliBrain::GunsJink(void)
{
//edg: lets get rid of this for now
// seems to be some crash in release mode in this area
#if 0
    SimObjectLocalData* gunsJinkData;
    SimObjectType* obj = self->targetList;

	// OutputDebugString( "In GunsJink\n" );

   /*------------------------------------*/
   /* this maneuver is timed and removes */
   /* itself, but we must make sure the  */
   /* threat is still around             */
   /*------------------------------------*/
   while (obj)
   {
      if (obj == gunsJinkPtr)
         break;
      obj = obj->next;
   }

   gunsJinkPtr = obj;

   if (gunsJinkPtr == NULL)                      /* bail if no target       */
   {
      jinkTime = -1;
      return;
   }

   gunsJinkData = gunsJinkPtr->localData;

   if (jinkTime == -1)
   {
     if ( rand() & 1 )
         newroll = -1.00f;
     else
         newroll = 1.00f;
   }

   jinkTime++;

   if (jinkTime < SimLibMajorFrameRate*1.0F )
   {
	    MachHold(0.8, 0.0F, FALSE);
	    LevelTurn (1.0f, newroll, TRUE);
        AltitudeHold(holdAlt);
   }
   else if (jinkTime < SimLibMajorFrameRate*2.0F )
   {
	    MachHold(1.0, 0.0F, FALSE);
	    LevelTurn (0.0f, newroll, TRUE);
        AltitudeHold(holdAlt);
   }
   else
   {
         jinkTime = -1;
         gunsJinkPtr = NULL;
   }
#endif
}
