/**********************
 *
 *
 * Munitions stuff
 *
 *
 **********************/

#include "Graphics/Include/TimeMgr.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/Include/render3d.h"
#include "Graphics/Include/drawbsp.h"
#include "vu2.h"
#include "F4Thread.h"
#include "cmpclass.h"
#include "campstr.h"
#include "squadron.h"
#include "flight.h"
#include "find.h"
#include "misseval.h"
#include "vehicle.h"
#include "weaplist.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "cstores.h"
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "MsgInc/FalconFlightPlanMsg.h"
#include "Campaign.h"
#include "railinfo.h"
#include "sim/include/airframe.h"
#include "sim/include/simweapn.h"

#pragma warning(disable : 4244)  // for +=

enum
{
    POD_EMPTY             = 200001,
    POD_FULL              = 200002,
    POD_DIS               = 200003,
    POD_DIFF              = 200004,
    LAU3_EMPTY            = 200005,
    LAU3_FULL             = 200006,
    LAU3_DIS              = 200007,
    LAU3_DIFF             = 200008,
    LAU2R_EMPTY           = 200009,
    LAU2R_FULL            = 200010,
    LAU2R_DIS             = 200011,
    LAU2R_DIFF            = 200012,
    LAU2L_EMPTY           = 200013,
    LAU2L_FULL            = 200014,
    LAU2L_DIS             = 200015,
    LAU2L_DIFF            = 200016,
    SINGLE_EMPTY          = 200017,
    SINGLE_FULL           = 200018,
    SINGLE_DIS            = 200019,
    SINGLE_DIFF           = 200020,
    LAU2C_EMPTY           = 200021,
    LAU2C_FULL            = 200022,
    LAU2C_DIS             = 200023,
    LAU2C_DIFF            = 200024,
    TER_EMPTY             = 200025,
    TER_FULL              = 200026,
    TER_DIS               = 200027,
    TER_DIFF              = 200028,
    DOUBLE_TER_EMPTY      = 200029,
    DOUBLE_TER_FULL       = 200030,
    DOUBLE_TER_DIS        = 200031,
    DOUBLE_TER_DIFF       = 200032,
    TER2R_EMPTY           = 200033,
    TER2R_FULL            = 200034,
    TER2R_DIS             = 200035,
    TER2R_DIFF            = 200036,
    POD_EMPTY_DIFF        = 200037,
    LAU3_EMPTY_DIFF       = 200038,
    LAU2R_EMPTY_DIFF      = 200039,
    LAU2L_EMPTY_DIFF      = 200040,
    SINGLE_EMPTY_DIFF     = 200041,
    LAU2C_EMPTY_DIFF      = 200042,
    TER_EMPTY_DIFF        = 200043,
    DOUBLE_TER_EMPTY_DIFF = 200044,
    TER2R_EMPTY_DIFF      = 200045,
    LAU1_FULL             = 200046,
    LAU1_FULL_DIFF        = 200047,
    TER2L_EMPTY           = 200048,
    TER2L_EMPTY_DIFF      = 200049,
    TER2L_FULL            = 200050,
    TER2L_DIS             = 200051,
    TER2L_DIFF            = 200052,
    DOUBLE_TER_4_DIFF     = 200053,
    DOUBLE_TER_4   = 200054,
    DOUBLE_TER_5L_DIFF    = 200055,
    DOUBLE_TER_5L   = 200056,
    DOUBLE_TER_5R_DIFF    = 200057,
    DOUBLE_TER_5R   = 200058,
    QUAD_EMPTY   = 200059,
    QUAD_FULL   = 200060,
    QUAD_DIS   = 200061,
    QUAD_DIFF   = 200062,
    QUAD_EMPTY_DIFF       = 200063,
    QUAD1L   = 200064,
    QUAD1L_DIS   = 200065,
    QUAD1L_DIFF       = 200066,
    QUAD1R   = 200067,
    QUAD1R_DIS   = 200068,
    QUAD1R_DIFF       = 200069,
    QUAD2L   = 200070,
    QUAD2L_DIS   = 200071,
    QUAD2L_DIFF       = 200072,
    QUAD2R   = 200073,
    QUAD2R_DIS   = 200074,
    QUAD2R_DIFF       = 200075,
    QUAD3L   = 200076,
    QUAD3L_DIS   = 200078,
    QUAD3L_DIFF       = 200079,
    QUAD3R   = 200080,
    QUAD3R_DIS   = 200081,
    QUAD3R_DIFF       = 200082,
    INT_EMPTY             = 200083,
    INT_FULL              = 200084,
    INT_DIS               = 200085,
    INT_DIFF              = 200086,
};

extern int g_nLoadoutTimeLimit; // JB 010729
extern bool g_bNewRackData; // JPO

#define _WPN_MAX_ 6
#define STRING_BUFFER_SIZE    10

void DeleteGroupList(long ID);
short GetFlightStatusID(Flight element);
void MakeStoresList(C_Window *win, long client);
void Uni_Float(_TCHAR *buffer);
void SetCurrentLoadout(void);

StoresList *gStores = NULL;

short g3dObjectID = 0;
extern C_Handler *gMainHandler;
extern VU_ID gSelectedFlightID;
extern VU_ID gPlayerFlightID; // Flight Player is in (NULL) if not in a flight
VU_ID gLoadoutFlightID = FalconNullId;
extern short FlightStatusID[];

OBJECTINFO Object;

// Loaded stores values
LoadoutStruct gCurStores[5]; // Last slot is the Starting list for the flight (set by kevin)
LoadoutStruct gOriginalStores[5]; // Last slot is the Starting list for the flight (set by kevin)
RailList      gCurRails[4]; // per AC... four max

long HardPoints; // number of valid hardpoints
static long Quantity     [4][2][HARDPOINT_MAX]; // Totals [Aircraft][0=Weapon ID,1=qty][#slots for different types (can't have more types than hardpoints)]
static long QuantityCount[4]; // # of Quantity slots used
long PlaneEditList[4]; // Planes to modify when selecting stores
long FirstPlane = 0; // 1st plane to edit
long PlaneCount = 4;
//TJL 01/02/04 For the change skin counter
int prevtext1 = 0;
extern int set3DTexture;

static long  _MAX_WEIGHT_[4];
static long  _CLEAN_WEIGHT_[4];
static float _DRAG_FACTOR_[4];
static long  _MUNITIONS_WEIGHT_[4];
static long  _FUEL_WEIGHT_[4];
static long  _CURRENT_WEIGHT_[4];
static long  RackFlag = -1;
static long  VisFlag = -1;
long gFlightOverloaded = 0;

VehicleClassDataType *gVCPtr = NULL;
int gVehID = 0;

static Tpoint objPos;
static Trotation objRot;
void PositandOrientSetData(float x, float y, float z, float pitch, float roll, float yaw,
                           Tpoint* simView, Trotation* viewRotation);

void TallyStores()
{
    int i, j, k, wid;

    // JB 020219 Limit munition planecount to less than five otherwise we overwrite memory.
    for (i = 0; i < PlaneCount and i < 4; i++)
    {
        // Zero out all stores totals
        for (j = 0; j < HARDPOINT_MAX; j++)
        {
            Quantity[i][0][j] = 0;
            Quantity[i][1][j] = 0;
        }

        QuantityCount[i] = 0;

        for (j = 1; j < HardPoints; j++)
        {
            // Tally stores Types
            wid = -1;

            for (k = 0; k < QuantityCount[i]; k++)
                if (Quantity[i][0][k] == gCurStores[i].WeaponID[j])
                {
                    wid = k;
                    break;
                }

            if (wid == -1)
            {
                wid = QuantityCount[i]++;
                Quantity[i][0][wid] = gCurStores[i].WeaponID[j];
            }

            Quantity[i][1][wid] += gCurStores[i].WeaponCount[j];

        }
    }
}

void UpdateInventoryCount()
{
    STORESLIST *store;
    short i, j;

    // JB 020219 Limit munition planecount to less than five otherwise we overwrite memory.
    for (i = 0; i < PlaneCount and i < 4; i++)
        for (j = 0; j < HARDPOINT_MAX; j++)
        {
            if (Quantity[i][1][j] > 0)
            {
                store = NULL;
                ShiAssert(gStores);

                if (gStores)
                    store = gStores->Find(Quantity[i][0][j]);

                if (store)
                    store->Stock += Quantity[i][1][j];
            }
        }
}

short TotalAvailable(short weaponID)
{
    STORESLIST *store;
    short avail, onboard;
    short i, j;

    store = NULL;
    ShiAssert(gStores);

    if (gStores)
        store = gStores->Find(weaponID);

    if (store)
    {
        avail = store->Stock;
        onboard = 0;

        // JB 020219 Limit munition planecount to less than five otherwise we overwrite memory.
        for (i = 0; i < PlaneCount and i < 4; i++)
            for (j = 0; j < HARDPOINT_MAX; j++)
            {
                if (Quantity[i][0][j] == weaponID)
                    onboard += Quantity[i][1][j];
            }

        return(static_cast<short>(max(avail - onboard, 0)));
    }

    return(0);
}

void PlaceLoadedWeapons(LoadoutStruct *loadout)
{
    STORESLIST *cur;
    long i, j, count;

    for (i = 0; i < 4; i++)
        if (PlaneEditList[i])
            memset(&gCurStores[i], 0, sizeof(LoadoutStruct));

    TallyStores();

    for (i = 0; i < 4; i++)
        if (PlaneEditList[i])
        {
            for (j = 1; j < HardPoints; j++)
            {
                cur = NULL;
                ShiAssert(gStores);

                if (gStores)
                    cur = gStores->Find(loadout->WeaponID[j]);

                if (cur and loadout->WeaponCount[j])
                {
                    count = loadout->WeaponCount[j];

                    if (count > cur->HardPoint[j])
                        count = cur->HardPoint[j];

                    if (count > TotalAvailable(loadout->WeaponID[j]))
                        count = TotalAvailable(loadout->WeaponID[j]);

                    gCurStores[i].WeaponID[j] = (loadout->WeaponID[j]);
                    gCurStores[i].WeaponCount[j] = static_cast<uchar>(count);
                }
            }
        }

    TallyStores();
}


static short LastCount[4][HARDPOINT_MAX];

ushort AttachBits[] =
{
    0x0000,
    0x0001,
    0x0003,
    0x0007,
    0x000f,
    0x001f,
    0x003f,
    0x007f,
    0x00ff,
    0x01ff,
    0x03ff,
    0x07ff,
    0x0fff,
    0x1fff,
    0x3fff,
    0x7fff,
    0xffff,
};


// JPO
// experimental rack mappings


BOOL GetJRackAndWeapon(VehicleClassDataType* vc, Falcon4EntityClassType *classPtr,
                       short WeaponIndex, short count, short hardpoint,
                       RailInfo *rail)
{
    Falcon4EntityClassType *weapClassPtr;
    long bitflag;
    //Falcon4EntityClassType* rackClassPtr;

    //memset(rail,0,sizeof(RailInfo)); // kills the hardpoint object
    if ( not count)
        return(FALSE);

    weapClassPtr = &Falcon4ClassTable[WeaponDataTable[WeaponIndex].Index];
    int weaponrg = WeaponDataTable[WeaponIndex].SimweapIndex;
    int idx = SimACDefTable[classPtr->vehicleDataIndex].airframeIdx;
    AuxAeroData *aux = aeroDataset[idx].auxaeroData;
    int planerg = aux->hardpointrg[hardpoint];


    bitflag = 1 << hardpoint;

    if ( not vc or not classPtr or not weapClassPtr)
        return(FALSE);

    if ( not (vc->VisibleFlags bitand bitflag))
        return(FALSE);

    /*
    int rackno = FindBestRackIDByPlaneAndWeapon(planerg, weaponrg, count);
    if (rackno == -1) return FALSE;
    RackObject *rackptr = &RackObjectTable[rackno];
    ShiAssert(rackptr->ctind > 0 and rackptr->ctind < NumEntities);
    rackClassPtr = &Falcon4ClassTable[rackptr->ctind];
    */

    rail->weaponCount = count;
    rail->hardPoint.DetermineRackData(planerg, WeaponIndex, count);
    rail->hardPoint.weaponId = WeaponIndex;

    // Use a rack
    /*
    if((vc->RackFlags bitand bitflag) or (WeaponDataTable[WeaponIndex].Flags bitand WEAP_ALWAYSRACK))
    {
        if(rackClassPtr->visType[0])
        {
     rail->rackID=rackptr->ctind;
     rail->weaponID=WeaponDataTable[WeaponIndex].Index;
     rail->startBits=AttachBits[count];
     rail->currentBits=AttachBits[count];
        }
    }
    else
    {
        if(weapClassPtr->visType[0])
        {
     rail->weaponID=WeaponDataTable[WeaponIndex].Index;
     rail->startBits=AttachBits[1];
     rail->currentBits=AttachBits[1];
        }
    }
    */
    return TRUE;
}


// 0=left,1=center,2=right
struct RackData
{
    short RackID[3];
};

// Consists of... NO_RACK,
RackData HeliRacks[] =
{
    { 0, 0, 0 }, // No Rack
    { VIS_HONERACK, VIS_HONERACK, VIS_HONERACK }, // Single
    { VIS_HBIRACK, VIS_HBIRACK, VIS_HBIRACK }, // Double
    { VIS_HTRIRACK, VIS_HTRIRACK, VIS_HTRIRACK }, // Tripple
    { VIS_QUAD_RACK, VIS_QUAD_RACK, VIS_QUAD_RACK }, // Quad
    { VIS_SIX_RACK, VIS_SIX_RACK, VIS_SIX_RACK }, // Fiver
    { VIS_SIX_RACK, VIS_SIX_RACK, VIS_SIX_RACK }, // Sixer
};

RackData ACRacks[] =
{
    { 0, 0, 0 }, // No Rack
    { VIS_SINGLE_RACK, VIS_SINGLE_RACK, VIS_SINGLE_RACK }, // Single
    { VIS_TRIPLE_RACK, VIS_BIRACK, VIS_RTRIRACK }, // Double
    { VIS_TRIPLE_RACK, VIS_TRIPLE_RACK, VIS_RTRIRACK }, // Tripple
    { VIS_SIX_RACK, VIS_SIX_RACK, VIS_SIX_RACK }, // Quad
    { VIS_SIX_RACK, VIS_SIX_RACK, VIS_SIX_RACK }, // Fiver
    { VIS_SIX_RACK, VIS_SIX_RACK, VIS_SIX_RACK }, // Sixer
};

RackData RocketRack[] =
{
    { 0, 0, 0 }, // No Rack
    { VIS_ONELAU3A, VIS_ONELAU3A, VIS_ONELAU3A }, // Single
    { VIS_BILAU3A, VIS_BILAU3A, VIS_BILAU3A }, // Double
    { VIS_TRILAU3A, VIS_TRILAU3A, VIS_TRILAU3A }, // Tripple
    { 0, 0, 0 }, // Quad
    { 0, 0, 0 }, // Fiver
    { 0, 0, 0 }, // Sixer
};

RackData Hellfires[] =
{
    { 0, 0, 0 }, // No Rack
    { VIS_QUAD_RACK, VIS_QUAD_RACK, VIS_QUAD_RACK }, // Single
    { VIS_QUAD_RACK, VIS_QUAD_RACK, VIS_QUAD_RACK }, // Double
    { VIS_QUAD_RACK, VIS_QUAD_RACK, VIS_QUAD_RACK }, // Tripple
    { VIS_QUAD_RACK, VIS_QUAD_RACK, VIS_QUAD_RACK }, // Quad
    { 0, 0, 0 }, // Fiver
    { 0, 0, 0 }, // Sixer
};

RackData Maverick[] =
{
    { 0, 0, 0 }, // No Rack
    { VIS_MAVRACK, VIS_MAVRACK, VIS_MAVRACK }, // Single
    { VIS_MAVRACK, VIS_MAVRACK, VIS_MAVRACK }, // Double
    { VIS_MAVRACK, VIS_MAVRACK, VIS_MAVRACK }, // Tripple
    { 0, 0, 0 }, // Quad
    { 0, 0, 0 }, // Fiver
    { 0, 0, 0 }, // Sixer
};

short FindRackIndex(short visID)
{
    Falcon4EntityClassType* classPtr;
    int index;

    if ( not visID)
        return(0);

    for (index = 0; index < NumEntities; index++)
    {
        classPtr = &Falcon4ClassTable[index];

        if (classPtr->visType[0] == visID)
            return(index);
    }

    return(0);
}
void ConvertToIndex(RackData Rack[])
{
    short i, j;

    for (i = 0; i < 7; i++)
    {
        for (j = 0; j < 3; j++)
        {
            Rack[i].RackID[j] = FindRackIndex(MapVisId(Rack[i].RackID[j]));
        }
    }
}

static short Validated = 0;
void ValidateRackData()
{
    if (Validated)
        return;

    ConvertToIndex(HeliRacks);
    ConvertToIndex(ACRacks);
    ConvertToIndex(RocketRack);
    ConvertToIndex(Hellfires);
    ConvertToIndex(Maverick);

    Validated = 1;
}

#if 0
BOOL GetRackAndWeapon(VehicleClassDataType* vc, short VehID, short WeaponIndex, short count, short hardpoint, short center, RailInfo *rail)
{
    Falcon4EntityClassType* classPtr, *weapClassPtr;
    long bitflag;
    short side;
    RackData *RackList;

    memset(rail, 0, sizeof(RailInfo));

    if ( not count)
        return(FALSE);

    if (count >= sizeof(ACRacks) / sizeof(ACRacks[0]))
        count =  sizeof(ACRacks) / sizeof(ACRacks[0]) - 1;

    classPtr = &Falcon4ClassTable[VehID];
    weapClassPtr = &Falcon4ClassTable[WeaponDataTable[WeaponIndex].Index];

    bitflag = 1 << hardpoint;

    if ( not vc or not classPtr or not weapClassPtr)
        return(FALSE);

    if ( not (vc->VisibleFlags bitand bitflag))
        return(FALSE);

    if (hardpoint < center)
        side = 0;
    else if (hardpoint > center)
        side = 2;
    else
        side = 1;

    if ((weapClassPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET or weapClassPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_LAUNCHER))
    {
        // Rocket POD... virtual weapons...ALWAYS get a rack
        rail->rackID = RocketRack[count].RackID[side];
        return(TRUE);
    }

    if (weapClassPtr->visType[0] == VIS_HELLFIRE)
    {
        RackList = Hellfires;
    }
    else if ((weapClassPtr->visType[0] == MapVisId(VIS_AGM65B)) or (weapClassPtr->visType[0] == MapVisId(VIS_AGM65D)) or (weapClassPtr->visType[0] == MapVisId(VIS_AGM65G)))
    {
        RackList = Maverick;
    }
    else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER) // if Helicopters... use their racks
    {
        RackList = HeliRacks;
    }
    else
    {
        RackList = ACRacks;
    }

    if (vc->RackFlags bitand bitflag) // Use a rack
    {
        if (RackList[count].RackID[side])
        {
            rail->rackID = RackList[count].RackID[side];
            rail->weaponID = WeaponDataTable[WeaponIndex].Index;
            rail->startBits = AttachBits[count];
            rail->currentBits = AttachBits[count];
        }
    }
    else
    {
        if (weapClassPtr->visType[0])
        {
            rail->weaponID = WeaponDataTable[WeaponIndex].Index;
            rail->startBits = AttachBits[1];
            rail->currentBits = AttachBits[1];
        }
    }

    return(TRUE);
}
#endif




void ClearHardPoint(long plane, long hardpoint, long, RailInfo *rail)
{
    rail->hardPoint.DeleteAllWeaponBSP();

    VuBin<SimWeaponClass> weapPtr;
    VuBin<SimWeaponClass> nextPtr;

    weapPtr = rail->hardPoint.weaponPointer;

    while (weapPtr)
    {
        nextPtr.reset(weapPtr->GetNextOnRail());
        weapPtr.reset();
        weapPtr = nextPtr;
    }

    rail->hardPoint.weaponPointer.reset();
    rail->hardPoint.DeleteRackBSP();
    rail->hardPoint.DeletePylonBSP();
    rail->hardPoint.SetParentDrawPtr(NULL);
    rail->hardPoint.SetRackId(0);
    rail->hardPoint.SetPylonId(0);
    rail->weaponCount = 0;

    /*
    BSPLIST *Plane;
    BSPLIST *Rack;
    BSPLIST *Weapon;
    short bits,i;

    Plane=gUIViewer->Find((plane << 24));
    if(Plane == NULL) return;

    if(rail->rackID)
    {
     Rack=gUIViewer->Find((plane << 24) + (hardpoint << 16));
     if(Rack)
     {
     if(rail->weaponID and rail->startBits)
     {
     bits=rail->startBits;
     i=0;
     while(bits)
     {
     if(bits bitand 1)
     {
     Weapon=gUIViewer->Find((plane << 24) + (hardpoint << 16) + (i+1));
     if(Weapon)
     {
     // JB 020314 work from the back to the font (like the 3d code)
     ((DrawableBSP*)Rack->object)->DetachChild(((DrawableBSP*)Weapon->object),
     ((DrawableBSP*)Rack->object)->instance.ParentObject->nSlots - i - 1);
     gUIViewer->Remove((plane << 24) + (hardpoint << 16) + (i+1));
     }
     }
     bits >>= 1;
     i++;
     }
     }
     ((DrawableBSP*)Plane->object)->DetachChild(((DrawableBSP*)Rack->object),hardpoint-1);
     gUIViewer->Remove((plane << 24) + (hardpoint << 16));
     Rack=NULL;
     }
    }
    else if(rail->weaponID)
    {
     if(rail->startBits)
     {
     Weapon=gUIViewer->Find((plane << 24) + (hardpoint << 16) + 1);
     if(Weapon)
     {
     ((DrawableBSP*)Plane->object)->DetachChild(((DrawableBSP*)Weapon->object),hardpoint-1);
     gUIViewer->Remove((plane << 24) + (hardpoint << 16) + 1);
     }
     }
    }*/
    //rail->rackID=0; // MLR 2/25/2004 -
    //rail->weaponID=0;
    rail->startBits = 0;
}

void ClearAllHardPointBSPs(void)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 1; j < HARDPOINT_MAX; j++)
        {
            if (j and j < HardPoints)
            {
                ClearHardPoint(i, j, HardPoints / 2, &gCurRails[i].rail[j]);
            }
        }
    }
}


#if 0
void Check_HTS_Tirn(DrawableBSP *plane, LoadoutStruc *stores)
{
    short hts, tirn;

    hts = 0;
    tirn = 0;


    for (i = 1; i < HardPoints; i++)
    {
        if (stors->WeaponCount[i])
        {
            weapClassPtr = &Falcon4ClassTable[WeaponDataTable[stores->WeaponID].Index];

            if (weapClassPtr)
            {
            }
        }
    }
}
#endif

SimWeaponClass* InitABomb(FalconEntity* parent, ushort type, int slot);

void LoadHardPoint(long plane, long hardpoint, long, RailInfo *rail)
{
    //Falcon4EntityClassType* rackPtr,*weapPtr;
    BSPLIST *Plane;
    DrawableBSP *PlaneBSP;
    // BSPLIST *Rack;
    // BSPLIST *Weapon;
    short /*bits,*/i;

    Plane = gUIViewer->Find((plane << 24));

    if (Plane == NULL) return;

    PlaneBSP = (DrawableBSP *)Plane->object;

    rail->hardPoint.SetHPId(hardpoint);
    rail->hardPoint.SetParentDrawPtr(PlaneBSP);
    rail->hardPoint.AttachPylonBSP();
    rail->hardPoint.AttachRackBSP();

    VuBin<SimWeaponClass> weapPtr;
    VuBin<SimWeaponClass> lastPtr;

    for (i = 0; i < rail->weaponCount; i++)
    {
        // Load from back of rack to front (ie, 2 missiles on a tri-rack will
        // load into slot 1 and 2, not 0 and 1)
        weapPtr.reset(new SimWeaponClass(WeaponDataTable[rail->hardPoint.weaponId].Index + VU_LAST_ENTITY_TYPE));

        if (weapPtr)
        {
            weapPtr->SetRackSlot(rail->hardPoint.NumPoints() - (i + 1));

            if (lastPtr)
            {
                weapPtr->nextOnRail = lastPtr;
            }

            lastPtr = weapPtr;
        }
    }

    rail->hardPoint.weaponPointer = weapPtr;
    int *lo = rail->hardPoint.GetLoadOrder();

    if (lo)
    {
        for (i = 0; i < rail->weaponCount and weapPtr; i++)
        {
            weapPtr->SetRackSlot(lo[i]);
            weapPtr = weapPtr->nextOnRail;
        }
    }

    rail->hardPoint.AttachAllWeaponBSP();

    // if(g3dObjectID == VIS_F16C)
    // Check_HTS_Tirn((DrawableBSP*)Plane->object,&gCurStores[plane]);
    /*

    if(rail->rackID)
    {
     rackPtr=&Falcon4ClassTable[rail->rackID];
     Rack=gUIViewer->LoadBSP((plane << 24) + (hardpoint << 16),rackPtr->visType[0]);
     if(Rack)
     {
     ((DrawableBSP*)Plane->object)->AttachChild(((DrawableBSP*)Rack->object),hardpoint-1);

     if(rail->weaponID and rail->startBits)
     {
     weapPtr=&Falcon4ClassTable[rail->weaponID];
     bits=rail->startBits;
     i=0;
     while(bits)
     {
     Weapon=gUIViewer->LoadBSP((plane << 24) + (hardpoint << 16) + (i+1),weapPtr->visType[0]);
     if(Weapon)
     // JB 020314 work from the back to the font (like the 3d code)
     ((DrawableBSP*)Rack->object)->AttachChild(((DrawableBSP*)Weapon->object),
     ((DrawableBSP*)Rack->object)->instance.ParentObject->nSlots - i - 1);
     bits >>= 1;
     i++;
     }
     }
     }
    }
    else if(rail->weaponID)
    {
     if(rail->startBits)
     {
     weapPtr=&Falcon4ClassTable[rail->weaponID];
     Weapon=gUIViewer->LoadBSP((plane << 24) + (hardpoint << 16) + 1,weapPtr->visType[0]);
     if(Weapon)
     ((DrawableBSP*)Plane->object)->AttachChild(((DrawableBSP*)Weapon->object),hardpoint-1);
     }
    }
    */
}

void LoadHardPoint(long plane, long num, long center)
{
    Falcon4EntityClassType* classPtr;
    BSPLIST *Plane;
    BSPLIST *Rack;
    BSPLIST *Weapon;
    int i;

    Plane = gUIViewer->Find((plane << 24));

    if (Plane == NULL) return;

    if ( not (VisFlag bitand (1 << num))) return;

    Rack = gUIViewer->Find((plane << 24) + (num << 16));

    for (i = 0; i < LastCount[plane][num]; i++)
    {
        Weapon = gUIViewer->Find((plane << 24) + (num << 16) + i + 1);

        if (Rack and Weapon)
        {
            if (LastCount[plane][num] == 2)
            {
                if (num > center)
                {
                    if ( not i)
                        ((DrawableBSP*)Rack->object)->DetachChild(((DrawableBSP*)Weapon->object), i + 2);
                    else
                        ((DrawableBSP*)Rack->object)->DetachChild(((DrawableBSP*)Weapon->object), i);
                }
                else
                    ((DrawableBSP*)Rack->object)->DetachChild(((DrawableBSP*)Weapon->object), i);
            }
            else
                ((DrawableBSP*)Rack->object)->DetachChild(((DrawableBSP*)Weapon->object), i);

            gUIViewer->Remove((plane << 24) + (num << 16) + i + 1);
        }
        else if (Weapon)
        {
            ((DrawableBSP*)Plane->object)->DetachChild(((DrawableBSP*)Weapon->object), num - 1);
            gUIViewer->Remove((plane << 24) + (num << 16) + i + 1);
        }

        Weapon = NULL;
    }

    if (Rack)
    {
        ((DrawableBSP*)Plane->object)->DetachChild(((DrawableBSP*)Rack->object), num - 1);
        gUIViewer->Remove((plane << 24) + (num << 16));
        Rack = NULL;
    }

    if (RackFlag bitand (1 << num))
    {
        if (gCurStores[plane].WeaponCount[num] == 1)
            Rack = gUIViewer->LoadBSP((plane << 24) + (num << 16), VIS_SINGLE_RACK);
        else if (gCurStores[plane].WeaponCount[num] > 1)
            Rack = gUIViewer->LoadBSP((plane << 24) + (num << 16), VIS_TRIPLE_RACK);

        if (Rack)
            ((DrawableBSP*)Plane->object)->AttachChild(((DrawableBSP*)Rack->object), num - 1);
    }

    classPtr = &Falcon4ClassTable[WeaponDataTable[gCurStores[plane].WeaponID[num]].Index];

    if (classPtr)
    {
        for (i = 0; i < gCurStores[plane].WeaponCount[num] and i < _WPN_MAX_; i++)
        {
            Weapon = gUIViewer->LoadBSP((plane << 24) + (num << 16) + i + 1, classPtr->visType[0]);

            if (Weapon)
            {
                if (Rack)
                {
                    if (gCurStores[plane].WeaponCount[num] == 2)
                    {
                        if (num > center)
                        {
                            if ( not i)
                                ((DrawableBSP*)Rack->object)->AttachChild(((DrawableBSP*)Weapon->object), i + 2);
                            else
                                ((DrawableBSP*)Rack->object)->AttachChild(((DrawableBSP*)Weapon->object), i);
                        }
                        else
                            ((DrawableBSP*)Rack->object)->AttachChild(((DrawableBSP*)Weapon->object), i);
                    }
                    else
                        ((DrawableBSP*)Rack->object)->AttachChild(((DrawableBSP*)Weapon->object), i);
                }
                else
                    ((DrawableBSP*)Plane->object)->AttachChild(((DrawableBSP*)Weapon->object), num - 1);
            }
        }

        LastCount[plane][num] = gCurStores[plane].WeaponCount[num];
    }
}

void LoadFlight(VU_ID flightID)
{
    Flight flt;
    VehicleClassDataType* vc;
    Falcon4EntityClassType* classPtr;
    BSPLIST *obj;
    int vid, v, i, j, ac, loads;

    if (gStores)
    {
        gStores->Cleanup();
        delete gStores;
    }

    memset(LastCount, 0, sizeof(LastCount));

    gStores = new StoresList;

    flt = (Flight)FindUnit(flightID);

    if (flt == NULL) return;

    for (i = 0; i < 5; i++)
    {
        memset(&gCurStores[i], 0, sizeof(LoadoutStruct));
        memset(&gOriginalStores[i], 0, sizeof(LoadoutStruct));
    }

    vid = flt->GetVehicleID(0);
    vc = GetVehicleClassData(vid);
    classPtr = &Falcon4ClassTable[vid];

    gVCPtr = vc;
    gVehID = vid;

    RackFlag = vc->RackFlags;
    VisFlag = vc->VisibleFlags;
    g3dObjectID = classPtr->visType[0];

    for (i = HARDPOINT_MAX - 1; i >= 0; i--)
    {
        if (vc->Weapon[i] not_eq 0)
            break;
    }

    if (i >= 0)
        i ++;
    else
        i = 0;

    HardPoints = i;
    ac = flt->GetTotalVehicles();
    loads = flt->GetNumberOfLoadouts();

    // JB 020219 Limit munition planecount to less than five otherwise we overwrite memory.
    ShiAssert(ac > 0 and loads > 0 and ac < 4);

    // save info from what is in the flight
    for (v = 0; v < ac and v < 4; v++)
    {
        if (v < loads)
            memcpy(&gCurStores[v], flt->GetLoadout(v), sizeof(LoadoutStruct));
        else
            memcpy(&gCurStores[v], flt->GetLoadout(0), sizeof(LoadoutStruct));
    }

    memcpy(&gCurStores[4], flt->GetLoadout(0), sizeof(LoadoutStruct));

    for (i = 0; i < 5; ++i) // save what we originally came into the screen with
    {
        gOriginalStores[i] = gCurStores[i];
    }

    gMainHandler->EnterCritical();

    for (i = 0; i < PlaneCount; i++)
    {
        obj = gUIViewer->LoadBSP((i << 24), classPtr->visType[0], TRUE);

        ShiAssert(obj);

        if ( not i)
            Object.PosZ = 0;

        if (classPtr->visType[0] == MapVisId(VIS_F16C) or
            (((DrawableBSP*)obj->object)->instance.ParentObject->nSwitches >= 10 and 
             ((DrawableBSP*)obj->object)->instance.ParentObject->nDOFs >= 24))
        {
            // F16 switches/DOFS

            // MLR 12/26/2003 - fix loadout LOD - now the gear is closed up
            ((DrawableBSP*)obj->object)->SetSwitchMask(5, 1);
            ((DrawableBSP*)obj->object)->SetSwitchMask(10, 1);
            ((DrawableBSP*)obj->object)->SetSwitchMask(31, 1);

            /* // MLR 12/26/2003 - I commented all this out.
            ((DrawableBSP*)obj->object)->SetSwitchMask(1, 1); // Landing Gear stuff
            ((DrawableBSP*)obj->object)->SetSwitchMask(2, 1); //
            ((DrawableBSP*)obj->object)->SetSwitchMask(3, 1); //
            ((DrawableBSP*)obj->object)->SetSwitchMask(4, 1); //

            //((DrawableBSP*)obj->object)->SetSwitchMask(8, TRUE); // Lights (other than landing)

            ((DrawableBSP*)obj->object)->SetSwitchMask(10, 1); // Afterburner

            // More landing gear stuff
            ((DrawableBSP*)obj->object)->SetDOFangle(19,90.0f * DTR);
            ((DrawableBSP*)obj->object)->SetDOFangle(22,90.0f * DTR);
            ((DrawableBSP*)obj->object)->SetDOFangle(20,75.0f * DTR);
            ((DrawableBSP*)obj->object)->SetDOFangle(21,75.0f * DTR);
            ((DrawableBSP*)obj->object)->SetDOFangle(23,75.0f * DTR);
            ((DrawableBSP*)obj->object)->SetDOFangle(24,75.0f * DTR);
            */
        }
        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER)
        {
            ((DrawableBSP*)obj->object)->SetSwitchMask(0, 2); // Turn on rotors

            if ( not i)
                Object.PosZ = -5;


            // RV - Biker - Use switch 24 because we need switch 2 for gear stuff
            // only do so if we have HPs to put some weapons
            //if(classPtr->visType[0] == MapVisId(VIS_UH60L))
            //((DrawableBSP*)obj->object)->SetSwitchMask(2, 1); // Landing Gear
            if (HardPoints > 1)
                ((DrawableBSP*)obj->object)->SetSwitchMask(24, 1);
        }
        else
        {
            // Non F16 switches/DOFS
            // MLR 12/27/2003 - disable this too
            //((DrawableBSP*)obj->object)->SetSwitchMask(2, 1); // Landing Gear
            //((DrawableBSP*)obj->object)->SetSwitchMask(1, 1); // Lights (other than landing)
        }

        //TJL 01/03/04 If player changes a skin, then always show what was last selected.
        //Since set3DTexture is a global, it stays sets during the same session.
        //This makes sure the same texture is displayed when returning from the 3D world as well.
        if (set3DTexture not_eq -1)
            ((DrawableBSP*)obj->object)->SetTextureSet(set3DTexture);

        // Figure out the weapons

        for (j = 1; j < HardPoints; j++)
        {
            //if (g_bNewRackData)
            GetJRackAndWeapon(vc, classPtr, gCurStores[i].WeaponID[j], gCurStores[i].WeaponCount[j], static_cast<short>(j), &gCurRails[i].rail[j]);
            //else
            //GetRackAndWeapon(vc,static_cast<short>(vid),gCurStores[i].WeaponID[j],gCurStores[i].WeaponCount[j],static_cast<short>(j),static_cast<short>(HardPoints/2),&gCurRails[i].rail[j]);
        }

        for (j = 1; j < HardPoints; j++)
        {
            LoadHardPoint(i, j, HardPoints / 2, &gCurRails[i].rail[j]);
        }

        if ( not i)
        {
            Object.Heading = 180.0f;
            Object.Pitch = -10.0f;
            Object.Distance = ((DrawableBSP*)obj->object)->Radius() * 3;
            Object.Direction = 0.0f;

            Object.MinDistance = ((DrawableBSP*)obj->object)->Radius() + 20;
            Object.MaxDistance = ((DrawableBSP*)obj->object)->Radius() * 10;
            Object.MinPitch = 0;
            Object.MaxPitch = 0;
            Object.CheckPitch = FALSE;

            Object.PosX = 0;
            Object.PosY = 0;
        }

        PositandOrientSetData(Object.PosX, Object.PosY, Object.PosZ, 0.0f, 0.0f, 0.0f, &objPos, &objRot);
        ((DrawableBSP*)obj->object)->Update(&objPos, &objRot);
    }

    TallyStores();
    gMainHandler->LeaveCritical();
}

//TJL 01/02/04 Change Skin Function
void ChangeSkin()
{
	BSPLIST* obj = NULL;
    long plane = 0;
    int i;

    for (i = 0; i < PlaneCount; i++)
    {
        obj = gUIViewer->Find((i << 24));
    }

    if (obj)
    {
        int newtext;
        newtext = ((DrawableBSP*)obj->object)->GetNTextureSet() - 1;

        if (newtext >= prevtext1)
        {
            prevtext1++;
        }

        if (prevtext1 > newtext)
        {
            prevtext1 = 0;
        }
    }

    //TJL step through again and apply the skins
    //This sets the skins on each aircraft in the flight
    for (i = 0; i < PlaneCount; i++)
    {
        obj = gUIViewer->Find((i << 24));
        ((DrawableBSP*)obj->object)->SetTextureSet(prevtext1);
        set3DTexture = prevtext1;
    }

}
//end

void SetPlaneToArm(long Plane, BOOL ArmIt)
{
    int i;
    PlaneEditList[Plane] = ArmIt;

    // JB 020219 Limit munition planecount to less than five otherwise we overwrite memory.
    for (i = 0; i < PlaneCount and i < 4; i++)
        if (PlaneEditList[i])
        {
            FirstPlane = i;
            return;
        }
}

BOOL MuniTimeCB(C_Base *control)
{
    C_Window *win;
    Flight flt;
    long takeoff;
    C_Text *txt;
    _TCHAR buf[200];

    if ((vuxGameTime - control->GetUserNumber(0)) < VU_TICS_PER_SECOND)
        return(FALSE);

    control->SetUserNumber(0, vuxGameTime);

    txt = (C_Text*)control;

    if (txt)
    {
        flt = (Flight)vuDatabase->Find(gLoadoutFlightID);

        if (flt)
        {
            // update weapon loadout if things have changed
            int numac = flt->GetTotalVehicles();
            int loads = flt->GetNumberOfLoadouts();
            bool ref  = false;

            for (int aci = 0; aci < numac; aci++)
            {
                if (aci < loads)
                {
                    LoadoutStruct flightLOS;

                    memcpy(&flightLOS, flt->GetLoadout(aci), sizeof(LoadoutStruct));

                    for (int hpi = 0; hpi < HARDPOINT_MAX ; ++hpi)
                    {
                        if ((gOriginalStores[aci].WeaponID[hpi] not_eq flightLOS.WeaponID[hpi]) or
                            (gOriginalStores[aci].WeaponCount[hpi] not_eq flightLOS.WeaponCount[hpi]))
                        {
                            // update the info for the loadout
                            gOriginalStores[aci].WeaponID[hpi] = flightLOS.WeaponID[hpi];
                            gOriginalStores[aci].WeaponCount[hpi] = flightLOS.WeaponCount[hpi];
                            gCurStores[aci].WeaponID[hpi] = flightLOS.WeaponID[hpi];
                            gCurStores[aci].WeaponCount[hpi] = flightLOS.WeaponCount[hpi];

                            ref = true;
                        }
                    }
                }
            }

            // check flight time till launch
            if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
            {
                takeoff = 1;
            }
            else
            {
                takeoff = (flt->GetFirstUnitWP()->GetWPDepartureTime() - vuxGameTime);
            }

            // 2001-10-23 MODIFIED BY S.G. Now either the original code or we are in Tac edit can go in and don't use takeoff but do just like in 'SetupMunitionsWindow' to get the time
            // if((takeoff / VU_TICS_PER_SECOND) > g_nLoadoutTimeLimit) // JB 010729
            // {
            // GetTimeString(takeoff,buf);
            // txt->Refresh();
            // txt->SetText(buf);
            if ((takeoff / VU_TICS_PER_SECOND) > g_nLoadoutTimeLimit or (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)) // JB 010729
            {
                txt->Refresh();

                if (flt->GetFirstUnitWP())
                {
                    GetTimeString(flt->GetFirstUnitWP()->GetWPDepartureTime(), buf);
                    txt->SetText(buf);
                }
                else
                {
                    txt->SetText("");
                }

                // END OF MODIFIED SECTION  2001-10-23

                if ((takeoff / VU_TICS_PER_SECOND) > 240)
                    txt->SetFGColor(0x00ff00);
                else if ((takeoff / VU_TICS_PER_SECOND) > 180)
                    txt->SetFGColor(0x00ffff);
                else
                    txt->SetFGColor(0x0000ff);

                txt->Refresh();
            }
            else
            {
                txt->SetFlagBitOff(C_BIT_TIMER);
                txt->Refresh();
                txt->SetText(TXT_INFLIGHT);
                txt->SetFGColor(0x0000ff);
                txt->Refresh();
                win = txt->GetParent();

                if (win)
                    win->DisableCluster(-100);

                win->RefreshWindow();
            }

            if (ref)
            {
                // win=txt->GetParent();

                // if (win)
                // {
                SetCurrentLoadout();
                control->Parent_->RefreshClient(2);
                // win->ScanClientAreas();
                // win->RefreshWindow();
                // }
            }
        }
        else
        {
            txt->SetFlagBitOff(C_BIT_TIMER);
            txt->Refresh();
            txt->SetText(" ");
            txt->Refresh();
        }
    }

    return(TRUE);
}

void DetermineWeight(VU_ID FlightID)
{
    VehicleClassDataType *vc;
    Falcon4EntityClassType *rackPtr, *weapPtr;
    WeaponClassDataType  *wc;
    STORESLIST *store;
    Flight flt;
    long i, j, vid, PlaneCount/*,count,bitflag*/;

    flt = (Flight)FindUnit(FlightID);

    if ( not flt)
    {
        for (i = 0; i < 4; i++)
        {
            _MAX_WEIGHT_[i] = 0;
            _CLEAN_WEIGHT_[i] = 0;
            _DRAG_FACTOR_[i] = 0;
            _MUNITIONS_WEIGHT_[i] = 0;
            _FUEL_WEIGHT_[i] = 0;
            _CURRENT_WEIGHT_[i] = 0;
        }

        gFlightOverloaded = 0;
        return;
    }

    vid = flt->GetVehicleID(0);
    PlaneCount = flt->GetTotalVehicles();
    vc = GetVehicleClassData(vid);
    gFlightOverloaded = 0;

    for (i = 0; i < 4; i++)
    {
        _MAX_WEIGHT_[i] = vc->MaxWt;
        _CLEAN_WEIGHT_[i] = vc->EmptyWt;
        _DRAG_FACTOR_[i] = 1;
        _MUNITIONS_WEIGHT_[i] = 0;
        _FUEL_WEIGHT_[i] = vc->FuelWt;

        if (i < PlaneCount)
        {
            for (j = 0; j < QuantityCount[i]; j++)
            {
                store = NULL;
                ShiAssert(gStores);

                if (gStores)
                    store = gStores->Find(Quantity[i][0][j]);

                if (store)
                {
                    if (store->Type == StoresList::_TYPE_FUEL_)
                        _FUEL_WEIGHT_[i] += store->Fuel * Quantity[i][1][j];

                    // _MUNITIONS_WEIGHT_[i]+=store->Weight * Quantity[i][1][j];
                    // _DRAG_FACTOR_[i]+=store->DragFactor * Quantity[i][1][j];
                }
            }

            // MLR 3/1/2004 - eeheeheehee
            for (j = 1; j < HardPoints; j++)
            {
                // Add up pylon weight bitand drag
                int pylonid = gCurRails[i].rail[j].hardPoint.GetPylonId();

                if (pylonid and gCurRails[i].rail[j].weaponCount)
                {
                    rackPtr = &Falcon4ClassTable[WeaponDataTable[pylonid].Index]; // MLR 2/29/2004 -

                    if (rackPtr)
                    {
                        wc = (WeaponClassDataType *)rackPtr->dataPtr;

                        if (wc)
                        {
                            _MUNITIONS_WEIGHT_[i] += (wc->Weight);

                            if (vc->VisibleFlags bitand (1 << j)) // only do drag if it's visible
                                _DRAG_FACTOR_[i] += wc->DragIndex;
                        }
                    }
                }

                // Add rack weight bitand drag
                int rackid = gCurRails[i].rail[j].hardPoint.GetRackId();

                if (rackid and gCurRails[i].rail[j].weaponCount)
                {
                    rackPtr = &Falcon4ClassTable[WeaponDataTable[rackid].Index]; // MLR 2/29/2004 -

                    if (rackPtr)
                    {
                        wc = (WeaponClassDataType *)rackPtr->dataPtr;

                        if (wc)
                        {
                            _MUNITIONS_WEIGHT_[i] += (wc->Weight);

                            if (vc->VisibleFlags bitand (1 << j)) // only do drag if it's visible
                                _DRAG_FACTOR_[i] += wc->DragIndex;
                        }
                    }
                }

                // Add weapon(s) weight bitand drag
                int weapid = gCurRails[i].rail[j].hardPoint.weaponId;

                if (weapid and gCurRails[i].rail[j].weaponCount)
                {
                    weapPtr = &Falcon4ClassTable[WeaponDataTable[weapid].Index];

                    if (weapPtr)
                    {
                        wc = (WeaponClassDataType *)weapPtr->dataPtr;

                        if (wc)
                        {

                            _MUNITIONS_WEIGHT_[i] += wc->Weight * gCurRails[i].rail[j].weaponCount;

                            if (vc->VisibleFlags bitand (1 << j)) // only do drag if it's visible
                                _DRAG_FACTOR_[i] += wc->DragIndex * gCurRails[i].rail[j].weaponCount;
                        }
                    }
                }
            }


#if 0

            for (j = 1; j < HardPoints; j++)
            {
                if (gCurRails[i].rail[j].hardPoint.GetRackId() and gCurRails[i].rail[j].currentBits) // MLR 2/25/2004 - Added currentBits
                {
                    rackPtr = &Falcon4ClassTable[gCurRails[i].rail[j].hardPoint.GetRackId()]; // MLR 2/29/2004 -

                    if (rackPtr)
                    {
                        wc = (WeaponClassDataType *)rackPtr->dataPtr;

                        if (wc)
                        {
#if 0 // MLR 2/25/2004 - Change to weight computation.

                            if (gCurRails[i].rail[j].hardPoint.weaponId) // MLR 2/29/2004 -
                                _MUNITIONS_WEIGHT_[i] += (wc->Weight * 0.85);
                            else
                                _MUNITIONS_WEIGHT_[i] += wc->Weight;

#else

                            if (gCurRails[i].rail[j].hardPoint.weaponId) // MLR 2/29/2004 -
                                _MUNITIONS_WEIGHT_[i] += (wc->Weight);

#endif
                            _DRAG_FACTOR_[i] += wc->DragIndex;
                        }
                    }
                }
            }

            // Add up visible stores weights
            for (j = 1; j < HardPoints; j++)
            {
                if (gCurRails[i].rail[j].hardPoint.weaponId and gCurRails[i].rail[j].currentBits)
                {
                    bitflag = gCurRails[i].rail[j].currentBits;
                    count = 0;

                    while (bitflag)
                    {
                        if (bitflag bitand 1)
                            count++;

                        bitflag >>= 1;
                    }

                    weapPtr = &Falcon4ClassTable[gCurRails[i].rail[j].hardPoint.weaponId];

                    if (weapPtr)
                    {
                        wc = (WeaponClassDataType *)weapPtr->dataPtr;

                        if (wc)
                        {
                            _MUNITIONS_WEIGHT_[i] += wc->Weight * count;
                            _DRAG_FACTOR_[i] += wc->DragIndex * count; // MLR 2/25/2004 - added "* count"
                        }
                    }
                }
            }

            // Add up internal stores weights
            for (j = 1; j < HardPoints; j++)
            {
                if ( not (vc->VisibleFlags bitand (1 << j)) and gCurStores[i].WeaponID[j])
                {
                    store = NULL;
                    ShiAssert(gStores);

                    if (gStores)
                        store = gStores->Find(gCurStores[i].WeaponID[j]);

                    if (store)
                        _MUNITIONS_WEIGHT_[i] += store->Weight * gCurStores[i].WeaponCount[j];
                }
            }

#endif
        }

        _CURRENT_WEIGHT_[i] = _CLEAN_WEIGHT_[i] + _MUNITIONS_WEIGHT_[i] + _FUEL_WEIGHT_[i];

        if (_CURRENT_WEIGHT_[i] > _MAX_WEIGHT_[i])
        {
            gFlightOverloaded or_eq 1 << i;
        }
    }
}

//void UpdateMunitionsWindowInfo(const VU_ID flightID)
//{
//}

void SetupMunitionsWindow(VU_ID FlightID)
{
    C_Window *win = NULL;
    C_Button *btn = NULL;
    C_Text *txt = NULL;
    Flight flt;
    long vid = 0, status = 0;
    _TCHAR buf[200];
    int i = 0;
    long takeoff = 0;
    VehicleClassDataType *vc = NULL;
    FalconSessionEntity *session = NULL;

    win = gMainHandler->FindWindow(MUNITIONS_WIN);

    if (win == NULL)
        return;

    flt = (Flight)FindUnit(FlightID);

    if (flt == NULL)
    {
        txt = (C_Text*)win->FindControl(FLIGHT_CALLSIGN);

        if (txt)
            txt->SetText(" ");

        txt = (C_Text*)win->FindControl(STATUS_FIELD);

        if (txt)
            txt->SetText(" ");

        for (i = 0; i < 4; i++)
            SetPlaneToArm(i, FALSE);

        btn = (C_Button *)win->FindControl(AIR_1);

        if (btn)
        {
            btn->Refresh();
            btn->SetState(0);
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->SetAllLabel(" ");
            btn->Refresh();
        }

        btn = (C_Button *)win->FindControl(AIR_2);

        if (btn)
        {
            btn->Refresh();
            btn->SetState(0);
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->SetAllLabel(" ");
            btn->Refresh();
        }

        btn = (C_Button *)win->FindControl(AIR_3);

        if (btn)
        {
            btn->Refresh();
            btn->SetState(0);
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->SetAllLabel(" ");
            btn->Refresh();
        }

        btn = (C_Button *)win->FindControl(AIR_4);

        if (btn)
        {
            btn->Refresh();
            btn->SetState(0);
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->SetAllLabel(" ");
            btn->Refresh();
        }

        win->DisableCluster(-100);
        return;
    }

    txt = (C_Text*)win->FindControl(FLIGHT_CALLSIGN);

    if (txt)
    {
        GetCallsign(flt, buf);
        txt->SetText(buf);
    }

    status = 0;
    txt = (C_Text*)win->FindControl(STATUS_FIELD);

    if (txt)
    {
        txt->SetTimerCallback(MuniTimeCB);

        if (( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)) and ( not (TheCampaign.Flags bitand CAMP_LIGHT)))
        {
            status = GetFlightStatusID(flt);

            if ( not status)
            {
                takeoff = flt->GetFirstUnitWP()->GetWPDepartureTime() - vuxGameTime;

                //if(takeoff < 120)
                if ((takeoff / VU_TICS_PER_SECOND) < g_nLoadoutTimeLimit) // JB 010729
                    status = 1;
            }
        }

        if (status)
        {
            txt->SetText(TXT_INFLIGHT);
            txt->SetFGColor(0x0000ff);
        }
        else
        {
            if (win)
                win->EnableCluster(-100);

            txt->SetFlagBitOn(C_BIT_TIMER);

            if (flt->GetFirstUnitWP())
            {
                GetTimeString(flt->GetFirstUnitWP()->GetWPDepartureTime(), buf);
                txt->SetText(buf);
            }
            else
            {
                txt->SetText("");
            }

            // 2001-10-23 ADDED BY S.G. Set the time color according to the time until takeoff
            if ((takeoff / VU_TICS_PER_SECOND) > 240)
                txt->SetFGColor(0x00ff00);
            else if ((takeoff / VU_TICS_PER_SECOND) > 180)
                txt->SetFGColor(0x00ffff);
            else
                txt->SetFGColor(0x0000ff);

            txt->Refresh();
            // END OF ADDED SECTION 2001-10-23
        }
    }

    txt = (C_Text*)win->FindControl(FLIGHT_STATUS);

    if (txt)
    {
        txt->Refresh();

        if (status)
            txt->SetText(TXT_FLIGHT_STATUS);
        else
            txt->SetText(TXT_FLIGHT_TAKEOFF);
    }

    if (status)
        win->DisableCluster(-100);
    else
        win->EnableCluster(-100);

    vid = flt->GetVehicleID(0);
    PlaneCount = flt->GetTotalVehicles();
    vc = GetVehicleClassData(vid);

    for (i = 0; i < 4; i++)
    {
        if (i < PlaneCount)
        {
            switch (i)
            {
                case 0:
                    btn = (C_Button *)win->FindControl(AIR_1);
                    break;

                case 1:
                    btn = (C_Button *)win->FindControl(AIR_2);
                    break;

                case 2:
                    btn = (C_Button *)win->FindControl(AIR_3);
                    break;

                case 3:
                    btn = (C_Button *)win->FindControl(AIR_4);
                    break;
            }

            if (btn)
            {
                btn->Refresh();
                btn->SetState(1);
                btn->SetFlagBitOn(C_BIT_ENABLED);
                session = gCommsMgr->FindCampaignPlayer(flt->Id(), static_cast<uchar>(i));

                if (session)
                {
                    _stprintf(buf, "%s", session->GetPlayerName());
                }
                else
                {
                    TheCampaign.MissionEvaluator->GetPilotName(i, buf);
                }

                btn->SetAllLabel(buf);
                btn->Refresh();  //@ mark here:  Set pilots
            }

            SetPlaneToArm(i, TRUE);
        }
        else
        {
            switch (i)
            {
                case 0:
                    btn = (C_Button *)win->FindControl(AIR_1);
                    break;

                case 1:
                    btn = (C_Button *)win->FindControl(AIR_2);
                    break;

                case 2:
                    btn = (C_Button *)win->FindControl(AIR_3);
                    break;

                case 3:
                    btn = (C_Button *)win->FindControl(AIR_4);
                    break;
            }

            if (btn)
            {
                btn->Refresh();
                btn->SetState(0);
                btn->SetFlagBitOff(C_BIT_ENABLED);
                btn->SetAllLabel(" ");
                btn->Refresh();
            }

            SetPlaneToArm(i, FALSE);
        }
    }

    win->ScanClientAreas();
    win->RefreshWindow();
}

void UpdateStoresTally(C_Window *win)
{
    long i, wid, color, avail, availID;
    CONTROLLIST *cur;
    C_Text *txt;
    _TCHAR buf[STRING_BUFFER_SIZE];

    TallyStores();

    cur = win->GetControlList();

    while (cur)
    {
        // Update Inventory Numbers (Out -> High)
        if ((cur->Control_->GetID() bitand 0xff000000) == (1 << 25))
        {
            cur->Control_->Refresh();
            avail = TotalAvailable(static_cast<short>(cur->Control_->GetID() bitand 0x0000ffff));

            if ( not avail)
            {
                availID = TXT_SUPPLY_OUT;
                color = 0x0000ff;
            }
            else if (avail < 700)
            {
                availID = TXT_SUPPLY_LOW;
                color = 0x00ffff;
            }
            else if (avail < 1500)
            {
                availID = TXT_SUPPLY_MEDIUM;
                color = 0xeeeeee;
            }
            else
            {
                availID = TXT_SUPPLY_HIGH;
                color = 0x00ff00;
            }

            ((C_Text*)cur->Control_)->SetText(availID);
            ((C_Text*)cur->Control_)->SetFGColor(color);
            cur->Control_->Refresh();
        }

        // Update Onboard Count
        if ((cur->Control_->GetID() bitand 0x0f000000) == (1 << 24))
        {
            cur->Control_->Refresh();
            _tcscpy(buf, " ");
            ((C_Text*)cur->Control_)->SetText(buf);

            wid = -1;

            for (i = 0; i < QuantityCount[FirstPlane]; i++)
            {
                if (Quantity[FirstPlane][0][i] == (cur->Control_->GetID() bitand 0x00ffffff))
                {
                    wid = i;
                    break;
                }
            }

            if (wid not_eq -1)
                if (Quantity[FirstPlane][1][wid])
                {
                    _stprintf(buf, "%1d", Quantity[FirstPlane][1][wid]);
                    ((C_Text*)cur->Control_)->SetText(buf);
                }

            cur->Control_->Refresh();
        }

        cur = cur->Next;
    }

    DetermineWeight(gLoadoutFlightID);

    txt = (C_Text*)win->FindControl(MAX_WEIGHT);

    if (txt)
    {
        _stprintf(buf, "%1d", _MAX_WEIGHT_[FirstPlane]);
        txt->SetText(buf);
        txt->Refresh();
    }

    txt = (C_Text*)win->FindControl(CURRENT_WEIGHT);

    if (txt)
    {
        if (_CURRENT_WEIGHT_[FirstPlane] > _MAX_WEIGHT_[FirstPlane])
            txt->SetFGColor(0x0000ff);
        else
            txt->SetFGColor(0xd9a051);

        _stprintf(buf, "%1d", _CURRENT_WEIGHT_[FirstPlane]);
        txt->SetText(buf);
        txt->Refresh();
    }

    txt = (C_Text*)win->FindControl(CLEAN_WEIGHT);

    if (txt)
    {
        _stprintf(buf, "%1d", _CLEAN_WEIGHT_[FirstPlane]);
        txt->SetText(buf);
        txt->Refresh();
    }

    txt = (C_Text*)win->FindControl(DRAG_FACTOR);

    if (txt)
    {
        _stprintf(buf, "%5.1f", _DRAG_FACTOR_[FirstPlane]);
        Uni_Float(buf);
        txt->SetText(buf);
        txt->Refresh();
    }

    txt = (C_Text*)win->FindControl(MUNITIONS_WEIGHT);

    if (txt)
    {
        _stprintf(buf, "%1d", _MUNITIONS_WEIGHT_[FirstPlane]);
        txt->SetText(buf);
        txt->Refresh();
    }

    txt = (C_Text*)win->FindControl(FUEL_WEIGHT);

    if (txt)
    {
        _stprintf(buf, "%1d", _FUEL_WEIGHT_[FirstPlane]);
        txt->SetText(buf);
        txt->Refresh();
    }
}

void InternalArmPlaneCB(long ID, short hittype, C_Base *control)
{
    long hp, count, weaponID, startcount;
    CONTROLLIST *cur;
    STORESLIST *store;
    int i;

    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    hp = ID >> 16;
    weaponID = ID bitand 0x0000ffff;
    store = NULL;
    ShiAssert(gStores);

    if (gStores)
        store = gStores->Find(weaponID);

    if (store == NULL)
        return;

    count = control->GetUserNumber(0);
    startcount = count;

    if (control->GetRelY() < control->GetH() / 2)
        count++;
    else
        count--;

    if (count < 0)
        count = 0;

    if (count > control->GetUserNumber(1))
        count = control->GetUserNumber(1);

    if (count and not TotalAvailable(static_cast<short>(weaponID)))
        count--;

    if ( not count and count == startcount) //None available
    {
        return;
    }

    cur = control->Parent_->GetControlList();

    while (cur)
    {
        if (cur->Control_->GetGroup() == hp)
        {
            if (count)
            {
                cur->Control_->SetState(C_STATE_DISABLED);
                cur->Control_->SetUserNumber(0, 0);
            }
            else
            {
                cur->Control_->SetState(0);
                cur->Control_->SetUserNumber(0, 0);
            }

            cur->Control_->Refresh();
        }

        cur = cur->Next;
    }

    if (count)
        control->SetState(2);
    else
        control->SetState(0);

    control->SetUserNumber(0, count);

    for (i = 0; i < 4; i++)
    {
        if (PlaneEditList[i])
        {
            gCurStores[i].WeaponID[hp] = static_cast<short>(weaponID);

            if (count and not TotalAvailable(static_cast<short>(weaponID)))
                count--;

            gCurStores[i].WeaponCount[hp] = static_cast<uchar>(count);
        }
    }

    UpdateStoresTally(control->Parent_);

    control->Parent_->RefreshClient(2);
}

void ArmPlaneCB(long ID, short hittype, C_Base *control)
{
    long hp, count, weaponID, startcount;
    CONTROLLIST *cur;
    STORESLIST *store;
    int i;
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    hp = ID >> 16;
    weaponID = ID bitand 0x0000ffff;
    store = NULL;
    ShiAssert(gStores);

    if (gStores)
        store = gStores->Find(weaponID);

    if (store == NULL or not control)
        return;

    count = control->GetUserNumber(0);
    startcount = count;
    count++;

    if (count > control->GetUserNumber(1))
        count = 0;

    if (count and not TotalAvailable(static_cast<short>(weaponID)))
        count--;

    if ( not count and count == startcount) //None available
    {
        return;
    }

    Leave = UI_Enter(control->Parent_);
    cur = control->Parent_->GetControlList();

    while (cur)
    {
        if (cur->Control_->GetGroup() == hp)
        {
            if (count)
            {
                cur->Control_->SetState(C_STATE_DISABLED);
                cur->Control_->SetUserNumber(0, 0);
            }
            else
            {
                cur->Control_->SetState(0);
                cur->Control_->SetUserNumber(0, 0);
            }

            cur->Control_->Refresh();
        }

        cur = cur->Next;
    }

    short state = count << 1;

    if (state > C_STATE_20) // JPO - lock to max state
        state = C_STATE_20;

    control->SetState(state);
    control->SetUserNumber(0, count);

    for (i = 0; i < 4; i++)
    {
        if (PlaneEditList[i])
        {
            int ok;
            gCurStores[i].WeaponID[hp] = static_cast<short>(weaponID);

            if (count and not TotalAvailable(static_cast<short>(weaponID)))
                count--;

            gCurStores[i].WeaponCount[hp] = static_cast<uchar>(count);
            ClearHardPoint(i, hp, HardPoints / 2, &gCurRails[i].rail[hp]);
            //if (g_bNewRackData) {
            Falcon4EntityClassType* classPtr = &Falcon4ClassTable[gVehID];
            ok = GetJRackAndWeapon(gVCPtr, classPtr, gCurStores[i].WeaponID[hp], gCurStores[i].WeaponCount[hp], static_cast<short>(hp), &gCurRails[i].rail[hp]);
            //}
            //else
            // ok = GetRackAndWeapon(gVCPtr,static_cast<short>(gVehID),gCurStores[i].WeaponID[hp],gCurStores[i].WeaponCount[hp],static_cast<short>(hp),static_cast<short>(HardPoints/2),&gCurRails[i].rail[hp]);

            if (ok)
                LoadHardPoint(i, hp, HardPoints / 2, &gCurRails[i].rail[hp]);
        }
    }

    UpdateStoresTally(control->Parent_);

    control->Parent_->RefreshClient(2);
    UI_Leave(Leave);
}

void SetCurrentLoadout()
{
    int i, j, Diff;
    CONTROLLIST *cur;
    C_Window *win;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(MUNITIONS_WIN);

    if (win == NULL)
        return;

    Leave = UI_Enter(win);

    for (j = 1; j < HardPoints; j++)
    {
        cur = win->GetControlList();

        while (cur)
        {
            if (cur->Control_->GetGroup() == j)
            {
                if (cur->Control_->GetID() == ((j << 16) bitor gCurStores[FirstPlane].WeaponID[j]))
                {
                    Diff = 0;

                    for (i = 0; i < 4; i++)
                    {
                        if (PlaneEditList[i] and (gCurStores[FirstPlane].WeaponID[j] not_eq gCurStores[i].WeaponID[j] or gCurStores[FirstPlane].WeaponCount[j] not_eq gCurStores[i].WeaponCount[j]))
                            Diff = 1;
                    }

                    cur->Control_->SetUserNumber(0, gCurStores[FirstPlane].WeaponCount[j]);
                    short state = (gCurStores[FirstPlane].WeaponCount[j] << 1) + Diff;

                    if (state > C_STATE_20)
                    {
                        state = Diff ? C_STATE_19 : C_STATE_20;
                    }

                    cur->Control_->SetState(state);
                }
                else
                {
                    Diff = 0;

                    for (i = 0; i < 4; i++)
                    {
                        if (PlaneEditList[i] and (cur->Control_->GetID() == ((j << 16) bitor gCurStores[i].WeaponID[j]) and i not_eq FirstPlane))
                            Diff = 1;
                    }

                    cur->Control_->SetUserNumber(0, 0);

                    if (gCurStores[FirstPlane].WeaponCount[j] and not Diff)
                        cur->Control_->SetState(C_STATE_DISABLED);
                    else
                        cur->Control_->SetState(static_cast<short>(Diff));
                }

                cur->Control_->Refresh();
            }

            cur = cur->Next;
        }
    }

    UpdateStoresTally(win);
    UI_Leave(Leave);
}

void SetupLoadoutDisplay()
{
    Flight flt;
    Squadron sqd;
    VehicleClassDataType *vc;
    WeaponClassDataType  *wc;
    Falcon4EntityClassType* classPtr;
    int vid, i, j, slist = -1, wtype;
    long fuel, avail;
    STORESLIST *wpn;

    flt = (Flight)vuDatabase->Find(gLoadoutFlightID);

    if ( not flt)
        return;

    sqd = (Squadron)flt->GetUnitSquadron();

    if ( not sqd)
        return;

    vid = flt->GetVehicleID(0);
    vc = GetVehicleClassData(vid);

    if ( not vc)
        return;

    for (i = 1; i < HardPoints; i++)
    {
        if (vc->Weapon[i] and vc->Weapons[i])
        {
            if (vc->Weapons[i] == 255)
            {
                for (j = 0; j < MAX_WEAPONS_IN_LIST; j++)
                {
                    ShiAssert(gStores);

                    if (GetListEntryWeapon(vc->Weapon[i], j) and gStores)
                    {
                        wpn = gStores->Find(GetListEntryWeapon(vc->Weapon[i], j));

                        if (wpn == NULL)
                        {
                            if (TheCampaign.Flags bitand CAMP_TACTICAL)
                                avail = 2000;
                            else
                            {
                                avail = sqd->GetAvailableStores(GetListEntryWeapon(vc->Weapon[i], j));

                                switch (avail)
                                {
                                    case 0:
                                        break;

                                    case 1:
                                        avail = 500;
                                        break;

                                    case 2:
                                        avail = 1000;
                                        break;

                                    default:
                                        avail = 2000;
                                        break;
                                }
                            }

                            fuel = 0;
                            wc = &WeaponDataTable[GetListEntryWeapon(vc->Weapon[i], j)];
                            classPtr = &Falcon4ClassTable[wc->Index];

                            if (classPtr)
                            {
                                if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
                                {
                                    if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_MISSILE_AIR_AIR)
                                    {
                                        wtype = StoresList::_TYPE_MISSILE_;
                                        slist = StoresList::_AIR_TO_AIR_;
                                    }
                                    else
                                    {
                                        wtype = StoresList::_TYPE_MISSILE_;
                                        slist = StoresList::_AIR_TO_GROUND_;
                                    }
                                }
                                else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET or
                                         classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_LAUNCHER)
                                {
                                    wtype = StoresList::_TYPE_ROCKET_;
                                    slist = StoresList::_AIR_TO_GROUND_;
                                }
                                else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BOMB)
                                {
                                    wtype = StoresList::_TYPE_BOMB_;
                                    slist = StoresList::_AIR_TO_GROUND_;
                                }
                                else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_GUN)
                                {
                                    wtype = StoresList::_TYPE_GUN_;
                                    slist = StoresList::_AIR_TO_GROUND_;
                                }
                                else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_FUEL_TANK)
                                {
                                    wtype = StoresList::_TYPE_FUEL_;
                                    slist = StoresList::_OTHER_;
                                    fuel = wc->Strength;
                                }
                                else
                                {
                                    wtype = StoresList::_TYPE_OTHER_;
                                    slist = StoresList::_OTHER_;
                                }

                                wpn = gStores->Create(GetListEntryWeapon(vc->Weapon[i], static_cast<short>(j)), wc->Name, wtype, wc->Weight, fuel, wc->DragIndex, static_cast<short>(avail)); // add stores wgt,drag factor
                            }
                            else
                            {
                                wpn = gStores->Create(GetListEntryWeapon(vc->Weapon[i], static_cast<short>(j)), wc->Name, StoresList::_TYPE_OTHER_, wc->Weight, fuel, wc->DragIndex, static_cast<short>(avail)); // add stores wgt,drag factor
                            }

                            gStores->Add(wpn, slist);
                        }

                        wpn->HardPoint[i] = static_cast<short>(GetListEntryWeapons(vc->Weapon[i], j));
                    }
                }
            }
            else
            {
                wpn = NULL;
                ShiAssert(gStores);

                if (gStores)
                    wpn = gStores->Find(vc->Weapon[i]);

                if (wpn == NULL)
                {
                    if (TheCampaign.Flags bitand CAMP_TACTICAL)
                        avail = 2000;
                    else
                    {
                        avail = sqd->GetAvailableStores(vc->Weapon[i]);

                        switch (avail)
                        {
                            case 0:
                                break;

                            case 1:
                                avail = 500;
                                break;

                            case 2:
                                avail = 1000;
                                break;

                            default:
                                avail = 2000;
                                break;
                        }
                    }

                    fuel = 0;
                    wc = &WeaponDataTable[vc->Weapon[i]];
                    classPtr = &Falcon4ClassTable[wc->Index];

                    if (classPtr)
                    {
                        if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
                        {
                            if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_MISSILE_AIR_AIR)
                            {
                                wtype = StoresList::_TYPE_MISSILE_;
                                slist = StoresList::_AIR_TO_AIR_;
                            }
                            else
                            {
                                wtype = StoresList::_TYPE_MISSILE_;
                                slist = StoresList::_AIR_TO_GROUND_;
                            }
                        }
                        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET or
                                 classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_LAUNCHER)
                        {
                            wtype = StoresList::_TYPE_ROCKET_;
                            slist = StoresList::_AIR_TO_GROUND_;
                        }
                        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BOMB)
                        {
                            wtype = StoresList::_TYPE_BOMB_;
                            slist = StoresList::_AIR_TO_GROUND_;
                        }
                        else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_FUEL_TANK)
                        {
                            wtype = StoresList::_TYPE_FUEL_;
                            slist = StoresList::_OTHER_;
                            fuel = wc->Strength;
                        }
                        else
                        {
                            wtype = StoresList::_TYPE_OTHER_;
                            slist = StoresList::_OTHER_;
                        }

                        wpn = gStores->Create(vc->Weapon[i], wc->Name, wtype, wc->Weight, fuel, wc->DragIndex, static_cast<short>(avail)); // add stores wgt,drag factor
                    }
                    else
                    {
                        slist = StoresList::_OTHER_;
                        wpn = gStores->Create(vc->Weapon[i], wc->Name, StoresList::_TYPE_OTHER_, wc->Weight, fuel, wc->DragIndex, static_cast<short>(avail)); // add stores wgt,drag factor
                    }

                    gStores->Add(wpn, slist);
                }

                wpn->HardPoint[i] = vc->Weapons[i];
            }
        }
    }

    ShiAssert(gStores);

    if (gStores)
        gStores->Sort();
}

void MakeStoresList(C_Window *win, long client)
{
    C_ListBox *lbox;
    C_Text *txt;
    C_Button *btn;
    C_Line *line;
    STORESLIST *cur;
    BOOL ShowLoadedOnly = FALSE, Drawit;
    int GetType, x, y, i, j, hpnum, availID;
    long color, avail;
    _TCHAR buf[STRING_BUFFER_SIZE];
    F4CSECTIONHANDLE *Leave;

    if (win == NULL)
        return;

    Leave = UI_Enter(win);

    DeleteGroupList(win->GetID());
    win->ScanClientArea(client);

    lbox = (C_ListBox*)win->FindControl(WEAPON_LIST_CTRL);

    if (lbox)
    {
        switch (lbox->GetTextID())
        {
            case SHOW_LOADOUT:
                GetType = StoresList::_ALL_;
                ShowLoadedOnly = TRUE;
                break;

            case SHOW_AA:
                GetType = StoresList::_AIR_TO_AIR_;
                break;

            case SHOW_AG:
                GetType = StoresList::_AIR_TO_GROUND_;
                break;

            case SHOW_OTHER:
                GetType = StoresList::_OTHER_;
                break;

            default:
                GetType = StoresList::_ALL_;
        }
    }
    else
        GetType = StoresList::_ALL_;

    y = 1;
    x = 174 + 240 - (HardPoints / 2) * 30 + 15 * ((HardPoints - 1) bitand 1);

    // store's column headings
    hpnum = 1;

    for (i = HardPoints - 1; i > 0; i--)
    {
        if (VisFlag bitand (1 << (HardPoints - i)))
            _stprintf(buf, "%1d", hpnum++);
        else
        {
            memset(buf, 0, sizeof buf);
            _tcscpy(buf, gStringMgr->GetString(TXT_INT));
            ShiAssert(buf[STRING_BUFFER_SIZE - 1] == 0);
            buf[STRING_BUFFER_SIZE - 1] = 0;
        }

        txt = new C_Text;
        txt->Setup(C_DONT_CARE, 0);
        txt->SetFixedWidth(_tcsclen(buf) + 1);
        txt->SetText(buf);
        txt->SetFont(win->Font_);
        txt->SetXY(x + (i - 1) * 30 - 3 + 15 + 6, 278);
        txt->SetFGColor(0xad8041);
        txt->SetFlagBitOn(C_BIT_ABSOLUTE bitor C_BIT_HCENTER);
        txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
        win->AddControl(txt);
    }

    // Vertical lines separation the stores
    for (i = 0; i < HardPoints; i++)
    {
        line = new C_Line;
        line->Setup(C_DONT_CARE, C_TYPE_VERTICAL);
        line->SetXY(x + i * 30 + 6 - 3, 294);
        line->SetWH(1, 103);
        line->SetColor(0xad8041);
        line->SetFlagBitOn(C_BIT_ABSOLUTE);
        line->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
        win->AddControl(line);
    }

    cur = NULL;
    ShiAssert(gStores);

    if (gStores)
        cur = gStores->GetFirst(GetType);

    while (cur)
    {
        if (ShowLoadedOnly)
        {
            Drawit = FALSE;

            // JB 020219 Limit munition planecount to less than five otherwise we overwrite memory.
            for (i = 0; i < PlaneCount and Drawit == FALSE and i < 4; i++)
                for (j = 1; j < HardPoints and Drawit == FALSE; j++)
                    if (cur->ID == gCurStores[i].WeaponID[j] and gCurStores[i].WeaponCount[j])
                        Drawit = TRUE;
        }
        else
            Drawit = TRUE;

        if (Drawit)
        {
            // Store's name
            txt = new C_Text;
            txt->Setup(C_DONT_CARE, 0);
            txt->SetText(cur->Name);
            txt->SetFont(win->Font_);
            txt->SetXY(6, y + 4);
            txt->SetClient(static_cast<short>(client));
            txt->SetFGColor(0xc0c0c0);
            txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            win->AddControl(txt);

            // # in stock
            avail = TotalAvailable(static_cast<short>(cur->ID));

            if ( not avail)
            {
                availID = TXT_SUPPLY_OUT;
                color = 0x0000ff;
            }
            else if (avail < 700)
            {
                availID = TXT_SUPPLY_LOW;
                color = 0x00ffff;
            }
            else if (avail < 1500)
            {
                availID = TXT_SUPPLY_MEDIUM;
                color = 0xeeeeee;
            }
            else
            {
                availID = TXT_SUPPLY_HIGH;
                color = 0x00ff00;
            }

            txt = new C_Text;
            txt->Setup((1 << 25) bitor cur->ID, 0);
            txt->SetText(availID);
            txt->SetFont(win->Font_);
            txt->SetXY(122, y + 4);
            txt->SetClient(static_cast<short>(client));
            txt->SetFGColor(color);
            txt->SetFlagBitOn(C_BIT_HCENTER);
            txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            win->AddControl(txt);

            // # on board
            txt = new C_Text;
            txt->Setup((1 << 24) bitor cur->ID, 0);
            txt->SetFixedWidth(5);
            txt->SetText(" ");
            txt->SetFont(win->Font_);
            txt->SetXY(157, y + 4);
            txt->SetClient(static_cast<short>(client));
            txt->SetFGColor(0xc0c0c0);
            txt->SetFlagBitOn(C_BIT_HCENTER);
            txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            win->AddControl(txt);

            // configuration
            for (i = 1; i < HardPoints; i++)
            {
                if (cur->HardPoint[i])
                {
                    btn = new C_Button;
                    btn->Setup(i << 16 bitor cur->ID, C_TYPE_CUSTOM, x + ((HardPoints - i) - 1) * 30 + 1, y + 4);
                    btn->SetUserNumber(1, cur->HardPoint[i]);
                    btn->SetUserNumber(2, 1);

                    if ( not (VisFlag bitand (1 << (i)))) // Internal stores
                    {
                        btn->SetBackImage(INT_EMPTY);
                        btn->SetImage(C_STATE_0, INT_EMPTY);
                        btn->SetImage(C_STATE_1, INT_DIFF); // should be diff
                        btn->SetImage(C_STATE_2, INT_FULL);
                        btn->SetImage(C_STATE_3, INT_DIFF);
                        btn->SetImage(C_STATE_DISABLED, INT_DIS);
                    }
                    else if (cur->Type == StoresList::_TYPE_GUN_)
                    {
                        btn->SetBackImage(SINGLE_EMPTY);
                        btn->SetImage(C_STATE_0, SINGLE_EMPTY);
                        btn->SetImage(C_STATE_1, SINGLE_EMPTY_DIFF); // should be diff
                        btn->SetImage(C_STATE_2, SINGLE_FULL);
                        btn->SetImage(C_STATE_3, SINGLE_DIFF);
                        btn->SetImage(C_STATE_DISABLED, SINGLE_DIS);
                        btn->SetUserNumber(1, 1);
                        btn->SetUserNumber(2, cur->HardPoint[i]);
                    }
                    else switch (cur->HardPoint[i])
                        {
                            case 1:
                                switch (cur->Type)
                                {
                                    case StoresList::_TYPE_FUEL_:
                                        btn->SetBackImage(POD_EMPTY);
                                        btn->SetImage(C_STATE_0, POD_EMPTY);
                                        btn->SetImage(C_STATE_1, POD_EMPTY_DIFF); // should be diff
                                        btn->SetImage(C_STATE_2, POD_FULL);
                                        btn->SetImage(C_STATE_3, POD_DIFF);
                                        btn->SetImage(C_STATE_DISABLED, POD_DIS);
                                        break;

                                    default:
                                        btn->SetBackImage(SINGLE_EMPTY);
                                        btn->SetImage(C_STATE_0, SINGLE_EMPTY);
                                        btn->SetImage(C_STATE_1, SINGLE_EMPTY_DIFF); // should be diff
                                        btn->SetImage(C_STATE_2, SINGLE_FULL);
                                        btn->SetImage(C_STATE_3, SINGLE_DIFF);
                                        btn->SetImage(C_STATE_DISABLED, SINGLE_DIS);
                                        break;
                                }

                                break;

                            case 2:
                                switch (cur->Type)
                                {
                                    case StoresList::_TYPE_MISSILE_:
                                        btn->SetImage(C_STATE_2, LAU1_FULL);
                                        btn->SetImage(C_STATE_3, LAU1_FULL_DIFF);

                                        if (i > (HardPoints / 2))
                                        {
                                            btn->SetBackImage(LAU2L_EMPTY);
                                            btn->SetImage(C_STATE_0, LAU2L_EMPTY);
                                            btn->SetImage(C_STATE_1, LAU2L_EMPTY_DIFF); // should be diff
                                            btn->SetImage(C_STATE_4, LAU2L_FULL);
                                            btn->SetImage(C_STATE_5, LAU2L_DIFF);
                                            btn->SetImage(C_STATE_DISABLED, LAU2L_DIS);
                                        }
                                        else if (i == (HardPoints / 2) and not (HardPoints bitand 1))
                                        {
                                            btn->SetBackImage(LAU2C_EMPTY);
                                            btn->SetImage(C_STATE_0, LAU2C_EMPTY);
                                            btn->SetImage(C_STATE_1, LAU2C_EMPTY_DIFF); // should be diff
                                            btn->SetImage(C_STATE_4, LAU2C_FULL);
                                            btn->SetImage(C_STATE_5, LAU2C_DIFF);
                                            btn->SetImage(C_STATE_DISABLED, LAU2C_DIS);
                                        }
                                        else
                                        {
                                            btn->SetBackImage(LAU2R_EMPTY);
                                            btn->SetImage(C_STATE_0, LAU2R_EMPTY);
                                            btn->SetImage(C_STATE_1, LAU2R_EMPTY_DIFF); // should be diff
                                            btn->SetImage(C_STATE_4, LAU2R_FULL);
                                            btn->SetImage(C_STATE_5, LAU2R_DIFF);
                                            btn->SetImage(C_STATE_DISABLED, LAU2R_DIS);
                                        }

                                        break;

                                    default:
                                        btn->SetImage(C_STATE_2, LAU1_FULL);
                                        btn->SetImage(C_STATE_3, LAU1_FULL_DIFF);

                                        if (i > (HardPoints / 2))
                                        {
                                            btn->SetBackImage(TER2L_EMPTY);
                                            btn->SetImage(C_STATE_0, TER2L_EMPTY);
                                            btn->SetImage(C_STATE_1, TER2L_EMPTY_DIFF); // should be diff
                                            btn->SetImage(C_STATE_4, TER2L_FULL);
                                            btn->SetImage(C_STATE_5, TER2L_DIFF);
                                            btn->SetImage(C_STATE_DISABLED, TER2L_DIS);
                                        }
                                        else
                                        {
                                            btn->SetBackImage(TER2R_EMPTY);
                                            btn->SetImage(C_STATE_0, TER2R_EMPTY);
                                            btn->SetImage(C_STATE_1, TER2R_EMPTY_DIFF); // should be diff
                                            btn->SetImage(C_STATE_4, TER2R_FULL);
                                            btn->SetImage(C_STATE_5, TER2R_DIFF);
                                            btn->SetImage(C_STATE_DISABLED, TER2R_DIS);
                                        }

                                        break;
                                }

                                break;

                            case 3:
                                switch (cur->Type)
                                {
                                    case StoresList::_TYPE_MISSILE_:
                                        btn->SetBackImage(LAU3_EMPTY);
                                        btn->SetImage(C_STATE_0, LAU3_EMPTY);
                                        btn->SetImage(C_STATE_1, LAU3_EMPTY_DIFF); // should be diff
                                        btn->SetImage(C_STATE_2, LAU1_FULL);
                                        btn->SetImage(C_STATE_3, LAU1_FULL_DIFF);

                                        if (i > (HardPoints / 2))
                                        {
                                            btn->SetImage(C_STATE_4, LAU2L_FULL);
                                            btn->SetImage(C_STATE_5, LAU2L_DIFF);
                                        }
                                        else if (i == (HardPoints / 2) and not (HardPoints bitand 1))
                                        {
                                            btn->SetImage(C_STATE_4, LAU2C_FULL);
                                            btn->SetImage(C_STATE_5, LAU2C_DIFF);
                                        }
                                        else
                                        {
                                            btn->SetImage(C_STATE_4, LAU2R_FULL);
                                            btn->SetImage(C_STATE_5, LAU2R_DIFF);
                                        }

                                        btn->SetImage(C_STATE_6, LAU3_FULL);
                                        btn->SetImage(C_STATE_7, LAU3_DIFF);
                                        btn->SetImage(C_STATE_DISABLED, LAU3_DIS);
                                        break;

                                    default:
                                        btn->SetBackImage(TER_EMPTY);
                                        btn->SetImage(C_STATE_0, TER_EMPTY);
                                        btn->SetImage(C_STATE_1, TER_EMPTY_DIFF); // should be diff
                                        btn->SetImage(C_STATE_2, LAU1_FULL);
                                        btn->SetImage(C_STATE_3, LAU1_FULL_DIFF);

                                        if (i > (HardPoints / 2))
                                        {
                                            btn->SetImage(C_STATE_4, TER2L_FULL);
                                            btn->SetImage(C_STATE_5, TER2L_DIFF);
                                        }
                                        else
                                        {
                                            btn->SetImage(C_STATE_4, TER2R_FULL);
                                            btn->SetImage(C_STATE_5, TER2R_DIFF);
                                        }

                                        btn->SetImage(C_STATE_6, TER_FULL);
                                        btn->SetImage(C_STATE_7, TER_DIFF);
                                        btn->SetImage(C_STATE_DISABLED, TER_DIS);
                                        break;
                                }

                                break;

                            case 4:
                                btn->SetBackImage(QUAD_EMPTY);
                                btn->SetImage(C_STATE_0, QUAD_EMPTY);
                                btn->SetImage(C_STATE_1, QUAD_EMPTY_DIFF); // should be diff

                                if (i > (HardPoints / 2))
                                {
                                    btn->SetImage(C_STATE_2, QUAD1L);
                                    btn->SetImage(C_STATE_3, QUAD1L_DIFF);
                                    btn->SetImage(C_STATE_4, QUAD2L);
                                    btn->SetImage(C_STATE_5, QUAD2L_DIFF);
                                    btn->SetImage(C_STATE_6, QUAD3L);
                                    btn->SetImage(C_STATE_7, QUAD3L_DIFF);
                                }
                                else
                                {
                                    btn->SetImage(C_STATE_2, QUAD1R);
                                    btn->SetImage(C_STATE_3, QUAD1R_DIFF);
                                    btn->SetImage(C_STATE_4, QUAD2R);
                                    btn->SetImage(C_STATE_5, QUAD2R_DIFF);
                                    btn->SetImage(C_STATE_6, QUAD3R);
                                    btn->SetImage(C_STATE_7, QUAD3R_DIFF);
                                }

                                btn->SetImage(C_STATE_8, QUAD_FULL);
                                btn->SetImage(C_STATE_9, QUAD_DIFF);
                                btn->SetImage(C_STATE_DISABLED, QUAD_DIS);
                                break;

                            case 5:
                            case 6:
                            default: // JPO - best of a bad lot, for > 6, we at least get some feedback
                                btn->SetBackImage(DOUBLE_TER_EMPTY);
                                btn->SetImage(C_STATE_0, DOUBLE_TER_EMPTY);
                                btn->SetImage(C_STATE_1, DOUBLE_TER_DIFF); // should be diff
                                btn->SetImage(C_STATE_2, LAU1_FULL);
                                btn->SetImage(C_STATE_3, LAU1_FULL_DIFF);

                                if (i > (HardPoints / 2))
                                {
                                    btn->SetImage(C_STATE_4, TER2L_FULL);
                                    btn->SetImage(C_STATE_5, TER2L_DIFF);
                                    btn->SetImage(C_STATE_10, DOUBLE_TER_5L);
                                    btn->SetImage(C_STATE_11, DOUBLE_TER_5L_DIFF);
                                }
                                else
                                {
                                    btn->SetImage(C_STATE_4, TER2R_FULL);
                                    btn->SetImage(C_STATE_5, TER2R_DIFF);
                                    btn->SetImage(C_STATE_10, DOUBLE_TER_5R);
                                    btn->SetImage(C_STATE_11, DOUBLE_TER_5R_DIFF);
                                }

                                btn->SetImage(C_STATE_6, TER_FULL);
                                btn->SetImage(C_STATE_7, TER_DIFF);
                                btn->SetImage(C_STATE_8, DOUBLE_TER_4);
                                btn->SetImage(C_STATE_9, DOUBLE_TER_4_DIFF);
                                btn->SetImage(C_STATE_12, DOUBLE_TER_FULL);
                                btn->SetImage(C_STATE_13, DOUBLE_TER_DIFF);
                                btn->SetImage(C_STATE_14, DOUBLE_TER_FULL);
                                btn->SetImage(C_STATE_15, DOUBLE_TER_DIFF);
                                btn->SetImage(C_STATE_16, DOUBLE_TER_FULL);
                                btn->SetImage(C_STATE_17, DOUBLE_TER_DIFF);
                                btn->SetImage(C_STATE_18, DOUBLE_TER_FULL);
                                btn->SetImage(C_STATE_19, DOUBLE_TER_DIFF);
                                btn->SetImage(C_STATE_20, DOUBLE_TER_FULL);
                                btn->SetImage(C_STATE_DISABLED, DOUBLE_TER_DIS);
                                break;

                                // JPO - from default, to 0. Move default up to 5/6 case
                                // this will only handle the no weapon case now I think,
                                // which may not even exist.
                            case 0: //marked
                                btn->SetBackImage(POD_EMPTY);
                                btn->SetImage(C_STATE_0, POD_EMPTY);
                                btn->SetImage(C_STATE_1, POD_EMPTY_DIFF); // should be diff
                                btn->SetImage(C_STATE_2, POD_FULL);
                                btn->SetImage(C_STATE_3, POD_DIFF);
                                btn->SetImage(C_STATE_DISABLED, POD_DIS);
                                break;
                        }

                    btn->SetClient(static_cast<short>(client));
                    btn->SetFont(win->Font_);
                    btn->SetGroup(i);

                    if ( not (VisFlag bitand (1 << (i)))) // Internal stores
                        btn->SetCallback(InternalArmPlaneCB);
                    else
                        btn->SetCallback(ArmPlaneCB);

                    btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                    win->AddControl(btn);
                }
            }

            line = new C_Line;
            line->Setup(C_DONT_CARE, C_TYPE_HORIZONTAL);
            line->SetXY(0, y + 25);
            line->SetWH(win->ClientArea_[1].right - win->ClientArea_[1].left + 1, 1);
            line->SetColor(0xad8041);
            line->SetClient(static_cast<short>(client));
            line->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            win->AddControl(line);

            y += 26;
        }

        cur = gStores->GetNext();
    }

    SetCurrentLoadout();
    win->ScanClientAreas();
    win->RefreshWindow();
    UI_Leave(Leave);
}

void RestoreStores(C_Window *win)
{
    Flight flt;
    int v, ac, loads;
    short i, j;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(win);
    flt = (Flight)vuDatabase->Find(gLoadoutFlightID);

    ac = flt->GetTotalVehicles();
    loads = flt->GetNumberOfLoadouts();
    ShiAssert(ac > 0 and loads > 0);

    for (v = 0; v < ac; v++)
    {
        if (v < loads)
            memcpy(&gCurStores[v], flt->GetLoadout(v), sizeof(LoadoutStruct));
        else
            memcpy(&gCurStores[v], flt->GetLoadout(0), sizeof(LoadoutStruct));
    }

    memcpy(&gCurStores[4], flt->GetLoadout(0), sizeof(LoadoutStruct));

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < HARDPOINT_MAX; j++)
        {
            if (j and j < HardPoints)
            {
                int ok;
                ClearHardPoint(i, j, HardPoints / 2, &gCurRails[i].rail[j]);
                //if (g_bNewRackData) {
                Falcon4EntityClassType* classPtr = &Falcon4ClassTable[gVehID];
                ok = GetJRackAndWeapon(gVCPtr, classPtr, gCurStores[i].WeaponID[j], gCurStores[i].WeaponCount[j], static_cast<short>(j), &gCurRails[i].rail[j]);

                //}
                //else
                // ok = GetRackAndWeapon(gVCPtr,static_cast<short>(gVehID), gCurStores[i].WeaponID[j],gCurStores[i].WeaponCount[j],static_cast<short>(j),static_cast<short>(HardPoints/2),&gCurRails[i].rail[j]);
                if (ok)
                    LoadHardPoint(i, j, HardPoints / 2, &gCurRails[i].rail[j]);
            }
        }
    }

    MakeStoresList(win, 1);
    win->RefreshWindow();
    UI_Leave(Leave);
}

void ClearStores(C_Window *win)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        if (PlaneEditList[i])
        {
            for (j = 1; j < HARDPOINT_MAX; j++)
            {
                gCurStores[i].WeaponID[j] = 0;
                gCurStores[i].WeaponCount[j] = 0;

                if (j and j < HardPoints)
                {
                    ClearHardPoint(i, j, HardPoints / 2, &gCurRails[i].rail[j]);
                }
            }

            //memset(gCurRails[i].rail,0,sizeof(RailList));
        }
    }

    MakeStoresList(win, 1);
    win->RefreshWindow();
}

void UseStores()
{
    Flight flt;
    int i, ac, hp, loads;
    Squadron sq;
    LoadoutStruct *loadout, *newloadout;

    flt = (Flight)vuDatabase->Find(gLoadoutFlightID);

    if (flt == NULL)
        return;

    sq = (Squadron) flt->GetUnitSquadron();

    if ( not sq)
        return;

    ac = flt->GetTotalVehicles();
    loadout = flt->GetLoadout();
    loads = flt->GetLoadouts();

    if (loads < 1)
        return;

    // KCK: Need to rationalize the loadout structure
    for (i = 0; i < ac; i++)
    {
        for (hp = 0; hp < HARDPOINT_MAX; hp++)
        {
            if ( not gCurStores[i].WeaponCount[hp])
                gCurStores[i].WeaponID[hp] = 0;
        }
    }

    CampEnterCriticalSection();

    // Notify the squadron that we're returning some weapons
    if (loads < ac)
        sq->UpdateSquadronStores(loadout[0].WeaponID, loadout[0].WeaponCount, 0, -ac);
    else
    {
        for (i = 0; i < loads; i++)
            sq->UpdateSquadronStores(loadout[i].WeaponID, loadout[i].WeaponCount, 0, -1);
    }

    // Notify the squadron that we've used some weapons
    for (i = 0; i < ac; i++)
        sq->UpdateSquadronStores(gCurStores[i].WeaponID, gCurStores[i].WeaponCount, 0, 1);

    // KCK: Gilman wanted weapons used by the player to have a larger effect.. This isn't exactly very easy to
    // do, because if they then change this loadout later, they won't be put back...
    // i = FalconLocalSession->GetAircraftNum();
    // if (i < PILOTS_PER_FLIGHT)
    // UpdateSquadronStores (sq, gCurStores[i].WeaponID, gCurStores[i].WeaponCount, 0, 4);

    // KCK: Flights optimize to use a single LoadoutStruct for all aircraft,
    // After the player mucks with it, we're going to use one PER aircraft.
    newloadout = new LoadoutStruct[ac];

    for (i = 0; i < ac; i++)
    {
        // Hardpoint 0 never changes (KCK: this seems like a weird way to do this,
        // but Peter doesn't save off the gun loadout because he wants to be able to
        // use the loadout for multiple aircraft -although, I have my doubts that
        // that's possible).
        newloadout[i].WeaponID[0] = loadout[0].WeaponID[0];
        newloadout[i].WeaponCount[0] = loadout[0].WeaponCount[0];

        for (hp = 1; hp < HardPoints; hp++)
        {
            newloadout[i].WeaponID[hp] = gCurStores[i].WeaponID[hp];
            newloadout[i].WeaponCount[hp] = gCurStores[i].WeaponCount[hp];
        }
    }

    flt->SetLoadout(newloadout, ac);

    CampLeaveCriticalSection();

    // Update the mission evaluator
    if (gLoadoutFlightID == FalconLocalSession->GetPlayerFlightID())
        TheCampaign.MissionEvaluator->PreMissionEval(flt, FalconLocalSession->GetPilotSlot());
}
