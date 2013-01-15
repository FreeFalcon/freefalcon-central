#ifndef _C_BASE_CLASS_
#define _C_BASE_CLASS_

class C_Window;

#pragma warning (push)
#pragma warning (disable : 4100)

typedef struct
{
	short type;
	union
	{
		long number;
		void *ptr;
	} data;
} USERDATA;


class C_Base
{
	public:
		enum
		{
			// Userdata stuff
			CSB_IS_VALUE=1,
			CSB_IS_PTR,
			CSB_IS_CLEANUP_PTR,
		};
	protected:
		enum
		{
			_GROUP_=0,
			_CLUSTER_,
			NUM_SECTIONS,
		};
		// Save from here
		long	ID_;
		long	Section_[NUM_SECTIONS];
		long	Flags_;
		short	_CType_;
		short	Type_;
		long	x_,y_,w_,h_;
		short	Client_;

		// Don't save
		short	Ready_;
		C_Hash	*User_;

	public:
		C_Window *Parent_;

		C_Base()
		{
			short i;

			ID_=0;
			_CType_=0;
			Type_=0;

			for(i=0;i<NUM_SECTIONS;i++)
				Section_[i]=0;

			Client_=0;

			x_=0;
			y_=0;
			w_=0;
			h_=0;

			Flags_=0;
			Parent_=NULL;
			User_=NULL;
			Ready_ = 0;		// OW
		}

		C_Base(char **stream);
		C_Base(FILE *fp);

		virtual ~C_Base()
		{
			if(User_)
			{
				User_->Cleanup();
				delete User_;
			}
		}

		long Size();
		void Save(char **stream);
		void Save(FILE *fp);
// Assignment Functions
		void SetID(long id)										{ ID_=id; }
		void SetType(short type)								{ Type_=type; }
		void _SetCType_(short ctype)							{ _CType_=ctype; }
		void SetGroup(long id)									{ Section_[_GROUP_]=id; }
		void SetCluster(long id)								{ Section_[_CLUSTER_]=id; }
		void SetClient(short client)							{ Client_=client; }
		void SetControlFlags(long flags)						{ Flags_=flags; }
		virtual void SetFlags(long flags)						{ Flags_=flags; }
		virtual void SetFlagBitOn(long bits)					{ Flags_ |= bits; }
		virtual void SetFlagBitOff(long bits)					{ Flags_ &= ~bits; }
		virtual void SetX(long x)								{ x_=x; }
		virtual void SetY(long y)								{ y_=y; }
		virtual void SetW(long w)								{ w_=w; }
		virtual void SetH(long h)								{ h_=h; }
		virtual void SetXY(long x,long y)						{ x_=x; y_=y; }
		virtual void SetWH(long w,long h)						{ w_=w; h_=h; }
		virtual void SetXYWH(long x,long y,long w,long h)		{ x_=x; y_=y; w_=w; h_=h; }
		virtual void SetRelX(long x)							{}
		virtual void SetRelY(long y)							{}
		virtual void SetRelXY(long x,long y)					{}
		void EnableGroup(long ID)								{ SetFlags(Flags_ & ~C_BIT_INVISIBLE); }
		void DisableGroup(long ID)								{ SetFlags(Flags_ | C_BIT_INVISIBLE); }
		void SetParent(C_Window *win)							{ Parent_=win; }
		void SetReady(short r)									{ Ready_=r; }
		void SetUserNumber(long idx,long value);
		void SetUserPtr(long idx,void *value);
		void SetUserCleanupPtr(long idx,void *value);
		virtual void SetState(short state)						{}
		virtual void SetHotKey(short key)						{}
		virtual void SetMenu(long ID)							{}
		virtual void SetFont(long ID)							{}
		virtual void SetSound(long ID,short type)				{}
		virtual void SetCursorID(long id)						{}
		virtual void SetDragCursorID(long id)					{}
		virtual void SetHelpText(long id)						{}
		virtual void SetMouseOver(short state)					{}
		virtual void SetMouseOverColor(COLORREF color)			{}
		virtual void SetMouseOverPerc(short perc)				{}
		virtual void SetCallback(void (*cb)(long,short,C_Base*)){}

// Querry Functions
		long  GetID()				{ return(ID_); }
		short GetType()				{ return(Type_); }
		short _GetCType_()			{ return(_CType_); }
		long  GetGroup()			{ return(Section_[_GROUP_]); }
		long  GetCluster()			{ return(Section_[_CLUSTER_]); }
		long  GetFlags()			{ return(Flags_); }
		short GetClient()			{ return(Client_); }
		long GetX()					{ return(x_); }
		long GetY()					{ return(y_); }
		long GetW()					{ return(w_); }
		long GetH()					{ return(h_); }
		long  GetUserNumber(long idx);
		void *GetUserPtr(long idx);
		virtual long GetRelX()		{ return(0); }
		virtual long GetRelY()		{ return(0); }
		C_Window *GetParent()		{ return(Parent_); }
		short  Ready()				{ return(Ready_); }
		virtual short GetState()	{ return(0); }
		virtual short GetHotKey()	{ return(0); }
		virtual long  GetMenu()		{ return(0); }
		virtual long  GetFont()		{ return(0); }
		virtual long  GetHelpText()	{ return(0); }
		virtual SOUND_RES *GetSound(short Type) { return(NULL); }
		virtual short GetMouseOver() { return(0); }
		virtual long  GetCursorID()	{ return(0); }
		virtual long  GetDragCursorID()	{ return(0); }
		virtual void (*GetCallback())(long,short,C_Base*) { return(NULL); }

// Other Functions
		virtual BOOL IsBase()							{ return(TRUE); }
		virtual BOOL IsControl()						{ return(FALSE); }
		virtual void Refresh()							{}
		virtual void Draw(SCREEN *surface,UI95_RECT *cliprect)			{}
		virtual void HighLite(SCREEN *surface,UI95_RECT *cliprect)		{}
		virtual void SetSubParents(C_Window *Parent)	{}
		virtual void Cleanup()							{}
		virtual BOOL TimerUpdate()						{ return(FALSE); }
		virtual void Activate()							{}
		virtual void Deactivate()						{}
		virtual long CheckHotSpots(long relx,long rely)	{ return(0); }
		virtual BOOL CheckKeyboard(uchar DKScanCode,uchar Ascii,uchar ShiftStates,long RepeatCount) { return(FALSE); }
		virtual BOOL Process(long ID,short HitType)		{ return(FALSE); }
		virtual BOOL CloseWindow()						{ return(FALSE); }
		virtual BOOL MouseOver(long relX,long relY,C_Base *me) { return(FALSE); }
		virtual C_Base *GetMe()							{ return(this); }
		virtual BOOL Dragable(long ID)					{ return(FALSE); }
		virtual void GetItemXY(long ID,long *x,long *y)	{}
		virtual BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over) { return(FALSE); }
		virtual BOOL Drop(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over) { return(FALSE); }
		/** sfr: wheel handler
		* @param increments +1 of positive movement, -1 for negative
		* @return TRUE if wheel is processed 
		*/
		virtual BOOL Wheel(int increments, WORD MouseX, WORD MouseY)            { return FALSE; }

// Parser Stuff
#ifdef _UI95_PARSER_
		virtual short LocalFind(char *str) { return(-1); } // Search local token list
		virtual void LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *Hndlr) {};
		virtual void SaveText(HANDLE ofp,C_Parser *Parser) {};

		short BaseFind(char *token);
		void BaseFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveTextCommon(HANDLE ofp,C_Parser *Parser,long DefFlags);
#endif // parser
};

#pragma warning (pop)

#endif
