#ifndef _THOOK_H_
#define _THOOK_H_


class C_TimerHook : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		void (*UpdateCallback_)(long ID,short hittype,C_Base *control);
		void (*RefreshCallback_)(long ID,short hittype,C_Base *control);
		void (*DrawCallback_)(long ID,short hittype,C_Base *control);
		long DefaultFlags_;

	public:
		C_TimerHook();
		C_TimerHook(char **stream);
		C_TimerHook(FILE *fp);
		~C_TimerHook();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short type);
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void SetUpdateCallback(void (*routine)(long ID,short hittype,C_Base *control)) { UpdateCallback_=routine; }
		void SetRefreshCallback(void (*routine)(long ID,short hittype,C_Base *control)) { RefreshCallback_=routine; }
		void SetDrawCallback(void (*routine)(long ID,short hittype,C_Base *control)) { DrawCallback_=routine; }
		BOOL TimerUpdate();
		void Refresh();
		void Draw(SCREEN *,UI95_RECT *cliprect);
#ifdef _UI95_PARSER_
		short LocalFind(char *)	{ return 0;	}
		void LocalFunction(short ,long,_TCHAR *,C_Handler *) { ; }
		void SaveText(HANDLE ,C_Parser *) { ; }
#endif // PARSER
};

#endif // _THOOK_H_
