#ifndef _UI_TACREF_H_
#define _UI_TACREF_H_

#pragma warning ( disable: 4200 )	// Prevent the zero length array warning
#pragma pack(1)

enum
{
	_ENTITY_=1,
	_STATS_,
	_CATEGORY_,
	_DESCRIPTION_,
	_RWR_DATA_,
	_TEXT_,
};

struct Header
{
	short type;
	short size;
	char  Data[];
};

struct Entity
{
	// UI Ids
	long GroupID;
	long SubGroupID;
	long EntityID;

	// 3d Model ID
	short ModelID;
	short ZoomAdjust;
	short VerticalOffset;
	short HorizontalOffset;
	short MissileFlag;

	_TCHAR Name[32];
	char  PhotoFile[32];
	char  Data[];
};

struct Category
{
	//long GroupID;
	//long SubGroupID;
	//long EntityID;
	_TCHAR Name[40];
	char Data[];
};

struct TextString
{
	short length;
	_TCHAR String[];
};

struct Text4CatString
{
	long GroupID;
	long SubGroupID;
	long EntityID;
	short length;
	_TCHAR String[];
};

struct RWR_Data
{
	short SearchState; // Button state
	short LockState; // Button state
	long SearchTone; // Sound ID (when used, subtract 1 for real sound ID)
	long LockTone; // Sound ID  (when used, subtract 1 for real sound ID)
	_TCHAR Name[32];
};

#pragma pack()
#pragma warning ( default: 4200 )	// Restore normal warning behavior

#if 0
class TacticalReference
{
	private:

		C_Hash *Index_; // Index based on EntityID

		long Size_;
		char *TacRefData_;

	public:
		TacticalReference();
		~TacticalReference();

		BOOL Load(char *filename);
		void Cleanup();

		void MakeIndex(long Size);

		Entity *Find(long EntityID);
		Entity *FindFirst(long GroupID,long SubGroupID);

		Entity *GetFirst(long *offset);
		Entity *GetNext(long *offset);

		Category *GetFirstCat(Entity *ent,long *offset);
		Category *GetNextCat(Entity *ent,long *offset);

		TextString *GetFirstCatText(Category *cat,long *offset);
		TextString *GetNextCatText(Category *cat,long *offset);
};
#endif
#endif // _UI_TACREF_H_