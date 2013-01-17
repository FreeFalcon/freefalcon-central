#ifndef _MOVIE_RESOURCE_H_
#define _MOVIE_RESOURCE_H_

class MOVIE_RES
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long ID;
		char Movie[MAX_PATH];
		char SubTitle[MAX_PATH];
		MOVIE_RES *Next;
};

class C_Movie
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from Here
		long flags_;
		long x_,y_;

		// Don't save from here
		MOVIE_RES *Root_;

	public:
		enum
		{
			_NO_FLAGS_			=0x0000,
			_USE_SUBTITLES_		=0x0001,
		};

		C_Movie();
		C_Movie(char **stream);
		C_Movie(FILE *fp);
		~C_Movie();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup();
		void Cleanup();

		void SetXY(long x,long y) { x_=x; y_=y; }
		long GetX() { return(x_); }
		long GetY() { return(y_); }
		BOOL AddMovie(long ID,char *fname);
		BOOL AddSubTitle(long ID,char *fname);
		MOVIE_RES *GetMovie(long ID);
		void SetFlags(long flag) { flags_=flag; }
		long GetFlags() { return(flags_); }
		BOOL Play(long ID);
		BOOL RemoveMovie(long ID);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }
#endif // PARSER
};

extern C_Movie *gMovieMgr;

#endif
