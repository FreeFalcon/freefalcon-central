#include "stdhdr.h"
#include "simfile.h"
#include "otwdrive.h"
#include "object.h"
#include "simmover.h"
#include "visual.h"
#include "entity.h" // MN
#include "classtbl.h"//Cobra

extern int g_nMissileFix;

#define VISUAL_DIR       "sim\\sensdata\\visual"
#define VISUAL_DATASET   "visual.lst"

VisualDataType* VisualDataTable = NULL;
short NumVisualEntries = 0;

VisualClass::VisualClass(int idx, SimMoverClass* self) : SensorClass(self)
{
    dataProvided = PositionAndOrientation;
    typeData = &(VisualDataTable[min(max(idx, 0), NumVisualEntries - 1)]);
    sensorType = Visual;
}


VisualClass::~VisualClass(void)
{
}


int VisualClass::CanSeeObject(SimObjectType* obj)
{
    // 2002-04-17 MN "GPS type" weapons can see and detect always

    // RV - Biker - Add check for bomb or missile to avoid AI AC to always see target
    if ((g_nMissileFix bitand 0x100) and (platform->IsBomb() or platform->IsMissile()))
    {

        Falcon4EntityClassType* classPtr = NULL;
        classPtr = (Falcon4EntityClassType*)platform->EntityType();
        WeaponClassDataType *wc = NULL;

        ShiAssert(classPtr);

        if (classPtr)
            wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

        ShiAssert(wc);

        if (wc and (wc->Flags bitand WEAP_BOMBGPS))
            return true;
    }

    SimObjectLocalData* objData = obj->localData;

    // This test seems a little weird.  Could it be optimized by moving the adds and using fabs()?
    if (objData->az < (seekerAzCenter + typeData->right) and 
        objData->az > (seekerAzCenter + typeData->left) and 
        objData->el < (seekerElCenter + typeData->top) and 
        objData->el > (seekerElCenter + typeData->bottom))
        return TRUE;
    else
        return FALSE;
}
// 2000-10-02 FUNCTION ADDED BY S.G. SO WE CAN SEE IF WE CAN SEE SOMETHING BY JUST ITS az AND el
inline int VisualClass::CanSeeObject(float az, float el)
{
    // This test seems a little weird.  Could it be optimized by moving the adds and using fabs()?
    if (az < (seekerAzCenter + typeData->right) and 
        az > (seekerAzCenter + typeData->left) and 
        el < (seekerElCenter + typeData->top) and 
        el > (seekerElCenter + typeData->bottom))
        return TRUE;
    else
        return FALSE;
}

float VisualClass::GetSignature(SimObjectType*)
{
#if 0 // SCR 10/23/98:  This seems nice and all, but we don't have any data for this...
    /*
     SIGNATURE_DATA_TYPE* visData;
     Falcon4EntityClassType* classPtr;
     static int i, j;

     classPtr = (Falcon4EntityClassType *) obj->BaseData()->EntityType();

     visData = VisDataTable[min (classPtr->visType[0], NumSignaturesLoaded - 1)];

     // TODO:  Factor in light level
     return Math.TwodInterp (obj->localData->azFrom, obj->localData->elFrom,
     visData->azData, visData->elData,
     visData->signature, visData->numAz, visData->numEl, &i, &j);
    */
#else
    // Return a unit signal strength (could/should this be based on object size?)
    return 1.0f;
#endif
}


int VisualClass::CanDetectObject(SimObjectType* obj)
{
    // 2002-04-17 MN "GPS type" weapons can see and detect always
    if (g_nMissileFix bitand 0x100)
    {
        Falcon4EntityClassType* classPtr = NULL;
        classPtr = (Falcon4EntityClassType*)platform->EntityType();
        WeaponClassDataType *wc = NULL;

        //Cobra added this check because aircraft got in here were detected incorrect due to flag confusion
        if (classPtr->vuClassData.classInfo_[VU_TYPE] == STYPE_BOMB_GPS)
        {
            ShiAssert(classPtr);

            if (classPtr)
                wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

            ShiAssert(wc);

            if (wc and (wc->Flags bitand WEAP_BOMBGPS))
                return true;
        }
    }

    float signalStr;

    // edg: this is how we LOD ground vehicles.  1st determine if the
    // object is a ground vehicle.  If so, see if it has its inView variable
    // set.  If not return FALSE immediately since that vehicle will be LOD'd
    // out
    if (obj->BaseData()->IsSim() and ((SimBaseClass *)obj->BaseData())->IsSetLocalFlag(IS_HIDDEN))
    {
        return FALSE;
    }

    //Cobra moved this above signalStr ... why check all that if don't have LOS?
    if (platform->CheckCompositeLOS(obj))
    {
        // Get Observer Signature
        signalStr = GetSignature(obj);
    }
    else
    {
        signalStr = 0; //Can't See
    }

    return (FloatToInt32(signalStr));

    //Cobra GetSig now returns visDetNM
    // Scale for range squared
    //signalStr *= typeData->nominalRange * typeData->nominalRange;
    //signalStr /= obj->localData->range  * obj->localData->range;

    // Check threshold
    //if (signalStr < 1.0F)
    //return FALSE;

    // Check line of sight (terrain and clouds)
    //signalStr *= OTWDriver.CheckCompositLOS( platform, obj->BaseData() );
    //signalStr *= platform->CheckCompositeLOS( obj );


    // Return our decision
    //return (signalStr >= 1.0F);
}
