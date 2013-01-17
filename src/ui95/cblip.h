#ifndef _EVENT_BLIP_H_
#define _EVENT_BLIP_H_

class BLIP
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		short x,y;
		uchar side;
		uchar state;
		long  time;
		BLIP *Next;
};

class C_Blip : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;

		BLIP *Root_;
		BLIP *Last_;

		IMAGE_RSC *BlipImg_[8][8];
		O_Output  *Drawer_;

	public:
		C_Blip();
		C_Blip(char **stream);
		C_Blip(FILE *fp);
		~C_Blip();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type);
		void Cleanup(void);
		void InitDrawer(); // MUST BE CALLED AFTER SetImage() 
		void AddBlip(short x,short y,uchar side,long starttime);
		void BlinkLast();
		void Update(long curtime);
		void SetImage(IMAGE_RSC *img,uchar side,uchar blipno) { BlipImg_[side&7][blipno&7]=img; }
		void SetImage(long ImageID,uchar side,uchar blipno) { SetImage(gImageMgr->GetImage(ImageID),side,blipno); }
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void Refresh(BLIP *blip);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void RemoveAll();

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short,long,_TCHAR *,C_Handler *);
		void SaveText(HANDLE,C_Parser *) { ; }

#endif // PARSER
};

#endif
