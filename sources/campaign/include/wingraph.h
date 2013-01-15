#include "Windows.h"
#include "resource.h"


#define _GFILLINTERIOR	0
#define _GBORDER        1

#define Black			0
#define Blue			1
#define Green			2
#define Yellow			3
#define Red				4
#define Magenta			5
#define Brown			6
#define Gray			7
#define Orange			8
#define LightBlue		9
#define LightGreen		10
#define LightBrown		11
#define LightRed		12
#define LightGray		13
#define DarkGray		14
#define White			15

#define RGB_BLUE		RGB(0,0,255)
#define RGB_LIGHTBLUE	RGB(0,128,255)
#define RGB_CYAN		RGB(0,128,128)
#define RGB_RED			RGB(255,0,0)
#define RGB_LIGHTRED	RGB(255,60,60)
#define RGB_GREEN		RGB(0,128,0)		// 0, 117, 0 looks nice, too
//#define RGB_LIGHTGREEN	RGB(31,183,20)
#define RGB_LIGHTGREEN	RGB(11,152,17)		// RGB(0,255,0)
#define RGB_BROWNGREEN	RGB(128,128,0)
#define RGB_BROWN		RGB(140,70,0)
#define RGB_LIGHTBROWN	RGB(200,100,0)
#define RGB_MAGENTA		RGB(128,0,128)
#define RGB_ORANGE		RGB(255,128,0)
#define RGB_YELLOW		RGB(252,243,5)
#define RGB_GRAY		RGB(128,128,128)
#define RGB_LIGHTGRAY	RGB(188,188,188)
#define RGB_DARKGRAY	RGB(80,80,80)
#define RGB_WHITE		RGB(255,255,255)
#define RGB_BLACK		RGB(0,0,0)

extern HPEN				Pens[16];
extern HBRUSH			Brushes[16];

void _initgraphics(HDC DC);

void _shutdowngraphics();

void _rectangle(HDC DC, int mode, int ULX, int ULY, int LRX, int LRY);

void _ellipse(HDC DC, int mode, int ULX, int ULY, int LRX, int LRY);

void _setcolor(HDC DC, int color);

void _moveto(HDC DC, int X, int Y);

void _lineto(HDC DC, int X, int Y);

void _outgtext(HDC DC, char *str);

void _wgprintf (HDC DC, int x, int y, char *string, ... );

void _wprintf (HDC DC, char *string, ... );

void _drawbmap(HDC DC, int num, int ULX, int ULY, int size, int ofx, int ofy);

void _drawsbmap(HDC DC, int num, int ULX, int ULY, int size);
