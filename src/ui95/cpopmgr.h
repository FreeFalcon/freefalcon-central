#ifndef _POPUP_MENU_MGR_H_
#define _POPUP_MENU_MGR_H_

typedef struct PopupMenuListStr POPUPMENU;

struct PopupMenuListStr
{
	C_PopupList	*Menu;
	long		flags;
	POPUPMENU	*Next;
};

class C_PopupMgr
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from here
		long		Flags_;
		short		CurrentType_;
		short		CurrentClient_;
		short		LastX_,LastY_;
		short		ready_;

		// Don't save from here down
		POPUPMENU	*Root_;
		POPUPMENU	*Current_;
		C_Base		*Control_;
		C_Handler	*Handler_;
		short Ready() { return(ready_); }

	public:
		C_PopupMgr();
		C_PopupMgr(char **stream);
		C_PopupMgr(FILE *fp);
		~C_PopupMgr();
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);

		void Setup(C_Handler *handler);
		void Cleanup();

		void AddMenu(C_PopupList *newmenu);
		C_PopupList *GetMenu(long ID);
		C_PopupList *GetCurrent() { if(Current_) return(Current_->Menu); return(NULL); }
		void GetCurrentXY(short *x,short *y) { *x=LastX_; *y=LastY_; }
		void RemoveMenu(long ID);
		void SetFlags(long flags) { Flags_=flags; }
		void SetFlagBitOns(long flags) { Flags_|=flags; }
		void SetFlagBitOff(long flags) { Flags_&= (0xffffffff ^ flags); }
		short GetCallingType() { return(CurrentType_); }
		short GetCallingClient() { return(CurrentClient_); }
		C_Base *GetCallingControl() { return(Control_); }
		BOOL OpenWindowMenu(C_Window *win,long x,long y);
		BOOL OpenMenu(long ID,long x,long y,C_Base *control);
		BOOL Opened(long ID);
		BOOL AMenuOpened();
		void CloseMenu();
};

extern C_PopupMgr *gPopupMgr;

#endif
