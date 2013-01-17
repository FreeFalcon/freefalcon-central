#ifndef _LISTBOX_H_
#define _LISTBOX_H_

// Sorry, this one is kind of recursive in its definitions
// (a window inside a control, instead of vice versa)

class C_ListBox;

#define LISTBOX_MAX_LEN 65

class LISTBOX
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		C_Button *Label_;
		LISTBOX *Next;
};

class C_ListBox : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	protected:
		// Save from here down
		long			DefaultFlags_;
		long			LabelVal_;
		long			Font_;
		short			WinType_;
		short			Selected_;
		short			Count_;
		short			ScrollCount_; // sfr: added for scrollbar control
		float			Opaque_;
		UI95_RECT		BgRect_;
		COLORREF		NormalColor_, SelColor_, DisColor_, BarColor_, BgColor_, LabelColor_;

		// Don't save from here down
		LISTBOX			*Root_;
		O_Output		*BgImage_;
		O_Output		*Label_;
		C_ScrollBar		*ScrollBar_;
		O_Output		*DropDown_;

		C_Window		*Window_;
		C_Window		*SaveParent_;
		C_Handler		*Handler_;
		void			(*OpenCallback_)(C_Base *);

		short			GetListHeight();
		BOOL			OpenWindow(short x,short y,short w,short h);

	public:
#ifdef _UI95_PARSER_
		long BtnIDs_[4];
#endif // PARSER
		C_ListBox();
		C_ListBox(char **stream);
		C_ListBox(FILE *fp);
		~C_ListBox();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type,C_Handler *Handler);
		C_ListBox *AddItem(long ID,short,_TCHAR *Str);
		C_ListBox *AddItem(long ID,short Type,long txtID);
		void RemoveItem(long) { ; }
		void RemoveAllItems();
 		void SetItemFlags(long ID,long flags);
 		void SetItemGroup(long ID,long group);
 		void SetItemCluster(long ID,long cluster);
 		void SetItemUserData(long ID,short idx,long value);
		void SetRoot(LISTBOX *NewRoot);
		void SetBgImage(long ID);
		void SetNormColor(COLORREF color) { NormalColor_=color; }
		void SetSelColor(COLORREF color) { SelColor_=color; }
		void SetDisColor(COLORREF color) { DisColor_=color; }
		void SetBarColor(COLORREF color) { BarColor_=color; }
		void SetBgColor(COLORREF color) { BgColor_=color; }
		void SetLabelColor(COLORREF color) { LabelColor_=color; }
		void SetOpaque(float percentage) { Opaque_=percentage; }
		void SetBgFill(int x, int y, int w, int h);
		void SetFlags(long flags);
		void SetFont(long font);
		LISTBOX *GetRoot() { return(Root_); }
		LISTBOX	*FindID(long ID);
		long GetFont() { return(Font_); }
		void SetDropDown(long ID);
		// sfr: added last parameter for scroll window number of items
		void AddScrollBar(long MinusUp, long MinusDown, long PlusUp, long PlusDown, long Slider, long nItems=-1);
		C_Button *GetItem(long ID);
		BOOL CloseWindow();

		// Cleanup functions
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		// Query Functions
		_TCHAR *GetText() { if (Label_ != NULL) return(Label_->GetText()); return(NULL); }
		long GetTextID() { return(LabelVal_); }
		void SetValue(long ID);
		void SetValueText(long inText); // JB 011124

		void SetOpenCallback(void (*cb)(C_Base*))	{ OpenCallback_=cb; }

		// Handler/Window Functions
		long CheckHotSpots(long relX,long relY);
		BOOL Process(long ID,short ButtonHitType);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		// This MouseOver actually does Window I/O type stuff
		void SetSubParents(C_Window *Parent);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *Hndlr);
		void SaveText(HANDLE ,C_Parser *) { ; }
#endif // PARSER
};

#endif
