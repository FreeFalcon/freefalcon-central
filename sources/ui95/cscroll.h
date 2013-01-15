#ifndef _SCROLL_BAR_H_
#define _SCROLL_BAR_H_

#define HORIZONTAL_SCROLLBAR 1
#define VERTICAL_SCROLLBAR   2
#define SCROLL_MINUS 5000
#define SCROLL_PLUS  5001

class C_ScrollBar : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;
		long BGX_,BGY_,BGW_,BGH_;
		long VirtualW_,VirtualH_;
		long SX_,SY_; // Changes on the fly
		COLORREF lite_,medium_,dark_;
		COLORREF LineColor_;
		long MinPos_,Distance_,MaxPos_;
		float Range_;

		UI95_RECT SliderRect_;
		short ControlPressed_; // 1=Bar on Minus side of slider,2=Bar on Plus side of slider,3=Slider,4=Minus,5=Plus
		O_Output *BgImage_;

		void CalcRanges();

	public:
		C_Button *Plus_,*Minus_;
		IMAGE_RSC *Slider_;

		C_ScrollBar();
		C_ScrollBar(char **stream);
		C_ScrollBar(FILE *fp);
		~C_ScrollBar();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		// Setup Functions
		void Setup(long ID,short Type);
		void Cleanup();

		void SetDistance(int dist) { Distance_=(short)dist; }//! 
		//sfr: i wonder why dist is cast to short...
		long GetDistance() { return Distance_; }
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		// if using bitmaps for all
		void SetBgImage(long ImageID);
		void SetSliderImage(long SliderID);
		// if you want ME to draw everything
		// note: Background bar will be the size of the x,y,w,h of the control minus the Plus/Minus buton widths/heights
		void SetSliderRect(UI95_RECT slider) { SliderRect_=slider; SetReady(1);}
		void SetSliderRect(short x,short y,short w,short h) { SliderRect_.left=x; SliderRect_.top=y; SliderRect_.right=x+w; SliderRect_.bottom=y+h; SetReady(1);}
		void SetColors(COLORREF lite,COLORREF medium,COLORREF dark);
		void ClearVW() { VirtualW_=-1; }
		void ClearVH() { VirtualH_=-1; }
		void ClearVWH() { VirtualW_=-1; VirtualH_=-1; }
		void SetVirtualW(long w) { if(-w < VirtualW_) VirtualW_=-w; CalcRanges(); }
		void SetVirtualH(long h) { if(-h < VirtualH_) VirtualH_=-h; CalcRanges(); }
		void SetLineColor(COLORREF color) { LineColor_=color; }
		void UpdatePosition(); 
		void SetButtonImages(long MinusUp,long MinusDown,long PlusUp,long PlusDown);
		long CheckHotSpots(long relX,long relY); // returns ID of button hit
		BOOL Process(long,short ButtonHitType);
		// sfr: added mouse wheel
		BOOL Wheel(int increments, WORD MouseX, WORD MouseY);
		BOOL Dragable(long);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void GetItemXY(long,long *x,long *y);
		BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *)	{ return FALSE;	}
		void SetSubParents(C_Window *Parent);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }

#endif // PARSER
};

#endif
