#ifndef _C_FORCE_H_
#define _C_FORCE_H_

#define _LEVEL_MAX_TEAMS_ (8)

class LEVEL
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		short value;
		short y;
		LEVEL *Next;
};

class C_Level : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		UI95_RECT  DrawArea_;
		long  MinValue_,MaxValue_;
		short Count_;
		long  Start_,End_;

		_TCHAR Y_Labels_[4][20];

		COLORREF Color_[_LEVEL_MAX_TEAMS_];

		LEVEL *Root_[_LEVEL_MAX_TEAMS_];

	public:
		C_Level();
		C_Level(char **)	{ ; }
		C_Level(FILE *)		{ ; }
		~C_Level();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short type);
		void Cleanup();

		void SetStart(long val) { Start_=val; }
		void SetEnd(long val) { End_=val; }

		long GetStart() { return(Start_); }
		long GetEnd() { return(End_); }

		void SetDrawArea(UI95_RECT rect) { DrawArea_=rect; }
		void SetDrawArea(short x,short y,short w,short h) { DrawArea_.left=x; DrawArea_.top=y; DrawArea_.right=x+w; DrawArea_.bottom=y+h; }

		void SetTeamColor(short team,COLORREF color) { if(team < _LEVEL_MAX_TEAMS_) Color_[team]=color; }

		void AddPoint(short team,short value);
		void CalcPositions();

		void SetMinValue(long val) { MinValue_=val; }
		void SetMaxValue(long val) { MaxValue_=val; }

		long GetMinValue() { return(MinValue_); }
		long GetMaxValue() { return(MaxValue_); }

		void SetYLabel(short idx,_TCHAR *lbl) { if(idx < 4) _tcscpy(Y_Labels_[idx],lbl); }

		_TCHAR *GetYLabel(short idx) { if(idx < 4) return(Y_Labels_[idx]); return(NULL); }

		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};



#endif
