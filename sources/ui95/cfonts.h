#ifndef _FONT_LIST_H_
#define _FONT_LIST_H_

class FONTLIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long ID_;
		HFONT Font_;
		long Spacing_;
		TEXTMETRIC Metrics_;
		int *Widths_;
#ifdef _UI95_PARSER_
		LOGFONT logfont;
#endif
		FONTLIST *Next;
};

class C_Font
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from here
		long Spacing_;

		// Don't save from here
		C_Handler *Handler_;
		FONTLIST *Root_;
		C_Hash *Fonts_; // Font Resource List (Actual ones used)

	public:
		C_Font();
		C_Font(char **stream);
		C_Font(FILE *fp);
		~C_Font();
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);

		// Setup functions
		void Setup(C_Handler *handler);
		BOOL AddFont(long ,LOGFONT *);
		void RemoveFont(long ) { ; }

		FONTLIST *FindID(long ID);
		// Query Functions
		HFONT GetFont(long ID);
		int GetHeight(long ID);
		int StrWidth(long fontID,_TCHAR *Str);
		int StrWidth(long fontID,_TCHAR *Str,int len);

		C_Fontmgr *Find(long ID);
		void LoadFont(long ID,char *filename);

		C_Hash *GetHash() { return(Fonts_); }

		// Cleanup Functions
		void Cleanup(void);

#ifdef _UI95_PARSER_

		short FontFind(char *token);
		void FontFunction(short ID,long P[],_TCHAR *str,LOGFONT *lgfnt,long *NewID);
		void SaveText(HANDLE,C_Parser *) { ; }

#endif
};

extern C_Font *gFontList;

#endif
