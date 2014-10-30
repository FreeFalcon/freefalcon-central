#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "Find.h"
#include "objectiv.h"
#include "Campaign.h"
#include "CampList.h"
#include "feature.h"
#include "vehicle.h"
#include "name.h"
#include "initdata.h"
#include "Camp2Sim.h"
#include "simbase.h"
#include "f4find.h"
#include "Team.h"
#include "Weather.h"
#include "AIInput.h"
#include "CUIEvent.h"
#include "MsgInc/ObjectiveMsg.h"
#include "MsgInc/CampEventMsg.h"
#include "MsgInc/AirTaskingMsg.h"
#include "MsgInc/CampDataMsg.h"
#include "MsgInc/CampWeaponFireMsg.h"
#include "MsgInc/SimCampMsg.h"
#include "CampStr.h"
#include "ATM.h"
#include "PtData.h"
#include "CmpClass.h"
#include "Gtmobj.h"
#include "PlayerOp.h"
#include "Utils/Lzss.h"
#include "FalcSess.h"
#include "SimDrive.h"
#include "OTWDrive.h"
#include "simFiltr.h"
#include "classtbl.h"
#include "uiwin.h"
#include "Tacan.h"
#include "Persist.h"
#include "CmpRadar.h"
#include "Atcbrain.h"
#include "Flight.h"
#include "F4Version.h"
#include "flight.h"
#include "simfeat.h"

//sfr: added for checks
#include "InvalidBufferException.h"

/* 2001-04-06 S.G. 'CanDetect' */
#include "Graphics/Include/TMap.h"

using namespace std;

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL ObjectiveClass::pool;
extern MEM_POOL gObjMemPool;
// MEM_POOL gObjHeapMemPool;
#endif

//=========================
// Externals
//=========================

extern int ReadVersionNumber(char *saveFile);
extern void RedrawCell(MapData md, GridIndex x, GridIndex y);
extern void EvaluateKill(FalconDeathMessage *dtm, SimBaseClass *simShooter, CampBaseClass *campShooter, SimBaseClass *simTarget, CampBaseClass *campTarget);
extern int RepairObjective;
extern int DestroyObjective;
extern int ClearObjManualFlags;
extern int AwakeCampaignEntities;

// =========================
// Module variabls and stuff
// =========================

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

#ifdef DEBUG
#define KEEP_STATISTICS
extern int AS_Kills;
extern int AS_Shots;
int ObjsDeagg = 0;
int FeatsDeagg = 0;
int gObjectiveCount = 0;
class ObjListCounter
{
    enum { MAXOBJS = 3000, };
    int objlist[MAXOBJS];
    int lastptr;
public:
    ObjListCounter()
    {
        memset(objlist, 0, sizeof(objlist));
        lastptr = 0;
    };
    void Report()
    {
        MonoPrint("%d objectives left\n", gObjectiveCount);

        for (int i = 0; i < lastptr; i++)
            if (objlist[i] > 0)
                MonoPrint("Id %d still left\n", objlist[i]);
    }
    void AddObj(int id)
    {
        if (lastptr < MAXOBJS) objlist[lastptr++] = id;
    };
    void DelObj(int id)
    {
        for (int i = 0; i < lastptr; i++)
            if (objlist[i] == id)
                objlist[i] = 0;
    }
};
static ObjListCounter myolist;
void ObjectiveReport(void)
{
    myolist.Report();
}

#endif

// -------------------------
// Local Function Prototypes
// =========================

void UpdateObjectiveLists(Objective o);

// ---------------------------------
// Global Function (ADT) Definitions
// =================================

ObjectiveClass::ObjectiveClass(int typeindex) : CampBaseClass(typeindex, GetIdFromNamespace(ObjectiveNS))
{
    int size;

    dirty_objective = 0;
    static_data.first_owner = 0;
    static_data.nameid = 0;
    static_data.class_data = (ObjClassDataType*)Falcon4ClassTable[typeindex - VU_LAST_ENTITY_TYPE].dataPtr;
    static_data.links = 0;
    static_data.radar_data = 0;
    obj_data.priority = 10;
    obj_data.status = 100;
    obj_data.obj_flags = 0;
    obj_data.fuel = 0;
    obj_data.supply = 0;
    obj_data.losses = 0;
    obj_data.last_repair = 0;
    size = ((static_data.class_data->Features * 2) + 7) / 8;
#ifdef USE_SH_POOLS
    obj_data.fstatus = (uchar *)MemAllocPtr(gObjMemPool, sizeof(uchar) * size, 0);
#else
    obj_data.fstatus = new uchar[size];
#endif
    memset(obj_data.fstatus, 0, size);
    // for (i=0; i<size; i++)
    // obj_data.fstatus[i] = 0xFF;

    link_data = NULL;

    if (GetType() == TYPE_AIRBASE or
        GetType() == TYPE_AIRSTRIP)
    {

        if (GetType() == TYPE_AIRBASE)
        {
            SetTacan(1);
        }

        brain = new ATCBrain(this);
    }
    else
    {
        brain = NULL;
    }

#ifdef DEBUG_COUNT
    myolist.AddObj(Id().num_);
    gObjectiveCount++;
#endif
}

ObjectiveClass::ObjectiveClass(VU_BYTE **stream, long *rem) : CampBaseClass(stream, rem)
{
    uchar size, nsize, i;

    dirty_objective = 0;
    static_data.class_data = (ObjClassDataType*)Falcon4ClassTable[share_.entityType_ - VU_LAST_ENTITY_TYPE].dataPtr;

    //#ifdef CAMPTOOL
    // if (gRenameIds) {
    // VU_ID new_id = FalconNullId;
    //
    // // Rename this ID
    // for (new_id.num_ = FIRST_OBJECTIVE_VU_ID_NUMBER; new_id.num_ < LAST_OBJECTIVE_VU_ID_NUMBER; new_id.num_++) {
    // if ( not vuDatabase->Find(new_id)) {
    // RenameTable[share_.id_.num_] = new_id.num_;
    // share_.id_ = new_id;
    // break;
    // }
    // }
    // }
    //#endif

    ShiAssert(static_data.class_data);

    if ( not static_data.class_data)
    {
        obj_data.fstatus = new uchar[0];
        SetObjectiveType(TYPE_TOWN);
    }

    memcpychk(&obj_data.last_repair, stream, sizeof(CampaignTime), rem);

    if (gCampDataVersion > 1)
    {
        memcpychk(&obj_data.obj_flags, stream, sizeof(ulong), rem);
    }
    else
    {
        short temp;
        memcpychk(&temp, stream, sizeof(short), rem);
        obj_data.obj_flags = temp;
    }

    memcpychk(&obj_data.supply, stream, sizeof(uchar), rem);
    memcpychk(&obj_data.fuel, stream, sizeof(uchar), rem);
    memcpychk(&obj_data.losses, stream, sizeof(uchar), rem);
    memcpychk(&size, stream, sizeof(uchar), rem);
    nsize = (uchar)(((static_data.class_data->Features * 2) + 7) / 8);
#ifdef USE_SH_POOLS
    obj_data.fstatus = (uchar *)MemAllocPtr(gObjMemPool, sizeof(uchar) * nsize, 0);
#else
    obj_data.fstatus = new uchar[nsize];
#endif

    if (nsize == size)
    {
        // Newer class same size
        memcpychk(obj_data.fstatus, stream, size, rem);
    }
    else if (nsize > size)
    {
        // Newer class data is larger - copy what we have, other is zeroed
        memset(obj_data.fstatus, 0, nsize);
        memcpychk(obj_data.fstatus, stream, size, rem);
    }
    else
    {
        // Newer class data is smaller - copy enough to fill, increment pointer correctly
        //sfr: yeah, helluva hack... correcting, dont CHANGE
        memcpychk(obj_data.fstatus, stream, nsize, rem);
        //we skip next bytes because of the way they made this, which is stupid(check orignal source)
        *stream += (size - nsize);
        rem[0] -= (size - nsize);
    }

    memcpychk(&obj_data.priority, stream, sizeof(uchar), rem);
    obj_data.status = 100; // JPO init

    memcpychk(&static_data.nameid, stream, sizeof(short), rem);
    memcpychk(&static_data.parent, stream, sizeof(VU_ID), rem);
#ifdef DEBUG
    static_data.parent.num_ and_eq 0x0000ffff;
#endif
    memcpychk(&static_data.first_owner, stream, sizeof(Control), rem);
    memcpychk(&static_data.links, stream, sizeof(uchar), rem);

    if (static_data.links)
    {
#ifdef USE_SH_POOLS
        link_data = (CampObjectiveLinkDataType *)MemAllocPtr(gObjMemPool, sizeof(CampObjectiveLinkDataType) * static_data.links, 0);
#else
        link_data = new CampObjectiveLinkDataType[static_data.links];
#endif
    }
    else
    {
        link_data = NULL;
    }

    for (i = 0; i < static_data.links; i++)
    {
        memcpychk(&link_data[i], stream, sizeof(CampObjectiveLinkDataType), rem);
#ifdef DEBUG
        link_data[i].id.num_ and_eq 0x0000ffff;
#endif
    }

    // Read Radar data
    if (gCampDataVersion >= 20)
    {
        memcpychk(&i, stream, sizeof(uchar), rem);

        if (i)
        {
#ifdef USE_SH_POOLS
            static_data.radar_data = (RadarRangeClass *)MemAllocPtr(gObjMemPool, sizeof(RadarRangeClass), 0);
#else
            static_data.radar_data = new RadarRangeClass;
#endif
            memcpychk(static_data.radar_data, stream, sizeof(RadarRangeClass), rem);
        }
        else
        {
            static_data.radar_data = 0;
        }
    }
    else
    {
        static_data.radar_data = 0;
    }

    obj_data.aiscore = 0;
    ResetObjectiveStatus();

    if ((GetType() == TYPE_AIRBASE) or (GetType() == TYPE_AIRSTRIP))
    {
        if (GetType() == TYPE_AIRBASE)
        {
            SetTacan(1);
        }

        brain = new ATCBrain(this);
    }
    else
    {
        brain = NULL;
    }

    // Set the owner to the game master.
    if ( not FalconLocalGame->IsLocal())
    {
        share_.ownerId_ = FalconLocalGame->OwnerId();
    }

    if ( not static_data.class_data->Features)
    {
        // Since there's nothing to do in this case, might as well mark us as awake and save the Sim time...
        SetAggregate(false);
        SetAwake(1);
    }

#ifdef DEBUG

    // Clear out objectives owned by non-existant teams
    // KCK NOTE: This doesn't work in multi-player remote, as we often don't have teams at this point
    if (FalconLocalGame and FalconLocalGame->IsLocal())
    {
        if ( not TeamInfo[GetObjectiveOldown()])
            SetObjectiveOldown(GetOwner());
    }

#endif

#ifdef DEBUG_COUNT
    gObjectiveCount++;
    myolist.AddObj(Id().num_);
#endif
}

ObjectiveClass::~ObjectiveClass(void)
{
    delete [] obj_data.fstatus;
    delete [] link_data;

    if (static_data.radar_data)
        delete static_data.radar_data;

    if (brain)
    {
        delete brain;
        brain = NULL;
    }

#ifdef DEBUG_COUNT
    myolist.DelObj(Id().num_);
    gObjectiveCount--;
#endif
}

int ObjectiveClass::SaveSize(void)
{
    int size = CampBaseClass::SaveSize()
               + sizeof(CampaignTime)
               + sizeof(ulong)
               + sizeof(uchar)
               + sizeof(uchar)
               + sizeof(uchar)
               + sizeof(uchar)
               + ((static_data.class_data->Features * 2) + 7) / 8
               + sizeof(uchar)
               + sizeof(short)
               + sizeof(VU_ID)
               + sizeof(Control)
               + sizeof(uchar)
               + sizeof(uchar)
               + static_data.links * sizeof(CampObjectiveLinkDataType);

    if (static_data.radar_data)
        size += sizeof(RadarRangeClass);

    return size;
}

//int ObjectiveClass::SaveSize(int update)
int ObjectiveClass::SaveSize(int)
{
    return sizeof(CampaignTime)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + ((static_data.class_data->Features * 2) + 7) / 8;
}

int ObjectiveClass::Save(VU_BYTE **stream)
{
    int len, i;
    uchar num = 0;

    CampBaseClass::Save(stream);

    if ( not IsAggregate())
    {
        // KCK TODO: We need to send the deaggregated data as well
    }

    memcpy(*stream, &obj_data.last_repair, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &obj_data.obj_flags, sizeof(ulong));
    *stream += sizeof(ulong);
    memcpy(*stream, &obj_data.supply, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &obj_data.fuel, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &obj_data.losses, sizeof(uchar));
    *stream += sizeof(uchar);
    len = ((static_data.class_data->Features * 2) + 7) / 8;
    memcpy(*stream, &len, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, obj_data.fstatus, len);
    *stream += len;
    memcpy(*stream, &obj_data.priority, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &static_data.nameid, sizeof(short));
    *stream += sizeof(short);
#ifdef CAMPTOOL

    if (gRenameIds)
        static_data.parent.num_ = RenameTable[static_data.parent.num_];

#endif
    memcpy(*stream, &static_data.parent, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &static_data.first_owner, sizeof(Control));
    *stream += sizeof(Control);
    memcpy(*stream, &static_data.links, sizeof(uchar));
    *stream += sizeof(uchar);

    for (i = 0; i < static_data.links; i++)
    {
#ifdef CAMPTOOL

        if (gRenameIds)
            link_data[i].id.num_ = RenameTable[link_data[i].id.num_];

#endif
        memcpy(*stream, &link_data[i], sizeof(CampObjectiveLinkDataType));
        *stream += sizeof(CampObjectiveLinkDataType);
    }

    if (static_data.radar_data)
    {
        // We have radar data
        num = 1;
        memcpy(*stream, &num, sizeof(uchar));
        *stream += sizeof(uchar);
        memcpy(*stream, static_data.radar_data, sizeof(RadarRangeClass));
        *stream += sizeof(RadarRangeClass);
    }
    else
    {
        // We don't have radar data
        num = 0;
        memcpy(*stream, &num, sizeof(uchar));
        *stream += sizeof(uchar);
    }

    return ObjectiveClass::SaveSize();
}

int ObjectiveClass::Save(VU_BYTE **stream, int update)
{
    uchar len = (uchar)(((static_data.class_data->Features * 2) + 7) / 8);
    uchar owner = GetOwner();

    memcpy(*stream, &obj_data.last_repair, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &owner, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &obj_data.supply, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &obj_data.fuel, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &obj_data.losses, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &len, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, obj_data.fstatus, len);
    *stream += len;

    return ObjectiveClass::SaveSize(update);
}

void ObjectiveClass::UpdateFromData(VU_BYTE **stream, long *rem)
{
    uchar len;
    uchar owner;

    memcpychk(&obj_data.last_repair, stream, sizeof(CampaignTime), rem);
    memcpychk(&owner, stream, sizeof(uchar), rem);
    SetOwner(owner);
    memcpychk(&obj_data.supply, stream, sizeof(uchar), rem);
    memcpychk(&obj_data.fuel, stream, sizeof(uchar), rem);
    memcpychk(&obj_data.losses, stream, sizeof(uchar), rem);
    memcpychk(&len, stream, sizeof(uchar), rem);
#ifdef USE_SH_POOLS
    obj_data.fstatus = (uchar *)MemAllocPtr(gObjMemPool, sizeof(uchar) * len, 0);
#else
    obj_data.fstatus = new uchar[len];
#endif

    if (gCampDataVersion < 64)
    {
        memcpychk(obj_data.fstatus, stream, 1, rem);
        memset(obj_data.fstatus + 1, 0, len - 1);
    }
    else
    {
        memcpychk(obj_data.fstatus, stream, len, rem);
    }

    ResetObjectiveStatus();
}

// event Handlers
VU_ERRCODE ObjectiveClass::Handle(VuFullUpdateEvent *event)
{
    // copy data from temp entity to current entity
    Objective tmp_ent = (Objective)(event->expandedData_.get());

    ShiAssert( not IsLocal());

    memcpy(&obj_data.last_repair, &tmp_ent->obj_data.last_repair, sizeof(CampaignTime));
    memcpy(&obj_data.obj_flags, &tmp_ent->obj_data.obj_flags, sizeof(short));
    memcpy(&obj_data.supply, &tmp_ent->obj_data.supply, sizeof(uchar));
    memcpy(&obj_data.fuel, &tmp_ent->obj_data.fuel, sizeof(uchar));
    memcpy(&obj_data.losses, &tmp_ent->obj_data.losses, sizeof(uchar));
    memcpy(&obj_data.priority, &tmp_ent->obj_data.priority, sizeof(uchar));

    return CampBaseClass::Handle(event);
}

//
// Required pure virtuals
//
void ObjectiveClass::SendDeaggregateData(VuTargetEntity *target)
{
    if (IsAggregate())
        return;

    uchar len, value;
    int totalsize, fid, f, classID;
    uchar *ddptr = NULL;
    FalconSimCampMessage *msg;
    SimBaseClass *feature;
    FeatureClassDataType* fc;

    if ( not GetComponents())
        return;

    len = (uchar)(((static_data.class_data->Features * 2) + 7) / 8);
    totalsize =  sizeof(uchar) + sizeof(uchar) + sizeof(VU_ID_NUMBER);
    totalsize *= static_data.class_data->Features;
    totalsize += sizeof(VU_SESSION_ID) + sizeof(VU_SESSION_ID) + sizeof(short) + sizeof(uchar) + len + sizeof(uchar);

    msg = new FalconSimCampMessage(Id(), target);
    msg->dataBlock.message = FalconSimCampMessage::simcampDeaggregateFromData;
    msg->dataBlock.from = GetDeagOwner();
    msg->dataBlock.size = (unsigned short)totalsize;
    msg->dataBlock.data = new uchar[totalsize];
    ddptr = msg->dataBlock.data;
    VU_ID tempId = GetDeagOwner();
    // pass some status stuff along, so we're on the same page.
    memcpy(ddptr, &(tempId.creator_), sizeof(VU_SESSION_ID));
    ddptr += sizeof(VU_SESSION_ID);
    tempId = FalconLocalSession->Id();
    memcpy(ddptr, &(tempId.creator_), sizeof(VU_SESSION_ID));
    ddptr += sizeof(VU_SESSION_ID);
    memcpy(ddptr, &obj_data.obj_flags, sizeof(short));
    ddptr += sizeof(short);
    memcpy(ddptr, &len, sizeof(uchar));
    ddptr += sizeof(uchar);
    memcpy(ddptr, obj_data.fstatus, len);
    ddptr += len;
    memcpy(ddptr, &static_data.class_data->Features, sizeof(uchar));
    ddptr += sizeof(uchar);

    //MonoPrint ("Features %08x:%08x = %d\n", Id(), static_data.class_data->Features);

    fid = static_data.class_data->FirstFeature;
    VuListIterator myit(GetComponents());
    feature = (SimBaseClass*) myit.GetFirst();

    for (f = 0; f < static_data.class_data->Features; f++, fid++)
    {
        if (feature)
        {
            classID = GetFeatureID(f);

            if (classID)
            {
                fc = GetFeatureClassData(classID);

                if ( not fc or fc->Flags bitand FEAT_VIRTUAL)
                {
                    // Gotta notify the remote machine that this is virtual.
                    value = 255;
                    memcpy(ddptr, &value, sizeof(uchar));
                    ddptr += sizeof(uchar);
                }
                else
                {
                    // Find the correct feature from our component list
                    ShiAssert(feature->GetSlot() == f);
                    value = (uchar)f;
                    memcpy(ddptr, &value, sizeof(uchar));
                    ddptr += sizeof(uchar);
                    value = (uchar)feature->Status();
                    memcpy(ddptr, &value, sizeof(uchar));
                    ddptr += sizeof(uchar);
                    VU_ID_NUMBER num = feature->Id().num_;
                    memcpy(ddptr, &num, sizeof(VU_ID_NUMBER));
                    ddptr += sizeof(VU_ID_NUMBER);
                    feature = (SimBaseClass*) myit.GetNext();
                }
            }
        }
        else
        {
            value = 255;
            memcpy(ddptr, &value, sizeof(uchar));
            ddptr += sizeof(uchar);
        }
    }

    // Send Deaggregation data to everyone in the group
    FalconSendMessage(msg, TRUE);
}

int ObjectiveClass::Deaggregate(FalconSessionEntity *session)
{
    if ( not IsLocal() or not IsAggregate())
    {
        return 0;
    }

    ShiAssert(FalconLocalGame->IsLocal());

    int f, fid, added = 0;
    VehicleID classID;
    float x, y, z;
    SimInitDataClass simdata;
    FeatureClassDataType* fc;
    ObjClassDataType* oc;
    SimBaseClass* newObject;
    // FalconCampDataMessage *msg = NULL;
    // uchar value,*ddptr = NULL;
    SimFeatureFilter filter;

    memset(&simdata, 0, sizeof(simdata));

    SetAggregate(0);

    // Set the owner of the newly created deaggregated entities
    SetDeagOwner(session->Id());

    SetComponents(new TailInsertList(&filter));
    GetComponents()->Register();

    // Set up the init data structure.
    oc = GetObjectiveClassData();

    ShiAssert(static_data.class_data == oc);

    memset(simdata.weapon, 0, sizeof(short)*HARDPOINT_MAX);
    memset(simdata.weapons, 0, sizeof(unsigned char)*HARDPOINT_MAX);
    simdata.vehicleInUnit = 255;
    simdata.playerSlot = 255;
    simdata.skill = 0;
    simdata.fuel = 0;
    simdata.waypointList = NULL;
    simdata.currentWaypoint = 0;
    simdata.numWaypoints = 0;
    simdata.createType = SimInitDataClass::CampaignFeature;
    simdata.z = 0;
    simdata.side = GetOwner();
    simdata.campBase = this;
    simdata.createFlags = SIDC_SILENT_INSERT;

    if (session not_eq FalconLocalSession)
    {
        simdata.createFlags or_eq SIDC_REMOTE_OWNER;
        simdata.owner = session;
    }

    fid = oc->FirstFeature;

    for (f = 0; f < oc->Features; f++, fid++)
    {
        classID = (short)GetFeatureID(f);

        if (classID)
        {
            fc = GetFeatureClassData(classID);

            if ( not fc or fc->Flags bitand FEAT_VIRTUAL)
            {
                // or fc->Priority > PlayerOptions.BuildingDetailLevel())
                continue;
            }

            simdata.campSlot = f;
            simdata.status = GetFeatureStatus(f);
            simdata.descriptionIndex = classID + VU_LAST_ENTITY_TYPE;
            simdata.flags = fc->Flags;
            simdata.specialFlags = FeatureEntryDataTable[fid].Flags;
            GetFeatureOffset(f, &y, &x, &z);
            simdata.x = XPos() + x;
            simdata.y = YPos() + y;
            simdata.heading = (float)(FeatureEntryDataTable[fid].Facing) * DEG_TO_RADIANS;
            simdata.displayPriority = fc->Priority;
            newObject = AddObjectToSim(&simdata, 0);

            if (newObject)
            {
                GetComponents()->ForcedInsert(newObject);
                added++;
            }
        }
    }

    if (TheCampaign.IsOnline())
    {
        SendDeaggregateData(FalconLocalGame);
    }

#if NEW_WAKE

    if (OwnerId() == vuLocalSessionEntity->OwnerId())
    {
        Wake();
    }
    else
    {
        SetAwake(0);
    }

#else

    if (session == FalconLocalSession or FalconLocalSession->InSessionBubble(this, 1.0F) > 0)
    {
        Wake();
    }
    else
    {
        SetAwake(0);
    }

#endif


#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->ForcedInsert(this);
#else
    deaggregatedMap->insert(CampBaseBin(this));
#endif

    return 1;
}

//int ObjectiveClass::RecordCurrentState (FalconSessionEntity *session, int byReag)
int ObjectiveClass::RecordCurrentState(FalconSessionEntity *session, int)
{
    // Record current state of components
    if (GetComponents())
    {
        VuEntity *feature, *next;
        int f;
        VuListIterator myit(GetComponents());

        for (feature = myit.GetFirst(); feature not_eq NULL; feature = next)
        {
            next = myit.GetNext();
            f = ((SimBaseClass*)feature)->GetSlot();

            if (GetFeatureID(f) == feature->Type() - VU_LAST_ENTITY_TYPE)
            {
                SetFeatureStatus(f, ((SimBaseClass*)feature)->Status() bitand VIS_TYPE_MASK);
            }

            if (session)
            {
                ((SimBaseClass*)feature)->ChangeOwner(session->Id());
            }
        }
    }

    ResetObjectiveStatus();
    return 0;
}

//int ObjectiveClass::Reaggregate (FalconSessionEntity* session)
int ObjectiveClass::Reaggregate(FalconSessionEntity*)
{
    if (IsAggregate() or not IsLocal())
        return 0;

    // Record current state of components
    RecordCurrentState(NULL, TRUE);

    if (IsAwake())
    {
        Sleep();
    }

    // No one is looking at this thing - Time to reaggregate
    if (GetComponents())
    {
        CampEnterCriticalSection();
        {
            // this iterator needs to be destroyed before the list, otherwise we CTD
            VuListIterator myit(GetComponents());
            VuEntity *feature = myit.GetFirst();

            while (feature)
            {
                VuEntity *next = myit.GetNext();
                // OW FIXME: this seals the feature memory leak but I dunno if this breaks Multiplay
                ((SimBaseClass*)feature)->SetRemoveSilentFlag();
                feature = next;
            }
        }
        GetComponents()->Unregister();
        delete GetComponents();
        SetComponents(NULL);
        CampLeaveCriticalSection();
    }

    if (TheCampaign.IsOnline())
    {
        // Send Reaggregation data to everyone in the group
        uchar len = (uchar)(((static_data.class_data->Features * 2) + 7) / 8);
        uchar *dataptr;
        FalconSimCampMessage *msg = new FalconSimCampMessage(Id(), FalconLocalGame);
        msg->dataBlock.message = FalconSimCampMessage::simcampReaggregateFromData;
        msg->dataBlock.from = GetDeagOwner();
        msg->dataBlock.size = (ushort)(len + sizeof(uchar));
        msg->dataBlock.data = new uchar[msg->dataBlock.size];
        dataptr = msg->dataBlock.data;
        memcpy(dataptr, &len, sizeof(uchar));
        dataptr += sizeof(uchar);
        memcpy(dataptr, obj_data.fstatus, len);
        dataptr += len;
        FalconSendMessage(msg, TRUE);
    }

    SetAggregate(1);

#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->Remove(this);
#else
    deaggregatedMap->remove(this->Id());
#endif

    return 1;
}

int ObjectiveClass::TransferOwnership(FalconSessionEntity* session)
{
    if (IsAggregate() or not IsLocal())
        return 0;

#ifdef DEAG_DEBUG
    MonoPrint("Transfering ownership of objective #%d - owner is: %d\n", GetCampID(), session->Id().creator_.value_);
#endif

    ShiAssert(FalconLocalGame->IsLocal());

    // Send the transfer owenership message
    if (TheCampaign.IsOnline())
    {
        uchar len = (uchar)(((static_data.class_data->Features * 2) + 7) / 8);
        FalconCampDataMessage *msg = new FalconCampDataMessage(Id(), FalconLocalGame);
        msg->dataBlock.type = FalconCampDataMessage::campDeaggregateStatusChangeData;
        msg->dataBlock.size = (ushort)(sizeof(VU_ID) + len + sizeof(uchar));
        msg->dataBlock.data = new uchar[msg->dataBlock.size];
        uchar *dataptr = msg->dataBlock.data;
        VU_ID vuid = session->Id();
        memcpy(dataptr, &vuid, sizeof(VU_ID));
        dataptr += sizeof(VU_ID);
        memcpy(dataptr, &len, sizeof(uchar));
        dataptr += sizeof(uchar);
        memcpy(dataptr, obj_data.fstatus, len);
        dataptr += len;
        FalconSendMessage(msg, TRUE);
    }

    // Change the owner of the deaggregated entities
    SetDeagOwner(session->Id());

    // Record current state of components and change ownership locally
    RecordCurrentState(session, FALSE);

    // Update our local wake status
    if (IsAwake() and not FalconLocalSession->InSessionBubble(this, REAGREGATION_RATIO))
    {
        Sleep();
    }
    else if ( not IsAwake() and (session == FalconLocalSession or FalconLocalSession->InSessionBubble(this, 1.0F) > 0))
    {
        Wake();
    }

    return 1;
}

int ObjectiveClass::Wake()
{
    // sfr: in MP we need to run entities even if we are not in game
#if not NEW_WAKE
    if ( not OTWDriver.IsActive())
    {
        return 0;
    }

#endif

    if (GetComponents())
    {
        SetAwake(1);
        SimDriver.WakeCampaignBase(FALSE, this, GetComponents());
    }
    else
    {
        return 0;
    }

    AwakeCampaignEntities++;
    // RemoveFromSimLists();

    return 1;
}

int ObjectiveClass::Sleep(void)
{
    // OTWDriver.LockObject ();
    // 2002-04-14 put back in by MN - we need to sleep our features, and this does it,
    //while a more general function name could have been chosen ;)
    SimDriver.SleepCampaignFlight(GetComponents()); //2002-02-11 REMOVED BY S.G. MPS original Cut and paste bug from UnitClass. Objectives have no flights

    SetAwake(0);
    AwakeCampaignEntities--;

    // InsertInSimLists(XPos(),YPos());
    // OTWDriver.UnLockObject ();

    return 1;
}

void ObjectiveClass::InsertInSimLists(float cameraX, float cameraY)
{
    // SetChecked(1);

    // This case is for the destructor's sleep call.
    // Basically, we're going away, so don't put us in any lists.
    if (VuState() > VU_MEM_ACTIVE)
    {
        return;
    }

    if (InSimLists())
        return;

    SetInSimLists(1);
    SimDriver.AddToCampFeatList(this);
    cameraX;
    cameraY;
}

void ObjectiveClass::RemoveFromSimLists(void)
{
    if ( not InSimLists())
        return;

    SetInSimLists(0);
    SimDriver.RemoveFromCampFeatList(this);
}

void ObjectiveClass::DeaggregateFromData(VU_BYTE* data, long size)
{
    if (IsLocal() or not IsAggregate() or FalconLocalGame->IsLocal())
    {
        return;
    }

    //we use this for size tracking
    long *rem = &size;

    int f, fid, classID, added = 0;
    SimInitDataClass simdata;
    SimBaseClass* newObject;
    FalconSessionEntity* session;
    VU_ID vuid;
    uchar len, features, value;
    FeatureClassDataType* fc;
    float x, y, z;
    SimFeatureFilter filter;
    ulong creator;

    SetComponents(new TailInsertList(&filter));
    GetComponents()->Register();

    vuid.num_ = VU_SESSION_ENTITY_ID;
    memcpychk(&vuid.creator_, &data, sizeof(VU_SESSION_ID), rem);
    memcpychk(&creator, &data, sizeof(VU_SESSION_ID), rem);
    memcpychk(&obj_data.obj_flags, &data, sizeof(short), rem);
    memcpychk(&len, &data, sizeof(uchar), rem);
    memcpychk(obj_data.fstatus, &data, len, rem);
    memcpychk(&features, &data, sizeof(uchar), rem);

    if ((session = (FalconSessionEntity*) vuDatabase->Find(vuid)) == NULL)
    {
        return;
    }

    // Set the owner of the newly created deaggregated entities
    SetDeagOwner(session->Id());
    simdata.forcedId.creator_ = creator;

    SetAggregate(false);

    // Set up the init data structure.
    memset(simdata.weapon, 0, sizeof(short)*HARDPOINT_MAX);
    memset(simdata.weapons, 0, sizeof(unsigned char)*HARDPOINT_MAX);
    simdata.vehicleInUnit = 255;
    simdata.playerSlot = 255;
    simdata.skill = 0;
    simdata.fuel = 0;
    simdata.waypointList = NULL;
    simdata.currentWaypoint = 0;
    simdata.numWaypoints = 0;
    simdata.createType = SimInitDataClass::CampaignFeature;
    simdata.z = 0;
    simdata.side = GetOwner();
    simdata.campBase = this;
    simdata.createFlags = SIDC_SILENT_INSERT bitor SIDC_FORCE_ID;

    if (session not_eq FalconLocalSession)
    {
        simdata.createFlags or_eq SIDC_REMOTE_OWNER;
        simdata.owner = session;
    }

    fid = static_data.class_data->FirstFeature;

    for (f = 0; f < features; f++, fid++)
    {
        memcpychk(&value, &data, sizeof(uchar), rem);
        simdata.campSlot = (Int32) value;

        if (simdata.campSlot == 255)
            continue;

        ShiAssert(simdata.campSlot == f);
        classID = GetFeatureID(f);
        fc = GetFeatureClassData(classID);
        simdata.descriptionIndex = classID + VU_LAST_ENTITY_TYPE;
        simdata.flags = fc->Flags;
        simdata.specialFlags = FeatureEntryDataTable[fid].Flags;
        GetFeatureOffset(simdata.campSlot, &y, &x, &z);
        simdata.x = XPos() + x;
        simdata.y = YPos() + y;
        simdata.heading = (float)(FeatureEntryDataTable[fid].Facing) * DEG_TO_RADIANS;
        simdata.displayPriority = fc->Priority;

        memcpychk(&value, &data, sizeof(uchar), rem);
        simdata.status = (Int32) value;
        memcpychk(&simdata.forcedId.num_, &data, sizeof(VU_ID_NUMBER), rem);
        newObject = AddObjectToSim(&simdata, 0);

        if (newObject)
        {
            GetComponents()->ForcedInsert(newObject);
            added++;
        }
    }

    // Update our local wake status
#if NEW_WAKE

    if (OwnerId() == vuLocalSessionEntity->OwnerId())
    {
        Wake();
    }
    else
    {
        SetAwake(0);
    }

#else

    if ((session == FalconLocalSession) or (FalconLocalSession->InSessionBubble(this, 1.0F) > 0))
    {
        Wake();
    }
    else
    {
        SetAwake(0);
    }

#endif

    //finally insert it at deagg list
#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->ForcedInsert(this);
#else
    deaggregatedMap->insert(CampBaseBin(this));
#endif
}

void ObjectiveClass::ReaggregateFromData(VU_BYTE* data, long size)
{
    if (IsLocal() or IsAggregate())
        return;

#ifdef DEAG_DEBUG
    MonoPrint("Got remote reaggregation message for Objective #%d\n", GetCampID());
#endif

    ShiAssert( not FalconLocalGame->IsLocal());

    // Get current status
    uchar len;
    memcpy(&len, data, sizeof(uchar));
    data += sizeof(uchar);
    memcpy(obj_data.fstatus, data, len);
    data += len;

    if (IsAwake())
        Sleep();

    if (GetComponents())
    {
        CampEnterCriticalSection();
        {
            // destroy iterator before components
            VuEntity *feature, *nextFeature;
            VuListIterator myit(GetComponents());

            for (
                feature = myit.GetFirst();
                feature not_eq NULL;
                feature = nextFeature
            )
            {
                nextFeature = myit.GetNext();
                ((SimBaseClass*)feature)->SetRemoveSilentFlag();
            }
        }
        GetComponents()->Unregister();
        delete GetComponents();
        SetComponents(NULL);
        CampLeaveCriticalSection();
    }

    SetAggregate(1);

#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->Remove(this);
#else
    deaggregatedMap->remove(this->Id());
#endif
}

void ObjectiveClass::TransferOwnershipFromData(VU_BYTE* data, long size)
{
    if (IsAggregate() or IsLocal() or not data)
    {
        return;
    }

    VU_ID vuid;
    uchar len;

    // Change the owner of the deaggregated entities
    SetDeagOwner(*(VU_ID*)data);
    data += sizeof(VU_ID);
    //memcpy(&deag_owner, data, sizeof(VU_ID)); data += sizeof(VU_ID);
    memcpy(&len, data, sizeof(uchar));
    data += sizeof(uchar);
    memcpy(obj_data.fstatus, data, len);
    data += len;

#ifdef DEAG_DEBUG
    // MonoPrint ("Transfering ownership of remote objective #%d - new owner is %d\n",GetCampID(),deag_owner.creator_.value_);
#endif

    // Change ownership locally
    if (GetComponents())
    {
        VuEntity* feature;
        VuListIterator myit(GetComponents());

        feature = myit.GetFirst();

        while (feature)
        {
            ((SimBaseClass*)feature)->ChangeOwner(GetDeagOwner());
            feature = myit.GetNext();
        }
    }

    // Update our local wake status
    if (IsAwake() and not FalconLocalSession->InSessionBubble(this, REAGREGATION_RATIO))
    {
        Sleep();
    }
    else if (
 not IsAwake() and (
            GetDeagOwner() == FalconLocalSession->Id() or FalconLocalSession->InSessionBubble(this, 1.0F) > 0
        )
    )
    {
        Wake();
    }

    return;
}

// This Apply damage is called only for local entities, and resolves the number and type
// of weapon shots vs this objective.
// HOWEVER, it them broadcasts a FalconWeaponFireMessage which will generate visual effects,
// update remote copies of this entity, call the mission evaluation/event storage routines,
// and add any craters we require.
int ObjectiveClass::ApplyDamage(FalconCampWeaponsFire *cwfm, uchar bonusToHit)
{
    Int32 i, hc, range, shot, losses = 0, totalShots = 0;
    GridIndex sx, sy, tx, ty;
    int str = 0, strength, flags;
    DamType dt;
    Unit shooter = (Unit)vuDatabase->Find(cwfm->dataBlock.shooterID);
    uchar size, addcrater = 0;

    if ( not IsLocal())
        return 0;

    if ( not shooter)
        return 0;

    ShiAssert(IsAggregate())

    gDamageStatusPtr = gDamageStatusBuffer + 1;
    gDamageStatusBuffer[0] = 0;
    shooter->GetLocation(&sx, &sy);
    GetLocation(&tx, &ty);
    range = FloatToInt32(Distance(sx, sy, tx, ty));

    // Unit type specific stuff
    if (shooter->IsFlight() and cwfm->dataBlock.dPilotId == 255)
    {
        WayPoint w = shooter->GetCurrentUnitWP();

        // If we're attacking our pre-assigned WP target, check for a specific feature ID
        if (w and w->GetWPTargetID() == shooter->GetTargetID())
            cwfm->dataBlock.dPilotId = w->GetWPTargetBuilding();
    }

    for (i = 0; i < MAX_TYPES_PER_CAMP_FIRE_MESSAGE and cwfm->dataBlock.weapon[i] and cwfm->dataBlock.shots[i]; i++)
    {
        hc = GetWeaponHitChance(cwfm->dataBlock.weapon[i], NoMove, range) + bonusToHit;

        // Flight's get bonuses to hit based on vehicle type (ground vehicles should too -
        // but at this point, we don't really know which vehicle shot which weapon)

        // A.S. removed, as objectives have no defensive bonus
        //Cobra Let's put this back for now
        //if ( not CampBugFixes)
        //{
        if (shooter->IsFlight())
            hc += GetVehicleClassData(shooter->GetVehicleID(0))->HitChance[NoMove];

        //}
        // end removed

        // HARMs will snap to current radar feature, if we're emitting
        if ((WeaponDataTable[cwfm->dataBlock.weapon[i]].GuidanceFlags bitand WEAP_ANTIRADATION) and IsEmitting())
            cwfm->dataBlock.dPilotId = static_data.class_data->RadarFeature;

        // Tally the losses
        strength = GetWeaponStrength(cwfm->dataBlock.weapon[i]);
        str = 0;
        dt = (DamType)GetWeaponDamageType(cwfm->dataBlock.weapon[i]);

        if (dt == NuclearDam)
            hc = max(95, hc); // minimum of 95% hitchance for nukes.

        flags = GetWeaponFlags(cwfm->dataBlock.weapon[i]);
        shot = 0;
        totalShots += cwfm->dataBlock.shots[i];

        while (cwfm->dataBlock.shots[i] - shot > 0)
        {
            if (rand() % 100 < hc)
            {
                str += strength;
                losses += ApplyDamage(dt, &str, cwfm->dataBlock.dPilotId, (short)flags);
            }
            else if (shooter->IsFlight() and addcrater < 3)
            {
                // Add a few craters if it's an air attack
                addcrater++;
            }

            shot += 1 + (rand() % (losses + 1)); // Random stray shots - let's be nice
        }
    }

#ifdef KEEP_STATISTICS

    if (shooter->IsFlight())
    {
        AS_Kills += losses;
        AS_Shots += totalShots;
    }

#endif

    // Record # of craters to add
    *gDamageStatusPtr = addcrater;
    gDamageStatusPtr++;

    // Record the final state, to keep remote entities consitant
    size = (uchar)((static_data.class_data->Features + 3) / 4);
    *gDamageStatusPtr = size;
    gDamageStatusPtr++;
    memcpy(gDamageStatusPtr, obj_data.fstatus, size);
    gDamageStatusPtr += size;

    // Copy the data into the message
    cwfm->dataBlock.size = (ushort)(gDamageStatusPtr - gDamageStatusBuffer);
    cwfm->dataBlock.data = new uchar[cwfm->dataBlock.size];
    memcpy(cwfm->dataBlock.data, gDamageStatusBuffer, cwfm->dataBlock.size);

    // Send the weapon fire message (with target's post-damage status)
    FalconSendMessage(cwfm, FALSE);

    return losses;
}

// This ApplyDamage() simply chooses which features to eliminate and sets their status appropriately
int ObjectiveClass::ApplyDamage(DamType d, int *str, int f, short flags)
{
    int fid, hp, lost = 0, count = 0, s, this_pass;
    FeatureClassDataType* fc;

    while (*str > 0 and count < MAX_DAMAGE_TRIES)
    {
        count++;

        if (f >= static_data.class_data->Features or f < 0 or GetFeatureStatus(f) == VIS_DESTROYED or not GetFeatureID(f))
        {
            // Find something to bomb
            for (fid = 0, f = 255; fid < static_data.class_data->Features and f >= static_data.class_data->Features ; fid++)
            {
                if (GetFeatureStatus(fid) not_eq VIS_DESTROYED and GetFeatureClassData(GetFeatureID(fid))->DamageMod[d] > 0)
                    f = fid;
            }

            continue; // Force another pass;
        }

        fid = GetFeatureID(f);
        fc = GetFeatureClassData(fid);

        if (fc and fc->DamageMod[d] > 0)
        {
            hp = fc->HitPoints * 100 / fc->DamageMod[d];

            // Check if high explosive damage will do more
            if ((flags bitand WEAP_AREA) and fc->DamageMod[HighExplosiveDam] > fc->DamageMod[d])
                hp = fc->HitPoints * 100 / fc->DamageMod[HighExplosiveDam];

            s = GetFeatureStatus(f);

            if (s == VIS_DAMAGED)
                hp /= 2; // Feature is already damaged

            this_pass = 1;

            //if (s not_eq VIS_DESTROYED and *str > rand()%hp)
            if (s not_eq VIS_DESTROYED and (hp == 0 or *str > rand() % hp)) // JB 010401 CTD
                s = VIS_DESTROYED;
            //else if (s not_eq VIS_DESTROYED and s not_eq VIS_DAMAGED and *str > rand()%(hp/2))
            else if (s not_eq VIS_DESTROYED and s not_eq VIS_DAMAGED and (hp < 2 or *str > rand() % (hp / 2))) // JB 010401 CTD
                s = VIS_DAMAGED;
            else
            {
                count = MAX_DAMAGE_TRIES; // Stop applying damage, since we can't hurt our target
                this_pass = 0; // Didn't actually kill this
                return lost;
            }

            if (this_pass)
            {
                SetFeatureStatus(f, s);
                *gDamageStatusPtr = (uchar)f;
                gDamageStatusPtr++;
                *gDamageStatusPtr = (uchar)s;
                gDamageStatusPtr++;
                gDamageStatusBuffer[0]++;
                lost++;
            }

            if ( not (flags bitand WEAP_AREA)) // Not area effect weapon, only get one kill per shot
                *str = 0;
            else if (*str > MINIMUM_STRENGTH * 2) // Otherwise halve our strength and keep applying damage
                *str /= 2; // NOTE: This doesn't guarentee nearest adjacent feature
            else
                count = MAX_DAMAGE_TRIES;
        }
        else
            f = rand() % static_data.class_data->Features;
    }

    ResetObjectiveStatus();

    return lost;
}

// This is where the guts of the damage routine take place.
// All players handle this message in order to keep objective status and event messages consistant
int ObjectiveClass::DecodeDamageData(uchar *data, Unit shooter, FalconDeathMessage *dtm)
{
    int lost, f, i, s, islocal = IsLocal();
    uchar size, addcrater;

    lost = *data;
    data++;

    for (i = 0; i < lost; i++)
    {
        // Score each hit, for mission evaluator
        f = *data;
        data++;
        s = *data;
        data++;

        // Record status only for remote entities
        if ( not islocal)
            SetFeatureStatus(f, s);

        // Add runway craters
        if (Falcon4ClassTable[GetFeatureID(f)].vuClassData.classInfo_[VU_TYPE] == TYPE_RUNWAY) // (IS_RUNWAY)
        {
            if (s == VIS_DESTROYED)
                AddRunwayCraters(this, f, 8);
            else if (s == VIS_DAMAGED)
                AddRunwayCraters(this, f, 4);
        }

        // Generate a death message if we or the shooter is a member of the package
        if (dtm)
        {
            FeatureClassDataType* fc;
            fc = GetFeatureClassData(GetFeatureID(f));
            dtm->dataBlock.dPilotID = 255;
            dtm->dataBlock.dIndex = (ushort)(fc->Index + VU_LAST_ENTITY_TYPE);
            // Update squadron and pilot statistics
            EvaluateKill(dtm, NULL, shooter, NULL, this);
        }
        else if (shooter->IsFlight())
            EvaluateKill(dtm, NULL, shooter, NULL, this);
    }

    // Add any necessary craters
    addcrater = *data;
    data++;
    AddMissCraters(this, addcrater);

    // Record the current state of all features, for consistancy
    // (this is admittidly redundant, but could help avoid problems with missed messages)
    if ( not islocal)
    {
        size = *data;
        data++;
        ShiAssert(size == (static_data.class_data->Features + 3) / 4);
        memcpy(obj_data.fstatus, data, size);
        data += size;
    }

    // Reset objective status upon losses
    if (lost)
    {
        if (GetObjectiveStatus() == 100)
            SetObjectiveRepairTime(Camp_GetCurrentTime());

        // Reset local status
        ResetObjectiveStatus();

        // The local entity sends atm a message if there's a chance we lost a runway
        // 2001-08-01 MODIFIED BY S.G. ARMYBASE SHOULD BE DEALT WITH TOO SINCE THEY CARRY CHOPPERS
        // if (islocal and GetType() == TYPE_AIRBASE and GetObjectiveStatus() < 51)
        if (islocal and (GetType() == TYPE_AIRBASE or GetType() == TYPE_ARMYBASE) and GetObjectiveStatus() < 51)
            TeamInfo[GetTeam()]->atm->SendATMMessage(Id(), GetTeam(), FalconAirTaskingMessage::atmZapAirbase, 0, 0, NULL, 0);
    }

    ResetObjectiveStatus();

    return lost;
}

void ObjectiveClass::Repair(void)
{
    int repair, bf;
    CampaignTime time;

    if (GetObjectiveStatus() not_eq 100)
    {
        time = Camp_GetCurrentTime() - GetObjectiveRepairTime();
        repair = time / CampaignHours;
        bf = BestRepairFeature(this, &repair);

        // JB 000811
        // Repair one feature at a time and reset the repair time
        // only after something has been fixed.
        /*//- while (bf > -1)
          {
          RepairFeature(bf);
          bf = BestRepairFeature(this, &repair);
          }
        //-*/
        if (bf > -1)
        {
            RepairFeature(bf);
            SetObjectiveRepairTime(Camp_GetCurrentTime());
        }
    }

    // SetObjectiveRepairTime(Camp_GetCurrentTime()); //-
    // JB 000811

    ResetObjectiveStatus();
}

uchar* ObjectiveClass::GetDamageModifiers(void)
{
    ObjClassDataType* oc;

    oc = GetObjectiveClassData();

    if ( not oc)
        return 0;

    return oc->DamageMod;
}

// Returns best hitchance of the objective at a given range
//int ObjectiveClass::GetHitChance (int mt, int range)
int ObjectiveClass::GetHitChance(int, int)
{
    return 0;
    // Commented out body removed ny leonr
}

// Quicker, aproximate version of the above
//int ObjectiveClass::GetAproxHitChance (int mt, int range)
int ObjectiveClass::GetAproxHitChance(int, int)
{
    return 0;
    // Commented out body removed ny leonr
}

// Returns strength of the objective at a given range
//int ObjectiveClass::GetCombatStrength (int mt, int range)
int ObjectiveClass::GetCombatStrength(int, int)
{
    return 0;
    // Commented out body removed ny leonr
}

// Quicker, aproximate version of the above
//int ObjectiveClass::GetAproxCombatStrength (int mt, int range)
int ObjectiveClass::GetAproxCombatStrength(int, int)
{
    // Commented out body removed ny leonr
    return 0;
}

//int ObjectiveClass::GetWeaponRange (int mt)
int ObjectiveClass::GetWeaponRange(int, FalconEntity *target)  // 2008-03-08 ADDED SECOND DEFAULT PARM
{
    // Commented out body removed ny leonr
    return 0;
}

// Returns the maximum range of the objective (this is precalculated)
//int ObjectiveClass::GetAproxWeaponRange (int mt)
int ObjectiveClass::GetAproxWeaponRange(int)
{
    // Commented out body removed ny leonr
    return 0;
}

int ObjectiveClass::GetDetectionRange(int mt)
{
    ObjClassDataType* oc = GetObjectiveClassData();
    int dr = 0;

    ShiAssert(oc);

    if (IsEmitting() and oc->RadarFeature < 255 and GetFeatureStatus(oc->RadarFeature) not_eq VIS_DESTROYED)
        // 2001-04-21 MODIFIED BY S.G. ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD...
        // dr = oc->Detection[mt];
    {
        if ((dr = oc->Detection[mt]) > 250)
            dr = 250 + (oc->Detection[mt] - 250) * 50;
    }

    // END OF MODIFIED SECTION
    if ( not dr)
        dr = GetVisualDetectionRange(mt);

    return dr;
}

int ObjectiveClass::GetElectronicDetectionRange(int mt)
{
    if (static_data.class_data->RadarFeature < 255 and GetFeatureStatus(static_data.class_data->RadarFeature) not_eq VIS_DESTROYED)
        // 2001-04-21 MODIFIED BY S.G. ABOVE 250 HAS A NEW MEANING SO USE THE UNIT ELECTRONIC DETECTION RANGE INSTEAD...
        // return static_data.class_data->Detection[mt];
    {
        if (static_data.class_data->Detection[mt] > 250)
            return 250 + (static_data.class_data->Detection[mt] - 250) * 50;

        return static_data.class_data->Detection[mt];
    }

    return 0;
}

int ObjectiveClass::CanDetect(FalconEntity* ent)
{
    float ds, mrs, vdr, dx, dy;
    MoveType mt;

    mt = ent->GetMovementType();
    ds = DistSqu(XPos(), YPos(), ent->XPos(), ent->YPos());
    mrs = (float)(GetDetectionRange(mt) * KM_TO_FT);
    mrs *= mrs;

    if (ds > mrs)
        return 0;

    // Additional detection requirements against aircraft
    if (mt == LowAir or mt == Air)
    {
        // 2001-04-22 ADDED BY S.G. OBJECTIVES NEEDS TO BE AFFECTED BY SOJ AS MUCH AS UNITS
        if (ent->IsFlight())
        {
            Flight ecmFlight = ((FlightClass *)ent)->GetECMFlight();

            if (((FlightClass *)ent)->HasAreaJamming())
                ecmFlight = (FlightClass *)ent;
            else if (ecmFlight)
            {
                if ( not ecmFlight->IsAreaJamming())
                    ecmFlight = NULL;
            }

            if (ecmFlight)
            {
                // Now, here's what we need to do:
                // 1. For now jamming power has 360 degrees coverage
                // 2. The radar range will be reduced by the ratio of its normal range and the jammer's range to the radar to the power of two
                // 3. The jammer is dropping the radar gain, effectively dropping its detection distance
                // 4. If the flight is outside this new range, it's not detected.

                // Get the range of the SOJ to the radar
                float jammerRange = DistSqu(ecmFlight->XPos(), ecmFlight->YPos(), XPos(), YPos());

                // If the SOJ is within the radar normal range, 'adjust' it. If this is now less that ds (our range to the radar), return 0.
                // SOJ can jamm even if outside the detection range of the radar
                if (jammerRange < mrs * 2.25f)
                {
                    jammerRange = jammerRange / (mrs * 2.25f); // No need to check for zero because jammerRange has to be LESS than mrs to go in
                    mrs *= jammerRange * jammerRange;

                    if (ds > mrs)
                    {
                        // The radar is being jammed, check if visual detection will do
                        vdr = (float)GetVisualDetectionRange(mt) * KM_TO_FT;
                        vdr *= vdr;

                        if (ds < vdr)
                            return 1;

                        // Nope, then it's not detected.
                        return 0;
                    }
                }
            }
        }

        // END OF ADDED SECTION
        if ( not HasRadarRanges())
        {
            // Only check vs visual detection range
            // 2001-03-16 MODIFIED BY S.G. LOOKS LIKE THEY FORGOT GetVisualDetectionRange IS IN KILOMETERS AND NOT FEET
            // vdr = (float)GetVisualDetectionRange(mt);
            vdr = (float)GetVisualDetectionRange(mt) * KM_TO_FT;
            vdr *= vdr;

            if (ds < vdr)
                return 1;

            return 0;
        }

        dx = ent->XPos() - XPos();
        dy = ent->YPos() - YPos();

        // Stealth aircraft act as if they're flying at double their range
        if (ent->IsFlight())
        {
            UnitClassDataType *uc = ((Flight)ent)->GetUnitClassData();

            if (uc->Flags bitand VEH_STEALTH)
            {
                // 2001-04-29 MODIFIED BY S.G. IF IT'S A STEALTH AND IT GOT HERE, IT WASN'T DETECTED VISUALLY SO ABORT RIGHT NOW
                // dx *= 2.0F;
                // dy *= 2.0F;
                vdr = (float)GetVisualDetectionRange(mt) * KM_TO_FT;
                vdr *= vdr;

                if (ds < vdr)
                    return 1;

                return 0;
            }
        }

        // 2001-04-06 MODIFIED BY S.G. NEED TO ACCOMODATE FOR THE OBJECTIVE'S MSL ALTITUDE SINCE ZPos IS 0 FOR OBJECTIVE...
        // return static_data.radar_data->CanDetect(dx,dy,ent->ZPos()-ZPos());
        ShiAssert(ZPos() == 0.0f); // Warn if the objective ZPos is something else than 0
        float AGLz = ent->ZPos() + TheMap.GetMEA(XPos(), YPos());

        if (AGLz > 0.0f)
            return 0; // Can't see very well if our target is lower than us...

        return static_data.radar_data->CanDetect(dx, dy, AGLz); // GetMEA returns a positive number that we must substract from our target altitude
    }

    return 1;
}

int ObjectiveClass::GetRadarMode(void)
{
    if (IsEmitting())
        return FEC_RADAR_SEARCH_100;
    else
        return FEC_RADAR_OFF;
}

int ObjectiveClass::GetRadarType(void)
{
    ObjClassDataType *data = GetObjectiveClassData();
    ShiAssert(data);

    if (data->RadarFeature < 255)
    {
        return GetFeatureClassData(GetFeatureID(data->RadarFeature))->RadarType;
    }
    else
    {
        return RDR_NO_RADAR;
    }
}

int ObjectiveClass::GetNumberOfArcs(void)
{
    if ( not HasRadarRanges())
        return 1;

    return static_data.radar_data->GetNumberOfArcs();
}

float ObjectiveClass::GetArcRatio(int anum)
{
    if ( not HasRadarRanges())
        return 0.0F;

    return static_data.radar_data->GetArcRatio(anum);
}

float ObjectiveClass::GetArcRange(int anum)
{
    if ( not HasRadarRanges())
        return 0.0F;

    return static_data.radar_data->GetArcRange(anum);
}

void ObjectiveClass::GetArcAngle(int anum, float* a1, float *a2)
{
    if ( not HasRadarRanges())
    {
        *a1 = 0.0F;
        *a2 = 2.0F * PI;
        return;
    }

    static_data.radar_data->GetArcAngle(anum, a1, a2);
}

int ObjectiveClass::SiteCanDetect(FalconEntity* ent)
{
    float dx, dy;

    if ( not HasRadarRanges())
        return 0;

    dx = ent->XPos() - XPos();
    dy = ent->YPos() - YPos();

    // Stealth aircraft act as if they're flying at double their range
    if (ent->IsFlight())
    {
        UnitClassDataType *uc = ((Flight)ent)->GetUnitClassData();

        if (uc->Flags bitand VEH_STEALTH)
        {
            dx *= 2.0F;
            dy *= 2.0F;
        }
    }

    return static_data.radar_data->CanDetect(dx, dy, ent->ZPos() - ZPos());
}

float ObjectiveClass::GetSiteRange(FalconEntity* ent)
{
    float dx, dy;

    if ( not HasRadarRanges())
        return 0.0F;

    dx = ent->XPos() - XPos();
    dy = ent->YPos() - YPos();
    return static_data.radar_data->GetRadarRange(dx, dy, ent->ZPos() - ZPos());
}

void ObjectiveClass::SetObjectiveClass(int dindex)
{
    int nsize;

    SetEntityType((ushort)dindex);
    static_data.class_data = (ObjClassDataType*)Falcon4ClassTable[dindex - VU_LAST_ENTITY_TYPE].dataPtr;

    if (obj_data.fstatus)
        delete [] obj_data.fstatus;

    nsize = ((static_data.class_data->Features * 2) + 7) / 8;
#ifdef USE_SH_POOLS
    obj_data.fstatus = (uchar *)MemAllocPtr(gObjMemPool, sizeof(uchar) * nsize, 0);
#else
    obj_data.fstatus = new uchar[nsize];
#endif
    memset(obj_data.fstatus, 0, nsize);
}

void ObjectiveClass::SendObjMessage(VU_ID from, short mes, short d1, short d2, short d3)
{
    FalconObjectiveMessage *message = new FalconObjectiveMessage(Id(), FalconLocalGame);

    message->dataBlock.from = from;
    message->dataBlock.message = mes;
    message->dataBlock.data1 = d1;
    message->dataBlock.data2 = d2;
    message->dataBlock.data3 = d3;
    FalconSendMessage(message, TRUE);
}

void ObjectiveClass::AddObjectiveNeighbor(Objective o, uchar c[MOVEMENT_TYPES])
{
    int i;
    Objective n;
    CampObjectiveLinkDataType* tmp_link;

    // Find if it exists first
    for (i = 0; i < static_data.links; i++)
    {
        n = (Objective)(vuDatabase->Find(link_data[i].id));

        if (n and n == o)
        {
            SetNeighborCosts(i, c);
            return;
        }
    }

    // Otherwise, add a new one
    static_data.links++;
    tmp_link = link_data;
#ifdef USE_SH_POOLS
    link_data = (CampObjectiveLinkDataType *)MemAllocPtr(gObjMemPool, sizeof(CampObjectiveLinkDataType) * static_data.links, 0);
#else
    link_data = new CampObjectiveLinkDataType[static_data.links];
#endif
    memcpy(link_data, tmp_link, (static_data.links - 1)*sizeof(CampObjectiveLinkDataType));
    delete [] tmp_link;
    link_data[static_data.links - 1].id = o->Id();
    SetNeighborCosts(i, c);
}

void ObjectiveClass::SetNeighborCosts(int num, uchar c[MOVEMENT_TYPES])
{
    int j;

    if (num >= static_data.links)
        return;

    for (j = 0; j < MOVEMENT_TYPES; j++)
    {
        if (c[j] < 255)
            link_data[num].costs[j] = (uchar) c[j];
        else
            link_data[num].costs[j] = 255;
    }
}

void ObjectiveClass::RemoveObjectiveNeighbor(int n)
{
    CampObjectiveLinkDataType* tmp_link;
    int nn;

    if (n >= static_data.links)
        return;

    static_data.links--;
    tmp_link = link_data;
#ifdef USE_SH_POOLS
    link_data = (CampObjectiveLinkDataType *)MemAllocPtr(gObjMemPool, sizeof(CampObjectiveLinkDataType) * static_data.links, 0);
#else
    link_data = new CampObjectiveLinkDataType[static_data.links];
#endif

    for (nn = 0; nn < static_data.links + 1; nn++)
    {
        if (nn < n)
            memcpy(&link_data[nn], &tmp_link[nn], sizeof(CampObjectiveLinkDataType));

        if (nn > n)
            memcpy(&link_data[nn - 1], &tmp_link[nn], sizeof(CampObjectiveLinkDataType));
    }

    delete [] tmp_link;
}

void ObjectiveClass::SetObjectiveType(ObjectiveType t)
{
    uchar type, stype;
    int dindex;

    type = t;
    stype = 1; // Try for a real objective
    dindex = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, type, stype, 0, 0, 0, 0);

    if ( not dindex)
        return;

    SetObjectiveClass(dindex + VU_LAST_ENTITY_TYPE);
    UpdateObjectiveLists();
}

void ObjectiveClass::SetObjectiveSType(uchar s)
{
    uchar type, stype;
    int dindex;

    type = GetType();
    stype = s; // Try for a real objective
    dindex = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, type, stype, 0, 0, 0, 0);

    if ( not dindex)
        return;

    SetObjectiveClass(dindex + VU_LAST_ENTITY_TYPE);
    UpdateObjectiveLists();
}

void ObjectiveClass::SetObjectiveName(char* name)
{
    int nid;

    nid = GetObjectiveNameID();

    if (name == NULL or name[0] == 0 or name[0] == '0')
    {
        if ( not nid)
            return;

        SetObjectiveNameID(0);
        return;
    }

    if (nid == 0)
    {
        nid = AddName(name);
        SetObjectiveNameID((short)nid);
    }
    else
        SetName(nid, name);
}

_TCHAR* ObjectiveClass::GetName(_TCHAR* name, int size, int mode)
{
    int nid, pnid = 0;
    Objective p;
    _TCHAR buffer[80];

    nid = GetObjectiveNameID();

    if (nid == 0)
    {
        p = GetObjectiveParent();

        if (p)
        {
            pnid = p->GetObjectiveNameID();

            if (gLangIDNum == F4LANG_FRENCH)
            {
                _TCHAR namestr[80];
                ReadNameString(pnid, namestr, 79);

                if (namestr[0] == 'A' or namestr[0] == 'a' or namestr[0] == 'E' or namestr[0] == 'e' or
                    namestr[0] == 'I' or namestr[0] == 'i' or namestr[0] == 'O' or namestr[0] == 'o' or
                    namestr[0] == 'U' or namestr[0] == 'u')
                    _sntprintf(name, size, "%s d'%s", ObjectiveStr[GetType()], namestr);
                else
                    _sntprintf(name, size, "%s de %s", ObjectiveStr[GetType()], namestr);
            }
            else if (gLangIDNum == F4LANG_ITALIAN or gLangIDNum == F4LANG_SPANISH or gLangIDNum == F4LANG_PORTUGESE)
                _sntprintf(name, size, "%s %s", ObjectiveStr[GetType()], ReadNameString(pnid, buffer, 79));
            else
                _sntprintf(name, size, "%s %s", ReadNameString(pnid, buffer, 79), ObjectiveStr[GetType()]);
        }
        else
            _sntprintf(name, size, "%s", ReadNameString(nid, buffer, 79));
    }
    else
        _sntprintf(name, size, "%s", ReadNameString(nid, buffer, 79));

    if (mode and _istlower(name[0]))
        name[0] = (char)_toupper(name[0]);

    // _sntprintf should do this for us, but for some reason it sometimes doesn't
    name[size - 1] = 0;

    return name;
}

_TCHAR* ObjectiveClass::GetFullName(_TCHAR* name, int size, int object)
{
    return GetName(name, size, object);
}

void ObjectiveClass::DisposeObjective(void)
{
    Objective  no;
    int n, on;

    // Kill links _from_ linked objectives. Our links will die when we do.
    for (n = 0; n < static_data.links; n++)
    {
        no = GetNeighbor(n);

        for (on = 0; on < no->static_data.links; on++)
        {
            if (no->GetNeighborId(on) == Id())
                no->RemoveObjectiveNeighbor(on);
        }
    }

    Remove();
}

int ObjectiveClass::IsPrimary(void)
{
    if (GetType() == TYPE_CITY and obj_data.priority > PRIMARY_OBJ_PRIORITY)
        return 1;

    return 0;
}

int ObjectiveClass::IsSecondary(void)
{
    // Only cities and towns can be secondary objectives, and ALL automatically are
    if ((GetType() == TYPE_CITY or GetType() == TYPE_TOWN) and obj_data.priority > SECONDARY_OBJ_PRIORITY)
        return 1;

    return 0;
}

int ObjectiveClass::IsSupplySource(void)
{
    if (GetType() == TYPE_CITY or GetType() == TYPE_PORT or GetType() == TYPE_DEPOT or GetType() == TYPE_ARMYBASE)
    {
        if ( not IsFrontline() and not IsSecondline())
        {
            return 1;
        }
    }

    return 0;
}

int ObjectiveClass::HasRadarRanges(void)
{
    if (static_data.radar_data)
        return 1;

    return 0;
}

void ObjectiveClass::SetManual(int s)
{
    obj_data.obj_flags or_eq O_MANUAL_SET;

    if ( not s)
        obj_data.obj_flags xor_eq O_MANUAL_SET;
}

void ObjectiveClass::SetJammed(int j)
{
    obj_data.obj_flags or_eq O_JAMMED;

    if ( not j)
        obj_data.obj_flags xor_eq O_JAMMED;
}

void ObjectiveClass::SetSamSite(int s)
{
    obj_data.obj_flags or_eq O_SAM_SITE;

    if ( not s)
        obj_data.obj_flags xor_eq O_SAM_SITE;
}

void ObjectiveClass::SetArtillerySite(int a)
{
    obj_data.obj_flags or_eq O_ARTILLERY_SITE;

    if ( not a)
        obj_data.obj_flags xor_eq O_ARTILLERY_SITE;
}

void ObjectiveClass::SetAmbushCAPSite(int a)
{
    obj_data.obj_flags or_eq O_AMBUSHCAP_SITE;

    if ( not a)
        obj_data.obj_flags xor_eq O_AMBUSHCAP_SITE;
}

void ObjectiveClass::SetBorderSite(int a)
{
    obj_data.obj_flags or_eq O_BORDER_SITE;

    if ( not a)
        obj_data.obj_flags xor_eq O_BORDER_SITE;
}

void ObjectiveClass::SetMountainSite(int a)
{
    obj_data.obj_flags or_eq O_MOUNTAIN_SITE;

    if ( not a)
        obj_data.obj_flags xor_eq O_MOUNTAIN_SITE;
}

void ObjectiveClass::SetCommandoSite(int c)
{
    obj_data.obj_flags or_eq O_COMMANDO_SITE;

    if ( not c)
        obj_data.obj_flags xor_eq O_COMMANDO_SITE;
}

void ObjectiveClass::SetFlatSite(int a)
{
    obj_data.obj_flags or_eq O_FLAT_SITE;

    if ( not a)
        obj_data.obj_flags xor_eq O_FLAT_SITE;
}

void ObjectiveClass::SetRadarSite(int r)
{
    obj_data.obj_flags or_eq O_RADAR_SITE;

    if ( not r)
        obj_data.obj_flags xor_eq O_RADAR_SITE;
}

void ObjectiveClass::SetAbandoned(int a)
{
    obj_data.obj_flags or_eq O_ABANDONED;

    if ( not a)
        obj_data.obj_flags xor_eq O_ABANDONED;
}

void ObjectiveClass::SetNeedRepair(int r)
{
    obj_data.obj_flags or_eq O_NEED_REPAIR;

    if ( not r)
        obj_data.obj_flags xor_eq O_NEED_REPAIR;
}

// This will add the objectives into the emitter and sam lists
void ObjectiveClass::UpdateObjectiveLists(void)
{
}

Objective ObjectiveClass::GetNeighbor(int num)
{
    Objective n;

    if (num >= static_data.links)
        return NULL;

    n = (Objective) vuDatabase->Find(link_data[num].id);

    if ( not n)
    {
        // Better axe this, since we couldn't find it.
        RemoveObjectiveNeighbor(num);
    }

    return n;
}

Objective ObjectiveClass::GetObjectiveSecondary(void)
{
    if (IsSecondary())
        return this;
    else
        return (Objective) vuDatabase->Find(static_data.parent);
}

Objective ObjectiveClass::GetObjectivePrimary(void)
{
    if (IsPrimary())
        return this;
    else if (IsSecondary())
        return (Objective) vuDatabase->Find(static_data.parent);
    else
    {
        Objective so = (Objective) vuDatabase->Find(static_data.parent);

        if (so)
            return so->GetObjectivePrimary();
    }

    return NULL;
}

void ObjectiveClass::SetFeatureStatus(int f, int n)
{
    int i = f / 4;

    if (GetFeatureStatus(f) == n)
        return;

    // Check for critical links and set those features accordingly. NOTE: repair accross critical links too..
    if (n == VIS_DESTROYED or n == VIS_REPAIRED)
    {
        if (FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Flags bitand FEAT_PREV_CRIT)
            SetFeatureStatus(f - 1, n, f);

        if (FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Flags bitand FEAT_NEXT_CRIT)
            SetFeatureStatus(f + 1, n, f);
    }

    f -= i * 4;
    obj_data.fstatus[i] = (uchar)((obj_data.fstatus[i] bitand compl (3 << (f * 2))) bitor (n << (f * 2)));
    //MakeObjectiveDirty (DIRTY_STATUS, DDP[9].priority);
    MakeObjectiveDirty(DIRTY_STATUS, SEND_NOW);
    SetDelta(1);
}

// This sets a feature and all it's critically linked features to a given status
// from is the feature causing the set or -1 if it's the first feature to be set (to prevent a loop from forming)
void ObjectiveClass::SetFeatureStatus(int f, int n, int from)
{
    int i = f / 4;

    if (GetFeatureStatus(f) == n)
        return;

    // Check for critical links and set those features accordingly.
    if (n == VIS_DESTROYED or n == VIS_REPAIRED)
    {
        if (from not_eq f - 1 and (FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Flags bitand FEAT_PREV_CRIT))
            SetFeatureStatus(f - 1, n, f);

        if (from not_eq f + 1 and (FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Flags bitand FEAT_NEXT_CRIT))
            SetFeatureStatus(f + 1, n, f);
    }

    f -= i * 4;
    obj_data.fstatus[i] = (uchar)((obj_data.fstatus[i] bitand compl (3 << (f * 2))) bitor (n << (f * 2)));
    ResetObjectiveStatus();
    SetDelta(1);
}

short ObjectiveClass::GetObjectiveDataRate(void)
{
    if ( not static_data.class_data)
        return 0;

    return (short)(static_data.class_data->DataRate * GetObjectiveStatus() / 100);
}

short ObjectiveClass::GetAdjustedDataRate(void)
{
    int almost;

    if ( not static_data.class_data)
        return 0;

    if ( not static_data.class_data->DataRate)
        static_data.class_data->DataRate = 1;

    almost = (100 / static_data.class_data->DataRate) - 1;
    return (short)((GetObjectiveStatus() + almost) * static_data.class_data->DataRate / 100);
}

int ObjectiveClass::GetFeatureStatus(int f)
{
    int i = f / 4;
    f -= i * 4;

    //if (f<=0)Cobra Put this back to below. This breaks damage stuff
    if (f < 0)
        return 0;

    if (f > 255) // FRB - garbage check
        return 0;

    if ( not obj_data.fstatus)
        return 0;

    return (obj_data.fstatus[i] >> (f * 2)) bitand 0x03;
}

int ObjectiveClass::GetFeatureValue(int f)
{
    if (f < 0)
        return 0;

    if (f > 255) // FRB - garbage check
        return 0;

    if ( not static_data.class_data)
        return 0;

    return FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Value;
}

int ObjectiveClass::GetFeatureRepairTime(int f)
{
    if (f < 0)
        return 0;

    if (f > 255) // FRB - garbage check
        return 0;

    if ( not static_data.class_data)
        return 0;

    return ::GetFeatureRepairTime(FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Index);
}

int ObjectiveClass::GetFeatureID(int f)
{
    if (f < 0)
        return 0;

    if (f > 255) // FRB - garbage check
        return 0;

    if ( not static_data.class_data)
        return 0;

    return FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Index;
}

int ObjectiveClass::GetFeatureOffset(int f, float* x, float* y, float* z)
{
    if (f < 0)
        return 0;

    if (f > 255) // FRB - garbage check
        return 0;

    if ( not static_data.class_data)
        return 0;

    *x = FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Offset.x;
    *y = FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Offset.y;
    *z = FeatureEntryDataTable[static_data.class_data->FirstFeature + f].Offset.z;
    return 1;
}

void ObjectiveClass::ResetObjectiveStatus(void)
{
    int f, s;

    s = 100;

    // Airbases use their own (slightly different) version of determining status:
    if (GetType() == TYPE_AIRBASE)
    {
        // AIRBASE version
        for (f = 0; s and f < static_data.class_data->Features; f++)
        {
            // Only adjust status for non-runways
            if (Falcon4ClassTable[GetFeatureID(f)].vuClassData.classInfo_[VU_TYPE] not_eq TYPE_RUNWAY) // (IS_RUNWAY)
            {
                if (GetFeatureStatus(f) == VIS_DAMAGED)
                    s -= GetFeatureValue(f) / 2;

                if (GetFeatureStatus(f) == VIS_DESTROYED)
                    s -= GetFeatureValue(f);
            }
        }

        if (s <= 0)
            s = 0;
        else
        {
            // Make sure we're below our maximum for # of active runways
            int index, runways = 0, inactive = 0, max;
            ObjClassDataType *oc = GetObjectiveClassData();
            index = oc->PtDataIndex;

            while (index)
            {
                if (PtHeaderDataTable[index].type == RunwayPt)
                {
                    runways++;

                    if (CheckHeaderStatus(this, index) == VIS_DESTROYED)
                        inactive++;
                }

                index = PtHeaderDataTable[index].nextHeader;
            }

            if ( not runways)
                max = 0;
            else
                max = ((runways - inactive) * 100) / runways;

            if (s > max)
                s = max;
        }
    }
    else
    {
        for (f = 0; s > 0 and f < static_data.class_data->Features; f++)
        {
            if (GetFeatureStatus(f) == VIS_DAMAGED)
                s -= GetFeatureValue(f) / 2;

            if (GetFeatureStatus(f) == VIS_DESTROYED)
                s -= GetFeatureValue(f);
        }

        if (s < 0)
            s = 0;
    }

    if (s not_eq obj_data.status)
    {
        SetObjectiveStatus((uchar)s);
    }
}

uchar ObjectiveClass::GetExpectedStatus(int hours)
{
    int bf, s;

    s = GetObjectiveStatus();
    hours += ((Camp_GetCurrentTime() - GetObjectiveRepairTime()) / CampaignHours);
    bf = BestRepairFeature(this, &hours);

    while (bf > -1)
    {
        s += GetFeatureValue(bf) / 2;
        bf = BestRepairFeature(this, &hours);
    }

    if (s > 100)
        s = 100;

    return (uchar)s;
}

// returns # of hours until the objective becomes status% operational
int ObjectiveClass::GetRepairTime(int status)
{
    int s, hours = 2400, f;

    ResetObjectiveStatus();
    s = GetObjectiveStatus();

    while (s < status)
    {
        f = BestRepairFeature(this, &hours);

        if (f < 0)
            break;

        s += GetFeatureValue(f) / 2;
    }

    return 2400 - hours;
}

uchar ObjectiveClass::GetBestTarget(void)
{
    int i, v, bv = 0, f = 0;

    for (i = 0; i < static_data.class_data->Features; i++)
    {
        v = GetFeatureValue(i);

        if (v > bv and GetFeatureStatus(i) not_eq VIS_DESTROYED)
        {
            bv = v;
            f = i;
        }
    }

    return (uchar)f;
}

ObjClassDataType* ObjectiveClass::GetObjectiveClassData(void)
{
    return static_data.class_data;
}

void ObjectiveClass::RepairFeature(int f)
{
    int cur;

    cur = GetFeatureStatus(f);

    if (cur == VIS_DAMAGED or cur == VIS_DESTROYED)
    {
        SetFeatureStatus(f, VIS_REPAIRED);

        if (Falcon4ClassTable[GetFeatureID(f)].vuClassData.classInfo_[VU_TYPE] == TYPE_RUNWAY) // (IS_RUNWAY)
            CleanupLinkedPersistantObjects(this, f, MapVisId(VIS_RWYPATCH), 1);
    }

    // KCK: Used to go from destroyed to damaged. This seems a little weird now that I think about it,
    // except for the case of runways...
    // if (cur == VIS_DESTROYED)
    // {
    // SetFeatureStatus(f,VIS_DAMAGED);
    // if (Falcon4ClassTable[GetFeatureID(f)].vuClassData.classInfo_[VU_TYPE] == TYPE_RUNWAY) // (IS_RUNWAY)
    // CleanupLinkedPersistantObjects (this, f, VIS_RWYPATCH, 2);
    // }
}

void ObjectiveClass::RecalculateParent(void)
{
    Objective n, s = NULL, bp = NULL;
    Int32 i, j, d, bd = 9999;
    Team who, own;
    GridIndex x, y, X, Y;
    // POData pod=NULL;
    // SOData sod=NULL;

    if ( not this)
        return;

    if (IsPrimary())
    {
        // Primary Objective, no parent
        SetObjectiveParent(FalconNullId);
    }
    else if (IsSecondary())
    {
        // Secondary Objective. Find closest Primary (Modify distances by relations and scores)
        GetLocation(&x, &y);
        own = GetTeam();
        {
            VuListIterator myit(POList);
            n = GetFirstObjective(&myit);

            while (n not_eq NULL)
            {
                n->GetLocation(&X, &Y);
                who = n->GetTeam();
                d = FloatToInt32(Distance(X, Y, x, y));

                if (GetTTRelations(who, own) == Allied)
                    d /= 2;
                else if (GetTTRelations(who, own) == War)
                    d *= 2;
                else
                    d = 9999;

                d = FloatToInt32((float)d / (n->GetObjectivePriority() / 100.0F));

                if (d < bd)
                {
                    s = n;
                    bd = d;
                }

                n = GetNextObjective(&myit);
            }
        }

        if (s)
        {
            SetObjectiveParent(s->Id());
            // pod = GetPOData(s);
            // if (pod)
            // pod->children++;
        }
        else
        {
            SetObjectiveParent(FalconNullId);
        }

        // Set frontline flags for us and our PO, if any.
        // if (IsNearfront())
        // {
        // sod = GetSOData(this);
        // if (sod)
        // sod->flags or_eq GTMOBJ_FRONTLINE;
        // if (pod)
        // pod->flags or_eq GTMOBJ_FRONTLINE;
        // }
    }
    else
    {
        // Everything else. Find best Secondary for parent
        i = 0;

        while (i < static_data.links)
        {
            n = GetNeighbor(i);

            if (n and n->IsSecondary())
            {
                SetObjectiveParent(n->Id());
                // KCK WARNING: Am I using this?
                // sod = GetSOData(n);
                // if (sod)
                // sod->children++;
                return;
            }

            i++;
        }

        i = 0;
        bd = 0;

        while (i < static_data.links)
        {
            n = GetNeighbor(i);
            j = 0;

            while (n and j < n->static_data.links)
            {
                s = n->GetNeighbor(j);

                if (s and s->IsSecondary() and s->GetObjectivePriority() > bd)
                {
                    bp = s;
                    bd = s->GetObjectivePriority();
                }

                j++;
            }

            i++;
        }

        if (bp)
        {
            SetObjectiveParent(bp->Id());
            // KCK WARNING: Am I using this?
            // sod = GetSOData(bp);
            // if (sod)
            // sod->children++;
        }
        else
            SetObjectiveParent(FalconNullId);
    }
}


// ===================================
// Global functions on Objective Class
// ===================================

// ===================================
// Functions on Objective Lists
// ===================================

Objective NewObjective(void)
{
    Objective   o;
    int cid;

    cid = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, TYPE_CITY, 1, 0, 0, 0, 0);

    if ( not cid)
    {
        return NULL;
    }

    cid += VU_LAST_ENTITY_TYPE;

    //VuEnterCriticalSection();
    //lastVolatileId = vuAssignmentId;
    //vuAssignmentId = lastObjectiveId;
    //vuLowWrapNumber = FIRST_OBJECTIVE_VU_ID_NUMBER;
    //vuHighWrapNumber = LAST_OBJECTIVE_VU_ID_NUMBER;

    o = new ObjectiveClass(cid);
    // these will be read from the other side as well
    o->SetSendCreate(VuEntity::VU_SC_DONT_SEND);
    VU_ERRCODE ret = vuDatabase->/*Silent*/Insert(
                         o/*, lastObjectiveId+1, FIRST_OBJECTIVE_VU_ID_NUMBER, LAST_OBJECTIVE_VU_ID_NUMBER*/
                     );

    if (ret not_eq VU_SUCCESS)
    {
        delete o;
        o = NULL;
    }


    //lastObjectiveId = vuAssignmentId;
    //vuAssignmentId = lastVolatileId;
    //vuLowWrapNumber = FIRST_VOLATILE_VU_ID_NUMBER;
    //vuHighWrapNumber = LAST_VOLATILE_VU_ID_NUMBER;
    //VuExitCriticalSection();

    return o;
}

Objective NewObjective(short tid, VU_BYTE **stream, long *rem)
{
    Objective   o;
    int i;

    if (tid == 0)
    {
        return NULL;
    }

    CampEnterCriticalSection();
    o = new ObjectiveClass(stream, rem);

    if (RepairObjective)  // Activated by command line parameter '-repair',
    {
        for (i = 0; i < o->GetTotalFeatures(); i++)
        {
            if (o->GetFeatureID(i))
            {
                o->SetFeatureStatus(i, VIS_NORMAL);
            }
            else
            {
                o->SetFeatureStatus(i, VIS_DESTROYED);
            }
        }
    }
    else if (DestroyObjective)//activated by commandline parameter '-armageddon'
    {
        for (i = 0; i < o->GetTotalFeatures(); i++)
        {
            o->SetFeatureStatus(i, VIS_DESTROYED);
        }
    }

    CampLeaveCriticalSection();

    // these will be read from the other side as well
    o->SetSendCreate(VuEntity::VU_SC_DONT_SEND);
    vuDatabase->/*Silent*/Insert(o);
    return o;
}

int LoadBaseObjectives(char* scenario)
{
    int old_version;
    Objective   o;
    short       num, i, type;
    long size, newsize;
    uchar *buffer, *bufptr;
    uchar /* *data,*/*data_ptr;

    old_version = gCampDataVersion;

    CampaignData cd = ReadCampFile(scenario, "obj");

    if (cd.dataSize == -1)
    {
        gCampDataVersion = old_version;
        return 0;
    }

    gCampDataVersion = ReadVersionNumber(scenario);

    // Read Number of Objectives..

    data_ptr = (uchar*)cd.data;
    long rem = cd.dataSize;

    memcpychk(&num, &data_ptr, sizeof(short), &rem);
    memcpychk(&size, &data_ptr, sizeof(long), &rem);
    memcpychk(&newsize, &data_ptr, sizeof(long), &rem);

    long tSize = size + MAX_POSSIBLE_OVERWRITE;
    buffer = new uchar[tSize];

    if (LZSS_Expand(data_ptr, rem, buffer, size) < 0)
    {
        // char err[200];
        // sprintf(err, "%s %d: error expanding buffer", __FILE__, __LINE__);
        // throw std::InvalidBufferException(err);
    }

    //now we have the uncompressed buffer, of size tSize
    bufptr = buffer;
    rem = tSize;

    for (i = 0; i < num; i++)
    {
        memcpychk(&type, &bufptr, sizeof(short), &rem);
        o = NewObjective(type, &bufptr, &rem);

        if (o == NULL)
        {
            fprintf(stderr, "%s %d: error creating object from stream\n", __FILE__, __LINE__);
        }
        else
        {
            o->SetAggregate(1);
        }
    }

    delete [] buffer;
    delete cd.data;

    gCampDataVersion = old_version;

    return 1;
}

int LoadObjectiveDeltas(char* savefile)
{
    long csize;
    uchar /* *data,*/ *data_ptr;

    if (strcmp(savefile, TheCampaign.Scenario) == 0)
    {
        // KCK Temporary: Reset dirty flags and return;
        if ( not AllObjList)
        {
            return 1; // InstantAction/DogFight
        }

        Objective o;
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o)
        {
            o->SetDelta(0);
            o = GetNextObjective(&myit);
        }

        return 1;
    }

    CampaignData cd = ReadCampFile(savefile, "obd");

    if (cd.dataSize == -1)
    {
        return 0;
    }

    data_ptr = (uchar*)cd.data;
    long rem = cd.dataSize;

    memcpychk(&csize, &data_ptr, sizeof(long), &rem);

    DecodeObjectiveDeltas(&data_ptr, &rem, NULL);
    delete cd.data;
    return 1;
}

void SaveBaseObjectives(char* scenario)
{
    FILE *fp;
    short num = 0, type;
    long            size = 0, newsize;
    Objective o;
    uchar           *buffer, *cbuffer, *bufptr;

    if ((fp = OpenCampFile(scenario, "obj", "wb")) == NULL)
    {
        return;
    }

    // Count # of objectives
    {
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o)
        {
            num++;
            size += o->SaveSize() + sizeof(short);
            o = GetNextObjective(&myit);
        }
    }

    // Save Number of Objectives and sizes
    fwrite(&num, sizeof(short), 1, fp);
    fwrite(&size, sizeof(long), 1, fp);

    buffer = new uchar[size];
    cbuffer = new uchar[size + MAX_POSSIBLE_OVERWRITE];
    bufptr = buffer;

    // Now save
    {
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o)
        {
            type = o->Type();
            memcpy(bufptr, &type, sizeof(short));
            bufptr += sizeof(short);
            o->Save(&bufptr);
            // Clear dirty flag
            o->SetDelta(0);
            o = GetNextObjective(&myit);
        }
    }

    newsize = LZSS_Compress(buffer, cbuffer, size);
    fwrite(&newsize, sizeof(long), 1, fp);
    fwrite(cbuffer, newsize, 1, fp);
    CloseCampFile(fp);
    delete [] buffer;
    delete [] cbuffer;
}

void SaveObjectiveDeltas(char* savefile)
{
    long csize;
    uchar *cbuffer;
    FILE *fp;

    if ((fp = OpenCampFile(savefile, "obd", "wb")) == NULL)
        return;

    csize = EncodeObjectiveDeltas(&cbuffer, NULL);

    fwrite(&csize, sizeof(long), 1, fp);
    fwrite(cbuffer, csize, 1, fp);
    delete cbuffer;
    CloseCampFile(fp);
}

int BestRepairFeature(Objective o, int *hours)
{
    int score, best = 0;
    int s, v, t, bt = 9999, f, bf = -1;

    bool assignedEng = false;

    GridIndex ux = 0, uy = 0;
    GridIndex ox = 0, oy = 0;

    VuListIterator uit(AllUnitList);
    Unit u;

    // RV - Biker - Where is the object
    o->GetLocation(&ox, &oy);

    // RV - Biker - Loop through ground units to find engineer battalion assigned for repair
    u = GetFirstUnit(&uit);

    while (u)
    {
        if (u->IsBrigade() or u->GetDomain() not_eq DOMAIN_LAND or u->GetTeam() not_eq o->GetTeam())
        {
            u = GetNextUnit(&uit);
            continue;
        }

        u->GetLocation(&ux, &uy);
        float dx = float(ux - ox);
        float dy = float(uy - oy);

        float dist = sqrt(dx * dx + dy * dy);

        // RV - Biker - Check for engineer type maybe we need some more check
        if (u->GetSType() == STYPE_UNIT_ENGINEER or u->GetSType() == STYPE_WHEELED_ENGINEER)
        {
            if (dist < 1.0f)
                assignedEng = TRUE;
            else
                assignedEng = FALSE;
        }

        u = GetNextUnit(&uit);
    }

    // Find 'quickest fix'. That is, the feature which will give us the most operational
    // percentage in the shortest time.
    for (f = 0; f < o->GetTotalFeatures(); f++)
    {
        s = o->GetFeatureStatus(f);

        if (s not_eq VIS_NORMAL and s not_eq VIS_REPAIRED)
        {
            v = o->GetFeatureValue(f);

            // RV - Biker - If we have a engineer battalion assigned repair time is one fourth
            if (assignedEng)
                t = int(o->GetFeatureRepairTime(f) / 4);
            else
                t = o->GetFeatureRepairTime(f);

            score = (v * 100) / (t + 1);

            if (score > best)
            {
                best = score;
                bf = f;
                bt = t;
            }
        }

        // if (s not_eq VIS_DESTROYED and not o->GetFeatureID(f))
        // o->SetFeatureStatus(f,VIS_DESTROYED);
    }

    // 2001-03-12 MODIFIED BY S.G. SO IT DOESN'T WAIT ONE MORE HOUR BEFORE FINISHING THE REPAIR
    // 2001-03-13 REINSTATED BECAUSE THE DATA FILE ISN'T ADJUSTED FOR IT YET,
    if (bt < *hours)
        // if (bt <= *hours)
    {
        *hours -= bt;
        return bf;
    }

    return -1;
}

int BestTargetFeature(Objective o, uchar targeted[])
{
    int score, best = 0;
    int s, f, bf = 0;

    // Find 'quickest fix'. That is, the feature which will give us the most operational
    // percentage in the shortest time.
    for (f = 0; f < o->GetTotalFeatures(); f++)
    {
        s = o->GetFeatureStatus(f);

        if (s not_eq VIS_DESTROYED and targeted and not targeted[f])
        {
            score = o->GetFeatureValue(f);

            if (score > best)
            {
                best = score;
                bf = f;
            }
        }
    }

    return bf;
}

// This attempts to repair objectives owned by this machine, based on the Team's repair status
void RepairObjectives(void)
{
    Objective      o;
    VuListIterator myit(AllObjList);
    o = GetFirstObjective(&myit);

    while (o not_eq NULL)
    {
        o->Repair();
        o = GetNextObjective(&myit);
    }
}

// This will add all child objectives to the passed list.
void AddChildObjectives(Objective o, Objective p, F4PFList list, int maxdist, int level, int flags)
{
    Objective n;
    int i;

    o->SetObjectiveScore((short)level);

    if ( not CampSearch[o->GetCampID()])
        list->ForcedInsert(o);

    CampSearch[o->GetCampID()] = 1;

    if ( not maxdist)
        return;

    for (i = 0; i < o->static_data.links; i++)
    {
        n = o->GetNeighbor(i);

        if ( not n or (CampSearch[n->GetCampID()] and n->GetObjectiveScore() <= level + 1))
            continue;

        if (flags bitand FIND_THISOBJONLY and n->GetObjectiveParentID() not_eq p->Id())
            continue;

        if (flags bitand FIND_STANDARDONLY and n->IsSecondary())
            continue;

        if (flags bitand FIND_FINDFRIENDLY and not GetRoE(o->GetTeam(), n->GetTeam(), ROE_GROUND_MOVE))
            continue;

        AddChildObjectives(n, p, list, maxdist - 1, level + 1, flags);
    }
}

F4PFList GetChildObjectives(Objective o, int maxdist, int flags)
{
    F4PFList list;

    if ( not o)
        return NULL;

    memset(CampSearch, 0, MAX_CAMP_ENTITIES);
    list = new FalconPrivateList(&AllObjFilter);

    if ( not list)
        return NULL;

    list->Register();
    AddChildObjectives(o, o, list, maxdist, 0, flags);
    return list;
}

Objective GetFirstObjective(F4LIt l)
{
    VuEntity* e;

    e = l->GetFirst();

    while (e)
    {
        if (GetEntityClass(e) == CLASS_OBJECTIVE)
            return (Objective)e;

        e = l->GetNext();
    }

    return NULL;
}

Objective GetNextObjective(F4LIt l)
{
    VuEntity* e;

    e = l->GetNext();

    while (e)
    {
        if (GetEntityClass(e) == CLASS_OBJECTIVE)
            return (Objective)e;

        e = l->GetNext();
    }

    return NULL;
}

Objective GetFirstObjective(VuGridIterator* l)
{
    VuEntity* e;

    e = l->GetFirst();

    while (e)
    {
        if (GetEntityClass(e) == CLASS_OBJECTIVE)
            return (Objective)e;

        e = l->GetNext();
    }

    return NULL;
}

Objective GetNextObjective(VuGridIterator* l)
{
    VuEntity* e;

    e = l->GetNext();

    while (e)
    {
        if (GetEntityClass(e) == CLASS_OBJECTIVE)
            return (Objective)e;

        e = l->GetNext();
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CaptureObjective(Objective co, Control who, Unit u)
{
    FalconCampEventMessage *newEvent;
    VU_ID vuid;
    Team newown;
    Unit unit;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator fit(RealUnitProxList, co->YPos(), co->XPos(), 5000.0F);
#else
    VuGridIterator fit(RealUnitProxList, co->XPos(), co->YPos(), 5000.0F);
#endif

    if (u)
    {
        vuid = u->Id();
    }
    else
    {
        vuid = co->Id();
    }

    newEvent = new FalconCampEventMessage(vuid, FalconLocalGame);

    if ( not GetRoE(GetTeam(who), GetTeam(co->GetObjectiveOldown()), ROE_GROUND_CAPTURE))
    {
        newown = ::GetTeam(co->GetObjectiveOldown());
        newEvent->dataBlock.data.formatId = 1831;
    }
    else
    {
        newown = ::GetTeam(who);
        newEvent->dataBlock.data.formatId = 1830;
    }

    co->SetDelta(1);
    co->SendObjMessage(vuid, FalconObjectiveMessage::objCaptured, newown, 0, 0);
    newEvent->dataBlock.team = GetTeam(who);
    newEvent->dataBlock.eventType = FalconCampEventMessage::objectiveCaptured;
    co->GetLocation(&newEvent->dataBlock.data.xLoc, &newEvent->dataBlock.data.yLoc);
    newEvent->dataBlock.data.vuIds[0] = co->Id();
    newEvent->dataBlock.data.owners[0] = who;
    newEvent->dataBlock.data.owners[1] = newown;
    SendCampUIMessage(newEvent);

    // sfr: we should check if this is an airbase.
    if ((co->GetType() == TYPE_AIRBASE) or (co->GetType() == TYPE_AIRSTRIP))
    {
        // if it is, we should remove all flights belonging to it which hav not taken off
        // Now remove any enemy flights which are based here and have not taken off yet
        for (unit = (Unit) fit.GetFirst(); (unit not_eq NULL); unit = (Unit) fit.GetNext())
        {
            FlightClass *f;

            if ((unit == NULL) or ( not unit->IsFlight()))
            {
                continue;
            }

            f = (FlightClass*)unit;

            if (
                (GetRoE(newown, unit->GetTeam(), ROE_GROUND_CAPTURE) == ROE_ALLOWED) and 
                (f->GetUnitAirbase() == co) and 
                ((f->GetCurrentUnitWP() == NULL) or (f->GetCurrentUnitWP()->GetWPAction() == WP_TAKEOFF)) and 
                (f->IsAggregate())
            )
            {
                //sfr: placed CancelFlight back and agged check
                CancelFlight((Flight)unit);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int EncodeObjectiveDeltas(VU_BYTE **stream, FalconSessionEntity *owner)
{
    long            size = 0, newsize;
    short count = 0;
    Objective o;
    CampEntity ent;
    VU_BYTE *buf, *sptr, *bufhead;
    VU_ID vuid, ownerid;

    if (owner)
    {
        ownerid = owner->Id();
    }

    // Count size of dirty objectives changable data
    {
        // Use AllCamp iterator - just in case we don't have an ObjList (Dogfight)
        VuListIterator myit(AllCampList);
        ent = (CampEntity) myit.GetFirst();

        while (ent)
        {
            if (
                (ent->EntityType())->classInfo_[VU_CLASS] == CLASS_OBJECTIVE and 
                ( not owner or ent->OwnerId() == ownerid)
            )
            {
                o = (Objective)ent;

                if (o->HasDelta())
                {
                    size += sizeof(VU_ID) + o->SaveSize(1);
                    count++;
                }
            }

            ent = (CampEntity) myit.GetNext();
        }
    }

    buf = new VU_BYTE[size + 1];
    bufhead = buf;

    // Encode the data
    {
        VuListIterator myit(AllObjList);
        ent = (CampEntity) myit.GetFirst();

        while (ent)
        {
            if (
                (ent->EntityType())->classInfo_[VU_CLASS] == CLASS_OBJECTIVE and 
                ( not owner or ent->OwnerId() == ownerid)
            )
            {
                o = (Objective)ent;

                if (o->HasDelta())
                {
                    vuid = o->Id();
                    memcpy(buf, &vuid, sizeof(VU_ID));
                    buf += sizeof(VU_ID);
                    o->Save(&buf, 1);
                }
            }

            ent = (CampEntity) myit.GetNext();
        }
    }

    // Compress it and return
    *stream = new VU_BYTE[size + sizeof(short) + sizeof(long) + MAX_POSSIBLE_OVERWRITE];
    sptr = *stream;
    memcpy(sptr, &count, sizeof(short));
    sptr += sizeof(short);
    memcpy(sptr, &size, sizeof(long));
    sptr += sizeof(long);
    buf = bufhead;
    newsize = LZSS_Compress(buf, sptr, size);
    delete bufhead;

    return newsize + sizeof(short) + sizeof(long);
}

//int DecodeObjectiveDeltas(VU_BYTE **stream, FalconSessionEntity *owner)
int DecodeObjectiveDeltas(VU_BYTE **stream, long *rem, FalconSessionEntity *)
{
    long            size;
    short count;
    VU_ID vuid;
    Objective o;
    VU_BYTE *buf, *bufhead;

    memcpychk(&count, stream, sizeof(short), rem);
    memcpychk(&size, stream, sizeof(long), rem);

    buf = new VU_BYTE[size];
    bufhead = buf;

    if (LZSS_Expand(*stream, rem[0], buf, size) < 0)
    {
        // char err[200];
        // sprintf(err, "%s %d: error expanding data", __FILE__, __LINE__);
        // throw std::InvalidBufferException(err);
    }

    //our new buffer is size size
    rem[0] = size;

    while (count)
    {

        memcpychk(&vuid, &buf, sizeof(VU_ID), rem);
        o = FindObjective(vuid);

        ShiAssert(o);

        if (o)
        {
            o->UpdateFromData(&buf, rem);
            count --;
        }
        else
        {
            count = 0;
        }
    }

    delete bufhead;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ObjectiveClass::MakeObjectiveDirty(Dirty_Objective bits, Dirtyness score)
{
    if ( not IsLocal() or (VuState() not_eq VU_MEM_ACTIVE))
    {
        return;
    }

    if ( not IsAggregate() and (score not_eq SEND_RELIABLEANDOOB))
    {
        score = static_cast<Dirtyness>(score << 4);
    }

    dirty_objective or_eq bits;
    MakeDirty(DIRTY_OBJECTIVE, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ObjectiveClass::WriteDirty(unsigned char **stream)
{
    short
    size;

    unsigned char
    *start,
    *stp,
    *ptr;

    start = *stream;
    ptr = start;

    // MonoPrint ("OB %08x\n", dirty_objective);

    // Encode it up

    *ptr = (unsigned char) dirty_objective;
    ptr += sizeof(unsigned char);

    if (dirty_objective bitand DIRTY_STATUS)
    {
        *(uchar*)ptr = obj_data.status;
        ptr += sizeof(uchar);

        size = (short)(((static_data.class_data->Features * 2) + 7) / 8);

        stp = obj_data.fstatus;

        while (size)
        {
            *(unsigned char*)ptr = *stp;
            size --;
            ptr += sizeof(uchar);
            stp += sizeof(uchar);
        }
    }

    dirty_objective = 0;

    *stream = ptr;

    //MonoPrint ("(%d)", *stream - start);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//sfr: changed this function, check header file for original proto
void ObjectiveClass::ReadDirty(VU_BYTE **stream, long *rem)
{

    unsigned char *stp, bits;

    //get bitfield
    memcpychk(&bits, stream, sizeof(VU_BYTE), rem);

    if (bits bitand DIRTY_STATUS)
    {
        memcpychk(&obj_data.status, stream, sizeof(unsigned char), rem);

        int size = ((static_data.class_data->Features * 2) + 7) / 8;
        stp = obj_data.fstatus;

        // for (num = 0; num < size; num ++){
        memcpychk(stp, stream, sizeof(unsigned char)*size, rem);
        // stp += sizeof (unsigned char);
        // }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

