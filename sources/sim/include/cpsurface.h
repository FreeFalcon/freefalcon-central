#ifndef _CPSURFACE_H
#define _CPSURFACE_H

#include "Graphics\Include\imagebuf.h"
#include "Graphics\Include\define.h"

#include <vector>

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


typedef struct {
			int				idNum;
			BOOL				persistant;
			BYTE				*psrcBuffer;
			ImageBuffer		*pOtwImage;			
			RECT				srcRect;
			} SurfaceInitStr;

//====================================================//
// Foward Declarations
//====================================================//


class CPSurface {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	int				mIdNum;

	RECT				mSrcRect;
	ImageBuffer		*mpSurfaceBuffer;
	GLubyte			*mpSourceBuffer;
	ImageBuffer		*mpOTWImage;

	BOOL				mPersistant;

	int				mWidth;
	int				mHeight;

	// OW
	PaletteHandle *m_pPalette;
	std::vector<TextureHandle *> m_arrTex;

	CPSurface(SurfaceInitStr*);
	virtual ~CPSurface();

	void DisplayBlit(BYTE, BOOL, RECT*, int, int);

	void CreateLit(void);
	void DiscardLit(void);
	void Translate(WORD*);
	void Translate(DWORD*);

	// OW
	void DisplayBlit3D(BYTE, BOOL, RECT*, int, int);
	void Translate3D(DWORD*);
};

#endif
