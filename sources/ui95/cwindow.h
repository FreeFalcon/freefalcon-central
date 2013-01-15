#ifndef _PLAN_WINDOW_H_
#define _PLAN_WINDOW_H_

class C_Handler;
class C_Window;
class C_Base;
class C_Control;
class C_ScrollBar;

#ifdef _UI95_PARSER_
class C_Parser;
#endif

#define WIN_MAX_CLIENTS (8)
#define WIN_MAX_RECTS   (200)
#define WIN_HASH_SIZE	(50)

#define MAX_CURSORS (30)

//XX
class ImageBuffer;

class SCREEN
{
	public:
		WORD *mem;
		short width;
		short height;
//		short pitch;	// OW

		//XX
		BYTE bpp;
		ImageBuffer* owner;
};

#include "chash.h"
#include "cresmgr.h"
#include "cfontres.h"
#include "cfonts.h"
#include "cimagerc.h"
#include "csoundrc.h"
#include "canimrc.h"
#include "cstringrc.h"
#include "cmovie.h"
#include "csndbite.h"

// output classes
#include "ooutput.h"

F4CSECTIONHANDLE* UI_Enter(C_Window *win); // Must be defined prior to cwindow.h
void UI_Leave(F4CSECTIONHANDLE* section);

typedef struct
{
	long ID_;
	long StartX_,StartY_;
	long ItemX_,ItemY_;
//!	short GrabType_;
	long GrabType_;
	C_Window *Window_;
	C_Base *Control_;
} GRABBER;

typedef struct
{
	C_Base *Control_;
	long Time_;
	_TCHAR *Tip_;
	short MouseX_,MouseY_;

	short HelpOn_;
	long HelpFont_;
	UI95_RECT Area_;
} TOOL_TIP;

// control classed
#include "cbase.h" // BASE CLASS for ALL controls
#include "ccontrol.h" // Inherited stuff for Selectable controls (Anything the user can click on)
#include "canim.h"
#include "cbitmap.h"
#include "ctile.h"
#include "cbuttons.h"
#include "cblip.h"
#include "clistbox.h"
#include "ctree.h"
#include "cscroll.h"
#include "cpopup.h"
#include "ceditbox.h"
#include "ctext.h"
#include "cline.h"
#include "cbox.h"
#include "ccursor.h"
#include "cmarque.h"
#include "cslider.h"
#include "cpanner.h"
#include "cthook.h"
#include "cfill.h"
#include "cpopmgr.h"
#include "csclbmp.h"
#include "ccustom.h"
#include "cclock.h"
#include "chelp.h"

#ifdef _UI95_PARSER_
#include "cparser.h" // this file MUST follow ALL controls (which can be parsed) in order to work properly
#endif

typedef struct ControlListStr CONTROLLIST;

struct ControlListStr
{
	C_Base *Control_;
	CONTROLLIST *Prev,*Next;
};

class C_Window
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		enum
		{
			_CHR_CLIP_LEFT=0x01,
			_CHR_CLIP_RIGHT=0x02,
			_CHR_CLIP_TOP=0x04,
			_CHR_CLIP_BOTTOM=0x08,
		};

		// Save stuff starting here
		long		ID_;
		long		Flags_; // Flags for window
		long		Section_; // Used for sections of the game
		long		Group_; // Group ID
		long		Cluster_; // Cluster ID (Similar use as group)
		long		DefaultFlags_;
		long		MenuID_;
		long		RemoveImage_;
		long		CursorID_;
		long		MenuFlags_;
		long		ClientMenuID_[WIN_MAX_CLIENTS];
		long		ClientFlags_[WIN_MAX_CLIENTS];

	public:
		long		update_;
		long		Font_;
		UI95_RECT	Area_;

		COLORREF	BorderLite;	// Brightest color for border line
		COLORREF	BorderMedium;	// normal color
		COLORREF	BorderDark;	// Shadow color
		COLORREF	BGColor;		// Background of client area color
		COLORREF	SelectBG;		// Background color if selected
		COLORREF	NormalText;	// Normal Text Color
		COLORREF	ReverseText;	// Reverse color for text (when drawn over SelColor?)
		COLORREF	DisabledText;	// Normal Text Color
		COLORREF	TextColor_;
		COLORREF	BgColor_;

		// Client Areas (upto WIN_MAX_CLIENTS (8) supported)
		UI95_RECT	ClientArea_[WIN_MAX_CLIENTS];
		UI95_RECT	FullClientArea_[WIN_MAX_CLIENTS]; // Used to restore client area when scrollbars are used


	protected:
		short		Type_; // 0=Unknown,1=Window,2=Toolbar...
		short		Depth_; // used to determine which windows are in front of which
		short		x_,y_,w_,h_; // User Setable
		short		MinX_,MinY_,MaxX_,MaxY_; // Min/Max values of WindowX,WindowY
		short		MinW_,MinH_,MaxW_,MaxH_;
		short		DragH_; // Used to determine whether we can drag this window or not

		// Don't save from here down
//XX
		//WORD		r_mask_,r_shift_,r_max_; // AND flag,shift values to convert 16bit RGB to usable value
		//WORD		g_mask_,g_shift_,g_max_;
		//WORD		b_mask_,b_shift_,b_max_;

		DWORD		r_mask_; 
		WORD		r_shift_,r_max_; // AND flag,shift values to convert 16bit RGB to usable value
		DWORD		g_mask_;
		WORD		g_shift_,g_max_;
		DWORD		b_mask_;
		WORD		b_shift_,b_max_;

	public:
		long		VX_[WIN_MAX_CLIENTS],VY_[WIN_MAX_CLIENTS]; // x,y relative to scrollbar (Use for drawing EVERYTHING in client area)
		long		VW_[WIN_MAX_CLIENTS],VH_[WIN_MAX_CLIENTS]; // w,h relative to scrollbar

	private:
		short		Width_,Height_; // DDraw surface w/h
		short		FontHeight_;
		short		ControlCount_;
	public:
		short		rectflag_[WIN_MAX_RECTS];
		short		rectcount_;
		UI95_RECT	rectlist_[WIN_MAX_RECTS];

	private:
		POINT		Cursor_;
		C_ScrollBar *VScroll_[WIN_MAX_CLIENTS],*HScroll_[WIN_MAX_CLIENTS];
		ImageBuffer *imgBuf_;
		C_Base		*Owner_;
		C_Hash		*Hash_;
		CONTROLLIST *Controls_;
		CONTROLLIST *Last_;
		C_Base		*CurControl_;
		void (*DragCallback_)(C_Window *win);
		F4CSECTIONHANDLE* Critical;
		BOOL (*KBCallback_)(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount);

		CONTROLLIST *FindControlInList(C_Base *cntrl);
		void GetScreenFormat();
		long SetCheckedUpdateRect(long x1,long y1,long x2,long y2);

		void Fill(SCREEN *surface,WORD Color,UI95_RECT *rect);

	public:
		C_Handler	*Handler_; // Pointer to Handler class

		C_Window();
		C_Window(char **)	{ ; }
		C_Window(FILE *)	{ ; }
		~C_Window()			{ ; }
		long Size()			{ return 0;	}
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void SetCritical(F4CSECTIONHANDLE* section) { Critical=section; }
		F4CSECTIONHANDLE* GetCritical() { return(Critical); }

		// Setup Functions
		void Setup(long wID,short Type,short w,short h);
		void ResizeSurface(short w,short h);
		void SetType(short Type) { Type_=Type; }
		void SetX(short x);
		void SetY(short y);
		void SetXY(short x,short y);
		void SetW(short w);
		void SetH(short h);
		void SetRanges(short x1,short y1,short x2,short y2,short w,short h);
		void SetDepth(short newdepth) { Depth_=newdepth; }
		void SetDragH(short h) { DragH_=h; }
		short GetDepth() { return(Depth_); }
		short GetDragH() { return(DragH_); }
		void ScanClientArea(long client);
		void ScanClientAreas();
		void SetClientArea(UI95_RECT *rect,short ID) { if(ID >= WIN_MAX_CLIENTS) return; ClientArea_[ID]=*rect; VX_[ID]=ClientArea_[ID].left; VY_[ID]=ClientArea_[ID].top;}
		void SetClientArea(long x,long y,long w,long h,short ID) { if(ID >= WIN_MAX_CLIENTS) return; ClientArea_[ID].left=x; ClientArea_[ID].top=y; ClientArea_[ID].right=x+w; ClientArea_[ID].bottom=y+h; VX_[ID]=x; VY_[ID]=y; }
		void SetVirtual(long x,long y,long w,long h,short ID) { if(ID >= WIN_MAX_CLIENTS) return; VX_[ID]=-x;VY_[ID]=-y;VW_[ID]=w;VH_[ID]=h;}
		void SetVirtualX(long x,long Client) { if(Client >= WIN_MAX_CLIENTS) return; VX_[Client]=-x;}
		void SetVirtualY(long y,long Client) { if(Client >= WIN_MAX_CLIENTS) return; VY_[Client]=-y;}
		void SetVirtualW(long w,long Client) { if(Client >= WIN_MAX_CLIENTS) return; VW_[Client]=w;}
		void SetVirtualH(long h,long Client) { if(Client >= WIN_MAX_CLIENTS) return; VH_[Client]=h;}
		void SetFlags(long flag);
		void DeactivateControl();
		void Activate();
		void Deactivate();
		void SetCursorID(long ID) { if(ID < MAX_CURSORS) CursorID_=ID; }
		long GetCursorID() { return(CursorID_); }
		long GetFlags() { return(Flags_); }
		void SetFlagBitOn(long flag);
		void SetFlagBitOff(long flag);
		void SetBgColor(COLORREF color) { BgColor_=color; SetFlagBitOn(C_BIT_USEBGFILL); }
		void SetMenu(long ID) { MenuID_=ID; }
		void SetClientMenu(long Client,long ID) { if(Client < WIN_MAX_CLIENTS) ClientMenuID_[Client]=ID; }
		void SetDragCallback(void (*cb)(C_Window *)) { DragCallback_=cb; }
		long GetMenu() { return(MenuID_); }
		long GetClientMenu(long Client) { if(Client < WIN_MAX_CLIENTS) return(ClientMenuID_[Client]); return(0); }
		void SetClientFlags(long Client,long flags) {  if(Client < WIN_MAX_CLIENTS) ClientFlags_[Client]=flags; }
		long GetClientFlags(long Client) {  if(Client < WIN_MAX_CLIENTS) return(ClientFlags_[Client]); return(0); }
		void AdjustScrollbar(long client);
		void AddScrollBar(C_ScrollBar *scroll);
		void AddControl(C_Base *NewButton);
		void AddControlTop(C_Base *NewButton);
		void RemoveControl(long ControlID);
		CONTROLLIST *RemoveControl(CONTROLLIST *ctrl);
		void RemoveAllControls();
		BOOL SetFont(long ID) { Font_=ID; return(TRUE); }
		void SetHandler(C_Handler *handler) {Handler_=handler;}
		void SetKBCallback(BOOL (*cb)(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount)) { KBCallback_=cb; }
		CONTROLLIST *GetControlList() { return(Controls_); }
		void SetOwner(C_Base *ctrl) { Owner_=ctrl; }
		C_Base *GetOwner() { return(Owner_); }

		// Keyboard Support Routines
		BOOL CheckKeyboard(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount); // Called whenever a key is pressed
		BOOL CheckHotKeys(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount); // Called whenever a key is pressed & CheckKeyboard returned FALSE
		void SetControl(long ID); // Called when mouse is used over this control
		void SetPrevControl(); // Called when SHIFT & TAB are pressed
		void SetNextControl(); // Called when TAB is pressed

		// Cleanup Functions
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		// Query Functions
		long GetID() { return(ID_); }
		void AddUpdateRect(long x1,long y1,long x2,long y2);
		void SetUpdateRect(long x1,long y1,long x2,long y2,long flags,long client);
		void ClearCheckedUpdateRect(long x1,long y1,long x2,long y2);
		void ClearUpdateRect(long x1,long y1,long x2,long y2);
		void SetSection(long sctn) { Section_=sctn; }
		void SetGroup(long grp) { Group_=grp; }
		void SetCluster(long clst) { Cluster_=clst; }
		short GetX() { return(x_); }
		short GetY() { return(y_); }
		short GetW() { return(w_); }
		short GetH() { return(h_); }
		short GetType() { return(Type_);}
		short GetPrimaryW();
		long GetSection(void) { return(Section_);}
		long GetGroup(void) { return(Group_);}
		long GetCluster(void) { return(Cluster_);}
		UI95_RECT GetClientArea(long ID) { if(ID < WIN_MAX_CLIENTS) return ClientArea_[ID]; return(ClientArea_[0]); }
		BOOL Minimized() { if(w_ == MinW_ && h_ == MinH_) return(TRUE); return(FALSE); }
		void Minimize();
		void Maximize();
		C_Handler *GetHandler(void) { return(Handler_);}
		BOOL ClipToArea(UI95_RECT *src,UI95_RECT *dst,UI95_RECT *ClipArea);
		BOOL InsideClientWidth(long left,long right,long Client);
		BOOL InsideClientHeight(long top,long bottom,long Client);
		BOOL BelowClient(long y,long Client);
		void SetMenuFlags(long flag) { MenuFlags_=flag; }
		long IsMenu() { return(MenuFlags_); }

		void EnableGroup(long ID);
		void DisableGroup(long ID);
		void UnHideGroup(long ID);
		void HideGroup(long ID);
		void EnableCluster(long ID);
		void DisableCluster(long ID);
		void UnHideCluster(long ID);
		void HideCluster(long ID);
		void SetGroupState(long GroupID,short state);

		void GetRGBValues(DWORD &rm,WORD &rs, DWORD &gm,WORD &gs, DWORD &bm,WORD &bs) 
		{ 
			rm=r_mask_; rs=r_shift_; gm=g_mask_; gs=g_shift_; bm=b_mask_; bs=b_shift_; 
		}

		// Internally supported Drawing functions
		void RefreshWindow();
		void RefreshClient(long Client);
		void DrawWindow(SCREEN *surface);
		BOOL UpdateTimerControls();
		void DrawTimerControls();
		void TextColor(COLORREF color) { TextColor_=color; }
		void TextBkColor(COLORREF color) { BgColor_=color; }
		void Blend(WORD *front,UI95_RECT *frect,short fwidth,WORD *back,UI95_RECT *brect,short bwidth,WORD *dest,UI95_RECT *drect,short dwidth,short fperc,short bperc);
		void BlendTransparent(WORD Mask,WORD *front,UI95_RECT *frect,short fwidth,WORD *back,UI95_RECT *brect,short bwidth,WORD *dest,UI95_RECT *drect,short dwidth,short fperc,short bperc);
		void Translucency(WORD *front,UI95_RECT *frect,short fwidth,WORD *dest,UI95_RECT *drect,short dwidth);
		void BlitTranslucent(SCREEN *surface,COLORREF color,long Perc,UI95_RECT *rect,long Flags,long client);
		void CustomBlitTranslucent(SCREEN *surface,COLORREF color,long Perc,UI95_RECT *rect,long Flags,long Client);
		void DitherFill(SCREEN *surface,COLORREF color,long perc,short size,char *pattern,UI95_RECT *rect,long Flags,long client);
		void GradientFill(SCREEN *surface,COLORREF Color,long Perc,UI95_RECT *dst,long Flags,long Client);
		void BlitFill(SCREEN *surface,COLORREF Color,UI95_RECT *dst,long Flags,long Client);
		void BlitFill(SCREEN *surface,COLORREF Color,long x,long y,long w,long h,long Flags,long Client,UI95_RECT *clip);
		void DrawHLine(SCREEN *surface,COLORREF color,long x,long y,long w,long Flags,long Client,UI95_RECT *clip);
		void DrawVLine(SCREEN *surface,COLORREF color,long x,long y,long h,long Flags,long Client,UI95_RECT *clip);
		BOOL CheckLine(long x1,long y1,long x2,long y2,long minx,long miny,long maxx,long maxy);
		void DrawLine(SCREEN *surface,COLORREF color,long x1,long y1,long x2,long y2,long Flags,long Client,UI95_RECT *clip);
		BOOL ClipLine(long *x1,long *y1,long *x2,long *y2,UI95_RECT *clip);
		void DrawClipLine(SCREEN *surface,long x1,long y1,long x2,long y2,UI95_RECT *clip,WORD color);
		void DrawCircle(SCREEN *surface,COLORREF color,long x,long y,float radius,long Flags,long Client,UI95_RECT *clip);
		void DrawArc(SCREEN *surface,COLORREF color,long x,long y,float radius,short section,long Flags,long Client,UI95_RECT *clip);
		void ClearWindow(SCREEN *surface,long Client);
		void ClearArea(SCREEN *surface,long x1,long y1,long w,long h,long flags,long Client);
		ImageBuffer *GetSurface() { return(imgBuf_);}
		void SetSurface(ImageBuffer *newsurface) { imgBuf_=newsurface; }
		BOOL KeyboardMode();

		void RemovingControl(C_Base *control);
		// Handler Functions
		C_Base *FindControl(long ID);
		C_Base *GetControl(long *ID,long relX,long relY);
		C_Base *MouseOver(long relX,long relY,C_Base *last);
		BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over);
		BOOL Drop(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over);
		// sfr: corrects window offscreen coordinates
		void ConstraintsCorrection(long w, long h);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveTextControls(HANDLE ofp,C_Parser *Parser);
		void SaveText(HANDLE ,C_Parser *)	{ ; }

#endif // PARSER
};

extern HCURSOR gCursors[MAX_CURSORS];

#endif
