#ifndef _ANIM_RESOURCE_H_
#define _ANIM_RESOURCE_H_

#define RLE_END			0x8000
#define RLE_SKIPROW		0x4000
#define RLE_SKIPCOL		0x2000
#define RLE_REPEAT		0x1000
#define RLE_NO_DATA		0xffff

#define RLE_KEYMASK		0xf000
#define RLE_COUNTMASK	0x0fff

#define COMP_NONE  0
#define COMP_RLE   1
#define COMP_DELTA 2

#pragma warning ( disable: 4200 )	// Prevent the zero length array warning
class ANIMATION
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		char  Header[4];
		long  Version;
		long  Width;
		long  Height;
		long  Frames;
		short Compression;
		short BytesPerPixel;
		long  Background;
		char  Start[];
};

typedef struct
{
	long Size;
	char Data[];
} ANIM_FRAME;
#pragma warning ( default: 4200 )	// Restore normal warning behavior

class ANIM_RES
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long ID;
		long flags;
		ANIMATION *Anim;
		ANIM_RES *Next;
};

class C_Animation
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		ANIM_RES *Root_;

		void C_Animation::ConvertAnim(ANIMATION *Data);
		void C_Animation::Convert16BitRLE(ANIMATION *Data);
		void C_Animation::Convert16Bit(ANIMATION *Data);

	public:
		C_Animation();
		C_Animation(char **stream);
		C_Animation(FILE *fp);
		~C_Animation();
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);

		void Setup();
		void Cleanup();

		ANIM_RES *LoadAnim(long ID,char *file);
		ANIM_RES *GetAnim(long ID);
		void SetFlags(long ID,long flags);
		long GetFlags(long ID);
		BOOL RemoveAnim(long ID);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *);
		void SaveText(HANDLE, C_Parser *) { ; }

#endif // PARSER
};

extern C_Animation *gAnimMgr;

#endif
