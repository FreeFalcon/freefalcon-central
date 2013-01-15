/***************************************************************************\
    RenderTV.h
    Scott Randolph
    August 12, 1996

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#ifndef _RENDERTV_H_
#define _RENDERTV_H_

#include "RenderOW.h"



class RenderTV : public RenderOTW {
  public:
	RenderTV()			{};
	virtual ~RenderTV()	{};

	virtual void Setup( ImageBuffer *imageBuffer, RViewPoint *vp );

	virtual void StartDraw( void );
	virtual void EndDraw( void );

	virtual void SetColor( DWORD packedRGBA );

  protected:
	// Overloaded to provide appropriate sky effects
	virtual void DrawSun( void );
	virtual void DrawMoon( void );
 	virtual void ComputeHorizonEffect( HorizonRecord *pHorizon )	{ pHorizon->horeffect = 0; };

	virtual void ProcessColor( Tcolor *color );
	virtual void ComputeVertexColor(TerrainVertex *vert,Tpost *post,float distance,float x,float y);
};


#endif // _RENDERTV_H_
