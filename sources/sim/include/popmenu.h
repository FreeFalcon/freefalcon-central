#ifndef _POPMENU_H
#define _POPMENU_H

#include "stdhdr.h"
#include "msginc\wingmanmsg.h"
#include "mesg.h"
#include "wingorder.h"

#define TYPE_MENUMANGER_STR	"MENUMANAGER"
#define TYPE_MENU_STR			"MENU"
#define TYPE_PAGE_STR			"PAGE"
#define TYPE_ITEM_STR			"ITEM"
#define TYPE_RES_STR				"RESOLUTION"


// ---------------------------------------------------------
// Color Structure
// ---------------------------------------------------------

typedef struct MenuColorStruct {
	char		mName[20];
	ULONG		mValue;
} MenuColorStruct;



BOOL FindMenuColorIndex(char*, int*);
BOOL FindMenuColorValue(char*, ULONG*);

// ---------------------------------------------------------
// Rectangle Dimensions for Different Resolutions
// ---------------------------------------------------------

typedef struct ResStruct {
	int	xRes;
	int	yRes;
	RECT	mDimensions;

} ResStruct;


// ---------------------------------------------------------
// Menu Item Definition
// ---------------------------------------------------------

typedef struct ItemStruct {
	char*			mpText;
	int			mMessage;
	float			mSpacing;
	long			mNormColor;
	long			mDrawColor;
	int			mCondition;
	BOOL			mIsAvailable;
	BOOL			mPoll;
} ItemStruct;

// ---------------------------------------------------------
// Menu Page Definition
// ---------------------------------------------------------

typedef struct PageStruct {

	char*			mpTitle;
	int			mNumItems;
	ItemStruct*	mpItems;
	int			mCondition;
	long			mDrawColor;
} PageStruct;

// ---------------------------------------------------------
// Menu Defintion
// ---------------------------------------------------------

typedef struct MenuStruct {

	char*			mpTitle;
	int			mMsgId;
	int			mWingExtent;
	int			mNumPages;
	PageStruct*	mpPages;
	int			mCondition;
	long			mDrawColor;
} MenuStruct;


// ---------------------------------------------------------
// Menu Defintion
// ---------------------------------------------------------

class MenuManager
{
private:

	BOOL				mIsActive;
	BOOL				mIsPolling;
	int				mNumMenus;
	MenuStruct*		mpMenus;
	RECT				mDestRect;
	ResStruct*		mpResDimensions;
	int				mMaxTextLen;
	int				mTotalRes;
	int				mResCount;

	int				mCurMenu;
	int				mCurPage;

	float				mLeft;
	float				mRight;
	float				mTop;
	float				mBottom;

	VU_ID				mTargetId;

	// These members are for initializing when steping
	// to new pages they are for convenience.
	int				mCallerIdx;
	int				mNumInFlight;
	int				mExtent;
	int				mNumItems;
	PageStruct		*mpPage;	
	int				mOnGround;			
	int				mAWACSavail;
	//

	void				ReadDataFile(char* pfileName);
	void				ParseManagerInfo(char* plinePtr);
	void				ParseMenuInfo(char* plinePtr, int* menuNumber, int* pageNumber);
	void				ParsePageInfo(char* plinePtr, int* menuNumber, int* pageNumber, int* itemNumber);
	void				ParseItemInfo(char* plinePtr, int* menuNumber, int* pageNumber, int* itemNumber);
	void				ParseResInfo(char* plinePtr, int resCount);
	void				SelectItem (unsigned long, int);
	void				SendMenuMsg(int, int, int, VU_ID);
	void				CheckItemConditions(BOOL);
	void				InitPage(void);

						
public:
	BOOL				IsActive() { return mIsActive; }  // ASSOCIATOR 1/12/03: Added to know when the menu is active

	void				DisplayDraw(void);

	void				ProcessInput (unsigned long, int, int, int extent = AiNoExtent);
	void				StepNextPage(int);
	void				StepPrevPage(int);
	void				DeActivate(void);
	void				DeActivateAndClear(void);

	MenuManager(int, int);
	~MenuManager();
};

void MenuSendAtc(int, int sendRequest=TRUE);
void MenuSendAwacs(int, VU_ID, int sendRequest=TRUE);
void MenuSendTanker(int);

extern const RECT gDestLoc6x4;
extern const RECT gDestLoc8x6;

#endif