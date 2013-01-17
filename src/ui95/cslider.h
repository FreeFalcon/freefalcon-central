#ifndef _SLIDER_H_
#define _SLIDER_H_

class C_Slider : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;

		long BGX_,BGY_;
		long SX_,SY_; // Changes on the fly

		long MinPos_,MaxPos_;
		long Steps_;

		IMAGE_RSC *BgImage_; // draw at x,y of control
		IMAGE_RSC *Slider_;

	public:
		C_Slider();
		C_Slider(char **stream);
		C_Slider(FILE *fp);
		~C_Slider();
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
		void SetSliderImage(long SliderID);
		void SetSliderRange(const long Min,const long Max);
		long CheckHotSpots(long relX,long relY); // returns ID of button hit
		BOOL Process(long ID,short ButtonHitType);
		// sfr: mouse wheel 
		BOOL Wheel(int increments, WORD MouseX, WORD MouseY);
		BOOL Dragable(long)		{	return TRUE;	}
		void SetSteps(short num) { Steps_=num; }
		void Refresh();
		long GetSteps() { return(Steps_); }
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetSliderPos(long Pos);
		long GetSliderMax() { return(MaxPos_); }
		long GetSliderMin() { return(MinPos_); }
		long GetSliderPos() { if(GetType() == C_TYPE_VERTICAL) return(SY_); return(SX_); }
		BOOL MouseOver(long relX,long relY,C_Base *);
		void HighLite(SCREEN *surface,UI95_RECT *cliprect);
		void GetItemXY(long,long *x,long *y);
		BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *)	{ return FALSE; }
		void SetSubParents(C_Window *);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE,C_Parser *) { ; }

#endif // PARSER
};

#endif
