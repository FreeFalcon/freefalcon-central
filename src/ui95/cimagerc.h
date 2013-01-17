#ifndef _IMAGE_RESOURCE_H_
#define _IMAGE_RESOURCE_H_

class C_Image
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
#ifdef _UI95_PARSER_
		long	LastID_;
#endif
		C_Hash	*Root_;
		C_Hash	*Finder_;
		C_Hash	*ColorOrder_;
		C_Hash	*IDOrder_;
		WORD	ColorKey_;
		short	red_shift_, green_shift_, blue_shift_;


		long BuildColorTable(WORD *,long ,long ,long );
		void MakePalette(WORD *,long );
		void ConvertTo8Bit(WORD *,unsigned char *,long ,long );
		void CopyArea(WORD *src,WORD *dest,long w,long h);

	public:
		C_Image();
		~C_Image();

		void Setup();
		void Cleanup();

		void SetScreenFormat(short rs,short gs,short bs) { red_shift_=rs; green_shift_=gs; blue_shift_=bs; }

		C_Resmgr *AddImage(long ID,long LastID,UI95_RECT *rect,short x,short y);
		C_Resmgr *AddImage(long ID,long LastID,short x,short y,short w,short h,short cx,short cy);
		C_Resmgr *LoadImage(long ID,char *file,short x,short y);
		C_Resmgr *LoadFile(long ID,char *file,short x,short y);
		C_Resmgr *LoadRes(long ID,char *file);
		C_Resmgr *LoadPrivateRes(long ID,char *file);

		void SetColorKey(WORD colorkey) { ColorKey_=colorkey; }

		IMAGE_RSC *GetImage(long ID);
		C_Resmgr *GetImageRes(long ID);

		BOOL RemoveImage(long ID);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *);
		void SaveText(HANDLE ,C_Parser *) { ; }
#endif // PARSER
};

extern C_Image *gImageMgr;

#endif
