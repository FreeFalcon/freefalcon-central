#ifndef _C_DRAW_LIST_H_
#define _C_DRAW_LIST_H_

class C_DrawList : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;
		C_Hash *Root_;
		MAPICONLIST *Last_;

	public:
		C_DrawList();
		C_DrawList(char **stream);
		C_DrawList(FILE *fp);
		~C_DrawList();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type);
		void Cleanup(void);
		void Add(MAPICONLIST *item);
		void Remove(long ID);

		long GetIconID() { if(Last_) return(Last_->ID); return(0); }
		long GetIconType() { if(Last_) return(Last_->Type); return(0); }
 		MAPICONLIST	*GetLastItem() { return(Last_); }

		long CheckHotSpots(long relX,long relY);
		BOOL Process(long ID,short HitType);

		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};

#endif
