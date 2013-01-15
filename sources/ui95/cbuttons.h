#ifndef _BUTTON_H_
#define _BUTTON_H_

//#pragma warning (push)
//#pragma warning (disable : 4100)

class BUTTONLIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		COLORREF	FgColor_;
		COLORREF	BgColor_;
		O_Output	*Image_;
		O_Output	*Label_;
		long		x_,y_; // Label Offsets - Not used for image
};

class C_Button : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from here
		long origx_,origy_;
		long LabelFlags_;
		long DefaultFlags_;
		long Font_;
		short state_; // 0=Current,1=Up,2=Down,3=Disabled (Current means... if user hits return... this will be the button)
		short laststate_;
		short Percent_;
		short FixedHotSpot_;
		UI95_RECT HotSpot_;

		// Don't save from here
		C_Hash *Root_;
		O_Output *BgImage_;
		C_Base *Owner_;
	public:
#ifdef _UI95_PARSER_
		short UseHotSpot_;
#endif // PARSER
		C_Button();
		C_Button(char **stream);
		C_Button(FILE *fp);
		~C_Button(void);
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		// Setup Functions 
		void Setup(long ID,short Type,long x,long y);
		void SetHotSpot(UI95_RECT hotspot) { HotSpot_=hotspot; if(hotspot.left == -1 && hotspot.right == -1) UseHotSpot_=0; else UseHotSpot_=1; }
		void SetHotSpot(long x,long y,long x2,long y2) { HotSpot_.left=x;HotSpot_.top=y;HotSpot_.right=x2;HotSpot_.bottom=y2; if(x == -1 && x2 == -1) UseHotSpot_=0; else UseHotSpot_=1; }
		UI95_RECT GetHotSpot(void) { return(HotSpot_); }
		void SetBackImage(long ImageID);
		void SetLabel(long ID,_TCHAR *txt);
		void SetLabel(long ID,long txtID);
//!		void SetLabel(short ID,_TCHAR *txt);
//!		void SetLabel(short ID,long txtID);
		_TCHAR *GetLabel(short ID);
		void SetAllLabel(_TCHAR *txt);
		void SetAllLabel(long txtID);
		void SetColor(short ID,COLORREF color);
		void SetLabelOffset(short ID,long x,long y);
		void SetLabelColor(short ID,COLORREF color);
		void SetState(short state) { state_=state; }
		short GetState() { return(state_); }
		void SetImage(short ID,long ImageID);
		void ClearImage(short ID,long ImageID);
		void SetImage(short ID,IMAGE_RSC *img);
		void SetAnim(short BtnID,long AnimID,short animtype,short dir);
		void SetText(short ID, const _TCHAR *str);
		void SetText(short ID,long txtID);
		void SetFill(short ID,short w,short h);
		void SetXY(long x,long y);
		void SetPercent(short perc) { Percent_=perc; }
		short GetPercent() { return(Percent_); }
		BOOL TimerUpdate();

		_TCHAR *GetText(short ID);
		void SetFgColor(short ID,COLORREF color);
		void SetBgColor(short ID,COLORREF color);
		void SetFont(long FontID);
		void SetFlags(long flags);
		void SetOwner(C_Base *me) { Owner_=me; }

		// Cleanup Functions
		void Cleanup(void);

		// Query Functions
		long GetFont() { return(Font_); }
		short GetImageW(short ID);
		short GetImageH(short ID);
		O_Output *GetImage(short ID);

		void SetLabelInfo();
		void SetLabelFlagBitsOn(long flags);
		void SetLabelFlagBitsOff(long flags);

		void SetFixedHotSpot(short val) { FixedHotSpot_=val; }

		// Handler/Window Functions
		long CheckHotSpots(long relx,long rely);
		BOOL CheckKeyboard(unsigned char DKScanCode,unsigned char ,unsigned char ShiftStates,long ) { if((DKScanCode | (ShiftStates << 8)) == GetHotKey()) return(TRUE); return(FALSE); }
		BOOL Process(long ID,short ButtonHitType);
		BOOL Dragable(long ) {return(GetFlags() & C_BIT_DRAGABLE);}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void HighLite(SCREEN *surface,UI95_RECT *cliprect);
		BOOL MouseOver(long relX,long relY,C_Base *me);
		void GetItemXY(long ,long *x,long *y) { *x=GetX();*y=GetY();}
		BOOL Drag(GRABBER *,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *) { return FALSE; } 
		void SetSubParents(C_Window *);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }

#endif // PARSER
};

//#pragma warning (pop)
#endif
