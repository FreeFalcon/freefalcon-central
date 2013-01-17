/***************************************************************************\
    RenderWire.h
    Scott Randolph
    May 5, 1998

    This sub class draws an out the window view in wire frame mode.
\***************************************************************************/
#ifndef _RENDERWIRE_H_
#define _RENDERWIRE_H_

#include "RenderOW.h"



class RenderWire : public RenderOTW {
  public:
	RenderWire()			{};
	virtual ~RenderWire()	{};

	virtual void Setup( ImageBuffer *imageBuffer, RViewPoint *vp );

	virtual void StartFrame( void );

  protected:
	// Overloaded to provide appropriate sky effects
	virtual void DrawSun( void )	{};
	virtual void DrawMoon( void )	{};
	virtual void DrawStars( void )	{};
 	virtual void ComputeHorizonEffect( HorizonRecord *pHorizon )	{ pHorizon->horeffect = 0; };

	// Overloaded to prevent the ambient light level and sky color from changing
	virtual void SetTimeOfDayColor( void )	{};
	virtual void AdjustSkyColor( void )		{};

	virtual void ComputeVertexColor( TerrainVertex *vert, Tpost *post, float distance );
};


#endif // _RENDERWIRE_H_
