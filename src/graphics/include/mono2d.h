/***************************************************************************\
    Mono2D.h
    Scott Randolph
    February 5, 1996

    This class provides 2D drawing functions for a Hercules monochrome
	display.
\***************************************************************************/
#ifndef _MONO2D_H_
#define _MONO2D_H_

#include "Ttypes.h"
#include "Display.h"


class MonochromeDisplay : public VirtualDisplay {
  public:
	MonochromeDisplay()					{};
	virtual ~MonochromeDisplay()		{};

	void Setup( void );	// Setup the display is graphics mode
	virtual void Cleanup( void );

	virtual void StartDraw( void )		{};
	virtual void ClearDraw( void );
	virtual void EndDraw( void );

	void Render2DPoint( int x1, int y1 );
	void Render2DPoint( float x1, float y1 );
	void Render2DLine( float x1, float y1, float x2, float y2 );


	// Use these with caution -- they will disrupt normal use of the graphics function above
	void EnterTextOnlyMode( void );
	void LeaveTextOnlyMode( void );
	void PrintTextOnly( char *string, ... );
	void ClearTextOnly( void );
	
  protected:
	int				textX;
	int				textY;
	
  protected:
	unsigned char *NewLineTextOnly( void );
	void ScrollTextOnly();
};


#endif // _MONO2D_H_
