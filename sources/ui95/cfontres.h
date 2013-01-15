#ifndef _C_FONT_RES_H_
#define _C_FONT_RES_H_

enum
{
	_FNT_CHECK_KERNING_		=0x40,
};

#pragma pack(1)

class KerningStr
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		short first;
		short second;
		short  add;
};

class CharStr
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		unsigned char flags;
		unsigned char w;

		char lead;
		char trail;
};

#pragma pack()

class C_Fontmgr
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		ID_;

		char		name_[32];
		long		height_;
		long		bytesperline_;

		long		fNumChars_;
		CharStr		*fontTable_;

		long		dSize_;
		char		*fontData_;

		long		kNumKerns_;
		KerningStr	*kernList_;

		short		first_;
		short		last_;

	public:

		C_Fontmgr();
		~C_Fontmgr();

		void Setup(long ID,char *fontfile);
		void Cleanup();

		long GetID() { return(ID_); }

		short First() { return(first_); }
		short Last()  { return(last_); }
		long ByteWidth() { return(bytesperline_); }

		long Width(_TCHAR *str);
		long Width(_TCHAR *str,long length);
		long Height();
		CharStr *GetChar(short ID);
		char *GetData() { return(fontData_); }
		char *GetName() { return(name_); }

		// no cliping version (except for screen)
		void Draw(SCREEN *surface,_TCHAR *str,WORD color,long x,long y);
		void Draw(SCREEN *surface,_TCHAR *str,long length,WORD color,long x,long y);
//!		void Draw(SCREEN *surface,_TCHAR *str,short length,WORD color,long x,long y);
		void DrawSolid(SCREEN *surface,_TCHAR *str,long length,WORD color,WORD bgcolor,long x,long y);
		void _DrawSolid16(SCREEN *surface,_TCHAR *str,long length,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect);//XX
		void _DrawSolid32(SCREEN *surface,_TCHAR *str,long length, DWORD color, DWORD bgcolor,long x,long y,UI95_RECT *cliprect);//XX
//!		void DrawSolid(SCREEN *surface,_TCHAR *str,short length,WORD color,WORD bgcolor,long x,long y);
		void DrawSolid(SCREEN *surface,_TCHAR *str,WORD color,WORD bgcolor,long x,long y);

		// clipping version (use cliprect)
		void Draw(SCREEN *surface,_TCHAR *str,WORD color,long x,long y,UI95_RECT *cliprect);
		void Draw(SCREEN *surface,_TCHAR *str,long length,WORD color,long x,long y,UI95_RECT *cliprect);
		void _Draw16(SCREEN *surface,_TCHAR *str,long length,WORD color,long x,long y,UI95_RECT *cliprect);//XX		
		void _Draw32(SCREEN *surface,_TCHAR *str,long length, DWORD dwColor,long x,long y, UI95_RECT *cliprect);//XX

//!		void Draw(SCREEN *surface,_TCHAR *str,short length,WORD color,long x,long y,UI95_RECT *cliprect);
		void DrawSolid(SCREEN *surface,_TCHAR *str,long length,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect);
//!		void DrawSolid(SCREEN *surface,_TCHAR *str,short length,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect);
		void DrawSolid(SCREEN *surface,_TCHAR *str,WORD color,WORD bgcolor,long x,long y,UI95_RECT *cliprect);

		// Font creation Functions (for converting winders fonts to my BFT format)
		void SetID(long ID)									{ ID_ = ID; }
		void SetName(char *name)							{ if(name) strcpy(name_,name); else memset(name,0,32); }
		void SetHeight(long height)							{ height_ = height; }
		void SetRange(long first,long last)					{ first_ = static_cast<short>(first); last_ = static_cast<short>(last); } 
		void SetBPL(long BPL)								{ bytesperline_ = BPL; }
		void SetTable(long count,CharStr *table)			{ fNumChars_ = count; if(count) fontTable_=table; else fontTable_=NULL; }
		void SetData(long size,char *data)					{ dSize_=size; if(size) fontData_=data; else fontData_=NULL; }
		void SetKerning(long count,KerningStr *kernlist)	{ kNumKerns_=count; if(count) kernList_=kernlist; else kernList_=NULL; }
		void Save(char *filename); 
};

#endif
