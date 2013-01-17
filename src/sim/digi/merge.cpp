#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "object.h"
#include "airframe.h"
#include "aircrft.h"
#include "fakerand.h"

void DigitalBrain::MergeCheck(void)
{
float breakRange;

   /*-----------*/
   /* no target */
   /*-----------*/
   if (targetPtr == NULL) 
   {
      return;
   }

   /*-------*/
   /* entry */
   /*-------*/
   if (curMode != MergeMode)
   {
      if (-self->ZPos() > 3000 && targetData->range <= (1000) && targetData->ata < 45.0f * DTR && fabs(self->Pitch()) < 45.0F*DTR)
      {
      float dx, dy;

         dx = targetPtr->BaseData()->XPos() - self->XPos();
         dy = targetPtr->BaseData()->YPos() - self->YPos();

         // Max range when on target nose, 0 if a stern chase
         breakRange = ((targetPtr->BaseData()->GetKias() * self->GetKias())) * //me123 
            (1.0F - targetData->ataFrom / (180.0F * DTR)) *
            (1.0F - targetData->ataFrom / (180.0F * DTR));

         if (dx*dx + dy*dy < breakRange && targetData->ataFrom < 45.0F * DTR)
            AddMode(MergeMode);
      }
   }
   /*------*/
   /* exit */
   /*------*/
   else if (curMode == MergeMode)
   {
      if (-self->ZPos() > 3000 && SimLibElapsedTime < mergeTimer)
         AddMode(MergeMode);
      else
         mergeTimer = 0;
   }
}

void DigitalBrain::MergeManeuver(void)
{
int mnverFlags;
float eDroll;
float curRoll = self->Roll();

   /*-----------*/
   /* no target */
   /*-----------*/
   if (targetPtr == NULL) 
   {
      return;
   }

   // Pick bank angle on first pass
   if (lastMode != MergeMode)
   {
      // Mil power except for Vertical;

      mergeTimer = SimLibElapsedTime + 3 * SEC_TO_MSEC;//me123 from 5
      mnverFlags = maneuverClassData[self->CombatClass()].flags;
      switch (mnverFlags & (CanLevelTurn | CanSlice | CanUseVertical))
      {
         case CanLevelTurn:
            if ((mnverFlags & CanOneCircle) && (self->GetKias() < cornerSpeed))//me123
            {
               // One Circle, turn away from the target
               newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
			   MachHold (cornerSpeed, self->GetKias(), FALSE);
            }
            else
            {
               // Two Circle, turn towards the target
               newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
			      MachHold (cornerSpeed, self->GetKias(), TRUE);
            }
         break;

         case CanSlice:
            if (curRoll > 0.0F)
			{
               newroll = 135.0F * DTR;
		      MachHold (cornerSpeed, self->GetKias(), FALSE);//me123
			}
            else
			{
               newroll = -135.0F * DTR;
		      MachHold (cornerSpeed, self->GetKias(), FALSE);//me123
			}
         break;

         case CanUseVertical:
            newroll = 0.0F;
			MachHold (cornerSpeed, self->GetKias(), TRUE);
            // Full burner for the pull
         break;

         case CanLevelTurn | CanSlice:
            // level turn or slice?
            if ((self->GetKias() > cornerSpeed) && -self->ZPos() >3000.0f )//me123
            {
               // Level Turn
               if ((mnverFlags & CanOneCircle) && (self->GetKias() < cornerSpeed))
               {
                  // One Circle, turn away from the target
                  newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
  			      MachHold (0.7f* cornerSpeed, self->GetKias(), FALSE);//me123 addet *0.4
               }
               else
               {
                  // Two Circle, turn towards the target
                  newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
				  MachHold (cornerSpeed, self->GetKias(), TRUE);
               }
            }
            else
            {
               if (curRoll > 0.0F)
			   {
                  newroll = 135.0F * DTR;
				  MachHold (cornerSpeed, self->GetKias(), FALSE);
			   }
               else
			   {
                  newroll = -135.0F * DTR;
				  MachHold (cornerSpeed, self->GetKias(), FALSE);
			   }
            }
         break;

         case CanLevelTurn | CanUseVertical:
            // level turn or vertical?
            if (self->GetKias() < cornerSpeed* 1.2)//me123
            {
               // Level Turn
               if ((mnverFlags & CanOneCircle) && (self->GetKias() < cornerSpeed))
               {
                  // One Circle, turn away from the target
                  newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
				  MachHold (0.7f*cornerSpeed, self->GetKias(), FALSE);
               }
               else
               {
                  // Two Circle, turn towards the target
                  newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
				  MachHold (cornerSpeed, self->GetKias(), TRUE);
               }
            }
            else
            {
               newroll = 0.0F;
			   MachHold (cornerSpeed, self->GetKias(), TRUE);
               // Full burner for the pull
            }
         break;

         case CanSlice | CanUseVertical:
            // slice or vertical?
            if ((self->GetKias() < cornerSpeed) && -self->ZPos() >3000.0f)//me123
            {
               if (curRoll > 0.0F)
			   {
                  newroll = 135.0F * DTR;
				  MachHold (cornerSpeed, self->GetKias(), FALSE);
			   }
               else
			   {
                  newroll = -135.0F * DTR;
				  MachHold (cornerSpeed, self->GetKias(), FALSE);
			   }
            }
            else
            {
               newroll = 0.0F;
			   MachHold (cornerSpeed, self->GetKias(), TRUE);
               // Full burner for the pull
            }
         break;

         case CanLevelTurn | CanSlice | CanUseVertical:
            // slice, level turn, or vertical?
            if ((self->GetKias() < cornerSpeed*0.7)&& -self->ZPos() >3000.0f)//me123
            {
               if ((mnverFlags & CanOneCircle))
			   {
                  // One Circle, turn away from the target
                  newroll = (targetData->az > 0.0F ? -135.0F * DTR : 135.0F * DTR);
				  MachHold (0.7f*cornerSpeed, self->GetKias(), FALSE);
			   }
               else
			   {
                  // Two Circle, turn towards the target
                  newroll = (targetData->az > 0.0F ? 135.0F * DTR : -135.0F * DTR);
				  MachHold (cornerSpeed, self->GetKias(), TRUE);
			   }
            }
            else if ((self->GetKias() < cornerSpeed * 1.2))//me123
            {
               // Level Turn
               if ((mnverFlags & CanOneCircle) && (self->GetKias() < cornerSpeed))
               {
                  // One Circle, turn away from the target
                  newroll = (targetData->az > 0.0F ? -90.0F * DTR : 90.0F * DTR);
				  MachHold (0.7f*cornerSpeed, self->GetKias(), FALSE);
               }
               else
               {
                  // Two Circle, turn towards the target
                  newroll = (targetData->az > 0.0F ? 90.0F * DTR : -90.0F * DTR);
				  MachHold (cornerSpeed, self->GetKias(), TRUE);
               }
            }
            else
            {
               newroll = 0.0F;
			   MachHold (cornerSpeed* 1.2f, self->GetKias(), TRUE);
               // Full burner for the pull
            }
         break;
      }
   }

   eDroll = newroll - self->Roll();
   SetYpedal( 0.0F );
   SetRstick( eDroll * 2.0F * RTD);
   SetPstick(maxGs, maxGs, AirframeClass::GCommand);
   SetMaxRoll (newroll * RTD);
   SetMaxRollDelta (eDroll*RTD);

}

void DigitalBrain::AccelCheck(void)
{
	//Leon, if you are in waypoint mode, or loiter mode, it may be desired to fly at less than corner speed
	//this is only important in combat
	if(nextMode >= MergeMode && nextMode <= BVREngageMode && nextMode != GroundAvoidMode)
	{
	   if ((self->Pitch() > 50.0F * DTR && self->GetKias() < cornerSpeed * 0.4F) ||//me123 150kias
		  (self->Pitch() > 0.0F * DTR && self->GetKias() < cornerSpeed * 0.35F))//me123 100 GetKias
	   {
		  AddMode (AccelMode);
	   }
	   else if (curMode == AccelMode && self->GetKias() < cornerSpeed * 0.447F && 
		   self->Pitch() > 0.0F * DTR && self->GetKias())//me123 180kias
	   {
		  AddMode (AccelMode);
	   }
	}
}

void DigitalBrain::AccelManeuver(void) 
{
float eDroll;
float tmp;
//MonoPrint ("Accelmaneuver");
   // Choose plane?
   if ((self->Roll()) >= 0)
	{tmp = 170.0F * DTR;
//   MonoPrint (">0");
	}
   else
   {tmp = -170.0F * DTR;
//   MonoPrint ("<0");
	}

   eDroll = tmp - self->Roll();
   SetYpedal( 0.0F );
   SetRstick( eDroll *2 * RTD);
   MachHold (cornerSpeed, self->GetKias(), FALSE);
   if (fabs(eDroll* RTD) > 10.0F )
   SetPstick(0.0F, maxGs, AirframeClass::GCommand);
   else
   SetPstick(4.0F, maxGs, AirframeClass::GCommand);
   SetMaxRoll (170.0F);
}
