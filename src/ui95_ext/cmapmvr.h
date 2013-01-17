#ifndef _MAP_MOVER_H_
#define _MAP_MOVER_H_

class C_MapMover : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;
		long SX_,SY_; // Direction being dragged
		long Draging_;

		void (*DrawCallback_)(long,short,C_Base *);

	public:
		C_MapMover();
		C_MapMover(char **stream);
		C_MapMover(FILE *fp);
		~C_MapMover();
		long Size();
		void Save(char **)	  { ; }
		void Save(FILE *)	  { ; }

		// Setup Functions
		void Setup(long ID,short Type);
		void Cleanup();

		long GetDraging() { return(Draging_); }
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		BOOL TimerUpdate() { return(FALSE); }
		void SetDrawCallback(void (*routine)(long,short,C_Base *)) { DrawCallback_=routine; }
		long CheckHotSpots(long relX,long relY); // returns ID of button hit
		BOOL Process(long,short ButtonHitType);
		BOOL Dragable(long)	{ return TRUE; }
		void Refresh();
		void Draw(SCREEN *,UI95_RECT *);
		long GetHRange() { return(SX_); }
		long GetVRange() { return(SY_); }
		void GetItemXY(long,long *x,long *y);
		BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *)	{ return FALSE; }
		// sfr: mouse wheel
		BOOL Wheel(int increment, WORD MouseX, WORD MouseY);

};

#endif
