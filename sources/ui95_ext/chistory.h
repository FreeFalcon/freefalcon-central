#ifndef _C_HISTORY_H_
#define _C_HISTORY_H_

#define _HIST_MAX_TEAMS_ (8)

class C_History : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		short		Count_;
		O_Output	*Data_;

		long ImageID_[_HIST_MAX_TEAMS_];

	public:
		C_History();
		C_History(char **stream);
		C_History(FILE *fp);
		~C_History();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short type,short count);
		void Cleanup();

		void SetImage(long idx,long ID) { if(idx < _HIST_MAX_TEAMS_) ImageID_[idx]=ID; }

		void AddIconSet(short idx,short team,short x,short y);

		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};



#endif
