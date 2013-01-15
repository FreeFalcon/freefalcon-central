#ifndef _OUTPUT_TYPES_H_
#define _OUTPUT_TYPES_H_

class WORDWRAP
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		short Index;
		short Length;
		short y;
};

class O_Output
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_GENERAL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	protected:
		// Stuff that can be saved
		long		flags_;
		long		Font_;
		long		ScaleSet_;
		COLORREF	FgColor_,BgColor_;
		UI95_RECT	Src_,Dest_;
		long		origx_,origy_;
		long		x_,y_; // relative x,y (offset from the Draw(x,y,...) parameters
		long		w_,h_; // relative w,h
		long		lastx_,lasty_,lastw_,lasth_;
		short		_OType_;
		short		animtype_;
		short		frame_;
		short		direction_;
		short		ready_;
		short		fperc_,bperc_;
		short		LabelLen_;
		short		WWWidth_;
		short		WWCount_;
		short		OpStart_,OpEnd_;

	// Don't save this stuff
		long		*Rows_;
		long		*Cols_;
		WORDWRAP	*Wrap_;
		_TCHAR		*Label_;
		IMAGE_RSC	*Image_;
		ANIM_RES	*Anim_;
		C_Base		*Owner_; // pointer to creator control

		void O_Output::ExtractAnim(SCREEN *surface,long FrameNo,long x,long y,UI95_RECT *src,UI95_RECT *dest);
		void O_Output::Extract16BitRLE(SCREEN *surface,long FrameNo,long x,long y,UI95_RECT *src,UI95_RECT *dest);
		void O_Output::Extract16Bit(SCREEN *,long ,long ,long ,UI95_RECT *,UI95_RECT *);

		int O_Output::FitString(int idx); // returns # characters to keep on this line
		void O_Output::WordWrap(); // handles the word wrapping stuff

	public:

		enum
		{
			_OUT_TEXT_=100,
			_OUT_BITMAP_,
			_OUT_SCALEBITMAP_,
			_OUT_ANIM_,
			_OUT_FILL_,
		};

		O_Output()
		{
			origx_=0,origy_=0;
			x_=0,y_=0;
			w_=0,h_=0;
			lastx_=0,lasty_=0;
			lastw_=0,lasth_=0;
			_OType_=0;
			flags_=0;
			animtype_=0;
			frame_=0;
			direction_=0;
			ready_=0;
			fperc_=100,bperc_=0;
			FgColor_=0xcccccc,BgColor_=0;
			Font_=1;
			LabelLen_=0;
			Src_.left=0; Src_.top=0; Src_.right=0; Src_.bottom=0;
			Dest_.left=0; Dest_.top=0; Dest_.right=0; Dest_.bottom=0;
			Rows_=NULL;
			Cols_=NULL;
			ScaleSet_=1;
			OpStart_=0;
			OpEnd_=10000;
			WWWidth_=0;
			WWCount_=0;
			Wrap_=NULL;
			Label_=NULL;
			Image_=NULL;
			Anim_=NULL;
			Owner_=NULL;
		}

		O_Output(char **stream);
		O_Output(FILE *fp);
		~O_Output() {}
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);

// Setting Functions
		void _SetOType_(short type)				{ _OType_=type; } // Private...Called by setup

		void SetXY(long x,long y)				{ origx_ = x; origy_ = y; }
		void SetX(long x)						{ origx_ = x; }
		void SetY(long y)						{ origy_ = y; }
		void SetWH(long w,long h)				{ w_ = w; h_ = h; }
		void SetW(long w)						{ w_ = w; }
		void SetH(long h)						{ h_ = h; }
		void SetOwner(C_Base *owner)			{ Owner_ = owner; SetInfo(); }
		void SetReady(short val)				{ ready_ = val; }
		void SetFlags(long flags)				{ flags_=flags; }
		void SetFont(long FID)					{ Font_=FID; }
		void SetFgColor(COLORREF color)			{ FgColor_=color; }
		void SetBgColor(COLORREF color)			{ BgColor_=color; }
		void SetFrame(long frame)				{ frame_=(short)frame; }		//! 
		void SetDirection(long dir)				{ direction_=(short)dir; }		//! 
		void SetAnimType(long type)				{ animtype_=(short)type; }		//! 
		void SetSrcRect(UI95_RECT *rect)		{ Src_=*rect; }
		void SetDestRect(UI95_RECT *rect)		{ Dest_=*rect; }
		void SetScaleInfo(long scale);
		void SetFrontPerc(long perc)			{ fperc_=(short)perc; }			//! 
		void SetBackPerc(long perc)				{ bperc_=(short)perc; }			//! 
		void SetOpaqueRange(short os,short oe)	{ OpStart_=os; OpEnd_=oe; }

		void SetWordWrapWidth(long w)			{ WWWidth_=(short)w; }			//! 

// Query Functions
		short _GetOType_()				{ return(_OType_); }

		long GetX()						{ return(x_); }
		long GetY()						{ return(y_); }
		long GetW()						{ return(w_); }
		long GetH()						{ return(h_); }
		short Ready()					{ return(ready_); }
		long GetFlags()					{ return(flags_); }
		_TCHAR *GetText()				{ return(Label_); }
		short GetTextBufferLen()		{ return(LabelLen_); }
		IMAGE_RSC *GetImage()			{ return(Image_); }
		ANIM_RES *GetAnim()				{ return(Anim_); }
		COLORREF GetFgColor()			{ return(FgColor_); }
		COLORREF GetBgColor()			{ return(BgColor_); }
		short GetFrame()				{ return(frame_); }
		short GetDirection()			{ return(direction_); }
		short GetAnimType()				{ return(animtype_); }
		C_Base *Owner()					{ return(Owner_); }
		UI95_RECT *GetSrcRect()			{ return(&Src_); }
		UI95_RECT *GetDestRect()		{ return(&Dest_); }

// Non Inline functions
		long GetCursorPos(long relx,long rely); // Based on mouse location
		void GetCharXY(short idx,long *x,long *y); // Based on cursor location
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void Blend4Bit(SCREEN *surface,BYTE *overlay,WORD *Palette[],UI95_RECT *cliprect);
		void Cleanup();
		void SetInfo();
		void SetText(_TCHAR *str);
		void SetTextWidth(long w);
		void SetImage(long ID);
		void SetImage(IMAGE_RSC *img);
		void SetImagePtr(IMAGE_RSC *img);
		void SetScaleImage(long ID);
		void SetScaleImage(IMAGE_RSC *img);
		void SetAnim(long ID);
		void SetAnim(ANIM_RES *anim);
		void SetFill();
};

#endif // _OUTPUT_TYPES_H_
