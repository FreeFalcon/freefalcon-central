#ifndef _HELP_GUIDE_H_
#define _HELP_GUIDE_H_

class C_Help : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from here
		long DefaultFlags_;

		COLORREF	FgColor_;
		long		Font_;

		// Don't save from here
		O_Output	*Picture_;
		O_Output	*Text_;

	public:
		C_Help();
		C_Help(char **stream);
		C_Help(FILE *fp);
		~C_Help();

		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type);
		void Cleanup(void);

		void SetImage(long x,long y,long ImageID);
		void SetText(long x,long y,long w,long TextID);
		void SetFont(long FontID) { Font_=FontID; }
		void SetFgColor(COLORREF color) { FgColor_=color; }
		void SetSubParents(C_Window *parent);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		long CheckHotSpots(long ,long );
		BOOL Process(long ID,short hittype);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};

#endif
