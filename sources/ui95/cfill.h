#ifndef _C_FILL_H_
#define _C_FILL_H_

class C_Fill : public C_Base
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
		COLORREF Color_;
		short Start_,End_;
		short Area_,Dir_;
		short DitherSize_;
		float Step_;

		// Don't save from here
		char *DitherPattern_;
	public:
		C_Fill();
		C_Fill(char **stream);
		C_Fill(FILE *fp);
		~C_Fill();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type);
		void SetColor(COLORREF color) { F4CSECTIONHANDLE* Leave=UI_Enter(Parent_); Color_=color; UI_Leave(Leave); }
		void SetGradient(long s,long e);
		void SetDither(short size,short range);
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }
#endif // PARSER
};

#endif
