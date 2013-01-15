#ifndef _SPLASH_SCREEN_H
#define _SPLASH_SCREEN_H_

#define SPL_UPDATE		0x0003 // Combined Page1 & Page2
#define SPL_UPDATE_PG1	0x0001
#define SPL_UPDATE_PG2	0x0002

#define SPL_MOUSEOVER	0x0004

#define MAX_KEYSIZE	20
#define MAX_TITLESIZE	20

typedef struct ButtonData
{
	int id;
	int x,y,w,h;
	int draw_x,draw_y;
	int start_index;
	char *AnimFile;
	TCHAR *title;
} ButtonData;

typedef enum ButtonState{
	Off,	// button is at normal state
	On,	// button is at selecting state
	Dim,	// button is currently disabled
	Selected	// button was last selected
};

typedef struct ButtonInfo
{
	int id;
	// Hot Spots
	int x,y,w,h;
	// Top left corner of image (width and height is in Anim structure)
	int draw_x,draw_y;
	int index;
	int direction;
	WORD flags;
	ANIMATION *animation;
	LPDIRECTDRAWSURFACE surface;
	BITMAPINFO bmi;
	TCHAR title[MAX_TITLESIZE];
        ButtonState state; // button state
	ButtonInfo *next;
} ButtonInfo;

typedef struct ScreenData
{
	char *BkgFile;
	char *BkgFile2;
	int NumButtons;
	ButtonData *Buttons;
} ScreenData;

class CScreen
{
	char *BkgImage;
	char *BkgImage2;
	BITMAPINFO bmi, bmi2;
	WORD flags;
	WORD CurrentPage;
	ButtonInfo *ButtonList;
	HWND hwnd;
	ImageBuffer *Bkg2ImageBuf;

	void SetUpdateRect(ButtonInfo *Cur);

public:
	CScreen();
	~CScreen();
	BOOL ScreenInit(ScreenData *Scr, HWND appWnd);
	BOOL ScreenRemove();
	int GetButtonID(WORD X,WORD Y);
	int MouseOver(WORD X,WORD Y);
	int UpdateAnims(void);
	void RefreshScreen(ImageBuffer *Image);

        int selector;	// button ID that is last selected.
	HCURSOR hCursor;
	int defaultESC;	// the command id for ESC key

	BOOL AddAnimButton(ButtonData *Button);
	BOOL DelAnimButton(int ID);
	BOOL DelAnimButton(ButtonInfo *Button) { return DelAnimButton(Button->id); }
	void SetAnimButtonState(int ID, char *stateStr);
	BOOL AddBkg2(char *BkgFName);
};

#endif _SPLASH_SCREEN_H_
