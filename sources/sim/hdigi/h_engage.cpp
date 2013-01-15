#include "stdhdr.h"
#include "classtbl.h"
#include "hdigi.h"
#include "object.h"
#include "simbase.h"
#include "Entity.h"

void HeliBrain::WvrEngageCheck(void)
{
Falcon4EntityClassType *classPtr;

   /*---------------------*/
   /* return if no target */
   /*---------------------*/
   if (maxTargetPtr == NULL)
   {
      return;
   }

   /*-------*/
   /* entry */
   /*-------*/
   if (curMode != WVREngageMode)
   {
      /*--------------------------------*/
      /* check against threshold values */
      /*--------------------------------*/
		classPtr = (Falcon4EntityClassType*) (maxTargetPtr->BaseData()->EntityType());
      if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_AIRPLANE) // &&
/*
          targetPtr->sensorState    >= modeData[WVREngageMode].minSensorState &&
          targetPtr->sensorId       >= modeData[WVREngageMode].minSensorId &&
          targetPtr->pcRange        <= modeData[WVREngageMode].maxRange &&
          targetPtr->pc_ata          <= modeData[WVREngageMode].max_ata)
*/
      {
         AddMode(WVREngageMode);
      }
   } 
   /*------------------------------------*/
   /* engagement changes and transitions */
   /*------------------------------------*/
   else
   {
      /*----------------------------------*/
      /* target change to a higher threat */
      /*----------------------------------*/
      /* handled in target selection */

      /*-----------------------------------*/
      /* target change to an easier target */
      /*-----------------------------------*/
      /* handled in target selection */
   
      /*------*/
      /* exit */
      /*------*/

      /*---------------------------------*/
      /* target outside threshold values */
      /*---------------------------------*/
   } 
}
