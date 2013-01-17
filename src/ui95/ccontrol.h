#ifndef _CONTROL_BASE_CLASS_
#define _CONTROL_BASE_CLASS_

class C_Window;

class C_Control : public C_Base
{
	public:
		enum
		{
			CSB_IS_VALUE=1,
			CSB_IS_PTR,
			CSB_IS_CLEANUP_PTR,
			CSB_MAX_USERDATA=8,
		};
		// Save from here
	protected:
		long		Cursor_;
		long		DragCursor_;
		long		MenuID_;
		long		HelpTextID_;
		COLORREF	MouseOverColor_;
		long		RelX_,RelY_;
		short		HotKey_;
		short		MouseOver_;
		short		MouseOverPercent_;
		// sfr: added for mousewheel
		int         Increment_;

		// Don't save from here
		C_Hash		*Sound_;
		void		(*Callback_)(long,short,C_Base*);

	public:
		C_Control();
		C_Control(char **stream);
		C_Control(FILE *fp);

		virtual ~C_Control()
		{
			if(Sound_)
			{
				Sound_->Cleanup();
				delete Sound_;
			}
		}
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);


		BOOL MouseOver(long relX,long relY,C_Base *me);

// Assignment Functions
		void SetRelX(long x)							{ RelX_=x; }
		void SetRelY(long y)							{ RelY_=y; }
		void SetRelXY(long x,long y)					{ RelX_=x; RelY_=y; }
		void SetCursorID(long id)						{ Cursor_=id; }
		void SetDragCursorID(long id)					{ DragCursor_=id; }
		void SetMenu(long id)							{ MenuID_=id; }
		void SetHelpText(long id)						{ HelpTextID_=id; }
		void SetHotKey(short key)						{ HotKey_=key; }
		void SetCallback(void (*cb)(long,short,C_Base*)) { Callback_=cb; }
		void SetSound(long ID,short type);
		void SetMouseOver(short state)					{ MouseOver_=state; }
		void SetMouseOverColor(COLORREF color)			{ MouseOverColor_=color; }
		void SetMouseOverPerc(short perc)				{ MouseOverPercent_=perc; }

// Querry Functions
		BOOL  IsBase()					{ return(FALSE); }
		BOOL  IsControl()				{ return(TRUE); }
		long GetRelX()					{ return(RelX_); }
		long GetRelY()					{ return(RelY_); }
		int  GetIncrement()             { return(Increment_); }
		long  GetCursorID()				{ return(Cursor_); }
		long  GetDragCursorID()			{ return(DragCursor_); }
		long  GetMenu()					{ return(MenuID_); }
		long  GetHelpText()				{ return(HelpTextID_); }
		short GetHotKey()				{ return(HotKey_); }
		short GetMouseOver()			{ return(MouseOver_); }
		void HighLite(SCREEN *surface,UI95_RECT *cliprect);
		void (*GetCallback())(long,short,C_Base*)	{ return(Callback_); }
		SOUND_RES *GetSound(short type);
};

#endif
