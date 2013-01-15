/***************************************************************************\
    GMComposite.h
    Scott Randolph
    August 5, 1998

    This class provides the ground mapping radar image composition and
	beam sweep.
\***************************************************************************/
#ifndef _GMCOMPOSITE_H_
#define _GMCOMPOSITE_H_

#include "Tex.h"
#include "Render2D.h"
#include "gmRadar.h"


static const int	GM_TEXTURE_SIZE		= 128;
static const float	GM_OVERSCAN			= 0.2f;
static const float	GM_OVERSCAN_RNG		= 1.0f + GM_OVERSCAN;
static const float	GM_OVERSCAN_H		= 1.0f + GM_OVERSCAN;
static const float	GM_OVERSCAN_V		= 1.0f + GM_OVERSCAN + GM_OVERSCAN;


class RenderGMComposite : public Render2D {
  public:
	RenderGMComposite();
	virtual ~RenderGMComposite()	{};

	// Setup and Cleanup need to have additions here, but still call the parent versions
	virtual void Setup( ImageBuffer *output, void(*tgtDrawCallback)(void*,RenderGMRadar*, bool), void *tgtDrawParam );
	virtual void Cleanup( void );

	void	SetGimbalLimit( float angleLimit )			{ gimbalLimit = angleLimit; };
	void	SetBeam(Tpoint *from, Tpoint *at, Tpoint *center, float platformHdg, float beamAngle, int beamPercent, float cursorAngle, BOOL movingRight, bool Shaped  );
	void	DrawComposite( Tpoint *centerPoint, float platformHdg );

	// These are passed through to the RenderGMRadar object we're using
	void	SetRange( float newRange, int newLOD );
	void	SetGain( float newGain )					{ radar.SetGain( newGain ); };

	float	GetRange( void )			{ return radar.GetRange() / GM_OVERSCAN_RNG; };
	int		GetLOD( void )				{ return radar.GetLOD(); };
	float	GetGimbalLimit( void )		{ return gimbalLimit; };
	float	GetGain( void )				{ return radar.GetGain(); };


	void	DebugDrawLeftTexture( Render2D *renderer );
	void	DebugDrawRadarImage( ImageBuffer *target );

  protected:
	ImageBuffer *m_pRenderTarget;
	ImageBuffer *m_pBackupBuffer;
	ImageBuffer *m_pRenderBuffer;
	RenderGMRadar	radar;
	PaletteHandle *paletteHandle;
	bool m_bRenderTargetOwned;

	void(*tgtDrawCB)(void*, RenderGMRadar*, bool);
	void	*tgtDrawCBparam;

	float	range;
	float	worldToUnitScale;

	TextureHandle *lTexHandle;
	float	lOriginX;
	float	lOriginY;
	float	lAngle;

	TextureHandle *rTexHandle;
	float	rOriginX;
	float	rOriginY;
	float	rAngle;

	int		prevBeamPercent;
	BOOL	prevBeamRight;
	float	gimbalLimit;
	BOOL	DrawChanged;

	// OW
	TextureHandle *nTexHandle;		// noise texture

	const struct OpRecord	*nextOperation;

  protected:
	bool	BackgroundGeneration( Tpoint *from, Tpoint *at, float platformHdg, int beamPercent, BOOL movingRight, bool Shaped  );
	void	NewImage( Tpoint *at, float platformHdg, BOOL replaceRight, bool Shaped );
};

#endif // _GMCOMPOSITE_H_
