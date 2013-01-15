#ifndef _C_Victory_H_
#define _C_Victory_H_

class C_Victory : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		Defaultflags_;
		long		Font_;

		short		Section_; // 1=Team control... 5=Points control

		short		State_;
		short		NumberX_;
		short		TeamX_;
		short		ActionX_;
		short		TargetX_;
		short		ArgsX_;
		short		PointsX_;

		COLORREF	Color_[3];

		C_Text		*Number_;
		C_ListBox	*Team_;
		C_ListBox	*Action_;
		C_Button	*Target_;
		C_ListBox	*Args_;
		C_EditBox	*Points_;

		TREELIST	*Owner_;
		void		*ptr_;

	public:

		C_Victory();
		C_Victory(char **stream);
		C_Victory(FILE *fp);
		~C_Victory();
		long Size();
		void Save(char **)		{ ; }
		void Save(FILE *)		{ ; }

		void Setup(long ID,short Type);
		void Cleanup();

		void SetOwner(TREELIST *limb)					{ Owner_=limb; }
		TREELIST *GetOwner()							{ return(Owner_); }

		void SetX(long x);
		void SetXY(long x,long y);
		void SetXYWH(long x,long y,long w,long h);

		void SetNumberX(short x)	{ NumberX_=x; if(Number_) Number_->SetX(GetX()+x); }
		void SetTeamX(short x)		{ TeamX_=x; if(Team_) Team_->SetX(GetX()+x); }
		void SetActionX(short x)	{ ActionX_=x; if(Action_) Action_->SetX(GetX()+x); }
		void SetTargetX(short x)	{ TargetX_=x; if(Target_) Target_->SetX(GetX()+x); }
		void SetArgsX(short x)		{ ArgsX_=x; if(Args_) Args_->SetX(GetX()+x); }
		void SetPointsX(short x)	{ PointsX_=x; if(Points_) Points_->SetX(GetX()+x); }

		void SetNumber(C_Text *txt)		{ Number_=txt; }
		void SetTeam(C_ListBox *lbox)	{ Team_=lbox; }
		void SetAction(C_ListBox *lbox)	{ Action_=lbox; }
		void SetTarget(C_Button *btn)	{ Target_=btn; }
		void SetArgs(C_ListBox *lbox)	{ Args_=lbox; }
		void SetPoints(C_EditBox *ebox)	{ Points_=ebox; }

		C_Text    *GetNumber()		{ return(Number_); }
		C_ListBox *GetTeam()		{ return(Team_); }
		C_ListBox *GetAction()		{ return(Action_); }
		C_Button  *GetTarget()		{ return(Target_); }
		C_ListBox *GetArgs()		{ return(Args_); }
		C_EditBox *GetPoints()		{ return(Points_); }

		void SetPtr(void *ptr)		{ ptr_=ptr; }
		void *GetPtr()				{ return(ptr_); }

		void SetState(short state);
		void SetFont(long FontID);
		void SetColors(COLORREF off,COLORREF on,COLORREF done) { Color_[0]=off; Color_[1]=on; Color_[2]=done; }
		void Activate();
		void Deactivate();

		BOOL CheckKeyboard(uchar DKScanCode,uchar Ascii,uchar ShiftStates,long RepeatCount);
		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		void SetSubParents(C_Window *Parent);
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)		{ return FALSE; }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};

#endif
