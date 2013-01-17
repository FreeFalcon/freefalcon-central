#ifndef _GENERIC_TEXT_H_
#define _GENERIC_TEXT_H_

class C_Text : public C_Base
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
		long FixedSize_;

		// Don't save from here down
		O_Output *Text_;
		O_Output *BgImage_;
		BOOL (*TimerCallback_)(C_Base *me);
		void SetTextID(_TCHAR *text);

	public:
		C_Text();
		C_Text(char **stream);
		C_Text(FILE *fp);
		~C_Text();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		// sfr: virtual so that versiontext setup can be called in local function
		virtual void Setup(long ID,short Type);
		void SetColors(COLORREF fore,COLORREF back);
		void SetFGColor(COLORREF fore);
		void SetBGColor(COLORREF back);
		// sfr: virtual so versiontext can override
		virtual void SetText(_TCHAR *txt);
		virtual void SetText(long txtID);
		void SetFixedWidth(long w);
		void SetBgImage(long ImageID,short x,short y);
		void SetFont(long FontID);
		void SetFlags(long flags);
		long GetFont() { return(Font_); }
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		_TCHAR *GetText() { if(Text_) return(Text_->GetText()); return(NULL); }
		COLORREF GetFGColor() { if(Text_) return(Text_->GetFgColor()); return(0); }
		COLORREF GetBGColor() { if(Text_) return(Text_->GetBgColor()); return(0); }
		void SetTimerCallback(BOOL (*Callback)(C_Base *me)) { TimerCallback_=Callback; }
		void Cleanup(void);
		BOOL TimerUpdate();
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetSubParents(C_Window *);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }
#endif // PARSER
};

// sfr: added versiontext
class C_VersionText : public C_Text {
public:
	explicit C_VersionText() : C_Text(){}
	virtual void Setup(long id, short type);
	// do nothing these ones
	void SetText(_TCHAR *txt){}
	void SetText(long txtID){}
};

#endif
