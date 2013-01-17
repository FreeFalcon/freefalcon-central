#ifndef _COLOR_SCHEME_H_
#define _COLOR_SCHEME_H_

typedef struct
{
	COLORREF TitleText;	// Color of window title
	COLORREF TitleBG;		// Fill color behind window title
	COLORREF BorderLite;	// Brightest color for border line
	COLORREF BorderMedium;	// normal color
	COLORREF BorderDark;	// Shadow color
	COLORREF BGColor;		// Background of client area color
	COLORREF SelectBG;		// Background color if selected
	COLORREF NormalText;	// Normal Text Color
	COLORREF ReverseText;	// Reverse color for text (when drawn over SelColor?)
	COLORREF DisabledText;	// Normal Text Color
} COLORSCHEME;

typedef struct ColorSchemeStr COLORLIST;

struct ColorSchemeStr
{
	long ID;
	COLORSCHEME Color;
	COLORLIST *Next;
};

class C_ColorMgr
{
	private:
		COLORLIST *Root_;

	public:
		C_ColorMgr();
		~C_ColorMgr();

		void Setup();
		void Cleanup();

		void AddScheme(long ID,COLORSCHEME *newscheme);
		COLORSCHEME *GetScheme(long ID);
		BOOL FindScheme(long ID);
};

extern C_ColorMgr *gColorMgr;

#endif