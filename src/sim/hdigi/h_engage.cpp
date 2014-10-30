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
    if (curMode not_eq WVREngageMode)
    {
        /*--------------------------------*/
        /* check against threshold values */
        /*--------------------------------*/
        classPtr = (Falcon4EntityClassType*)(maxTargetPtr->BaseData()->EntityType());

        if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_AIRPLANE) // and 
            /*
                      targetPtr->sensorState    >= modeData[WVREngageMode].minSensorState and 
                      targetPtr->sensorId       >= modeData[WVREngageMode].minSensorId and 
                      targetPtr->pcRange        <= modeData[WVREngageMode].maxRange and 
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
