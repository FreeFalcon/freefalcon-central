#ifndef _GENERIC_LINE_H_
#define _GENERIC_LINE_H_

class C_Line : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;
		COLORREF Color_;

	public:
		C_Line();
		C_Line(char **stream);
		C_Line(FILE *fp);
		~C_Line();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type);
		void SetColor(COLORREF color);
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
#ifdef _UI95_PARSER_
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *) { ; }
#endif // PARSER
};

#endif
