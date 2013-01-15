#ifndef _BULLS_EYE_H_
#define _BULLS_EYE_H_

class C_BullsEye : public C_Base
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
		float Scale_;
		float WorldX_,WorldY_;

	public:
		C_BullsEye();
		C_BullsEye(char **stream);
		C_BullsEye(FILE *fp);
		~C_BullsEye();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type);
		void SetColor(COLORREF color) { Color_=color; }
		void SetPos(float x,float y);
		void SetScale(float scl);
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
};

#endif
