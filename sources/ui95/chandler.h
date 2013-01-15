#ifndef _WINDOW_HANDLER_H_
#define _WINDOW_HANDLER_H_

#define _UI95_PARSER_ // for Layout ONLY

#define HND_MAX_RECTS (200)

class C_Handler;

#include <tchar.h>
#include <string.h>
#include <float.h>
#include <stdlib.h>
//#include <ddraw.h>
#define USE_DINPUT_8
#ifndef USE_DINPUT_8	// Retro 15Jan2004
#define DIRECTINPUT_VERSION 0x0700
#else
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include "f4error.h"
#include "f4thread.h"
#include "Graphics\Include\imagebuf.h"
#include "fsound.h"
#include "ui95_dd.h"
#include "ui95defs.h" // BIG Enum for all internal IDs
#include "IsBad.h"

#ifdef USE_SH_POOLS
extern MEM_POOL UI_Pools[UI_MAX_POOLS];
#endif

#include "cwindow.h"
#include "ui\include\falcuser.h"
#include "stdlib.h"
#include "debuggr.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\shmalloc.h"
#include "SmartHeap\Include\smrtheap.hpp"
#endif

#define _SHIFT_DOWN_	(0x01)
#define _CTRL_DOWN_		(0x02)
#define _ALT_DOWN_		(0x04)


extern unsigned char _Keys_[][2];

extern long _LOAD_ART_RESOURCES_;

class WHLIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		C_Window *win;
		long Flags;
		void (*DrawAfter)(long ImageID);
		WHLIST *Next,*Prev;
};

class CBLIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		void (*Callback)();
		CBLIST *Next;
};

class C_Handler
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		WHLIST *Root_; // Root pointer to windows in handler
		CBLIST *UserRoot_; // Root pointer to windows in handler
		HWND AppWindow_;
		ImageBuffer *Primary_; // primary surface pointer
		ImageBuffer *Front_; // Surface to blit to
		SCREEN surface_;
		RECT FrontRect_;
//		RECT PrimaryRect_;
		volatile long TimerLoop_,ControlLoop_,OutputLoop_;
		long OutputWait_;
		unsigned TimerID_;
		unsigned OutputID_;
		unsigned ControlID_;
		HANDLE TimerThread_;
		HANDLE OutputThread_;
		HANDLE ControlThread_;
		HANDLE WakeOutput_;
		HANDLE WakeControl_;
		F4CSECTIONHANDLE* UI_Critical;

		BOOL (*MouseCallback_)(C_Base *,WORD,WORD,C_Window *,short);
		long TimerSleep_;
		long ControlSleep_;
		BOOL KeyboardMode_;
		long EnabledTime_;
		long CurrentSection_;
		C_Base *OverControl_;

		TOOL_TIP OverLast_;

// Grab control (item found in CheckHotSpots)
		GRABBER		Grab_;

// Drag stuff
		GRABBER		Drag_;

// Current Window
		C_Window	*CurWindow_;

// Last Message was MouseDown (will be either L or R mouse)... 0 if not MouseDown... used for repeating
		long		MouseDown_;
		long		MouseDownTime_;
		long		LastUp_;
		long		LastUpTime_;
		long		DoubleClickTime_;
		C_Base		*MouseControl_;

//		DDSURFACEDESC ScreenFormat;

		UI95_RECT rectlist_[HND_MAX_RECTS];
		short rectcount_;

		long HandlingMessage;
		void PostTimerMessage();
		void DoControlLoop();
		static unsigned int __stdcall TimerLoop(void *myself);
		static unsigned int __stdcall OutputLoop(void *myself);
		static unsigned int __stdcall ControlLoop(void *myself);
		void DoOutputLoop();
		BOOL OldInputMessage();
		void HelpOff();
		void CheckHelpText(SCREEN *surface);
		void Fill(SCREEN *surface,COLORREF Color,long x1,long y1,long x2,long y2); //!
		void Fill(SCREEN *surface,COLORREF Color,UI95_RECT *dst);

	public:
		long UpdateFlag;
		long DrawFlags;

		C_Handler();
		~C_Handler();

		// Setup Functions
		// if you want to update immediately... set Work == Primary
		void EnterCritical();
		void LeaveCritical();
		void Setup(HWND hwnd,ImageBuffer *,ImageBuffer *Primary);   // Initialize pointers
		BOOL AddWindow(C_Window *thewin,long Flags);     // Add a window to the Handler's list
		void StartOutputThread();
		void EndOutputThread();
		void SuspendOutput();
		void ResumeOutput();
		void StartControlThread(long Interval);
		void EndControlThread();
		void SuspendControl();
		void ResumeControl();
		void StartTimerThread(long Interval);
		void EndTimerThread();
		void SuspendTimer();
		void ResumeTimer();
		void SetSection(long ID) { CurrentSection_=ID; }
		long GetSection() { return(CurrentSection_); }
		F4CSECTIONHANDLE* GetCritical() { return(UI_Critical); }
		BOOL AddUserCallback(void (*cb)());
		BOOL RemoveUserCallback(void (*cb)());
		void ProcessUserCallbacks();
		BOOL ShowWindow(C_Window *thewin); // Make window display (if disabled)
		BOOL HideWindow(C_Window *thewin); // Hide window (if enabled)
		BOOL RemoveWindow(C_Window *thewin); // Remove a window from handler's list
		void SetBehindWindow(C_Window *thewin);
		void SetCallback(BOOL (*Callback)(C_Base *,WORD,WORD,C_Window *,short)) { MouseCallback_=Callback; }
		void SetKeyboardMode(BOOL mode) { KeyboardMode_=mode; }
		BOOL KeyboardMode() { return(KeyboardMode_); }
		void SetOutputDelay(long delay=80) { OutputWait_=delay; }
		void SetControlDelay(long delay=80) { ControlSleep_=delay; }
		// Cleanup Functions
		void Cleanup(void); // cleanup mess left when done
		void ReleaseControl(C_Base *control);

		void DisableSection(long ID); // Used for closing windows opened in a "section of the game"

		void EnableGroup(long ID);
		void DisableGroup(long ID);
		void EnableWindowGroup(long ID);
		void DisableWindowGroup(long ID);

		void SetEnableTime(long atime) { EnabledTime_=atime; }

		BOOL CheckHotKeys(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long RepeatCount);

		// Mostly Useless querry functions
		short GetX() { return(0);}
		short GetY() { return(0);}
		long  GetW() { return(FrontRect_.right-FrontRect_.left);}
		long  GetH() { return(FrontRect_.bottom-FrontRect_.top);}

//		long Busy() { return(HandlingMessage); }

		void SetDrawFlag(long val) { DrawFlags=val; }
		long GetDrawFlag() { return(DrawFlags); }

		void WindowToFront(C_Window *thewin); // move a window to end of handler's list
		long EventHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); // process events
		void SetUpdateRect(UI95_RECT *upd);
		void ClearHiddenRects(WHLIST *behind);
		void CheckDrawThrough();
		void ClearAllHiddenRects();
		BOOL ClipRect(UI95_RECT *src,UI95_RECT *dst,UI95_RECT *ClientArea);
		void RefreshAll(UI95_RECT *updaterect); // Tell ALL visible windows to redraw everything
		void Update(); // BLIT drawn areas to background surface (OR primary if set that way)
		void CopyToPrimary();
		void UpdateTimerControls(void);
		void CheckTranslucentWindows();
		C_Base *Over() { return(MouseControl_); }
		long UpdateWaiting() { return(UpdateFlag & C_DRAW_UPDATE); }
		void PostUpdate();
		void SendUpdate();
		long GetWindowFlags(long ID); // find a window by its ID
		C_Window *GetWindow(short x,short y); // get window mouse is over
		C_Window *FindWindow(long ID); // find a window by its ID
		C_Window *_GetFirstWindow(); // Get the First Window
		C_Window *_GetNextWindow(C_Window *win); // find the window following win
		ImageBuffer *GetFront() { return(Front_); }
		ImageBuffer *GetPrimary() { return(Primary_); }
		HWND GetAppWnd() { return(AppWindow_);}
		void BlitWindowNow(C_Window *win);
		void *Lock();
		void Unlock();

		void RemovingControl(C_Base *control);
		void DropControl();
		void StartDrag();
//!		short GetDragX(WORD MouseX);
//!		short GetDragY(WORD MouseY);
		long GetDragX(WORD MouseX);
		long GetDragY(WORD MouseY);
		BOOL Dragging(void) { if(Drag_.Control_ != NULL || Drag_.Window_ != NULL) return(TRUE); return(FALSE);}
		BOOL GrabItem(WORD MouseX,WORD MouseY,C_Window *overme,long GrabType);
//!		BOOL GrabItem(WORD MouseX,WORD MouseY,C_Window *overme,short GrabType);
		BOOL DragItem(WORD MouseX,WORD MouseY,C_Window *overme);
		BOOL DropItem(WORD MouseX,WORD MouseY,C_Window *overme);
};
#endif
