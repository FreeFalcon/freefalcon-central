/** @file F4VU.cpp
* implementation of application specific part of VU
*/

#include <time.h>
#include "f4thread.h"
#include "falclib.h"
#include "classtbl.h"
#include "sim/include/simlib.h"
#include "sim/include/simvucb.h"
#include "vu2/src/vu_priv.h"
#include "sim/include/simbase.h"
#include "campbase.h"
#include "falcmesg.h"
#include "FalcSess.h"
#include "F4Comms.h"
#include "F4Thread.h"
#include "F4Vu.h"
#include "F4Version.h"

FILE* F4EventFile;

#define HOSTNAMEPACKET                  1
#define HOSTLISTREJECTPACKET       		2
#define PROPOSEHOSTNAMEPACKET      		3
#define NEWGROUPPACKET                  4
#define PROPOSEGROUPNAMEPACKET   		5
#define GROUPREJECTPACKET               6
#define JOINGROUPPACKET                 7
#define LEAVEGROUPPACKET                8
#define CLOSESESSIONPACKET       		9
#define CLOSEGROUPPACKET         		10
#define OPENGROUPPACKET                 11
#define DELETEGROUPPACKET               12

#define SESSIONUPDATEPACKET             1024

//#define F4_ENTITY_TABLE_SIZE			5000		// Size of vu's hash table
const int F4_ENTITY_TABLE_SIZE = 10529;			// Size of vu's hash table	 - this is a prime number // JB 010718

// =========================
// Some external functions
// =========================

class UnitClass;
class ObjectiveClass;
class CampManagerClass;
//sfr: added rem
extern UnitClass* NewUnit (short tid, VU_BYTE **stream, long *rem);
extern ObjectiveClass* NewObjective (short tid, VU_BYTE **stream, long *rem);
extern VuEntity* NewManager (short tid, VU_BYTE **stream, long *rem);
extern void TcpConnectCallback(ComAPIHandle ch, int ret);
extern int MajorVersion;
extern int MinorVersion;
extern int BuildNumber;

//extern VU_ID_NUMBER vuAssignmentId;
//extern VU_ID_NUMBER vuLowWrapNumber;
//extern VU_ID_NUMBER vuHighWrapNumber;

// =========================
// VU required globals
// =========================

ulong vuxVersion = 1;
SM_SCALAR vuxTicsPerSec = 1000.0F;
VU_TIME vuxGameTime = 0;
VU_TIME vuxTargetGameTime = 0;
VU_TIME vuxLastTargetGameTime = 0;
VU_TIME vuxDeadReconTime = 0;
VU_TIME vuxCurrentTime = 0;
VU_TIME lastTimingMessage = 0;
VU_TIME vuxTransmitTime = 0;
//ulong vuxLocalDomain = 1;	// range = 1-31
ulong vuxLocalDomain = 0xffffffff;	// range = 1-31 // JB 010718
VU_BYTE vuxLocalSession = 1;
#define EBS_BASE_NAME    "EBS"
char *vuxWorldName = 0;
VU_TIME vuxRealTime = 0;

VuSessionEntity* vuxCreateSession (void);


// =================================
// VU related globals for Falcon 4.0
// =================================

VuMainThread *gMainThread = 0;
Falcon4EntityClassType* Falcon4ClassTable;
F4CSECTIONHANDLE* vuCritical = NULL;
int NumEntities;
VU_ID FalconNullId;
//FalconAllFilterType		FalconAllFilter;
FalconNothingFilterType	FalconNothingFilter;

#define VU_VERSION_USED    3
#define VU_REVISION_USED   1
#define VU_PATCH_USED      0

#ifndef CLASSMKR

// ==================================
// Functions for VU's CS
// ==================================
extern char g_strWorldName[0x40];

void InitVU (void)
{
	char tmpStr[256];

	#ifdef USE_SH_POOLS
	gVuMsgMemPool = MemPoolInit( 0 );
	gVuFilterMemPool = MemPoolInit( 0 );

	VuLinkNode::InitializeStorage();
	VuRBNode::InitializeStorage();
	#endif

   // Make sure we're using the right VU
#if (VU_VERSION_USED  != VU_VERSION)
#error "Incorrect VU Version"
#endif

#if (VU_REVISION_USED != VU_REVISION)
#error "Incorrect VU Revision"
#endif

#if (VU_PATCH_USED    != VU_PATCH)
#error "Incorrect VU Patch"
#endif

#ifdef NDEBUG // Differentiate Debug & Release versions so they can't be seen by each other (PJW)
   sprintf (tmpStr, "R%5d.%2d.%02d.%s.%d_\0", BuildNumber, gLangIDNum, MinorVersion, "EBS", MajorVersion);
#else
   sprintf (tmpStr, "K%5d.%2d.%02d.%s.%d_\0", BuildNumber, gLangIDNum, MinorVersion, "EBS", MajorVersion);
#endif

	MonoPrint ("Version %s %s %s\n", tmpStr, __DATE__, __TIME__);

	// Change this to stop different versions taking to each other

	// OW FIXME
	// strcpy (tmpStr, "F527");
	//	strcpy(tmpStr, "F552");		//  according to REVISOR this will allow connections to 1.08 servers. we'll see
	//strcpy(tmpStr, "E109newmp"); //me123 we are not interested in 108 conections anymore since they'll ctd us
	strcpy(tmpStr, g_strWorldName);

	vuxWorldName = new char[strlen(tmpStr) + 1];
	strcpy (vuxWorldName, tmpStr);
#if VU_USE_ENUM_FOR_TYPES
	FalconMessageFilter falconFilter(FalconEvent::SimThread, true);
#else
	FalconMessageFilter falconFilter(FalconEvent::SimThread, VU_VU_MESSAGE_BITS);
#endif
	vuCritical = F4CreateCriticalSection("Vu");
	//VU_ID_NUMBER low = FIRST_VOLATILE_VU_ID_NUMBER;
	//VU_ID_NUMBER hi = LAST_VOLATILE_VU_ID_NUMBER;
	gMainThread = new VuMainThread(
		/*low, hi, */F4_ENTITY_TABLE_SIZE, &falconFilter, F4_EVENT_QUEUE_SIZE, vuxCreateSession
	);

	// Default VU namespace
	/*vuAssignmentId = FIRST_VOLATILE_VU_ID_NUMBER;
	vuLowWrapNumber = FIRST_VOLATILE_VU_ID_NUMBER;
	vuHighWrapNumber = LAST_VOLATILE_VU_ID_NUMBER;*/
}
 
void ExitVU (void)
{
   delete (gMainThread);
   delete [] vuxWorldName;
   gMainThread = NULL;
   F4DestroyCriticalSection(vuCritical);
   vuCritical = NULL;

	#ifdef USE_SH_POOLS
   VuLinkNode::ReleaseStorage();
   VuRBNode::ReleaseStorage();
   MemPoolFree( gVuMsgMemPool );
   MemPoolFree( gVuFilterMemPool );
	#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity* VuxCreateEntity(ushort type, ushort size, VU_BYTE *dataPtr)
{
	VuEntity *retval = 0;
	VuEntityType* classPtr = VuxType(type);
	F4Assert(classPtr!= NULL);

	//sfr: rem for compatibility reasons
	long rem = size;

	VU_BYTE **data = &dataPtr;

	switch (classPtr->classInfo_[VU_CLASS]) {
		case (CLASS_VEHICLE):{
			retval =  SimVUCreateVehicle (type, size, dataPtr);
			break;
		}
		case (TYPE_EJECT):{
			retval = SimVUCreateVehicle (type, size, dataPtr); 
			break;
		}
		case (CLASS_FEATURE):{
			ShiWarning ("We shouldn't be creating features this way");
			retval = NULL;			 
			break;
		}
		case (CLASS_UNIT): {
			retval = (VuEntity*) NewUnit(type, data, &rem);
			// This is a valid creation call only if this entity is still owned by
			// the owner of our game
			if (!FalconLocalGame || FalconLocalGame->OwnerId() != retval->OwnerId())
			{
				VuReferenceEntity(retval);
				VuDeReferenceEntity(retval);
				retval = NULL;
			}
			break;
		}
		case (CLASS_MANAGER): {
			retval = (VuEntity*) NewManager (type, data, &rem);
			// This is a valid creation call only if this entity is still owned by
			// the owner of our game
			if (!FalconLocalGame || FalconLocalGame->OwnerId() != retval->OwnerId())
			{
				VuReferenceEntity(retval);
				VuDeReferenceEntity(retval);
				retval = NULL;
			}
			break;
		}
		case (CLASS_OBJECTIVE): {
			retval = (VuEntity*) NewObjective (type, data, &rem);
			// This is a valid creation call only if this entity is still owned by
			// the owner of our game
			if (!FalconLocalGame || FalconLocalGame->OwnerId() != retval->OwnerId())
			{
				VuReferenceEntity(retval);
				VuDeReferenceEntity(retval);
				retval = NULL;
			}
			break;
		}
		case (CLASS_SESSION): {
			retval = (VuEntity*) new FalconSessionEntity(data, &rem);
			((VuSessionEntity*)retval)->SetKeepaliveTime (vuxRealTime);
			break;
		}
		case (CLASS_GROUP): {
			retval = (VuEntity*) new FalconGameEntity(data, &rem);			// FalconGroupEntity at some point..
			break;
		}
		case (CLASS_GAME): {
			retval = (VuEntity*) new FalconGameEntity(data, &rem);
			break;
		}
		default: {
			// This is not a class table entry so to speak, but it is a ground spot
			//retval = (VuEntity*) new GroundSpotEntity(type);
			retval = (VuEntity*) new SpotEntity(data, &rem); // JB 010718
		}
	}
	return retval;

	/*if (classPtr->classInfo_[VU_CLASS] == CLASS_VEHICLE)
	{
		retval = SimVUCreateVehicle (type, size, data);
	}
	else if (classPtr->classInfo_[VU_TYPE] == TYPE_EJECT)
	{
		retval = SimVUCreateVehicle (type, size, data);
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_FEATURE)
	{
		ShiWarning ("We shouldn't be creating features this way");
		retval = NULL;
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_UNIT)
	{
		retval = (VuEntity*) NewUnit (type, &data);
		// This is a valid creation call only if this entity is still owned by
		// the owner of our game
		if (!FalconLocalGame || FalconLocalGame->OwnerId() != retval->OwnerId())
		{
			VuReferenceEntity(retval);
			VuDeReferenceEntity(retval);
			retval = NULL;
		}
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_MANAGER)
	{
		retval = (VuEntity*) NewManager (type, data);
		// This is a valid creation call only if this entity is still owned by
		// the owner of our game
		if (!FalconLocalGame || FalconLocalGame->OwnerId() != retval->OwnerId())
		{
			VuReferenceEntity(retval);
			VuDeReferenceEntity(retval);
			retval = NULL;
		}
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_OBJECTIVE)
	{
		retval = (VuEntity*) NewObjective (type, &data);
		// This is a valid creation call only if this entity is still owned by
		// the owner of our game
		if (!FalconLocalGame || FalconLocalGame->OwnerId() != retval->OwnerId())
		{
			VuReferenceEntity(retval);
			VuDeReferenceEntity(retval);
			retval = NULL;
		}
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_SESSION)
	{
		retval = (VuEntity*) new FalconSessionEntity(&data);
		((VuSessionEntity*)retval)->SetKeepaliveTime (vuxRealTime);
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_GROUP)
	{
		retval = (VuEntity*) new FalconGameEntity(&data);			// FalconGroupEntity at some point..
	}
	else if (classPtr->classInfo_[VU_CLASS] == CLASS_GAME)
	{
		retval = (VuEntity*) new FalconGameEntity(&data);
	}
	else
	{
		// This is not a class table entry so to speak, but it is a ground spot
		//retval = (VuEntity*) new GroundSpotEntity(type);
		retval = (VuEntity*) new GroundSpotEntity(&data, &rem); // JB 010718
	}
	return retval;*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntityType* VuxType(ushort id)
{
VuEntityType *retval = 0;

    F4Assert(id >= VU_LAST_ENTITY_TYPE && id - VU_LAST_ENTITY_TYPE < NumEntities);
	if (id >= VU_LAST_ENTITY_TYPE && id - VU_LAST_ENTITY_TYPE < NumEntities)
		retval = (VuEntityType*)&(Falcon4ClassTable[id - VU_LAST_ENTITY_TYPE]);
	
	return retval;
}

void VuxRetireEntity (VuEntity* theEntity)
{
	F4Assert(theEntity);
	MonoPrint ("You dropped an entity, better find it!!!\n");
}

VuSessionEntity* vuxCreateSession (void)
{
	return (VuSessionEntity*) new FalconSessionEntity(vuxLocalDomain,"Falcon 4.0");
}

// ======================================
// Functions
// ======================================

void VuxLockMutex(VuMutex m){
	F4EnterCriticalSection(static_cast<F4CSECTIONHANDLE*>(m));
}

void VuxUnlockMutex(VuMutex m){
	F4LeaveCriticalSection(static_cast<F4CSECTIONHANDLE*>(m));
}

VuMutex VuxCreateMutex(const char *name){
	return static_cast<VuMutex>(F4CreateCriticalSection(name));
}

void VuxDestroyMutex(VuMutex m){
	F4DestroyCriticalSection(static_cast<F4CSECTIONHANDLE*>(m));
}


VuMutex idMutex = VuxCreateMutex("ids mutex");

VU_ID_NUMBER VuxGetId(){
	return GetIdFromNamespace(VolatileNS);
}

/*
void VuxGetIdAndWraps(const VuEntity *ce, VU_ID_NUMBER &id, VU_ID_NUMBER &low, VU_ID_NUMBER &hi){

	//VuScopeLock lock(idMutex);

	// sigh. I have to const cast to get the IsXXX functions
	// @todo make them const
	VuEntity *e = const_cast<VuEntity*>(ce);
	VU_ID_NUMBER *idp;
	ushort type = e->Type();
	if (
		(type > VU_PLAYER_POOL_GROUP_ENTITY_TYPE) ||
		(type < VU_SESSION_ENTITY_TYPE)
	){
		FalconEntity *fe = static_cast<FalconEntity*>(e);
		if (fe->IsPackage()){
			idp = &++lastPackageId;
			low = FIRST_LOW_VOLATILE_VU_ID_NUMBER_1;
			hi = LAST_LOW_VOLATILE_VU_ID_NUMBER_1;
			goto end;
		}
		else if (fe->IsFlight()){
			idp = &++lastFlightId;
			low = FIRST_LOW_VOLATILE_VU_ID_NUMBER_2;
			hi = LAST_LOW_VOLATILE_VU_ID_NUMBER_2;
			goto end;
		}
		else if (fe->IsSquadron()){
			idp = &++lastNonVolatileId;
			low = FIRST_NON_VOLATILE_VU_ID_NUMBER;
			hi = LAST_NON_VOLATILE_VU_ID_NUMBER;
			goto end;
		}
		else if (fe->IsBrigade() || fe->IsBattalion()){
			idp = &++lastNonVolatileId;
			low = FIRST_NON_VOLATILE_VU_ID_NUMBER;
			hi = LAST_NON_VOLATILE_VU_ID_NUMBER;
			goto end;
		}
		else if (fe->IsTaskForce()){
			idp = &++lastNonVolatileId;
			low = FIRST_NON_VOLATILE_VU_ID_NUMBER;
			hi = LAST_NON_VOLATILE_VU_ID_NUMBER;	
			goto end;
		}
		else if (fe->IsObjective()){
			idp = &++lastObjectiveId;
			low = FIRST_OBJECTIVE_VU_ID_NUMBER;
			hi = LAST_OBJECTIVE_VU_ID_NUMBER;
			goto end;
		}
	}
	// default values
	idp = &++lastVolatileId;
	low = FIRST_VOLATILE_VU_ID_NUMBER;
	hi = LAST_VOLATILE_VU_ID_NUMBER;

end:
	// cover wrap
	if ((*idp < low) || (*idp > hi)){
		*idp = low;
	}
	// return the id
	id = *idp;
}
*/



void VuEnterCriticalSection(void){ 
	F4EnterCriticalSection(vuCritical); 
}

void VuExitCriticalSection(void){ 
	F4LeaveCriticalSection(vuCritical);
}

bool VuHasCriticalSection(){
	return F4CheckHasCriticalSection(vuCritical) ? true : false;
}

#endif
