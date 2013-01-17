#ifndef _SCROLLING_MARQUE_H_
#define _SCROLLING_MARQUE_H_

class C_Marque : public C_Base
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
		long Font_;
		long Position_,Direction_;
//!		short Position_,Direction_;
		long MarqueLen_;
//!		short MarqueLen_;

		// Don't save from here
		O_Output *Text_;
		O_Output *BgImage_;

	public:
		C_Marque();
		C_Marque(char **stream);
		C_Marque(FILE *fp);
		~C_Marque();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type,_TCHAR *text);
		void Setup(long ID,short Type,long txtID);
		void SetColors(COLORREF fore,COLORREF back);
		void SetFGColor(COLORREF fore);
		void SetBGColor(COLORREF back);
		void SetBGImage(long ImageID);
		void SetDirection(short Dir) { Direction_=Dir; }
		void SetFont(long FontID);
		void SetFlags(long flags);
		void SetText(_TCHAR *txt);
		void SetText(long txtID);
		long GetFont() { return(Font_); }
		_TCHAR *GetText() { if(Text_) return(Text_->GetText()); return(NULL); }
		COLORREF GetFGColor() { if(Text_) return(Text_->GetFgColor()); return(0); }
		COLORREF GetBGColor() { if(Text_) return(Text_->GetBgColor()); return(0); }
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		BOOL TimerUpdate();
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetSubParents(C_Window *Parent);

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }

#endif // PARSER
};

#endif
