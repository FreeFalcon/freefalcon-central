/*
** Name: SFX.CPP
** Description:
**		Special Effects Class functions
** History:
**		23-jul-97 (edg)  We go traipsing in....
*/
#include "Graphics\Include\Rviewpnt.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawparticlesys.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\drawtrcr.h"
#include "Graphics\Include\drawgrnd.h"
#include "Graphics\Include\terrtex.h"
#include "Graphics\Include\tod.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "simbase.h"
#include "ClassTbl.h"
#include "fsound.h"
#include "soundfx.h"
#include "sfx.h"
#include "fakerand.h"
#include "simfeat.h"
#include "weather.h"
#include "acmi\src\include\acmirec.h"
#include "falcmesg.h"
#include "mesg.h"
#include "MsgInc\MissileEndMsg.h"
#include "MsgInc\DamageMsg.h"
#include "entity.h"

#ifdef USE_SH_POOLS
MEM_POOL	SfxClass::pool;
#endif

// a distance to use (ft) for further improving LOD on some effects
#define SFX_LOD_DIST		( 50000.0f )

#define	TRACER_VELOCITY		4000.0f
#define MAX_TIME_TO_LIVE	( 0.8f * TRACER_VELOCITY/( GRAVITY * 0.5f ) )

void CalcTransformMatrix (SimBaseClass* theObject);

// detail level of sfx ( 0.0 - 1.0 )
float gSfxLOD = 1.0f;

// for ACMI recording
ACMIStationarySfxRecord acmiStatSfx;
ACMIMovingSfxRecord acmiMoveSfx;

// this probably isn't correct.  I suspect the sfx aren't moving
// correctly based on SimLibMajorFrameTime.  I just want to do
// my own timing right now
DWORD lastViewTime = 0;
float sfxFrameTime;

Tpoint gWindVect = { 0.0f, 0.0f, 0.0f };
VU_TIME gWindTimer = 0;

// These are counters for sfx -- we may want to use these for debugging
// as well as determining how many are running for CPU loading
int gTotSfx = 0;
int gSfxLODCutoff = 40;
int gSfxLODDistCutoff = 150;
int gSfxLODTotCutoff = 400;
int gTotHighWaterSfx = 0;
int gSfxCount[ SFX_NUM_TYPES ] = {0};
int gSfxHighWater[ SFX_NUM_TYPES ] = {0};

// define for how often to check view distance (secs)
#define VIEW_DIST_INTERVAL		( 0.5f )

// just testing some stuff for oriented billboards
// (i.e. clouds)
/*
Tpoint cverts[4] =
{
	{ 0.0f, -250.0f, -120.0f },
	{ 0.0f,  250.0f, -120.0f },
	{ 0.0f,  250.0f,  120.0f },
	{ 0.0f, -250.0f,  120.0f },
};
Tpoint cuvs[4] =
{
	{  0.15f, 	0.07f,	 0.0f },
	{  0.73f, 	0.07f,	 0.0f },
	{  0.73f, 	0.95f,	 0.0f },
	{  0.15f, 	0.95f,	 0.0f },
};
*/

Tpoint gFireVerts[4] =
{
	{-0.5f, -0.5f, -0.75f },
	{-0.5f,  0.5f, -0.75f },
	{ 0.5f,  0.5f,  0.75f },
	{ 0.5f, -0.5f,  0.75f },
};
Tpoint gGroundVerts[4] =
{
	{-0.5f, -0.5f, -1.0f },
	{-0.5f,  0.5f, -1.0f },
	{ 0.5f,  0.5f,  1.0f },
	{ 0.5f, -0.5f,  1.0f },
};
Tpoint gWaterVerts[4] =
{
	{-0.3f, -0.3f, -1.5f },
	{-0.3f,  0.3f, -1.5f },
	{ 0.3f,  0.3f,  1.5f },
	{ 0.3f, -0.3f,  1.5f },
};
Tpoint gFireUvs[4] =
{
	{  0.0f, 	0.0f,	 0.0f },
	{  1.0f, 	0.0f,	 0.0f },
	{  1.0f, 	1.0f,	 0.0f },
	{  0.0f, 	1.0f,	 0.0f },
};
Tpoint gShockVerts[4] =
{
	{ 1.0f, -1.0f,  0.0f },
	{ 1.0f,  1.0f,  0.0f },
	{-1.0f,  1.0f,  0.0f },
	{-1.0f, -1.0f,  0.0f },
};

int AddParticleEffect(int SfxId, Tpoint *pos, Tpoint *vec)
{
	if(DrawableParticleSys::IsValidPSId(SfxId+1))
	{
		Tpoint z = {0.,0.,0.};
		if(!vec)
		{
			vec = &z;
		}
		OTWDriver.AddSfxRequest( new SfxClass (SfxId, pos,	vec,	1,	1 ) );	
		return 1;
	}
	return 0;
}

int AddParticleEffect(char *name, Tpoint *pos, Tpoint *vec)
{
	int id = DrawableParticleSys::GetNameId(name);

	if(DrawableParticleSys::IsValidPSId(id))
	{
		Tpoint zero = {0.,0.,0.};
		if(!vec)
		{
			vec = &zero;
		}


		DrawableParticleSys *ps;
		ps = new DrawableParticleSys(id,1);
		ps->AddParticle(pos, vec);

		OTWDriver.AddSfxRequest( new SfxClass ( ps ) );	
		return 1;
	}
	return 0;
}

int SfxClass::TryParticleEffect(void)
{
	if(DrawableParticleSys::IsValidPSId(type+1))
	{
		objParticleSys = new DrawableParticleSys(type+1,1);
		//objParticleSys->SetHeadVelocity(&vec);
		objParticleSys->AddParticle(&pos, &vec);
		type=SFX_PARTICLE_KLUDGE;
		return 1;
	}
	return 0;
}

// HACK: turn off optimizations for this function.  The optimizer
// may have a bug in it.....
#pragma optimize( "", off )

SfxClass::SfxClass ( DrawableParticleSys *drawPartSys )
{
	inACMI = FALSE;
	type = SFX_PARTICLE_KLUDGE;
	flags = 0;
	vec.x = 0.0f;
	vec.y = 0.0f;
	vec.z = 0.0f;
	drawPartSys->GetPosition(&pos);

	timeToLive = 1;
	// scale = scaleSfx * ( 0.6f + 0.4f * gSfxLOD );
	scale = 1;
	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;

	objParticleSys = drawPartSys;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	obj2d = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

		viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}

}

/*
** Name: SfxClass Constructors
** Description:
**		creates an effect of the appropriate type
**
*/

/*
**		Message Timer
*/
SfxClass::SfxClass (FalconMissileEndMessage *endM,
					FalconDamageMessage *damM )
{

	inACMI = FALSE;
	type = SFX_MESSAGE_TIMER;
	flags = SFX_TIMER_FLAG | SFX_SECONDARY_DRIVER;
	endMessage = endM;
	damMessage = damM;
	vec.x = 0.0f;
	vec.y = 0.0f;
	vec.z = 0.0f;
	pos.x = 0.0f;
	pos.y = 0.0f;
	pos.z = 0.0f;
	timeToLive = 0.0f;
	// scale = scaleSfx * ( 0.6f + 0.4f * gSfxLOD );
	scale = 0.0f;
	secondaryCount = 1;
	secondaryInterval = 0.0f;
	secondaryTimer = SimLibElapsedTime / SEC_TO_MSEC + PRANDFloatPos() * 25.0f;
	travelDist = 0.0f;

	objParticleSys = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	obj2d = NULL;
	baseObj = NULL;

	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}


}

/*
**		NonMoving.
*/
SfxClass::SfxClass (int 	typeSfx,
					Tpoint *posSfx,
					float timeToLiveSfx,
					float scaleSfx )
{

	inACMI = FALSE;
	type = typeSfx;
	flags = 0;
	pos = *posSfx;
	vec.x = 0.0f;
	vec.y = 0.0f;
	vec.z = 0.0f;
	timeToLive = timeToLiveSfx;
	// scale = scaleSfx * ( 0.6f + 0.4f * gSfxLOD );
	scale = scaleSfx;
	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;

	objParticleSys = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	obj2d = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		MonoPrint("Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}

	switch( type )
	{
		case SFX_AIR_HANGING_EXPLOSION:
			switch( PRANDInt5() )
			{
				case 0:
    				obj2d = new Drawable2D( DRAW2D_HIT_EXPLOSION, scale, &pos );
					break;
				case 1:
    				obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION2, scale, &pos );
					break;
				case 2:
    				obj2d = new Drawable2D( DRAW2D_CHEM_EXPLOSION, scale, &pos );
					break;
				case 3:
    				obj2d = new Drawable2D( DRAW2D_DEBRIS_EXPLOSION, scale, &pos );
					break;
				case 4:
				default:
    				obj2d = new Drawable2D( DRAW2D_DEBRIS_EXPLOSION, scale, &pos );
					break;
			}
			timeToLive += 1.0f;
			secondaryCount = 1;
			break;
		case SFX_TRAILSMOKE:
    		obj2d = new Drawable2D( DRAW2D_TRAILSMOKE, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_AIR_EXPLOSION:
			secondaryCount = 1;
			timeToLive += 1.0f;
    		obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION2, scale, &pos );
			break;
		case SFX_INCENDIARY_EXPLOSION:
			secondaryCount = 1;
    		obj2d = new Drawable2D( DRAW2D_INCENDIARY_EXPLOSION, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_AIR_EXPLOSION_NOGLOW:
			timeToLive += 1.0f;
    		obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION2, scale, &pos );
			break;
		case SFX_LONG_HANGING_SMOKE:
    		// obj2d = new Drawable2D( DRAW2D_LONG_HANGING_SMOKE, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_LONG_HANGING_SMOKE2, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_LONG_HANGING_SMOKE2:
			objTrail = new DrawableTrail(30);
			timeToLive = 3000;
    		//obj2d = new Drawable2D( DRAW2D_LONG_HANGING_SMOKE2, scale, &pos );
			//timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FAST_FADING_SMOKE:
    		obj2d = new Drawable2D( DRAW2D_FAST_FADING_SMOKE, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_AIR_DUSTCLOUD:
    		obj2d = new Drawable2D( DRAW2D_AIR_DUSTCLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_GROUND_DUSTCLOUD:
    		obj2d = new Drawable2D( DRAW2D_GROUND_DUSTCLOUD, scale, &pos );
			break;
		case SFX_WATER_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_STEAM_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_STEAM_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_BLUE_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_BLUE_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_LIGHT_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_AIR_SMOKECLOUD:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_AIR_SMOKECLOUD2:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD2, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_NOTRAIL_FLARE:
    		obj2d = new Drawable2D( DRAW2D_FLARE, scale, &pos );
			break;
		case SFX_GUNSMOKE:
    		obj2d = new Drawable2D( DRAW2D_GUNSMOKE, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_GROUND_PENETRATION:
			secondaryCount = 1;
			break;
		case SFX_AIR_PENETRATION:
			secondaryCount = 1;
			break;
		case SFX_GROUND_EXPLOSION:
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_FIRE4 ] > gSfxLODCutoff ||
				 gSfxCount[ SFX_FIRE5 ] > gSfxLODCutoff )
			{
				scale *= 0.20f;
				pos.z -= scale;
    			obj2d = new Drawable2D( DRAW2D_GROUND_STRIKE, scale, &pos, 4, gGroundVerts, gFireUvs );
				secondaryCount = 1;
			}
			else
			{
				pos.z -= scale;
				secondaryCount = 10;
				secondaryInterval = 0.1f;
			}
			break;
		case SFX_GROUND_EXPLOSION_NO_CRATER:
			pos.z -= scale;
    		obj2d = new Drawable2D( DRAW2D_GROUND_STRIKE, scale, &pos, 4, gGroundVerts, gFireUvs );
			break;
		case SFX_WATER_EXPLOSION:
			if ( gTotSfx >= gSfxLODTotCutoff )
			{
				scale *= 0.20f;
				pos.z -= scale;
    			obj2d = new Drawable2D( DRAW2D_WATER_STRIKE, scale, &pos, 4, gWaterVerts, gFireUvs );
			}
			else
			{
				pos.z -= scale;
			}
			secondaryCount = 1;
			break;
		case SFX_SHOCK_RING:
    		// obj2d = new Drawable2D( DRAW2D_SHOCK_RING, scale, &pos, 4, gShockVerts, gFireUvs );
    		obj2d = new Drawable2D( DRAW2D_SHOCK_RING, scale, &pos, (struct Trotation *)&IMatrix );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_SHOCK_RING_SMALL:
    		// obj2d = new Drawable2D( DRAW2D_SHOCK_RING, scale, &pos, 4, gShockVerts, gFireUvs );
    		obj2d = new Drawable2D( DRAW2D_SHOCK_RING_SMALL, scale, &pos, (struct Trotation *)&IMatrix );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_GROUND_FLASH:
    		obj2d = new Drawable2D( DRAW2D_GROUND_FLASH, scale, &pos );
			// SCR 11/17/98  Lets not draw ground flashes when its light out
			if (TheTimeOfDay.GetLightLevel() < 0.5f)
			{
				timeToLive = obj2d->GetAlphaTimeToLive();
			} else {
				timeToLive = 0.0f;
			}
			break;
		case SFX_GROUND_GLOW:
    		obj2d = new Drawable2D( DRAW2D_GROUND_GLOW, scale, &pos );
			break;
		case SFX_MISSILE_BURST:
    		obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
			secondaryCount = 1;
			// just testing this....
    		// obj2d = new Drawable2D( DRAW2D_CLOUD1, 1.0f, &pos, 4, cverts, cuvs );
			// timeToLive = 30.0f;
			break;
		case SFX_SMALL_HIT_EXPLOSION:
			switch( PRANDInt5() )
			{
				case 0:
    				obj2d = new Drawable2D( DRAW2D_SMALL_HIT_EXPLOSION, scale, &pos );
					break;
				case 1:
    				obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
					break;
				case 2:
    				obj2d = new Drawable2D( DRAW2D_SMALL_CHEM_EXPLOSION, scale, &pos );
					break;
				case 3:
    				obj2d = new Drawable2D( DRAW2D_SMALL_DEBRIS_EXPLOSION, scale, &pos );
					break;
				case 4:
				default:
    				obj2d = new Drawable2D( DRAW2D_SMALL_DEBRIS_EXPLOSION, scale, &pos );
					break;
			}
			break;
		case SFX_SMALL_AIR_EXPLOSION:
    		obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
			break;
		case SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL:
			secondaryCount = 1;
			break;
		case SFX_HIT_EXPLOSION:
		case SFX_HIT_EXPLOSION_NOGLOW:
		case SFX_HIT_EXPLOSION_DEBRISTRAIL:
		case SFX_HIT_EXPLOSION_NOSMOKE:
			switch( PRANDInt5() )
			{
				case 0:
    				obj2d = new Drawable2D( DRAW2D_HIT_EXPLOSION, scale, &pos );
					break;
				case 1:
    				obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION2, scale, &pos );
					break;
				case 2:
    				obj2d = new Drawable2D( DRAW2D_CHEM_EXPLOSION, scale, &pos );
					break;
				case 3:
    				obj2d = new Drawable2D( DRAW2D_DEBRIS_EXPLOSION, scale, &pos );
					break;
				case 4:
				default:
    				obj2d = new Drawable2D( DRAW2D_DEBRIS_EXPLOSION, scale, &pos );
					break;
			}
			timeToLive += 1.0f;
			secondaryCount = 1;
			// just testing this....
    		// obj2d = new Drawable2D( DRAW2D_CLOUD1, 1.0f, &pos, 4, cverts, cuvs );
			// timeToLive = 30.0f;
			break;
		case SFX_CAMP_FIRE:
			secondaryCount = 1;
			break;
		case SFX_FIRE:
    		obj2d = new Drawable2D( DRAW2D_FIRE, scale, &pos, 4, gFireVerts, gFireUvs );
			secondaryCount = (int)(timeToLive * 1.0f);
			secondaryInterval = 1.0f;
			break;
		case SFX_FIRE_EXPAND:
    		obj2d = new Drawable2D( DRAW2D_FIRE_EXPAND, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			secondaryCount = (int)(timeToLive * 1.0f);
			secondaryInterval = 1.0f;
			break;
		case SFX_FIRE_NOSMOKE:
    		obj2d = new Drawable2D( DRAW2D_FIRE, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE1:
    		obj2d = new Drawable2D( DRAW2D_FIRE1, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE2:
    		obj2d = new Drawable2D( DRAW2D_FIRE2, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE3:
    		obj2d = new Drawable2D( DRAW2D_FIRE3, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE4:
    		obj2d = new Drawable2D( DRAW2D_FIRE4, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE5:
    		obj2d = new Drawable2D( DRAW2D_FIRE5, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE6:
    		obj2d = new Drawable2D( DRAW2D_FIRE6, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE7:
    		obj2d = new Drawable2D( DRAW2D_FIRE7, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE_HOT:
    		obj2d = new Drawable2D( DRAW2D_FIRE_HOT, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE_MED:
    		obj2d = new Drawable2D( DRAW2D_FIRE_MED, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE_COOL:
    		obj2d = new Drawable2D( DRAW2D_FIRE_COOL, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_FIRE_EXPAND_NOSMOKE:
    		obj2d = new Drawable2D( DRAW2D_FIRE_EXPAND, scale, &pos, 4, gFireVerts, gFireUvs );
			break;
		case SFX_SPARKS:
		case SFX_SPARKS_NO_DEBRIS:
    		obj2d = new Drawable2D( DRAW2D_SPARKS, scale * 0.2f, &pos, 4, gGroundVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			secondaryCount = (int)(timeToLive * 10.0f);
			secondaryInterval = 0.1f;
			break;
			/*
		case SFX_SPARKS: // MLR 2/3/2004 - 
		case SFX_SPARKS_NO_DEBRIS:
    		objParticleSys = new DrawableParticleSys(PST_EXPLOSION_SMALL,1);
			objParticleSys->AddParticle(&pos);
			timeToLive = 10;

			//secondaryCount = (int)(timeToLive * 10.0f);
			//secondaryInterval = 0.1f;
			break;
			*/
		case SFX_ARTILLERY_EXPLOSION:
			pos.z -= scale * 0.5f;
    		obj2d = new Drawable2D( DRAW2D_ARTILLERY_EXPLOSION, scale, &pos, 4, gFireVerts, gFireUvs );
			pos.z += scale * 0.5f;
			secondaryCount = 1;
			break;
		case SFX_GROUND_STRIKE_NOFIRE:
			secondaryCount = 1;
			pos.z -= scale * 0.5f;
			if ( (rand() & 3 ) == 3 )
    			obj2d = new Drawable2D( DRAW2D_GROUND_STRIKE, scale, &pos, 4, gGroundVerts, gFireUvs );
			else
    			obj2d = new Drawable2D( DRAW2D_ARTILLERY_EXPLOSION, scale, &pos, 4, gFireVerts, gFireUvs );
			pos.z += scale * 0.5f;
			break;
		case SFX_GROUND_STRIKE:
			secondaryCount = 1;
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_FIRE4 ] > gSfxLODCutoff ||
				 gSfxCount[ SFX_FIRE5 ] > gSfxLODCutoff )
			{
    			obj2d = new Drawable2D( DRAW2D_GROUND_STRIKE, scale, &pos, 4, gGroundVerts, gFireUvs );
			}
			break;
		case SFX_WATER_STRIKE:
			secondaryCount = 1;
    		obj2d = new Drawable2D( DRAW2D_WATER_STRIKE, scale, &pos, 4, gWaterVerts, gFireUvs );
			break;
		case SFX_CLUSTER_BURST:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_EXPLCROSS_GLOW:
    		obj2d = new Drawable2D( DRAW2D_EXPLCROSS_GLOW, scale, &pos );
			break;
		case SFX_EXPLSTAR_GLOW:
    		obj2d = new Drawable2D( DRAW2D_EXPLSTAR_GLOW, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_EXPLCIRC_GLOW:
    		obj2d = new Drawable2D( DRAW2D_EXPLCIRC_GLOW_FADE, scale, &pos );
			break;
		case SFX_SMOKERING:
    		obj2d = new Drawable2D( DRAW2D_SMOKERING, scale, &pos );
			break;
		case SFX_CRATER2:
    		objBSP = new DrawableGroundVehicle( MapVisId(VIS_CRATER2),  &pos, 0.0f, 1.0f  );
			break;
		case SFX_CRATER3:
    		objBSP = new DrawableGroundVehicle( MapVisId(VIS_CRATER3),  &pos, 0.0f, 1.0f  );
			break;
		case SFX_CRATER4:
    		objBSP = new DrawableGroundVehicle( MapVisId(VIS_CRATER4),  &pos, 0.0f, 1.0f  );
			break;
		case SFX_BURNING_PART:
		case SFX_CLUSTER_BOMB:
			// edg NOTE: should never do this one here.  However, I think there's
			// a problem with the creation of the burning part object and I've seen
			// ACMI try to use this constructor to create the effect.  This shouldn't
			// be a big problem since the fire will be created anyway.  This will just
			// be a kind of NULL effect in ACMI
			break;
//		case SFX_PILOT_SPLAT:
//    		obj2d = new Drawable2D( DRAW2D_GROUND_STRIKE, scale, &pos, 4, gFireVerts, gFireUvs );
		default:
			// VP_changes This should be checked and modified, frequently stops here, yeah. Oct 7, 2002
			ShiWarning ("Bad SFX Type");
			break;
	}

	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}


}

/*
** This is an effect that just drives other secondary effects
** over time
*/
SfxClass::SfxClass( int typeSfx,
  		  Tpoint *posSfx,
		  int count,
		  float interval )
{

	inACMI = FALSE;
	type = typeSfx;
	flags = SFX_SECONDARY_DRIVER;
	pos = *posSfx;
	vec.x = 0.0f;
	vec.y = 0.0f;
	vec.z = 0.0f;
	timeToLive = 0.0f;
	scale = 1.0f;
	secondaryCount = min(count, 100);
	secondaryInterval = interval;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;

	objParticleSys = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	obj2d = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		MonoPrint( "Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}

	switch ( type )
	{
		case SFX_DIST_AIRBURSTS:
		case SFX_DIST_GROUNDBURSTS:
		case SFX_DIST_ARMOR:
		case SFX_DIST_INFANTRY:
		case SFX_DIST_SAMLAUNCHES:
		case SFX_DIST_AALAUNCHES:
			// secondaryCount = (int)( ((float)count) * gSfxLOD );
			// secondaryInterval /= gSfxLOD;
			secondaryInterval = max( interval, 2.5f );
			secondaryTimer += PRANDFloatPos() * 15.0f;
			break;
		default:
			break;
	}


	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}
}


/*
** This is an effect thatr just drives other secondary effects
** over time also has a movement vector
** NOTE: movement vector is assumed to be normalized (depending on usage)
*/
SfxClass::SfxClass( int typeSfx,
  		  Tpoint *posSfx,
  		  Tpoint *vecSfx,
		  int count,
		  float interval )
{

	inACMI = FALSE;
	type = typeSfx;
	flags = SFX_SECONDARY_DRIVER;
	pos = *posSfx;
	vec = *vecSfx;
	timeToLive = 0.0f;
	scale = 1.0f;
	secondaryCount = min(count, 100);
	secondaryInterval = interval;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;

	objParticleSys = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	obj2d = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		MonoPrint( "Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}

	viewPoint = OTWDriver.GetViewpoint();

	// for this type, vec.x and y are the dimensions of the explosion
	// volume ( z is movement up ).  Base the scale on the smaller of the
	// x and y
	if ( type ==  SFX_RISING_GROUNDHIT_EXPLOSION_DEBRISTRAIL )
	{
		scale = min( 400.0f, max( vec.x * 1.5f, vec.y * 1.5f) );
	}

	// for feature chain reaction set scale to vec.x and set the timer to
	// go off at the next interval
	if ( type == SFX_FEATURE_CHAIN_REACTION )
	{
		secondaryTimer += interval;
		scale = vec.x;
	}

	switch ( type )
	{
		case SFX_DIST_AIRBURSTS:
		case SFX_DIST_GROUNDBURSTS:
		case SFX_DIST_ARMOR:
		case SFX_DIST_INFANTRY:
		case SFX_DIST_SAMLAUNCHES:
		case SFX_DIST_AALAUNCHES:
			// secondaryCount = (int)( ((float)count) * gSfxLOD );
			// secondaryInterval /= gSfxLOD;
			secondaryInterval = max( interval, 2.5f );
			secondaryTimer += PRANDFloatPos() * 15.0f;
			travelDist = (float)sqrt( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + 0.1f );
			break;
		default:
			break;
	}

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}
}


/*
**		Move an existing BSP object
*/
SfxClass::SfxClass( int typeSfx,
	  		  Tpoint *posSfx,
	  		  Tpoint *vecSfx,
           	  DrawableBSP* theObject,
			  float timeToLiveSfx,
			  float scaleSfx )
{
	inACMI = FALSE;
	type = typeSfx;
	pos = *posSfx;
	vec = *vecSfx;
	timeToLive = timeToLiveSfx;
	scale = scaleSfx;
	secondaryCount = 0;
	secondaryInterval = 0;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;

	objParticleSys = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = theObject;
	obj2d = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

	viewPoint = OTWDriver.GetViewpoint();
    flags = SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES;

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		//ShiAssert( !"Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}


	switch( type )
	{
		case SFX_BURNING_PART:
			flags = 0;
			secondaryCount = 1;
			break;
		default:
			break;
	}

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}
}

/*
**		Just a timer for when to delete the object
*/
SfxClass::SfxClass ( float timeToLiveSfx,
					 DrawableTrail *trail )
{
	inACMI = FALSE;
	type = SFX_TIMER;
	flags = SFX_TIMER_FLAG;
	timeToLive = timeToLiveSfx;
	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;
	pos.x = 0.0f; // JPO initialise, it is used.
	pos.y = 0.0f;
	pos.z = 0.0f;

	objParticleSys = NULL;
	objTrail = trail;
	objTracer = NULL;
	objBSP = NULL;
	obj2d = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}

}

/*
** Moving(or not) object with rotation
*/
SfxClass::SfxClass (int 	typeSfx,
				   	int  flagsSfx,
				  	Tpoint *posSfx,
					Trotation *rotSfx,
				  	Tpoint *vecSfx,
				  	float timeToLiveSfx,
				 	float scaleSfx )
{

	inACMI = FALSE;
	rot = *rotSfx;
	type = typeSfx;
	flags = flagsSfx;
	pos = *posSfx;
	vec = *vecSfx;
	timeToLive = timeToLiveSfx;
	// scale = scaleSfx * ( 0.6f + 0.4f * gSfxLOD );
	scale = scaleSfx;
	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;

	objParticleSys = NULL;
    obj2d = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;
	travelDist = 0.0f;

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		MonoPrint( "Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}


	switch( type )
	{
		case SFX_AIR_HANGING_EXPLOSION:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_HIT_EXPLOSION, scale, &pos );
			secondaryCount = (int)(timeToLive * 2.0f);
			secondaryInterval = 0.5f;
			break;
		case SFX_AIR_SMOKECLOUD:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			break;
		case SFX_AIR_DUSTCLOUD:
    		obj2d = new Drawable2D( DRAW2D_AIR_DUSTCLOUD, scale, &pos );
			break;
		case SFX_GROUND_DUSTCLOUD:
    		obj2d = new Drawable2D( DRAW2D_GROUND_DUSTCLOUD, scale, &pos );
			break;
		case SFX_WATER_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			break;
		case SFX_STEAM_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_STEAM_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_LIGHT_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			break;
		case SFX_VERTICAL_SMOKE:
    		obj2d = new Drawable2D( DRAW2D_AIR_DUSTCLOUD, scale, &pos, 4, gWaterVerts, gFireUvs );
			break;
		case SFX_AIR_SMOKECLOUD2:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD2, scale, &pos );
			break;
		case SFX_GUNSMOKE:
    		obj2d = new Drawable2D( DRAW2D_GUNSMOKE, scale, &pos );
			break;
		case SFX_RAND_CRATER:
    		obj2d = new Drawable2D( DRAW2D_CRATER1 + PRANDInt3(), scale, &pos, &rot );
			break;
		case SFX_CRATER2:
    		objBSP = new DrawableGroundVehicle( MapVisId(VIS_CRATER2),  &pos, 0.0f, 1.0f  );
			break;
		case SFX_CRATER3:
    		objBSP = new DrawableGroundVehicle( MapVisId(VIS_CRATER3),  &pos, 0.0f, 1.0f  );
			break;
		case SFX_CRATER4:
    		objBSP = new DrawableGroundVehicle( MapVisId(VIS_CRATER4),  &pos, 0.0f, 1.0f  );
			break;
		case SFX_TRAIL_SMOKECLOUD:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			objTrail = new DrawableTrail(TRAIL_THINFIRE);
			break;
		case SFX_TRAIL_FIREBALL:
    		obj2d = new Drawable2D( DRAW2D_FLARE, scale, &pos );
			objTrail = new DrawableTrail(TRAIL_THINFIRE);
			break;
		case SFX_NOTRAIL_FLARE:
    		obj2d = new Drawable2D( DRAW2D_FLARE, scale, &pos );
			break;
		case SFX_EJECT1:
    		objBSP = new DrawableBSP( MapVisId(VIS_EJECT1), &pos, &rot, scale );
			break;
		case SFX_EJECT2:
    		objBSP = new DrawableBSP( MapVisId(VIS_EJECT2), &pos, &rot, scale );
			break;
		/*
		case SFX_MISSILE_LAUNCH:
    		objBSP = new DrawableBSP( VIS_MISS_LAUN, &pos, &rot, scale );
			break;
		*/
		case SFX_DUST1:
    		objBSP = new DrawableBSP( MapVisId(VIS_DUST1), &pos, &rot, scale );
			break;
		default:
			// bzzzzt
			ShiWarning ("Bad SFX Type");
			break;
	}

	viewPoint = OTWDriver.GetViewpoint();


	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}
}

/*
** Moving object
*/
SfxClass::SfxClass (int 	typeSfx,
				   	int  flagsSfx,
				  	Tpoint *posSfx,
				  	Tpoint *vecSfx,
				  	float timeToLiveSfx,
				 	float scaleSfx )
{
	float len;

	inACMI = FALSE;
	type = typeSfx;
	flags = flagsSfx;
	pos = *posSfx;
	vec = *vecSfx;
	timeToLive = timeToLiveSfx;
	// scale = scaleSfx * ( 0.6f + 0.4f * gSfxLOD );
	scale = scaleSfx;
	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	travelDist = 0.0f;

	objParticleSys = NULL;
    obj2d = NULL;
	objTrail = NULL;
	objTracer = NULL;
	objBSP = NULL;
	baseObj = NULL;
	endMessage = NULL;
	damMessage = NULL;

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		MonoPrint("Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}

	switch( type )
	{
		case SFX_SPARK_TRACER:
    		objTracer = new DrawableTracer( 0.1f * scale);
	  		objTracer->SetRGB( 1.0f, 1.0f, 0.8f );
	  		objTracer->SetAlpha( 1.0f );
	  		objTracer->SetWidth( 0.1f * scale );
			break;
		case SFX_SPARKS:
    		obj2d = new Drawable2D( DRAW2D_SPARKS, scale * 0.2f, &pos, 4, gGroundVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			secondaryCount = (int)(timeToLive * 10.0f);
			secondaryInterval = 0.1f;
			break;
		case SFX_EXPLSTAR_GLOW:
    		obj2d = new Drawable2D( DRAW2D_EXPLSTAR_GLOW, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE_EXPAND:
    		obj2d = new Drawable2D( DRAW2D_FIRE_EXPAND, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			secondaryCount = (int)(timeToLive * 1.0f);
			secondaryInterval = 1.0f;
		case SFX_FIRE_EXPAND_NOSMOKE:
    		obj2d = new Drawable2D( DRAW2D_FIRE_EXPAND, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_CLUSTER_BOMB:
    		// obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			secondaryCount = 1;
			break;
		case SFX_WATER_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			timeToLive= min(timeToLiveSfx, timeToLive); // JPO allow shorter
			break;
		case SFX_WATER_WAKE:
			//objTrail = new DrawableTrail(TRAIL_AIM120);
			rot = IMatrix;
			obj2d = new Drawable2D(DRAW2D_WATERWAKE, scale, &pos, &rot);
			timeToLive = obj2d->GetAlphaTimeToLive();
			timeToLive= min(timeToLiveSfx, timeToLive); // JPO allow shorter
			break;

		case SFX_BLUE_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_BLUE_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_SHAPED_FIRE_DEBRIS:
			// we need to set the x basis vector in the matrix based on
			// the velocity of this object
			len = (float)sqrt( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );
			rot.M11 = vec.x/len;
			rot.M12 = vec.y/len;
			rot.M13 = vec.z/len;
    		obj2d = new Drawable2D( DRAW2D_SHAPED_FIRE_DEBRIS, scale, &pos, &rot );
			break;
		case SFX_SMALL_HIT_EXPLOSION:
    		obj2d = new Drawable2D( DRAW2D_SMALL_HIT_EXPLOSION, scale, &pos );
			break;
		case SFX_AIR_HANGING_EXPLOSION:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_HIT_EXPLOSION, scale, &pos );
			secondaryCount = (int)(timeToLive * 2.0f);
			secondaryInterval = 0.5f;
			break;
		case SFX_MISSILE_BURST:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_SMALL_HIT_EXPLOSION, scale, &pos );
			secondaryCount = 1;
			break;
		case SFX_ROCKET_BURST:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_SMALL_HIT_EXPLOSION, scale, &pos );
			secondaryCount = 1;
			break;
		case SFX_EXPLCIRC_GLOW:
    		obj2d = new Drawable2D( DRAW2D_EXPLCIRC_GLOW_FADE, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_DARK_DEBRIS:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_DARK_DEBRIS, scale, &pos );
			break;
		case SFX_FIRE_DEBRIS:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_FIRE_DEBRIS, scale, &pos );
			break;
		case SFX_FLAME:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_FLAME, scale, &pos );
			break;
		case SFX_LIGHT_DEBRIS:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
    		obj2d = new Drawable2D( DRAW2D_LIGHT_DEBRIS, scale, &pos );
			break;
		case SFX_GROUNDBURST:
    		// obj2d = new Drawable2D( DRAW2D_AIR_EXPLOSION1, scale, &pos );
			if ( rand() & 1 )
    			obj2d = new Drawable2D( DRAW2D_LIGHT_DEBRIS, scale, &pos );
			else
    			obj2d = new Drawable2D( DRAW2D_DARK_DEBRIS, scale, &pos );
			break;
		case SFX_SMOKETRAIL:
			objTrail = new DrawableTrail(TRAIL_MISLSMOKE);
			break;
		case SFX_FIRE1:
    		obj2d = new Drawable2D( DRAW2D_FIRE1, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE2:
    		obj2d = new Drawable2D( DRAW2D_FIRE2, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE3:
    		obj2d = new Drawable2D( DRAW2D_FIRE3, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE4:
    		obj2d = new Drawable2D( DRAW2D_FIRE4, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE5:
    		obj2d = new Drawable2D( DRAW2D_FIRE5, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_FIRE6:
    		obj2d = new Drawable2D( DRAW2D_FIRE6, scale, &pos, 4, gFireVerts, gFireUvs );
			timeToLive = obj2d->GetAlphaTimeToLive();
			break;
		case SFX_DEBRISTRAIL:

			objTrail = new DrawableTrail(TRAIL_DARKSMOKE);
			if ( (rand() & 3 ) == 3 )
    			obj2d = new Drawable2D( DRAW2D_DARK_DEBRIS, 12.0f, &pos );

			/*
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_TRAILSMOKE ] > gSfxLODCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_SMOKE);
				objTrail->SetScale( max( 1.0f, scale * 0.06f ) );
			}
			else
			{
				secondaryInterval = 1.0f * (1.0f-gSfxLOD) + 0.1f;
				secondaryCount = (int)(timeToLive * (1.0f/secondaryInterval) );
			}
			*/
			break;
		case SFX_SAM_LAUNCH:
    		// obj2d = new Drawable2D( DRAW2D_STEAM_CLOUD, scale, &pos );
			secondaryCount = 1;
			break;
		case SFX_DEBRISTRAIL_DUST:
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_TRAILDUST ] > gSfxLODCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_SMOKE);
				objTrail->SetScale( max( 1.0f, scale * 0.06f ) );
			}
			else
			{
				secondaryInterval = 1.0f * (1.0f-gSfxLOD) + 0.1f;
				secondaryCount = (int)(timeToLive * (1.0f/secondaryInterval) );
			}
			break;
		case SFX_FIRETRAIL:
			objTrail = new DrawableTrail(TRAIL_FIRE2);

			// edg: weird crash on this -- leave it out for now....
    		// obj2d = new Drawable2D( DRAW2D_DARK_DEBRIS, 10.0f, &pos );

			/*
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_FIRE_EXPAND ] > gSfxLODCutoff ||
				 gSfxCount[ SFX_FIRE_EXPAND_NOSMOKE ] > gSfxLODCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_FIRE1);
				objTrail->SetScale( max( 1.0f, scale * 0.06f ) );
			}
			else
			{
				secondaryInterval = 1.0f * (1.0f-gSfxLOD) + 0.1f;
				secondaryCount = (int)(timeToLive * (1.0f/secondaryInterval) );
			}
			*/
			break;
		case SFX_FIREBALL:
			if ( gTotSfx >= gSfxLODTotCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_FIRE1);
				objTrail->SetScale( max( 1.0f, scale * 0.06f ) );
			}
			else
			{
				secondaryInterval = 1.0f * (1.0f-gSfxLOD) + 0.3f;
				secondaryCount = (int)(timeToLive * (1.0f/secondaryInterval) );
				initSecondaryCount = secondaryCount;
			}
			break;
		case SFX_WATER_FIREBALL:
			if ( gTotSfx >= gSfxLODTotCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_FIRE1);
				objTrail->SetScale( max( 1.0f, scale * 0.06f ) );
			}
			else
			{
				secondaryInterval = 1.0f * (1.0f-gSfxLOD) + 0.3f;
				secondaryCount = (int)(timeToLive * (1.0f/secondaryInterval) );
				initSecondaryCount = secondaryCount;
			}
			break;
		case SFX_WATERTRAIL:
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_WATER_CLOUD ] > gSfxLODCutoff ||
				 gSfxCount[ SFX_BLUE_CLOUD ] > gSfxLODCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_AIM120);
				objTrail->SetScale( max( 1.0f, scale * 0.06f ) );
			}
			else
			{
				secondaryInterval = 1.0f * (1.0f-gSfxLOD) + 0.2f;
				secondaryCount = (int)(timeToLive * (1.0f/secondaryInterval) );
				initSecondaryCount = secondaryCount;
			}
			break;
		case SFX_DURANDAL:
			objTrail = new DrawableTrail(TRAIL_AIM120);
			secondaryCount = 1;
			break;
		case SFX_AIR_SMOKECLOUD:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_FIRESMOKE:
			if ( rand() & 1 )
    			obj2d = new Drawable2D( DRAW2D_FIRESMOKE, scale, &pos );
			else
    			obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_TRAILSMOKE:
    		obj2d = new Drawable2D( DRAW2D_TRAILSMOKE, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 1.0f;
			vec.y += gWindVect.y * 1.0f;
			break;
		case SFX_BIG_SMOKE:
			if ( rand() & 1 )
    			obj2d = new Drawable2D( DRAW2D_BIG_SMOKE1, scale, &pos );
			else
    			obj2d = new Drawable2D( DRAW2D_BIG_SMOKE2, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 1.0f;
			vec.y += gWindVect.y * 1.0f;
			break;
		case SFX_BIG_DUST:
    		obj2d = new Drawable2D( DRAW2D_BIG_DUST, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 1.0f;
			vec.y += gWindVect.y * 1.0f;
			break;
		case SFX_TRAILDUST:
    		obj2d = new Drawable2D( DRAW2D_TRAILDUST, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 1.0f;
			vec.y += gWindVect.y * 1.0f;
			break;
		case SFX_AIR_DUSTCLOUD:
    		obj2d = new Drawable2D( DRAW2D_AIR_DUSTCLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_GROUND_DUSTCLOUD:
			if ( gSfxCount[ SFX_GROUND_DUSTCLOUD ] < gSfxLODCutoff &&
				 gTotSfx < gSfxLODTotCutoff )
			{
    			obj2d = new Drawable2D( DRAW2D_GROUND_DUSTCLOUD, scale, &pos );
				timeToLive = obj2d->GetAlphaTimeToLive();
			}
			else
			{
				timeToLive = 0.0f;
			}
			vec.x += gWindVect.x * 1.0f;
			vec.y += gWindVect.y * 1.0f;
			break;
		case SFX_LIGHT_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_WATER_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_STEAM_CLOUD:
    		obj2d = new Drawable2D( DRAW2D_STEAM_CLOUD, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_AIR_SMOKECLOUD2:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD2, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_GUNSMOKE:
    		obj2d = new Drawable2D( DRAW2D_GUNSMOKE, scale, &pos );
			timeToLive = obj2d->GetAlphaTimeToLive();
			vec.x += gWindVect.x * 2.0f;
			vec.y += gWindVect.y * 2.0f;
			break;
		case SFX_TRAIL_SMOKECLOUD:
    		obj2d = new Drawable2D( DRAW2D_SMOKECLOUD1, scale, &pos );
			objTrail = new DrawableTrail(TRAIL_THINFIRE);
			vec.x += gWindVect.x * 1.0f;
			vec.y += gWindVect.y * 1.0f;
			break;
		case SFX_MISSILE_LAUNCH:
    		obj2d = new Drawable2D( DRAW2D_MISSILE_GLOW, scale, &pos );
			objTrail = new DrawableTrail(TRAIL_DISTMISSILE);
			break;
		case SFX_TRAIL_FIREBALL:
    		obj2d = new Drawable2D( DRAW2D_FLARE, scale, &pos );
			objTrail = new DrawableTrail(TRAIL_THINFIRE);
			break;
		case SFX_TRAIL_FIRE:
			objTrail = new DrawableTrail(TRAIL_THINFIRE);
			break;
		case SFX_NOTRAIL_FLARE:
    		obj2d = new Drawable2D( DRAW2D_FLARE, scale, &pos );
			break;
		case SFX_EJECT1:
			// rot = IMatrix;
			// at the moment ejection comes out sideways, swap vectors
			rot.M11 = 1.0f;
			rot.M12 = 0.0f;
			rot.M13 = 0.0f;
			rot.M21 = 0.0f;
			rot.M22 = 0.0f;
			rot.M23 = -1.0f;
			rot.M31 = 0.0f;
			rot.M32 = 1.0f;
			rot.M33 = 0.0f;
			secondaryCount = 1;
    		objBSP = new DrawableBSP( MapVisId(VIS_EJECT1), &pos, &rot, scale );
			break;
		case SFX_EJECT2:
			rot = IMatrix;
    		objBSP = new DrawableBSP( MapVisId(VIS_EJECT2), &pos, &rot, scale );
			break;
		case SFX_TRACER_FIRE:
    		objTracer = new DrawableTracer(scale);
	  		objTracer->SetRGB( 1.0f, 1.0f, 0.5f );
	  		objTracer->SetAlpha( 1.0f );
	  		objTracer->SetWidth( 2.0f * scale );
			break;
		case SFX_GUN_TRACER:
    		objTracer = new DrawableTracer(scale);
	  		objTracer->SetRGB( 1.0f, 1.0f, 0.5f );
	  		objTracer->SetAlpha( 1.0f );
	  		objTracer->SetWidth( 2.0f * scale );
			break;
		case SFX_VERTICAL_SMOKE:
    		obj2d = new Drawable2D( DRAW2D_AIR_DUSTCLOUD, scale, &pos, 4, gWaterVerts, gFireUvs );
			break;
		default:
			// VP_changes. This should be checked or modified - yeah. Oct 7, 2002.
			ShiWarning ("Bad SFX Type");
			break;
	}

	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}

}


/*
** Moving object
*/
SfxClass::SfxClass (int 	typeSfx,
					int  flagsSfx,
					SimBaseClass *baseobjSfx,
					float timeToLiveSfx,
					float scaleSfx )
{

	// ShiAssert(baseobjSfx && !vuDatabase->Find(baseobjSfx->Id()) && baseobjSfx->VuState() == VU_MEM_CREATED);

	inACMI = FALSE;
	type = typeSfx;
	flags = flagsSfx;
	baseObj = baseobjSfx;
	vec.x = baseObj->XDelta();
	vec.y = baseObj->YDelta();
	vec.z = baseObj->ZDelta();
	timeToLive = timeToLiveSfx;
	scale = scaleSfx;
	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	OTWDriver.ObjectSetData( baseObj, &pos, &rot );
	objParticleSys = NULL;
    obj2d = NULL;
    objBSP = NULL;
    objTrail = NULL;
	objTracer = NULL;
	endMessage = NULL;
	damMessage = NULL;
	travelDist = 0.0f;

  F4Assert( vec.x > -10000.0F && vec.y > -10000.0F && vec.z > -10000.0F);
  F4Assert( vec.x <  10000.0F && vec.y <  10000.0F && vec.z <  10000.0F);

	if ( pos.x > -10000.0f && pos.x < 10000000.0f &&
		 pos.y > -10000.0f && pos.y < 10000000.0f &&
		 pos.z < 8000.0f && pos.z > -150000.0f )
 	{
	}
	else
	{
		//ShiAssert( !"Bad SFX Position Passed in!" );
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
	}

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}

	switch( type )
	{
		case SFX_SMOKING_PART:
			objTrail = new DrawableTrail(TRAIL_SMOKE);
			if ( gTotSfx <= gSfxLODTotCutoff &&
				 gSfxCount[ SFX_AIR_SMOKECLOUD ] < gSfxLODCutoff )
			{
				// for smoking part, randomly allow piece to smoke for a
				// while and hit ground and stay there
				if ( PRANDInt3() == 0 )
				{
					timeToLive += 90.0f;
					flags &= ~SFX_EXPLODE_WHEN_DONE;
				}

				secondaryInterval = 1.0f;
				secondaryCount = (int)(timeToLive * 1.0f);
			}

			break;
		case SFX_FLAMING_PART:
			if ( gTotSfx >= gSfxLODTotCutoff ||
				 gSfxCount[ SFX_FIRE1 ] > gSfxLODCutoff )
			{
				objTrail = new DrawableTrail(TRAIL_FIRE1);
			}
			else
			{
				secondaryInterval = 0.1f;
				secondaryCount = (int)(timeToLive * 10.0f);
			}
			break;
		case SFX_BURNING_PART:
			secondaryCount = 1;
			break;
		case SFX_SMOKING_FEATURE:
		case SFX_STEAMING_FEATURE:
			// smoking features is a special case of effect.  It's used
			// for having smoke rising off of smoke stacks.  The flags passed
			// in will tell us which slot to use for the hardpoint

			// we MUST do a VuRef so that the feature isn't deleted out
			// from under us
			VuReferenceEntity( baseObj );

			// temporarily use the objBSP ptr
			objBSP = (DrawableBSP *)baseObj->drawPointer;
			objBSP->GetChildOffset( flags, &pos );

			// get rotated offset
			vec.x = pos.x * objBSP->orientation.M11 + pos.y * objBSP->orientation.M12;
			vec.y = pos.x * objBSP->orientation.M21 + pos.y * objBSP->orientation.M22;
			vec.z = pos.z;

			objBSP->GetPosition( &pos );

			switch(type) // MLR 12/19/2003 - Using New Trails for feature smoke!
			{
				case SFX_SMOKING_FEATURE:
				case SFX_STEAMING_FEATURE:
					objTrail=new DrawableTrail(TRAIL_FEATURESMOKE);
					Tpoint vel={0,0,-5};					
					objTrail->SetHeadVelocity(&vel);
					break;
			}

			/*
			pos.x += vec.x;
			pos.y += vec.y;
			pos.z += vec.z;
			*/
			// we're done borrowing this
			objBSP = NULL;

			// flags = SFX_TIMER_FLAG; // MLR 12/20/2003 - Can't have this set anymore

			break;
		default:
			// bzzzzt
			ShiWarning ("Bad SFX Type");
			break;
	}

	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx )
	{
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] )
	{
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}

}



////////////////////////////
// F16 crash landing
SfxClass::SfxClass (int typeSfx,
					int flagsSfx,
					SimBaseClass *baseobjSfx,
					float timeToLiveSfx,
					float scaleSfx,
					Tpoint *slot,
					float restpitch, float restroll)
{
	inACMI = FALSE;
	type = typeSfx;
	flags = flagsSfx;
	baseObj = baseobjSfx;
	vec.x = baseObj->XDelta();
	vec.y = baseObj->YDelta();
	vec.z = baseObj->ZDelta();
	timeToLive = timeToLiveSfx;
	scale = scaleSfx;
	RestPitch = restpitch;
	RestRoll = restroll;
	CrashSlot = *slot;
	CrashPos.x = baseObj->XPos();
	CrashPos.y = baseObj->YPos();
	CrashPos.z = baseObj->ZPos();

	secondaryCount = 0;
	secondaryInterval = 0.0f;
	secondaryTimer = (float)SimLibElapsedTime / SEC_TO_MSEC;
	OTWDriver.ObjectSetData( baseObj, &pos, &rot );

	objParticleSys = NULL;
    obj2d = NULL;
    objBSP = NULL;
    objTrail = NULL;
	objTracer = NULL;
	endMessage = NULL;
	damMessage = NULL;
	travelDist = 0.0f;

	if(TryParticleEffect()) // if it succeeds then we can just exit 
	{
		return;
	}



	switch( type ) {
		case SFX_SMOKING_PART:
			objTrail = new DrawableTrail(TRAIL_SMOKE);
			if ( gTotSfx <= gSfxLODTotCutoff && gSfxCount[ SFX_AIR_SMOKECLOUD ] < gSfxLODCutoff ) {
				timeToLive += 90.0f;
				flags &= ~SFX_EXPLODE_WHEN_DONE;
				secondaryInterval = 1.0f;
				secondaryCount = FloatToInt32 (timeToLive);
			}
			break;
		default:
			break;
	}

	viewPoint = OTWDriver.GetViewpoint();

	// update counters
	gTotSfx++;
	if ( gTotSfx > gTotHighWaterSfx ) {
		gTotHighWaterSfx = gTotSfx;
//		MonoPrint( "SFX Total High Water Mark Reached = %d\n", gTotHighWaterSfx );
	}
	gSfxCount[ type ]++;
	if ( gSfxCount[type] > gSfxHighWater[type] ) {
		gSfxHighWater[type] = gSfxCount[type];
//		MonoPrint( "SFX Type %d High Water Reached = %d\n", type, gSfxHighWater[type] );
	}

}
////////////////////////////


/*
** Name: SfxClass Destructor
** Description:
**		Cleans up memory associated with sfx and removes objects
**		from display lists
*/
SfxClass::~SfxClass (void)
{
	if ( inACMI == FALSE )
	{
		if ( objParticleSys)
		{
			OTWDriver.RemoveObject(objParticleSys, TRUE);
		}
		if ( obj2d )
		{
			OTWDriver.RemoveObject(obj2d, TRUE);
		}
	
		if ( objBSP )
		{
			OTWDriver.RemoveObject(objBSP, TRUE);
		}
	
		// Smoketrails are removed
		// by a timer special effect.
		if ( objTrail )
		{
			OTWDriver.RemoveObject(objTrail, TRUE);
			objTrail = NULL;
		}
	
		if ( objTracer )
		{
			OTWDriver.RemoveObject(objTracer, TRUE);
		}
	
		if ( baseObj )
		{
   			if ( type != SFX_SMOKING_FEATURE && type != SFX_STEAMING_FEATURE )
			{
				if (baseObj->drawPointer)
				{
					OTWDriver.RemoveObject(baseObj->drawPointer, TRUE);
					baseObj->drawPointer = NULL;
				}
	
				// delete the object
				delete baseObj;
			}
			else
			{
				VuDeReferenceEntity( baseObj );
			}
			baseObj = NULL;
		}

		if ( endMessage )
		{
			delete endMessage;
			endMessage = NULL;
		}

		if ( damMessage )
		{
			delete damMessage;
			damMessage = NULL;
		}
	}
	// destruction when running from ACMI
	else
	{
		if ( obj2d )
		{
			if ( obj2d->InDisplayList() )
			{
				viewPoint->RemoveObject( obj2d );
			}
			delete obj2d;
		}
	
		if ( objBSP )
		{
			if ( objBSP->InDisplayList() )
			{
				viewPoint->RemoveObject( objBSP );
			}
			delete objBSP;
		}
	
		if ( objTrail )
		{
			if ( objTrail->InDisplayList() )
			{
				viewPoint->RemoveObject( objTrail );
			}
			delete objTrail;
		}
	
		if ( objTracer )
		{
			if ( objTracer->InDisplayList() )
			{
				viewPoint->RemoveObject( objTracer );
			}
			delete objTracer;
		}
	
		if ( baseObj )
		{
			if ( baseObj->drawPointer->InDisplayList() )
			{
				viewPoint->RemoveObject( baseObj->drawPointer );
			}

			// Get rid of the drawable
			delete baseObj->drawPointer;
			baseObj->drawPointer = NULL;

			// delete the object
			delete baseObj;
		}
	}

	// update counters
	gTotSfx--;

	if (type >= 0 && !F4IsBadReadPtr(gSfxCount, sizeof(int))) // JB 010318 CTD
		gSfxCount[ type ]--;
}


/*
** Name: Start
** Description:
**		Starts the effect by adding it to the draw list.
**		And sets the timetolive
*/
void
SfxClass::Start (void)
{
	float sinAng, cosAng;

	// if it's got a trajectory, we need to recalc
	if ( flags & SFX_TRAJECTORY )
	{
		timeToLive = min( timeToLive * 2.0f, MAX_TIME_TO_LIVE );

		sinAng = GRAVITY * 0.5f * timeToLive / TRACER_VELOCITY;
		cosAng = (float)sqrt( 1.0f - sinAng * sinAng );
		vec.x *= cosAng;
		vec.y *= cosAng;
		vec.z =  vec.z * cosAng - TRACER_VELOCITY * sinAng;
	}

	// check acmi recording
	if ( gACMIRec.IsRecording() )
	{
		// stationary sfx have no flags
		if ( flags == 0 )
		{
			acmiStatSfx.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			acmiStatSfx.data.type = type;
			acmiStatSfx.data.x = pos.x;
			acmiStatSfx.data.y = pos.y;
			acmiStatSfx.data.z = pos.z;
			acmiStatSfx.data.timeToLive = timeToLive;
			acmiStatSfx.data.scale = scale;
			gACMIRec.StationarySfxRecord( &acmiStatSfx );
		}
		else if ( flags & SFX_MOVES || type == SFX_BURNING_PART )
		{
			acmiMoveSfx.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			acmiMoveSfx.data.type = type;
			acmiMoveSfx.data.flags = flags;
			acmiMoveSfx.data.user = -1;
			acmiMoveSfx.data.x = pos.x;
			acmiMoveSfx.data.y = pos.y;
			acmiMoveSfx.data.z = pos.z;
			acmiMoveSfx.data.dx = vec.x;
			acmiMoveSfx.data.dy = vec.y;
			acmiMoveSfx.data.dz = vec.z;
			acmiMoveSfx.data.timeToLive = timeToLive;
			acmiMoveSfx.data.scale = scale;
			// if we've got a base obj, store it's vistype in user for
			// later reconstruction
			if ( baseObj )
			{
				acmiMoveSfx.data.user = ((DrawableBSP *)baseObj->drawPointer)->GetID();
			}
			gACMIRec.MovingSfxRecord( &acmiMoveSfx );
		}
	}

	// set the time it will die
	timeToLive += SimLibElapsedTime / SEC_TO_MSEC;

	// get the approximate distance to the viewer
	GetApproxViewDist( (float)SimLibElapsedTime / SEC_TO_MSEC );

	// timers don't get added to draw list
	if ( flags & SFX_TIMER_FLAG )
		return;

	// insert the effect's object into the display list
	if ( objParticleSys ) // MLR 2/3/2004 - 
	{
		OTWDriver.InsertObject(objParticleSys);
	}
	if ( obj2d )
	{
		OTWDriver.InsertObject(obj2d);
	}
	if ( objBSP )
	{
		OTWDriver.InsertObject(objBSP);
	}
	if ( objTrail )
	{
		// a hack -- scale trail of missile launch based on dist
		if ( type == SFX_MISSILE_LAUNCH )
		{
			float distScale = max( 0.1f, approxDist/10000.0f );
			objTrail->SetScale( distScale );
		}
		OTWDriver.InsertObject(objTrail);

		// put in the 1st point at starting position
		OTWDriver.AddTrailHead (objTrail, pos.x, pos.y, pos.z);
	}
	if ( objTracer )
	{
		OTWDriver.InsertObject(objTracer);
	}
	if ( baseObj)
	{ 
		if ( type != SFX_SMOKING_FEATURE && type != SFX_STEAMING_FEATURE ) // MLR 12/20/2003 - added
		{
			OTWDriver.InsertObject(baseObj->drawPointer);
		}
	}

}

///////////////////////////////////
void SfxClass::CalculateRestingObjectMatrix (float pitch, float roll, float mat[3][3])
{
mlTrig trig;

   mlSinCos (&trig, roll);
	float rs = trig.sin;
	float rc = trig.cos;

   mlSinCos (&trig, pitch);
	float ps = trig.sin;
	float pc = trig.cos;
	mat[0][0] =  pc;
	mat[0][1] =  0.0f;
	mat[0][2] = -ps;
	mat[1][0] =  ps * rs;
	mat[1][1] =  rc;
	mat[1][2] =  pc * rs;
	mat[2][0] =  ps * rc;
	mat[2][1] = -rs;
	mat[2][2] =  pc * rc;
}

void SfxClass::CalculateGroundMatrix (Tpoint *normal, float yaw, float mat[3][3])
{
mlTrig trig;

   mlSinCos (&trig, yaw);
	float yc = trig.cos;
	float ys = trig.sin;
	float Nx, Ny, Nz, x, y, z, s;
	Nx = -normal -> x;
	Ny = -normal -> y;
	Nz = -normal -> z;
	s =  1.0f / (float) sqrt ( Nx*Nx + Ny*Ny + Nz*Nz );
	mat[2][0] = Nx*s;
	mat[2][1] = Ny*s;
	mat[2][2] = Nz*s;
	x =  Nz*yc;
	y =  Nz*ys;
	z = -Nx*yc -Ny*ys;
	s =  1.0f / (float) sqrt ( x*x + y*y + z*z );
	mat[0][0] = x*s;
	mat[0][1] = y*s;
	mat[0][2] = z*s;
	x = -Nz*ys;
	y =  Nz*yc;
	z =  Nx*ys - Ny*yc;
	s =  1.0f / (float) sqrt ( x*x + y*y + z*z );
	mat[1][0] = x*s;
	mat[1][1] = y*s;
	mat[1][2] = z*s;
}

void SfxClass::MultiplyMatrix (float result[3][3], float mat[3][3], float mat1[3][3])
{
	result[0][0] = mat[0][0]*mat1[0][0] + mat[1][0]*mat1[0][1] + mat[2][0]*mat1[0][2];
	result[1][0] = mat[0][0]*mat1[1][0] + mat[1][0]*mat1[1][1] + mat[2][0]*mat1[1][2];
	result[2][0] = mat[0][0]*mat1[2][0] + mat[1][0]*mat1[2][1] + mat[2][0]*mat1[2][2];
	result[0][1] = mat[0][1]*mat1[0][0] + mat[1][1]*mat1[0][1] + mat[2][1]*mat1[0][2];
	result[1][1] = mat[0][1]*mat1[1][0] + mat[1][1]*mat1[1][1] + mat[2][1]*mat1[1][2];
	result[2][1] = mat[0][1]*mat1[2][0] + mat[1][1]*mat1[2][1] + mat[2][1]*mat1[2][2];
	result[0][2] = mat[0][2]*mat1[0][0] + mat[1][2]*mat1[0][1] + mat[2][2]*mat1[0][2];
	result[1][2] = mat[0][2]*mat1[1][0] + mat[1][2]*mat1[1][1] + mat[2][2]*mat1[1][2];
	result[2][2] = mat[0][2]*mat1[2][0] + mat[1][2]*mat1[2][1] + mat[2][2]*mat1[2][2];
}

void SfxClass::TransformPoint (Tpoint *result, Tpoint *point, float mat[3][3])
{
	result -> x = point -> x * mat[0][0] + point -> y * mat[1][0] + point -> z * mat[2][0];
	result -> y = point -> x * mat[0][1] + point -> y * mat[1][1] + point -> z * mat[2][1];
	result -> z = point -> x * mat[0][2] + point -> y * mat[1][2] + point -> z * mat[2][2];
}

void SfxClass::CopyMatrix (float result[3][3], float mat[3][3])
{
	result[0][0] = mat[0][0];
	result[1][0] = mat[1][0];
	result[2][0] = mat[2][0];
	result[0][1] = mat[0][1];
	result[1][1] = mat[1][1];
	result[2][1] = mat[2][1];
	result[0][2] = mat[0][2];
	result[1][2] = mat[1][2];
	result[2][2] = mat[2][2];
}

void SfxClass::GetOrientation (float mat[3][3], float *yaw, float *pitch, float *roll)
{
	*pitch = -(float)asin(mat[0][2]);
	*yaw   = (float)atan2(mat[0][1], mat[0][0]);
	*roll  = (float)atan2(mat[1][2], mat[2][2]);
}

float SfxClass::AdjustAngle180 (float angle)
{
	while (angle >  PI*2) angle -= PI*2;
	while (angle < -PI*2) angle += PI*2;
	if (angle > PI) angle -= PI*2;
	return angle;
}

int SfxClass::RestPiece (float *angle, float rest, float multiplier, float min)
{
	int stopit = 0;
	*angle = AdjustAngle180(*angle);
	float angle1 = (rest - *angle);
	if (fabs(angle1) < min) stopit = 1;
	else {
		angle1 *= multiplier;
		*angle += angle1;
	}
	return stopit;
}


#define	F16CRASH_MAXSOUND	5
#define F16CRASH_BUMPMASK	0x01
#define F16CRASH_FLOATMASK	0x02
#define F16CRASH_BOOMMASK	0x04
#define F16CRASH_DRAGMASK	0x08
#define F16CRASH_FELLMASK	0x10

void PlayCrashSound(int mask, int soundindex, int flag=1, int time=0, Tpoint *pos = (Tpoint *)0)
{
	static int timePlaying[F16CRASH_MAXSOUND];
	static int SoundIndex[F16CRASH_MAXSOUND];
	static int DelayTime[F16CRASH_MAXSOUND] = { 100, 1000, 500, 50, 50 };
	int	i, index;

	if (flag) {	
		index = 0;
		for (i=0; i < F16CRASH_MAXSOUND; i++) {
			if (mask & 1) {
				SoundIndex[index] = soundindex;
				break;
			}
			mask >>= 1;
			index++;
		}
	}
	else {		// play sound
		index = 0;
		for (i=0; i < F16CRASH_MAXSOUND; i++) {
			if (mask & 1) {
				if (timePlaying[index] > time + DelayTime[index]) timePlaying[index] = 0;
				if (timePlaying[index] < time) {
					timePlaying[index] = time + DelayTime[index];
					F4SoundFXSetPos(SoundIndex[index], TRUE, pos -> x, pos -> y, pos -> z, 1.0f );
				}
			}
			mask >>= 1;
			index++;
		}
	}
}

///////////////////////////////////



/*
** Name: Exec
** Description:
**		Executes the effect
**		Returns FALSE when completed
*/
BOOL
SfxClass::Exec (void)
{
	float groundZ=0.0F;
    int groundType=COVERAGE_PLAINS;
	BOOL hitGround = FALSE;


	if(objParticleSys)
	{
		objParticleSys->Exec();
		if(objParticleSys->HasParticles())
			return TRUE;
		return FALSE;
	}

	// get approx distance to viewer based on timer
	if ( SimLibElapsedTime / SEC_TO_MSEC >= distTimer )
		GetApproxViewDist((float)SimLibElapsedTime / SEC_TO_MSEC );

	// special case smoking feature -- they continue to run until the
	// feature goes to sleep
	if ( type == SFX_SMOKING_FEATURE || type == SFX_STEAMING_FEATURE )
	{
		// if we'vee got no base object return FALSE
		if ( baseObj == NULL )
			return FALSE;

		// check to see if the object is awake or if its state
		// is no longer OK
		if ( !baseObj->IsAwake() ||
		     (baseObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED ||
		     (baseObj->Status() & VIS_TYPE_MASK) == VIS_DAMAGED ||
			 baseObj->IsDead() )
		{
			// deref the entity
			VuDeReferenceEntity( baseObj );

			baseObj = NULL;

			// we're done
			return FALSE;
		}
		// run any secondary effects here
		if ( SimLibElapsedTime / SEC_TO_MSEC >= secondaryTimer )
		{
			// temporarily use the objBSP ptr
			// se need to keep getting the base position since LOD may change
			// on terrain
			objBSP = (DrawableBSP *)baseObj->drawPointer;
			objBSP->GetPosition( &pos );

			pos.x += vec.x;
			pos.y += vec.y;
			pos.z += vec.z;

			// we're done borrowing this
			objBSP = NULL;

			groundZ = approxDist/30000.0f;
			secondaryTimer = SimLibElapsedTime / SEC_TO_MSEC + 0.50f + 2.5F * groundZ;
	
			RunSecondarySfx();
		}

		Draw();
		return TRUE;

	}

	if ( SimLibElapsedTime >= gWindTimer )
	{
		mlTrig trig;
		float hdg,vel;

		gWindTimer = SimLibElapsedTime + 120000;


		hdg = ((WeatherClass*)realWeather)->WindHeadingAt(&pos);
		vel = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
   		mlSinCos (&trig, hdg);
		gWindVect.x = trig.cos * vel;
		gWindVect.y = trig.sin * vel;
	}
	

	// run any secondary effects here
	if ( secondaryCount > 0 && SimLibElapsedTime / SEC_TO_MSEC >= secondaryTimer )
	{
		secondaryCount--;
		secondaryTimer = SimLibElapsedTime / SEC_TO_MSEC + secondaryInterval +
						 secondaryInterval * (PRANDFloat() + 1.0f - gSfxLOD);

		RunSecondarySfx();
	}

	// if this effect is only drives other secondary effects, see if
	// the count has reached 0, then kill it if so
	if ( flags & SFX_SECONDARY_DRIVER )
	{
		if ( secondaryCount <= 0 )
		{
			return FALSE;
		}
		else
		{
			Draw();
			return TRUE;
		}
	}

///////////////////////////////////////
	if (baseObj && flags & SFX_F16CRASHLANDING) {

		Tpoint GroundNormal;
	 	groundZ = OTWDriver.GetGroundLevel( pos.x, pos.y, &GroundNormal );
		if ( pos.z - groundZ > -5.0f) {
			hitGround = TRUE;
			groundType = OTWDriver.GetGroundType (pos.x, pos.y);
		}
		int coverage = (groundType == COVERAGE_WATER) || (groundType == COVERAGE_RIVER);

		int curtime = SimLibElapsedTime / SEC_TO_MSEC;
		int lastHit = hitGround;
		int CrashSoundMask = 0;

		if (hitGround && coverage) {
			flags &= ~SFX_F16CRASHLANDING;
			flags &= ~SFX_MOVES;
		}
		else if (hitGround && !coverage) {
			hitGround = FALSE;
			float scale = 15.0f;
			float vecz = (float)fabs(vec.z) * 0.01f;
			if (vecz < 10.0f) scale -= (10.0f - vecz);

/*
			baseObj->SetYPRDelta(baseObj->YawDelta(),
								baseObj->PitchDelta(),// * 0.75f,
								baseObj->RollDelta());// * 0.75f);
*/
			vec.x *= 0.75f;
			vec.y *= 0.75f;
			vec.z *= 0.25f;
			if ( vec.z > GRAVITY ) vec.z = -vec.z;

			if (!(flags & SFX_F16CRASH_HITGROUND)) {
				int i = FloatToInt32 (9.99f * PRANDFloatPos());
				if (i < 5) i = SFX_BOOMA1 + i;
				else i = SFX_BOOMG1 + i - 5;
				CrashSoundMask |= F16CRASH_BOOMMASK;
				PlayCrashSound (F16CRASH_BOOMMASK, i);

				flags |= SFX_F16CRASH_HITGROUND;
				OTWDriver.AddSfxRequest(
						new SfxClass(SFX_GROUND_STRIKE_NOFIRE,	// type
								&pos,							// world pos
								4.0f,			// time to live
								scale) );		// scale
			}
			else {
				if (fabs( vec.x ) > 1.0f && 
					fabs( vec.y ) > 1.0f &&
					fabs( vec.z ) > 5.0f  ) {

					if (flags & SFX_F16CRASH_OBJECT) {
						CrashSoundMask |= F16CRASH_BUMPMASK;
						PlayCrashSound (F16CRASH_BUMPMASK, SFX_GROUND_CRUNCH);
					}

					OTWDriver.AddSfxRequest(
							new SfxClass(SFX_SMALL_AIR_EXPLOSION,				// type
							&pos,							// world pos
							2.5f,			// time to live
							scale) );		// scale
				}
			}

			if ((flags & SFX_MOVES) && !(flags & SFX_F16CRASH_STOP)) {
				flags |= SFX_F16CRASH_ADJUSTANGLE;
			}
		}


		// lived long enough?
		if ( curtime > timeToLive || hitGround ) {
			RunSfxCompletion( hitGround, groundZ, groundType );
		// done with this effect
			return FALSE;
		}	

		if (flags & SFX_F16CRASH_ADJUSTANGLE) {
			if ( pos.z - groundZ > -30.0f) {
				float mat[3][3];
				CalculateGroundMatrix (&GroundNormal, baseObj->Yaw(), mat);
				float yaw, pitch, roll;
				GetOrientation (mat, &yaw, &pitch, &roll);
				float pitch1 = AdjustAngle180 (baseObj->Pitch());
				if (pitch1 > PI/2) {
					pitch1 = PI - pitch1;
					yaw += PI;
				}
				else if (pitch1 < -PI/2) {
					pitch1 = -PI - pitch1;
					yaw += PI;
				}
				float roll1 = baseObj->Roll();
				int stopit = RestPiece (&pitch1, pitch, 0.5f, 3.0f*DTR);
				stopit += (RestPiece (&roll1, roll, 0.5f, 3.0f*DTR) << 1);
				if (stopit == 3) {
					flags &= ~SFX_F16CRASH_ADJUSTANGLE;
					baseObj->SetYPR(yaw, pitch, roll);
				}
				else baseObj->SetYPR(yaw, pitch1, roll1);
			}
			else flags &= ~SFX_F16CRASH_ADJUSTANGLE;
		}

		// update position based on vector
		if (flags & SFX_MOVES) {
			if ( fabs( vec.x ) < 2.0f && fabs( vec.y ) < 2.0f) {
				baseObj->SetYPRDelta(
								baseObj->YawDelta() * 0.95f,
								baseObj->PitchDelta() * 0.9f,
								baseObj->RollDelta() * 0.9f);//75f);
			}

			if (!(flags & SFX_F16CRASH_STOP)) {
				if (lastHit) {
					baseObj->SetYPR( 
						baseObj->Yaw() + baseObj->YawDelta() * sfxFrameTime,
						baseObj->Pitch(),
						baseObj->Roll());

					if ((flags & SFX_F16CRASH_OBJECT) && (fabs(baseObj->YawDelta()) > 20.0f*DTR )) {
						CrashSoundMask |= F16CRASH_DRAGMASK;
						PlayCrashSound (F16CRASH_DRAGMASK, SFX_TAILSCRAPE);
					}
				}
				else {
					if (flags & SFX_F16CRASH_ADJUSTANGLE) {
						baseObj->SetYPR( 
							baseObj->Yaw() + baseObj->YawDelta() * sfxFrameTime,
							baseObj->Pitch(),
							baseObj->Roll());
					}
					else {
						baseObj->SetYPR( 
							baseObj->Yaw(),
							baseObj->Pitch() + baseObj->PitchDelta() * sfxFrameTime,
							baseObj->Roll() + baseObj->RollDelta() * sfxFrameTime);
					}
				}
			}

			vec.x *= 0.99f;
			vec.y *= 0.99f;
			vec.z *= 0.99f;
			if (fabs(vec.x) < 1.0f && 
				fabs(vec.y) < 1.0f && 
				lastHit && 
				!(flags & SFX_F16CRASH_ADJUSTANGLE) &&
				fabs(baseObj->YawDelta()) < 15.0f * DTR && 
				fabs(baseObj->PitchDelta()) < 15.0f * DTR && 
				fabs(baseObj->RollDelta()) < 15.0f * DTR && 
				fabs(vec.z) < 5.0f  ) {

				flags |= SFX_F16CRASH_STOP;
				float pitch = baseObj->Pitch();
				int stopit = RestPiece (&pitch, RestPitch);
				float roll = baseObj->Roll();
				stopit += (RestPiece (&roll, RestRoll, 0.375f) << 1);
				if (stopit == 3) {
					flags &= ~SFX_MOVES;
					flags &= ~SFX_F16CRASH_STOP;
					vec.x = vec.y = vec.z = 0.0f;
				}
				baseObj->SetYPR (baseObj->Yaw(), pitch, roll);

				if (flags & SFX_F16CRASH_OBJECT) {
					CrashSoundMask |= F16CRASH_FELLMASK;
					PlayCrashSound (F16CRASH_FELLMASK, SFX_FLAPLOOP);
				}
			}
			else if (!lastHit && !(flags & SFX_F16CRASH_SKIPGRAVITY)) vec.z += GRAVITY * sfxFrameTime;

			CalcTransformMatrix (baseObj);
			Tpoint point;
			TransformPoint (&point, &CrashSlot, baseObj -> dmx);
			point.x *= OTWDriver.Scale();
			point.y *= OTWDriver.Scale();
			point.z *= OTWDriver.Scale();
			CrashPos.x += vec.x * sfxFrameTime;
			CrashPos.y += vec.y * sfxFrameTime;
			CrashPos.z += vec.z * sfxFrameTime;
			pos.x = CrashPos.x + point.x;
			pos.y = CrashPos.y + point.y;
			pos.z = CrashPos.z + point.z;

			float gZ = OTWDriver.GetGroundLevel( pos.x, pos.y);
			if (pos.z > gZ) {
				pos.z = gZ;
				if ( fabs( vec.x ) < 1.0f && fabs( vec.y ) < 1.0f && vec.z < GRAVITY) {
					flags |= SFX_F16CRASH_SKIPGRAVITY;
					vec.z = 0.0f;
				}
				else if (vec.z > GRAVITY) vec.z = -vec.z;
			}
			gZ = OTWDriver.GetGroundLevel (CrashPos.x, CrashPos.y);
			if (CrashPos.z > gZ) {
				CrashPos.z = gZ;
			}

			baseObj->SetPosition (pos.x, pos.y, pos.z);
			baseObj->SetDelta (vec.x, vec.y, vec.z );

/*
			if (fabs( vec.x ) > 10.0f && 
				fabs( vec.y ) > 10.0f && 
				fabs( vec.z ) > 10.0f &&
				curtime & 8) {
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_SMALL_AIR_EXPLOSION,				// type
						&pos,							// world pos
						1.0f,			// time to live
						5.0f) );		// scale

				CrashSoundMask |= F16CRASH_FLOATMASK;
				PlayCrashSound (F16CRASH_FLOATMASK, SFX_IMPACTG6);
			}
*/
		}
		else {
			if (!(flags & SFX_F16CRASH_STOP)) {
				flags |= SFX_F16CRASH_STOP;
				float mat[3][3], mat1[3][3];
				Tpoint normal;
				CalculateRestingObjectMatrix (RestPitch, RestRoll, mat1);
				CalculateGroundMatrix (&normal, baseObj->Yaw(), mat);
				MultiplyMatrix (baseObj->dmx, mat, mat1);
				float yaw, pitch, roll;
				GetOrientation (baseObj->dmx, &yaw, &pitch, &roll);
				baseObj->SetYPR(yaw, pitch, roll);
			}
		}
		PlayCrashSound (CrashSoundMask, 0, 0, SimLibElapsedTime, &pos);

		if (flags & SFX_F16CRASH_OBJECT) {
			OTWDriver.SetOwnshipPosition (CrashPos.x, CrashPos.y, CrashPos.z);
			OTWDriver.SetEndFlightPoint(
					CrashPos.x - baseObj->dmx[0][0] * 100.0f,
					CrashPos.y - baseObj->dmx[0][1] * 100.0f,
					CrashPos.z - baseObj->dmx[0][2] * 100.0f );
			OTWDriver.SetEndFlightVec(
					-baseObj->dmx[0][0],
					-baseObj->dmx[0][1],
					-baseObj->dmx[0][2] );
		}

		Trotation objrot;
		objrot.M11 = baseObj->dmx[0][0];
		objrot.M21 = baseObj->dmx[0][1];
		objrot.M31 = baseObj->dmx[0][2];
		objrot.M12 = baseObj->dmx[1][0];
		objrot.M22 = baseObj->dmx[1][1];
		objrot.M32 = baseObj->dmx[1][2];
		objrot.M13 = baseObj->dmx[2][0];
		objrot.M23 = baseObj->dmx[2][1];
		objrot.M33 = baseObj->dmx[2][2];
		Tpoint objpos;
		objpos.x = baseObj->XPos();
		objpos.y = baseObj->YPos();
		objpos.z = baseObj->ZPos();
		((DrawableBSP*)(baseObj->drawPointer))->Update(&objpos, &objrot);
		baseObj->drawPointer->SetScale(OTWDriver.Scale());
		return TRUE;
	}
///////////////////////////////////////


	// check for hit with ground
	if ( (flags & SFX_MOVES) && !(flags & SFX_NO_GROUND_CHECK) )
	{
		// 1st get approximation
		groundZ = OTWDriver.GetApproxGroundLevel (pos.x, pos.y);
		if (  pos.z - groundZ  > -100.0f )
		{
		 	groundZ = OTWDriver.GetGroundLevel( pos.x, pos.y);
			if ( pos.z >= groundZ )
			{
				hitGround = TRUE;
				groundType = OTWDriver.GetGroundType (pos.x, pos.y);
			}
		}
	}

	int coverage = (groundType == COVERAGE_WATER) || (groundType == COVERAGE_RIVER);

	// does this object bounce?
	if ( hitGround && (flags & (SFX_BOUNCES | SFX_BOUNCES_HARD) ) && !coverage)
	{
		// calcuate the new movement vector
		GroundReflection();
		pos.z = groundZ - 4.0f;
		// momentum loss
		if ( flags & SFX_BOUNCES )
		{
			vec.x *= 0.30f;
			vec.y *= 0.30f;
			vec.z *= 0.30f;
		}
		else
		{
			vec.x *= 0.80f;
			vec.y *= 0.80f;
			vec.z *= 0.80f;
		}
		if ( fabs( vec.x ) < 10.0f && fabs( vec.y ) < 10.0f && fabs( vec.z ) < 10.0f  )
		{
			flags &= ~SFX_MOVES;
		}

		hitGround = FALSE;
		if ( baseObj )
			baseObj->SetYPRDelta( 0.0f, 0.0f, 0.0f );
	}


	// lived long enough?
	if ( SimLibElapsedTime / SEC_TO_MSEC > timeToLive || hitGround )
	{
		RunSfxCompletion( hitGround, groundZ, groundType );

		// done with this effect
		return FALSE;

	}

	// if it's fire, play sound
	if ( type == SFX_FIRE )
	{
		F4SoundFXSetPos( SFX_FIRELOOP, TRUE, pos.x, pos.y, pos.z, 1.0f );
	}

	// do we need to move it?
	if ( !(flags & SFX_MOVES) || (flags & SFX_TIMER_FLAG) )
	{
		Draw();
	    return TRUE;
	}

	if ( flags & SFX_USES_GRAVITY )
	{
		// gravity also assumes air friction
		vec.x *= 0.999F;
		vec.y *= 0.999F;
		if (vec.z > 500.0F)
			vec.z *= 0.999F;
		else
			vec.z += GRAVITY * sfxFrameTime;

	}
	else if ( flags & SFX_TRAJECTORY )
	{
		// trajectory has no air friction
		vec.z += GRAVITY * sfxFrameTime;
	}

	if ( (flags & SFX_NO_DOWN_VECTOR ) && vec.z > 0.0f )
	{
		vec.z = 0.0f;
	}

	// update position based on vector
	pos.x += vec.x * sfxFrameTime;
	pos.y += vec.y * sfxFrameTime;
	pos.z += vec.z * sfxFrameTime;

	if ( baseObj )		// for bsp objects
	{
		baseObj->SetPosition (pos.x, pos.y, pos.z );
		baseObj->SetDelta (vec.x, vec.y, vec.z );
		baseObj->SetYPR(
				baseObj->Yaw() + baseObj->YawDelta() * sfxFrameTime,
				baseObj->Pitch() + baseObj->PitchDelta() * sfxFrameTime,
				baseObj->Roll() + baseObj->RollDelta() * sfxFrameTime);
		CalcTransformMatrix (baseObj );
	}

	Draw();
	return TRUE;
}

/*
** Name: Draw
** Description:
**		Update the position
**		Returns FALSE when completed
*/
BOOL
SfxClass::Draw (void)
{
	Tpoint mpos;
	Trotation rot = IMatrix;
	float scaleOTW = OTWDriver.Scale();

	// do we need to move it?

	// this type has no drawing
	if ( type == SFX_SMOKING_FEATURE || type == SFX_STEAMING_FEATURE )
		return TRUE;

	// this type has no drawing
	if ( (!(flags & SFX_F16CRASHLANDING) && !(flags & SFX_MOVES)) || (flags & SFX_TIMER_FLAG) )
	{
		if ( obj2d )
		{
			obj2d->SetScale2D( scaleOTW );
		}
		if ( objBSP )
		{
			objBSP->SetScale( scaleOTW );
		}
		if ( baseObj )
		{
			baseObj->drawPointer->SetScale( scaleOTW );
		}
	    return TRUE;
	}

	// for 2d objects set new position
	if ( obj2d )
	{
		obj2d->SetPosition( &pos );
		obj2d->SetScale2D( scaleOTW );
	}

	// for BSP objects set new position
	if ( objBSP )
	{
		Trotation rot;

		objBSP->SetScale( scaleOTW );
      if ( type == SFX_EJECT1 )
      {
         // hack! ejection is sideways
         rot.M11 = 1.0f;
         rot.M12 = 0.0f;
         rot.M13 = 0.0f;
         rot.M21 = 0.0f;
         rot.M22 = 0.0f;
         rot.M23 = -1.0f;
         rot.M31 = 0.0f;
         rot.M32 = 1.0f;
         rot.M33 = 0.0f;
      }
      else if (type == SFX_MOVING_BSP)
      {
         rot.M11 = objBSP->orientation.M11;
         rot.M12 = objBSP->orientation.M12;
         rot.M13 = objBSP->orientation.M13;
         rot.M21 = objBSP->orientation.M21;
         rot.M22 = objBSP->orientation.M22;
         rot.M23 = objBSP->orientation.M23;
         rot.M31 = objBSP->orientation.M31;
         rot.M32 = objBSP->orientation.M32;
         rot.M33 = objBSP->orientation.M33;
      }
      else
		{
			// right now, no rotation
			rot = IMatrix;
		}

		objBSP->Update( &pos, &rot );
	}

	// for bsp objects
	if ( baseObj )
	{
		OTWDriver.ObjectSetData (baseObj, &pos, &rot);
		((DrawableBSP*)(baseObj->drawPointer))->Update(&pos, &rot);
		baseObj->drawPointer->SetScale( scaleOTW );
	}

	// for drawable trails 
	if ( objTrail )
	{
		OTWDriver.AddTrailHead (objTrail, pos.x, pos.y, pos.z);
	}

	// for drawable tracers
	if ( objTracer )
	{
		if ( type == SFX_SPARK_TRACER )
		{
			float curra = objTracer->GetAlpha();
			float r,g,b;

			objTracer->GetRGB( &r, &g, &b );
			curra -= sfxFrameTime * 0.8f;
			if ( curra < 0.0f )
				curra = 0.0f;
			b -= sfxFrameTime * 1.5f;
			if ( b < 0.0f )
				b = 0.0f;
			objTracer->SetAlpha( curra );
			objTracer->SetRGB( r, g, b );
			mpos.x = pos.x - ( vec.x * 0.25f );
			mpos.y = pos.y - ( vec.y * 0.25f );
			mpos.z = pos.z - ( vec.z * 0.25f );
		}

		// tracers take a start and end position
		// use a value that's about 1/4 their velocity vector
		else if ( type == SFX_GUN_TRACER )
		{
			mpos.x = pos.x - ( vec.x * sfxFrameTime * 0.2f );
			mpos.y = pos.y - ( vec.y * sfxFrameTime * 0.2f ) ;
			mpos.z = pos.z - ( vec.z * sfxFrameTime * 0.2f );
		}
		else
		{
			mpos.x = pos.x - ( vec.x * 0.02f );
			mpos.y = pos.y - ( vec.y * 0.02f );
			mpos.z = pos.z - ( vec.z * 0.02f );
		}
		objTracer->Update( &pos, &mpos );
	}

	return TRUE;
}


/*
** Name: GetApproxViewDist
** Description:
**		This function is periodically called by effects to get the
**		distance to the viewer position.  Basically does a manhatten dist.
*/
void
SfxClass::GetApproxViewDist ( float currTime )
{
	float absmax, absmid, absmin, tmp;
	Tpoint viewLoc;

	if (!viewPoint) // JB 010528
		return;

	if (F4IsBadReadPtr(viewPoint, sizeof *viewPoint)) return; // JPO CTD fix?

	// get view pos
	viewPoint->GetPos( &viewLoc ); // CTD Pos


	absmax = (float)fabs(viewLoc.x - pos.x);
	absmid = (float)fabs(viewLoc.y - pos.y);

	// less weight on z dist
	absmin = (float)fabs(viewLoc.z - pos.z) * 0.1f;

	// find the max
	if ( absmax < absmid )
	{
		//swap
		tmp = absmid;
		absmid = absmax;
		absmax = tmp;
	}

	if ( absmax < absmin )
	{
		// absmin is actually the max
		approxDist =  absmin +  ( absmax + absmid ) * 0.5f ;
	}
	else
	{
		approxDist =  absmax + ( absmin + absmid ) * 0.5f ;
	}

	// this is the next time to check
	distTimer = currTime + VIEW_DIST_INTERVAL;
}


/*
** Name: RunSecondarySfx
** Description:
**		Basically a big switch statement by sfx type for running
**		secondary effects.
*/
void
SfxClass::RunSecondarySfx( void )
{
	Tpoint mpos={0.0F}, mvec={0.0F}, vec2={0.0F};
	float distScale=0.0F;
	int mflags = SFX_MOVES | SFX_USES_GRAVITY | SFX_EXPLODE_WHEN_DONE;
	int numBursts=0,i=0;
	float rads=0.0F, radstep=0.0F;

	switch ( type )
	{
		case SFX_DURANDAL:
			mvec.x = vec.x * 0.5f;
			mvec.y = vec.y * 0.5f;
			mvec.z = -20.0f;

			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_WATER_CLOUD,			// type
				SFX_MOVES,						// flags
				&pos,							// world pos
				&mvec,							// vector
				2.0f,							// time to live
				10.5f ));		// scale
			break;

		case SFX_MESSAGE_TIMER:
			if ( endMessage )
			{
				FalconSendMessage(endMessage, FALSE);
				endMessage = NULL;
			}
			if ( damMessage )
			{
				FalconSendMessage(damMessage, FALSE);
				damMessage = NULL;
			}
			break;

#if 1  // MLR 12/19/2003 - Out with the crap, in with the new
		case SFX_SMOKING_FEATURE:
		case SFX_STEAMING_FEATURE:
		if(objTrail)
		{
			mlTrig trigWind;
			Tpoint windvec;
			float wind;

			// current wind
			mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pos));
			wind =  ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
			windvec.x = trigWind.cos * wind;
			windvec.y = trigWind.sin * wind;
			windvec.z = -10;
			
			objTrail->SetHeadVelocity(&windvec);
			OTWDriver.AddTrailHead(objTrail, pos.x, pos.y, pos.z);
		}
		break;
#else
		case SFX_SMOKING_FEATURE:

			mvec.x = 20.0f * PRANDFloat();
			mvec.y = 20.0f * PRANDFloat();
			mvec.z = -80.0f;
			if ( rand() & 1 )
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_FIRESMOKE,				// type
					SFX_MOVES,
					&pos,							// world pos
					&mvec,							// world pos
					2.5f,							// time to live
					scale ) );		// scale
			else
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_STEAM_CLOUD,				// type
					SFX_MOVES,
					&pos,							// world pos
					&mvec,							// world pos
					2.5f,							// time to live
					scale ) );		// scale
			break;

		case SFX_STEAMING_FEATURE:

			mvec.x = 20.0f * PRANDFloat();
			mvec.y = 20.0f * PRANDFloat();
			mvec.z = -80.0f;
			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_STEAM_CLOUD,				// type
				SFX_MOVES,
				&pos,							// world pos
				&mvec,							// world pos
				2.5f,							// time to live
				scale ) );		// scale
			break;
#endif
		case SFX_FEATURE_CHAIN_REACTION:
			// continue applying chain reaction until returns false.
			if ( SimFeatureClass::ApplyChainReaction( &pos, scale ) == TRUE )
			{
				secondaryTimer = SimLibElapsedTime / SEC_TO_MSEC + 1.0f;
				secondaryCount = 1;
			}
			break;

		case SFX_CLUSTER_BOMB:
			mvec.x = vec.x * 0.5f;
			mvec.y = vec.y * 0.5f;
			mvec.z = -20.0f;

			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_WATER_CLOUD,			// type
				SFX_MOVES,						// flags
				&pos,							// world pos
				&mvec,							// vector
				2.0f,							// time to live
				10.5f ));		// scale

			if ( gSfxCount[ SFX_GROUNDBURST ] > gSfxLODCutoff ||
			 	 gTotSfx >= gSfxLODTotCutoff )
			{
				numBursts = 10 + FloatToInt32(PRANDFloatPos() * 20.0f);
			}
			else
			{
				numBursts = 40 + FloatToInt32(PRANDFloatPos() * 40.0f);
			}
			for ( i = 0; i < numBursts; i++ )
			{
				// mvec.x = 1.2f * (mpos.x - pos.x) + 40.0f * PRANDFloat();
				// mvec.y = 1.2f * (mpos.y - pos.y) + 40.0f * PRANDFloat();
				// mvec.z = 1.2f * (mpos.z - pos.z) - 30.0f * PRANDFloatPos();
				mvec.x = vec.x + 80.0f * PRANDFloat();
				mvec.y = vec.y + 80.0f * PRANDFloat();
				mvec.z = vec.z + 40.0f * PRANDFloat();
		
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_GROUNDBURST,		// type
					SFX_MOVES | SFX_USES_GRAVITY,
					&pos,					// world pos
					&mvec,					// vel vector
					20.0f,					// time to live
					1.5f ) );				// scale
			}

			/*
			// 1st time thru we do detonation
			if ( secondaryCount == 1 )
			{
				mpos.z = OTWDriver.GetGroundLevel( pos.x, pos.y );

				// approx time to ground impact
				rads = (mpos.z - pos.z)/vec.z;

				for ( i = 0; i < 80; i++ )
				{

					// mvec.x = 1.2f * (mpos.x - pos.x) + 40.0f * PRANDFloat();
					// mvec.y = 1.2f * (mpos.y - pos.y) + 40.0f * PRANDFloat();
					// mvec.z = 1.2f * (mpos.z - pos.z) - 30.0f * PRANDFloatPos();
					mvec.x = vec.x + 70.0f * PRANDFloat();
					mvec.y = vec.y + 70.0f * PRANDFloat();
					mvec.z = vec.z + 20.0f * PRANDFloat();
			
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUNDBURST,		// type
						SFX_MOVES | SFX_USES_GRAVITY,
						&pos,					// world pos
						&mvec,					// vel vector
						20.0f,					// time to live
						2.5f ) );				// scale
				}
				secondaryTimer = SimLibElapsedTime / SEC_TO_MSEC + rads;
				pos.x += vec.x * rads;
				pos.y += vec.y * rads;
			}
			// 2nd time thru, run the ground bursts
			else
			{
				mpos = pos;
				mvec.x = vec.x * 2.0f;
				mvec.y = vec.y * 2.0f;
				mvec.z = 0.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_GROUNDBURST,		// type
					&mpos,					// world pos
					&mvec,					// vel vector
					8,						// time to live
					0.3f ) );				// scale
			}
			*/
			break;
		case SFX_SPARKS:
		case SFX_SPARKS_NO_DEBRIS:
			if ( gSfxCount[ SFX_EXPLSTAR_GLOW ] > gSfxLODCutoff ||
				 gSfxCount[ SFX_SPARK_TRACER ] > gSfxLODCutoff ||
			 	 gTotSfx >= gSfxLODTotCutoff ||
				 (approxDist > 30000.0f && scale < 50.0f) )
			{
				break;
			}

			/*
			mvec.x = PRANDFloat() * 25.0f;
			mvec.y = PRANDFloat() * 25.0f;
			mvec.z = PRANDFloat() * 25.0f;
			mpos.x = pos.x + PRANDFloat() * scale;
			mpos.y = pos.y + PRANDFloat() * scale;
			mpos.z = pos.z + PRANDFloat() * scale;

			OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_EXPLSTAR_GLOW,				// type
							SFX_MOVES | SFX_BOUNCES | SFX_USES_GRAVITY,						// flags
							&mpos,							// world pos
							&mvec,							// vector
							1.3f,							// time to live
							scale * 0.08f ) );							// scale
			*/

			mpos.x = pos.x + PRANDFloat() * scale * 0.3f;
			mpos.y = pos.y + PRANDFloat() * scale * 0.3f;
			mpos.z = pos.z + PRANDFloat() * scale * 0.3f;

			mvec.x = (mpos.x - pos.x) * 3.0f;
			mvec.y = (mpos.y - pos.y) * 3.0f;
			mvec.z = (mpos.z - pos.z) * 3.0f;

			OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_SPARK_TRACER,				// type
							SFX_MOVES | SFX_BOUNCES | SFX_USES_GRAVITY,						// flags
							&mpos,							// world pos
							&mvec,							// vector
							1.3f,							// time to live
							scale ) );							// scale

			mpos.x = pos.x + PRANDFloat() * scale * 0.3f;
			mpos.y = pos.y + PRANDFloat() * scale * 0.3f;
			mpos.z = pos.z + PRANDFloat() * scale * 0.3f;

			mvec.x = (mpos.x - pos.x) * 3.0f;
			mvec.y = (mpos.y - pos.y) * 3.0f;
			mvec.z = (mpos.z - pos.z) * 3.0f;

			OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_SPARK_TRACER,				// type
							SFX_MOVES | SFX_BOUNCES | SFX_USES_GRAVITY,						// flags
							&mpos,							// world pos
							&mvec,							// vector
							1.3f,							// time to live
							scale ) );							// scale
			break;
		case SFX_BILLOWING_SMOKE:
			mvec.x = PRANDFloat() * 35.0f;
			mvec.y = PRANDFloat() * 35.0f;
			mvec.z = -80.0f;
			mpos.x = pos.x + PRANDFloat() * 30.0f;
			mpos.y = pos.y + PRANDFloat() * 30.0f;
			mpos.z = pos.z;

			if ( gSfxCount[ SFX_FIRESMOKE ] > gSfxLODCutoff ||
			 	 gSfxCount[ SFX_TRAILSMOKE ] > gSfxLODCutoff ||
			 	 gTotSfx >= gSfxLODTotCutoff )
			{
				break;
			}

			if ( rand() & 1 )
			{
				OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_FIRESMOKE,				// type
							SFX_MOVES,						// flags
							&mpos,							// world pos
							&mvec,							// vector
							3.5f,							// time to live
							34.5f ) );							// scale
			}
			else
			{
				OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_TRAILSMOKE,				// type
							SFX_MOVES,						// flags
							&mpos,							// world pos
							&mvec,							// vector
							3.5f,							// time to live
							34.5f ) );							// scale
			}
			break;
		case SFX_FIRE:
		case SFX_FIRE_EXPAND:

			rads = approxDist/30000.0f;
			secondaryTimer = SimLibElapsedTime / SEC_TO_MSEC + 0.90f + 2.5F * rads;
			secondaryCount = 1;

			mvec.x = PRANDFloat() * 45.0f;
			mvec.y = PRANDFloat() * 45.0f;
			mvec.z = -80.0f;
			mpos.x = pos.x + PRANDFloat() * 30.0f;
			mpos.y = pos.y + PRANDFloat() * 30.0f;
			mpos.z = pos.z - scale * 0.65f;
			if ( (rand() & 3 ) == 3 )
			{
				mvec.x *= 0.5f;
				mvec.y *= 0.5f;
				OTWDriver.AddSfxRequest(
		   			new SfxClass( SFX_FIRE2,		// type
					SFX_NO_GROUND_CHECK | SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,						// flags
					&mpos,							// world pos
					&mvec,							// vector
					3.5f,							// time to live
					scale * 0.4f ) );							// scale

			}
			else
			{
				// try a reduction in the number of effects running
				// if ( approxDist > 10000.0f && gSfxCount[ SFX_FIRESMOKE ] > gSfxLODCutoff )
				if ( gSfxCount[ SFX_FIRESMOKE ] > gSfxLODCutoff ||
				 	 gTotSfx >= gSfxLODTotCutoff )
					break;

				OTWDriver.AddSfxRequest(
		   			new SfxClass( SFX_FIRESMOKE,				// type
					SFX_MOVES,				 // flags
					&mpos,							// world pos
					&mvec,							// vector
					3.5f,							// time to live
					scale ) );							// scale

				if ( gSfxCount[ SFX_FIRE2 ] > gSfxLODCutoff/2 ||
				 	 gTotSfx >= gSfxLODTotCutoff/2 )
					break;

				mvec.x *= 0.5f;
				mvec.y *= 0.5f;
				mpos.z = pos.z - scale * 0.15f;
				OTWDriver.AddSfxRequest(
		   			new SfxClass( SFX_FIRE2,		// type
					SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,						// flags
					&mpos,							// world pos
					&mvec,							// vector
					3.5f,							// time to live
					scale * 0.4f ) );							// scale
			}


			break;
		case SFX_FLAMING_PART:
			mpos = pos;
			// NOTE: if using scatter plot fire, add a bit to the position
			mpos.z += 15.0f;
			// mpos.z -= 15.0f;
			OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE1,				// type
						&mpos,							// world pos
						0.2f,			// time to live
						30.0f) );		// scale
			break;
		case SFX_BURNING_PART:
			mpos = pos;
			// NOTE: if using scatter plot fire, add a bit to the position
			// mpos.z += 15.0f;
			// mpos.z -= 15.0f;
			// NOTE: we have to subtract out SimLibElapsedTime because it's
			// already been added to timeToLive
			OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE,				// type
					&mpos,							// world pos
					timeToLive - SimLibElapsedTime / SEC_TO_MSEC,			// time to live
					40.0f) );		// scale
			break;
		case SFX_FIREBALL:
			mpos.x = (float)( (float)secondaryCount/(float)initSecondaryCount );
			if ( mpos.x > 0.90f )
			{
				OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_HOT,				// type
						&pos,							// world pos
						0.4f,			// time to live
						scale * 0.4f ) );		// scale
			}
			else if ( mpos.x > 0.85f )
			{
				if ( ( rand() & 1 ) )
				{
					mvec.x = PRANDFloat() * 25.0f;
					mvec.y = PRANDFloat() * 25.0f;
					mvec.z = -5.0f;
					OTWDriver.AddSfxRequest(
							new SfxClass (SFX_FIRE2,		// type
							SFX_MOVES,						// flags
							&pos,							// world pos
							&mvec,							// vector
							2.0f,							// time to live
							scale * 0.2f ));		// scale
				}
				else
				{
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_MED,				// type
						&pos,							// world pos
						0.4f,			// time to live
						scale * 0.4f ) );		// scale
				}
			}
			else if ( mpos.x > 0.70f )
			{
				if ( ( rand() & 1 ) )
				{
					mvec.x = PRANDFloat() * 25.0f;
					mvec.y = PRANDFloat() * 25.0f;
					mvec.z = -5.0f;
					OTWDriver.AddSfxRequest(
							new SfxClass (SFX_FIRE2,		// type
							SFX_MOVES,						// flags
							&pos,							// world pos
							&mvec,							// vector
							2.0f,							// time to live
							scale * 0.2f ));		// scale
				}
				else
				{
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_COOL,				// type
						&pos,							// world pos
						0.3f,			// time to live
						scale * 0.5f ) );		// scale
				}
			}
			/*
			else if ( mpos.x > 0.70f )
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = -5.0f;
				OTWDriver.AddSfxRequest(
						new SfxClass (SFX_FIRE2,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						scale * 0.2f ));		// scale
			}
			*/
			else if ( mpos.x > 0.90f )
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = -5.0f;
				if ( ( rand() & 1 ) )
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_TRAILSMOKE,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						5.2f ));		// scale
				else
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_TRAILDUST,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						5.2f ));		// scale
			}
			else
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = -10.0f;
				if ( ( rand() & 3 ) == 3)
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_TRAILSMOKE,			// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						5.5f ));		// scale
				else
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_TRAILDUST,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						5.2f ));		// scale
			}

			break;
		case SFX_WATER_FIREBALL:
			mpos.x = (float)( (float)secondaryCount/(float)initSecondaryCount );
			if ( mpos.x > 0.85f )
				OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_HOT,				// type
						&pos,							// world pos
						0.8f,			// time to live
						scale * 0.4f ) );		// scale
			else if ( mpos.x > 0.50f )
				OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_MED,				// type
						&pos,							// world pos
						0.5f,			// time to live
						scale * 0.4f ) );		// scale
			else if ( mpos.x > 0.25f )
			{
				OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_COOL,				// type
						&pos,							// world pos
						0.3f,			// time to live
						scale * 0.5f ) );		// scale
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = -25.0f;
				if ( (rand() & 3) == 3 )
				{
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_FIRE3,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						10.5f ));		// scale
				}
				else
				{
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_BLUE_CLOUD,			// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						10.5f ));		// scale
				}
			}
			else
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z =  -20.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_WATER_CLOUD,			// type
					SFX_MOVES,						// flags
					&pos,							// world pos
					&mvec,							// vector
					2.0f,							// time to live
					10.5f ));		// scale
			}

			break;
		case SFX_FIRETRAIL:
			OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE_EXPAND,				// type
					&pos,							// world pos
					2.0f,			// time to live
					scale * 0.2f ) );		// scale
			break;
		case SFX_WATERTRAIL:
			mpos.x = (float)( (float)secondaryCount/(float)initSecondaryCount );
			if ( mpos.x > 0.70f )
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = -25.0f;
				if ( (rand() & 3) == 3 )
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_FIRE3,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						10.5f ));		// scale
				else
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_BLUE_CLOUD,				// type
						&pos,							// world pos
						0.8f,			// time to live
						scale * 0.4f ) );		// scale
			}
			else if ( mpos.x > 0.50f )
			{
				if ( (rand() & 3) == 3 )
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_WATER_CLOUD,				// type
						&pos,							// world pos
						0.5f,			// time to live
						scale * 0.4f ) );		// scale
				else
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_BLUE_CLOUD,				// type
						&pos,							// world pos
						0.5f,			// time to live
						scale * 0.4f ) );		// scale
			}
			else if ( mpos.x > 0.25f )
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = -25.0f;
				if ( (rand() & 3) == 3 )
				{
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_BLUE_CLOUD,		// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						10.5f ));		// scale
				}
				else
				{
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_WATER_CLOUD,			// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						2.0f,							// time to live
						10.5f ));		// scale
				}
			}
			else
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z =  -20.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_WATER_CLOUD,			// type
					SFX_MOVES,						// flags
					&pos,							// world pos
					&mvec,							// vector
					2.0f,							// time to live
					10.5f ));		// scale
			}
			break;
		case SFX_SMOKING_PART:
		    if ( gSfxCount[ SFX_TRAILSMOKE ] < gSfxLODCutoff &&
				 gTotSfx < gSfxLODTotCutoff )
			{
				mvec.x = PRANDFloat() * 25.0f;
				mvec.y = PRANDFloat() * 25.0f;
				mvec.z = PRANDFloat() * 25.0f;

				// 1st get approximation
				rads = OTWDriver.GetApproxGroundLevel (pos.x, pos.y);
				if (  pos.z - rads  > -80.0f )
				{
					// remove the trail when we near the fround
					if ( objTrail )
					{
						OTWDriver.AddSfxRequest(
							new SfxClass (
							21.5f,							// time to live
							objTrail ));					// trail
						objTrail = NULL;
					}

					mvec.z = -40.0f;
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_TRAILSMOKE,			// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						1.5f,							// time to live
						3.5f ));		// scale
				}
				else {
					OTWDriver.AddSfxRequest(
						new SfxClass (SFX_TRAILSMOKE,			// type
						SFX_MOVES,						// flags
						&pos,							// world pos
						&mvec,							// vector
						1.0f,							// time to live
						3.5f ));		// scale
				}
			}
			break;
		case SFX_DEBRISTRAIL:
			mvec.x = PRANDFloat() * 25.0f;
			mvec.y = PRANDFloat() * 25.0f;
			mvec.z = PRANDFloat() * 25.0f;
			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_TRAILSMOKE,			// type
				SFX_MOVES,						// flags
				&pos,							// world pos
				&mvec,							// vector
				2.0f,							// time to live
				3.5f ));		// scale
			break;
		case SFX_SAM_LAUNCH:

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );
		 	pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 5.0f;

			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_GROUND_FLASH,			// type
				&pos,					// world pos
				1.0f,					// time to live
				distScale * 6300.0f ) );				// scale

			for ( i = 0; i < 8; i++ )
			{
				mpos = pos;

				mpos.x += PRANDFloat() * 55.0f;
				mpos.y += PRANDFloat() * 55.0f;

				mvec.x = PRANDFloat() * 125.0f;
				mvec.y = PRANDFloat() * 125.0f;
				mvec.z = PRANDFloat() * 125.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_STEAM_CLOUD,			// type
					SFX_USES_GRAVITY | SFX_MOVES | SFX_NO_DOWN_VECTOR | SFX_NO_GROUND_CHECK,				 // flags
					&mpos,							// world pos
					&mvec,							// vector
					2.0f,							// time to live
					40.5f ));		// scale
			}
			break;
		case SFX_DEBRISTRAIL_DUST:
			mvec.x = PRANDFloat() * 25.0f;
			mvec.y = PRANDFloat() * 25.0f;
			mvec.z = PRANDFloat() * 25.0f;
			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_TRAILDUST,			// type
				SFX_MOVES,						// flags
				&pos,							// world pos
				&mvec,							// vector
				2.0f,							// time to live
				3.5f ));		// scale
			break;
		case SFX_AIR_HANGING_EXPLOSION:
			mvec.x = 0.0f;
			mvec.y = 0.0f;
			mvec.z = -20.0f;
			StartRandomDebris();
			OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_LONG_HANGING_SMOKE2,				// type
							&pos,							// world pos
							60.5f,							// time to live
							scale * 0.2f ) );							// scale
			break;
		case SFX_GROUND_STRIKE_NOFIRE:

			mpos = pos;

			mvec.x = 0.0f;
			mvec.y = 0.0f;
			mvec.z = -40.0f;

		    if ( gSfxCount[ SFX_TRAILSMOKE ] < gSfxLODCutoff &&
			     gSfxCount[ SFX_FIRESMOKE ] < gSfxLODCutoff &&
				 gTotSfx < gSfxLODTotCutoff )
			{
				switch ( PRANDInt5() )
				{
					case 0:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_FIRESMOKE,				// type
							SFX_MOVES | SFX_NO_DOWN_VECTOR | SFX_NO_GROUND_CHECK,				 // flags
							&mpos,							// world pos
							&mvec,							// vector
							3.5f,							// time to live
							scale * 0.2f ) );							// scale
						break;
					case 1:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_TRAILSMOKE,				// type
							SFX_MOVES | SFX_NO_DOWN_VECTOR | SFX_NO_GROUND_CHECK,				 // flags
							&mpos,							// world pos
							&mvec,							// vector
							3.5f,							// time to live
							scale * 0.2f ) );							// scale
						break;
					default:
						break;
				}
			}

			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_GROUND_FLASH,			// type
				&mpos,					// world pos
				2.2f,					// time to live
				scale * 2.0f ) );				// scale

			/*
			if ( rand() & 1 )
			{
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SHOCK_RING_SMALL,			// type
					&mpos,					// world pos
					2.2f,					// time to live
					scale * 0.4f ) );				// scale
			}
			*/

			// NOTE: if using scatter plot fire, add a bit to the position
			// mpos.z -= 15.0f;
			StartRandomDebris();

			break;
		case SFX_ARTILLERY_EXPLOSION:
			mpos = pos;

			mvec.x = 0.0f;
			mvec.y = 0.0f;
			mvec.z = -40.0f;
			OTWDriver.AddSfxRequest(
		   		new SfxClass( SFX_FIRESMOKE,				// type
				SFX_MOVES | SFX_NO_DOWN_VECTOR | SFX_NO_GROUND_CHECK,				 // flags
				&mpos,							// world pos
				&mvec,							// vector
				3.5f,							// time to live
				scale * 0.2f ) );							// scale

			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_GROUND_FLASH,			// type
				&mpos,					// world pos
				2.2f,					// time to live
				scale * 2.0f ) );				// scale

			StartRandomDebris();
			break;
		case SFX_GROUND_STRIKE:
			mpos = pos;
			// NOTE: if using scatter plot fire, add a bit to the position


			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_BILLOWING_SMOKE,			// type
				&mpos,					// world pos
				2,					// time to live
				0.3f ) );				// scale

			mpos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 5.0f;
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_GROUND_FLASH,			// type
				&mpos,					// world pos
				2.2f,					// time to live
				scale * 2.0f ) );				// scale
			/*
			mpos.z += 15.0f;
			// mpos.z -= 15.0f;
			OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE,				// type
						&mpos,							// world pos
						35.2f,			// time to live
						70.0f) );		// scale
			*/
			// StartRandomDebris();
			// if there'2 no 2d animation, we do a firetrail
			if ( !obj2d )
			{
				mpos.x = pos.x;
				mpos.y = pos.y;
				numBursts = 3 + (int)( 3.0f * gSfxLOD );
	
				for ( i = 0; i < numBursts; i++ )
				{
					// mvec.x = 10.0f * PRANDFloat() * scale * 0.01f;
					// mvec.y = 10.0f * PRANDFloat() * scale * 0.01f;
					mvec.z = -40.0f + PRANDFloatPos() * -80.0f;
					mvec.x = 90.0f * PRANDFloat();
					mvec.y = 90.0f * PRANDFloat();
	
					if ( (i & 3) == 3 )
					{

						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_FIRE4,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
							&mpos,					// world pos
							&mvec,					// vel vector
							1.5,					// time to live
							5.0f ) );				// scale
					}
					else
					{
						if ( gSfxCount[ SFX_DEBRISTRAIL ] < gSfxLODCutoff &&
				 			gTotSfx < gSfxLODTotCutoff )
						{
							OTWDriver.AddSfxRequest(
								new SfxClass( SFX_DEBRISTRAIL,		// type
								SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
								&mpos,					// world pos
								&mvec,					// vel vector
								5.0,					// time to live
								scale * 0.25f ) );				// scale
						}
					}
				}
				/*
				OTWDriver.AddSfxRequest(
						new SfxClass( SFX_FIREBALL,		// type
						SFX_MOVES | SFX_NO_GROUND_CHECK,
						&mpos,					// world pos
						&mvec,					// vel vector
						3.0,					// time to live
						scale * 0.50f ) );				// scale
				*/
			}
			break;
		case SFX_WATER_STRIKE:
			StartRandomDebris();
			mpos = pos;
			mpos.z += scale;
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_SHOCK_RING_SMALL,			// type
				&mpos,					// world pos
				1.5f,					// time to live
				scale * 0.3f ) );				// scale
			break;
		case SFX_WATER_EXPLOSION:
			StartRandomDebris();
			mpos = pos;
			mpos.z += scale;
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_SHOCK_RING,			// type
				&mpos,					// world pos
				2.2f,					// time to live
				1.0f ) );				// scale

			// send up some water trails
			if ( obj2d == NULL )
			{
				numBursts =  2 + (int)( 6.0f * gSfxLOD );
				if ( numBursts )
				{
					mpos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 10.0f;
					mpos.x = pos.x;
					mpos.y = pos.y;
					mvec.z = -10.0f * scale * 0.02f;
				}
	
				for ( i = 0; i < numBursts; i++ )
				{
					mvec.x = 30.0f * PRANDFloat() * scale * 0.01f;
					mvec.y = 30.0f * PRANDFloat() * scale * 0.01f;
	
					/*
					if ( i & 1)
					{
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_WATER_FIREBALL,		// type
							SFX_MOVES | SFX_USES_GRAVITY,
							&mpos,					// world pos
							&mvec,					// vel vector
							3.0,					// time to live
							scale * 0.25f ) );				// scale
	
					}
					else
					*/
					{
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_WATERTRAIL,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
							&mpos,					// world pos
							&mvec,					// vel vector
							3.0,					// time to live
							scale * 0.25f ) );				// scale
					}
				}
			}

			break;
		case SFX_GROUND_PENETRATION:
			mpos = pos;
			StartRandomDebris();
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_SHOCK_RING,			// type
				&mpos,					// world pos
				2.2f,					// time to live
				10.0f ) );				// scale


			// send up some fire trails
			numBursts = 4 + (int)( 6.0f * gSfxLOD );
			mpos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 10.0f;
			mpos.x = pos.x;
			mpos.y = pos.y;

			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_BILLOWING_SMOKE,			// type
				&mpos,					// world pos
				30,					// time to live
				0.3f ) );				// scale

			for ( i = 0; i < numBursts; i++ )
			{
				mvec.z = -80.0f * PRANDFloatPos() * scale * 0.01f;
				mvec.x = 20.0f * PRANDFloat() * scale * 0.01f;
				mvec.y = 20.0f * PRANDFloat() * scale * 0.01f;

				if ( (i & 7 ) == 7 )
				{
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_FIRE5,		// type
						SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
						&mpos,					// world pos
						&mvec,					// vel vector
						3.0,					// time to live
						scale * 0.05f ) );				// scale
				}
				else if ( (rand() & 3) == 3 )
				{
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_DEBRISTRAIL_DUST,		// type
						SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
						&mpos,					// world pos
						&mvec,					// vel vector
						3.0,					// time to live
						scale * 0.25f ) );				// scale
				}
				else
				{
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_DEBRISTRAIL,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
							&mpos,					// world pos
							&mvec,					// vel vector
							8.0,					// time to live
							scale * 0.25f ) );				// scale
				}
			}

			break;

		case SFX_AIR_PENETRATION:
			mpos = pos;
			StartRandomDebris();

			// send up some fire trails
			numBursts = 2 + (int)( 2.0f * gSfxLOD );

			for ( i = 0; i < numBursts; i++ )
			{
				mvec.x = 70.0f * PRANDFloat();
				mvec.y = 70.0f * PRANDFloat();
				mvec.z = 70.0f * PRANDFloat();

				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_DEBRISTRAIL,		// type
					SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
					&mpos,					// world pos
					&mvec,					// vel vector
					3.0,					// time to live
					scale * 0.15f ) );				// scale
			}

			break;
		case SFX_GROUND_EXPLOSION:

			if ( secondaryCount == 9 || obj2d )
			{
				StartRandomDebris();
				pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y );
				mpos = pos;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SHOCK_RING,			// type
					&mpos,					// world pos
					2.2f,					// time to live
					10.0f ) );				// scale
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_GROUND_FLASH,			// type
					&mpos,					// world pos
					2.2f,					// time to live
					scale  ) );				// scale
				mpos.z = pos.z - 25.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_BILLOWING_SMOKE,			// type
					&mpos,					// world pos
					3,					// time to live
					0.3f ) );				// scale
			}

			if ( obj2d )
				break;

			if ( secondaryCount == 9 )
			{
				// mvec.z = -20.0f * scale * 0.02f;

				if ( gSfxCount[ SFX_DEBRISTRAIL ] < gSfxLODCutoff &&
					 gSfxCount[ SFX_FIRETRAIL ] < gSfxLODCutoff &&
				 	 gTotSfx < gSfxLODTotCutoff )
				{
					numBursts = 8 + FloatToInt32( PRANDFloatPos() * 6.0f );
				}
				else
				{
					numBursts = 3 + FloatToInt32( PRANDFloatPos() * 4.0f );
				}
	
				for ( i = 0; i < numBursts; i++ )
				{
					mpos.x = pos.x + PRANDFloat() * scale * 0.11f;
					mpos.y = pos.y + PRANDFloat() * scale * 0.11f;
					mpos.z = pos.z - PRANDFloatPos() * scale * 0.11f;

					mvec.x = 30.0f * PRANDFloat() * scale * 0.01f;
					mvec.y = 30.0f * PRANDFloat() * scale * 0.01f;
					mvec.z = -8.0f * PRANDFloatPos() * scale * 0.02f;

					/*
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_FIRE_EXPAND_NOSMOKE,				// type
						SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
						&mpos,					// world pos
						&mvec,					// vel vector
						1.5,					// time to live
						scale * PRANDFloatPos() * 0.2f ) );		// scale
					*/
					if ( (rand() & 3 ) == 3 )
					{
						OTWDriver.AddSfxRequest(
							new SfxClass(SFX_HIT_EXPLOSION_NOSMOKE,				// type
							&mpos,					// world pos
							1.8f,					// time to live
							scale * 0.2f + scale * PRANDFloatPos() * 0.3f ) );		// scale
					}

					mpos.x = pos.x + PRANDFloat() * scale * 0.05f;
					mpos.y = pos.y + PRANDFloat() * scale * 0.05f;
					mpos.z = OTWDriver.GetGroundLevel( mpos.x, mpos.y ) - 5.0f;
	
					mvec.x = 10.0f * PRANDFloat() * scale * 0.01f;
					mvec.y = 10.0f * PRANDFloat() * scale * 0.01f;
					mvec.z = -18.0f * PRANDFloatPos() * scale * 0.01f;
					if ( approxDist < SFX_LOD_DIST &&
						 gSfxCount[ SFX_DEBRISTRAIL ] < gSfxLODCutoff &&
						 gSfxCount[ SFX_FIRETRAIL ] < gSfxLODCutoff &&
				 		gTotSfx < gSfxLODTotCutoff )
					{
						if ( rand() & 1 )
							OTWDriver.AddSfxRequest(
								new SfxClass( SFX_FIRETRAIL,		// type
								SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
								&mpos,					// world pos
								&mvec,					// vel vector
								4.0,					// time to live
								scale * 0.25f ) );				// scale
						else
							OTWDriver.AddSfxRequest(
									new SfxClass( SFX_DEBRISTRAIL,		// type
									SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
									&mpos,					// world pos
									&mvec,					// vel vector
									6.0,					// time to live
									scale * 0.25f ) );				// scale
					}
				}
				vec.z -= scale * 0.60f;
			}
			else
			{
				// vec.z -= scale * 0.15f;
				// if ( (rand() & 3) == 3 )
				if ( secondaryCount & 1 )
				{
					vec.z -= scale * 0.20f;
					mpos.x = pos.x + PRANDFloat() * scale * 0.21f;
					mpos.y = pos.y + PRANDFloat() * scale * 0.21f;
					mpos.z = pos.z + vec.z;
	
					mvec.x = 10.0f * PRANDFloat() * scale * 0.01f;
					mvec.y = 10.0f * PRANDFloat() * scale * 0.01f;
					mvec.z = -10.0f * PRANDFloatPos() * scale * 0.01f;
	
					if ( (rand() & 1) == 1 )
					{
						mpos.z = pos.z + vec.z * 0.3F;
						OTWDriver.AddSfxRequest(
							new SfxClass(SFX_HIT_EXPLOSION_NOSMOKE,				// type
							&mpos,					// world pos
							1.5f,					// time to live
							scale * 0.2f + scale * PRANDFloatPos() * 0.3f ) );		// scale
					}
					else if ( (rand() & 1) == 1 )
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_BIG_DUST,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
							&mpos,					// world pos
							&mvec,					// vel vector
							1.5,					// time to live
							scale * 0.55f ) );				// scale
					else
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_BIG_SMOKE,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
							&mpos,					// world pos
							&mvec,					// vel vector
							1.5,					// time to live
							scale * 0.55f ) );				// scale
				}
				else
				{
					if ( (rand() & 1) == 1 )
					{
						mpos.x = pos.x + PRANDFloat() * scale * 0.21f;
						mpos.y = pos.y + PRANDFloat() * scale * 0.21f;
						mpos.z = pos.z + vec.z * 0.3F;
						OTWDriver.AddSfxRequest(
							new SfxClass(SFX_HIT_EXPLOSION_NOSMOKE,				// type
							&mpos,					// world pos
							1.5f,					// time to live
							scale * 0.2f + scale * PRANDFloatPos() * 0.3f ) );		// scale
					}

					if ( approxDist < SFX_LOD_DIST &&
						 gSfxCount[ SFX_DEBRISTRAIL ] < gSfxLODCutoff &&
						 gSfxCount[ SFX_FIRETRAIL ] < gSfxLODCutoff &&
				 		gTotSfx < gSfxLODTotCutoff )
					{

						for ( i = 0; i < 2; i++ )
						{
							mpos.x = pos.x + PRANDFloat() * scale * 0.05f;
							mpos.y = pos.y + PRANDFloat() * scale * 0.05f;
							mpos.z = OTWDriver.GetGroundLevel( mpos.x, mpos.y ) - 5.0f;
			
							mvec.x = 10.0f * PRANDFloat() * scale * 0.01f;
							mvec.y = 10.0f * PRANDFloat() * scale * 0.01f;
							mvec.z = -38.0f * PRANDFloatPos() * scale * 0.01f;
			
							OTWDriver.AddSfxRequest(
									new SfxClass( SFX_DEBRISTRAIL,		// type
									SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
									&mpos,					// world pos
									&mvec,					// vel vector
									6.0,					// time to live
									scale * 0.25f ) );				// scale
						}
					}
				}
			}

			break;
		case SFX_AIR_EXPLOSION:
			/*
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_EXPLCROSS_GLOW,			// type
				&pos,					// world pos
				1.2f,					// time to live
				0.1f ) );				// scale
			*/
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_EXPLCIRC_GLOW,			// type
				&pos,					// world pos
				1.8f,					// time to live
				scale ) );				// scale
			break;
		case SFX_ROCKET_BURST:
			if ( gSfxCount[ SFX_SPARKS ] < gSfxLODCutoff &&
				 gSfxCount[ SFX_FIRETRAIL ] < gSfxLODCutoff &&
				 gTotSfx < gSfxLODTotCutoff )
			{
				numBursts = 24 + FloatToInt32( PRANDFloatPos() * 10.0f );
			}
			else
			{
				numBursts = 5 + FloatToInt32( PRANDFloatPos() * 10.0f );
			}
			for ( i = 0; i < numBursts; i++ )
			{
				mvec.x = vec.x + 200.0f * PRANDFloat();
				mvec.y = vec.y + 200.0f * PRANDFloat();
				mvec.z = vec.z + 200.0f * PRANDFloat();
				/*
				if ( (rand() & 3) == 3 )
				{
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_EXPLCIRC_GLOW,		// type
						SFX_MOVES | SFX_USES_GRAVITY,
						&pos,					// world pos
						&mvec,					// vel vector
						3.5f,					// time to live
						12.0f ) );				// scale
				}
				else
				*/
				{
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_SPARKS,		// type
						SFX_MOVES | SFX_USES_GRAVITY,
						&pos,					// world pos
						&mvec,					// vel vector
						3.5f,					// time to live
						12.0f ) );				// scale
				}
			}
			break;
		case SFX_MISSILE_BURST:
			mvec.x = 0.0f;
			mvec.y = 0.0f;
			mvec.z = -20.0f;
			OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_LONG_HANGING_SMOKE2,				// type
							&pos,							// world pos
							30.5f,							// time to live
							scale * 0.5f ) );							// scale

			// do som extra busts possibly...
			/*
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_EXPLCROSS_GLOW,			// type
				&pos,					// world pos
				1.2f,					// time to live
				0.1f ) );			 	// scale
			*/
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_EXPLCIRC_GLOW,			// type
				&pos,					// world pos
				1.8f,					// time to live
				scale ) );			 	// scale

			numBursts = (int)(5.0f * gSfxLOD) + PRANDInt3();
			for ( i = 0; i < numBursts; i++ )
			{
				mvec.x = vec.x + 100.0f * PRANDFloat(); // MLR 1/2/2004 - 
				mvec.y = vec.y + 100.0f * PRANDFloat();
				mvec.z = vec.z + 100.0f * PRANDFloat();
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SMOKETRAIL,		// type
					SFX_MOVES | SFX_USES_GRAVITY,
					&pos,					// world pos
					&mvec,					// vel vector
					1.0,					// time to live
					10.0f ) );				// scale
			}

			// just some falling pieces of junk
			for ( i = 0; i < numBursts; i++ )
			{
				mvec.x = 100.0f * PRANDFloat(); // MLR 1/2/2004 - 
				mvec.y = 100.0f * PRANDFloat();
				mvec.z = 100.0f * PRANDFloat();
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SMOKETRAIL,		// type
					SFX_MOVES | SFX_USES_GRAVITY,
					&pos,					// world pos
					&mvec,					// vel vector
					1.0,					// time to live
					10.0f ) );				// scale
			}

			break;
		case SFX_HIT_EXPLOSION_NOGLOW:
			mvec.x = 0.0f;
			mvec.y = 0.0f;
			mvec.z = -20.0f;
			StartRandomDebris();
			if ( rand() & 1 )
				OTWDriver.AddSfxRequest(
		   			new SfxClass( SFX_LONG_HANGING_SMOKE2,				// type
					&pos,							// world pos
					60.5f,							// time to live
					scale * 0.5f ) );							// scale

			break;
		case SFX_HIT_EXPLOSION_NOSMOKE:
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_EXPLCIRC_GLOW,			// type
				&pos,					// world pos
				1.8f,					// time to live
				scale ) );				// scale
			break;
		case SFX_HIT_EXPLOSION:
			mvec.x = 0.0f;
			mvec.y = 0.0f;
			mvec.z = -20.0f;
			StartRandomDebris();
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_EXPLCIRC_GLOW,			// type
				&pos,					// world pos
				1.8f,					// time to live
				scale ) );				// scale
			if ( rand() & 1 )
				OTWDriver.AddSfxRequest(
		   					new SfxClass( SFX_LONG_HANGING_SMOKE2,				// type
							&pos,							// world pos
							60.5f,							// time to live
							scale * 0.5f ) );							// scale

			break;

		// these next 2 are for campaign calls only (from campweaponfire)
		case SFX_CAMP_FIRE:
			pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 40.0f;
			OTWDriver.AddSfxRequest( new SfxClass( SFX_FIRE,
				 &pos,
				 60.0f,
				 90.0f ) );
			break;
		case SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL:
			if ( pos.z > 0.0f )
				pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 40.0f;
			OTWDriver.AddSfxRequest( new SfxClass( SFX_HIT_EXPLOSION_DEBRISTRAIL,
				 &pos,
				 2.0f,
				 200.0f ) );
			break;

		case SFX_HIT_EXPLOSION_DEBRISTRAIL:
			numBursts = (int)(6.0f * gSfxLOD) + PRANDInt3();
			for ( i = 0; i < numBursts; i++ )
			{
				mvec.x = 60.0f * PRANDFloat();
				mvec.y = 60.0f * PRANDFloat();
				mvec.z = -80.0f * PRANDFloatPos();

				if ( approxDist < SFX_LOD_DIST &&
					 gSfxCount[ SFX_DEBRISTRAIL ] < gSfxLODCutoff &&
					 gSfxCount[ SFX_FIRETRAIL ] < gSfxLODCutoff &&
				 	gTotSfx < gSfxLODTotCutoff )
				{
					if ( PRANDInt3() == 1 )
					{
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_DEBRISTRAIL,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
							&pos,					// world pos
							&mvec,					// vel vector
							6.0,					// time to live
							10.0f ) );				// scale
					}
					else
					{
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_FIRETRAIL,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
							&pos,					// world pos
							&mvec,					// vel vector
							6.0,					// time to live
							10.0f ) );				// scale
					}
				}
			}

			break;
		case SFX_DIST_AIRBURSTS:

			// do LOD for distant sfx
			if ( gTotSfx > gSfxLODDistCutoff )
			{
				break;
			}

			// check distance to view, if too close don't run....
			// about 6 miles?
			if ( approxDist < 10000.0f )
				break;

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );
	
			mpos.x = pos.x + 6000.0f * PRANDFloat() * distScale;
			mpos.y = pos.y + 6000.0f * PRANDFloat() * distScale;
			mpos.z = pos.z - 6000.0f * PRANDFloatPos() * distScale;

			switch( PRANDInt5() )
			{
				case 0:
				case 4:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_AIR_EXPLOSION_NOGLOW,			// type
						&mpos,					// world pos
						1.2f,					// time to live
						600.0f * distScale ) );				// scale
					break;
				case 1:
				case 2:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_SPARKS,			// type
						&mpos,					// world pos
						1.2f,					// time to live
						4.0f * 800.0f * distScale ) );				// scale
					break;
				case 3:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_AIR_HANGING_EXPLOSION,			// type
						&mpos,					// world pos
						2.2f,					// time to live
						500.0f * distScale ) );				// scale
					break;
			}


			break;

		case SFX_AIRBURST:

			mpos.x = pos.x + 800.0f * PRANDFloat();
			mpos.y = pos.y + 800.0f * PRANDFloat();
			mpos.z = pos.z + 100.0f * PRANDFloat();

			switch( PRANDInt5() )
			{
				case 0:
					break;
				case 1:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_SMALL_AIR_EXPLOSION,			// type
						&mpos,					// world pos
						1.3f,					// time to live
						14.0f ) );				// scale
					break;
				case 4:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_SMALL_AIR_EXPLOSION,			// type
						&mpos,					// world pos
						1.3f,					// time to live
						14.0f ) );				// scale
					break;
				case 2:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_SMALL_HIT_EXPLOSION,			// type
						&mpos,					// world pos
						1.4f,					// time to live
						13.0f ) );				// scale
					break;
				case 3:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_SMALL_HIT_EXPLOSION,			// type
						&mpos,					// world pos
						1.4f,					// time to live
						15.0f ) );				// scale
					break;
			}

			if ( gSfxCount[ SFX_LONG_HANGING_SMOKE2 ] < gSfxLODCutoff &&
				 gTotSfx < gSfxLODTotCutoff )
			{
				OTWDriver.AddSfxRequest(
   					new SfxClass( SFX_LONG_HANGING_SMOKE2,				// type
					&mpos,							// world pos
					30.5f,							// time to live
					10.0f ) );							// scale
			}

			// sound
			F4SoundFXSetPos( SFX_IMPACTA1 + PRANDInt5(), TRUE, mpos.x, mpos.y, mpos.z, 1.0f );

			break;
		
		case SFX_DIST_GROUNDBURSTS:

			// do LOD for distant sfx
			if ( gTotSfx > gSfxLODDistCutoff )
			{
				break;
			}

			// check distance to view, if too close don't run....
			// about 6 miles?
			if ( approxDist < 10000.0f )
				break;

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );

			mpos.x = pos.x + 6000.0f * PRANDFloat()* distScale ;
			mpos.y = pos.y + 6000.0f * PRANDFloat()* distScale ;
			mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 10.0f;

			switch( PRANDInt5() )
			{
				case 0:
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_GROUND_FLASH,			// type
					&mpos,					// world pos
					2.2f,					// time to live
					distScale * 6300.0f ) );				// scale
				case 4:
					OTWDriver.AddSfxRequest(
						// new SfxClass( SFX_GROUND_EXPLOSION_NO_CRATER,	// type
						new SfxClass( SFX_SPARKS,	// type
						&mpos,					// world pos
						2.2f,					// time to live
						4.0f * 800.0f * distScale ) );				// scale
					break;
				case 1:
					OTWDriver.AddSfxRequest(
					new SfxClass( SFX_GROUND_GLOW,			// type
					&mpos,					// world pos
					4.2f,					// time to live
					distScale * 3300.0f ) );				// scale
					break;
				case 2:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_ARTILLERY_EXPLOSION,			// type
						&mpos,					// world pos
						1.2f,					// time to live
						800.0f * distScale ) );				// scale
					break;
				case 3:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_AIR_HANGING_EXPLOSION,			// type
						&mpos,					// world pos
						2.2f,					// time to live
						500.0f * distScale ) );				// scale
					break;
			}


			break;

		case SFX_GROUNDBURST:

			// evry time thru, we increase the radius
			vec.z += 40.0f;
			pos.x += vec.x * sfxFrameTime;
			pos.y += vec.y * sfxFrameTime;
			numBursts = 3 + (int)( vec.z * 0.08f * gSfxLOD );
			radstep = 2.0f * PI / (float)numBursts;
			rads = 0;

			// perpendiclular vec
			distScale = (float)sqrt( vec.x * vec.x + vec.y * vec.y );
			if ( distScale < 0.0001f ) distScale = 1.0f;
			mvec.x = -vec.y / distScale;
			mvec.y = vec.x / distScale;

			distScale = min( vec.z, 200.0f );

			for ( i=0; i < numBursts; i++ )
			{
				// mpos.x = pos.x + distScale * cos( rads ) + PRANDFloat() * distScale * 0.4f;
				// mpos.y = pos.y + distScale * sin( rads ) + PRANDFloat() * distScale * 0.4f;
				mpos.x = pos.x + mvec.x * PRANDFloat() * distScale;
				mpos.y = pos.y + mvec.y * PRANDFloat() * distScale;
				mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 10.0f;

				rads += radstep;

				switch( PRANDInt5() )
				{
					case 0:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_GROUND_STRIKE_NOFIRE,	// type
							&mpos,					// world pos
							1.5f,					// time to live
							15.0f ) );				// scale
						break;
					case 1:
						vec2.x = 0.0f;
						vec2.y = 0.0f;
						vec2.z = -50.0f;
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_FIRE4,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
							&mpos,					// world pos
							&vec2,					// vel vector
							1.5,					// time to live
							20.25f ) );				// scale
						break;
					case 2:
						OTWDriver.AddSfxRequest(
							new SfxClass(SFX_SMALL_HIT_EXPLOSION,				// type
							&mpos,							// world pos
							1.5f,			// time to live
							15.0f) );		// scale
						break;
					case 3:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_GROUND_STRIKE_NOFIRE,	// type
							&mpos,					// world pos
							1.5f,					// time to live
							15.0f ) );				// scale
						break;
					case 4:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_SMALL_AIR_EXPLOSION,	// type
							&mpos,					// world pos
							1.5f,					// time to live
							15.0f ) );				// scale
				}
			}
			F4SoundFXSetPos( SFX_BOOMG1 + PRANDInt5(), TRUE, mpos.x, mpos.y, mpos.z, 1.0f );

			break;

		case SFX_DIST_ARMOR:

			// do LOD for distant sfx
			if ( gTotSfx > gSfxLODDistCutoff )
			{
				break;
			}

			// check distance to view, if too close don't run....
			// about 6 miles?
			if ( approxDist < 10000.0f )
				break;

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );

			mpos.x = pos.x + 6000.0f * PRANDFloat()* distScale ;
			mpos.y = pos.y + 6000.0f * PRANDFloat()* distScale ;
			mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 20.0f;

			switch( PRANDInt5() )
			{
				case 0:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUND_FLASH,			// type
						&mpos,					// world pos
						2.2f,					// time to live
						distScale * 6800.0f ) );				// scale
						break;
				case 1:
				case 4:
					// vec is normalized, further away = faster
					// mvec.x = vec.x * ( 150.0f + 3800.0f * distScale );
					// mvec.y = vec.y * ( 150.0f + 3800.0f * distScale );
					// mvec.z = vec.z * ( 150.0f + 3800.0f * distScale );
					if ( travelDist < 12000.0f )
					{
						mvec.x = vec.x / travelDist * TRACER_VELOCITY;
						mvec.y = vec.y / travelDist * TRACER_VELOCITY;
						mvec.z = vec.z / travelDist * TRACER_VELOCITY;
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_TRACER_FIRE,			// type
							SFX_EXPLODE_WHEN_DONE | SFX_MOVES | SFX_NO_GROUND_CHECK ,						// flags
							&mpos,					// world pos
							&mvec,					// vector for movement
							travelDist/TRACER_VELOCITY,					// time to live
							180.0f * distScale) );				// scale
					}
					else
					{
						mpos.x += vec.x;
						mpos.y += vec.y;
						mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 20.0f;
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_ARTILLERY_EXPLOSION,			// type
							&mpos,					// world pos
							1.2f,					// time to live
							800.0f * distScale ) );				// scale
					}
					break;
				case 2:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUND_GLOW,			// type
						&mpos,					// world pos
						4.2f,					// time to live
						3300.0f * distScale ) );				// scale
					break;
				case 3:
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_AIR_SMOKECLOUD,			// type
						&mpos,					// world pos
						12.2f,					// time to live
						420.0f  * distScale ) );				// scale
					break;
			}


			break;

		case SFX_DIST_INFANTRY:

			// do LOD for distant sfx
			if ( gTotSfx > gSfxLODDistCutoff )
			{
				break;
			}

			// check distance to view, if too close don't run....
			// about 6 miles?
			if ( approxDist < 10000.0f )
				break;

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );

			mpos.x = pos.x + 6000.0f * PRANDFloat()* distScale ;
			mpos.y = pos.y + 6000.0f * PRANDFloat()* distScale ;
			mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 20.0f;

			switch( PRANDInt5() )
			{
				case 0:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUND_FLASH,			// type
						&mpos,					// world pos
						2.2f,					// time to live
						distScale * 6800.0f ) );				// scale
						break;
				case 1:
					// vec is normalized, further away = faster
					// mvec.x = vec.x * ( 150.0f + 3800.0f * distScale );
					// mvec.y = vec.y * ( 150.0f + 3800.0f * distScale );
					// mvec.z = vec.z * ( 150.0f + 3800.0f * distScale );
					mvec.x = vec.x / travelDist * TRACER_VELOCITY;
					mvec.y = vec.y / travelDist * TRACER_VELOCITY;
					mvec.z = vec.z / travelDist * TRACER_VELOCITY;
					if ( travelDist < 12000.0f )
					{
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_TRACER_FIRE,			// type
							SFX_EXPLODE_WHEN_DONE | SFX_MOVES | SFX_NO_GROUND_CHECK | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,						// flags
							&mpos,					// world pos
							&mvec,					// vector for movement
							travelDist/TRACER_VELOCITY,					// time to live
							180.0f * distScale ) );				// scale
					}
					else
					{
						mpos.x += vec.x;
						mpos.y += vec.y;
						mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 20.0f;
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_ARTILLERY_EXPLOSION,			// type
							&mpos,					// world pos
							1.2f,					// time to live
							800.0f * distScale ) );				// scale
					}
					break;
				case 2:
				case 3:
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUND_GLOW,			// type
						&mpos,					// world pos
						4.2f,					// time to live
						3300.0f * distScale ) );				// scale
					break;
				case 4:
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_AIR_SMOKECLOUD,			// type
						&mpos,					// world pos
						12.2f,					// time to live
						420.0f  * distScale ) );				// scale
					break;
			}


			break;

		case SFX_DIST_SAMLAUNCHES:

			// do LOD for distant sfx
			if ( gTotSfx > gSfxLODDistCutoff )
			{
				break;
			}

			// check distance to view, if too close don't run....
			// about 6 miles?
			if ( approxDist < 10000.0f )
				break;

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );

			mpos.x = pos.x + 6000.0f * PRANDFloat() * distScale;
			mpos.y = pos.y + 6000.0f * PRANDFloat() * distScale;
			mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 50.0f;

			// start sam launch effect
			/*
			OTWDriver.AddSfxRequest(
				SFX_SAM_LAUNCH,			// type
				0,						// flags
				&mpos,					// world pos
				&rot,					// orientation matrix
				&mvec,					// vector for movement
				2.2,					// time to live
				52.0f );				// scale
				*/

			// start missile launch from sam loc
			OTWDriver.AddSfxRequest(
				new SfxClass(SFX_AIR_SMOKECLOUD,			// type
				&mpos,					// world pos
				12.2f,					// time to live
				420.0f  * distScale ) );				// scale


			if ( !PRANDInt3() )
			{
				// vec is normalized, further away = faster
				// mvec.x = vec.x * ( 150.0f + 1000.0f * distScale );
				// mvec.y = vec.y * ( 150.0f + 1000.0f * distScale );
				// mvec.z = vec.z * ( 150.0f + 1000.0f * distScale );
				mvec.x = vec.x / travelDist * 1500.0f;
				mvec.y = vec.y / travelDist * 1500.0f;
				mvec.z = vec.z / travelDist * 1500.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_MISSILE_LAUNCH,			// type
					mflags,						// flags
					&mpos,					// world pos
					&mvec,					// vector for movement
					travelDist/1500.0f,					// time to live
					420.0f * distScale ) );				// scale
			}
			else
			{
				// vec is normalized, further away = faster
				// mvec.x = vec.x * ( 150.0f + 3800.0f * distScale );
				// mvec.y = vec.y * ( 150.0f + 3800.0f * distScale );
				// mvec.z = vec.z * ( 150.0f + 3800.0f * distScale );
				mvec.x = vec.x / travelDist * TRACER_VELOCITY;
				mvec.y = vec.y / travelDist * TRACER_VELOCITY;
				mvec.z = vec.z / travelDist * TRACER_VELOCITY;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_TRACER_FIRE,			// type
					SFX_EXPLODE_WHEN_DONE | SFX_MOVES | SFX_NO_GROUND_CHECK | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,						// flags
					&mpos,					// world pos
					&mvec,					// vector for movement
					travelDist/TRACER_VELOCITY,					// time to live
					180.0f * distScale ) );				// scale
			}

			break;

		case SFX_DIST_AALAUNCHES:

			// do LOD for distant sfx
			if ( gTotSfx > gSfxLODDistCutoff )
			{
				break;
			}

			// check distance to view, if too close don't run....
			// about 6 miles?
			if ( approxDist < 10000.0f )
				break;

			// get a distance scale where 1.0 is about 60 miles away
			distScale = max( 0.1f, approxDist/200000.0f );

			mpos.x = pos.x + 2000.0f * PRANDFloat() * distScale;
			mpos.y = pos.y + 2000.0f * PRANDFloat() * distScale;
			mpos.z = OTWDriver.GetGroundLevel (mpos.x, mpos.y) - 50.0f;

			if ( !PRANDInt3() )
			{
				// vec is normalized, further away = faster
				// mvec.x = vec.x * ( 150.0f + 1000.0f * distScale );
				// mvec.y = vec.y * ( 150.0f + 1000.0f * distScale );
				// mvec.z = vec.z * ( 150.0f + 1000.0f * distScale );
				mvec.x = vec.x / travelDist * 1500.0f;
				mvec.y = vec.y / travelDist * 1500.0f;
				mvec.z = vec.z / travelDist * 1500.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_MISSILE_LAUNCH,			// type
					mflags,						// flags
					&mpos,					// world pos
					&mvec,					// vector for movement
					travelDist/1500.0f,					// time to live
					420.0f * distScale ) );				// scale
			}
			else
			{
				// vec is normalized, further away = faster
				// mvec.x = vec.x * ( 150.0f + 3800.0f * distScale );
				// mvec.y = vec.y * ( 150.0f + 3800.0f * distScale );
				// mvec.z = vec.z * ( 150.0f + 3800.0f * distScale );
				mvec.x = vec.x / travelDist * TRACER_VELOCITY;
				mvec.y = vec.y / travelDist * TRACER_VELOCITY;
				mvec.z = vec.z / travelDist * TRACER_VELOCITY;
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_TRACER_FIRE,			// type
					SFX_EXPLODE_WHEN_DONE | SFX_MOVES | SFX_NO_GROUND_CHECK | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,						// flags
					&mpos,					// world pos
					&mvec,					// vector for movement
					travelDist/TRACER_VELOCITY,					// time to live
					180.0f * distScale) );				// scale
			}

			break;
		case SFX_INCENDIARY_EXPLOSION:
			mpos.x = pos.x;
			mpos.y = pos.y;
			mpos.z = OTWDriver.GetGroundLevel( pos.x, pos.y );
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_GROUND_GLOW,			// type
				&mpos,					// world pos
				obj2d->GetAlphaTimeToLive(),					// time to live
				scale ) );				// scale
			break;

		case SFX_NAPALM:
			// send up some fire trails
			/*
			** EDG: Broken right now!
			*/
			numBursts =  2 + (int)( 2.0f * gSfxLOD );
			rads = OTWDriver.GetGroundLevel( pos.x, pos.y );
			mvec.z = 0.0f;

			for ( i = 0; i < numBursts; i++ )
			{
				mpos.x = pos.x + 600.0f * PRANDFloat();
				mpos.y = pos.y + 600.0f * PRANDFloat();
				mpos.z = rads  - 30.0f - PRANDFloatPos() * 100.0f;

				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_INCENDIARY_EXPLOSION,		// type
					&mpos,					// world pos
					2.5f,					// time to live
					200.0f + 500.0f * PRANDFloatPos() ) );				// scale
			}

			pos.x += vec.x * 0.2f;
			pos.y += vec.y * 0.2f;
			F4SoundFXSetPos( SFX_BOOMG1, TRUE, mpos.x, mpos.y, mpos.z, 1.0f );
			break;

		case SFX_RISING_GROUNDHIT_EXPLOSION_DEBRISTRAIL:
			// mpos.x = pos.x + vec.x * PRANDFloat();
			// mpos.y = pos.y + vec.y * PRANDFloat();
			int randint;

			randint = rand() & 0x03;

			if ( randint == 1 )
				mpos.x = pos.x - vec.x;
			else if ( randint == 0 )
				mpos.x = pos.x + vec.x;
			else
				mpos.x = pos.x + vec.x * PRANDFloat();

			randint = rand() & 0x03;

			if ( randint == 1 )
				mpos.y = pos.y - vec.y;
			else if ( randint == 0 )
				mpos.y = pos.y + vec.y;
			else
				mpos.y = pos.y + vec.y * PRANDFloat();

			mpos.z = pos.z;

			OTWDriver.AddSfxRequest(
				new SfxClass(SFX_HIT_EXPLOSION_DEBRISTRAIL,				// type
				&mpos,							// world pos
				1.5f,			// time to live
				scale) );		// scale


			mpos.z = OTWDriver.GetGroundLevel( mpos.x, mpos.y );
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_GROUND_FLASH,			// type
				&mpos,					// world pos
				2.2f,					// time to live
				scale * 1.4f ) );				// scale

			// sound
			F4SoundFXSetPos( SFX_BOOMG1 + PRANDInt5(), TRUE, mpos.x, mpos.y, mpos.z, 1.0f );
			// raise it
			pos.z += vec.z;
			break;
		default:
			break;
	} // end switch
}


/*
** Name: RunSfxCompletion
** Description:
**		What happens to effect when it's done?
*/
void
//SfxClass::RunSfxCompletion( BOOL hitGround, float groundZ, int groundType )
SfxClass::RunSfxCompletion( BOOL hitGround, float, int groundType )
{
	// do we explode at end?
	if ( flags & SFX_EXPLODE_WHEN_DONE )
	{
		// add new effect!
		if ( hitGround )
		{
			pos.z -= 40.0f;
			//  water river is 1-2
		 	if ( (groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER) )
			{
				F4SoundFXSetPos( SFX_SPLASH, TRUE, pos.x, pos.y, pos.z, 1.0f );
				OTWDriver.AddSfxRequest(
						new SfxClass( SFX_WATER_STRIKE,				// type
						&pos,					// world pos
						2.0f,							// time to live
						100.0f ) );							// scale
			}
			else
			{
				if ( PRANDInt6() )
				{
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUND_STRIKE,				// type
						&pos,					// world pos
						2.0f,							// time to live
						100.0f ) );							// scale
				}
				else
				{
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_ARTILLERY_EXPLOSION,				// type
						&pos,					// world pos
						2.0f,							// time to live
						100.0f ) );							// scale
				}

				// temp craters
				/*
				OTWDriver.AddSfxRequest(
							new SfxClass( SFX_RAND_CRATER,				// type
							0,								// flags
							&pos,							// world pos
							(struct Trotation *)&IMatrix,						// world orientation
							&vec,							// vector
							15.0f,							// time to live
							50.0f + 50.0f * PRANDFloatPos() ) );							// scale
				*/
			}
		}
		else if ( type == SFX_NOTRAIL_FLARE ||
				  type == SFX_SMOKING_PART ||
				  type == SFX_FLAMING_PART ||
				  type == SFX_MISSILE_LAUNCH )
		{
			OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SMALL_AIR_EXPLOSION,				// type
					&pos,					// world pos
					1.2f,							// time to live
					40.0f ) );						// scale
		}
		else if ( type == SFX_TRACER_FIRE )
		{
			float distScale = max( 0.1f, approxDist/200000.0f );
			OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SMALL_AIR_EXPLOSION,				// type
					&pos,					// world pos
					1.5f,							// time to live
					400.0f * distScale) );						// scale
		}
		else
		{
			OTWDriver.AddSfxRequest(
					new SfxClass( SFX_AIR_HANGING_EXPLOSION,		// type
					&pos,					// world pos
					2.0f,							// time to live
					250.0f ) );						// scale
		}
		if ( hitGround )
			F4SoundFXSetPos( SFX_BOOMG1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f );
		else
			F4SoundFXSetPos( SFX_BOOMA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f );
	}
	else if ( hitGround )
	{
		pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 10.0f;
	 	if ( (groundType == COVERAGE_WATER ||
			  groundType == COVERAGE_RIVER) &&
			  type != SFX_LIGHT_DEBRIS )
		{
			F4SoundFXSetPos( SFX_SPLASH, TRUE, pos.x, pos.y, pos.z, 1.0f );
			OTWDriver.AddSfxRequest(
					new SfxClass( SFX_WATER_STRIKE,				// type
					&pos,					// world pos
					1.5f,							// time to live
					20.0f ) );							// scale
		}
		else
		{
			if ( type == SFX_GROUNDBURST )
			{
				switch( PRANDInt5() )
				{
					case 0:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_GROUND_STRIKE_NOFIRE,	// type
							&pos,					// world pos
							1.5f,					// time to live
							15.0f ) );				// scale
						break;
					case 1:
						vec.x = 0.0f;
						vec.y = 0.0f;
						vec.z = -50.0f;
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_FIRE4,		// type
							SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
							&pos,					// world pos
							&vec,					// vel vector
							1.5,					// time to live
							20.25f ) );				// scale
						break;
					case 2:
						OTWDriver.AddSfxRequest(
							new SfxClass(SFX_SMALL_HIT_EXPLOSION,				// type
							&pos,							// world pos
							1.5f,			// time to live
							15.0f) );		// scale
						F4SoundFXSetPos( SFX_BOOMG1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f );
						break;
					case 3:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_GROUND_FLASH,			// type
							&pos,					// world pos
							2.2f,					// time to live
							75.4f ) );				// scale
						break;
					case 4:
						OTWDriver.AddSfxRequest(
							new SfxClass( SFX_SMALL_AIR_EXPLOSION,	// type
							&pos,					// world pos
							1.5f,					// time to live
							15.0f ) );				// scale
						F4SoundFXSetPos( SFX_BOOMG1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f );
						break;
				}
			}
			else if ( type == SFX_GUN_TRACER )
			{
				OTWDriver.AddSfxRequest(
					new SfxClass(SFX_SPARKS,				// type
					&pos,							// world pos
					0.75f,							// time to live
					31.0f));						// scale
			}
		}
	}

	/*
	if ( type == SFX_FIRE1 )
	{
		OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE2,				// type
					&pos,							// world pos
					0.3f,			// time to live
					scale * 1.1 ) );		// scale
	}
	else if ( type == SFX_FIRE2 )
	{
		OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE3,				// type
					&pos,							// world pos
					0.3f,			// time to live
					scale * 1.1 ) );		// scale
	}
	else if ( type == SFX_FIRE3 )
	{
		OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE4,				// type
					&pos,							// world pos
					0.3f,			// time to live
					scale * 1.1 ) );		// scale
	}
	else if ( type == SFX_FIRE4 )
	{
		OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE5,				// type
					&pos,							// world pos
					0.3f,			// time to live
					scale * 1.1 ) );		// scale
	}
	else if ( type == SFX_FIRE5 )
	{
		OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE6,				// type
					&pos,							// world pos
					0.3f,			// time to live
					scale * 1.1 ) );		// scale
	}
	else if ( type == SFX_FIRE6 )
	{
		OTWDriver.AddSfxRequest(
					new SfxClass(SFX_FIRE7,				// type
					&pos,							// world pos
					0.3f,			// time to live
					scale * 1.1 ) );		// scale
	}
	else
	*/

	// all trails get timed out
	if ( objTrail && type != SFX_TIMER )
	{
		OTWDriver.AddSfxRequest(
				new SfxClass( 
				30.0f,							// time to live
				objTrail ) );						// scale
		objTrail = NULL;
	}
}


/*
** Name: GroundReflection
** Description:
**		Reflects movement vector based on hit with the ground
*/
void
SfxClass::GroundReflection( void )
{
	// NOTE: a big simplification is that we will assume the ground
	// normal is always pointing up ( 0, 0, -1 ).  The reflection vector
	// therefore becomes a simple matter of making the z component of
	// movement vector negative.  To be accurate, we may need to end up
	// constructing what amounts to a reflection matrix built from
	// retrieving the ground normal and then multiply the vector by the
	// matrix.
	if ( vec.z > 0.0f )
		vec.z = -vec.z;
}


/*
** Name: ACMIStart
** Description:
**		Starts the effect by adding it to the draw list.
**		And sets the timetolive
**		ACMI Version
*/
void
SfxClass::ACMIStart (RViewPoint *acmiView, float startTime, float currTime )
{
	inACMI = TRUE;
	viewPoint = acmiView;

	// set the time it will die
	timeToLive += startTime;

	// last ACMI time set
	lastACMItime = (float)currTime;
	startACMItime = startTime;

	// get the approximate distance to the viewer
	GetApproxViewDist( currTime );

	// timers don't get added to draw list
	if ( flags & SFX_TIMER_FLAG )
		return;

	// insert the effect's object into the display list
	if ( obj2d )
	{
		obj2d->SetStartTime( (DWORD)( startTime * 1000 ), (DWORD)( currTime * 1000 ) );
		viewPoint->InsertObject(obj2d);
	}
	if ( objBSP )
	{
		viewPoint->InsertObject(objBSP);
	}
	if ( objTrail )
	{
		// a hack -- scale trail of missile launch based on dist
		if ( type == SFX_MISSILE_LAUNCH )
		{
			float distScale = max( 0.1f, approxDist/10000.0f );
			objTrail->SetScale( distScale );
		}
		objTrail->KeepStaleSegs( TRUE );
		viewPoint->InsertObject(objTrail);
	}
	if ( objTracer )
	{
		viewPoint->InsertObject(objTracer);
	}
	if ( baseObj )
	{
		viewPoint->InsertObject(baseObj->drawPointer);
	}

}

/*
** Name: ACMIExec
** Description:
**		Executes the effect
**		Returns FALSE when completed
**		ACMI Version
*/
BOOL
SfxClass::ACMIExec ( float currTime )
{
	float groundZ=0.0F;
	BOOL hitGround = FALSE;
	Tpoint mpos={0.0F}, newpos={0.0F}, newvec={0.0F};
	Trotation rot = IMatrix;
	int groundType= COVERAGE_PLAINS;
	float dTFrame=0.0F, dT=0.0F;

	// set delta time from last acmitime
	dTFrame = (float)currTime - lastACMItime;
	lastACMItime = (float)currTime;

	// set deltaT from the start of the effect
	// position and movement is always determined relative to start time
	dT = (float)currTime - startACMItime;


	// get approx distance to viewer based on timer
	if ( currTime >= distTimer )
		GetApproxViewDist(currTime);
	

	// if this effect is only drives other secondary effects, see if
	// the count has reached 0, then kill it if so
	// shouldn't have these in ACMI
	if ( flags & SFX_SECONDARY_DRIVER )
	{
		return FALSE;
	}

	// check for hit with ground
	if ( (flags & SFX_MOVES) && !(flags & SFX_NO_GROUND_CHECK) )
	{
		// 1st get approximation
		groundZ = OTWDriver.GetApproxGroundLevel(pos.x, pos.y);
		if (  pos.z - groundZ  > -100.0f )
		{
		 	groundZ = OTWDriver.GetGroundLevel( pos.x, pos.y );
			if ( pos.z >= groundZ )
			{
				hitGround = TRUE;
				groundType = OTWDriver.GetGroundType (pos.x, pos.y);
			}
		}
	}

	// does this object bounce?
	/*
	if ( hitGround && (flags & SFX_BOUNCES) && groundType > 2 )
	{
		// calcuate the new movement vector
		GroundReflection();
		pos.z = groundZ - 4.0f;
		// momentum loss
		vec.x *= 0.50f;
		vec.y *= 0.50f;
		vec.z *= 0.50f;
		hitGround = FALSE;
	}
	*/

	// lived long enough?
	if ( currTime > timeToLive || hitGround )
	{
		// probably will need to remove from draw list here ...
		// done with this effect
		return FALSE;

	}

	// do we need to move it?
	if ( !(flags & SFX_MOVES) || (flags & SFX_TIMER_FLAG) )
	    return TRUE;

	newvec = vec;
	if ( flags & (SFX_USES_GRAVITY | SFX_TRAJECTORY) )
	{
		// gravity = 32 ft/secSq
		newvec.z += 32.0f * dT;
	}

	// update position based on vector
	newpos.x = pos.x + vec.x * dT;
	newpos.y = pos.y + vec.y * dT;
	if ( flags & (SFX_USES_GRAVITY | SFX_TRAJECTORY) )
	{
		// pos(t) = p0 + v0(t) + 1/2a(t^2)
		newpos.z = pos.z + vec.z * dT + 0.5f * 32.0f * dT * dT;
	}
	else
	{
		newpos.z = pos.z + vec.z * dT;
	}

	// hack, we've been noticing things still falling thru the ground
	// double check position
	if ( newpos.z > groundZ )
		newpos.z = groundZ;

	// for 2d objects set new position
	if ( obj2d )
	{
		obj2d->SetPosition( &newpos );
	}

	// for BSP objects set new position
	if ( objBSP )
	{
		Trotation rot;

		if ( type == SFX_EJECT1 )
		{
			// hack! ejection is sideways
			rot.M11 = 1.0f;
			rot.M12 = 0.0f;
			rot.M13 = 0.0f;
			rot.M21 = 0.0f;
			rot.M22 = 0.0f;
			rot.M23 = -1.0f;
			rot.M31 = 0.0f;
			rot.M32 = 1.0f;
			rot.M33 = 0.0f;
		}
	  	else if (type == SFX_MOVING_BSP)
      	{
         	MatrixTranspose (&objBSP->orientation, &rot);
      	}
      	else
	  	{
			// right now, no rotation
			rot = IMatrix;
	  	}

		objBSP->Update( &newpos, &rot );
	}

	// for bsp objects
	if ( baseObj )
	{
		baseObj->SetPosition (newpos.x, newpos.y, newpos.z );
		baseObj->SetDelta (newvec.x, newvec.y, newvec.z );

		if (newvec.x == 0.0f && newvec.y == 0.0f && newvec.z == 0.0f) 
			baseObj->SetYPR(0.0f, 0.0f, 0.0f);
		else
			baseObj->SetYPR(
				baseObj->Yaw() + baseObj->YawDelta() * dT,
				baseObj->Pitch() + baseObj->PitchDelta() * dT,
				baseObj->Roll() + baseObj->RollDelta() * dT);
		CalcTransformMatrix (baseObj );
		OTWDriver.ObjectSetData (baseObj, &newpos, &rot);
		((DrawableBSP*)(baseObj->drawPointer))->Update(&newpos, &rot);
	}

	// for drawable trails 
	if ( objTrail )
	{
		if ( dTFrame < 0.0f )
			objTrail->RewindTrail ( (DWORD)(currTime * 1000) );
		else if ( dTFrame > 0.0f )
			objTrail->AddPointAtHead (&newpos, (DWORD)(currTime * 1000) );
	}

	// for drawable tracers
	if ( objTracer )
	{
		// tracers take a start and end position
		// use a value that's about 1/4 their velocity vector
		if ( type == SFX_GUN_TRACER )
		{
			mpos.x = newpos.x - ( newvec.x * sfxFrameTime * 0.2f );
			mpos.y = newpos.y - ( newvec.y * sfxFrameTime * 0.2f ) ;
			mpos.z = newpos.z - ( newvec.z * sfxFrameTime * 0.2f );
		}
		else 
		{
			mpos.x = newpos.x - ( newvec.x * 0.02f );
			mpos.y = newpos.y - ( newvec.y * 0.02f );
			mpos.z = newpos.z - ( newvec.z * 0.02f );
		}
		objTracer->Update( &newpos, &mpos );
	}

	return TRUE;
}


/*
** Name: SetLOD
** Description:
**		Sets detail level for special effects.
**		static class function
*/
void
SfxClass::SetLOD ( float objDetail )
{
	// objDetail is based on PlayerOptions.SfxLevel
	// and has a value of 0.0 to 5.0.  We want to normalize this to
	// be in thee 0 - 1 range.
	gSfxLOD = max( objDetail/5.0f, 0.1f );

	// set the cutoff level for some special effects
	gSfxLODCutoff = 40 + (int)( 160.0f * gSfxLOD );

	// set the cutoff level for some special effects
	// this is the total number currently running
	gSfxLODTotCutoff = 200 + (int)( 600.0f * gSfxLOD );

	// distant effects are compared against total running
	// gSfxLODDistCutoff = 50 + (int)( 160.0f * gSfxLOD );
	gSfxLODDistCutoff = (int)((float)gSfxLODTotCutoff * 0.75f);

	// set the 2d detail level
 	Drawable2D::SetLOD( max( 0.8f, gSfxLOD ) );

	// MonoPrint( "New SFX LOD = %f, Obj Detail = %f\n", gSfxLOD, objDetail );
}

/*
** Name: StartRandomDebris
** Description:
*/
void
SfxClass::StartRandomDebris ( void )
{
	Tpoint mvec, mpos;
	int i, numBursts;
	float debrisScale;
	float timetl;
	int numRunning;
	BOOL groundHit = FALSE;

	numRunning = gSfxCount[ SFX_DARK_DEBRIS ] +
				 gSfxCount[ SFX_LIGHT_DEBRIS ] +
				 gSfxCount[ SFX_DEBRISTRAIL ] +
				 gSfxCount[ SFX_FIRETRAIL ] +
				 gSfxCount[ SFX_FIRE_DEBRIS ];

	if ( numRunning > gSfxLODCutoff * 3 ||
		 gTotSfx >= gSfxLODTotCutoff )
		return;

	debrisScale = 3.0f;
	timetl = 0.5f + 2.0f * debrisScale / 10.0f;
	mpos = pos;

	if ( type == SFX_WATER_EXPLOSION ||
		 type == SFX_GROUND_STRIKE ||
		 type == SFX_WATER_STRIKE ||
		 type == SFX_ARTILLERY_EXPLOSION ||
		 type == SFX_GROUND_EXPLOSION )
	{
		debrisScale = min( 7.0f, scale * 0.1f );
		mpos.z = OTWDriver.GetGroundLevel( mpos.x, mpos.y ) - 3.0f;
		groundHit = TRUE;
	}
	else if ( type == SFX_SPARKS )
	{
		mpos.z = OTWDriver.GetGroundLevel( mpos.x, mpos.y );
		if ( pos.z - mpos.z > -10.0f )
		{
			groundHit = TRUE;
			mpos.z -= 3.0f;
		}
		else
		{
			mpos.z = pos.z;
		}
	}

	// try and scale debris to smaller size if viewed close
	if ( approxDist < 15000.0f )
	{
		debrisScale = max( 0.5f, debrisScale * approxDist/15000.0f );
	}

	numBursts = (int)( 7.0f * gSfxLOD );

	for ( i = 0; i < numBursts; i++ )
	{

		if ( groundHit == TRUE )
		{
			mvec.x = 80.0f * PRANDFloat() * scale * 0.01f;
			mvec.y = 80.0f * PRANDFloat() * scale * 0.01f;
			mvec.z = -30.0f * PRANDFloatPos() * scale * 0.03f;
			mvec.z = min( -80.0f, mvec.z );
		}
		else
		{
			mvec.x = 90.0f * PRANDFloat();
			mvec.y = 90.0f * PRANDFloat();
			mvec.z = 90.0f * PRANDFloat();
		}

		if ( type == SFX_WATER_EXPLOSION ||
		 	 type == SFX_WATER_STRIKE )
	    {
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_LIGHT_DEBRIS,		// type
				SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
				&mpos,					// world pos
				&mvec,					// vel vector
				timetl,					// time to live
				debrisScale ) );				// scale
		}
		else if ( type == SFX_SPARKS )
		{
			if ( (rand() & 1) || groundHit )
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_DARK_DEBRIS,		// type
					SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
					&mpos,					// world pos
					&mvec,					// vel vector
					timetl,					// time to live
					debrisScale ) );				// scale
			else
				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_EXPLSTAR_GLOW,		// type
					SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
					&mpos,					// world pos
					&mvec,					// vel vector
					timetl,					// time to live
					debrisScale ) );				// scale
		}
		else
		{
			OTWDriver.AddSfxRequest(
				new SfxClass( SFX_DARK_DEBRIS,		// type
				SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
				&mpos,					// world pos
				&mvec,					// vel vector
				timetl,					// time to live
				debrisScale ) );				// scale
		}
	}
}

// optimize back on
#pragma optimize( "", on )




