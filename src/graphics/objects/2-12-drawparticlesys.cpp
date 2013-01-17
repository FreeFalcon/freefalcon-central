#if 1
#include "DrawParticleSys.h"

/***************************************************************************\
    Draw
    MLR
\***************************************************************************/

#include "TimeMgr.h"
#include "TOD.h"
#include "falclib/include/token.h"
#include "RenderOW.h"
#include "Matrix.h"
#include "TOD.h"
#include "Tex.h"
#include "RealWeather.h"
#include "falclib/include/falclib.h"
#include "sim\include\simlib.h" // MLR needed for SetVelocity since objects set there Delta values per frame
#include "sim\include\otwdrive.h" // MLR needed for SetVelocity since objects set there Delta values per frame
#include "falclib/include/fsound.h"
#include "drawsgmt.h"

// for when fakerand just won't do
#define	NRANDPOS ((float)( (float)rand()/(float)RAND_MAX ))
#define NRAND	 ( 1.0f - 2.0F * NRANDPOS )


extern int g_nGfxFix;
extern char FalconDataDirectory[];
extern char FalconObjectDataDir[];
extern int sGreenMode;


bool g_bNoParticleSys=0;
BOOL gParticleSysGreenMode=0;
Tcolor gParticleSysLitColor={1,1,1};

/**** Static class data ***/
BOOL    DrawableParticleSys::greenMode	= FALSE;
char	*DrawableParticleSys::nameList[]=
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
    "$RISING_GROUNDHIT_EXPLOSION_DEBRISTRAIL",
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
    "$WATER_WAKE"
};
int		DrawableParticleSys::nameListCount = sizeof(DrawableParticleSys::nameList)/sizeof(char *);
AList	DrawableParticleSys::textureList;
AList	DrawableParticleSys::paramList;
AList	DrawableParticleSys::dpsList;
float	DrawableParticleSys::groundLevel;
float	DrawableParticleSys::cameraDistance;

/****** Jams stuff ******/
static const float TEX_UV_LSB = 1.f/1024.f;
static const float TEX_UV_MIN = TEX_UV_LSB;
static const float TEX_UV_MAX = 1.f-TEX_UV_LSB;


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
	psRGBA  value;
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
	PSEM_PERSEC = 1
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

	union
	{
		float		 param[9];

		struct
		{
			Tpoint pos, 
				   size;
		} sphere;

		struct
		{
			float a,b,c,d;
		} plane;

		struct
		{
			Tpoint cornerA,
				   cornerB;
		} box;

		struct
		{
			Tpoint pos, size;
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

/****** Particle Parameter structure ******/
#define PS_NAMESIZE 32
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

#define PSMAX_EMITTERS 10
	struct
	{
		PSEmitterModeEnum  mode;
		ParticleDomain	   domain;
		ParticleDomain     target;
		float		   param[9];
		int            id;
		char           name[PS_NAMESIZE];
		int            stages;
		timedFloat     rate[10];
	} emitter[PSMAX_EMITTERS];

	float   bounce;

	char     texFilename[64];
	Texture *texture;
};

/** ParticleSys Flags **/
#define PSF_CHARACTERS "MG"
#define PSF_NONE	     (0) // use spacing value a a time (in seconds) instead of distance
#define PSF_MORPHATTRIBS (1<<0) // blend attributes over lifespan)
#define PSF_GROUNDTEST   (1<<1) // enable terrain surface level check

ParticleParamNode **PPN = 0;
int PPNCount = 0;


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
		delete ppn;
	}

	free(PPN);
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

	dps=(DPS_Node *)dpsList.GetHead();
	while(dps)
	{
		dps->owner->ClearParticles();
		dps=(DPS_Node *)dps->GetSucc();
	}

	if(PPN)
		free(PPN);

	while(ppn=(ParticleParamNode *)paramList.RemHead())
	{
		delete ppn;
	}


	sprintf (path, "%s\\terrdata\\%s", FalconDataDirectory, TRAILFILE); // MLR 12/14/2003 - This should probably be fixed
	fp = fopen(path, "r");
	if (fp == NULL) 
		return;

	ppn=0;

	int currentemitter=-1;

    while (fgets (buffer, sizeof buffer, fp)) 
	{		
		char *com,*ary,*arg;
		int  i; // array index

		if (buffer[0] == '#' || buffer[0] == ';' ||buffer[0] == '\n')
			continue;

		com=strtok(buffer,"=\n");
		arg=strtok(0,"\n\0");

		/* seperate command and array number 'command[arraynumber]*/
		com=strtok(com,"[");
		ary=strtok(0,"]");

		com=trim(com);
		ary=trim(ary);

		i=TokenI(ary,0);

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
					ppn->id              =-1;
					ppn->accel[0].value  = 1;
					ppn->alpha[0].value  = 1;
					ppn->velInherit      = 1;
					ppn->drawType        = PSDT_POLY;
					currentemitter       = -1;
					ppn->sndVol[0].value = 0;
					ppn->sndPitch[0].value = 1;
					ppn->trailId = -1;
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
		  ppn->colorStages=i+1;
		  ppn->color[i].value.r = TokenF(0);
		  ppn->color[i].value.g = TokenF(0);
		  ppn->color[i].value.b = TokenF(0);
		  ppn->color[i].value.a = TokenF(0);
		  ppn->color[i].time = TokenF(0);
		  continue;
		}

		On("light")
		{
		  ppn->lightStages=i+1;
		  ppn->light[i].value.r = TokenF(0);
		  ppn->light[i].value.g = TokenF(0);
		  ppn->light[i].value.b = TokenF(0);
		  ppn->light[i].time = TokenF(0);
		  continue;
		}

		On("size")
		{
		  ppn->sizeStages=i+1;
		  ppn->size[i].value = TokenF(0);
		  ppn->size[i].time  = TokenF(0);
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
		}

		On("gravity")
		{
		  ppn->gravityStages=i+1;
		  ppn->gravity[i].value = TokenF(0);
		  ppn->gravity[i].time  = TokenF(0);
		  continue;
		}

		On("alpha")
		{
		  ppn->alphaStages=i+1;
		  ppn->alpha[i].value = TokenF(0);
		  ppn->alpha[i].time  = TokenF(0);
		  continue;
		}

		On("bounce")
		{
			ppn->bounce=TokenF(0);
		}

		if(currentemitter<5)
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
					char *emitenums[]={"EMITONCE","EMITPERSEC",0};
					ppn->emitter[currentemitter].mode = (PSEmitterModeEnum)TokenEnum(emitenums,0);
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
					ppn->emitter[currentemitter].stages=i+1;
					ppn->emitter[currentemitter].rate[i].value = TokenF(0);
					ppn->emitter[currentemitter].rate[i].time  = TokenF(0);
				}
			}
		}

		On("acceleration")
		{
			ppn->accelStages	= i+1;
			ppn->accel[i].value = TokenF(1);
			ppn->accel[i].time  = TokenF(0);
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
			ppn->sndVolStages	= i+1;
			ppn->sndVol[i].value = (TokenF(0) - 1) * -10000;
			ppn->sndVol[i].time  = TokenF(0);
		}

		On("soundPitch")
		{
			ppn->sndPitchStages	= i+1;
			ppn->sndPitch[i].value = TokenF(1);
			ppn->sndPitch[i].time  = TokenF(0);
		}

		On("trailid")
		{
			ppn->trailId=TokenI(-1);
		}

		On("groundfriction")
		{
			ppn->groundFriction=TokenF(0);
		}
    }
    fclose (fp);

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
	PPN = (ParticleParamNode **)malloc(sizeof(ParticleParamNode *) * l);
	memset(PPN,0,sizeof(ParticleParamNode *) * l);
	PPNCount=l;

	ppn=(ParticleParamNode *)paramList.GetHead();
	while(ppn)
	{
		PPN[ppn->id]=ppn;
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}

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
			}
		}
		ppn=(ParticleParamNode *)ppn->GetSucc();
	}
}


int DrawableParticleSys::IsValidPSId(int id)
{
	if(id < PPNCount && PPN[id])
	{
		return 1;
	}
	return 0;
}








/***********************************************************************
  
	The ParticleNode
	
***********************************************************************/

class ParticleNode : public ANode
{
public:
	ParticleNode(int ID, Tpoint *Pos, Tpoint *Vel=0, ParticleDomain *target=0);
	~ParticleNode();
	void Exec(void);
	void Draw(class RenderOTW *renderer, int LOD);
	int IsDead(void);
private:
	void Init(int ID, Tpoint *Pos, Tpoint *Vel=0, ParticleDomain *target=0);

	void EvalAttrs(void);

	void EvalTimedFloat(int Count, timedFloat *input, float &retval);
	void EvalTimedRGBA(int Count, timedRGBA *input, psRGBA &retval);
	void EvalTimedRGB(int Count, timedRGB *input, psRGB &retval);
public:
	ParticleParamNode *ppn;

	int    birthTime; //
	float  lifespan;  // in seconds
	float  age;		  // in seconds
	float  life;      // normalized
	float  plife;     // life value for the previous exec

	float  gravity,
		   size,
		   accel;

	float  alpha;
	psRGB  color;
	psRGB  light; 

	Tpoint pos,
		   vel;

	F4SoundPos *soundPos;
	int sndId;
	int sndPlay;
	float sndVol;
	float sndPitch;

	DrawableTrail *trailObj;
};

/**** some useful crap ****/
#define MORPH(n,src,dif) ( (src) + ( (dif) * (n) ) )
#define ATTRMORPH(ATR) attrib.ATR = MORPH(life, params->birthAttrib.ATR, params->deathAttribDiff.ATR)
/**** end of useful crap ****/

ParticleNode::ParticleNode(int ID, Tpoint *Pos, Tpoint *Vel, ParticleDomain *target)
{
	Init(ID,Pos,Vel,target);
}


void ParticleNode::Init(int ID, Tpoint *Pos, Tpoint *Vel, ParticleDomain *target)
{
	trailObj=0;
	soundPos=0;
	
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

	if(target)
	{
		float v;

		v=ppn->velInitial + ppn->velVariation * NRAND;
		Tpoint w;
		target->GetRandomDirection(&w);
		vel.x+=w.x*v;
		vel.y+=w.y*v;
		vel.z+=w.z*v;

	}
	plife=0;
	
	sndPlay=0;
	sndId=ppn->sndId;
	if(sndId)
	{
		soundPos=new F4SoundPos();
		sndPlay=1;
	}
	sndVol=0;

	if(ppn->trailId>=0)
	{
		trailObj = new DrawableTrail(ppn->trailId,1);
		if(trailObj)
		{
			OTWDriver.InsertObject(trailObj);
		}
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

void ParticleNode::EvalAttrs(void)
{
	EvalTimedFloat( ppn->sizeStages, ppn->size, size );
	EvalTimedFloat( ppn->gravityStages, ppn->gravity, gravity );
	EvalTimedFloat( ppn->alphaStages, ppn->alpha, alpha );
	EvalTimedFloat( ppn->accelStages, ppn->accel, accel );
	if(sndId)
	{
		EvalTimedFloat( ppn->sndVolStages, ppn->sndVol, sndVol );
		EvalTimedFloat( ppn->sndPitchStages, ppn->sndPitch, sndPitch );
	}
	EvalTimedRGB  ( ppn->colorStages, ppn->color, color);
	EvalTimedRGB  ( ppn->lightStages, ppn->light, light);
}


ParticleNode::~ParticleNode()
{
	if(soundPos)
		delete soundPos;
	if(trailObj)
		OTWDriver.RemoveObject(trailObj,1);
}

int ParticleNode::IsDead(void) 
{
	if(life > 1.0)
	{
		// don't be dead, until these two guys are done
		if(trailObj && trailObj->IsTrailEmpty())
			return 0;
		if(soundPos && soundPos->IsPlaying(sndId,0))
			return 0;
		return 1;
	}
	return 0;
}



void ParticleNode::Exec(void)
{
	if(lifespan<=0) return;

	age  = (TheTimeManager.GetClockTime() - birthTime) * .001;
	life = age / lifespan;
	if(life>1.0 && plife<1.0)
	{
		life=1.0; // just to make sure we do the last stages
	}

	if(life>1.0) // waiting to die
		return;

	EvalAttrs();


	/************ Emitter Particle **************/
	int l;
	for(l = 0; ppn->emitter[l].stages && l<PSMAX_EMITTERS; l++)
	{
		float qty=0;
		switch(ppn->emitter[l].mode)
		{
		case PSEM_ONCE:
			{
				int t;

				for(t=0;t<ppn->emitter[l].stages;t++)
				{
					if( ppn->emitter[l].rate[t].time < life && 
						ppn->emitter[l].rate[t].time >= plife)
					{  // add qty if we've aged past a stage
						qty+=ppn->emitter[l].rate[0].value;
					}
				}
			}
			break;
		case PSEM_PERSEC:
			EvalTimedFloat(ppn->emitter[l].stages,ppn->emitter[l].rate,qty);
			qty*=SimLibMajorFrameTime;
			break;
		}

		while(qty > 0)
		{
			Tpoint w;

			ppn->emitter[l].domain.GetRandomPosition(&w);
			w.x+=pos.x;
			w.y+=pos.y;
			w.z+=pos.z;
			ParticleNode *n=new ParticleNode(ppn->emitter[l].id,&w,&vel,&ppn->emitter[l].target);
			n->InsertAfter((ANode *)this);
			qty-=1.0f;
		}
	}



	// update velocity values
	vel.z+=(32 * SimLibMajorFrameTime) * gravity;
	
	if(accel)
	{
		float fps = (accel * SimLibMajorFrameTime);
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
			
	pos.x+=vel.x * SimLibMajorFrameTime;
	pos.y+=vel.y * SimLibMajorFrameTime;
	pos.z+=vel.z * SimLibMajorFrameTime;

	if(pos.z > DrawableParticleSys::groundLevel)
	{
		pos.z=DrawableParticleSys::groundLevel;
		vel.z *= -ppn->bounce;
	}

	
	if(pos.z == DrawableParticleSys::groundLevel)
	{
		float fps = (ppn->groundFriction * SimLibMajorFrameTime);
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
	

	if(trailObj)
		trailObj->AddPointAtHead(&pos,0);

	if(soundPos && sndId && sndPlay)
	{
		soundPos->UpdatePos(pos.x, pos.y, pos.z, vel.x, vel.y, vel.z);
		soundPos->Sfx(sndId,0,sndPitch,sndVol);
		if(!ppn->sndLooped)
			sndPlay=0; // so we only play it once
	}



	plife=life;

}

void ParticleNode::Draw(class RenderOTW *renderer, int LOD)
{
	if(ppn->drawType==PSDT_POLY)
	{
		Tpoint os,pv;
		ThreeDVertex v0,v1,v2,v3;

			renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
		if(ppn->texture)
			renderer->context.SelectTexture1(ppn->texture->TexHandle());
		
		renderer->TransformPointToView(&pos,&pv);
		
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
}




static	Tcolor	gLight;

/***************************************************************************\
    Initialize a segmented trial object.
\***************************************************************************/
DrawableParticleSys::DrawableParticleSys( int particlesysType, float scale )
: DrawableObject( scale )
{
	ShiAssert( particlesysType >= 0 );

	// Store the particlesys type
	type=particlesysType;
	dpsList.AddHead(&dpsNode);
	dpsNode.owner=this;
	
	// Set to position 0.0, 0.0, 0.0;
	position.x = 0.0F;
	position.y = 0.0F;
	position.z = 0.0F;

	headFPS.x=0.0f;
	headFPS.y=0.0f;
	headFPS.z=0.0f;

	Something=0;
}



/***************************************************************************\
    Remove an instance of a segmented particlesys object.
\***************************************************************************/
DrawableParticleSys::~DrawableParticleSys( void )
{
	// Delete this object's particlesys
	dpsNode.Remove();
	ParticleNode *n;
	while(n=(ParticleNode *)particleList.RemHead())
	{
		delete n;
	}

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

	ParticleNode *n = new ParticleNode(Id, worldPos, v);
	particleList.AddHead(n);
}

/*
void DrawableParticleSys::AddParticle( char *name, Tpoint *worldPos )
{
	DWORD now;
	now = TheTimeManager.GetClockTime();

	ParticleParam *ppn;

	ParticleNode *n = new ParticleNode(Id, worldPos);
	particleList.AddHead(n);
}
*/


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
	ParticleNode *n;
	float xx,yy,zz;

	xx=position.x-renderer->X();
	yy=position.y-renderer->Y();
	zz=position.z-renderer->Z();

	cameraDistance=sqrt(xx*xx+yy*yy+zz*zz);

	n=(ParticleNode *)particleList.GetHead();

	while(n)
	{
		n->Draw(renderer,LOD);
		n=(ParticleNode *)n->GetSucc();
	}
}


void DrawableParticleSys::Exec( void)
{
	ParticleNode *n;

	n=(ParticleNode *)particleList.GetHead();

	groundLevel=OTWDriver.GetGroundLevel(n->pos.x,n->pos.y);

	while(n)
	{
		ParticleNode *n2 = (ParticleNode *)n->GetSucc();

		n->Exec();
		if(n->IsDead())
		{
			n->Remove();
			delete n;
		}
		n=n2;
	}
}

void DrawableParticleSys::ClearParticles(void)
{
	ParticleNode *n;

	while(n=(ParticleNode *)particleList.RemHead())
	{
		delete n;
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


void DrawableParticleSys::SetupTexturesOnDevice(DXContext *rc)
{
	ParticleTextureNode *n;

	n=(ParticleTextureNode *)textureList.GetHead();
	while(n)
	{
		n->texture.LoadAndCreate(n->filename,MPR_TI_DDS);
		n=(ParticleTextureNode *)n->GetSucc();
	}

}

void DrawableParticleSys::ReleaseTexturesOnDevice(DXContext *rc)
{
	ParticleTextureNode *n;

	n=(ParticleTextureNode *)textureList.GetHead();
	while(n)
	{
		n->texture.FreeAll();
		n=(ParticleTextureNode *)n->GetSucc();
	}
}





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