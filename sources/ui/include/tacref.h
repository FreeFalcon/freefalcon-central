#ifndef _UI_TACREF_H_
#define _UI_TACREF_H_

#pragma warning ( disable: 4200 )	// Prevent the zero length array warning
#pragma pack(1)
//TJL 12/27/03 
#include "Graphics\Include\drawbsp.h"

enum
{
	_ENTITY_=1,
	_STATS_,
	_CATEGORY_,
	_DESCRIPTION_,
	_RWR_MAIN_,
	_RWR_DATA_,
	_TEXT_,
};

struct TextString
{
	short length;
	_TCHAR String[];
};

struct Header
{
	short type;
	short size;
	char  Data[];
};

struct CatText
{
	long GroupID;
	long SubGroupID;
	long EntityID;
	short length;
	_TCHAR String[];
};

class Category
{
	public:
		short type; // Eliminates header in between
		short size;
		_TCHAR Name[40];
		char Data[];
	public:
		CatText *GetFirst(long *offset);
		CatText *GetNext(long *offset);
};


class Statistics
{
	private:
		short type;
		short size;
		char Data[];

	public:
		Category *GetFirst(long *offset);
		Category *GetNext(long *offset);
};

class Description
{
	private:
		short type;
		short size;
		char Data[];

	public:
		TextString *GetFirst(long *offset);
		TextString *GetNext(long *offset);
};

struct Radar
{
	short SearchState; // Button state
	short LockState; // Button state
	long SearchTone; // Sound ID (when used, subtract 1 for real sound ID)
	long LockTone; // Sound ID  (when used, subtract 1 for real sound ID)
	_TCHAR Name[32];
};

class RWR
{
	private:
		short type;
		short size;
		char Data[];

	public:
		Radar *GetFirst(long *offset);
		Radar *GetNext(long *offset);
};

class Entity
{
	public:
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
	

	public:
		Statistics *GetStats();
		Description *GetDescription();
		RWR *GetRWR();
};

#pragma pack()
#pragma warning ( default: 4200 )	// Restore normal warning behavior

class TacticalReference
{
	private:

		C_Hash *Index_; // Index based on EntityID
		long Size_;
		char *Data_;

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
		
};

#endif // _UI_TACREF_H_