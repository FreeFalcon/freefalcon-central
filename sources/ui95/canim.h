#ifndef _ANIM_H_
#define _ANIM_H_

class C_Anim : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;

		O_Output *Anim_;

	public:
		C_Anim();
		C_Anim(char **stream);
		C_Anim(FILE *fp);
		~C_Anim();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ;	}

		// Initialization Function
		void Setup(long ID,short Type,long AnimID);
		void SetAnim(long ID);
		void SetAnim(ANIM_RES *Anim);
		ANIM_RES *GetAnim(void) { if(Anim_) return(Anim_->GetAnim()); return(NULL); }
		// Free Function
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void SetFlags(long flags);
		void SetDirection(short dir);

		// Processing functions used by the Handler (No user calls needed)
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		BOOL TimerUpdate();

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE, C_Parser *)	{ ; }

#endif // PARSER
};

#endif
