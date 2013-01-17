#ifndef _C_PARSER_H_
#define _C_PARSER_H_

#ifdef _UI95_PARSER_

#define PARSE_MAX_PARAMS    (12)
#define MAX_WINDOWS_IN_LIST (200)
#define PARSE_HASH_SIZE		(1024)

typedef struct
{
	char Label[64];
	long Value;
} ID_TABLE;

class C_Parser
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long Idx_; // index into script
		char *script_; // script file (read into memory)
		long scriptlen_;

		long P_[PARSE_MAX_PARAMS]; // used for passing parameters to Setup routines for the new windows/controls
		_TCHAR *str_; // string pointer (only 1 allowed per command)

		C_Handler	*Handler_;	// pointer to Window Handler (from Setup())
		C_Window	*Window_;	// pointer to current window (from script)
		C_Base		*Control_;	// pointer to current control (from script)
		C_Font		*Font_;		// Pointer to current Font (from script)
		C_Image		*Image_;	// Pointer to current Image Mgr (from script)
		C_Animation	*Anim_;		// Pointer to current Animation Mgr (from Script)
		C_Sound		*Sound_;	// Pointer to current Sound Mgr (from script)
		C_PopupMgr	*Popup_;	// Pointer to Popup Menu Manager
		C_String	*String_;	// Pointer to String Manager (from script)
		C_Movie		*Movie_;	// Pointer to Movie Manager (from script)

		long       WindowList_[MAX_WINDOWS_IN_LIST];

		C_Hash	*IDOrder_;		// Hash List in ID order... for finding tokens
		C_Hash	*TokenOrder_;	// Hash List in "Token" order

		char ValueStr[40];		// string to contain values of IDs NOT found in table

		FILE *Perror_;
		// Current Token;
		short  tokenlen_;

		// Parameters
		short P_Idx_; // pointer to current parameter

		short        WinIndex_,WinLoaded_;

		C_Window *WindowParser();
		C_Base *ControlParser();
		C_Base *PopupParser();
		void AddInternalIDs(ID_TABLE tbl[]);
		long TokenizeIDs(char *idfile,long size);
		void LoadIDTable(char *filename);
		FILE *OpenArtFile(char *filename, const char *thrdir, const char *maindir, int hirescapable = 1);

	public:
		C_Parser();
		~C_Parser();

		void Setup(C_Handler *handler,C_Image *ImgMgr,C_Font *FontList,C_Sound *SndMgr,C_PopupMgr *PopupMgr,C_Animation *AnimMgr,C_String *StringMgr,C_Movie *MovieMgr);
		void Cleanup();
		char *FindIDStr(long ID);
		long FindID(char *token);
		long FindToken(char *token);

		void SetCheck(long ID) { if(TokenOrder_) TokenOrder_->SetCheck(ID); }
		void LoadIDList(char *filelist);
		BOOL LoadScript(char *filename);
		BOOL ParseScript(char *filename);
		BOOL LoadWindowList(char *filename);
		BOOL LoadSoundList(char *filename);
		BOOL LoadStringList(char *filename);
		BOOL LoadMovieList(char *filename);
		BOOL LoadImageList(char *filename);
		BOOL LoadPopupMenuList(char *filename);
		C_SoundBite *ParseSoundBite(char *filename);
		C_Base *ParseControl(char *filename);
		C_Window *ParseWindow(char *filename);
		C_Image *ParseImage(char *filename);
		C_Sound *ParseSound(char *filename);
		C_String *ParseString(char *filename);
		C_Movie *ParseMovie(char *filename);
		C_Font *ParseFont(char *filename);
		C_Base *ParsePopupMenu(char *filename);
		C_Hash *GetTokenHash() { return(TokenOrder_); }
		C_Hash *GetIDHash() { return(IDOrder_); }
		long GetFirstWindowLoaded() { WinIndex_=0; if(WinIndex_ < WinLoaded_) return(WindowList_[WinIndex_]); else return(0); }
		long GetNextWindowLoaded() { WinIndex_++; if(WinIndex_ < WinLoaded_) return(WindowList_[WinIndex_]); else return(0); }
		long AddNewID(char *label,long);
		void LogError(char *str);
};

// User ID Table file format: (MAX ID Length=35 chars) All values are signed longs
//                             IDs must start with a non-numeric/non-"white space" character
//                             Multiple IDs may have identical values (if you'd like)
//                             Sorry, No spaces in the ID name, the ID can be separated from it's value
//                             these characters only (" ",TAB,",",<cr>,<lf>) (as many as you'd like))
// 
// Line 1: SOMEID 5001    (where n is the value) 
//   .
//   .
//   .
// Line n: ANOTHERID, 23
//
// EACH Parser instance (parser=new C_Parser; parser->Setup() ...)
// maintains its own set of User-definable IDs
//
#endif // Parser
#endif // _H_
