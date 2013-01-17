#ifndef _POPUP_LIST_H_
#define _POPUP_LIST_H_

// Sorry, this one is kind of recursive in its definitions
// (a window inside a control, instead of vice versa)

class C_PopupList;

typedef struct PopUpStr POPUPLIST;

struct PopUpStr
{
	long ID_;
	long flags_;
	long Group_;
	O_Output *Label_;
	O_Output *MenuIcon_;
	O_Output *CheckIcon_;
	C_PopupList *SubMenu_;
	void (*Callback_)(long,short,C_Base *);
	POPUPLIST *Next;
	short Type_;
	short State_;
};

class C_PopupList : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from Here
		long DefaultFlags_;
		long Font_;
		long Opaque_;
		long MenuIconID_;
		long CheckIconID_;
		COLORREF NormalColor_,SelColor_,DisColor_,BarColor_,BgColor_,BorderColor_;
		short WinType_;
		short Direction_;

		// DOn't save from here down
		short Selected_;
		short Count_;
		POPUPLIST *Root_;
		C_Window *Window_;
		C_Handler *Handler_;

		POPUPLIST *FindID(long ID);
		void GetSize(short *width,short *height);

	public:
		void (*OpenCallback_)(C_Base *,C_Base*);
		C_PopupList();
		C_PopupList(char **stream);
		C_PopupList(FILE *fp);
		~C_PopupList();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short,C_Handler *Handler,long ArrowID,long CheckID);
		BOOL AddItem(long ID,short Type,_TCHAR *Str,long ParentID);
		BOOL AddItem(long ID,short Type,long txtID,long ParentID);
		void RemoveList(POPUPLIST *list);
		void RemoveItem(long)	{ ; }
		void RemoveAllItems();
		void SetFont(long font);
		void SetFlags(long flags);
		void SetCallback(long ID,void (*Routine)(long,short,C_Base *));
 		void SetItemFlagBitOn(long ID,long flags);
 		void SetItemFlagBitOff(long ID,long flags);
		void SetNormColor(COLORREF color) { NormalColor_=color; }
		void SetSelColor(COLORREF color) { SelColor_=color; }
		void SetDisColor(COLORREF color) { DisColor_=color; }
		void SetBarColor(COLORREF color) { BarColor_=color; }
		void SetBgColor(COLORREF color) { BgColor_=color; }
		void SetBorderColor(COLORREF color) { BorderColor_=color; }
		void SetOpaque(long percentage) { Opaque_=percentage; }
		// Cleanup functions
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void ClearRadioGroup(long GroupID);
		void SetItemState(long ID,short val);
		short GetItemState(long ID);
		void SetItemGroup(long ID,long Group);
		long GetItemGroup(long ID);
		void SetItemLabel(long ID,_TCHAR *txt);
		void SetItemLabel(long ID,long textID);
		_TCHAR *GetItemLabel(long ID);
		C_Window *GetWindow() { return(Window_); }
		void SetOpenCallback(void (*cb)(C_Base *menu,C_Base *caller)) { OpenCallback_=cb; }
		void *GetOpenCallback() { return(OpenCallback_); }

		long GetFont() { return(Font_); }

		void GetWindowSize(short *width,short *height);
		// Query Functions
		BOOL Opened() { if(Window_ != NULL) return(TRUE); return(FALSE);}
		void CloseSubMenus();

		// Once you have added all the menus... and are ready to use a Popup Menu
		// Call OpenWindow (Dir is either CPOP_LEFT or CPOP_RIGHT for side of menus
		// that sub-menus will open (until a side of the screen is hit))
		BOOL OpenWindow(short x,short y,short Dir);
		// When Done Call CloseWindow
		BOOL CloseWindow();

		// Handler/Window Functions
		long CheckHotSpots(long relX,long relY);
		BOOL Process(long ID,short ButtonHitType);
		BOOL Dragable(long)	{ return FALSE;	}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		// This MouseOver actually does Window I/O type stuff
		BOOL MouseOver(long relX,long relY,C_Base *);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *Hndlr);
		void SaveText(HANDLE,C_Parser *)	{ ; }

#endif // PARSER
};

#endif
