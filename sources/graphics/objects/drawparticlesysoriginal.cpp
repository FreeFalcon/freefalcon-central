#if 1
#include "DrawParticleSys.h"
#include "profiler.h"
#include "REDprofiler.h"

//JAM 19Apr04
struct Point { float x,y,z; };

/***************************************************************************\
    DrawParticleSys

	Several Classes and data structures for creating particle effects

	DrawableParticleSys
	  + ParticleNode
	      + SubPartXXX
		  + SubPartXXX
		  + ...
	  + ParticleNode
	      + SubPartXXX


    MLR
\***************************************************************************/

#include "TimeMgr.h"
#include "falclib/include/token.h"
#include "RenderOW.h"
#include "Matrix.h"
#include "Tex.h"
#include "Weather.h"
#include "FakeRand.h"
#include "falclib/include/falclib.h"
#include "sim\include\simlib.h" // MLR needed for SetVelocity since objects set there Delta values per frame
#include "sim\include\otwdrive.h" // MLR needed for SetVelocity since objects set there Delta values per frame
#include "falclib/include/fsound.h"
#include "drawsgmt.h"
#include "drawbsp.h"
//#include "F4Compress.h"
#include "terrtex.h"
#include "sfx.h"
#include "falclib/include/entity.h"
#include "PSData.h"
#include "drawable.h"

#ifndef F4COMPRESS


// for when fakerand just won't do
#define	NRANDPOS ((float)( (float)rand()/(float)RAND_MAX ))
#define DTR 0.01745329F

extern int g_nGfxFix;
extern char FalconDataDirectory[];
extern char FalconObjectDataDir[];
extern int sGreenMode;

// Cobra - Purge the PS list every PurgeTimeInc msec.
extern int g_nPSPurgeInterval;
static DWORD TimeToPurge = 0L;
static WORD	 ParticleFilterCount;							// COBRA - RED - Counter of SFX adding for filtering
#define	MAX_PARTICLE_FILTER_LEVEL	10						// COBRA - RED - Level for SFX Filter

//static DWORD PurgeTimeInc = 60000L;
static DWORD PurgeTimeInc = g_nPSPurgeInterval;
static DWORD TimeToPurgeAll = 0L;
static DWORD PurgeAllTimeInc = max(g_nPSPurgeInterval*10, 600000);

bool g_bNoParticleSys=0;
BOOL gParticleSysGreenMode=0;
Tcolor gParticleSysLitColor={1,1,1};

extern int g_nPSKillFPS;//Cobra
extern bool g_bHighSFX; // Cobra

/**** Static class data ***/
BOOL    DrawableParticleSys::greenMode	= FALSE;
char	*DrawableParticleSys::nameList[SFX_NUM_TYPES+1]=
{
	"$NONE",
    "$AIR_HANGING_EXPLOSION",
    "$SMALL_HIT_EXPLOSION",
    "$AIR_SMOKECLOUD",
    "$SMOKING_PART",
    "$FLAMING_PART",
    "$GROUND_EXPLOSION",
    "$TRAIL_SMOKECLOUD",
    "$TRAIL_FIREBALL",
    "$MISSILE_BURST",
    "$CLUSTER_BURST",
    "$AIR_EXPLOSION",
    "$EJECT1",
    "$EJECT2",
    "$SMOKERING",
    "$AIR_DUSTCLOUD",
    "$GUNSMOKE",
    "$AIR_SMOKECLOUD2",
    "$NOTRAIL_FLARE",
    "$FEATURE_CHAIN_REACTION",
    "$WATER_EXPLOSION",
    "$SAM_LAUNCH",
    "$MISSILE_LAUNCH",
    "$DUST1",
    "$EXPLCROSS_GLOW",
    "$EXPLCIRC_GLOW",
    "$TIMER",
    "$DIST_AIRBURSTS",
    "$DIST_GROUNDBURSTS",
    "$DIST_SAMLAUNCHES",
    "$DIST_AALAUNCHES",
    "$EXPLSTAR_GLOW",
    "$RAND_CRATER",
    "$GROUND_EXPLOSION_NO_CRATER",
    "$MOVING_BSP",
    "$DIST_ARMOR",
    "$DIST_INFANTRY",
    "$TRACER_FIRE",
    "$FIRE",
    "$GROUND_STRIKE",
    "$WATER_STRIKE",
    "$VERTICAL_SMOKE",
    "$TRAIL_FIRE",
    "$BILLOWING_SMOKE",
    "$HIT_EXPLOSION",
    "$SPARKS",
    "$ARTILLERY_EXPLOSION",
    "$SHOCK_RING",
    "$NAPALM",
    "$AIRBURST",
    "$GROUNDBURST",
    "$GROUND_STRIKE_NOFIRE",
    "$LONG_HANGING_SMOKE",
    "$SMOKETRAIL",
    "$DEBRISTRAIL",
    "$HIT_EXPLOSION_DEBRISTRAIL",
    "$RISING_GROUNDHIT_EXPLOSION_DEBR",
    "$FIRETRAIL",
    "$FIRE_NOSMOKE",
    "$LIGHT_CLOUD",
    "$WATER_CLOUD",
    "$WATERTRAIL",
    "$GUN_TRACER",
    "$DARK_DEBRIS",
    "$FIRE_DEBRIS",
    "$LIGHT_DEBRIS",
    "$SPARKS_NO_DEBRIS",
    "$BURNING_PART",
    "$AIR_EXPLOSION_NOGLOW",
    "$HIT_EXPLOSION_NOGLOW",
    "$SHOCK_RING_SMALL",
    "$FAST_FADING_SMOKE",
    "$LONG_HANGING_SMOKE2",
    "$SMALL_AIR_EXPLOSION",
    "$FLAME",
    "$AIR_PENETRATION",
    "$GROUND_PENETRATION",
    "$DEBRISTRAIL_DUST",
    "$FIRE_EXPAND",
    "$FIRE_EXPAND_NOSMOKE",
    "$GROUND_DUSTCLOUD",
    "$SHAPED_FIRE_DEBRIS",
    "$FIRE_HOT",
    "$FIRE_MED",
    "$FIRE_COOL",
    "$FIREBALL",
    "$FIRE1",
    "$FIRE2",
    "$FIRE3",
    "$FIRE4",
    "$FIRE5",
    "$FIRE6",
    "$FIRESMOKE",
    "$TRAILSMOKE",
    "$TRAILDUST",
    "$FIRE7",
    "$BLUE_CLOUD",
    "$WATER_FIREBALL",
    "$LINKED_PERSISTANT",
    "$TIMED_PERSISTANT",
    "$CLUSTER_BOMB",
    "$SMOKING_FEATURE",
    "$STEAMING_FEATURE",
    "$STEAM_CLOUD",
    "$GROUND_FLASH",
    "$GROUND_GLOW",
    "$MESSAGE_TIMER",
    "$DURANDAL",
    "$CRATER2",
    "$CRATER3",
    "$CRATER4",
    "$BIG_SMOKE",
    "$BIG_DUST",
    "$HIT_EXPLOSION_NOSMOKE",
    "$ROCKET_BURST",
    "$CAMP_HIT_EXPLOSION_DEBRISTRAIL",
    "$CAMP_FIRE",
    "$INCENDIARY_EXPLOSION",
    "$SPARK_TRACER",
    "$WATER_WAKE",
	"--- kludge ---",
	"$GUN_HIT_GROUND", 
	"$GUN_HIT_OBJECT",     
	"$GUN_HIT_WATER",
	"$VEHICLE_DIEING",
	"$VEHICLE_TAIL_SCRAPE",
	"$VEHICLE_DIE_SMOKE",
	"$VEHICLE_DIE_SMOKE2",
	"$VEHICLE_DIE_FIREBALL",
	"$VEHICLE_DIE_FIREBALL2",
	"$VEHICLE_DIE_FIRE",
	"$VEHICLE_DIE_INTERMITTENT_FIRE",
	"$GUN_SMOKE", //TJL
  "$NUKE",//TJL
};

int		DrawableParticleSys::nameListCount = sizeof(DrawableParticleSys::nameList)/sizeof(char *);
AList	DrawableParticleSys::textureList;
ProtectedAList	DrawableParticleSys::paramList;
AList	DrawableParticleSys::dpsList;
float	DrawableParticleSys::groundLevel;
float	DrawableParticleSys::cameraDistance;
int		DrawableParticleSys::reloadParameters=0;
float	DrawableParticleSys::winddx;
float	DrawableParticleSys::winddy;


/**** Macros ****/
#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))
#define NRESCALE(in,outmin,outmax)			  RESCALE(in,0,1,outmin,outmax)

/****** Basic Data Structs ******/
struct psRGBA
{
	float   r,g,b,a;
};

struct timedRGBA
{
	float   time;
	psRGBA  value;
};

struct psRGB
{
	float   r,g,b;
};


/****** Timed data structs ******/
struct timedRGB
{
	float   time;
	psRGB  value;
};

struct timedFloat
{
	float   time;
	float   value;
};

/****** Texture tracking ******/

class ParticleTextureNode : public ANode
{
public:
	char filename[64];
	Texture texture;
};

/****** Some ENUMS ******/

typedef enum 
{ 
	PSEM_ONCE = 0,  
	PSEM_PERSEC = 1,
	PSEM_IMPACT = 2,
	PSEM_EARTHIMPACT = 3,
	PSEM_WATERIMPACT = 4
} PSEmitterModeEnum;

typedef enum 
{ 
	PSD_SPHERE=0, 
	PSD_PLANE=1,  
	PSD_BOX=2,    
	PSD_BLOB=3,   
	PSD_CYLINDER=4, 
	PSD_CONE=5,
	PSD_TRIANGLE=6,
	PSD_RECTANGLE=7,
	PSD_DISC=8,
	PSD_LINE=9,
	PSD_POINT=10 
}  PSDomainEnum;

class ParticleDomain
{
public:
	PSDomainEnum type;
	void GetRandomDirection(Tpoint *p);
	void GetRandomPosition(Tpoint *p);

	//JAM 19Apr04 - Need to change these TPoints (Tpoint) to Points (defined above).
	union
	{
		float		 param[9];

		struct
		{
			Point pos, 
				   size;
		} sphere;

		struct
		{
			float a,b,c,d;
		} plane;

		struct
		{
			Point cornerA,
				   cornerB;
		} box;

		struct
		{
			Point pos, size;
			float  deviation;
		} blob;

	};

	void Parse(void);
};

typedef enum 
{
	PSDT_NONE  = 0,
	PSDT_POLY  = 1,
	PSDT_POINT = 2,
	PSDT_LINE  = 3
} PSDrawType;

typedef enum
{
	PSO_NONE      = 0,
	PSO_MOVEMENT  = 1
} PSOrientation;

/****** Particle Parameter structure ******/
#define PS_NAMESIZE 64

struct ParticleEmitterParam
{
	PSEmitterModeEnum  mode;
	ParticleDomain	   domain;
	ParticleDomain     target;
	float		   param[9];
	int            id;
	char           name[PS_NAMESIZE];
	int            stages;
	timedFloat     rate[10];
	float          velocity, velVariation;
};

class ParticleParamNode : public ANode
{
public:
	char    name[PS_NAMESIZE];
	int		id;
	int     subid;

	int     particleType; // which paritcle class to use for particles????

	float   lifespan, lifespanvariation; // how long a particle lasts in seconds
	int     flags;
	float   lodFactor;

	PSDrawType drawType;

	int       colorStages;
	timedRGB  color[10];

	int       lightStages;
	timedRGB  light[10];

	int			sizeStages;
	timedFloat	size[10];

	int			alphaStages;
	timedFloat	alpha[10];


	int			gravityStages;
	timedFloat  gravity[10]; // 0 floats, negative rises, positive sinks

	int         accelStages;
	timedFloat  accel[10];

	int sndId;
	int sndLooped;

	int sndPitchStages;
	timedFloat  sndPitch[10];

	int sndVolStages;
	timedFloat  sndVol[10];

	int trailId;

	float velInitial, 
		  velVariation;
	float velInherit; // inherit from emitter

	float groundFriction;

	float visibleDistance;

	float simpleDrag;

	int emitOneRandomly;

//#define PSMAX_EMITTERS 5  // Cobra - only in CO
#define PSMAX_EMITTERS 10
	ParticleEmitterParam emitter[PSMAX_EMITTERS];

	float   bounce;
	float   dieOnGround;

	char     texFilename[64];
	Texture *texture;

	// BSP particle data
	int bspCTID, bspVisType; // CT Number of BSP
	DrawableBSP *bspObj; // All particles can share 1 BSP.

	PSOrientation orientation;
};

/** ParticleSys Flags **/
#define PSF_CHARACTERS "MG"
#define PSF_NONE	     (0) // use spacing value a a time (in seconds) instead of distance
#define PSF_MORPHATTRIBS (1<<0) // blend attributes over lifespan)
#define PSF_GROUNDTEST   (1<<1) // enable terrain surface level check

ParticleParamNode **PPN = 0;
int PPNCount = 0;







/***********************************************************************
  
	The ParticleNode
	
***********************************************************************/

class ParticleNode : public ANode
{
public:
	ParticleNode(int ID, Tpoint *Pos, Tpoint *Vel=0, Tpoint *Aim=0);
	~ParticleNode();

	void Draw(class RenderOTW *renderer, int LOD);
	int IsDead(void);
	void Init(int ID, Tpoint *Pos, Tpoint *Vel=0, Tpoint *Aim=0);

	void EvalTimedFloat( int Count, timedFloat *input, float  &retval);
	void EvalTimedRGBA ( int Count, timedRGBA  *input, psRGBA &retval);
	void EvalTimedRGB  ( int Count, timedRGB   *input, psRGB  &retval);
public:
	ParticleParamNode *ppn;

	int    birthTime; //
	int    lastTime;  // last time we were updated
	float  lifespan;  // in seconds
	float  life;      // normalized

	static float      elapsedTime;
	static Trotation *rotation; // runtime computed, shared
	float  plife;     // life value for the previous exec

	Tpoint pos, vel;

	class SubPart *firstSubPart;
};

Trotation psIRotation = IMatrix;

float      ParticleNode::elapsedTime = 0;
Trotation *ParticleNode::rotation    = &psIRotation;
/***********************************************************************
  
	SubParticle Data
	
***********************************************************************/

class SubPart
{
public:
	SubPart(ParticleNode *owner) { next = 0;};
	SubPart() { next = 0;};
	virtual ~SubPart() {}
	SubPart *next;
	virtual void Run(RenderOTW *renderer, ParticleNode *owner) = 0;
	virtual int IsRunning(ParticleNode *owner) { return 0; }
};

#define SPRMODE_VEC 1

class SubPartMovementOrientation : public SubPart
{
public:
	SubPartMovementOrientation(ParticleNode *owner):SubPart(owner) { rotation = IMatrix; }; 
	Trotation rotation;
	virtual void Run(RenderOTW *renderer, ParticleNode *owner);
};


class SubPartPoly : public SubPart
{
public:
	SubPartPoly(ParticleNode *owner):SubPart(owner) { next = 0;};
	virtual void Run(RenderOTW *renderer, ParticleNode *owner);
};

class SubPartTrail : public SubPart
{
public:
	SubPartTrail(ParticleNode *owner);
	virtual ~SubPartTrail();

	DrawableTrail *trailObj;
	virtual void   Run(RenderOTW *renderer, ParticleNode *owner);
	virtual int    IsRunning(ParticleNode *owner);
};

class SubPartBSP : public SubPart
{
public:
	SubPartBSP(ParticleNode *owner);
	virtual ~SubPartBSP();
	Trotation    rotation;
	virtual void Run(RenderOTW *renderer, ParticleNode *owner);
};

class SubPartSound : public SubPart
{
public:
	SubPartSound(ParticleNode *owner);
	virtual ~SubPartSound();
	int        playSound;
	F4SoundPos soundPos;
	virtual void Run(RenderOTW *renderer, ParticleNode *owner);
	virtual int IsRunning(ParticleNode *owner);
};


// never attach this to the particle node
class SubEmitter : public SubPart
{
public:
	SubEmitter(ParticleNode *owner, ParticleEmitterParam *ed);
	virtual ~SubEmitter();
	struct ParticleEmitterParam *pep;
	float  rollover;
	virtual void Run(RenderOTW *renderer, ParticleNode *owner);
};

class SubPartEmitter : public SubPart
{
public:
	SubPartEmitter(ParticleNode *owner);
	virtual ~SubPartEmitter();
	virtual void Run(RenderOTW *renderer, ParticleNode *owner);
	SubPart *emitters;
	int count;
};


/*....................................................................*/

void SubPartMovementOrientation::Run(RenderOTW *renderer, ParticleNode *owner)
{
	owner->rotation = &rotation;

	float costha,sintha,cosphi,sinphi,cospsi,sinpsi;
	float p,r,y;

	y = atan2(owner->vel.y,owner->vel.x);
	r = 0;
	p = atan2(sqrt(owner->vel.x*owner->vel.x+owner->vel.y*owner->vel.y),owner->vel.z) - 90 * DTR;

	costha = cosf(p);
	sintha = sinf(p);
	cosphi = cosf(r);
	sinphi = sinf(r);
	cospsi = cosf(y);
	sinpsi = sinf(y);

	rotation.M11 = cospsi*costha;
	rotation.M21 = sinpsi*costha;
	rotation.M31 = -sintha;

	rotation.M12 = -sinpsi*cosphi+cospsi*sintha*sinphi;
	rotation.M22 = cospsi*cosphi+sinpsi*sintha*sinphi;
	rotation.M32 = costha*sinphi;

	rotation.M13 = sinpsi*sinphi+cospsi*sintha*cosphi;
	rotation.M23 = -cospsi*sinphi+sinpsi*sintha*cosphi;
	rotation.M33 = costha*cosphi;
}

/*....................................................................*/

SubPartTrail::SubPartTrail(ParticleNode *owner) : SubPart(owner)
{
	if(owner->ppn->trailId && owner->elapsedTime)
	{
		trailObj = new DrawableTrail(owner->ppn->trailId, 1);
	}
}

SubPartTrail::~SubPartTrail()
{
	delete trailObj;
}

void SubPartTrail::Run(RenderOTW *renderer, ParticleNode *owner)
{
	trailObj->AddPointAtHead(&owner->pos,0);
	trailObj->Draw(renderer, 1);
}

int SubPartTrail::IsRunning(ParticleNode *owner)
{
	return !trailObj->IsTrailEmpty();
}

/*....................................................................*/

SubPartSound::SubPartSound(ParticleNode *owner) : SubPart(owner)
{
	playSound = 1;
}

SubPartSound::~SubPartSound()
{
}

void SubPartSound::Run(RenderOTW *renderer, ParticleNode *owner)
{
	if(playSound && owner->elapsedTime)
	{
		float sndVol, sndPitch;
		owner->EvalTimedFloat( owner->ppn->sndVolStages,   owner->ppn->sndVol,   sndVol );
		owner->EvalTimedFloat( owner->ppn->sndPitchStages, owner->ppn->sndPitch, sndPitch );

		soundPos.UpdatePos(owner->pos.x, owner->pos.y, owner->pos.z, owner->vel.x, owner->vel.y, owner->vel.z);
		soundPos.Sfx(owner->ppn->sndId,0,sndPitch,sndVol);
		if(!owner->ppn->sndLooped)
			playSound=0; // so we only play it once
	}
}

int SubPartSound::IsRunning(ParticleNode *owner)
{
	return soundPos.IsPlaying(owner->ppn->sndId,0);
}

/*....................................................................*/

void SubPartPoly::Run(RenderOTW *renderer, ParticleNode *owner)
{
	if(DrawableParticleSys::cameraDistance>owner->ppn->visibleDistance)
		return;

	float size, alpha;
	psRGB color,light;

	// these must be computed even while paused, the renderer needs them
	owner->EvalTimedFloat( owner->ppn->sizeStages,  owner->ppn->size,  size );
	owner->EvalTimedFloat( owner->ppn->alphaStages, owner->ppn->alpha, alpha );
	owner->EvalTimedRGB  ( owner->ppn->colorStages, owner->ppn->color, color);
	owner->EvalTimedRGB  ( owner->ppn->lightStages, owner->ppn->light, light);

	Tpoint os,pv;
	ThreeDVertex v0,v1,v2,v3;

		renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
	if(owner->ppn->texture)
		renderer->context.SelectTexture1(owner->ppn->texture->TexHandle());
	
	renderer->TransformPointToView(&owner->pos,&pv);
	
	os.x =  0.f;
	os.y = -size;
	os.z = -size;
	renderer->TransformBillboardPoint(&os,&pv,&v0);
	
	os.x =  0.f;
	os.y =  size;
	os.z = -size;
	renderer->TransformBillboardPoint(&os,&pv,&v1);
	
	os.x =  0.f;
	os.y =  size;
	os.z =  size;
	renderer->TransformBillboardPoint(&os,&pv,&v2);
	
	os.x =  0.f;
	os.y = -size;
	os.z =  size;
	renderer->TransformBillboardPoint(&os,&pv,&v3);
	
	v0.u = TEX_UV_MIN, v0.v = TEX_UV_MIN, v0.q = v0.csZ * Q_SCALE;
	v1.u = TEX_UV_MAX, v1.v = TEX_UV_MIN, v1.q = v1.csZ * Q_SCALE;
	v2.u = TEX_UV_MAX, v2.v = TEX_UV_MAX, v2.q = v2.csZ * Q_SCALE;
	v3.u = TEX_UV_MIN, v3.v = TEX_UV_MAX, v3.q = v3.csZ * Q_SCALE;
	
	if(gParticleSysGreenMode)
	{	
		v0.r = v1.r = v2.r = v3.r = 0.f;
		v0.g = v1.g = v2.g = v3.g = .4f;
		v0.b = v1.b = v2.b = v3.b = 0.f;
	}
	else
	{	
		v0.r = v1.r = gParticleSysLitColor.r * color.r + light.r;
		v2.r = v3.r = gParticleSysLitColor.r * color.r * .68f + light.r;
		
		v0.g = v1.g = gParticleSysLitColor.g * color.g + light.g;
		v2.g = v3.g = gParticleSysLitColor.g * color.g * .68f + light.g;
		
		v0.b = v1.b = gParticleSysLitColor.b * color.b + light.b;
		v2.b = v3.b = gParticleSysLitColor.b * color.b * .68f + light.b;
	}
	
	v0.a = alpha;
	v1.a = alpha; 
	v2.a = alpha;
	v3.a = alpha;
	
	renderer->DrawSquare(&v0,&v1,&v2,&v3,CULL_ALLOW_ALL,(g_nGfxFix > 0));
}

/*....................................................................*/

SubPartBSP::SubPartBSP(ParticleNode *owner) : SubPart(owner)
{
}

SubPartBSP::~SubPartBSP()
{
}

void SubPartBSP::Run(RenderOTW *renderer, ParticleNode *owner)
{
	owner->ppn->bspObj->orientation = *(owner->rotation);
	owner->ppn->bspObj->SetPosition(&owner->pos);
	owner->ppn->bspObj->Draw(renderer);
}
/*....................................................................*/

SubPartEmitter::SubPartEmitter(ParticleNode *owner) : SubPart(owner)
{
	emitters=0;
	count=0;

	if(owner->ppn->emitOneRandomly)
	{
		for(count = 0; owner->ppn->emitter[count].stages && count<PSMAX_EMITTERS; count++);

		if(count)
		{
			int l = rand() % count;
			SubPart *sub = new SubEmitter(owner, &owner->ppn->emitter[l]);
			sub->next = emitters;
			emitters = sub;
			count = 1;
		}
	}
	else
	{
		for(count = 0; owner->ppn->emitter[count].stages && count<PSMAX_EMITTERS; count++)
		{
			SubPart *sub = new SubEmitter(owner, &owner->ppn->emitter[count]);
			sub->next = emitters;
			emitters = sub;
		}
	}

}

SubPartEmitter::~SubPartEmitter()
{
	SubPart *emit;
	while(emit = emitters)
	{
		emitters = emit->next;
		delete emit;
	}
}

void SubPartEmitter::Run(RenderOTW *renderer, ParticleNode *owner)
{
	if(owner->elapsedTime)
	{
		SubPart *emit = emitters;
		while(emit)
		{
			emit->Run(renderer, owner);
			emit = emit->next;
		}
	}
}


SubEmitter::SubEmitter(ParticleNode *owner, ParticleEmitterParam *EP) : SubPart(owner)
{
	rollover = 0;
	pep      = EP;
}

SubEmitter::~SubEmitter()
{
}

void SubEmitter::Run(RenderOTW *renderer, ParticleNode *owner)
{
	Tpoint epos = owner->pos;
	float qty=0;
	switch(pep->mode)
	{
	case PSEM_IMPACT:
		if( owner->pos.z >= DrawableParticleSys::groundLevel )
		{
			epos.z = DrawableParticleSys::groundLevel;
			qty+=pep->rate[0].value;
		}		
		break;
	case PSEM_EARTHIMPACT:
		if( owner->pos.z >= DrawableParticleSys::groundLevel )
		{
			int gtype = OTWDriver.GetGroundType(owner->pos.x,owner->pos.y);
			epos.z = DrawableParticleSys::groundLevel;


			if(!( gtype == COVERAGE_WATER || gtype == COVERAGE_RIVER ))
				qty+=pep->rate[0].value;
		}
		break;

	case PSEM_WATERIMPACT:
		if( owner->pos.z >= DrawableParticleSys::groundLevel )
		{
			int gtype = OTWDriver.GetGroundType(owner->pos.x,owner->pos.y);
			epos.z = DrawableParticleSys::groundLevel;


			if((  gtype == COVERAGE_WATER || gtype == COVERAGE_RIVER ))
				qty+=pep->rate[0].value;
		}
		break;
	case PSEM_ONCE:
		{
			int t;

			for(t=0;t<pep->stages;t++)
			{
				if( pep->rate[t].time < owner->life && 
					pep->rate[t].time >= owner->plife)
				{  // add qty if we've aged past a stage
					qty+=pep->rate[0].value;
				}
			}

		}
		break;
	case PSEM_PERSEC:
		owner->EvalTimedFloat(pep->stages,pep->rate,qty);
		qty = qty * owner->elapsedTime + rollover;
	}

	while(qty >= 1)
	{
		float v;
		Tpoint w, pos, aim, subvel;

		//Cobra TJL 10/30/04
		//v=pep->velocity + pep->velVariation * NRAND;

		pep->domain.GetRandomPosition(&w);
		MatrixMult(owner->rotation, &w, &pos);

		pep->target.GetRandomDirection(&w);
		MatrixMult(owner->rotation, &w, &aim);

		v=pep->velocity + pep->velVariation * NRAND;

		subvel.x = aim.x*v;
		subvel.y = aim.y*v;
		subvel.z = aim.z*v;

		subvel.x += owner->vel.x;
		subvel.y += owner->vel.y;
		subvel.z += owner->vel.z;

		pos.x+=epos.x;
		pos.y+=epos.y;
		pos.z+=epos.z;

		ParticleNode *n=new ParticleNode(pep->id,&pos,&subvel,&aim);
		n->InsertAfter((ANode *)owner);
		qty-=1.0f;
	}
	rollover=qty;
}

/*....................................................................*/



/**** some useful crap ****/
#define MORPH(n,src,dif) ( (src) + ( (dif) * (n) ) )
#define ATTRMORPH(ATR) attrib.ATR = MORPH(life, params->birthAttrib.ATR, params->deathAttribDiff.ATR)
/**** end of useful crap ****/

ParticleNode::ParticleNode(int ID, Tpoint *Pos, Tpoint *Vel, Tpoint *Aim)
{
	Init(ID,Pos,Vel,Aim);
}


void ParticleNode::Init(int ID, Tpoint *Pos, Tpoint *Vel, Tpoint *Aim)
{
	lastTime = TheTimeManager.GetClockTime();
	firstSubPart = 0;
	
	ppn=PPN[ID];

	pos=*Pos;
	if(Vel)
	{
		vel.x=Vel->x * ppn->velInherit;
		vel.y=Vel->y * ppn->velInherit;
		vel.z=Vel->z * ppn->velInherit;
	}
	else
		vel.x=vel.y=vel.z=0;

	if(Aim)
	{
		float v;

		v=ppn->velInitial + ppn->velVariation * NRAND;
		vel.x+=Aim->x*v;
		vel.y+=Aim->y*v;
		vel.z+=Aim->z*v;

	}
	plife=0;

	// note, subparts are execute in the opposite order as they are attached
	if(ppn->drawType==PSDT_POLY)
	{
		SubPart *sub = new SubPartPoly(this);
		sub->next = firstSubPart;
		firstSubPart = sub;
	}

	if(ppn->bspObj)
	{
		SubPart *sub = new SubPartBSP(this);
		sub->next = firstSubPart;
		firstSubPart = sub;
	}

	if(ppn->sndId)
	{
		SubPart *sub = new SubPartSound(this);
		sub->next = firstSubPart;
		firstSubPart = sub;
	}

	if(ppn->trailId>=0)
	{
		SubPart *sub = new SubPartTrail(this);
		sub->next = firstSubPart;
		firstSubPart = sub;
	}

	if(ppn->emitter[0].stages) // we have atleast 1 emitter
	{
		SubPart *sub = new SubPartEmitter(this);
		sub->next = firstSubPart;
		firstSubPart = sub;
	}
	
	/*int l;
	for(l = 0; ppn->emitter[l].stages && l<PSMAX_EMITTERS; l++)
	{
		SubPart *sub = new SubPartEmitter(this);
		sub->next = firstSubPart;
		firstSubPart = sub;
	}*/
	

	switch(ppn->orientation)
	{
	case PSO_MOVEMENT:
		{
			SubPart *sub = new SubPartMovementOrientation(this);
			sub->next = firstSubPart;
			firstSubPart = sub;
		}
		break;
	}


	birthTime = TheTimeManager.GetClockTime();
	lifespan  = ppn->lifespan + ppn->lifespanvariation * NRAND;
}

#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))

void ParticleNode::EvalTimedFloat(int Count, timedFloat *input, float &retval)
{
	if(life < input[0].time || Count < 2)
	{
		retval=input[0].value;
		return;
	}
	
	int l, l1;
	for(l = 0 ; l < (Count - 1) ; l++)
	{
		l1 = l + 1;
		if(life < input[l1].time)
		{
			retval = RESCALE(life, input[l].time, input[l1].time, input[l].value, input[l1].value);
			return;
		}
	}
	// assume we fell thru --- l=Count-1;
	retval=input[l].value;
}

#define MRESCALE(member) retval.member = RESCALE(life, input[l].time, input[l1].time, input[l].value.member, input[l1].value.member)

void ParticleNode::EvalTimedRGBA(int Count, timedRGBA *input, psRGBA &retval)
{
	if(life < input[0].time || Count < 2)
	{
		retval.r=input[0].value.r;
		retval.g=input[0].value.g;
		retval.b=input[0].value.b;
		retval.a=input[0].value.a;
		return;
	}
	
	int l, l1;
	for(l = 0 ; l < (Count - 1) ; l++)
	{
		l1 = l + 1;
		if(life < input[l1].time)
		{
			MRESCALE(r);
			MRESCALE(g);
			MRESCALE(b);
			MRESCALE(a);
			return;
		}
	}
	// assume we fell thru --- l=Count-1;
	retval.r=input[l].value.r;
	retval.g=input[l].value.g;
	retval.b=input[l].value.b;
	retval.a=input[l].value.a;
}

void ParticleNode::EvalTimedRGB(int Count, timedRGB *input, psRGB &retval)
{
	if(life < input[0].time || Count < 2)
	{
		retval.r=input[0].value.r;
		retval.g=input[0].value.g;
		retval.b=input[0].value.b;
		return;
	}
	
	int l, l1;
	for(l = 0 ; l < (Count - 1) ; l++)
	{
		l1 = l + 1;
		if(life < input[l1].time)
		{
			MRESCALE(r);
			MRESCALE(g);
			MRESCALE(b);
			return;
		}
	}
	// assume we fell thru --- l=Count-1;
	retval.r=input[l].value.r;
	retval.g=input[l].value.g;
	retval.b=input[l].value.b;
}

ParticleNode::~ParticleNode()
{
	SubPart *sub = firstSubPart;
	while(sub)
	{
		SubPart *next = sub->next;
		delete sub;
		sub = next;
	}
}

int ParticleNode::IsDead(void) 
{
	if(life > 1.0 || life < 0.0)
	{	
		SubPart *sub = firstSubPart;
		while(sub)
		{
			if(sub->IsRunning(this))
				return 0;
			sub = sub->next;
		}
		return 1;
	}
	return 0;
}

void ParticleNode::Draw(class RenderOTW *renderer, int LOD)
{
	if(lifespan<=0) return;

	float age, curTime;

	curTime     = TheTimeManager.GetClockTime();
	age         = (curTime - (float)birthTime) * .001f;
	elapsedTime = (curTime - (float)lastTime ) * .001f;
	life        = age / lifespan;

	if(life>1.0f && plife<1.0f)
	{
		life=1.0f; // just to make sure we do the last stages
	}
/*	else			// COBRA - RED - If returns before die, will not append new emitters or nodes
	{
		if(life>1.0f) // waiting to die
			return;
	}
*/					// COBRA - RED - End

	if(elapsedTime)
	{   // we only need to run this if some time has elapsed
		float gravity, accel;
		lastTime = curTime;

		EvalTimedFloat( ppn->gravityStages, ppn->gravity, gravity );
		EvalTimedFloat( ppn->accelStages, ppn->accel, accel );


		if(ppn->simpleDrag)
		{	float Drag_x_Time=ppn->simpleDrag * elapsedTime;		// COBRA - RED - Cached same Value

			vel.x-=DrawableParticleSys::winddx;
			vel.y-=DrawableParticleSys::winddy;
			vel.x-=vel.x*Drag_x_Time;
			vel.y-=vel.y*Drag_x_Time;
			vel.z-=vel.z*Drag_x_Time;
			vel.x+=DrawableParticleSys::winddx;
			vel.y+=DrawableParticleSys::winddy;
		}

		// update velocity values
		vel.z+=(32 * elapsedTime) * gravity;
		
		if(accel)
		{
			float fps = (accel * elapsedTime);
			float d=sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
			fps= d + fps;
			if(fps<0) 
				fps=0;
			if(d)
			{
				vel.x = (vel.x / d) * fps;
				vel.y = (vel.y / d) * fps;
				vel.z = (vel.z / d) * fps;
			}
		}

				
		pos.x+=vel.x * elapsedTime;
		pos.y+=vel.y * elapsedTime;
		pos.z+=vel.z * elapsedTime;



		DrawableParticleSys::groundLevel = OTWDriver.GetGroundLevel(pos.x, pos.y);

		if(pos.z >= DrawableParticleSys::groundLevel)
		{
			pos.z=DrawableParticleSys::groundLevel;
			vel.z *= -ppn->bounce;

			if(ppn->dieOnGround && lifespan > age) // this will make the particle die
			{                    // will also run the emitters

				lifespan=age;
			}
		}

		
		if(pos.z == DrawableParticleSys::groundLevel)
		{
			float fps = (ppn->groundFriction * elapsedTime);
			float d=sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
			fps= d + fps;
			if(fps<0) 
				fps=0;
			if(d)
			{
				vel.x = (vel.x / d) * fps;
				vel.y = (vel.y / d) * fps;
				//vel.z += (vel.z / d) * fps;
			}	
		}		
		// end of the "Exec" section
	}

	rotation = &psIRotation;

	SubPart *sub = firstSubPart;
	while(sub)
	{
		sub->Run(renderer, this);
		sub = sub->next;
	}

	plife=life;

}




static	Tcolor	gLight;

DrawableParticleSys::DrawableParticleSys( int particlesysType, float scale )
: DrawableObject( scale )
{
	ShiAssert( particlesysType >= 0 );

	// Store the particlesys type
	type=particlesysType;
	dpsNode.owner=this;
	paramList.Lock();
	dpsList.AddHead(&dpsNode);
	paramList.Unlock();
	
	// Set to position 0.0, 0.0, 0.0;
	position.x = 0.0F;
	position.y = 0.0F;
	position.z = 0.0F;

	headFPS.x=0.0f;
	headFPS.y=0.0f;
	headFPS.z=0.0f;

	Something=0;
}

DrawableParticleSys::~DrawableParticleSys( void )
{
	// Delete this object's particlesys
	paramList.Lock();
	dpsNode.Remove();

	ParticleNode *n;
	while(n=(ParticleNode *)particleList.RemHead())
	{
		delete n;
	}
	paramList.Unlock();

}

int  DrawableParticleSys::HasParticles(void)
{
	if(particleList.GetHead())
		return 1;
	return 0;
}



/***************************************************************************\
    Add a point to the list which define this segmented particlesys.
\***************************************************************************/
void DrawableParticleSys::AddParticle( Tpoint *worldPos, Tpoint *v)
{
	AddParticle(type,worldPos,v);
}


void DrawableParticleSys::AddParticle( int Id, Tpoint *worldPos, Tpoint *v )
{
	DWORD now;
	now = TheTimeManager.GetClockTime();
	
// COBRA - RED - New FPS Filter
	// Cobra - Keep the Particle list cleaned up
/*	if ((now >= TimeToPurge)&&PurgeTimeInc)
	{
		TimeToPurge = (now + PurgeTimeInc);
		CleanParticleList();
	}
	// Cobra - Purge the Particle list
	else if ((now >= TimeToPurgeAll)&&PurgeTimeInc)
	{
		TimeToPurgeAll = (now + PurgeAllTimeInc);
		ClearParticleList();
	}
	//cobra Bail out of FPS is lower than 10
	float fpsTest = OTWDriver.GetFPS();
 	if (fpsTest < g_nPSKillFPS)
		{
		return;
		}*/

	// COBRA - RED - The Particle Filetr starts here
	// The Filter works on a counter Basis...The counter rolls off each MAX_PARTICLE_FILTER_LEVEL counts
	// Partices are Added only if the actual counter level is beyond a Filter Level which is calculated
	// based on Actual Fps and a lower FPS limit. It works almost like a PID with a proportional Band 
	// of 50%, at fps less than 50% over the limit it start filtering Particles ( not adding them ),
	// the more near the limit the more are filtered...when at limit or ps under limit, no particles 
	// are more added.

	float fpsTest = OTWDriver.GetFPS();				// Get the Actal FPS	
	
	// Calculates the filter level ... the more near fps limit, the lower the filter level
	float FilterLevel=(fpsTest-g_nPSKillFPS)*(MAX_PARTICLE_FILTER_LEVEL/(g_nPSKillFPS*1.5f-g_nPSKillFPS+1.0f));
	
	// if less that 0 ( already under fps limit) then settle at 0 ( no New particle )
	if(FilterLevel<0) FilterLevel=0;
	
	// here the counter of the Filter is updated, the filter works Adding 'Filter' Particles out of the Counter
	ParticleFilterCount=(++ParticleFilterCount)%(MAX_PARTICLE_FILTER_LEVEL+1);

	// Ok, now we have how much is counting the counter, if its counting not more that the Filter Level 
	// Add this particle, if not bail out
	if(ParticleFilterCount<(WORD)FilterLevel){
		ParticleNode *n = new ParticleNode(Id, worldPos, v);
		particleList.AddHead(n);
	}
	REPORT_VALUE("P.Filter%",(FilterLevel>=MAX_PARTICLE_FILTER_LEVEL)?0:((MAX_PARTICLE_FILTER_LEVEL-FilterLevel)/MAX_PARTICLE_FILTER_LEVEL*100));
}

void DrawableParticleSys::SetHeadVelocity(Tpoint *FPS)
{
	headFPS = *FPS;
}


inline DWORD ROL(DWORD n)
{
	_asm
	{
		rol n,1;
	}
	return n;
}

void DrawableParticleSys::Draw( class RenderOTW *renderer, int LOD)
{
#ifdef Prof_ENABLED // MLR 5/21/2004 - 
	Prof(DrawableParticleSys_Draw);
#endif

	COUNT_PROFILE("Particle");

	ParticleNode *n;
	float xx,yy,zz;

	paramList.Lock();

	n=(ParticleNode *)particleList.GetHead();

	if(n)
	{
		groundLevel=OTWDriver.GetGroundLevel(n->pos.x,n->pos.y);
		mlTrig trigWind;
		float wind;

		// current wind
//		mlSinCos(&trigWind, TheWeather->GetWindHeading(&n->pos));
//		wind =  TheWeather->GetWindSpeedFPS(&n->pos);
		mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&n->pos));
		wind =  ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&n->pos);
		winddx = trigWind.cos * wind;
		winddy = trigWind.sin * wind;



		xx=n->pos.x-renderer->X();
		yy=n->pos.y-renderer->Y();
		zz=n->pos.z-renderer->Z();

		cameraDistance=sqrt(xx*xx+yy*yy+zz*zz);

		while(n)
		{
			n->Draw(renderer,LOD);
			// we have to get the next node after Exec, because our emitters
			// will add nodes directly after themselves.  Getting the next node
			// pointer before Exec will cause one newly emitted node to be skipped
			// in this loop, which then causes rendering bugs due to not having 
			// data that was Exec'd yet.
			ParticleNode *n2 = (ParticleNode *)n->GetSucc();
// Cobra - Moved the node killer to start of mission (otwloop.cpp) 
//					also in DrawableParticleSys::AddParticle()
			if((n->IsDead()) /*&& !PurgeTimeInc*/)						// COBRA - RED - If no1 updates PurgeTimeInc, 
			{														// when it could be 0...???
				n->Remove();
				delete n;
			}
			n=n2;
		}
	}
	paramList.Unlock();
}


void DrawableParticleSys::Exec( void)
{
	return;
}

void DrawableParticleSys::ClearParticles(void)
{
	ParticleNode *n;

	while(n=(ParticleNode *)particleList.RemHead())
	{
		delete n;
	}

}

void DrawableParticleSys::ClearParticleList(void)
{
	DPS_Node *dps;

	paramList.Lock();
	dps=(DPS_Node *)dpsList.GetHead();
	while(dps)
	{
		dps->owner->ClearParticles();
		dps=(DPS_Node *)dps->GetSucc();
	}
	paramList.Unlock();
}

void DrawableParticleSys::CleanParticleList(void)
{
	DPS_Node *dps;

	paramList.Lock();
	dps=(DPS_Node *)dpsList.GetHead();
	while(dps)
	{
		dps->owner->CleanParticles();
		dps=(DPS_Node *)dps->GetSucc();
	}
	paramList.Unlock();
}

void DrawableParticleSys::CleanParticles(void)
{
	ParticleNode *n;

	n=(ParticleNode *)particleList.GetHead();
	while(n)
	{
		ParticleNode *n2 = (ParticleNode *)n->GetSucc();
			if(n->IsDead())
			{
				n->Remove();
				delete n;
			}
			n=n2;
	}
}

void DrawableParticleSys::SetGreenMode(BOOL state)
{
	gParticleSysGreenMode = state;
}

void DrawableParticleSys::SetCloudColor(Tcolor *color)
{
	memcpy(&gParticleSysLitColor,color,sizeof(Tcolor));
}

ParticleTextureNode *DrawableParticleSys::FindTextureNode(char *fn)
{
	ParticleTextureNode *n;

	n=(ParticleTextureNode *)textureList.GetHead();
	while(n)
	{
		if(stricmp(n->filename,fn)==0)
		{
			return(n);
		}
		n=(ParticleTextureNode *)n->GetSucc();
	}
	return 0;
}

ParticleTextureNode *DrawableParticleSys::GetTextureNode(char *fn)
{
	ParticleTextureNode *n;

	n=FindTextureNode(fn);
	if(n) return n;

	n=new ParticleTextureNode;
	strcpy(n->filename,fn);
	textureList.AddHead(n);
	return n;
}

DXContext *psContext = 0;

void DrawableParticleSys::SetupTexturesOnDevice(DXContext *rc)
{
	psContext = rc;

	ParticleTextureNode *n;

	n=(ParticleTextureNode *)textureList.GetHead();
	while(n)
	{
		n->texture.LoadAndCreate(n->filename,MPR_TI_DDS);
		n=(ParticleTextureNode *)n->GetSucc();
	}

	ParticleParamNode *ppn;

	ppn=(ParticleParamNode *)paramList.GetHead();
	
	Tpoint    pos;
	Trotation rot = IMatrix;

	while(ppn)
	{
		if(ppn->bspCTID)
		{	
			ppn->bspObj   = new DrawableBSP(Falcon4ClassTable[ppn->bspCTID].visType[ppn->bspVisType],
		                     &pos,
							 &rot);
		}
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}


}

void DrawableParticleSys::ReleaseTexturesOnDevice(DXContext *rc)
{
	psContext = 0;

	ParticleTextureNode *n;

	n=(ParticleTextureNode *)textureList.GetHead();
	while(n)
	{
		n->texture.FreeAll();
		n=(ParticleTextureNode *)n->GetSucc();
	}
	ParticleParamNode *ppn;

	ppn=(ParticleParamNode *)paramList.GetHead();
	while(ppn)
	{
		if(ppn->bspObj)
		{	
			delete ppn->bspObj;
			ppn->bspObj = 0;
		}
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}


}

/*******************************************************************/

static const char TRAILFILE[] = "particlesys.ini";

extern FILE* OpenCampFile (char *filename, char *ext, char *mode);

int MatchString(char *arg, char **q)
{
	int i=0;
	while (*q)
	{
		i++;
		if(stricmp(arg,*q)==0)
		{
			return i;
		}
	}
	return(0);
}

void DrawableParticleSys::UnloadParameters(void)
{
	ParticleTextureNode *ptn;
	ParticleParamNode *ppn;

	while(ptn=(ParticleTextureNode *)textureList.RemHead())
	{
		delete ptn;
	}

	while(ppn=(ParticleParamNode *)paramList.RemHead())
	{
		if(ppn->bspObj)
			delete ppn->bspObj;
		delete ppn;
	}

	delete [] PPN;

	PPN=0;
}

char *trim(char *in)
{
	char *q;

	if(!in) return 0;

	while(*in==' ' || *in=='/t')
	{
		in++;
	}

	q=in;
	
	while(*q!=0)
		q++;

	q--;
	while(*q==' ' || *q=='/t')
	{
		*q=0;
		q--;
	}

	return(in);
}


void DrawableParticleSys::LoadParameters(void)
{
	char buffer[1024];
  char path[_MAX_PATH];
  FILE *fp;
	ParticleParamNode *ppn=0;

	DPS_Node *dps;

	paramList.Lock();
	dps=(DPS_Node *)dpsList.GetHead();
	while(dps)
	{
		dps->owner->ClearParticles();
		dps=(DPS_Node *)dps->GetSucc();
	}

	if(PPN)
		delete [] PPN;

	while(ppn=(ParticleParamNode *)paramList.RemHead())
	{
		delete ppn;
	}


	sprintf (path, "%s\\terrdata\\%s", FalconDataDirectory, TRAILFILE); // MLR 12/14/2003 - This should probably be fixed
	fp = fopen(path, "r");

	ppn=0;
	int currentemitter=-1;

	// Cobra - Use intrnal PS.ini
	int k=0;
	while (1)
	{		
		char *com,*ary,*arg;
		float  i; // time index

		if (fp == NULL)
		{
			if (g_bHighSFX)
			{
				while (strlen(PS_Data_High[k]) == 0)
					k++;
				strcpy(buffer, PS_Data_High[k]);
				if (stricmp(PS_Data_High[k], "END")==0)
					break;
				k++;
			}
			else
			{
				while (strlen(PS_Data_Low[k]) == 0)
					k++;
				strcpy(buffer, PS_Data_Low[k]);
				if (stricmp(PS_Data_Low[k], "END")==0)
					break;
				k++;
			}
		}
		else
		{
			if(fgets(buffer, sizeof buffer, fp) == 0)
			{
				fclose (fp);
				break;
			}
		}
		// end Cobra

		if (buffer[0] == '#' || buffer[0] == ';' ||buffer[0] == '\n')
			continue;

		int b;
		for(b=0;b<1024 && (buffer[b]==' ' || buffer[b]=='\t');b++);

		com=strtok(&buffer[b],"=\n");
		arg=strtok(0,"\n\0");

		if (com[0] == '#' || com[0] == ';')
			continue;


		/* seperate command and array number 'command[arraynumber]*/
		com=strtok(com,"[");
		ary=strtok(0,"]");

		com=trim(com);
		ary=trim(ary);

		i=TokenF(ary,0);

		/* kludge so that arg is the current string being parsed */
		SetTokenString(arg);

		if(!com)
			continue;

#define On(s) if(stricmp(com,s)==0)

		On("id")
		{
			char *n = TokenStr(0);
			if(n)
			{
				if(ppn=new ParticleParamNode)
				{
					memset(ppn,0,sizeof(*ppn));
					strncpy(ppn->name,n,PS_NAMESIZE);
					ppn->id					=-1;
					ppn->accel[0].value		= 1;
					ppn->alpha[0].value		= 1;
					ppn->velInherit			= 1;
					ppn->drawType			= PSDT_POLY;
					currentemitter			= -1;
					ppn->sndVol[0].value	= 0;
					ppn->sndPitch[0].value	= 1;
					ppn->trailId			= -1;
					ppn->visibleDistance	= 10000;
					paramList.AddTail(ppn);
				}
			}
		}

		// we need a valid ppn pointer
		if(!ppn)
			continue;

		On("lifespan")
		{
			ppn->lifespan          = TokenF(1);
			ppn->lifespanvariation = TokenF(0);
		}

		On("color")
		{ // color[x]=0,1,0,0,1
		  ppn->color[ppn->colorStages].value.r = TokenF(0);
		  ppn->color[ppn->colorStages].value.g = TokenF(0);
		  ppn->color[ppn->colorStages].value.b = TokenF(0);
		  ppn->color[ppn->colorStages].time    = i;
		  ppn->colorStages++;
		  continue;
		}

		On("light")
		{
		  ppn->light[ppn->lightStages].value.r = TokenF(0);
		  ppn->light[ppn->lightStages].value.g = TokenF(0);
		  ppn->light[ppn->lightStages].value.b = TokenF(0);
		  ppn->light[ppn->lightStages].time    = i;
		  ppn->lightStages++;
		  continue;
		}

		On("size")
		{
		  ppn->size[ppn->sizeStages].value = TokenF(0);
		  ppn->size[ppn->sizeStages].time  = i;
		  ppn->sizeStages++;
		  continue;
		}

		On("flags") 
			ppn->flags = TokenFlags(0,PSF_CHARACTERS);

		On("texture")
		{
			strncpy(ppn->texFilename,arg,64);
			ParticleTextureNode *ptn;
			ptn=GetTextureNode(ppn->texFilename);
			ppn->texture = &ptn->texture;
			float Frames=TokenF(0);
			if(Frames) ppn->texture=1000;
		}

		On("gravity")
		{
		  ppn->gravity[ppn->gravityStages].value = TokenF(0);
		  ppn->gravity[ppn->gravityStages].time  = i;
		  ppn->gravityStages++;
		  continue;
		}

		On("alpha")
		{
		  ppn->alpha[ppn->alphaStages].value = TokenF(0);
		  ppn->alpha[ppn->alphaStages].time  = i;
		  ppn->alphaStages++;
		  continue;
		}

		On("drag")
		{
			ppn->simpleDrag=TokenF(0);
		}

		On("bounce")
		{
			ppn->bounce=TokenF(0);
		}

		if(currentemitter<10)
		{
			On("addemitter")
			{
				currentemitter++;
			}

			if(currentemitter>=0)
			{
				On("emissionid")
				{
					char *n;
					n=TokenStr("none");
					ppn->emitter[currentemitter].id   = -1; 
					if(n)
						strncpy(ppn->emitter[currentemitter].name,n,PS_NAMESIZE);
				}

				On("emissionmode")
				{
					char *emitenums[]={"EMITONCE","EMITPERSEC","EMITONIMPACT","EMITONEARTHIMPACT","EMITONWATERIMPACT",0};
					ppn->emitter[currentemitter].mode = (PSEmitterModeEnum)TokenEnum(emitenums,-1);
					if(ppn->emitter[currentemitter].mode == -1)
					{
						char *emitenums[]={"ONCE","PERSEC","IMPACT","EARTHIMPACT","WATERIMPACT",0};
						ppn->emitter[currentemitter].mode = (PSEmitterModeEnum)TokenEnum(emitenums,0);
					}

				}
				On("emissiondomain")
				{
					ppn->emitter[currentemitter].domain.Parse();
				}

				On("emissiontarget")
				{
					ppn->emitter[currentemitter].target.Parse();
				}

				On("emissionrate")
				{
					ppn->emitter[currentemitter].rate[ppn->emitter[currentemitter].stages].value = TokenF(0);
					ppn->emitter[currentemitter].rate[ppn->emitter[currentemitter].stages].time  = i;
					ppn->emitter[currentemitter].stages++;
				}

				On("emissionvelocity")
				{
					ppn->emitter[currentemitter].velocity		= TokenF(0);
					ppn->emitter[currentemitter].velVariation	= TokenF(0);
				}

			}
		}

		On("acceleration")
		{
			ppn->accel[ppn->accelStages].value = TokenF(1);
			ppn->accel[ppn->accelStages].time  = i;
			ppn->accelStages++;
		}

		On("inheritvelocity")
		{
			ppn->velInherit = TokenF(1);
		}

		On("initialVelocity")
		{
			ppn->velInitial		= TokenF(1);
			ppn->velVariation	= TokenF(0);	
		}

		On("drawtype")
		{
			char *enumstr[]={"none","poly","point","line", 0};
			ppn->drawType = (PSDrawType)TokenEnum(enumstr,0);
		}

		On("soundid")
		{
			ppn->sndId=TokenI(0);
		}

		On("soundlooped")
		{
			ppn->sndLooped=TokenI(0);
		}

		On("soundVolume")
		{
			ppn->sndVol[ppn->sndVolStages].value = (TokenF(0) - 1) * -10000;
			ppn->sndVol[ppn->sndVolStages].time  = i;
			ppn->sndVolStages++;
		}

		On("soundPitch")
		{
			ppn->sndPitch[ppn->sndPitchStages].value = TokenF(1);
			ppn->sndPitch[ppn->sndPitchStages].time  = i;
			ppn->sndPitchStages++;
		}

		On("trailid")
		{
			ppn->trailId=TokenI(-1);
		}

		On("groundfriction")
		{
			ppn->groundFriction=TokenF(0);
		}

		On("visibledistance")
		{
			ppn->visibleDistance=TokenF(10000);
		}

		On("dieonground")
		{
			ppn->dieOnGround=TokenI(0);
		}

		On("modelct")
		{
			ppn->bspCTID    = TokenI(0);
			ppn->bspVisType = TokenI(0);
		}

		On("orientation")
		{
			char *enumstr[]={"none","movement", 0};
			ppn->orientation = (PSOrientation)TokenEnum(enumstr,0);
		}

		On("emitonerandomly")
		{
			ppn->emitOneRandomly = TokenI(0);
		}

    }
//    fclose (fp);

	/* id */
	int l;
	for(l=0;l<nameListCount;l++)
	{
		ppn=(ParticleParamNode *)paramList.GetHead();
		while(ppn)
		{
			if(stricmp(ppn->name,nameList[l])==0)
			{
				ppn->id=l;
				ppn=0;
			}
			else
				ppn=(ParticleParamNode *)ppn->GetSucc();
		}
	}


	// number all the nodes that don't match above;
	ppn=(ParticleParamNode *)paramList.GetHead();
	while(ppn)
	{
		if(ppn->id==-1)
		{
			ppn->id=l;
			l++;
		}
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}

	// copy pointers to PPN array here
	//PPN = (ParticleParamNode **)malloc(sizeof(ParticleParamNode *) * l);
	PPN = new ParticleParamNode * [l];
	memset(PPN,0,sizeof(ParticleParamNode *) * l);
	PPNCount=l;

	ppn=(ParticleParamNode *)paramList.GetHead();
	while(ppn)
	{
		PPN[ppn->id]=ppn;
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}

//------------------
//	fp = fopen("NoEmitterDefined.txt", "w");
//------------------

	// link the emmitter names to ids;
	ppn=(ParticleParamNode *)paramList.GetHead();
	while(ppn)
	{
		int t;
		for(t=0;t<PSMAX_EMITTERS && ppn->emitter[t].stages;t++)
		{
			ParticleParamNode *n2;

			n2=(ParticleParamNode *)paramList.GetHead();
			while(n2)
			{
				if(stricmp(n2->name,ppn->emitter[t].name)==0)
				{
					ppn->emitter[t].id=n2->id;
					n2=0;
				}
				else
					n2=(ParticleParamNode *)n2->GetSucc();
			}

			if(ppn->emitter[t].id==-1)
			{  // this will prevent a CTD
				ppn->emitter[t].stages=0;
//------------------
//				fprintf(fp, " %s\n", ppn->emitter[t].name);
//------------------
			}
		}
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}

//------------------
//	fclose(fp);
//------------------

	if(psContext)
	{
		DXContext *stored = psContext; // have to store it because release clears it.

		ReleaseTexturesOnDevice(stored);
		SetupTexturesOnDevice(stored);
	}

	paramList.Unlock();
}


int DrawableParticleSys::IsValidPSId(int id)
{
	if(id < PPNCount && PPN[id])
	{
		return 1;
	}
	return 0;
}

int DrawableParticleSys::GetNameId(char *name)
{
	int l;
	for(l = 0; l < PPNCount; l++)
	{
		if(PPN[l] && PPN[l]->name && stricmp(name, PPN[l]->name)==0)
		{
			return l;
		}
	}
	return 0;
}


/***********************************************************************
  
	The ParticleDomain
	
***********************************************************************/

void ParticleDomain::GetRandomPosition(Tpoint *p)
{
	p->x=NRAND * sphere.size.x + sphere.pos.x;
	p->y=NRAND * sphere.size.y + sphere.pos.y;
	p->z=NRAND * sphere.size.z + sphere.pos.z;
}

void ParticleDomain::GetRandomDirection(Tpoint *p)
{
	GetRandomPosition(p);

	float d = sqrt(p->x * p->x + p->y * p->y + p->z * p->z);

	if(d)
	{
		p->x/=d;
		p->y/=d;
		p->z/=d;
	}
	else
	{
		p->x=1;
		p->y=0;
		p->z=0;
	}
}

void ParticleDomain::Parse(void)
{
	char *enums[]=
	{
		"sphere", 
		"plane",  
		"box",    
		"blob",   
		"cylinder", 
		"cone",
		"triangle",
		"rectangle",
		"disc",
		"line",
		"point",
		0
	};
	type = (PSDomainEnum)TokenEnum(enums,0);		int l;
	for(l=0;l<9;l++)
	{
		param[l]=TokenF(0);
	}
}

#endif
#endif