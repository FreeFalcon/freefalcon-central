#ifndef _C_BSPLIST_H_
#define _C_BSPLIST_H_

typedef struct UI_BSPList BSPLIST;

enum
{
	BSP_DRAWOBJECT=1,
	BSP_DRAWBSP,
	BSP_DRAWBRIDGE,
	BSP_DRAWBUILDING,
};

typedef struct
{
	float Heading;
	float Pitch;
	float Distance;

	float Direction;
	float PosX;
	float PosY;
	float PosZ;
	float DeltaX;
	float DeltaY;
	float DeltaZ;
	float MinDistance,MaxDistance;
	BOOL  CheckPitch;
	float MinPitch,MaxPitch;
} OBJECTINFO;

struct UI_BSPList
{
	long ID;
	long type;
	BSPLIST *owner;
	DrawableObject *object;
	BSPLIST *Next;
};

class C_BSPList
{
	public:
		BSPLIST *Root_;
		long LockList_[1024];
		long LockCount_;

	public:
		C_BSPList()
		{
			Root_=NULL;
			LockCount_=0;
		}
		~C_BSPList() {}
		void Setup();
		void Cleanup();
		void Add(BSPLIST **list,BSPLIST *obj);
		void Add(BSPLIST *obj)	{ Add(&Root_,obj); }
		void RemoveList(BSPLIST **list);
		void RemoveAll()
		{
			RemoveList(&Root_);
			RemoveLockList();
		}
		void RemoveLockList();
		void Remove(long ID,BSPLIST **top);
		void Remove(long ID)	{ Remove(ID,&Root_); }
		BOOL FindLock(long ID);
		void Lock(long ID);
		BSPLIST *Find(long ID,BSPLIST *list);
		BSPLIST *Load(long ID,long objID);
		BSPLIST *LoadBSP(long ID,long objID);
		BSPLIST *LoadBridge(long ID,long objID);
		BSPLIST *LoadBuilding(long ID,long objID,Tpoint *pos,float heading);
		BSPLIST *CreateContainer(long ID,Objective obj,short f,short fid,Falcon4EntityClassType *classPtr,FeatureClassDataType*	fc);
		BSPLIST *LoadDrawableFeature(long ID,Objective obj,short f,short fid,Falcon4EntityClassType *classPtr,FeatureClassDataType*	fc,Tpoint *objPos,BSPLIST *Parent);
		BSPLIST *LoadDrawableUnit(long ID,long visType,Tpoint *objPos,float facing,uchar domain,uchar type,uchar stype);
		BSPLIST *Find(long ID)	{ return(Find(ID,Root_)); }
		BSPLIST *GetFirst() { return(Root_); }
};

extern C_BSPList *gBSPList;

#endif