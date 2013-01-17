#ifndef _C_IMAGE_RESOURCE_H_
#define _C_IMAGE_RESOURCE_H_

class C_Resmgr;

// include the resmgr's supported types here
#include "imagersc.h"
#include "soundrsc.h"
#include "flatrsc.h"

enum
{
	_RSC_8_BIT_			=0x00000001,
	_RSC_16_BIT_		=0x00000002,
	_RSC_USECOLORKEY_	=0x40000000,
	_RSC_SINGLE_		=0x00000001,
	_RSC_MULTIPLE_		=0x00000002,

// Add types as needed
	_RSC_IS_IMAGE_		=100,
	_RSC_IS_SOUND_		=101,
	_RSC_IS_FLAT_		=102,
};

class C_Resmgr
{
	friend class IMAGE_RSC;
	friend class SOUND_RSC;
	friend class FLAT_RSC;

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long ID_;
		long Type_;

		long ResIndexVersion_;
		long ResDataVersion_;

		short reds,greens,blues; // shift values to convert to SCREEN format

		C_Hash	*IDTable_;
		C_Hash	*Index_;
		char	*Idx_;
		char	*Data_;
		WORD	ColorKey_;

		char	name_[MAX_PATH];
		FILE *OpenResFile(const char *name, const char *sfx, const char *mode);
	public:

		C_Resmgr();
		~C_Resmgr();

		long GetID()	{ return(ID_); }
		long GetType()	{ return(Type_); }

// User callable functions
		void Setup(long ID,char *filename,C_Hash *IDList);
		void Setup(long ID);
		void Cleanup();

		long Status() { if(!Data_) { if(Index_) return(0x01); return(0); } return(0x03); }

		void SetColorKey(WORD Key) { ColorKey_=Key; }
		// Convert Data_ to Screen format
		void ConvertToScreen();
		void SetScreenFormat(short rs,short gs,short bs) { reds=rs; greens=gs; blues=bs; }
		void LoadIndex();
		void AddIndex(long ID,IMAGE_RSC *resheader);
		void AddIndex(long,SOUND_RSC*) {}
		void SetData(char *data) { if(Data_) delete Data_; Data_=data; }
		char *GetData() { return(Data_); }
		void LoadData();
		void UnloadData();
		void *Find(long ID) { if(Index_) return(Index_->Find(ID)); return(NULL); }

		char *ResName() { return(name_); }

		C_Hash *GetIDList() { return(Index_); }
};

#endif
