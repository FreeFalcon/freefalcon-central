/*
** Name: CANVAS3D.H
** Description:
**		Class description which allows 2d primitives to be used on a
**		"canvas" that exists somehwere in 3-space.  This class is derived
**		from Render3D and provides 2d operations
**		virtualized from the VirtualDisplay class.  The canvas is also a
**		kind of 3d object which has location and orientation in world
**		space.
** History:
**		3-nov-97 (edg)
**			We go marching in.....
*/
#ifndef _CANVAS3D_H_
#define _CANVAS3D_H_

#include "Ttypes.h"
#include "ImageBuf.h"
#include "Context.h"
#include "render3d.h"
#include "Display.h"


class Canvas3D : public VirtualDisplay
{
  public:
	Canvas3D()	{ r3d = NULL; };
	virtual ~Canvas3D()	{ ready = FALSE; };

	// Setup and Cleanup need to have additions here, but still call the parent versions
	// virtual void Setup( ImageBuffer *imageBuffer );
	virtual void Setup( Render3D * renderer );
	virtual void Cleanup( void );

	void	ResetTargetRenderer( Render3D * renderer )	{ r3d = renderer; };

	// we don't implement these for anything
    virtual void StartDraw ( void ) 	{} ;
    virtual void ClearDraw( void ) 	{} ;
    virtual void EndDraw( void )	{} ;
    virtual void Render2DPoint( float x1, float y1 );
    virtual void Render2DLine( float x1, float y1, float x2, float y2 );
    virtual void Render2DTri( float, float, float, float, float, float) {};

	// pass on to the 3d renderer
	virtual void SetLineStyle (int style )
	{
		r3d->SetLineStyle( style );
	};

	// OW - had to add this to fix broken 3D Cockpit GM radar and missile/laserpod views
	virtual DWORD Color( void )	{return r3d->context.CurrentForegroundColor(); };
	
	virtual void SetColor( DWORD packedRGBA )
	{
		r3d->SetColor( packedRGBA );
	};
	virtual void SetBackground( DWORD packedRGBA )
	{
		r3d->SetBackground( packedRGBA );
	};	

	// returns vals in normalized screen space
	virtual float TextWidth(char *string);
	virtual float TextHeight(void);
	
	// set the model space dimensions/location of the canvas using 3 points
	void SetCanvas( Tpoint *v1, Tpoint *v2, Tpoint *v3 );

	// we need to provide an update function for the canvas orientation and
	// position in the world
	void Update( const Tpoint *loc, const Trotation *rot );

	// 2d primtitives we need to provide
	// all args are in device independent coords ( -1.0 to 1.0 )
    virtual void Point( float x1, float y1 );
    virtual void Line( float x1, float y1, float x2, float y2 );
    virtual void Tri( float x1, float y1, float x2, float y2, float x3, float y3 );
	virtual void ScreenText( float xLeft, float yTop, const char *string, int boxed ){}
	virtual void TextLeft( float x1, float y1, const char *string, int boxed = 0 );
	virtual void TextRight( float x1, float y1, const char *string, int boxed = 0 );
	virtual void TextCenter( float x1, float y1, const char *string, int boxed = 0 );
	virtual void TextLeftVertical( float x1, float y1, const char *string, int boxed = 0 );
	virtual void TextRightVertical( float x1, float y1, const char *string, int boxed = 0 );
	virtual void TextCenterVertical( float x1, float y1, const char *string, int boxed = 0 );
	virtual void Circle ( float x, float y, float xRadius );
	virtual void Arc ( float x, float y, float xRadius, float start, float stop );
	virtual float NormalizedLineHeight( void );

  protected:
  	Tpoint canObjPos;
  	Tpoint canWorldPos;
	Tpoint canObjDown;
	Tpoint canObjRight;
	Tpoint canWorldDown;
	Tpoint canWorldRight;

	float canScaleX;
	float canScaleY;

	// the canvas *MUST* be associated with a 3d context found elsewhere
	Render3D *r3d;

};

#endif // _CANVAS3D_H_
