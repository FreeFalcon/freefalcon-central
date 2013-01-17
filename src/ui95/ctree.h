#ifndef _TREE_CLASS_H_
#define _TREE_CLASS_H_

//#pragma warning (push)
//#pragma warning (disable : 4100)


class TREELIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long ID_;
		long Type_; // Menu,Info,Button... (Menu gets (+/-) in front of it)
		long  x_,y_; // programatically set as items are open/closed
		long  state_; // Open/Closed
		C_Base *Item_;
		TREELIST *Prev,*Next; // Icons on same level as this one
		TREELIST *Child,*Parent; // Subgroup to this list
};

enum
{
	TREE_SORT_BY_ID=1,
	TREE_SORT_BY_ITEM_ID,
	TREE_SORT_CALLBACK,
	TREE_SORT_NO_ORDER,
};

class C_TreeList : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	protected:
	// variables
		// Save from here
		long		DefaultFlags_;
		long		Font_;
		short		SortType_;
		long		xoffset_,yoffset_;
								   // changes as branches are opened/closed (for scrolling)
		short		CheckFlag_;	// used for determining which part of a tree was selected
								// (Plus/Minus, Icon, Text part)

		// Don't save from here down
		long		treew_,treeh_; // Width/height of tree as it is currently drawn
		C_Hash		*Hash_;
		TREELIST	*Root_; // Very beginning of tree
		TREELIST	*LastFound_; // Item found in CheckHotSpot
		TREELIST	*LastActive_;
		TREELIST	*MouseFound_; // Item found in CheckHotSpot
		IMAGE_RSC	*ChildImage_[3];

		void (*DelCallback_)(TREELIST *);
		BOOL (*SortCB_)(TREELIST*,TREELIST*);
		BOOL (*SearchCB_)(TREELIST *); // used with SearchWithCB() - Kludge search engine so I can have non unique IDs
									   // 
	// routines
		long CalculateTreePositions(TREELIST *top,long offx,long offy);
		void DrawBranch(SCREEN *surface,TREELIST *branch,UI95_RECT *cliprect);
		TREELIST *CheckBranch(TREELIST *me,long mx,long my);
		TREELIST *FindItemWithCB(TREELIST *me);
		void SetControlParents(TREELIST *me);

		void Add(TREELIST *loc,TREELIST *item); // Dispense with error checking (& adding to hash table)
		void AddChild(TREELIST *loc,TREELIST *item); // Dispense with error checking (& adding to hash table)

	public:
		C_TreeList();
		C_TreeList(char **stream);
		C_TreeList(FILE *fp);
		~C_TreeList();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		// Setup Functions
		void Setup(long ID,short Type);
		void SetSortType(short sortby) { SortType_=sortby; }
		void SetImages(long ClosedID,long OpenID,long DisID);
		void SetXOffset(long xoff) { xoffset_=xoff; }
		void SetMinYOffset(long yoff) { yoffset_=yoff; }
		TREELIST *CreateItem(long ID,long ItemType,C_Base *Item);
		void SetDelCallback(void (*cb)(TREELIST *item)) { DelCallback_=cb; }
		BOOL FindVisible(TREELIST *top);
		BOOL AddItem(TREELIST *Brother,TREELIST *New);
		BOOL AddChildItem(TREELIST *Parent,TREELIST *NewItem);
		void DeleteItem(long cID);
		void DeleteItem(TREELIST *item);
		void DeleteBranch(TREELIST *top);
		void SetSearchCB(BOOL (*cb)(TREELIST*)) { SearchCB_=cb; }
		void SetSortCallback(BOOL (*cb)(TREELIST*,TREELIST*)) { SortCB_=cb; }
		BOOL ChangeItemID(TREELIST *item,long NewID);
		void MoveChildItem(TREELIST *newpar,TREELIST *item);
		long GetMenu();
		// Cleanup Functions
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		void SetFont(long ID) { Font_=ID; }
		long GetFont() { return(Font_); }

		void RemoveOldBranch(long UserSlot,long Age,TREELIST *me);
		void RemoveOld(long UserSlot,long Age);

		void SetRoot(TREELIST *newroot) { Root_=newroot; }
		TREELIST *Find(long cID);
		TREELIST *FindOpen(long cID);
		TREELIST *GetRoot() { return(Root_); }
		TREELIST *GetLastItem() { return(LastFound_); }
		TREELIST *GetNextBranch(TREELIST *me);
		TREELIST *GetChild(TREELIST *me);
		TREELIST *SearchWithCB(TREELIST *me) { if(SearchCB_ != NULL) return(FindItemWithCB(me)); return(NULL); }
		void SetItemState(long cID,short newstate);
		void ToggleItemState(long cID);
		void ToggleItemState(TREELIST *item) { if(item != NULL) item->state_=1-item->state_; }
		void SetAllBranches(long Mask,short newstate,TREELIST *me);
		void ClearAllStates(long Mask);
		void SetAllControlStates(short state,TREELIST *me);
		long CheckHotSpots(long relX,long relY);
		BOOL Process(long cID,short ButtonHitType);
		BOOL Dragable(long) {return(GetFlags() & C_BIT_DRAGABLE);}
		void RecalcSize();
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		BOOL MouseOver(long relX,long relY,C_Base *me);
		void GetItemXY(long ID,long *x,long *y);
		void SetSubParents(C_Window *);
		void HighLite(SCREEN *surface,UI95_RECT *cliprect);

		void ReorderBranch(TREELIST *branch); // Don't DO THIS unless you really need to
		void Activate();
		void Deactivate();
		C_Base *GetMe();
		BOOL CheckKeyboard(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount);

		// Making tree branches
		void AddTextItem(long ID,long Type,long ParentID,long TextID,long color);
		void AddTextItem(long ID,long Type,long ParentID,char *Text,long color);
		void AddWordWrapItem(long ID,long Type,long ParentID,long TextID,long w,long color);
		void AddBitmapItem(long ID,long Type,long ParentID,long ImageID);
		void AddHelpItem(long ID,long Type,long ParentID);
		void SetHelpItemImage(long ID,long ImageID,long x,long y);
		void SetHelpItemText(long ID,long TextID,long x,long y,long w,long color);
		void SetHelpFlagOn(long ID,long Flag);
		void SetHelpFlagOff(long ID,long Flag);
		void SetHelpItemFont(long ID,long FontID);

#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }
#endif // PARSER
};

//#pragma warning (pop)
#endif // _TREE_CLASS_H_
