#ifndef _EDIT_BOX_H_
#define _EDIT_BOX_H_

#define MAX_EDIT_LEN 256

class C_EditBox : public C_Control
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
		long Integer_; // Value if Type is ED_INTEGER
		long MinInteger_,MaxInteger_;
		COLORREF CursorColor_,OutlineColor_;
		short MaxLen_;
		short Decimal_;
		short Cursor_;
		short UseCursor_;
		short Start_,End_;
		double Float_; // Value if Type is ED_FLOAT
		double MinFloat_,MaxFloat_;
		short JustActivated_;
		short SelStart_,SelEnd_;
		short NoChanges_;

		// Don't save from here
		O_Output *Text_; // String Contents
		O_Output *BgImage_;
		_TCHAR *OrigText_; // Contents of string at activation

		void DeleteRange();
		void CopyToText();
		void CopyFromText();

	public:
		C_EditBox();
		C_EditBox(char **stream);
		C_EditBox(FILE *fp);
		~C_EditBox();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type);
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void SetMaxLen(short len);
		void SetText(_TCHAR *text);
		void SetText(long txtID);
		void SetFont(long FontID);
		void SetDecimalPlaces(short places) { Decimal_=places; }
		void SetInteger(long value);
		void SetMinInteger(long value) { MinInteger_=value; if(Integer_ < MinInteger_) {Integer_=MinInteger_;CopyToText();}}
		long GetMinInteger(void) { return(MinInteger_); }
		BOOL TimerUpdate() { return(FALSE); }
		void SetMaxInteger(long value) { MaxInteger_=value; if(Integer_ > MaxInteger_) {Integer_=MaxInteger_;CopyToText();}}
		long GetMaxInteger(void) { return(MaxInteger_); }
		void SetFloat(double value);
		void SetMinFloat(double value) { MinFloat_=value; if(Float_ < MinFloat_) {Float_=MinFloat_;CopyToText();}}
		double GetMinFloat(void) { return(MinFloat_); }
		void SetMaxFloat(double value) { MaxFloat_=value; if(Float_ > MaxFloat_) {Float_=MaxFloat_;CopyToText();}}
		double GetMaxFloat(void) { return(MaxFloat_); }
		short GetDecimalPlaces() { return(Decimal_); }
		long GetInteger() { return(Integer_); }
		double GetFloat() { return(Float_); }
		_TCHAR *GetText();
		short  CursorOn() { return(UseCursor_); }
		void SetBGImage(long ImageID);
		void SetCursorColor(COLORREF color) { CursorColor_=color; }
		void SetFgColor(COLORREF fore);
		void SetBgColor(COLORREF back);
		void SetOutlineColor(COLORREF color) { OutlineColor_=color; }

		long CheckHotSpots(long relX,long relY);
		BOOL CheckKeyDown(unsigned char key,unsigned char);
		BOOL CheckChar(unsigned char key);
		BOOL CheckKeyboard(unsigned char DKScanCode,unsigned char Ascii,unsigned char ShiftStates,long);
		BOOL Process(long,short HitType);
//		BOOL MouseOver(long relX,long relY,C_Base *me);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void HighLite(SCREEN *surface,UI95_RECT *cliprect);

		void SetSubParents(C_Window *);

		void Activate();
		void Deactivate();
		BOOL Dragable(long) { return(TRUE); }
		BOOL Drag(GRABBER *,WORD MouseX,WORD MouseY,C_Window *);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE,C_Parser *) { ; }
#endif // PARSER
};

#endif
