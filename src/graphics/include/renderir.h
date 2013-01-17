/***************************************************************************\
    RenderIR.h
    Scott Randolph
    August 5, 1996

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#ifndef _RENDERIR_H_
#define _RENDERIR_H_

#include "RenderTV.h"



class RenderIR : public RenderTV {
  public:
	RenderIR()			{};
	virtual ~RenderIR()	{};

	virtual void Setup( ImageBuffer *imageBuffer, RViewPoint *vp );

	//JAM 30Dec03
	virtual void StartDraw(void);
	virtual void EndDraw(void);

  protected:
	// Overloaded to provide appropriate sky effects
	virtual void DrawMoon( GLint )		{};
	virtual void DrawStars ( void )			{};

	// Overloaded to prevent the ambient light level and sky color from changing
	virtual void SetTimeOfDayColor( void )	{};
	virtual void AdjustSkyColor( void )		{};

	virtual void ComputeVertexColor(TerrainVertex *vert,Tpost *post,float distance,float x,float y);
};


#endif // _RENDERIR_H_
