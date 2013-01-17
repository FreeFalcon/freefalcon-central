#ifndef _CPOBJECT_H
#define _CPOBJECT_H

#include "cpmanager.h"
#include "cpcb.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Structures used for Initialization
//====================================================//

typedef struct {
	int					idNum;
	int					callbackSlot;
	int					transparencyType;
	int					cycleBits;
	int					persistant;
	int					bsurface;
	RECT				bsrcRect;
	RECT				bdestRect;
	RECT				destRect;
	ImageBuffer*		pOTWImage;
	ImageBuffer*		pTemplate;
	CockpitManager*	pCPManager;	
	// sfr: 2 scale factors
	float				hScale;			//Wombat778 10-06-2003 Changes scale from int to float
	float				vScale;			//Wombat778 10-06-2003 Changes scale from int to float
} ObjectInitStr;


//====================================================//
// CPObject Class defintion
//====================================================//

class CPObject{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// Identification Tag and Callback Ids
	//====================================================//
	int				mIdNum;
	int				mCallbackSlot;
	int				mTransparencyType;

	//====================================================//
	// Dimensions and Locations
	//====================================================//
	// sfr: changed to 2 factors
	float			mHScale;			//Wombat778 10-06-2003 Changes mScale from int to float
	float			mVScale;
	int				mWidth;
	int				mHeight;
	RECT			mDestRect;

	//====================================================//
	// Frame Execution and Timing
	//====================================================//
	int				mCycleBits;
	BOOL			mDirtyFlag;

	//====================================================//
	// Persistance Optimization
	//====================================================//
	int				mPersistant;
	int				mBSurface;
	RECT			mBSrcRect;
	RECT			mBDestRect;
	ImageBuffer		*mpBackgroundSurface;	//VWF May be able to remove later

	//====================================================//
	// Pointers to the Outside World
	//====================================================//
	SimBaseClass	*mpOwnship;
	ImageBuffer		*mpOTWImage;
	ImageBuffer		*mpTemplate;
	CockpitManager	*mpCPManager;

	// OW
	PaletteHandle *m_pPalette;
	std::vector<TextureHandle *> m_arrTex;

	//====================================================//
	// Pointers to Runtime Callback functions
	//====================================================//
	CPCallback		mExecCallback;
	CPCallback		mDisplayCallback;
	CPCallback		mEventCallback;

	//====================================================//
	// Runtime Member Functions
	//====================================================//
	void			Exec(void) {};
	virtual void	Exec(SimBaseClass*) {};
	virtual void	DisplayBlit(void) {};
	virtual void	DisplayDraw(void) {};

	virtual void	HandleEvent(void) {};
	void			SetDirtyFlag() {mDirtyFlag = TRUE;};
	virtual void 	CreateLit(void){};
	virtual void	DiscardLit(void);
	virtual void	Translate(WORD*) {};
	virtual void	Translate(DWORD*) {};		// OW added for 32 Bit rendering

	// OW
	virtual void DisplayBlit3D() { };
	void Translate3D(DWORD*);

	//====================================================//
	// Constructors and Destructors
	//====================================================//
	CPObject(const ObjectInitStr*);
    virtual ~CPObject();
};

#endif
