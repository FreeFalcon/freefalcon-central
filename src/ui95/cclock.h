#ifndef _CLOCK_EDIT_H_
#define _CLOCK_EDIT_H_

//#pragma warning (push)
//#pragma warning (disable : 4100)

class C_Clock : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from here
		long		Defaultflags_;

		long		Font_;
		COLORREF	NormColor_,SelColor_,CursorColor_;

		// Don't save from here
		C_EditBox	*Day_;
		C_EditBox	*Hour_;
		C_EditBox	*Minute_;
		C_EditBox	*Second_;
		O_Output	*Sep0_;
		O_Output	*Sep1_;
		O_Output	*Sep2_;

		BOOL (*TimerCallback_)(C_Base *me);

		C_EditBox	*CurEdit_;
		long		Section_;

	public:

		C_Clock();
		C_Clock(char **stream);
		C_Clock(FILE *fp);
		~C_Clock();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type);
		void Cleanup();

		void SetSep0Str(long TextID);
		void SetSep1Str(long TextID);
		void SetSep2Str(long TextID);

		void SetXY(long x,long y);
		void SetFont(long FontID);

		void SetNormColor(COLORREF color) { NormColor_=color; }
		void SetSelColor(COLORREF color) { SelColor_=color; }
		void SetCursorColor(COLORREF color) { CursorColor_=color; }

		void EnableDay();

		void SetDay(long dy)		{ if(Day_) Day_->SetInteger(dy+1); }
		void SetHour(long hr)		{ if(Hour_) Hour_->SetInteger(hr); }
		void SetMinute(long mn)		{ if(Minute_) Minute_->SetInteger(mn); }
		void SetSecond(long sc)		{ if(Second_) Second_->SetInteger(sc); }

		void SetLast(long val)		{ if(CurEdit_) CurEdit_->SetInteger(val); }

		long GetDay()				{ if(Day_) return(Day_->GetInteger()-1); return(0); }
		long GetHour()				{ if(Hour_) return(Hour_->GetInteger()); return(0); }
		long GetMinute()			{ if(Minute_) return(Minute_->GetInteger()); return(0); }
		long GetSecond()			{ if(Second_) return(Second_->GetInteger()); return(0); }

		long GetLast()				{ if(CurEdit_) return(CurEdit_->GetInteger()); return(0); }

		C_EditBox *GetCurrentCtrl()	{ return(CurEdit_); }
		C_EditBox *GetDayCtrl()		{ return(Day_); }
		C_EditBox *GetHourCtrl()	{ return(Hour_); }
		C_EditBox *GetMinuteCtrl()	{ return(Minute_); }
		C_EditBox *GetSecondCtrl()	{ return(Second_); }

		void SetTime(long theTime);
		long GetTime();

		void SetSubParents(C_Window *Parent);
		long CheckHotSpots(long relX,long relY);
		BOOL CheckKeyboard(unsigned char ,unsigned char ,unsigned char ,long ) { return FALSE; }
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)	{ return FALSE; }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetTimerCallback(BOOL (*Callback)(C_Base *me)) { TimerCallback_=Callback; }
		BOOL TimerUpdate();

		// for dragging/dropping, there needs to be support for going to
		// another window
// Parser Stuff
#ifdef _UI95_PARSER_

		short LocalFind(char *str);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE,C_Parser *) { ; }

#endif // parser

};

//#pragma warning (pop)

#endif
