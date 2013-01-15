#ifndef _PERSISTANT_H
#define _PERSISTANT_H

#include "f4vu.h"
#include "geometry.h"
#include "Graphics\Include\drawBsp.h"
#include "CampLib.h"

class ObjectiveClass;
typedef ObjectiveClass* Objective;

// =============================
// Defines and Flags
// =============================

#define MAX_PERSISTANT_OBJECTS		5000				// Max number of persistant objects

#define SPLF_IS_LINKED				0x01				// Linked persistant object
#define SPLF_IS_TIMED				0x02				// Timed persistant object
#define SPLF_IN_USE					0x04				// This entry is being used

// =============================
// Packed VU_ID & index data
// =============================

class PackedVUIDClass
	{
	public:
		unsigned long	creator_;
		unsigned long	num_;
		unsigned char	index_;
	};

// =============================
// Timed/Linked Persistant Class
// =============================

// Base persistant object class
class SimPersistantClass
	{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap fixed size pool
		void *operator new(size_t size) { ShiAssert( size == sizeof(SimPersistantClass) ); return MemAllocFS(pool);	};
		void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
		static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SimPersistantClass), MAX_PERSISTANT_OBJECTS, 0 ); };
		static void ReleaseStorage()	{ MemPoolFree( pool ); };
		static MEM_POOL	pool;
#endif

	public:
		float				x,y;
		DrawableBSP*		drawPointer;
		union
			{
			CampaignTime	removeTime;
			PackedVUIDClass	campObject;
			}				unionData;
		short				visType;
		short				flags;

	public:
		// constructors/destructors
		SimPersistantClass(void);
		~SimPersistantClass(void);

		// serialization functions
		void Load(VU_BYTE** stream);
		void Load(FILE* filePtr);
		int SaveSize();
		int Save(VU_BYTE **stream);									// returns bytes written
		int Save(FILE *file);										// returns bytes written

		// function interface
		void Init (int, float, float);								// Sets up initial data
		void Deaggregate (void);									// Makes this drawable
		void Reaggregate (void);									// Cleans up the drawable object
		int IsTimed (void)						{ return flags & SPLF_IS_TIMED; };
		int IsLinked (void)						{ return flags & SPLF_IS_LINKED; };
		int InUse (void)						{ return flags & SPLF_IN_USE; }
		void Cleanup (void);
		CampaignTime RemovalTime (void);
		FalconEntity *GetCampObject(void);
		int GetCampIndex(void);
	};

// =============================
// Default timeout values
// =============================

#define CRATER_REMOVAL_TIME		3600000			// Removal time, in campaign time
#define HULK_REMOVAL_TIME		7200000			// Removal time, in campaign time

// =============================
// Global access functions
// =============================

void InitPersistantDatabase (void);

void CleanupPersistantDatabase (void);

// These two functions will create a persistant object and broadcast to all remote machines
void AddToTimedPersistantList (int vistype, CampaignTime removalTime, float x, float y);
void AddToLinkedPersistantList (int vistype, FalconEntity *campObj, int campIdx, float x, float y);

// These two functions will create a persistant object LOCALLY ONLY
void NewTimedPersistantObject (int vistype, CampaignTime removalTime, float x, float y);
void NewLinkedPersistantObject (int vistype, VU_ID campObjID, int campIdx, float x, float y);

void SavePersistantList(char* scenario);

void LoadPersistantList(char* scenario);

int EncodePersistantList(VU_BYTE** stream, int maxSize);

//sfr: added rem
void DecodePersistantList(VU_BYTE** stream, long *rem);

int SizePersistantList(int maxSize);

void CleanupPersistantList (void);

void UpdatePersistantObjectsWakeState (float px, float py, float range, CampaignTime now);

void CleanupLinkedPersistantObjects (FalconEntity *campObject, int index, int newVis, int ratio);

// These functions were designed to allow the campaign to create local copies of persistant
// entities upon receiving a damage message
void AddRunwayCraters (Objective o, int f, int craters);
void AddMissCraters (FalconEntity *e, int craters);
void AddHulk (FalconEntity *e, int hulkVisId);

// This stuff should go to another file

void UpdateNoCampaignParentObjectsWakeState (float px, float py, float range);

#endif
