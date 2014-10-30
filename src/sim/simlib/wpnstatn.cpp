#include "stdhdr.h"
#include "hardpnt.h"
#include "otwdrive.h"
#include "SimWeapn.h"
#include "SimVeh.h"
#include "drawbsp.h"
#include "Classtbl.h"
#include "entity.h"
#include "sms.h"
#include "grtypes.h"
#include "rdrackdata.h"


extern short NumWeaponTypes;
extern bool g_bBMSRackData; // MLR 2/13/2004 -
extern bool g_bNewRackData; // JPO // SP3
extern short gRackId_Single_Rack;
extern short gRackId_Triple_Rack;
extern short gRackId_Quad_Rack;
extern short gRackId_Six_Rack;
extern short gRackId_Two_Rack;
extern short gRackId_Single_AA_Rack;
extern short gRackId_Mav_Rack;
extern short gRocketId;



// ========================================
// Basic weapon station data
// ========================================

#ifdef USE_SH_POOLS
MEM_POOL BasicWeaponStation::pool;
MEM_POOL AdvancedWeaponStation::pool;
#endif

BasicWeaponStation::BasicWeaponStation(void)
{
    weaponId = 0;
    weaponCount = 0;
    weaponPointer.reset();
}

BasicWeaponStation::~BasicWeaponStation()
{
}

GunClass* BasicWeaponStation::GetGun(void)
{
    if (weaponPointer and weaponPointer->IsGun())
        return (GunClass*) weaponPointer.get();
    else
        return NULL;
}

// ========================================
// Advanced weapon station data
// ========================================

AdvancedWeaponStation::AdvancedWeaponStation(void)
{
    xPos = 0.0F;
    yPos = 0.0F;
    zPos = 0.0F;
    az   = 0.0F;
    el   = 0.0F;
    xSub  = NULL;
    ySub  = NULL;
    zSub  = NULL;
    azSub = NULL;
    elSub = NULL;
    aGun = NULL;
    theRack = NULL;
    numPoints = 0;
    memset(&weaponData, 0, sizeof weaponData); // JPO initialise
    weaponData.weaponClass = wcNoWpn;
    weaponData.domain = wdNoDomain;
    weaponType = wtNone;
    pylonId = 0;
    rackId = 0;
    thePylon = NULL; // MLR 2/20/2004 -
    theSMS = NULL; // MLR 2/21/2004 -
    theParent = NULL; // MLR 2/21/2004 -
    rackDataFlags = 0;
    pylonmnemonic = 0;
    rackmnemonic = 0;
    loadOrder = 0;
}

AdvancedWeaponStation::~AdvancedWeaponStation(void)
{
    Cleanup();
}

void AdvancedWeaponStation::Cleanup(void)
{
    // 2002-03-26 MN CTD fix, only delete when we used "new" below
    if (xSub not_eq &xPos and numPoints not_eq 1)
    {
        if (xSub)
            delete [] xSub;

        if (ySub)
            delete [] ySub;

        if (zSub)
            delete [] zSub;

        if (azSub)
            delete [] azSub;

        if (elSub)
            delete [] elSub;
    }
}

void AdvancedWeaponStation::SetupPoints(int num)
{
    numPoints = num;
    xPos = yPos = zPos = 0;

    if (numPoints == 1)
    {
        xSub  = &xPos;
        ySub  = &yPos;
        zSub  = &zPos;
        azSub = &az;
        elSub = &el;

        az = el = 0;
    }
    else
    {
        xSub  = new float[numPoints];
        ySub  = new float[numPoints];
        zSub  = new float[numPoints];
        azSub = new float[numPoints];
        elSub = new float[numPoints];

        int l;

        for (l = 0; l < numPoints; l++)
        {
            azSub[l] = 0;
            elSub[l] = 0;
            xSub[l] = xPos;
            ySub[l] = yPos;
            zSub[l] = zPos;
        }
    }
}


DrawableBSP* AdvancedWeaponStation::GetTopDrawable(void)
{
    if (thePylon)
        return thePylon;

    if (theRack)
        return theRack;

    if (weaponPointer and weaponPointer->drawPointer)
        return (DrawableBSP *)weaponPointer->drawPointer;

    return NULL;
}

// MLR 2/20/2004 - Get the lowest storage object
DrawableBSP* AdvancedWeaponStation::GetRackOrPylon(void)
{
    if (theRack)
        return theRack;

    if (thePylon)
        return thePylon;

    return NULL;
}

void AdvancedWeaponStation::SetSMS(SMSClass *Sms)
{
    theSMS = Sms;
}


// note, it's important to set the hpId 1st
void BasicWeaponStation::SetParentDrawPtr(DrawableBSP* Parent)
{
    theParent = Parent;
}

void AdvancedWeaponStation::SetParentDrawPtr(DrawableBSP* Parent)
{
    ShiAssert(hpId > 0);
    theParent = Parent;

    if (theParent and hpId > 0)
    {
        Tpoint hpPos = {0, 0, 0};


        theParent->GetChildOffset(hpId - 1, &hpPos);

        xPos = hpPos.x;
        yPos = hpPos.y;
        zPos = hpPos.z;
    }
}

void AdvancedWeaponStation::AttachPylonBSP(void)
{
    Tpoint hpPos = {0, 0, 0};
    Trotation viewRot = IMatrix;


    if (theParent)
    {
        theParent->GetChildOffset(hpId - 1, &hpPos);

        xPos = hpPos.x;
        yPos = hpPos.y;
        zPos = hpPos.z;
    }


    if (theParent and 
 not thePylon and 
        pylonId > 0 and 
        pylonId < NumWeaponTypes and 
        WeaponDataTable[pylonId].Index >= 0)
    {
        thePylon = new DrawableBSP(Falcon4ClassTable[WeaponDataTable[pylonId].Index].visType[0],
                                   &hpPos,
                                   &viewRot,
                                   OTWDriver.Scale());


        if (thePylon)
        {
            thePylon->SetTextureSet(theParent->GetTextureSet());
            thePylon->GetChildOffset(0, &hpPos);
            xPos += hpPos.x;
            yPos += hpPos.y;
            zPos += hpPos.z;
        }

    }


    if (theParent and thePylon)
    {
        theParent->AttachChild(thePylon, hpId - 1); // for UI compatibility
    }
}
void AdvancedWeaponStation::AttachRackBSP(void)
{
    Tpoint hpPos = {0, 0, 0};
    Trotation viewRot = IMatrix;

    xPos = yPos = zPos = 0;

    if (theParent)
    {
        theParent->GetChildOffset(hpId - 1, &hpPos);
        xPos += hpPos.x;
        yPos += hpPos.y;
        zPos += hpPos.z;
    }

    if (thePylon)
    {
        thePylon->GetChildOffset(0, &hpPos);
        xPos += hpPos.x;
        yPos += hpPos.y;
        zPos += hpPos.z;
    }

    if (theParent and 
 not theRack and 
        rackId > 0 and 
        rackId < NumWeaponTypes and 
        WeaponDataTable[rackId].Index >= 0)
    {
        theRack = new DrawableBSP(Falcon4ClassTable[WeaponDataTable[rackId].Index].visType[0],
                                  &hpPos,
                                  &viewRot,
                                  OTWDriver.Scale());


        if (theRack)
        {
            for (int l = 0; l < numPoints; l++)
            {
                theRack->GetChildOffset(l, &hpPos);
                xSub[l] = hpPos.x + xPos;
                ySub[l] = hpPos.y + yPos;
                zSub[l] = hpPos.z + zPos;
            }
        }

    }


    if (theRack)
    {
        //Tpoint pos = {xPos,yPos,zPos};

        //pos.x=xPos;
        //pos.y=yPos;
        //pos.z=zPos;

        //theRack->SetPosition(&pos);

        if (thePylon)
        {
            thePylon->AttachChild(theRack, 0);
        }
        else
        {
            if (theParent)
            {
                theParent->AttachChild(theRack, hpId - 1);
            }
        }
    }
}


void AdvancedWeaponStation::DeletePylonBSP(void)
{
    DrawableBSP *pylon;

    if (pylon = DetachPylonBSP())
    {
        delete pylon;
    }
}

DrawableBSP *AdvancedWeaponStation::DetachPylonBSP(void)
{
    if ( not thePylon)
        return NULL;

    if (theParent and thePylon)
    {
        theParent->DetachChild(thePylon, hpId - 1);

        /* this is done in DrawableBSP::DetachChild()
        theParent->GetChildOffset(hpId-1,&p);
        pos.x+=p.x;
        pos.y+=p.y;
        pos.z+=p.z;

        p.x = theParent->XPos() + theParent->dmx[0][0]*pos.x + theParent->dmx[1][0]*pos.y + theParent->dmx[2][0]*pos.z;
        p.y = theParent->YPos() + theParent->dmx[0][1]*pos.x + theParent->dmx[1][1]*pos.y + theParent->dmx[2][1]*pos.z;
        p.z = theParent->ZPos() + theParent->dmx[0][2]*pos.x + theParent->dmx[1][2]*pos.y + theParent->dmx[2][2]*pos.z;

        thePylon->SetPosition(&p);
        thePylon->orientation = theParent->orientation;
        */
    }

    //pylonmnemonic = 0; // clears from SMS page
    DrawableBSP *bsp = thePylon;
    thePylon = NULL;
    return bsp;
}

void AdvancedWeaponStation::DeleteRackBSP(void)
{
    DrawableBSP *rack;

    if (rack = DetachRackBSP())
    {
        delete rack;
    }
}


DrawableBSP *AdvancedWeaponStation::DetachRackBSP(void)
{
    if ( not theRack)
        return NULL;

    if (thePylon)
    {
        thePylon->DetachChild(theRack, 0);
    }
    else
    {
        if (theParent)
        {
            theParent->DetachChild(theRack, hpId - 1);
        }
    }

    // DetachChild won't place the BSP in the correct position when it is detached
    // from thePylon, because thePylon is a child of another object - DetachChild
    // doesn't work for grandchildren.

    Tpoint pos = {0, 0, 0}, p = {0, 0, 0};

    if (thePylon)
    {
        memcpy(&pos, &thePylon->instance.ParentObject->pSlotAndDynamicPositions[0], sizeof(Tpoint));
    }

    if (theParent)
    {
        memcpy(&p, &theParent->instance.ParentObject->pSlotAndDynamicPositions[hpId - 1], sizeof(Tpoint));
        pos.x += p.x;
        pos.y += p.y;
        pos.z += p.z;

        if (theSMS) // may be called from UI
        {
            SimVehicleClass *ownship = theSMS->Ownship();

            if (ownship)
            {
                p.x = ownship->XPos() + ownship->dmx[0][0] * pos.x + ownship->dmx[1][0] * pos.y + ownship->dmx[2][0] * pos.z;
                p.y = ownship->YPos() + ownship->dmx[0][1] * pos.x + ownship->dmx[1][1] * pos.y + ownship->dmx[2][1] * pos.z;
                p.z = ownship->ZPos() + ownship->dmx[0][2] * pos.x + ownship->dmx[1][2] * pos.y + ownship->dmx[2][2] * pos.z;
            }
        }

        theRack->SetPosition(&p);
        theRack->orientation = theParent->orientation;
    }



    //rackmnemonic = 0; // clears from SMS page

    DrawableBSP *bsp = theRack;
    theRack = NULL;
    return bsp;
}

/******************************************************************
                             WEAPONS
*******************************************************************/

/*
void AdvancedWeaponStation::AttachWeaponBSP(int WeaponId, int Count)
{

}
*/

/* load all weapon BSPs */
void BasicWeaponStation::AttachAllWeaponBSP(void)
{
    SimWeaponClass *weapPtr;

    // if(podPointer)
    // weapPtr = podPointer;
    // else
    weapPtr = weaponPointer.get();

    if (weapPtr)
    {
        // basic only attaches 1st weapon BSP
        AttachWeaponBSP(weapPtr);
        weapPtr = weapPtr->GetNextOnRail();
    }
}

void AdvancedWeaponStation::AttachAllWeaponBSP(void)
{
    SimWeaponClass *weapPtr;

    // if(podPointer)
    // weapPtr = podPointer;
    // else
    weapPtr = weaponPointer.get();

    while (weapPtr)
    {
        AttachWeaponBSP(weapPtr);
        weapPtr = weapPtr->GetNextOnRail();
    }
}


void BasicWeaponStation::AttachWeaponBSP(SimWeaponClass *weapPtr)
{
    if ( not weapPtr or weapPtr not_eq weaponPointer) // only attach 1st weapon
        return;

    DrawableBSP *weapBSP = (DrawableBSP *)weapPtr->drawPointer;

    if ( not weapBSP)
    {
        Tpoint hpPos;
        Trotation viewRot = IMatrix;
        GetSubPosition(0, &hpPos.x, &hpPos.y, &hpPos.z);


        short wid = weapPtr->GetWeaponId();

        if (OTWDriver.IsActive())
        {
            OTWDriver.CreateVisualObject(weapPtr);
            weapBSP = (DrawableBSP *)weapPtr->drawPointer;

        }
        else
        {
            weapBSP = new DrawableBSP(Falcon4ClassTable[WeaponDataTable[wid].Index].visType[0],
                                      &hpPos,
                                      &viewRot,
                                      OTWDriver.Scale());

            weapPtr->drawPointer = (DrawableObject *)weapBSP;
        }

        if (weapPtr->GetType() == TYPE_LAUNCHER)
            weapBSP->SetSwitchMask(0, 1);
    }

    if (weapBSP and theParent)
    {
        theParent->AttachChild(weapBSP, hpId - 1);
    }
}

void AdvancedWeaponStation::AttachWeaponBSP(SimWeaponClass *weapPtr)
{
    int weapslot;

    if ( not weapPtr)
        return;

    weapslot = weapPtr->GetRackSlot();

    if (weapslot >= numPoints)
    {
        weapslot = numPoints - 1;
        weapPtr->SetRackSlot(weapslot);
    }

    /*
    xPos = yPos = zPos = 0;

    if( theParent )
    {
     theParent->GetChildOffset(hpId-1, &hpPos);
     xPos += hpPos.x; yPos += hpPos.y; zPos += hpPos.z;
    }

    if( thePylon )
    {
     thePylon->GetChildOffset(0, &hpPos);
     xPos += hpPos.x; yPos += hpPos.y; zPos += hpPos.z;
    }

    if( theRack )
    {
     theRack->GetChildOffset(weapslot, &hpPos);
     xPos += hpPos.x; yPos += hpPos.y; zPos += hpPos.z;
    }
    */

    DrawableBSP *weapBSP = (DrawableBSP *)weapPtr->drawPointer;

    if ( not weapBSP)
    {
        Tpoint hpPos;
        Trotation viewRot = IMatrix;

        short wid = weapPtr->GetWeaponId();

        GetSubPosition(weapslot, &hpPos.x, &hpPos.y, &hpPos.z);

        if (OTWDriver.IsActive())
        {
            OTWDriver.CreateVisualObject(weapPtr);
            weapBSP = (DrawableBSP *)weapPtr->drawPointer;
        }
        else
        {
            weapBSP = new DrawableBSP(
                Falcon4ClassTable[WeaponDataTable[wid].Index].visType[0],
                &hpPos, &viewRot, OTWDriver.Scale()
            );
            weapPtr->drawPointer = (DrawableObject *)weapBSP;
        }

        if (weapPtr->GetType() == TYPE_LAUNCHER)
        {
            weapBSP->SetSwitchMask(0, 1);
        }
    }


    if (weapBSP)
    {
        // RV - Biker - switch texture set for weapons
        //if (theParent) {
        // int t = theParent->GetTextureSet();
        // int nt = weapBSP->GetNTextureSet()-1;
        // weapBSP->SetTextureSet(max(min(t, nt), 0));
        //}

        //if(theSMS)
        // theSMS->AddStore(hpId, weaponId, 1);
        Tpoint pos;

        pos.x = xPos;
        pos.y = yPos;
        pos.z = zPos;

        weapBSP->SetPosition(&pos);

        if (theRack)
        {
            theRack->AttachChild(weapBSP, weapslot);
        }
        else
        {
            if (thePylon)
            {
                thePylon->AttachChild(weapBSP, 0);
            }
            else
            {
                if (theParent)
                {
                    theParent->AttachChild(weapBSP, hpId - 1);
                }
            }
        }
    }
}

VuBin<SimWeaponClass> BasicWeaponStation::DetachFirstWeapon(void)
{
    // with the basic class, we may want to load the next weapons BSP and attach it?
    VuBin<SimWeaponClass> weapptr = weaponPointer;

    if (weapptr)
    {
        weaponPointer.reset(weapptr->GetNextOnRail());
        DetachWeaponBSP(weapptr.get());
        weapptr->nextOnRail.reset();
    }

    return weapptr;
}


VuBin<SimWeaponClass> AdvancedWeaponStation::DetachFirstWeapon(void)
{
    // with the advanced class, we may want to load the next weapons BSP and attach it if
    // there is no rack or pylon?
    VuBin<SimWeaponClass> weapptr = weaponPointer;

    if (weapptr)
    {
        DetachWeaponBSP(weapptr.get());
        weaponPointer.reset(weapptr->GetNextOnRail());
        weapptr->nextOnRail.reset();
    }

    return weapptr;
}

void BasicWeaponStation::DetachAllWeaponBSP(void)
{
    SimWeaponClass *weapPtr;

    // if(podPointer)
    // weapPtr = podPointer;
    // else
    weapPtr = weaponPointer.get();

    if (weapPtr) // basics only load the 1st BSP
    {
        DetachWeaponBSP(weapPtr);
        weapPtr = weapPtr->GetNextOnRail();
    }
}

void AdvancedWeaponStation::DetachAllWeaponBSP(void)
{
    SimWeaponClass *weapPtr;

    // if(podPointer)
    // weapPtr = podPointer;
    // else
    weapPtr = weaponPointer.get();

    while (weapPtr)
    {
        DetachWeaponBSP(weapPtr);
        weapPtr = weapPtr->GetNextOnRail();
    }
}

void BasicWeaponStation::DeleteAllWeaponBSP(void)
{
    SimWeaponClass *weapPtr;

    // if(podPointer)
    // weapPtr = podPointer;
    // else
    weapPtr = weaponPointer.get();

    while (weapPtr)
    {
        if (weapPtr->drawPointer) // basics only load the 1st BSP
        {
            DetachWeaponBSP(weapPtr);
            delete weapPtr->drawPointer;
            weapPtr->drawPointer = 0;
        }

        weapPtr = weapPtr->GetNextOnRail();
    }
}

void AdvancedWeaponStation::DeleteAllWeaponBSP(void)
{
    SimWeaponClass *weapPtr;

    // if(podPointer)
    // weapPtr = podPointer;
    // else
    weapPtr = weaponPointer.get();

    while (weapPtr)
    {
        if (weapPtr->drawPointer) // basics only load the 1st BSP
        {
            DetachWeaponBSP(weapPtr);
            delete weapPtr->drawPointer;
            weapPtr->drawPointer = 0;
        }

        weapPtr = weapPtr->GetNextOnRail();
    }
}



void BasicWeaponStation::DetachWeaponBSP(SimWeaponClass *weapPtr)
{
    if ( not weapPtr or weapPtr not_eq weaponPointer)
        return;

    DrawableBSP *weapBSP = (DrawableBSP *)weapPtr->drawPointer;

    if ( not weapBSP)
        return;


    if (theParent)
    {
        theParent->DetachChild(weapBSP, hpId - 1);
    }
}


void AdvancedWeaponStation::DetachWeaponBSP(SimWeaponClass *weapPtr)
{
    if ( not weapPtr)
        return;

    DrawableBSP *weapBSP = (DrawableBSP *)weapPtr->drawPointer;

    if ( not weapBSP)
        return;

    if (theRack)
    {
        theRack->DetachChild(weapBSP, weapPtr->GetRackSlot());
    }
    else
    {
        if (thePylon)
        {
            thePylon->DetachChild(weapBSP, 0);
        }
        else
        {
            if (theParent)
            {
                theParent->DetachChild(weapBSP, hpId - 1);
            }
        }
    }

    // DetachChild won't place the BSP in the correct position when it is detached
    // from thePylon or theRack, because thePylon bitand theRack  are the children of
    // another object.
    // DetachChild doesn't work for (great)grandchildren.

    Tpoint pos = {0, 0, 0}, p = {0, 0, 0};

    /*
     if(thePylon)
     {
     memcpy(&pos,&thePylon->instance.ParentObject->pSlotAndDynamicPositions[0],sizeof(Tpoint));
     }

     if(theRack)
     {
     memcpy(&p,&theRack->instance.ParentObject->pSlotAndDynamicPositions[weapPtr->GetRackSlot()],sizeof(Tpoint));
     pos.x+=p.x;
     pos.y+=p.y;
     pos.z+=p.z;
     }

     if(theParent)
     {
     memcpy(&p,&theParent->instance.ParentObject->pSlotAndDynamicPositions[hpId-1],sizeof(Tpoint));
     pos.x+=p.x;
     pos.y+=p.y;
     pos.z+=p.z;
    */
    if (theSMS) // for UI
    {
        SimVehicleClass *ownship = theSMS->Ownship();
        GetSubPosition(weapPtr->GetRackSlot(), &pos.x, &pos.y, &pos.z);

        //if(ownship)
        {
            p.x = ownship->XPos() + ownship->dmx[0][0] * pos.x + ownship->dmx[1][0] * pos.y + ownship->dmx[2][0] * pos.z;
            p.y = ownship->YPos() + ownship->dmx[0][1] * pos.x + ownship->dmx[1][1] * pos.y + ownship->dmx[2][1] * pos.z;
            p.z = ownship->ZPos() + ownship->dmx[0][2] * pos.x + ownship->dmx[1][2] * pos.y + ownship->dmx[2][2] * pos.z;
        }
    }

    weapBSP->SetPosition(&p);
    weapBSP->orientation = theParent->orientation;
    // }
}

int BasicWeaponStation::DetermineRackData(int HPGroup, int WeaponId, int WeaponCount)
{
    weaponId = WeaponId;
    weaponCount = WeaponCount;
    return 1;
}


int AdvancedWeaponStation::DetermineRackData(int HPGroup, int WeaponId, int WeaponCount)
{
    RDRackData rd;
    weaponId = WeaponId;
    weaponCount = WeaponCount;

    rackId = 0;
    pylonId = 0;

    WeaponClassDataType* wc;
    Falcon4EntityClassType* classPtr;
    SimWeaponDataType* wpnDefinition;

    wc = &WeaponDataTable[weaponId];
    classPtr = &(Falcon4ClassTable[wc->Index]);
    wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];


    SetWeaponClass((WeaponClass)wpnDefinition->weaponClass);

    if (g_bBMSRackData)
    {
        if (RDFindBestRack(HPGroup, weaponId, WeaponCount, &rd))
        {
            SetupPoints(rd.rackStations);

            rackDataFlags = rd.flags;
            pylonmnemonic = rd.pylonmnemonic;
            rackmnemonic  = rd.rackmnemonic;
            loadOrder     = rd.loadOrder;

            if (rd.rackCT)
            {
                int w = (short)(
                            ((int)Falcon4ClassTable[rd.rackCT].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType)
                        );
                SetRackId(w);
            }

            if (rd.pylonCT)
            {
                int w = (short)(
                            ((int)Falcon4ClassTable[rd.pylonCT].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType)
                        );
                SetPylonId(w);
            }

            //MonoPrint("BMS Rack HP:%d - PylonId:%d  RackId:%d\n",hpId, pylonId, rackId);

            return 1;
        }
    }

    int wclass = SimWeaponDataTable[Falcon4ClassTable[WeaponDataTable[WeaponId].Index].vehicleDataIndex].weaponClass;

    switch (wclass)
    {
        case wcCamera:
        case wcECM:

            // rackDataFlags = RDF_SELECTIVE_JETT_RACK bitor RDF_SELECTIVE_JETT_WEAPON;
            // break;
        case wcAimWpn:
            rackDataFlags = 0;
            break;

        default:
            rackDataFlags = RDF_EMERGENCY_JETT_RACK bitor RDF_SELECTIVE_JETT_RACK;
    }

    // SP3 data
    if (g_bNewRackData)
    {
        // JPO new scheme
        int rack = FindBestRackIDByPlaneAndWeapon(HPGroup, WeaponDataTable[weaponId].SimweapIndex, WeaponCount);
        ShiAssert(rack < MaxRackObjects); // -1 means nothing defined currently fallback to old scheme

        if (rack > 0 and rack < MaxRackObjects)
        {
            RackObject *rackp = &RackObjectTable[rack];
            SetupPoints(rackp->maxoccupancy);

            if ((wc->Flags bitand WEAP_ALWAYSRACK) == 0)
            {
                int rackid = (short)(((int)
                                      Falcon4ClassTable[rackp->ctind].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType)
                                    );
                SetRackId(rackid);
            }

            //MonoPrint("SP3 Rack HP:%d - PylonId:%d  RackId:%d\n",hpId, pylonId, rackId);
            return 1;
        }
    }

    // fallback
    // Find the proper rack id and max points

    if (GetWeaponClass() == wcRocketWpn)
    {
        // The rack id should have already been set up in SMSBaseClass
        SetupPoints(1);
    }
    else if (WeaponCount == 1)
    {
        if (GetWeaponClass() == wcAimWpn)
        {
            SetupPoints(1);
            SetRackId(gRackId_Single_AA_Rack);
        }
        else
        {
            SetupPoints(1);
            SetRackId(gRackId_Single_Rack);
        }
    }
    else if (WeaponCount <= 2 and GetWeaponClass() == wcAimWpn)
    {
        SetupPoints(2);
        SetRackId(gRackId_Two_Rack);
    }
    else if (WeaponCount <= 3)
    {
        if (GetWeaponClass() == wcAgmWpn)
        {
            SetupPoints(3);
            SetRackId(gRackId_Mav_Rack);
        }
        else
        {
            SetupPoints(3);
            SetRackId(gRackId_Triple_Rack);
        }
    }
    else if (WeaponCount <= 4)
    {
        SetupPoints(4);
        SetRackId(gRackId_Quad_Rack);
    }
    else
    {
        SetupPoints(6);
        SetRackId(gRackId_Six_Rack);
    }

    MonoPrint("MPS Rack HP:%d - PylonId:%d  RackId:%d\n", hpId, pylonId, rackId);


    return 1;
}

int AdvancedWeaponStation::GetRackDataFlags(void)
{
    return rackDataFlags;
}

int BasicWeaponStation::GetRackDataFlags(void)
{
    return RDF_EMERGENCY_JETT_RACK bitor RDF_SELECTIVE_JETT_RACK;
}

char *AdvancedWeaponStation::GetRackMnemonic(void)
{
    return rackmnemonic;
}

char *AdvancedWeaponStation::GetPylonMnemonic(void)
{
    return pylonmnemonic;
}
