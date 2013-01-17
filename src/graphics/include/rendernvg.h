/***************************************************************************\
    RenderNVG.h
    Scott Randolph
    August 12, 1996

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#ifndef _RENDERNVG_H_
#define _RENDERNVG_H_

#include "RenderTV.h"



class RenderNVG : public RenderTV {
  public:
	RenderNVG()				{};
	virtual ~RenderNVG()	{};

	virtual void Setup( ImageBuffer *imageBuffer, RViewPoint *vp );

	virtual void StartDraw( void );

	virtual void SetColor( DWORD packedRGBA );

  protected:
	// Overloaded to provide appropriate lighting (or lack thereof)
	virtual void DrawSun( void );
	virtual void SetTimeOfDayColor( void );
	virtual void ProcessColor( Tcolor *color );
	virtual void ComputeVertexColor(TerrainVertex *vert,Tpost *post,float distance,float x,float y);
};


#endif // _RENDERTV_H_
