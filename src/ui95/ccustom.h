#ifndef _CUSTOM_CTRL_H_
#define _CUSTOM_CTRL_H_

class C_Custom : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		DefaultFlags_;

		long		*ItemValues_;
		O_Output	*Items_;

		short		Section_;

		short		Count_;
		short		Last_;

	public:
		C_Custom();
		C_Custom(char **stream);
		C_Custom(FILE *fp);
		~C_Custom();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type,short NumCtrls);
		void Cleanup(void);

		O_Output *GetItem(long idx);

		void SetValue(long idx,long value);
		long GetValue(long idx);

		long CheckHotSpots(long relX,long relY);
		BOOL Process(long ID,short HitType);

		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};

#endif
