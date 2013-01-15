
#ifdef CAMPTOOL

#include <io.h>
#include <stdarg.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <conio.h>
#include "wingraph.h"
#include "vutypes.h"

//#define LETTERX 8
//#define LETTERY 8

// ==============================================
// Graphical User Interface Functions 
// ==============================================

   #define Black         0
   #define Blue          1
   #define Green         2
   #define Yellow        3
   #define Red           4
   #define Magenta       5
   #define Brown         6
   #define Gray          7
   #define Orange        8
   #define LightBlue     9
   #define LightGreen   10
   #define LightBrown   11
   #define LightRed     12
   #define LightGray		13
   #define DarkGray     14
   #define White        15

	DWORD ColorTable16[16] = {	RGB_BLACK,RGB_BLUE,RGB_GREEN,RGB_YELLOW,
										RGB_RED,RGB_MAGENTA,RGB_BROWN,RGB_GRAY,
										RGB_ORANGE,RGB_LIGHTBLUE,RGB_LIGHTGREEN,RGB_LIGHTBROWN,
										RGB_LIGHTRED,RGB_LIGHTGRAY,RGB_DARKGRAY,RGB_WHITE};
	HPALETTE			hPal;

	HBITMAP			BMaps[16];
	HPEN				Pens[16];
	HBRUSH			Brushes[16];
	HDC				hMDC;
	HDC				BMapDC=NULL;
	HBITMAP			BMap;

	static int		CurrX,CurrY;

	extern HINSTANCE hInst;
 
	void _initgraphics(HDC DC)
		{
		int			i;
		HBITMAP		hOldBMap;

      // load bitmaps
      for (i=0;i<16;i++)
			{
//      	BMaps[i] = LoadBitmap( hInst, MAKEINTRESOURCE( BNames[i] ) ) ;
			Pens[i] = CreatePen(PS_SOLID,0,ColorTable16[i]);
			Brushes[i] = CreateSolidBrush(ColorTable16[i]);
			}
		hMDC = CreateCompatibleDC(DC);
		// New stuff
		BMap = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BIGBMAP));
		hOldBMap = (HBITMAP)SelectObject(hMDC, BMap);
		DeleteObject(hOldBMap);
		}

	void _shutdowngraphics()
		{
		int			i;

		for (i=0;i<16;i++)
			{
//			DeleteObject(BMaps[i]);
			DeleteObject(Pens[i]);
			DeleteObject(Brushes[i]);
			}
		DeleteObject(BMap);
		DeleteDC(hMDC);
		}

	void _drawbmap(HDC DC, int num, int ULX, int ULY, int size, int ofx, int ofy)
		{
		BitBlt(DC,	ULX, ULY, size, size, hMDC, 16*(num%8)+ofx, 16*(num/8)+ofy, SRCCOPY);
		}

/*
	void _drawsbmap(HDC DC, int num, int ULX, int ULY, int size)
		{
		HDC	hBmOld;

		hBmOld = (HBITMAP)SelectObject(hMDC, BMaps[num]);
		StretchBlt(DC, ULX, ULY, size, size, hMDC, 0, 0, 8, 8, SRCCOPY);
		}
*/

	void _rectangle(HDC DC, int mode, int ULX, int ULY, int LRX, int LRY)
  		{
		RECT		myrect;

		if (mode==_GFILLINTERIOR)
			Rectangle(DC,ULX,ULY,LRX,LRY);
		else
			{
			myrect.left = ULX;
			myrect.top = ULY;
			myrect.right = LRX;
			myrect.bottom = LRY;
			FrameRect(DC,&myrect,(HBRUSH)GetCurrentObject(DC,OBJ_BRUSH));
 			}
		}

	void _ellipse(HDC DC, int mode, int ULX, int ULY, int LRX, int LRY)
  		{
		HBRUSH	last;

		if (mode==_GBORDER)
			{
			last = (HBRUSH)SelectObject(DC,GetStockObject(HOLLOW_BRUSH));
			Ellipse(DC,ULX,ULY,LRX,LRY);
			SelectObject(DC,last);
			}
		else
			Ellipse(DC,ULX,ULY,LRX,LRY);
		}

	void _setcolor(HDC DC, int color)
		{								
		SelectObject(DC,Pens[color]);
		SelectObject(DC,Brushes[color]);
		}	

	void _moveto(HDC DC, int X, int Y)
		{
		MoveToEx(DC, X, Y, NULL);
		CurrX = X;
		CurrY = Y;
		} 

	void _lineto(HDC DC, int X, int Y)
		{
		LineTo(DC, X, Y);
		}

	void _outgtext(HDC DC, char *str)
		{
		TextOut(DC,CurrX,CurrY,str,strlen(str));
		}

   void _wgprintf (HDC DC, int x, int y, char *string, ... )
      {
      va_list params;   
      int   check;
      static char  _buffer[120];

      va_start( params, string );
      if( !string ) return;
      check = vsprintf( _buffer, string, params );
      va_end( params );
      _moveto(DC,x,y);
      _outgtext(DC,_buffer);
      }

   void _wprintf (HDC DC, char *string, ... )
      {
      va_list params;   
      int   check;
      static char  _buffer[120];

      va_start( params, string );
      if( !string ) return;
      check = vsprintf( _buffer, string, params );
      va_end( params );
      _outgtext(DC,_buffer);
      }

#endif CAMPTOOL

