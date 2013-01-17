#ifndef _PANNER_H_
#define _PANNER_H_

#define PAN_MAX_IMAGES 10

class C_Panner : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from Here
		long DefaultFlags_;

		short DeadW_,DeadH_;
		short DeadX_,DeadY_;

		// Don't save from here down
		long state_;
		long SX_,SY_; // Changes on the fly
		O_Output *BgImage_; // draw at x,y of control
		O_Output *Image_[PAN_MAX_IMAGES];

	public:
		C_Panner();
		C_Panner(char **stream);
		C_Panner(FILE *fp);
		~C_Panner();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		// Setup Functions
		void Setup(long ID,short Type);
		void Cleanup();

		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		// if using bitmaps for all
		void SetBgImage(long ImageID);
		void SetImage(short state,long ImageID);
		BOOL TimerUpdate() { return(FALSE); }
		long CheckHotSpots(long relX,long relY); // returns ID of button hit
		BOOL Process(long,short ButtonHitType);
		BOOL Dragable(long)	{ return FALSE; }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetDeadZone(short x,short y,short w,short h) { DeadX_=x; DeadY_=y; DeadW_=w; DeadH_=h; }
		short GetDeadZone() { return(0); }
		short GetState() { return static_cast<short>(state_); }//! 
		long GetHRange() { return(SX_); }	  //! 
		long GetVRange() { return(SY_); }	  //! 
		void GetItemXY(long,long *x,long *y);
		BOOL Drag(GRABBER *,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *);
		void SetSubParents(C_Window *);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *) { ; }

#endif // PARSER
};

#endif
