#ifndef _BUTTON_H
#define _BUTTON_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpcb.h"
#include "cpmanager.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif

class CPButtonView;
class ImageBuffer;

#define _CPBUTTON_USE_STL_CONTAINERS

#ifdef _CPBUTTON_USE_STL_CONTAINERS
#include <vector>
#endif


//====================================================//
// Normal Button States
//====================================================//

#define CPBUTTON_OFF			0
#define CPBUTTON_ON			1



//====================================================
// Initialization Struct for:
//							CPButtonObject Class
//							CPButtonView Class
//====================================================

//Wombat778 3-22-04 Added type to hold textures for rendered lights.
typedef struct {
	BYTE *buttonview;
	std::vector<TextureHandle *> m_arrTex;
	int mHeight;
	int mWidth;
} SourceButtonViewType;


	typedef struct
		{
		int	objectId;
		int	callbackSlot;
		int	totalStates;
		int	normalState;
		int	totalViews;
		int	cursorIndex;
		int	delay;
		int	sound1;
		int	sound2;
		}
	ButtonObjectInitStr;


	typedef struct
		{
		int				objectId;
		int				parentButton;
		int				transparencyType;
		int				states;
		BOOL				persistant;
		RECT				destRect;
		RECT*				pSrcRect;	// List of Rects
		ImageBuffer*	pOTWImage;
		ImageBuffer*	pTemplate;
		// sfr: changed to 2 scale factors
		float			hScale;		//Wombat778 10-06-2003 Changes scale from int to float
		float			vScale;		//Wombat778 10-06-2003 Changes scale from int to float
		SourceButtonViewType *sourcebuttonview;	//Wombat778 3-22-04
		}
	ButtonViewInitStr;



//====================================================
// CPButtonObject Class Definition
//====================================================

class CPButtonObject {

	//----------------------------------------------------
	// Object Identification
	//----------------------------------------------------

	int				mIdNum;

	//----------------------------------------------------
	// Sound Info
	//----------------------------------------------------

	int				mSound1;
	int				mSound2;

	//----------------------------------------------------
	// State Information
	//----------------------------------------------------

	int				mDelay;
	int				mDelayInc;
	int				mTotalStates;
	int				mNormalState;
	int				mCurrentState;

	//----------------------------------------------------
	// Pointers and data for Runtime Callback functions
	//----------------------------------------------------

	int					mCallbackSlot;
	ButtonCallback		mTransAeroToState;
	ButtonCallback		mTransStateToAero;

	//----------------------------------------------------
	// Pointer to this buttons views
	//----------------------------------------------------

#ifdef _CPBUTTON_USE_STL_CONTAINERS
	std::vector<CPButtonView*> mpButtonView;		//List of views
#else
	CPButtonView**	mpButtonView;		//List of views

	int				mTotalViews;
	int				mViewSlot;			// Used only for adding views to button
#endif

	//----------------------------------------------------
	// Indicies for Cursor Information
	//----------------------------------------------------

	int				mCursorIndex;

public:

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif


	//----------------------------------------------------
	// Access Functions
	//----------------------------------------------------

	int				GetCurrentState() const { return mCurrentState; }
	int				GetNormalState() const  { return mNormalState;  }
	int				GetCursorIndex() const  { return mCursorIndex;  }
	int				GetId() const           { return mIdNum;        }
	int				GetCallbackId() const   { return mCallbackSlot; }  //Wombat778 3-09-04 Necessary to expose mCallbackSlot
	int             GetTotalStates() const  { return mTotalStates;  }  // sfr: for dummy callbacks
	int				GetSound(int) const;
	void			SetSound(int, int);
	// state setting functions
	void			SetCurrentState(int);
	void			IncrementState(); // check state limits
	void			DecrementState(); // check state limits

	//----------------------------------------------------
	// Runtime Functions
	//----------------------------------------------------
	
	void				AddView(CPButtonView*);
	void				NotifyViews(void);
	void				HandleMouseEvent(int);
	void				HandleEvent(int);
	void				DecrementDelay(void);
	BOOL				DoBlit(void);
	void				UpdateStatus(void);

	//----------------------------------------------------
	// Constructors and Destructors
	//----------------------------------------------------



	CPButtonObject(ButtonObjectInitStr*);
	~CPButtonObject();
};



//====================================================
// CPButtonView Class Definition
//====================================================

class CPButtonView {

	//----------------------------------------------------
	// Identification Tag and Callback Ids
	//----------------------------------------------------

	int				mIdNum;
	int				mParentButton;
	int				mTransparencyType;
	RECT			mDestRect;
	BOOL			mDirtyFlag;
	BOOL			mPersistant;
	int				mStates;

	//----------------------------------------------------
	// Pointers to the Outside World
	//----------------------------------------------------

	ImageBuffer		*mpOTWImage;
	ImageBuffer		*mpTemplate;

	//----------------------------------------------------
	// Pointer to the Button Object that owns this view
	//----------------------------------------------------

	CPButtonObject	*mpButtonObject;

	//----------------------------------------------------
	// Source Locations for Template Surface
	//----------------------------------------------------

	RECT				*mpSrcRect;	// List of Rects
	// sfr: changed to 2 scale factors
	float			mHScale;			//Wombat778 10-06-2003 Changes mScale from int to float	
	float			mVScale;

public:

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

	//----------------------------------------------------
	// Runtime Member Functions
	//----------------------------------------------------

	void						DisplayBlit(void);
	BOOL						HandleEvent(int*, int, int, int);
	void						SetDirtyFlag() {mDirtyFlag = TRUE;};
	int						GetId();
	int						GetTransparencyType(void);
	int						GetParentButton(void);
	void						SetParentButtonPointer(CPButtonObject*);	
	void						Translate(WORD*) {};
	void						Translate(DWORD*) {};
	void						UpdateView();

	int						GetCallBackAndXY(int *x, int *y);	//Wombat778 3-09-04
	void					DisplayBlit3D();	//Wombat778 3-23-04

	//Wombat778 3-22-04 Stuff for rendered lights
	SourceButtonViewType		*mpSourceBuffer;
	PaletteHandle *m_pPalette;
	virtual void CreateLit(void);
	virtual void DiscardLit(void);
	void Translate3D(DWORD*);

	//Wombat778 End

	//----------------------------------------------------
	// Constructors and Destructors
	//----------------------------------------------------

	CPButtonView(ButtonViewInitStr*);
	~CPButtonView();
};

#endif
