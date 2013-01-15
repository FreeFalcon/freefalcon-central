#ifndef _CCURSOR_H_
#define _CCURSOR_H_

class C_Cursor : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from here
		long DefaultFlags_;
		COLORREF Color_,BoxColor_;
		short Percent_;

	public:
		short MinX_,MinY_;
		short MaxX_,MaxY_;

		C_Cursor();
		C_Cursor(char **stream);
		C_Cursor(FILE *fp);
		~C_Cursor();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		// Setup Functions 
		void Setup(long ID,short Type);
		void SetRanges(short x1,short y1,short x2,short y2) { MinX_=x1; MinY_=y1; MaxX_=x2; MaxY_=y2; }
		void SetColor(COLORREF color) { Color_=color; }
		void SetBoxColor(COLORREF color) { BoxColor_=color; }
		void SetPercentage(short p) { Percent_=p; }

		// Cleanup Functions
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		// Handler/Window Functions
		long CheckHotSpots(long relx,long rely);
		BOOL Process(long,short ButtonHitType);
		BOOL Dragable(long) {return(GetFlags() & C_BIT_DRAGABLE);}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void GetItemXY(long,long *x,long *y) { *x=GetX();*y=GetY();}
		BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD,WORD,C_Window *) { return FALSE; }

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *) { ; }

#endif // PARSER
};

#endif
