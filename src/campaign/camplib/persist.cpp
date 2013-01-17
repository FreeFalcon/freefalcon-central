#include "graphics/include/drawbsp.h"
#include "stdhdr.h"
#include "persist.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "Graphics/Include/drawGrnd.h"
#include "MsgInc/AddSFXMessage.h"
#include "SFX.h"
#include "Falcmesg.h"
#include "CampLib.h"
#include "falcsess.h"
#include "Objectiv.h"
#include "PtData.h"
#include "ClassTbl.h"
#include "InvalidBufferException.h"

// =============================
// Externals
// =============================

extern short SFXType;
extern int gCampDataVersion;
extern FILE* OpenCampFile (char *filename, char *ext, char *mode);
extern void CloseCampFile (FILE *fp);
extern float OffsetToMiddle;

// =============================
// Globals and stuff
// =============================

#ifdef USE_SH_POOLS
MEM_POOL	SimPersistantClass::pool;
#endif

#ifdef DEBUG
int Persistant_Hulks = 0;
int Persistant_Craters = 0;
int Persistant_Runway_Craters = 0;
#endif

SimPersistantClass*	PersistantObjects;
int persistantListTail = 0;

// =============================
// Base class functions
// =============================

SimPersistantClass::SimPersistantClass(void)
{
	x = y = 0;
	visType = 0;
	drawPointer = NULL;
	flags = 0;
}

SimPersistantClass::~SimPersistantClass(void)
{
	ShiAssert( !drawPointer );
//	if (drawPointer)
//		OTWDriver.RemoveObject(drawPointer, TRUE);
//	drawPointer = NULL;
}
 
// function interface
void SimPersistantClass::Load(FILE* filePtr)
{
	fread (&x, sizeof (float), 1, filePtr);
	fread (&y, sizeof (float), 1, filePtr);
	fread (&unionData, sizeof (PackedVUIDClass), 1, filePtr);
	fread (&visType, sizeof (short), 1, filePtr);
	fread (&flags, sizeof (short), 1, filePtr);
	drawPointer = NULL;
}

void SimPersistantClass::Load(VU_BYTE** stream)
{
	memcpy (&x, *stream, sizeof (float));						*stream += sizeof(float);
	memcpy (&y, *stream, sizeof (float));						*stream += sizeof(float);
	memcpy (&unionData, *stream, sizeof (PackedVUIDClass));		*stream += sizeof(PackedVUIDClass);
	memcpy (&visType, *stream, sizeof (short));					*stream += sizeof(short);
	memcpy (&flags, *stream, sizeof (short));					*stream += sizeof(short);
	drawPointer = NULL;
}

int SimPersistantClass::SaveSize()
{
	return sizeof (float)
		+ sizeof (float)
		+ sizeof (PackedVUIDClass)
		+ sizeof (short)
		+ sizeof (short);
}

int SimPersistantClass::Save(VU_BYTE **stream)
{
	memcpy (*stream, &x, sizeof (float));							*stream += sizeof(float);
	memcpy (*stream, &y, sizeof (float));							*stream += sizeof(float);
	memcpy (*stream, &unionData, sizeof (PackedVUIDClass));			*stream += sizeof(PackedVUIDClass);
	memcpy (*stream, &visType, sizeof (short));						*stream += sizeof(short);
	memcpy (*stream, &flags, sizeof (short));						*stream += sizeof(short);
	return SaveSize();
}

int SimPersistantClass::Save(FILE *filePtr)
{
	fwrite (&x, sizeof (float), 1, filePtr);
	fwrite (&y, sizeof (float), 1, filePtr);
	fwrite (&unionData, sizeof (PackedVUIDClass), 1, filePtr);
	fwrite (&visType, sizeof (short), 1, filePtr);
	fwrite (&flags, sizeof (short), 1, filePtr);
	return SaveSize();
}

void SimPersistantClass::Init (int visualType, float X, float Y)
{
	x = X;
	y = Y;
	visType = (short)visualType;
}

// Makes this drawable
void SimPersistantClass::Deaggregate()
{
	if (drawPointer)
		return;

	Tpoint    simView;
	simView.x     = x;
	simView.y     = y;
	simView.z     = 0;
	drawPointer = new DrawableGroundVehicle(visType, &simView, 0.0F, OTWDriver.Scale());
    OTWDriver.InsertObject (drawPointer);
}

// Cleans up the drawable object
void SimPersistantClass::Reaggregate (void)
{
	if (drawPointer)
		OTWDriver.RemoveObject(drawPointer, TRUE);
	drawPointer = NULL;
}

void SimPersistantClass::Cleanup (void)
{
	Reaggregate();
	flags = 0; 
}

CampaignTime SimPersistantClass::RemovalTime (void)
{
	if (IsLinked())
		return 0xffffffff;
	else
		return unionData.removeTime;
}

FalconEntity *SimPersistantClass::GetCampObject(void)
{
	if (IsTimed())
		return NULL;
	else
	{
		VU_ID	vuid;
		vuid.creator_ = unionData.campObject.creator_;
		vuid.creator_ = unionData.campObject.num_;
		return (FalconEntity*) vuDatabase->Find(vuid);
	}
}

int SimPersistantClass::GetCampIndex(void)
{
	if (IsTimed())
		return 0;
	else
		return unionData.campObject.index_;
}

// =============================
// Global functions
// =============================


void InitPersistantDatabase (void)
{
#ifdef USE_SH_POOLS
	SimPersistantClass::InitializeStorage();
#endif
	PersistantObjects = new SimPersistantClass[MAX_PERSISTANT_OBJECTS];
}

void CleanupPersistantDatabase (void)
{
	delete[] PersistantObjects;
#ifdef USE_SH_POOLS
	SimPersistantClass::ReleaseStorage();
#endif
}

// This is the correct way to add a timed persistant object
void AddToTimedPersistantList (int vistype, CampaignTime removalTime, float x, float y)
{
	FalconAddSFXMessage*	msg = new FalconAddSFXMessage(FalconNullId, FalconLocalGame);
	msg->dataBlock.type = SFX_TIMED_PERSISTANT;
	msg->dataBlock.visType = (short)vistype;
	msg->dataBlock.xLoc = x;
	msg->dataBlock.yLoc = y;
	msg->dataBlock.zLoc = 0.0F;
	msg->dataBlock.time = removalTime;
	FalconSendMessage(msg,FALSE);
}

// This is the correct way to add a linked persistant object
void AddToLinkedPersistantList (int vistype, FalconEntity *campObj, int campIdx, float x, float y)
{
	ShiAssert (campObj);

	FalconAddSFXMessage*	msg = new FalconAddSFXMessage(campObj->Id(), FalconLocalGame);
	msg->dataBlock.type = SFX_LINKED_PERSISTANT;
	msg->dataBlock.visType = (short)vistype;
	msg->dataBlock.xLoc = x;
	msg->dataBlock.yLoc = y;
	msg->dataBlock.zLoc = 0.0F;
	msg->dataBlock.time = campIdx;
	FalconSendMessage(msg,FALSE);
}

void NewTimedPersistantObject (int vistype, CampaignTime removalTime, float x, float y)
{
	int		i,ds,slot=-1;

	i = persistantListTail + 1;
	if (i >= MAX_PERSISTANT_OBJECTS)
		i = 0;
	ds = i;

	ShiAssert(persistantListTail < MAX_PERSISTANT_OBJECTS);

	while (i != persistantListTail && slot < 0)
		{
		if (!PersistantObjects[i].InUse())
			slot = i;
		i++;
		if (i >= MAX_PERSISTANT_OBJECTS)
			i = 0;
		}
	if (slot < 0)
		slot = ds;

	persistantListTail = slot;
	PersistantObjects[slot].Init (vistype, x, y);
	PersistantObjects[slot].unionData.removeTime = removalTime;
	PersistantObjects[slot].flags = SPLF_IS_TIMED | SPLF_IN_USE;
}

void NewLinkedPersistantObject (int vistype, VU_ID campObjID, int campIdx, float x, float y)
{
	int		i,ds,slot=-1;

	i = persistantListTail + 1;
	if (i >= MAX_PERSISTANT_OBJECTS)
		i = 0;
	ds = i;

	ShiAssert(persistantListTail < MAX_PERSISTANT_OBJECTS);

	while (i != persistantListTail && slot < 0)
		{
		if (!PersistantObjects[i].InUse())
			slot = i;
		i++;
		if (i >= MAX_PERSISTANT_OBJECTS)
			i = 0;
		}
	if (slot < 0)
		slot = ds;

	persistantListTail = slot;
	PersistantObjects[slot].Init (vistype, x, y);
	PersistantObjects[slot].unionData.campObject.creator_ = campObjID.creator_;
	PersistantObjects[slot].unionData.campObject.num_ = campObjID.num_;
	PersistantObjects[slot].unionData.campObject.index_ = (uchar)campIdx;
	PersistantObjects[slot].flags = SPLF_IS_LINKED | SPLF_IN_USE;
}

void SavePersistantList(char* scenario)
{
	int		i,count = 0;
	FILE*	fp;

	for (i=0; i<MAX_PERSISTANT_OBJECTS; i++)
	{
		if (PersistantObjects[i].InUse())
			count++;
	}

	if ((fp = OpenCampFile (scenario, "pst", "wb")) == NULL)
		return;

	fwrite (&count, sizeof(count), 1, fp);

	// Save 'em
	for (i=0; i<MAX_PERSISTANT_OBJECTS; i++)
	{
		if (PersistantObjects[i].InUse())
			PersistantObjects[i].Save(fp);
	}

	CloseCampFile (fp);
}

void LoadPersistantList(char* scenario)
{
	int			i,count = 0;
	FILE*		fp;

	if (gCampDataVersion < 69)
		{
		// Don't even try and load earlier versions
		CleanupPersistantList();
		return;
		}

	if ((fp = OpenCampFile (scenario, "pst", "rb")) == NULL)
		return;
	
	CleanupPersistantList();

	fread (&count, sizeof(count), 1, fp);
#ifndef NDEBUG
	MonoPrint ("%d craters\n", count);
#endif

	// Load 'em
	for (i=0; i<count; i++)
		PersistantObjects[i].Load(fp);
	persistantListTail = i%MAX_PERSISTANT_OBJECTS;

	CloseCampFile (fp);
}

int EncodePersistantList(VU_BYTE** stream, int maxSize)
{
	int	i,count = 0,size;

	for (i=0; i<MAX_PERSISTANT_OBJECTS; i++)
		{
		if (PersistantObjects[i].InUse())
			count++;
		}
	size = PersistantObjects[0].SaveSize();
	if ((size * count) > static_cast<short>(maxSize - sizeof(short)))
		count = (maxSize - sizeof(short)) / size;

	memcpy (*stream, &count, sizeof(short));		*stream += sizeof(short);
	size = sizeof(short);

	// Save 'em
	for (i=0; i<MAX_PERSISTANT_OBJECTS && i<count; i++)
		{
		if (PersistantObjects[i].InUse())
			size += PersistantObjects[i].Save(stream);
		}
	return size;
}

void DecodePersistantList(VU_BYTE** stream, long *rem) {
	short	i,count = 0;

	CleanupPersistantList();

	memcpychk(&count, stream, sizeof(short), rem);			
#ifndef NDEBUG
	MonoPrint ("%d craters\n", count);
#endif

	if (count > MAX_PERSISTANT_OBJECTS){
		char err[200];
		sprintf(err, "%s %d: error decoding persistant, invalid count", __FILE__, __LINE__);
		throw InvalidBufferException(err);	
	}
	//ShiAssert (count < MAX_PERSISTANT_OBJECTS);

	// Load 'em
	for (i=0; i<count; i++){
		PersistantObjects[i].Load(stream);
	}
	persistantListTail = i%MAX_PERSISTANT_OBJECTS;
}

int SizePersistantList(int maxSize)
{
	int		i,count=0,size;

	for (i=0; i<MAX_PERSISTANT_OBJECTS; i++)
		{
		if (PersistantObjects[i].InUse())
			count++;
		}
	size = PersistantObjects[0].SaveSize();
	if ((size * count) > static_cast<short>(maxSize - sizeof(short)))
		count = (maxSize - sizeof(short))/size;
	size = count * size;
	return size + sizeof(short);
}

void CleanupPersistantList (void)
{
	for (int i=0; i<MAX_PERSISTANT_OBJECTS; i++)
		PersistantObjects[i].Cleanup();
	persistantListTail = 0;
}

void UpdatePersistantObjectsWakeState (float px, float py, float range, CampaignTime now)
{
	SimPersistantClass* persist;
	int		i;
	float	dsq,rsq = range*range;
//	float	lasty = py+range;

	for (i=0; i<MAX_PERSISTANT_OBJECTS; i++)
	{
		if (PersistantObjects[i].InUse())
		{
			persist = &PersistantObjects[i];
			dsq = (px-persist->x)*(px-persist->x) + (py-persist->y)*(py-persist->y);
			if (dsq < rsq)
			{
				if (!persist->drawPointer)
					persist->Deaggregate();
			}
			else if (persist->drawPointer && dsq > rsq*1.2F)	// Reaggregate 20% further than we deaggregate
				persist->Reaggregate();
			else if (persist->IsTimed() && now > persist->unionData.removeTime)
				persist->Cleanup();
		}
	}
}

void CleanupLinkedPersistantObjects (FalconEntity *campObject, int index, int newVis, int ratio)
{
	SimPersistantClass* persist;
	int					i,converted=0;
	VU_ID				vuid = campObject->Id();

	for (i=0; i<MAX_PERSISTANT_OBJECTS; i++)
	{
		if (PersistantObjects[i].IsLinked())
		{
			persist = &PersistantObjects[i];
			if (persist->unionData.campObject.num_ == vuid.num_ && 
				persist->unionData.campObject.creator_ == vuid.creator_ &&
				persist->unionData.campObject.index_ == index)
			{
				if (newVis)
				{
					// Change 1 in "ratio" drawable objects to the newVis type
					// IE: ratio = 1 -> Convert all objects
					// IE: ratio = 5 -> Convert 1 in 5 objects
					if (converted == 0)
					{
						// Change the drawable object
						persist->visType = (short)newVis;
					}
					converted++;
					if (converted == ratio)
						converted = 0;
				}
				else
					persist->Cleanup();
			}
		}
	}
}

// ==================================================
// Here are some persistant object creation routines
// which are used by the campaign to add LOCAL
// persistant objects as a result of a damage message
// ==================================================

void AddRunwayCraters (Objective o, int f, int craters)
{
	// Add a few linked craters
	// NOTE: These need to be deterministically generated
	int		i,tp,rp;
	float	x1,y1,x,y,xd,yd,r;
	int		rwindex,runway = 0;
	ObjClassDataType	*oc;
	
	// Find the runway header this feature belongs to
	oc = o->GetObjectiveClassData();
	rwindex = oc->PtDataIndex;
	while (rwindex && !runway)
	{
		if (PtHeaderDataTable[rwindex].type == RunwayPt)
		{
			for (i=0; i<MAX_FEAT_DEPEND && !runway; i++)
			{
				if (PtHeaderDataTable[rwindex].features[i] == f)
					runway = rwindex;
			}
		}
		rwindex = PtHeaderDataTable[rwindex].nextHeader;
	}
	// Check for valid runway (could be a runway number or something)
	if (!runway)
		return;

	rp = GetFirstPt(runway);
	tp = rp + 1;
	TranslatePointData(o,rp,&xd,&yd);
	TranslatePointData(o,tp,&x1,&y1);
	xd -= x1;
	yd -= y1;
	// Seed the random number generator
	srand(o->Id().num_);

	for (i=0; i<craters; i++)
	{
		// Randomly place craters along the runway.
		r = ((float)(rand()%1000)) / 1000.0F;
		x = x1 + r*xd + (((float)(rand()%50)) / 50.0F);
		y = y1 + r*yd + (((float)(rand()%50)) / 50.0F);
		NewLinkedPersistantObject(MapVisId(VIS_CRATER2), o->Id(), f, x, y);
#ifdef DEBUG
		Persistant_Runway_Craters++;
#endif
	}
}

void AddMissCraters (FalconEntity *e, int craters)
{
	// Add a few timed craters
	// NOTE: These need to be deterministically generated
	// Seed the random number generator
	srand(e->Id().num_);
	for (int i=0; i<craters; i++)
	{
		NewTimedPersistantObject(MapVisId(VIS_CRATER2), Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, e->XPos()+rand()%(int)(GRID_SIZE_FT)-OffsetToMiddle, e->YPos()+rand()%(int)(GRID_SIZE_FT)-OffsetToMiddle);
#ifdef DEBUG
		Persistant_Craters++;
#endif
	}
}

void AddHulk (FalconEntity *e, int hulkVisId)
{
	// Add a timed hulk
	// NOTE: This need to be deterministically generated
	// Seed the random number generator
	srand(e->Id().num_);
	NewTimedPersistantObject(hulkVisId, Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, e->XPos()+rand()%(int)(GRID_SIZE_FT)-OffsetToMiddle, e->YPos()+rand()%(int)(GRID_SIZE_FT)-OffsetToMiddle);
#ifdef DEBUG
	Persistant_Hulks++;
#endif
}

// ==================================================
// This should go in a different file..
// Do later
// ==================================================

#include "Simbase.h"

void UpdateNoCampaignParentObjectsWakeState (float px, float py, float range){
	// Traverse the list of asleep detached objects and wake those in range
	float			dsq,rsq = range*range;
	SimBaseClass	*object;
	VuListIterator	dit(SimDriver.ObjsWithNoCampaignParentList);
	object = (SimBaseClass*)dit.GetFirst();
	while (object){
		if (object->IsLocal()){
			// All local objects should be awake (we need to manage them)
			if (!object->IsAwake()){
				object->Wake();
			}
		}
		else {
			// Only wake remote objects which are within a reasonable range
			dsq = (px-object->XPos())*(px-object->XPos()) + (py-object->YPos())*(py-object->YPos());
			if (dsq < rsq){
				// KCK: Probably should remove objects from this list when they're dead -
				// But I wasn't sure how ACMI uses the unset dead functionality
				if (!object->IsAwake() && !object->IsDead())
					object->Wake();
			}
			else if (dsq > rsq*1.2F){
				if (object->IsAwake() && //me123 host needs to drive missiles and bombs
				(
					!vuLocalSessionEntity->Game()->IsLocal() ||
					vuLocalSessionEntity->Game()->IsLocal() && 
					!object->IsBomb() && 
					!object->IsMissile())
				){
					object->Sleep();
				}
			}
		}
		object = (SimBaseClass*)dit.GetNext();
	}
}

