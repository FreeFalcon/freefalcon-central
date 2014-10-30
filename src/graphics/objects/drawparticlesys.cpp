#include <cISO646>
#include <math.h>
#include "profiler.h"
#include "Graphics/Include/Drawbsp.h"
#include "falclib/include/fsound.h"
#include "DrawParticleSys.h"
#include "tod.h"

#include <iostream>


//JAM 19Apr04

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
#include "falclib/include/falclib.h"
#include "sim/include/simlib.h" // MLR needed for SetVelocity since objects set there Delta values per frame
#include "sim/include/otwdrive.h" // MLR needed for SetVelocity since objects set there Delta values per frame
#include "drawsgmt.h"
#include "drawbsp.h"
#include "terrtex.h"
#include "sfx.h"
#include "falclib/include/entity.h"
#include "PSData.h"
#include "drawable.h"
#include "RedMacros.h"

// for when fakerand just won't do
#define NRANDPOS ((float)( (float)rand()/(float)RAND_MAX ))
#define DTR 0.01745329F

extern int g_nGfxFix;
extern char FalconDataDirectory[];
extern char FalconObjectDataDir[];
extern int sGreenMode;

// Cobra - Purge the PS list every PurgeTimeInc msec.
extern int g_nPSPurgeInterval;
static DWORD TimeToPurge = 0L;
static WORD  ParticleFilterCount; // COBRA - RED - Counter of SFX adding for filtering
#define MAX_PARTICLE_FILTER_LEVEL 10 // COBRA - RED - Level for SFX Filter

//static DWORD PurgeTimeInc = 60000L;
static DWORD PurgeTimeInc = g_nPSPurgeInterval;
static DWORD TimeToPurgeAll = 0L;
static DWORD PurgeAllTimeInc = max(g_nPSPurgeInterval * 10, 600000);
static char ErrorMessage[128];

bool g_bNoParticleSys = 0;
BOOL gParticleSysGreenMode = 0;
Tcolor gParticleSysLitColor = {1, 1, 1};

extern int g_nPSKillFPS;//Cobra
extern bool g_bHighSFX; // Cobra
extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

/**** Static class data ***/
BOOL    DrawableParticleSys::greenMode = FALSE;
char *DrawableParticleSys::nameList[SFX_NUM_TYPES + 1] =
{
    "$NONE",
    "$AC_AIR_EXPLOSION",
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
    "$WATERCLOUD",
    "$AIR_DUSTCLOUD",
    "$GUNSMOKE",
    "$AIR_SMOKECLOUD2",
    "$GUNFIRE",                //Used for AC gun fire effect at gun position
    "$FEATURE_CHAIN_REACTION",
    "$WATER_EXPLOSION",
    "$SAM_LAUNCH",
    "$MISSILE_LAUNCH",
    "$DUST1",
    "$CHAFF",    // Chaff effect by PS
    "$WATER_WAKE_MEDIUM",
    "$TIMER",
    "$DIST_AIRBURSTS",
    "$DIST_GROUNDBURSTS",
    "$DIST_SAMLAUNCHES",
    "$DIST_AALAUNCHES",
    "$WATER_WAKE_LARGE",
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
    "$DUSTCLOUD",
    "$NAPALM",
    "$AIRBURST",
    "$GROUNDBURST",
    "$GROUND_STRIKE_NOFIRE",
    "$LONG_HANGING_SMOKE",
    "$SMOKETRAIL",
    "$DEBRISTRAIL",
    "$VEHICLE_EXPLOSION",
    "$RISING_GROUNDHIT_EXPLOSION_DEBR",
    "$FIRETRAIL",
    "$FIRE_NOSMOKE",
    "$LANDING_SMOKE",
    "$WATER_CLOUD",
    "$WATERTRAIL",
    "$GUN_TRACER",
    "$AC_DEBRIS",
    "$CAT_LAUNCH",
    "$FLARE_GFX",
    "$SPARKS_NO_DEBRIS",
    "$BURNING_PART",
    "$AIR_EXPLOSION_NOGLOW",
    "$HIT_EXPLOSION_NOGLOW",
    "$SHOCK_RING_SMALL",
    "$FAST_FADING_SMOKE",
    "$LONG_HANGING_SMOKE2",
    "$AAA_EXPLOSION",
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
    "$SHIP_BURNING_FIRE",
    "$CAT_RANDOM_STEAM",
    "$CAT_STEAM",
    "$FIRE5",
    "$FIRE6",
    "$FIRESMOKE",
    "$TRAILSMOKE",
    "$VEHICLE_DUST",
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
    "$FEATURE_EXPLOSION",
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
    "$VEHICLE_BURNING",
    "$INCENDIARY_EXPLOSION",
    "$SPARK_TRACER",
    "$WATER_WAKE_SMALL",
    "--- kludge ---",
    "$GUN_HIT_GROUND",
    "$GUN_HIT_OBJECT",
    "$GUN_HIT_WATER",
    "$VEHICLE_DIEING",
    "$AC_EARLY_BURNING",
    "$AC_BURNING_1",
    "$AC_BURNING_2",
    "$AC_BURNING_3",
    "$AC_BURNING_4",
    "$AC_BURNING_5",
    "$AC_BURNING_6",
    "$GUN_SMOKE", //TJL
    "$NUKE",//TJL
    "$VORTEX_STRONG", //RV - I-Hawk Vortex
    "$VORTEX_MEDIUM",
    "$VORTEX_WEAK",
    "$VORTEX_LARGE_STRONG",
    "$VORTEX_LARGE_MEDIUM",
    "$VORTEX_LARGE_WEAK",
};

int DrawableParticleSys::nameListCount = sizeof(DrawableParticleSys::nameList) / sizeof(char *);
AList DrawableParticleSys::textureList;
ProtectedAList DrawableParticleSys::paramList;
AList DrawableParticleSys::dpsList;
TRAIL_HANDLE DrawableParticleSys::TrailsHandle;
float DrawableParticleSys::groundLevel;
float DrawableParticleSys::cameraDistance;
int DrawableParticleSys::reloadParameters = 0;
float DrawableParticleSys::winddx;
float DrawableParticleSys::winddy;
AList DrawableParticleSys::AnimationsList;
float DrawableParticleSys::Log10Array[LOG10_ARRAY_ITEMS];
float DrawableParticleSys::ASinArray[LOG10_ARRAY_ITEMS];
float DrawableParticleSys::FadeArray[FADE_ARRAY_ITEMS];
float DrawableParticleSys::SizeArray[SIZE_ARRAY_ITEMS];
class RenderOTW *DrawableParticleSys::PS_Renderer;

/**** Macros ****/
#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))
#define NRESCALE(in,outmin,outmax)   RESCALE(in,0,1,outmin,outmax)


#define K_CALC(Stage, Count) \
if(Count)\
{\
 Stage[Count-1].K = (fabs(Stage[Count].value) - fabs(Stage[Count-1].value)) /(Stage[Count].time - Stage[Count-1].time );\
 if(Stage[Count].value<0) \
 {\
	 Stage[Count].LogMode=true; \
 }\
 else \
 {\
     Stage[Count].LogMode=false;\
 }\
 Stage[Count].value = fabs(Stage[Count].value);\
}

#define K_CALCRGB(Stage, Count, color) if(Count){\
 Stage[Count-1].K.color = (fabs(Stage[Count].value.color) - fabs(Stage[Count-1].value.color))/(Stage[Count].time - Stage[Count-1].time );\
 if(Stage[Count].value.color<0) Stage[Count].LogMode=true; else Stage[Count].LogMode=false;\
 Stage[Count].value.color = fabs(Stage[Count].value.color);\
 }






/****** Texture tracking ******/

class ParticleTextureNode : public ANode
{
public:
    char TexName[32];
    DWORD TexHandle;
    CTextureItem *TexItem;
};


class ParticleGroupNode
{
public:

    void *GetRandomArgument(void)
    {
        return ptr[(int)(PRANDFloatPos() * ((float)Items - 0.1f))];
    }
    void *GetArgument(DWORD Index)
    {
        return Index < Items ? ptr[Index] : NULL;
    }

    ParticleGroupNode  *Next;
    ParticleGroupNode(char *name)
    {
        Items = 0;
        strcpy(Name, name);
        Next = NULL;
        Type = GRP_NONE;
    };
    GroupType Type;
    char Name[PS_NAMESIZE];
    DWORD Items;
    void *ptr[MAX_GROUP_ITEMS];
};

ParticleGroupNode  *DrawableParticleSys::Groups, *DrawableParticleSys::LastGroup;


/****** Some ENUMS ******/



/****** Particle Parameter structure ******/


class ParticleParamNode : public ANode
{
public:
    char    name[PS_NAMESIZE];
    int id;
    int     subid;

    int     particleType; // which paritcle class to use for particles????

    float   lifespan, lifespanvariation; // how long a particle lasts in seconds
    int     flags;
    float   lodFactor;

    DWORD GroupFlags;

    PSDrawType drawType;

    int       colorStages;
    timedRGB  color[10];

    int lightStages;
    timedRGB light[10];

    float SizeRandom;
    int sizeStages;
    timedFloat size[10];

    int alphaStages;
    timedFloat alpha[10];


    int gravityStages;
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
    float ParticleEmitterMin[PSMAX_EMITTERS];
    float ParticleEmitterMax[PSMAX_EMITTERS];


    float   bounce;
    float   dieOnGround;

    char     texFilename[PS_NAMESIZE];
    ParticleTextureNode *Texture;

    // BSP particle data
    int bspCTID, bspVisType; // CT Number of BSP
    DrawableBSP *bspObj; // All particles can share 1 BSP.

    PSOrientation orientation;

    ParticleAnimationNode *Animation[2];

    float RotationRateMin, RotationRateMax; // COBRA - RED - Minimum and maximum rotation for the particle, Units are in 2PI/Sec

    float   WindFactor; // RV - I-Hawk
};

/** ParticleSys Flags **/
#define PSF_CHARACTERS "MG"
#define PSF_NONE      (0) // use spacing value a a time (in seconds) instead of distance
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
    ParticleNode(int ID, Tpoint *Pos, Tpoint *Vel = 0, Tpoint *Aim = 0, float fRotationRate = 0);
    ~ParticleNode();

    void Draw(class RenderOTW *renderer, int LOD);
    int IsDead(void);
    void Init(int ID, Tpoint *Pos, Tpoint *Vel = 0, Tpoint *Aim = 0, float fRotationRate = 0);

    float EvalTimedLinLogFloat(int &LastStage, int Count, timedFloat *input);
    float EvalTimedFloat(int &LastStage, int Count, timedFloat *input);
    psRGBA EvalTimedRGBA(int &LastStage, int Count, timedRGBA  *input);
    psRGB EvalTimedRGB(int &LastStage, int Count, timedRGB   *input);
public:
    ParticleParamNode *ppn;

    int    birthTime; //
    int    lastTime; // last time we were updated
    float  lifespan; // in seconds
    float  life; // normalized
    float LastTimeRest[2]; // COBRA - RED - Features for Frame Sequences bitor Modulo ime for animations
    int FrameNr[2]; // COBRA - RED - Features for Frame Sequences bitor rame Number Displayed

    static float      elapsedTime;
    static Trotation *rotation; // runtime computed, shared
    float  plife;     // life value for the previous exec

    int AccelStage, GravityStage;
    Tpoint pos, vel;

    class SubPart *firstSubPart;

    float Rotation, RotationRate;
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
    SubPart(ParticleNode *owner)
    {
        next = 0;
        Alive = true;
    };
    SubPart()
    {
        next = 0;
        Alive = true;
    };
    virtual ~SubPart() {}
    SubPart *next;
    bool Alive;
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner) = 0;
    virtual int IsRunning(ParticleNode *owner)
    {
        return 0;
    }
};

#define SPRMODE_VEC 1

class SubPartMovementOrientation : public SubPart
{
public:
    SubPartMovementOrientation(ParticleNode *owner): SubPart(owner)
    {
        rotation = IMatrix;
    };
    Trotation rotation;
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner);
};


class SubPartPoly : public SubPart
{
public:
    SubPartPoly(ParticleNode *owner);
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner);
    // The Stages of this Poly
    int AlphaStage, ColorStage, SizeStage, LightStage;
    // The Vertices
    ThreeDVertex v0, v1, v2, v3;
    // The textuer Handle
    DWORD TexHandle;
    // The Size random CX
    float SizeRandom;
    CTextureItem *TexItem;
    // the Quad vertices
    D3DDYNVERTEX Quad[4];
};

class SubPartTrail : public SubPart
{
public:
    SubPartTrail(ParticleNode *owner);
    virtual ~SubPartTrail();

    DrawableTrail *trailObj;
    virtual bool   Run(RenderOTW *renderer, ParticleNode *owner);
    virtual int    IsRunning(ParticleNode *owner);
};

class SubPartBSP : public SubPart
{
public:
    SubPartBSP(ParticleNode *owner);
    virtual ~SubPartBSP();
    Trotation    rotation;
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner);
};

class SubPartSound : public SubPart
{
public:
    SubPartSound(ParticleNode *owner);
    virtual ~SubPartSound();
    int        playSound, VolumeStage, PitchStage;
    F4SoundPos soundPos;
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner);
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
    float  RndTime, RndTimeCx;
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner);
    int LastStage;
};

class SubPartEmitter : public SubPart
{
public:
    SubPartEmitter(ParticleNode *owner);
    virtual ~SubPartEmitter();
    virtual bool Run(RenderOTW *renderer, ParticleNode *owner);
    SubPart *emitters;
    int count;
};


/*....................................................................*/




bool SubPartMovementOrientation::Run(RenderOTW *renderer, ParticleNode *owner)
{
    owner->rotation = &rotation;

    float costha, sintha, cosphi, sinphi, cospsi, sinpsi;
    float p, r, y;

    y = (float)atan2(owner->vel.y, owner->vel.x);
    r = 0.0f;
    p = (float)atan2(sqrt(owner->vel.x * owner->vel.x + owner->vel.y * owner->vel.y), owner->vel.z) - 90 * DTR;

    costha = cosf(p);
    sintha = sinf(p);
    cosphi = cosf(r);
    sinphi = sinf(r);
    cospsi = cosf(y);
    sinpsi = sinf(y);

    rotation.M11 = cospsi * costha;
    rotation.M21 = sinpsi * costha;
    rotation.M31 = -sintha;

    rotation.M12 = -sinpsi * cosphi + cospsi * sintha * sinphi;
    rotation.M22 = cospsi * cosphi + sinpsi * sintha * sinphi;
    rotation.M32 = costha * sinphi;

    rotation.M13 = sinpsi * sinphi + cospsi * sintha * cosphi;
    rotation.M23 = -cospsi * sinphi + sinpsi * sintha * cosphi;
    rotation.M33 = costha * cosphi;

    return true;
}

/*....................................................................*/

SubPartTrail::SubPartTrail(ParticleNode *owner) : SubPart(owner)
{
    if (owner->ppn->trailId and owner->elapsedTime)
    {
        trailObj = new DrawableTrail(owner->ppn->trailId, 1);
    }
}

SubPartTrail::~SubPartTrail()
{
    delete trailObj;
}

bool SubPartTrail::Run(RenderOTW *renderer, ParticleNode *owner)
{
    //START_PROFILE("PS_TRAIL");
    trailObj->AddPointAtHead(&owner->pos, 0);
    trailObj->Draw(renderer, 1);
    //STOP_PROFILE("PS_TRAIL");
    return Alive;
}

int SubPartTrail::IsRunning(ParticleNode *owner)
{
    return not trailObj->IsTrailEmpty();
}

/*....................................................................*/

SubPartSound::SubPartSound(ParticleNode *owner) : SubPart(owner)
{
    playSound = 1;
    VolumeStage = PitchStage = 0;
}

SubPartSound::~SubPartSound()
{
}

bool SubPartSound::Run(RenderOTW *renderer, ParticleNode *owner)
{
    if (playSound and owner->elapsedTime)
    {
        float sndVol, sndPitch;
        sndVol = owner->EvalTimedFloat(VolumeStage, owner->ppn->sndVolStages,   owner->ppn->sndVol);
        sndPitch = owner->EvalTimedFloat(PitchStage, owner->ppn->sndPitchStages, owner->ppn->sndPitch);

        soundPos.UpdatePos(owner->pos.x, owner->pos.y, owner->pos.z, owner->vel.x, owner->vel.y, owner->vel.z);
        soundPos.Sfx(owner->ppn->sndId, 0, sndPitch, sndVol);

        if ( not owner->ppn->sndLooped)
            playSound = 0; // so we only play it once
    }/* else

 Alive = soundPos.IsPlaying(owner->ppn->sndId,0);*/
    return Alive;

}

int SubPartSound::IsRunning(ParticleNode *owner)
{
    return soundPos.IsPlaying(owner->ppn->sndId, 0);
}

/*....................................................................*/


SubPartPoly::SubPartPoly(ParticleNode *owner)
{
    ParticleParamNode *ppn = owner->ppn;


    // Setup the Size Cx
    SizeRandom = 1.0f + PRANDFloatPos() * ppn->SizeRandom;

    // initialize variables
    next = 0;
    AlphaStage = ColorStage = SizeStage = LightStage = 0;
    // Link here the Texture and get its U/V Coord

    ParticleTextureNode *pt = ppn->Texture;

    // check if depending on group texture
    if (ppn->GroupFlags bitand GRP_TEXTURE)
    {
        // if depending from a group, ask for a random texture node
        pt = (ParticleTextureNode*)(((ParticleGroupNode*)(ppn->Texture))->GetRandomArgument());
        // Security check
    }

    if (pt)
    {
        // use the PPN texture node
        TexHandle = pt->TexHandle;
        TexItem = pt->TexItem;

        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 0, v0.u, v0.v);
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 1, v1.u, v1.v);
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 2, v2.u, v2.v);
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 3, v3.u, v3.v);

        Quad[0].tu = v0.u, Quad[0].tv = v0.v;
        Quad[1].tu = v1.u, Quad[1].tv = v1.v;
        Quad[2].tu = v2.u, Quad[2].tv = v2.v;
        Quad[3].tu = v3.u, Quad[3].tv = v3.v;

    }
}

bool Sort = false;
bool DXMode = false;
bool Reverse = true;

bool SubPartPoly::Run(RenderOTW *renderer, ParticleNode *owner)
{


    float size, alpha;
    psRGB color, light;
    // these must be computed even while paused, the renderer needs them
    //START_PROFILE("SCALING");
    size = owner->EvalTimedLinLogFloat(SizeStage, owner->ppn->sizeStages,  owner->ppn->size) * SizeRandom;
    //STOP_PROFILE("SCALING");

    // Visibility Check, exit if not in the FOV
    if (TheDXEngine.DX2D_GetDistance((D3DXVECTOR3*)&owner->pos, size) < 0.0f) return Alive;


    //START_PROFILE("SCALING");
    alpha = owner->EvalTimedLinLogFloat(AlphaStage, owner->ppn->alphaStages, owner->ppn->alpha);
    //STOP_PROFILE("SCALING");
    alpha = min(alpha, min(1.0f, owner->ppn->visibleDistance / (DrawableParticleSys::cameraDistance * DrawableParticleSys::cameraDistance / 1000.0f)));

    if (alpha < 0.01f) return Alive;

    // Calculate a distance CX for far view
    float DistCx = size / (DrawableParticleSys::cameraDistance * TheDXEngine.LODBiasCx());

    // if less than 0.003 arbitrary
    if (DistCx < 0.005f)
    {

        // if less than 0.0002 arbitrary do not draw
        if (DistCx < 0.0005f) return Alive;

        DWORD Alpha = FloatToInt32(255.0f * alpha /*RESCALE(DistCx, 0.001f, 0.005f, 0.3f, 1.0f)*/);
        DWORD Color = 0x00202020 bitor (Alpha << 24);
        TheDXEngine.Draw3DPoint((D3DVECTOR*)&owner->pos, Color);

        return Alive;
    }

    //if(DrawableParticleSys::cameraDistance>owner->ppn->visibleDistance)
    // return ;


    //START_PROFILE("SCALING");
    color = owner->EvalTimedRGB(ColorStage, owner->ppn->colorStages, owner->ppn->color);
    light = owner->EvalTimedRGB(LightStage, owner->ppn->lightStages, owner->ppn->light);
    //STOP_PROFILE("SCALING");


    //START_PROFILE("PS DRAW");
    Tpoint os, pv;
    // COBRA - RED - Rotation, get the radius and angle
    mlTrig RotCx;
    mlSinCos(&RotCx, owner->Rotation);
    RotCx.cos *= size;
    RotCx.sin *= size;

    if (DXMode)
    {


        DWORD Alpha = F_TO_A(alpha);
        DWORD HiColor = F_TO_ARGB(alpha, color.r, color.g, color.b);
        DWORD LoColor = F_TO_ARGB(alpha, (color.r * 0.68f), (color.g * 0.68f), (color.b * 0.68f));
        DWORD LiteColor = F_TO_RGB(light.r, light.g, light.b);

        Quad[0].pos.x = Quad[1].pos.x = Quad[2].pos.x = Quad[3].pos.x = 0.f;
        Quad[0].pos.y = RotCx.cos;
        Quad[0].pos.z = RotCx.sin;
        Quad[1].pos.y = RotCx.sin;
        Quad[1].pos.z = -RotCx.cos;
        Quad[2].pos.y = -RotCx.cos;
        Quad[2].pos.z = -RotCx.sin;
        Quad[3].pos.y = -RotCx.sin;
        Quad[3].pos.z = RotCx.cos;

        Quad[0].dwColour = Quad[1].dwColour = HiColor;
        Quad[2].dwColour = Quad[3].dwColour = LoColor;
        Quad[0].dwSpecular = Quad[1].dwSpecular = Quad[2].dwSpecular = Quad[3].dwSpecular = LiteColor;
        // draw with BillBoard and declare as VISIBLE, the Visibility test was at function entry point ( DX2D_GetDistance()>0)
        TheDXEngine.DX2D_AddQuad(LAYER_AUTO, POLY_BB bitor POLY_VISIBLE, (D3DXVECTOR3*)&owner->pos, Quad, size, TexHandle);

    }
    else
    {



        renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
        ParticleParamNode *PPN = owner->ppn;

        renderer->TransformPointToView(&owner->pos, &pv);

        os.x = 0.f;
        os.y = RotCx.cos;
        os.z = RotCx.sin;
        renderer->TransformBillboardPoint(&os, &pv, &v0);

        os.x = 0.f;
        os.y = RotCx.sin;
        os.z = -RotCx.cos;
        renderer->TransformBillboardPoint(&os, &pv, &v1);

        os.x = 0.f;
        os.y = -RotCx.cos;
        os.z = -RotCx.sin;
        renderer->TransformBillboardPoint(&os, &pv, &v2);

        os.x =  0.f;
        os.y = -RotCx.sin;
        os.z = RotCx.cos;
        renderer->TransformBillboardPoint(&os, &pv, &v3);

        v0.q = v0.csZ * Q_SCALE;
        v1.q = v1.csZ * Q_SCALE;
        v2.q = v2.csZ * Q_SCALE;
        v3.q = v3.csZ * Q_SCALE;

        /* if(PS_NVG or PS_TV)
         {
         v0.r = v1.r = v2.r = v3.r = 0.f;
         v0.g = v1.g = v2.g = v3.g = .4f;
         v0.b = v1.b = v2.b = v3.b = 0.f;
         }
         else
         { */
        v0.r = v1.r = gParticleSysLitColor.r * color.r + light.r;
        v2.r = v3.r = gParticleSysLitColor.r * color.r * .68f + light.r;

        v0.g = v1.g = gParticleSysLitColor.g * color.g + light.g;
        v2.g = v3.g = gParticleSysLitColor.g * color.g * .68f + light.g;

        v0.b = v1.b = gParticleSysLitColor.b * color.b + light.b;
        v2.b = v3.b = gParticleSysLitColor.b * color.b * .68f + light.b;
        // }


        if (TexHandle)
        {
            v0.a = v1.a = v2.a = v3.a = alpha;
            renderer->context.SelectTexture1(TexHandle); // COBRA - RED - Simple Texture
            renderer->DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL, (g_nGfxFix > 0));
        }

        if (PPN->Animation[0])  // COBRA - RED - Frames Sequence
        {
            renderer->context.SelectTexture1(PPN->Animation[0]->Run(owner->FrameNr[0],
                                             owner->LastTimeRest[0], owner->elapsedTime, owner->pos, alpha)); // Selects It
            v0.a = v1.a = v2.a = v3.a = alpha;
            renderer->DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL, (g_nGfxFix > 0));
        }

        if (PPN->Animation[1])  // COBRA - RED - Frames Sequence
        {
            renderer->context.SelectTexture1(PPN->Animation[1]->Run(owner->FrameNr[1],
                                             owner->LastTimeRest[1], owner->elapsedTime, owner->pos, alpha)); // Selects It
            v0.a = v1.a = v2.a = v3.a = alpha;
            renderer->DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL, (g_nGfxFix > 0));
        }
    }

    //STOP_PROFILE("PS DRAW");

    return Alive;

}

/*....................................................................*/

SubPartBSP::SubPartBSP(ParticleNode *owner) : SubPart(owner)
{
}

SubPartBSP::~SubPartBSP()
{
}

bool SubPartBSP::Run(RenderOTW *renderer, ParticleNode *owner)
{
    owner->ppn->bspObj->orientation = *(owner->rotation);
    owner->ppn->bspObj->SetPosition(&owner->pos);
    owner->ppn->bspObj->Draw(renderer);
    return Alive;

}
/*....................................................................*/

SubPartEmitter::SubPartEmitter(ParticleNode *owner) : SubPart(owner)
{
    emitters = 0;
    count = 0;

    if (owner->ppn->emitOneRandomly)
    {
        for (count = 0; owner->ppn->emitter[count].stages and count < PSMAX_EMITTERS; count++);

        if (count)
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

        float Test = PRANDFloatPos();

        for (count = 0; owner->ppn->emitter[count].stages and count < PSMAX_EMITTERS; count++)
        {
            // Ok... check the probability to enable this emitter...
            if (Test >= owner->ppn->ParticleEmitterMin[count] and Test < owner->ppn->ParticleEmitterMax[count])
            {
                SubPart *sub = new SubEmitter(owner, &owner->ppn->emitter[count]);
                sub->next = emitters;
                emitters = sub;
            }
        }
    }

}

SubPartEmitter::~SubPartEmitter()
{
    SubPart *emit;

    while (emit = emitters)
    {
        emitters = emit->next;
        delete emit;
    }
}

bool SubPartEmitter::Run(RenderOTW *renderer, ParticleNode *owner)
{

    if (owner->elapsedTime)
    {
        // Default to die
        Alive = false;

        SubPart *Sub = emitters, *Last = NULL, *Next;

        while (Sub)
        {
            // get next item
            Next = Sub->next;

            // Run the Sub part
            if ( not Sub->Run(renderer, owner))
            {
                // if it's dead remove it from list
                if (Last) Last->next = Next;
                else emitters = Next;

                // and kill it
                delete Sub;
            }
            else
            {
                // Ok, a subEmitter running, we r still alive
                Alive = true;
                Last = Sub;
            }

            Sub = Next;
        }

    }

    return Alive;
}


SubEmitter::SubEmitter(ParticleNode *owner, ParticleEmitterParam *EP) : SubPart(owner)
{
    rollover  = 0;
    pep       = EP;
    LastStage = 0;
    // Get the randomic time CX
    RndTimeCx = EP->TimeVariation;
    // set the Random Time for 1st emission
    RndTime = PRANDFloat() * RndTimeCx;

}

SubEmitter::~SubEmitter()
{
}

bool SubEmitter::Run(RenderOTW *renderer, ParticleNode *owner)
{
    //START_PROFILE("PARTICLE EMITTER");

    Tpoint epos = owner->pos;
    float qty = 0;

    switch (pep->mode)
    {
        case PSEM_IMPACT:
        {
            float GroundLevel = DrawableParticleSys::groundLevel;

            if (owner->pos.z >= GroundLevel)
            {
                epos.z = GroundLevel;
                qty += pep->rate[0].value;
                // ok... done... then die...
                Alive = false;
            }
        }
        break;

        case PSEM_EARTHIMPACT:
        {
            float GroundLevel = DrawableParticleSys::groundLevel;

            if (owner->pos.z >= GroundLevel)
            {
                int gtype = OTWDriver.GetGroundType(owner->pos.x, owner->pos.y);
                epos.z = GroundLevel;


                if ( not (gtype == COVERAGE_WATER or gtype == COVERAGE_RIVER))
                {
                    qty += pep->rate[0].value;
                    // ok... done... then die...
                    Alive = false;
                }
            }
        }
        break;

        case PSEM_WATERIMPACT:
        {
            float GroundLevel = DrawableParticleSys::groundLevel;

            if (owner->pos.z >= GroundLevel)
            {
                int gtype = OTWDriver.GetGroundType(owner->pos.x, owner->pos.y);
                epos.z = GroundLevel;


                if ((gtype == COVERAGE_WATER or gtype == COVERAGE_RIVER))
                {
                    qty += pep->rate[0].value;
                    // ok... done... then die...
                    Alive = false;
                }
            }
        }
        break;

        case PSEM_ONCE:
        {
            while (LastStage < pep->stages and (pep->rate[LastStage].time + RndTime) <= owner->life)
            {
                // Add quantity and go to next stage
                qty += pep->rate[LastStage++].value;
                // Ok, recalculate random timing for next stage
                RndTime = PRANDFloat() * RndTimeCx;
            }

            // if gone past last stage, die..
            if (LastStage >= pep->stages) Alive = false;

        }
        break;

        case PSEM_PERSEC:
            qty = owner->EvalTimedFloat(LastStage, pep->stages, pep->rate);
            qty = qty * owner->elapsedTime + rollover;

            // if gone past last stage, die..
            if (LastStage >= pep->stages) Alive = false;
    }


    while (qty >= 1)
    {
        float v;
        Tpoint w, pos, aim, subvel;

        pep->domain.GetRandomPosition(&w);
        MatrixMult(owner->rotation, &w, &pos);

        pep->target.GetRandomDirection(&w);
        MatrixMult(owner->rotation, &w, &aim);

        v = pep->velocity + pep->velVariation * NRAND;

        subvel.x = aim.x * v;
        subvel.y = aim.y * v;
        subvel.z = aim.z * v;

        subvel.x += owner->vel.x;
        subvel.y += owner->vel.y;
        subvel.z += owner->vel.z;

        pos.x += epos.x;
        pos.y += epos.y;
        pos.z += epos.z;

        ParticleNode *n = new ParticleNode(pep->id, &pos, &subvel, &aim);
        n->InsertAfter((ANode *)owner);
        qty -= 1.0f;
    }

    rollover = qty;

    //STOP_PROFILE("PARTICLE EMITTER");
    return Alive;

}

/*....................................................................*/


// COBRA - RED - This function choose the right frame for an animation and updates
// caller animation parameters
GLint ParticleAnimationNode::Run(int &Frame, float &TimeRest, float Elapsed, Tpoint &pos, float &alpha)
{
    if ((Elapsed + TimeRest) >= Fps)  //
    {
        Frame++; // If time for a frame elapsed next frame

        if (Frame >= NFrames)   // Animation update
        {
            if (Flags bitand ANIM_LOOPING) Frame = 0; // * LOOPING CHECK *
            else Frame = NFrames - 1;
        }

        if (Frame == -1)Frame = 0; // Limit check to avoid CTD

        TimeRest = Elapsed - Fps; // Keep remaining time
    }
    else   // else, if not elapsed time for a frame
    {
        TimeRest += Elapsed; // just update Frame Time Counter
    }

    if (Flags bitand ANIM_DNVIEW)  // * DOWN VIEW ALPHA *
    {
        float cx = sqrt((pos.x - ObserverPosition.x) * (pos.x - ObserverPosition.x) //
                        + (pos.y - ObserverPosition.y) * (pos.y - ObserverPosition.y)); // ground distance from object
        cx = (float)atan2(ObserverPosition.z - pos.z, cx); // Angle CX
        alpha *= abs(cos(cx));
    }

    if (Flags bitand ANIM_UPVIEW)  // * DOWN VIEW ALPHA *
    {
        float cx = sqrt((pos.x - ObserverPosition.x) * (pos.x - ObserverPosition.x) //
                        + (pos.y - ObserverPosition.y) * (pos.y - ObserverPosition.y)); // ground distance from object
        cx = (float)atan2(ObserverPosition.z - pos.z, cx); // Angle CX
        alpha *= abs(sin(cx));
    }


    TextureLink *Tex = (TextureLink*)Sequence; // Gets the Base Frame
    Tex += Frame; // calculates the Frame Position
    return(Tex->TexHandle); // Selects It
}


/**** some useful crap ****/
#define MORPH(n,src,dif) ( (src) + ( (dif) * (n) ) )
#define ATTRMORPH(ATR) attrib.ATR = MORPH(life, params->birthAttrib.ATR, params->deathAttribDiff.ATR)
/**** end of useful crap ****/

ParticleNode::ParticleNode(int ID, Tpoint *Pos, Tpoint *Vel, Tpoint *Aim, float fRotationRate)
{
    Init(ID, Pos, Vel, Aim, RotationRate);
}


void ParticleNode::Init(int ID, Tpoint *Pos, Tpoint *Vel, Tpoint *Aim, float fRotationRate)
{
    lastTime = TheTimeManager.GetClockTime();
    firstSubPart = 0;

    ppn = PPN[ID];

    pos = *Pos;

    if (Vel)
    {
        float v = ppn->velInherit;
        vel.x = Vel->x * v;
        vel.y = Vel->y * v;
        vel.z = Vel->z * v;
    }
    else
        vel.x = vel.y = vel.z = 0;

    if (Aim)
    {
        float v;

        v = ppn->velInitial + ppn->velVariation * NRAND;
        vel.x += Aim->x * v;
        vel.y += Aim->y * v;
        vel.z += Aim->z * v;

    }

    // note, subparts are execute in the opposite order as they are attached
    if (ppn->drawType == PSDT_POLY)
    {
        SubPart *sub = new SubPartPoly(this);
        sub->next = firstSubPart;
        firstSubPart = sub;
    }

    if (ppn->bspObj)
    {
        SubPart *sub = new SubPartBSP(this);
        sub->next = firstSubPart;
        firstSubPart = sub;
    }

    if (ppn->sndId)
    {
        SubPart *sub = new SubPartSound(this);
        sub->next = firstSubPart;
        firstSubPart = sub;
    }

    if (ppn->trailId >= 0)
    {
        SubPart *sub = new SubPartTrail(this);
        sub->next = firstSubPart;
        firstSubPart = sub;
    }

    if (ppn->emitter[0].stages) // we have atleast 1 emitter
    {
        SubPart *sub = new SubPartEmitter(this);
        sub->next = firstSubPart;
        firstSubPart = sub;
    }

    /*int l;
    for(l = 0; ppn->emitter[l].stages and l<PSMAX_EMITTERS; l++)
    {
     SubPart *sub = new SubPartEmitter(this);
     sub->next = firstSubPart;
     firstSubPart = sub;
    }*/


    switch (ppn->orientation)
    {
        case PSO_MOVEMENT:
        {
            SubPart *sub = new SubPartMovementOrientation(this);
            sub->next = firstSubPart;
            firstSubPart = sub;
        }
        break;

        default :
            break;
    }

    lastTime = birthTime = TheTimeManager.GetClockTime();
    lifespan  = ppn->lifespan + ppn->lifespanvariation * NRAND;
    plife = life = 0.0f;
    FrameNr[0] = FrameNr[1] = 0;
    LastTimeRest[0] = LastTimeRest[1] = 0;
    GravityStage = AccelStage = 0;
    // No rotation at start
    Rotation = 0.0f;
    // Randomic rate btw Max and Min
    RotationRate = (PRANDFloatPos() * (ppn->RotationRateMax - ppn->RotationRateMin) + ppn->RotationRateMin);
    // randomic Sign
    RotationRate *= (PRANDFloat() >= 0.0f) ? 1.0f : -1.0f;
}

#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))


// This function evaluates linearly, or logaritmically if the incoming value is negative
float ParticleNode::EvalTimedLinLogFloat(int &LastStage, int Count, timedFloat *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return fabs(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return fabs(input[Count - 1].value);

    // return the result
    if (input[LastStage + 1].value >= 0) return (life - input[LastStage].time) * input[LastStage].K + fabs(input[LastStage].value);

    //return ( RESCALE(life, input[LastStage].time, input[LastStage+1].time, fabs(input[LastStage].value), fabs(input[LastStage+1].value)));

    // get the time difference
    float Time = (input[LastStage + 1].time - input[LastStage].time);
    // Calculate the Time Position btw 1 and 0
    Time = (life - input[LastStage].time) / Time;
    // scale to 100 to get a Log10 Array index
    int   Idx = (int)(Time * LOG10_ARRAY_ITEMS);

    // limit check
    if (Idx >= LOG10_ARRAY_ITEMS) Idx = LOG10_ARRAY_ITEMS - 1;

    // get the Value difference btw In and Out
    float RelValue = fabs(input[LastStage].value) - fabs(input[LastStage + 1].value);
    // return the value
    return RelValue * DrawableParticleSys::Log10Array[Idx] + fabs(input[LastStage + 1].value);
}


float ParticleNode::EvalTimedFloat(int &LastStage, int Count, timedFloat *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // return the result
    return (life - input[LastStage].time) * input[LastStage].K + input[LastStage].value;
    //return ( RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value, input[LastStage+1].value));
}

psRGBA ParticleNode::EvalTimedRGBA(int &LastStage, int Count, timedRGBA *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    psRGBA retVal;
    retVal.r = RESCALE(life, input[LastStage].time, input[LastStage + 1].time, input[LastStage].value.r, input[LastStage + 1].value.r);
    retVal.g = RESCALE(life, input[LastStage].time, input[LastStage + 1].time, input[LastStage].value.g, input[LastStage + 1].value.g);
    retVal.b = RESCALE(life, input[LastStage].time, input[LastStage + 1].time, input[LastStage].value.b, input[LastStage + 1].value.b);
    retVal.a = RESCALE(life, input[LastStage].time, input[LastStage + 1].time, input[LastStage].value.a, input[LastStage + 1].value.a);

    return (retVal);
}

psRGB ParticleNode::EvalTimedRGB(int &LastStage, int Count, timedRGB *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    psRGB retVal;
    /*retVal.r=RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value.r, input[LastStage+1].value.r);
    retVal.g=RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value.g, input[LastStage+1].value.g);
    retVal.b=RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value.b, input[LastStage+1].value.b);*/

    float elapsed = life - input[LastStage].time;
    retVal.r = elapsed * input[LastStage].K.r + input[LastStage].value.r;
    retVal.g = elapsed * input[LastStage].K.g + input[LastStage].value.g;
    retVal.b = elapsed * input[LastStage].K.b + input[LastStage].value.b;
    return (retVal);
}

ParticleNode::~ParticleNode()
{
    SubPart *sub = firstSubPart;

    /* if(ppn->Frames){ // COBRA - RED - If sequence
     free ((void*)ppn->texture); // delete Frames List from memory
     }*/
    while (sub)
    {
        SubPart *next = sub->next;
        delete sub;
        sub = next;
    }
}

int ParticleNode::IsDead(void)
{
    if (life > 1.0 or life < 0.0)
    {
        SubPart *sub = firstSubPart;

        while (sub)
        {
            if (sub->IsRunning(this))
                return 0;

            sub = sub->next;
        }

        return 1;
    }

    return 0;
}

void ParticleNode::Draw(class RenderOTW *renderer, int LOD)
{
    if (lifespan <= 0) return;

    // COUNT_PROFILE("PARTICLES");

    float age;
    DWORD curTime;

    curTime     = TheTimeManager.GetClockTime();
    age         = (float)(curTime - birthTime) * .001f;
    elapsedTime = (float)(curTime - lastTime) * .001f;
    life        = age / lifespan;

    if (life > 1.0f and plife < 1.0f)
    {
        life = 1.0f; // just to make sure we do the last stages
    }
    else // COBRA - RED - If returns before die, will not append new emitters or nodes
    {
        if (life > 1.0f) // waiting to die
            return;
    }

    // COBRA - RED - End
    if (elapsedTime)
    {
        // we only need to run this if some time has elapsed
        float gravity, accel;
        lastTime = (int)curTime;

        gravity = EvalTimedFloat(GravityStage, ppn->gravityStages, ppn->gravity);
        accel = EvalTimedFloat(AccelStage, ppn->accelStages, ppn->accel);


        if (ppn->simpleDrag)
        {
            float Drag_x_Time = ppn->simpleDrag * elapsedTime; // COBRA - RED - Cached same Value

            vel.x -= (vel.x - DrawableParticleSys::winddx) * Drag_x_Time;
            vel.y -= (vel.y - DrawableParticleSys::winddy) * Drag_x_Time;
            vel.z -= vel.z * Drag_x_Time;
            // Go to reach the wind speed
            //vel.x+=(DrawableParticleSys::winddx - vel.x)/10 * elapsedTime;
            //vel.y+=(DrawableParticleSys::winddy - vel.y)/10 * elapsedTime;

        }

        // update velocity values
        vel.z += (32 * elapsedTime) * gravity;

        if (accel)
        {
            float fps = (accel * elapsedTime);
            float d = sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
            fps = d + fps;

            if (fps < 0)
                fps = 0;

            if (d)
            {
                vel.x = (vel.x / d) * fps;
                vel.y = (vel.y / d) * fps;
                vel.z = (vel.z / d) * fps;
            }
        }


        pos.x += vel.x * elapsedTime;
        pos.y += vel.y * elapsedTime;
        pos.z += vel.z * elapsedTime;

        float GroundLevel = DrawableParticleSys::groundLevel = OTWDriver.GetGroundLevel(pos.x, pos.y);

        if (pos.z >= GroundLevel)
        {
            pos.z = GroundLevel;
            vel.z *= -ppn->bounce;

            if (ppn->dieOnGround and lifespan > age) // this will make the particle die
            {
                // will also run the emitters

                lifespan = age;
            }
        }


        if (pos.z == GroundLevel)
        {
            float fps = (ppn->groundFriction * elapsedTime);
            float d = sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
            fps = d + fps;

            if (fps < 0)
                fps = 0;

            if (d)
            {
                vel.x = (vel.x / d) * fps;
                vel.y = (vel.y / d) * fps;
                //vel.z += (vel.z / d) * fps;
            }
        }

        // COBRA - RED - The rotation Stuff
        Rotation += RotationRate * elapsedTime;

        // end of the "Exec" section
    }

#ifdef DEBUG_PS_ID

    if (DrawableBSP::drawLabels/* and ppn->name[0]=='$'*/)
    {
        // Now compute the starting location for our label text
        ThreeDVertex labelPoint;
        float x, y;
        renderer->TransformPoint(&pos, &labelPoint);

        if (labelPoint.clipFlag == ON_SCREEN)
        {
            x = labelPoint.x - 32; // Centers text
            y = labelPoint.y - 12; // Place text above center of object
            renderer->SetColor(ppn->name[0] == '$' ?  0xff0000ff : 0xffff0000);
            renderer->SetFont(2);
            renderer->ScreenText(x, y, ppn->name);
        }
    }

#endif

    rotation = &psIRotation;

    SubPart *Sub = firstSubPart, *Last = NULL, *Next;

    while (Sub)
    {

        // get next item
        Next = Sub->next;

        // Run the Sub part
        if ( not Sub->Run(renderer, this))
        {
            // if it's dead remove it from list
            if (Last) Last->next = Next;
            else firstSubPart = Next;

            // and kill it
            delete Sub;
        }
        else
        {
            Last = Sub;
        }

        Sub = Next;
    }

    plife = life;

}




static Tcolor gLight;

DrawableParticleSys::DrawableParticleSys(int particlesysType, float scale)
    : DrawableObject(scale)
{

    ShiAssert(particlesysType >= 0);

    // Store the particlesys type
    type = particlesysType;
    dpsNode.owner = this;
    paramList.Lock();
    dpsList.AddHead(&dpsNode);
    paramList.Unlock();

    // Set to position 0.0, 0.0, 0.0;
    position.x = 0.0F;
    position.y = 0.0F;
    position.z = 0.0F;


    headFPS.x = 0.0f;
    headFPS.y = 0.0f;
    headFPS.z = 0.0f;

    Groups = LastGroup = NULL;

    Something = 0;
}

DrawableParticleSys::~DrawableParticleSys(void)
{
    // Delete this object's particlesys
    paramList.Lock();
    dpsNode.Remove();

    ParticleNode *n;

    while (n = (ParticleNode *)particleList.RemHead())
    {
        delete n;
    }

    paramList.Unlock();
}

int  DrawableParticleSys::HasParticles(void)
{
    if (particleList.GetHead())
        return 1;

    return 0;
}


/***************************************************************************\
    Add a point to the list which define this segmented particlesys.
\***************************************************************************/
void DrawableParticleSys::AddParticle(Tpoint *worldPos, Tpoint *v)
{
    AddParticle(type, worldPos, v);
}


void DrawableParticleSys::AddParticle(int Id, Tpoint *worldPos, Tpoint *v)
{
    DWORD now;
    now = TheTimeManager.GetClockTime();

    // COBRA - RED - New FPS Filter
    // Cobra - Keep the Particle list cleaned up
    /* if ((now >= TimeToPurge) and PurgeTimeInc)
     {
     TimeToPurge = (now + PurgeTimeInc);
     CleanParticleList();
     }
     // Cobra - Purge the Particle list
     else if ((now >= TimeToPurgeAll) and PurgeTimeInc)
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

    float fpsTest = OTWDriver.GetFPS(); // Get the Actal FPS

    // Calculates the filter level ... the more near fps limit, the lower the filter level
    float FilterLevel = (fpsTest - g_nPSKillFPS) * (MAX_PARTICLE_FILTER_LEVEL / (g_nPSKillFPS * 1.5f - g_nPSKillFPS + 1.0f));

    // if less that 0 ( already under fps limit) then settle at 0 ( no New particle )
    if (FilterLevel < 0) FilterLevel = 0;

    // here the counter of the Filter is updated, the filter works Adding 'Filter' Particles out of the Counter
    ParticleFilterCount = (++ParticleFilterCount) % (MAX_PARTICLE_FILTER_LEVEL + 1);

    // Ok, now we have how much is counting the counter, if its counting not more that the Filter Level
    // Add this particle, if not bail out
#ifdef USE_NEW_PS
    PS_AddParticle((int)PPN[Id], worldPos, v);
#else

    if (ParticleFilterCount < (WORD)FilterLevel)
    {
        ParticleNode *n = new ParticleNode(Id, worldPos, v);
        particleList.AddHead(n);
    }

#endif
    //REPORT_VALUE("P.Filter%",(__int64)((FilterLevel>=MAX_PARTICLE_FILTER_LEVEL)?0:((MAX_PARTICLE_FILTER_LEVEL-FilterLevel)/MAX_PARTICLE_FILTER_LEVEL*100)));
}

void DrawableParticleSys::SetHeadVelocity(Tpoint *FPS)
{
    headFPS = *FPS;
}


inline DWORD ROL(DWORD n)
{
    _asm
    {
        rol n, 1;
    }
    return n;
}

void DrawableParticleSys::Draw(class RenderOTW *renderer, int LOD)
{

    //START_PROFILE("OLD PS TIME");

    // Setup the Renderer
    PS_Renderer = renderer;

    ParticleNode *n;
    float xx, yy, zz, rx, ry, rz;

    // Stored the renderer Position
    rx = renderer->X();
    ry = renderer->Y();
    rz = renderer->Z();

    paramList.Lock();

    n = (ParticleNode *)particleList.GetHead();

    // COBRA - DX - Setup the Squares in the 2D DX Engine for PS Draws
    TheDXEngine.DX2D_SetupSquareCx(1.0f, 1.0f);


    if (n)
    {
        groundLevel = OTWDriver.GetGroundLevel(n->pos.x, n->pos.y);
        mlTrig trigWind;
        float wind;

        // current wind
        // mlSinCos(&trigWind, TheWeather->GetWindHeading(&n->pos));
        // wind =  TheWeather->GetWindSpeedFPS(&n->pos);
        mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&n->pos));
        wind = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&n->pos);
        wind *= n->ppn->WindFactor * 0.5f ; // RV - I-Hawk
        winddx = trigWind.cos * wind;
        winddy = trigWind.sin * wind;

        xx = n->pos.x - rx;
        yy = n->pos.y - ry;
        zz = n->pos.z - rz;

        cameraDistance = sqrt(xx * xx + yy * yy + zz * zz);

        while (n)
        {
            //COUNT_PROFILE("PARTICLES");
            n->Draw(renderer, LOD);
            // we have to get the next node after Exec, because our emitters
            // will add nodes directly after themselves.  Getting the next node
            // pointer before Exec will cause one newly emitted node to be skipped
            // in this loop, which then causes rendering bugs due to not having
            // data that was Exec'd yet.
            ParticleNode *n2 = (ParticleNode *)n->GetSucc();

            // Cobra - Moved the node killer to start of mission (otwloop.cpp)
            // also in DrawableParticleSys::AddParticle()
            if ((n->IsDead()) /* and not PurgeTimeInc*/) // COBRA - RED - If no1 updates PurgeTimeInc,
            {
                // when it could be 0...???
                n->Remove();
                delete n;
            }

            n = n2;
        }
    }

    paramList.Unlock();
    //STOP_PROFILE("OLD PS TIME");

}


void DrawableParticleSys::Exec(void)
{
    return;
}

void DrawableParticleSys::ClearParticles(void)
{
    ParticleNode *n;

    while (n = (ParticleNode *)particleList.RemHead())
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
    memcpy(&gParticleSysLitColor, color, sizeof(Tcolor));
}



ParticleGroupNode *DrawableParticleSys::FindGroupNode(char *fn)
{
    ParticleGroupNode *g;

    g = (ParticleGroupNode *) DrawableParticleSys::Groups;

    while (g)
    {
        if ( not stricmp(g->Name, fn))
        {
            return(g);
        }

        g = g->Next;
    }

    return NULL;
}


ParticleTextureNode *DrawableParticleSys::FindTextureNode(char *fn)
{
    ParticleTextureNode *n;

    n = (ParticleTextureNode *)textureList.GetHead();

    while (n)
    {
        if (stricmp(n->TexName, fn) == 0)
        {
            return(n);
        }

        n = (ParticleTextureNode *)n->GetSucc();
    }

    return 0;
}

ParticleTextureNode *DrawableParticleSys::GetTextureNode(char *fn)
{
    ParticleTextureNode *n;

    n = FindTextureNode(fn);

    if (n) return n;

    n = new ParticleTextureNode;
    strcpy(n->TexName, fn);
    textureList.AddHead(n);
    return n;
}


// COBRA - RED - Adds a Frame References List taht is stored in the Node of a Particle Item
ParticleTextureNode *DrawableParticleSys::GetFramesList(char *fn, int Frames)
{
    char Name[PS_NAMESIZE];

    static TextureLink *List, *BaseList; // Item pointer in the List

    BaseList = (TextureLink*)malloc(sizeof(TextureLink) * Frames); // is Allocated for N Frames
    List = BaseList;

    if ( not List) return 0; // Out of Memory

    for (int a = 0; a < Frames; a++)   // for each Frame
    {
        strncpy(Name, fn, sizeof(Name)); // creates each frame name
        strtok(Name, "., "); // till extension
        sprintf(&Name[strlen(Name)], "_%03d", a); // with number appended
        List->TexHandle = TheDXEngine.GetTextureHandle(Name);
        List->TexItem = TheDXEngine.DX2D_GetTextureItem(Name);
        List++; // Each Pointer is Updated
    }

    return((ParticleTextureNode*)BaseList); // and returns it
}


// COBRA - RED - Find a named animation node or returns NULL pointer
ParticleAnimationNode *DrawableParticleSys::FindAnimationNode(char *fn)
{
    ParticleAnimationNode *n;

    n = (ParticleAnimationNode *)AnimationsList.GetHead();

    while (n)
    {
        if (stricmp(n->AnimationName, fn) == 0)
        {
            return(n);
        }

        n = (ParticleAnimationNode *)n->GetSucc();
    }

    return 0;
}

// COBRA - RED - returns and eventually creates a named animation node
ParticleAnimationNode *DrawableParticleSys::GetAnimationNode(char *fn)
{
    ParticleAnimationNode *n;

    n = FindAnimationNode(fn);

    if (n) return n;

    n = new ParticleAnimationNode;
    memset(n, 0x00, sizeof(ParticleAnimationNode));
    strcpy(n->AnimationName, fn);
    AnimationsList.AddHead(n);
    return n;
}




DXContext *psContext = 0;

void DrawableParticleSys::SetupTexturesOnDevice(DXContext *rc)
{
    psContext = rc;

    ParticleTextureNode *n;

    n = (ParticleTextureNode *)textureList.GetHead();

    while (n)
    {
        n->TexHandle = TheDXEngine.GetTextureHandle(n->TexName);
        n->TexItem = TheDXEngine.DX2D_GetTextureItem(n->TexName);
        n = (ParticleTextureNode *)n->GetSucc();
    }

    ParticleParamNode *ppn;

    ppn = (ParticleParamNode *)paramList.GetHead();

    Tpoint    pos;
    Trotation rot = IMatrix;

    while (ppn)
    {
        if (ppn->bspCTID)
        {
            ppn->bspObj   = new DrawableBSP(Falcon4ClassTable[ppn->bspCTID].visType[ppn->bspVisType],
                                            &pos,
                                            &rot);
        }

        ppn = (ParticleParamNode *)ppn->GetSucc();
    }


}

void DrawableParticleSys::ReleaseTexturesOnDevice(DXContext *rc)
{
    psContext = 0;

    ParticleTextureNode *n;

    n = (ParticleTextureNode *)textureList.GetHead();

    while (n)
    {
        n->TexHandle = 0;
        n->TexItem = NULL;
        n = (ParticleTextureNode *)n->GetSucc();
    }

    ParticleParamNode *ppn;

    ppn = (ParticleParamNode *)paramList.GetHead();

    while (ppn)
    {
        if (ppn->bspObj)
        {
            delete ppn->bspObj;
            ppn->bspObj = 0;
        }

        ppn = (ParticleParamNode *)ppn->GetSucc();
    }


}

/*******************************************************************/

static const char TRAILFILE[] = "particlesys.ini";

extern FILE* OpenCampFile(char *filename, char *ext, char *mode);

int MatchString(char *arg, char **q)
{
    int i = 0;

    while (*q)
    {
        i++;

        if (stricmp(arg, *q) == 0)
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

    while (ptn = (ParticleTextureNode *)textureList.RemHead())
    {
        delete ptn;
    }

    while (ppn = (ParticleParamNode *)paramList.RemHead())
    {
        if (ppn->bspObj)
            delete ppn->bspObj;

        delete ppn;
    }

    delete [] PPN;

    PPN = 0;

#ifdef USE_NEW_PS
    // remove all Lists
    PS_ListsRelease();
#endif

}

char *trim(char *in)
{
    char *q;

    if ( not in) return 0;

    while (*in == ' ' or *in == '/t')
    {
        in++;
    }

    q = in;

    while (*q not_eq 0)
        q++;

    q--;

    while (*q == ' ' or *q == '/t')
    {
        *q = 0;
        q--;
    }

    return(in);
}


char *DrawableParticleSys::GetErrorMessage(void)
{
    return(ErrorMessage);
}

// COBRA - RED - Returns False if any Failure
bool DrawableParticleSys::LoadParameters(void)
{
    return PS_LoadParameters();
}


int DrawableParticleSys::IsValidPSId(int id)
{
    if (id < PPNCount and PPN[id])
    {
        return 1;
    }

    return 0;
}

int DrawableParticleSys::GetNameId(char *name)
{
    int l;

    for (l = 0; l < PPNCount; l++)
    {
#ifdef USE_NEW_PS

        if (PPN[l] and PS_PPN[(DWORD)PPN[l]].name and stricmp(name, PS_PPN[(DWORD)PPN[l]].name) == 0) return l;

#else

        if (PPN[l] and PPN[l]->name and stricmp(name, PPN[l]->name) == 0) return l;

#endif
    }

    return 0;
}


/***********************************************************************

 The ParticleDomain

***********************************************************************/

void ParticleDomain::GetRandomPosition(Tpoint *p)
{
    p->x = NRAND * sphere.size.x + sphere.pos.x;
    p->y = NRAND * sphere.size.y + sphere.pos.y;
    p->z = NRAND * sphere.size.z + sphere.pos.z;
}

void ParticleDomain::GetRandomDirection(Tpoint *p)
{
    GetRandomPosition(p);

    float d = sqrt(p->x * p->x + p->y * p->y + p->z * p->z);

    if (d)
    {
        p->x /= d;
        p->y /= d;
        p->z /= d;
    }
    else
    {
        p->x = 1;
        p->y = 0;
        p->z = 0;
    }
}

void ParticleDomain::Parse(void)
{
    char *enums[] =
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
    type = (PSDomainEnum)TokenEnum(enums, 0);
    int l;

    for (l = 0; l < 9; l++)
    {
        param[l] = TokenF(0);
    }
}

////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//                              COBRA - RED - THE PS REWRITING                                     \\
////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

const TrailRefsType TrailsTable[] =
{

    TRAILREF(CONTRAIL),
    TRAILREF(VORTEX),
    TRAILREF(SAM),
    TRAILREF(AIM120),
    TRAILREF(MAVERICK),
    TRAILREF(SMOKE),
    TRAILREF(MEDIUM_SAM),
    TRAILREF(LARGE_AGM),
    TRAILREF(GUN2),
    TRAILREF(SARH_MISSILE),
    TRAILREF(GUN3),
    TRAILREF(GUN),
    TRAILREF(LWING),
    TRAILREF(RWING),
    TRAILREF(ROCKET),
    TRAILREF(MISLSMOKE),
    TRAILREF(IR_MISSILE),
    TRAILREF(DARKSMOKE),
    TRAILREF(FIRE2),
    TRAILREF(WINGTIPVTX),
    TRAILREF(A10CANNON1),
    TRAILREF(ENGINE_SMOKE_LIGHT),
    TRAILREF(FEATURESMOKE),
    TRAILREF(BURNINGDEBRIS),
    TRAILREF(FLARE),
    TRAILREF(FLAREGLOW),
    TRAILREF(CONTAIL_SHORT),
    TRAILREF(WING_COLOR),
    TRAILREF(MISSILEHIT_FIRE),
    TRAILREF(BURNING_SMOKE),
    TRAILREF(BURNING_SMOKE2),
    TRAILREF(BURNING_FIRE),
    TRAILREF(ENGINE_SMOKE_SHARED),
    TRAILREF(GROUND_EXP_SMOKE),
    TRAILREF(DUSTCLOUD),
    TRAILREF(MISTCLOUD),
    TRAILREF(LINGERINGSMOKE),
    TRAILREF(GROUNDVDUSTCLOUD),
    TRAILREF(B52H_ENGINE_SMOKE),
    TRAILREF(F4_ENGINE_SMOKE),
    TRAILREF(COLOR_0),
    TRAILREF(COLOR_1),
    TRAILREF(COLOR_2),
    TRAILREF(COLOR_3),
    TRAILREF(RWING_COLOR_0),
    TRAILREF(RWING_COLOR_1),
    TRAILREF(RWING_COLOR_2),
    TRAILREF(RWING_COLOR_3),
    TRAILREF(RWING_COLOR_4),
    TRAILREF(LWING_COLOR_0),
    TRAILREF(LWING_COLOR_1),
    TRAILREF(LWING_COLOR_2),
    TRAILREF(LWING_COLOR_3),
    TRAILREF(LWING_COLOR_4),
    TRAILREF(VORTEX_LARGE),
    {"", 0}
};




PS_ListType DrawableParticleSys::PS_Lists[PS_LISTS_NR];
PS_PPType *DrawableParticleSys::PS_PPN;
PS_PEPType *DrawableParticleSys::PS_PEP;
PS_TPType *DrawableParticleSys::PS_TPN;
DWORD DrawableParticleSys::PS_PPNNr;
DWORD DrawableParticleSys::PS_TPNNr;
DWORD DrawableParticleSys::PS_PEPNr;
DWORD DrawableParticleSys::PS_TrailsID[MAX_TRAIL_PARAMETERS];
DWORD DrawableParticleSys::PS_SubTrails;
DWORD DrawableParticleSys::PS_RunTime;
float DrawableParticleSys::PS_ElapsedTime;
DWORD DrawableParticleSys::PS_LastTime;
Tcolor DrawableParticleSys::PS_HiLightCx;
Tcolor DrawableParticleSys::PS_LoLightCx;
DXLightType PS_Light;
D3DXMATRIX PS_LightRot;
QuadRndType DrawableParticleSys::PS_QuadRndList[PS_MAXQUADRNDLIST];
bool DrawableParticleSys::PS_NVG;
bool DrawableParticleSys::PS_TV;

// This function evaluates linearly, or logaritmically if the incoming value is negative
inline float DrawableParticleSys::PS_EvalTimedLinLogFloat(float life, int &LastStage, int Count, timedFloat *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return input[Count - 1].value;

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return input[Count - 1].value;

    // return the result
    if (input[LastStage + 1].LogMode == false)
        return (life - input[LastStage].time) * input[LastStage].K + input[LastStage].value;

    // get the time difference
    float Time = (input[LastStage + 1].time - input[LastStage].time);
    // Calculate the Time Position btw 1 and 0
    Time = (life - input[LastStage].time) / Time;
    // scale to 100 to get a Log10 Array index
    int   Idx = (int)(Time * LOG10_ARRAY_ITEMS);

    // limit check
    if (Idx >= LOG10_ARRAY_ITEMS) Idx = LOG10_ARRAY_ITEMS - 1;

    // get the Value difference btw In and Out
    float RelValue = input[LastStage].value - input[LastStage + 1].value;
    // return the value
    return RelValue * DrawableParticleSys::Log10Array[Idx] + input[LastStage + 1].value;
}


inline float DrawableParticleSys::PS_EvalTimedFloat(float life, int &LastStage, int Count, timedFloat *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // return the result
    return (life - input[LastStage].time) * input[LastStage].K + input[LastStage].value;
}

inline psRGBA DrawableParticleSys::PS_EvalTimedRGBA(float life, int &LastStage, int Count, timedRGBA *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    psRGBA retVal;

    float elapsed = life - input[LastStage].time;
    retVal.r = elapsed * input[LastStage].K.r + input[LastStage].value.r;
    retVal.g = elapsed * input[LastStage].K.g + input[LastStage].value.g;
    retVal.b = elapsed * input[LastStage].K.b + input[LastStage].value.b;
    retVal.a = elapsed * input[LastStage].K.a + input[LastStage].value.a;

    return (retVal);
}

inline psRGB DrawableParticleSys::PS_EvalTimedRGB(float life, int &LastStage, int Count, timedRGB *input)
{
    // Get the 1st value if just one stage
    if (Count < 2) return (input[0].value);

    // if past the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    // if Life past the Stage Time update to the next Stage
    if (life > input[LastStage + 1].time) LastStage++;

    // again the check for the end of stages, return last stage value
    if (LastStage >= Count - 1) return(input[Count - 1].value);

    psRGB retVal;
    /*retVal.r=RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value.r, input[LastStage+1].value.r);
    retVal.g=RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value.g, input[LastStage+1].value.g);
    retVal.b=RESCALE(life, input[LastStage].time, input[LastStage+1].time, input[LastStage].value.b, input[LastStage+1].value.b);*/

    float elapsed = life - input[LastStage].time;
    retVal.r = elapsed * input[LastStage].K.r + input[LastStage].value.r;
    retVal.g = elapsed * input[LastStage].K.g + input[LastStage].value.g;
    retVal.b = elapsed * input[LastStage].K.b + input[LastStage].value.b;
    return (retVal);
}



// this function reinitialize and eventually allocate a passed PS List
void DrawableParticleSys::PS_ListInit(DWORD i, DWORD nr, DWORD size)
{
    // check for memory and eventually allocate it
    if ( not PS_Lists[i].ObjectList)
    {
        PS_Lists[i].ObjectList = calloc(nr, size);
    }

    if ( not PS_Lists[i].IndexList)
    {
        PS_Lists[i].IndexList = (PS_PTR*)calloc(nr, sizeof(PS_PTR));
    }

    // reset indexes
    for (DWORD n = 0; n < nr; n++)
    {
        PS_Lists[i].IndexList[n] = (PS_PTR)n;
    }

    // reset pointers
    PS_Lists[i].ObjectIn = PS_Lists[i].ObjectOut = 0;
    PS_Lists[i].ListEntry = PS_Lists[i].ListEnd = PS_NOPTR;
    PS_Lists[i].ListSize = nr;
}


// this function Initialize PS Lists
void DrawableParticleSys::PS_ListsInit(void)
{


    // Initlalize all supported Lists
    memset(PS_Lists, 0, sizeof(PS_Lists));

    PS_INIT_LIST(PS_PARTICLES_IDX, PS_MAX_PARTICLES, ParticleNodeType);
    PS_INIT_LIST(PS_POLYS_IDX, PS_MAX_POLYS, PolySubPartType);
    PS_INIT_LIST(PS_EMITTERS_IDX, PS_MAX_EMITTERS, EmitterPartType);
    PS_INIT_LIST(PS_SOUNDS_IDX, PS_MAX_SOUNDS, SoundSubPartType);
    PS_INIT_LIST(PS_CLUSTERS_IDX, PS_MAX_CLUSTERS, ClusterPosType);
    PS_INIT_LIST(PS_LIGHTS_IDX, PS_MAX_LIGHTS, LightPartType);
    PS_INIT_LIST(PS_TRAILS_IDX, PS_MAX_TRAILS, TrailEmitterType);

    if ( not PS_PPN)
    {
        PS_PPN = (PS_PPType*)malloc(MAX_PARTICLE_PARAMETERS * sizeof(PS_PPType));
        PS_PPNNr = 0;
    }

    if ( not PS_TPN)
    {
        PS_TPN = (PS_TPType*)malloc(MAX_TRAIL_PARAMETERS * sizeof(PS_TPType));
        PS_TPNNr = 0;
    }

    if ( not PS_PEP)
    {
        PS_PEP = (PS_PEPType*)malloc(MAX_EMITTERS_PARAMETERS * sizeof(PS_PEPType));
        PS_PEPNr = 0;
    }

    // initialize PS light stuff
    PS_Light.Flags.OwnLight = false;
    PS_Light.Flags.Static = false;
    PS_Light.Flags.NotSelfLight = true;
    PS_Light.Light.dltType = D3DLIGHT_POINT;
    PS_Light.Light.dcvAmbient.b = PS_Light.Light.dcvAmbient.g = PS_Light.Light.dcvAmbient.g = 0.0f;
    PS_Light.Light.dcvSpecular.r = PS_Light.Light.dcvSpecular.g = PS_Light.Light.dcvSpecular.b = 1.0f;
    PS_Light.Light.dcvDiffuse.r = PS_Light.Light.dcvDiffuse.g = PS_Light.Light.dcvDiffuse.b = 1.0f;
    PS_Light.Light.dvAttenuation0 = 0.0f;
    PS_Light.Light.dvAttenuation1 = 0.01f;

    PS_SubTrails = 0;
    TrailsHandle = 0;
    D3DXMatrixIdentity(&PS_LightRot);

}


// this function Releases PS Lists
void DrawableParticleSys::PS_ListsRelease(void)
{
    for (DWORD i = 0; i < PS_LISTS_NR; i++)
    {
        if (PS_Lists[i].IndexList)
        {
            free(PS_Lists[i].IndexList);
        }

        if (PS_Lists[i].ObjectList)
        {
            free(PS_Lists[i].ObjectList);
        }

        PS_Lists[i].IndexList = NULL;
        PS_Lists[i].ObjectList = NULL;
        PS_Lists[i].ListSize = 0;
    }

    if (PS_PPN)
    {
        free(PS_PPN);
        PS_PPN = NULL;
    }

    if (PS_PEP)
    {
        free(PS_PEP);
        PS_PEP = NULL;
    }

    if (PS_TPN)
    {
        free(PS_TPN);
        PS_TPN = NULL;
    }

    PS_SubTrails = 0;
}





///////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// *** OBJECT ADD MACRO ***

template <typename _part_t >
inline PS_PTR AddPart(PS_ListType &List, _part_t *Part, PS_PTR Item)
{
    // Next Pointer Reset
    Part[Item].NEXT = PS_NOPTR;

    // If no Entry Point in the List, THIS will be the one...
    if (List.ListEntry == PS_NOPTR) List.ListEntry = Item;
    // if Any List Entry, then there is an End, this Item will follow it
    else Part[List.ListEnd].NEXT = Item;

    // This is the Last Item in the List
    List.ListEnd = Item;
    // return the ITEM
    return Item;
}

/////////////////////////////////////////////////////////////////////////////////////////////////


// Function assigning a PS PTR in the List
PS_PTR DrawableParticleSys::PS_AddItem(DWORD ListIdx)
{
    PS_ListType &List = PS_Lists[ListIdx];
    PS_PTR Item, LastObj = List.ObjectIn;

    // get the 1st allocable Item
    Item = List.IndexList[List.ObjectIn++];

    // New entry, so rollover the entry ptr
    if (List.ObjectIn >= List.ListSize) List.ObjectIn = 0;

    // if List Full ( going to roll on out ptr ), return back
    // * so, finally, a list overflow would always replace last item in list *
    if (List.ObjectIn == List.ObjectOut)
    {
        List.ObjectIn = LastObj;
        return PS_NOPTR;
    }

    // here if exists a previous Item to link to
    switch (ListIdx)
    {

        case PS_PARTICLES_IDX :
                Item = AddPart(List, (ParticleNodeType*)List.ObjectList, Item);
            break;

        case PS_POLYS_IDX :
                Item = AddPart(List, (PolySubPartType*)List.ObjectList, Item);
            break;

        case PS_EMITTERS_IDX :
                Item = AddPart(List, (EmitterPartType*)List.ObjectList, Item);
            break;

        case PS_SOUNDS_IDX :
                Item = AddPart(List, (SoundSubPartType*)List.ObjectList, Item);
            break;

        case PS_LIGHTS_IDX :
                Item = AddPart(List, (LightPartType*)List.ObjectList, Item);
            break;

        case PS_TRAILS_IDX :
                Item = AddPart(List, (TrailEmitterType*)List.ObjectList, Item);
            break;

        case PS_CLUSTERS_IDX :
                Item = AddPart(List, (ClusterPosType*)List.ObjectList, Item);
            break;


    }


    // return the Item
    return Item;
}




///////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


template <typename _part_t >
inline PS_PTR RemovePart(PS_ListType &List, _part_t *Part, PS_PTR Item, PS_PTR Prev)
{
    // if LAST in List, make the Prev as Last
    if (List.ListEnd == Item) List.ListEnd = Prev;

    // if FIRST in List ( no previous pointer ), move the Next as 1st and exit here
    if (Prev == PS_NOPTR)
    {
        List.ListEntry = Part[Item].NEXT;
        return List.ListEntry;
    }

    // if in the middle
    Part[Prev].NEXT = Part[Item].NEXT;
    return Part[Item].NEXT;
}

/////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


// Function removing a PS PTR from the List and marking it as available
// Returns a PS_PTR to the next item
PS_PTR DrawableParticleSys::PS_RemoveItem(DWORD ListIdx, PS_PTR Item, PS_PTR Prev)
{
    PS_ListType &List = PS_Lists[ListIdx];

    // Security check
    if (Item == PS_NOPTR) return PS_NOPTR;

    // remove the Item and Roll the Out ptr
    List.IndexList[List.ObjectOut++] = Item;

    // New exit, so rollover the exit ptr
    if (List.ObjectOut >= List.ListSize) List.ObjectOut = 0;

    switch (ListIdx)
    {

        case PS_PARTICLES_IDX :
                Item = RemovePart(List, (ParticleNodeType*)List.ObjectList, Item, Prev);
            break;

        case PS_POLYS_IDX :
                Item = RemovePart(List, (PolySubPartType*)List.ObjectList, Item, Prev);
            break;

        case PS_EMITTERS_IDX :
                Item = RemovePart(List, (EmitterPartType*)List.ObjectList, Item, Prev);
            break;

        case PS_SOUNDS_IDX :
                Item = RemovePart(List, (SoundSubPartType*)List.ObjectList, Item, Prev);
            break;

        case PS_LIGHTS_IDX :
                Item = RemovePart(List, (LightPartType*)List.ObjectList, Item, Prev);
            break;

        case PS_TRAILS_IDX :
                Item = RemovePart(List, (TrailEmitterType*)List.ObjectList, Item, Prev);
            break;

        case PS_CLUSTERS_IDX :
                Item = RemovePart(List, (ClusterPosType*)List.ObjectList, Item, Prev);
            break;



    }

    return Item;
}




///////////////////////////////////////////////// SOUNDs STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


// Adds a SOUND PART to the Particle nodes list, setups all it's parameters
void DrawableParticleSys::PS_AddSound(PS_PTR owner, PS_PTR ID)
{
    // Get a free slot
    PS_PTR ptr = PS_AddItem(PS_SOUNDS_IDX);

    // security check
    if (ptr == PS_NOPTR) return;

    // Get the pointer in the list
    SoundSubPartType &pn = (((SoundSubPartType*)PS_Lists[PS_SOUNDS_IDX].ObjectList)[ptr]);
    // the owner
    pn.OWNER = owner;
    // The assigned Particle Node Parameter
    pn.PPN = ID;
    // last in List
    pn.NEXT = PS_NOPTR;

    // Pointer to particle parametes
    PS_PPType &ppn = PS_PPN[ID];
    // Ok, setup the parameters

    // The sound is active
    pn.Play = true;
    // Stages
    pn.VolumeStage = pn.PitchStage = 0;
    // The sound stuff
    pn.SoundId = ppn.sndId;
    pn.Looped = ppn.sndLooped ? true : false;

    pn.SoundPos = new F4SoundPos();
}






// Function pasring and updating all Polys... responsable to kill dead ones too
void  DrawableParticleSys::PS_SoundRun(void)
{
#ifdef DEBUG_NEW_PS_SOUNDS
    DWORD Count = 0;
#endif
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_SOUNDS_IDX].ListEntry, LastPtr = PS_NOPTR;

    float SndVol, SndPitch;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
#ifdef DEBUG_NEW_PS_SOUNDS
        Count++;
#endif;
        // The Poly
        SoundSubPartType &Sound = (((SoundSubPartType*)PS_Lists[PS_SOUNDS_IDX].ObjectList)[ptr]);
        // Pointer to particle
        ParticleNodeType &Part = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[Sound.OWNER]);

        // If Particle already dead, end here and go to next
        if ( not Part.Alive)
        {
            ptr = PS_RemoveItem(PS_SOUNDS_IDX, ptr, LastPtr);
            continue;
        }

        // The Life
        float Life = Part.life;
        // Pointer to particle parametes
        PS_PPType &ppn = PS_PPN[Sound.PPN];

        // * OK, the SOUND STUFF *
        if (Sound.Play and PS_ElapsedTime)
        {
            SndVol = PS_EvalTimedFloat(Life, Sound.VolumeStage, ppn.sndVolStages,   ppn.sndVol);
            SndPitch = PS_EvalTimedFloat(Life, Sound.PitchStage, ppn.sndPitchStages,  ppn.sndPitch);

            Sound.SoundPos->UpdatePos(Part.pos.x, Part.pos.y, Part.pos.z, Part.vel.x, Part.vel.y, Part.vel.z);
            Sound.SoundPos->Sfx(Sound.SoundId, 0, SndPitch, SndVol);

            if ( not Sound.Looped)
            {
                ptr = PS_RemoveItem(PS_SOUNDS_IDX, ptr, LastPtr);
                continue;
            }

        }

        LastPtr = ptr;
        ptr = Sound.NEXT;
    }

#ifdef DEBUG_NEW_PS_SOUNDS
    REPORT_VALUE("NEW PS SOUNDS", Count);
#endif
}




// Function pasring and updating all Polys... responsable to kill dead ones too
void  DrawableParticleSys::PS_LightsRun(void)
{
#ifdef DEBUG_NEW_PS_LIGHTS
    DWORD Count = 0;
#endif
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_LIGHTS_IDX].ListEntry, LastPtr = PS_NOPTR;


    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
#ifdef DEBUG_NEW_PS_LIGHTS
        Count++;
#endif;
        // The Poly
        LightPartType &Light = (((LightPartType*)PS_Lists[PS_LIGHTS_IDX].ObjectList)[ptr]);

        // If Particle already dead, end here and go to next
        if ( not Light.Alive)
        {
            ptr = PS_RemoveItem(PS_LIGHTS_IDX, ptr, LastPtr);
            continue;
        }

        // if a light value present
        if (Light.Light)
        {
            float Distance = TheDXEngine.DX2D_GetDistance((D3DXVECTOR3*)&Light.Pos);
            D3DXVECTOR3 k;
            TheDXEngine.DX2D_GetRelativePosition(&k);
            PS_Light.Light.dcvDiffuse = Light.Color;
            PS_Light.Light.dvRange = Light.Radius;
            PS_Light.Light.dvAttenuation1 = 20.0f / Light.Radius;
            TheLightEngine.AddDynamicLight(-1, &PS_Light, &PS_LightRot, (D3DVECTOR*)&k, Distance);
        }

        Light.Alive = false;
        Light.Light = 0;

        LastPtr = ptr;
        ptr = Light.NEXT;
    }

#ifdef DEBUG_NEW_PS_LIGHTS
    REPORT_VALUE("NEW PS LIGHTS", Count);
#endif
}






///////////////////////////////////////////////// POLYs STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

// Adds a PARTICLE NODE to the Particle nodes list, setups all it's parameters
void DrawableParticleSys::PS_AddPoly(PS_PTR owner, PS_PTR ID)
{
    // Get a free slot
    PS_PTR ptr = PS_AddItem(PS_POLYS_IDX);

    // security check
    if (ptr == PS_NOPTR) return;

    // Get the pointer in the list
    PolySubPartType &pn = (((PolySubPartType*)PS_Lists[PS_POLYS_IDX].ObjectList)[ptr]);
    // the owner
    pn.OWNER = owner;
    // The assigned Particle Node Parameter
    pn.PPN = ID;
    // last in List
    pn.NEXT = PS_NOPTR;

    // Pointer to particle parametes
    PS_PPType &ppn = PS_PPN[ID];
    // Ok, setup the parameters

    // initialize variables
    pn.AlphaStage = pn.ColorStage = pn.LightStage = 0;

    // Link here the Texture and get its U/V Coord
    ParticleTextureNode *pt = ppn.Texture;

    // check if depending on group texture
    if (ppn.GroupFlags bitand GRP_TEXTURE)
        // if depending from a group, ask for a random texture node
        pt = (ParticleTextureNode*)(((ParticleGroupNode*)(ppn.Texture))->GetRandomArgument());

    if (pt)
    {
        // use the PPN texture node
        pn.TexHandle = pt->TexHandle;
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 0, pn.tu[0], pn.tv[0]);
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 1, pn.tu[1], pn.tv[1]);
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 2, pn.tu[2], pn.tv[2]);
        TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 3, pn.tu[3], pn.tv[3]);
    }

    // reset rotation
    pn.Rotation = 0.0f;
    // Randomic rate btw Max and Min
    pn.RotationRate = (PRANDFloatPos() * (ppn.RotationRateMax - ppn.RotationRateMin) + ppn.RotationRateMin);
    // randomic Sign
    pn.RotationRate *= (PRANDFloat() >= 0.0f) ? 1.0f : -1.0f;
}



// Function pasring and updating all Polys... responsable to kill dead ones too
void  DrawableParticleSys::PS_PolyRun(void)
{
#ifdef DEBUG_NEW_PS_POLYS
    DWORD Count = 0;
#endif
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_POLYS_IDX].ListEntry, LastPtr = PS_NOPTR;
    psRGB HifColor;
    float light;
    D3DDYNVERTEX Quad[4];

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
#ifdef DEBUG_NEW_PS_POLYS
        Count++;
#endif;
        // The Poly
        PolySubPartType &Poly = (((PolySubPartType*)PS_Lists[PS_POLYS_IDX].ObjectList)[ptr]);
        // Pointer to particle
        ParticleNodeType &Part = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[Poly.OWNER]);

        // If Particle already dead, end here and go to next
        if ( not Part.Alive)
        {
            ptr = PS_RemoveItem(PS_POLYS_IDX, ptr, LastPtr);
            continue;
        }

        // The Life
        float Life = Part.life;
        // Pointer to particle parametes
        PS_PPType &ppn = PS_PPN[Poly.PPN];

        float Distance;
        float size = Part.Radius;

        // Default as a light is not in range, and object is Visible
        bool LightIn = false;
        bool Visible = true;
        float LightRadius = size * LIGHT_SIZE_CX;

        /////////////////////////////////////////// * CLUSTERING * \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // Check for Clustering enabled, if so
        // if whole cluster is OUTSIDE FOV, skip, nothing is visible
        // if whole cluster is INSIDE FOV, just update distance, no visibility check
        // if NOT FULLY INSIDE FOV, make visibility checks
        if (Part.Cluster)
        {
            // The cluster assigned
            ClusterPosType &Cluster = ((ClusterPosType*)PS_Lists[PS_CLUSTERS_IDX].ObjectList)[Part.CLUSTER];

            // Update light in status if an emitting poly
            LightIn = Cluster.LightIn and ppn.EmitLight;

            // 2 cases, based on light in stuff
            // ************* NOT IN LIGHT RANGE *************
            if ( not LightIn)
            {
                // Full out of FOV, skip
                if (Cluster.Out) goto Skip;

                // if not fully in, check for visibility
                if ( not Cluster.In)
                    if ( not TheDXEngine.DX2D_GetVisibility((D3DXVECTOR3*) &Part.pos, (ppn.EmitLight) ? size : LightRadius)) goto Skip;

                // Ok, if here, it's in, compute distance
                Distance = TheDXEngine.DX2D_GetDistance((D3DXVECTOR3*)&Part.pos);
            }
            // ************ LIGHT IN RANGE **************
            else
            {
                // Full out of FOV, mark as not visible
                if (Cluster.Out) Visible = false;

                // if not fully in but visible, check for visibility
                if (Visible and not Cluster.In)
                    if ( not TheDXEngine.DX2D_GetVisibility((D3DXVECTOR3*) &Part.pos, (ppn.EmitLight) ? size : LightRadius)) Visible = false;

                // if still visible, compute distance
                if (Visible) Distance = TheDXEngine.DX2D_GetDistance((D3DXVECTOR3*)&Part.pos);

            }
        }
        else
        {
            // CLUSTER NOT USED, ALWAYS PERFORM VISIBILITY CHECK
            // Get the Distance
            Distance = TheDXEngine.DX2D_GetDistance((D3DXVECTOR3*)&Part.pos);

            // if out of Distance, end here
            if (Distance > ppn.visibleDistance) goto Skip;

            // Update light in parameter if an emissive poly
            if (ppn.EmitLight and Distance < LightRadius) LightIn = true;

            // Perform visibility check
            if ( not TheDXEngine.DX2D_GetVisibility((D3DXVECTOR3*) &Part.pos, (ppn.EmitLight) ? size : LightRadius)) Visible = false;

            // skip all if not visible and not even in light range
            if ( not Visible and not LightIn) goto Skip;
        }

        /////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


        ////////////////////////////////// * UPDATE START * \\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // 1st of all, Update colors, as, even if poly is not visible, they may affect any light
        // and processor may be here just as a 'Light In Range' is present

        light = PS_EvalTimedFloat(Life, Poly.LightStage, ppn.lightStages, ppn.light);
        HifColor = PS_EvalTimedRGB(Life, Poly.ColorStage, ppn.colorStages, ppn.color);

        // Flash value goes from 0.5 to 1.0
        float Flash = (max(light, 0.5f) - 0.5f);
        // Subract from Light the Flash value
        light = (light - Flash) * 2;
        DWORD LiteColor;

        if (PS_NVG or PS_TV)
        {
            // NVG View
            HifColor.r = HifColor.r * PS_HiLightCx.r;
            HifColor.g = HifColor.g * PS_HiLightCx.g;
            HifColor.b = HifColor.b * PS_HiLightCx.b;
            Flash = min(255.0f, Flash * 768.0f);
            LiteColor = F_TO_UARGB(1.0f, Flash * .90f,  Flash,  Flash * .95f);
        }
        else
        {
            // NORMAL VIEW
            Flash *= 255.9f;
            LiteColor = F_TO_UARGB(1.0f, Flash,  Flash,  Flash);
            HifColor.r = (HifColor.r - HifColor.r * light) * PS_HiLightCx.r + HifColor.r * light;
            HifColor.g = (HifColor.g - HifColor.g * light) * PS_HiLightCx.g + HifColor.g * light;
            HifColor.b = (HifColor.b - HifColor.b * light) * PS_HiLightCx.b + HifColor.b * light;
        }

        // if a light assigned
        if (ppn.EmitLight and Part.LIGHT not_eq PS_NOPTR)
        {
            LightPartType &Light = (((LightPartType*)PS_Lists[PS_LIGHTS_IDX].ObjectList)[Part.LIGHT]);
            Light.Alive = true;

            // if this light more intense
            if (Light.Light < light)
            {
                Light.Radius = LightRadius * light;
                Light.Color.r = HifColor.r * HifColor.r / 65536.0f;
                Light.Color.g = HifColor.g * HifColor.g / 65536.0f;
                Light.Color.b = HifColor.b * HifColor.b / 65536.0f;
                Light.Light = light;
                Light.Pos = Part.pos;
            }
        }

        // ok, if not visible, skip all the rest
        if ( not Visible) goto Skip;

        // compute Alpha of the Poly
        float alpha = PS_EvalTimedLinLogFloat(Life, Poly.AlphaStage, ppn.alphaStages, ppn.alpha);

        DWORD HiColor = F_TO_UARGB(alpha, HifColor.r, HifColor.g, HifColor.b);
        DWORD LoColor = F_TO_UARGB(alpha, HifColor.r * .68f, HifColor.g * .68f, HifColor.b * .68f);

        // Rotation stuff
        Poly.Rotation += Poly.RotationRate * PS_ElapsedTime;

        Quad[0].dwColour = Quad[1].dwColour = HiColor;
        Quad[2].dwColour = Quad[3].dwColour = LoColor;
        Quad[0].dwSpecular = Quad[1].dwSpecular = Quad[2].dwSpecular = Quad[3].dwSpecular = LiteColor;

        // Assign drawing data
        Quad[0].tu = Poly.tu[0], Quad[0].tv = Poly.tv[0];
        Quad[1].tu = Poly.tu[1], Quad[1].tv = Poly.tv[1];
        Quad[2].tu = Poly.tu[2], Quad[2].tv = Poly.tv[2];
        Quad[3].tu = Poly.tu[3], Quad[3].tv = Poly.tv[3];

        // COBRA - RED - Rotation, get the radius and angle
        mlTrig RotCx;
        mlSinCos(&RotCx, Poly.Rotation);
        RotCx.cos *= size;
        RotCx.sin *= size;

        if (ppn.ZPoly)
        {
            Quad[0].pos.z = Quad[1].pos.z = Quad[2].pos.z = Quad[3].pos.z = 0.f;
            Quad[0].pos.y = RotCx.cos;
            Quad[0].pos.x = RotCx.sin;
            Quad[1].pos.y = RotCx.sin;
            Quad[1].pos.x = -RotCx.cos;
            Quad[2].pos.y = -RotCx.cos;
            Quad[2].pos.x = -RotCx.sin;
            Quad[3].pos.y = -RotCx.sin;
            Quad[3].pos.x = RotCx.cos;
            // draw and declare as VISIBLE, the Visibility test was at function entry point ( DX2D_GetDistance()>0)
            TheDXEngine.DX2D_AddQuad(LAYER_AUTO, POLY_VISIBLE, (D3DXVECTOR3*)&Part.pos, Quad, size, Poly.TexHandle);
        }
        else
        {
            Quad[0].pos.x = Quad[1].pos.x = Quad[2].pos.x = Quad[3].pos.x = 0.f;
            Quad[0].pos.y = RotCx.cos;
            Quad[0].pos.z = RotCx.sin;
            Quad[1].pos.y = RotCx.sin;
            Quad[1].pos.z = -RotCx.cos;
            Quad[2].pos.y = -RotCx.cos;
            Quad[2].pos.z = -RotCx.sin;
            Quad[3].pos.y = -RotCx.sin;
            Quad[3].pos.z = RotCx.cos;

            // draw with BillBoard and declare as VISIBLE, the Visibility test was at function entry point ( DX2D_GetDistance()>0)
            TheDXEngine.DX2D_AddQuad(LAYER_AUTO, POLY_BB bitor POLY_VISIBLE, (D3DXVECTOR3*)&Part.pos, Quad, size, Poly.TexHandle);
        }

        Skip:
        LastPtr = ptr;
        ptr = Poly.NEXT;
    }

#ifdef DEBUG_NEW_PS_POLYS
    REPORT_VALUE("NEW PS POLYS", Count);
#endif


}


///////////////////////////////////////////// EMITTERS STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

// Function pasring and updating all Emitters... responsable to kill dead ones too
void  DrawableParticleSys::PS_EmitterRun(void)
{
#ifdef DEBUG_NEW_PS_EMITTERS
    DWORD Count = 0;
#endif
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_EMITTERS_IDX].ListEntry, LastPtr = PS_NOPTR;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
#ifdef DEBUG_NEW_PS_EMITTERS
        Count++;
#endif;
        // The Emitter
        EmitterPartType &Emitter = (((EmitterPartType*)PS_Lists[PS_EMITTERS_IDX].ObjectList)[ptr]);
        // Pointer to particle
        ParticleNodeType &Part = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[Emitter.OWNER]);

        // If Particle already dead, end here and go to next
        if ( not Part.Alive)
        {
            ptr = PS_RemoveItem(PS_EMITTERS_IDX, ptr, LastPtr);
            continue;
        }

        // The Life
        float Life = Part.life;

        // if a light assigned, confirm still alive
        if (Emitter.LIGHT not_eq PS_NOPTR)((LightPartType*)PS_Lists[PS_LIGHTS_IDX].ObjectList)[Emitter.LIGHT].Alive = true;

        // cache the Position from the OWNER
        Tpoint epos = Part.pos;
        float qty = 0;
        bool Alive = true;

        // check for EMITTER MODE
        switch (Emitter.PEP->mode)
        {

                // * EMIT ON GROUND * if hit the ground emit the requested quantity and die
            case PSEM_IMPACT:
                    if (epos.z >= Part.GroundLevel) epos.z = Part.GroundLevel, qty += Emitter.PEP->rate[0].value, Alive = false;

                break;


                // * EARTH IMPACT * if on ground
            case PSEM_EARTHIMPACT:
                    if (epos.z >= Part.GroundLevel)
                    {
                        epos.z = Part.GroundLevel;
                        // get ground type
                        int gtype = OTWDriver.GetGroundType(epos.x, epos.y);

                        // if on WATER or RIVER add the quantity and die
                        if ( not (gtype == COVERAGE_WATER or gtype == COVERAGE_RIVER)) qty += Emitter.PEP->rate[0].value, Alive = false;
                    }

                break;

                // * WATER IMPACT *
            case PSEM_WATERIMPACT:
                    if (epos.z >= Part.GroundLevel)
                    {
                        epos.z = Part.GroundLevel;
                        // get ground type
                        int gtype = OTWDriver.GetGroundType(epos.x, epos.y);

                        // if on WATER or RIVER add the quantity and die
                        if (gtype == COVERAGE_WATER or gtype == COVERAGE_RIVER) qty += Emitter.PEP->rate[0].value, Alive = false;
                    }

                break;

                // * ONCE EMISSION *
            case PSEM_ONCE:
                    while (Emitter.LastStage < Emitter.PEP->stages and (Emitter.PEP->rate[Emitter.LastStage].time + Emitter.RndTime) <= Part.life)
                    {
                        // Add quantity and go to next stage
                        qty += Emitter.PEP->rate[Emitter.LastStage++].value;
                        // Ok, recalculate random timing for next stage
                        Emitter.RndTime = PRANDFloat() * Emitter.RndTimeCx;
                    }

                // if gone past last stage, die..
                if (Emitter.LastStage >= Emitter.PEP->stages) Alive = false;

                break;

                // * EMISSION PER SECONDS *
            case PSEM_PERSEC:
                    qty = PS_EvalTimedFloat(Part.life, Emitter.LastStage, Emitter.PEP->stages, Emitter.PEP->rate);
                qty = qty * PS_ElapsedTime + Emitter.rollover;

                // if gone past last stage, die..
                if (Emitter.LastStage >= Emitter.PEP->stages) Alive = false;

                break;
        }


        while (qty >= 1)
        {
            float v;
            Tpoint pos, aim, subvel, w;

            Emitter.PEP->domain.GetRandomPosition(&w);
            MatrixMult(Part.rotation, &w, &pos);

            Emitter.PEP->target.GetRandomDirection(&w);
            MatrixMult(Part.rotation, &w, &aim);

            v = Emitter.PEP->velocity + Emitter.PEP->velVariation * NRAND;

            subvel.x = aim.x * v;
            subvel.y = aim.y * v;
            subvel.z = aim.z * v;

            subvel.x += Part.vel.x;
            subvel.y += Part.vel.y;
            subvel.z += Part.vel.z;

            pos.x += epos.x;
            pos.y += epos.y;
            pos.z += epos.z;

            PS_AddParticle(Emitter.PEP->id, &pos, &subvel, &aim, 0, Part.CLUSTER, Emitter.LIGHT);
            qty -= 1.0f;
        }

        Emitter.rollover = qty;

        // If this was last time for the Emitter
        if ( not Alive) ptr = PS_RemoveItem(PS_EMITTERS_IDX, ptr, LastPtr);
        else LastPtr = ptr, ptr = Emitter.NEXT;
    }

#ifdef DEBUG_NEW_PS_EMITTERS
    REPORT_VALUE("NEW PS EMITTERS", Count);
#endif
}

void DrawableParticleSys::PS_AddEmitter(PS_PTR owner, ParticleEmitterParam *PEP, PS_PTR Light)
{
    // Get a free slot
    PS_PTR ptr = PS_AddItem(PS_EMITTERS_IDX);

    // security check
    if (ptr == PS_NOPTR) return;

    // Get the pointer in the list
    EmitterPartType &pn = (((EmitterPartType*)PS_Lists[PS_EMITTERS_IDX].ObjectList)[ptr]);

    // assign the PEP
    pn.PEP = PEP;
    // assign the OWNER
    pn.OWNER = owner;
    // and Light
    pn.LIGHT = Light;

    if (pn.LIGHT not_eq PS_NOPTR)((LightPartType*)PS_Lists[PS_LIGHTS_IDX].ObjectList)[pn.LIGHT].Light = 0.0f;

    // initialize variables
    pn.rollover  = 0;
    pn.LastStage = 0;
    // Get the randomic time CX
    pn.RndTimeCx = PEP->TimeVariation;
    // set the Random Time for 1st emission
    pn.RndTime = PRANDFloat() * pn.RndTimeCx;
}



// This function generates all the Emitters required by a particle
void DrawableParticleSys::PS_GenerateEmitters(PS_PTR owner, PS_PPType &PPN)
{
    int count;
    PS_PTR Light;
    // Get light status from parent
    Light = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[owner]).LIGHT;

    // if not a light already assigned and a light Root, get a new light to pass to emitters
    if (Light == PS_NOPTR and PPN.LightRoot) Light = PS_AddItem(PS_LIGHTS_IDX);

    // just one Random Emitter
    if (PPN.emitOneRandomly)
    {
        for (count = 0; PPN.emitter[count].stages and count < PSMAX_EMITTERS; count++);

        if (count)
        {
            int l = rand() % count;
            PS_AddEmitter(owner, &PPN.emitter[l], Light);
        }
    }
    else
    {
        // More randomic emitters
        float Test = PRANDFloatPos();

        for (count = 0; PPN.emitter[count].stages and count < PSMAX_EMITTERS; count++)
        {
            // Ok... check the probability to enable this emitter...
            if (Test >= PPN.ParticleEmitterMin[count] and Test < PPN.ParticleEmitterMax[count]) PS_AddEmitter(owner, &PPN.emitter[count], Light);
        }
    }
}





//////////////////////////////////////////// PARTICLES STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

// Thhe public Call
void DrawableParticleSys::PS_AddParticleEx(int ID, Tpoint *Pos, Tpoint *Vel)
{
    PS_AddParticle((int)PPN[ID], Pos, Vel);
}


// Adds a PARTICLE NODE to the Particle nodes list, setups all it's parameters
void DrawableParticleSys::PS_AddParticle(int ID, Tpoint *Pos, Tpoint *Vel, Tpoint *Aim, float fRotationRate, PS_PTR Cluster, PS_PTR Light)
{
    // Get a free slot
    PS_PTR ptr = PS_AddItem(PS_PARTICLES_IDX);

    // security check
    if (ptr == PS_NOPTR) return;

    // Get the pointer in the list
    ParticleNodeType &pn = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[ptr]);
    // Pointer to particle parametes
    PS_PPType &ppn = PS_PPN[ID];

    // Set up particle parameters
    // The index in the PPN Array
    pn.PPN = ID;
    // The passed position
    pn.pos = *Pos;
    // Light
    pn.LIGHT = Light;

    // defaults to no Cluster assigned
    pn.CLUSTER = PS_NOPTR;
    pn.Cluster = false;

    // Check for clustering - No clustering for -1
    if (ppn.ClusterMode not_eq NO_CLUSTERING)
    {
        // * CHILD CLUSTER *
        if (ppn.ClusterMode == CHILD_CLUSTER)
        {
            if (Cluster not_eq PS_NOPTR) pn.CLUSTER = Cluster, pn.Cluster = true;
        }
        else
        {
            // * NEW CLUSTER *
            // if not a cluster already assigned, or a new cluster, get a new one
            pn.CLUSTER = PS_AddItem(PS_CLUSTERS_IDX);

            // Check for Cluster Overflow
            if (pn.CLUSTER not_eq PS_NOPTR)
            {
                pn.Cluster = true;
                // check for cluster mode
                ClusterPosType &Cluster = (((ClusterPosType*)PS_Lists[PS_CLUSTERS_IDX].ObjectList)[pn.CLUSTER]);
                // it lives on
                Cluster.Alive = true;

                // Static or Dynamic cluster
                if (ppn.ClusterMode > 0) Cluster.Static = true, Cluster.Radius = ppn.ClusterMode;
                else Cluster.Static = false;
            }
        }
    }



    // Setup velocity
    if (Vel)
    {
        float v = ppn.velInherit;
        pn.vel.x = Vel->x * v;
        pn.vel.y = Vel->y * v;
        pn.vel.z = Vel->z * v;
    }
    else
    {
        pn.vel.x = pn.vel.y = pn.vel.z = 0;
    }

    // Wind effect
    pn.WindAffected = (ppn.WindAffected) ? 1 : 0;

    // Setup aiming
    if (Aim)
    {
        float v;
        v = ppn.velInitial + ppn.velVariation * NRAND;
        pn.vel.x += Aim->x * v;
        pn.vel.y += Aim->y * v;
        pn.vel.z += Aim->z * v;

    }

    // * LINKED POLY CREATION *
    if (ppn.drawType == PSDT_POLY) PS_AddPoly(ptr, ID);

    /* if(ppn->bspObj)
     {
     SubPart *sub = new SubPartBSP(this);
     sub->next = firstSubPart;
     firstSubPart = sub;
     }
    */
    // * LINKED SOUND *
    if (ppn.sndId) PS_AddSound(ptr, ID);


    // * LINKED TRAIL *
    if (ppn.trailId >= 0) PS_AddTrail(ppn.trailId, Pos, ptr);


    // * EMITTERS GENERATION *
    if (ppn.emitter[0].stages) PS_GenerateEmitters(ptr, ppn);


    /*switch(ppn->orientation)
    {
     case PSO_MOVEMENT:
     {
     SubPart *sub = new SubPartMovementOrientation(this);
     sub->next = firstSubPart;
     firstSubPart = sub;
     }
     break;

     default : break;
    }
    */

    pn.lastTime = pn.birthTime = TheTimeManager.GetClockTime();
    pn.lifespan  = ppn.lifespan + ppn.lifespanvariation * NRAND;
    pn.life = 0.0f;
    // pn.FrameNr[0]=FrameNr[1]=0;
    // pn.LastTimeRest[0]=LastTimeRest[1]=0;
    pn.GravityStage = pn.AccelStage = 0;
    pn.Alive = true;

    // Recalc Ground position
    pn.LastCalcPos = pn.pos;
    pn.GroundLevel = OTWDriver.GetGroundLevel(pn.pos.x, pn.pos.y);

    // Recalc Wind velocity
    mlTrig trigWind;
    float wind;
    mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pn.pos));
    wind = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pn.pos);
    wind *= ppn.WindFactor * 0.5f ; // RV - I-Hawk
    pn.Wind.x = trigWind.cos * wind;
    pn.Wind.y = trigWind.sin * wind;

    // TODO - Use this?
    pn.rotation = &psIRotation;

    // * SIZE CX *
    pn.SizeRandom = 1.0f + PRANDFloatPos() * ppn.SizeRandom;

    pn.SizeStage = 0;
    pn.Radius = ppn.size[0].value;
}



void DrawableParticleSys::PS_ParticleRun(void)
{
#ifdef DEBUG_NEW_PS_PARTICLES
    DWORD Count = 0;
#endif

    XMMVector XMMRadius, XMMPosMax, XMMPosMin;

    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_PARTICLES_IDX].ListEntry, LastPtr = PS_NOPTR;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
#ifdef DEBUG_NEW_PS_PARTICLES
        Count++;
#endif;
        ParticleNodeType &Part = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[ptr]);

        // If part is dead, remove it, all last stages have been already killed/exucuted
        if ( not Part.Alive)
        {
            ptr = PS_RemoveItem(PS_PARTICLES_IDX, ptr, LastPtr);
            continue;
        }

        // Security check, no negative life
        if (Part.lifespan <= 0)
        {
            Part.Alive = false;
            goto Skip;
        }

        // Life calculations
        float age;
        age = (float)(PS_RunTime - Part.birthTime) * .001f;
        Part.life = age / Part.lifespan;

        // Mark for die eventually
        if (Part.life >= 1.0f) Part.Alive = false;

        // Pointer to particle parametes
        PS_PPType &ppn = PS_PPN[Part.PPN];


        // Check if large position change from last Calc,
        // if so, recalc seom parameters
        if (F_ABS(Part.LastCalcPos.x - Part.pos.x) >= PS_RECALC_DELTA or F_ABS(Part.LastCalcPos.y - Part.pos.y) >= PS_RECALC_DELTA)
        {
            // Recalc Ground position
            Part.LastCalcPos = Part.pos;
            Part.GroundLevel = OTWDriver.GetGroundLevel(Part.pos.x, Part.pos.y);
            // Recalc Wind velocity
            Part.Wind = ((WeatherClass*)realWeather)->GetWindVector();
            Part.Wind.x *= ppn.WindFactor;
            Part.Wind.y *= ppn.WindFactor;
            Part.Wind.z *= ppn.WindFactor;
        }


        if (PS_ElapsedTime)
        {
            // we only need to run this if some time has elapsed
            float gravity, accel;

            gravity = PS_EvalTimedFloat(Part.life, Part.GravityStage, ppn.gravityStages, ppn.gravity);
            accel = PS_EvalTimedFloat(Part.life, Part.AccelStage, ppn.accelStages, ppn.accel);


            if (ppn.simpleDrag)
            {
                float Drag_x_Time = ppn.simpleDrag * PS_ElapsedTime; // COBRA - RED - Cached same Value

                Part.vel.x -= Part.vel.x * Drag_x_Time;
                Part.vel.y -= Part.vel.y * Drag_x_Time;
                Part.vel.z -= Part.vel.z * Drag_x_Time;
            }

            // update velocity values
            Part.vel.z += (32 * PS_ElapsedTime) * gravity;

            if (accel)
            {
                float fps = (accel * PS_ElapsedTime);
                float d = sqrt(Part.vel.x * Part.vel.x + Part.vel.y * Part.vel.y + Part.vel.z * Part.vel.z);
                fps = d + fps;

                if (fps < 0)
                    fps = 0;

                if (d)
                {
                    fps /= d;
                    Part.vel.x *= fps;
                    Part.vel.y *= fps;
                    Part.vel.z *= fps;
                }
            }


            Part.pos.x += Part.vel.x * PS_ElapsedTime;
            Part.pos.y += Part.vel.y * PS_ElapsedTime;
            Part.pos.z += Part.vel.z * PS_ElapsedTime;

            if (Part.pos.z >= Part.GroundLevel)
            {
                Part.pos.z = Part.GroundLevel;
                Part.vel.z *= -ppn.bounce * PRANDFloatPos();

                if (ppn.dieOnGround and Part.lifespan > age) // this will make the particle die
                {
                    // will also run the emitters

                    Part.lifespan = age;
                }

                float fps = (ppn.groundFriction * PS_ElapsedTime * (0.5f + 0.5f * PRANDFloatPos()));
                float d = sqrt(Part.vel.x * Part.vel.x + Part.vel.y * Part.vel.y + Part.vel.z * Part.vel.z);
                fps = d + fps;

                if (fps < 0)
                    fps = 0;

                if (d)
                {
                    Part.vel.x = (Part.vel.x / d) * fps;
                    Part.vel.y = (Part.vel.y / d) * fps;
                }
            }

            // Wind, affects anything having a Z velocity
            if (Part.WindAffected)
            {
                /*// if not grounded, apply wind
                Part.pos.x += Part.Wind.x * PS_ElapsedTime;
                Part.pos.y += Part.Wind.y * PS_ElapsedTime;*/
                // Go to reach the wind speed
                Part.vel.x += (Part.Wind.x - Part.vel.x)  * PS_ElapsedTime;
                Part.vel.y += (Part.Wind.x - Part.vel.y)  * PS_ElapsedTime;

            }

            // The Part Size
            if (ppn.drawType == PSDT_POLY)
                Part.Radius = PS_EvalTimedLinLogFloat(Part.life, Part.SizeStage, ppn.sizeStages,  ppn.size) * Part.SizeRandom;
        }

#ifdef DEBUG_PS_ID

        if (DrawableBSP::drawLabels/* and ppn->name[0]=='$'*/)
        {
            // Now compute the starting location for our label text
            ThreeDVertex labelPoint;
            float x, y;
            PS_Renderer->TransformPoint(&Part.pos, &labelPoint);

            if (labelPoint.clipFlag == ON_SCREEN)
            {
                x = labelPoint.x - 32; // Centers text
                y = labelPoint.y - 12; // Place text above center of object
                PS_Renderer->SetColor(ppn.name[0] == '$' ?  0xff0000ff : 0xffff0000);
                PS_Renderer->SetFont(2);
                PS_Renderer->ScreenText(x, y, ppn.name);
            }


        }

#endif


        /////////////////////// CLUSTER STUFF \\\\\\\\\\\\\\\\\\\\
        // execute only if clustering enabled
        if (Part.Cluster)
        {
            // Get the Cluster
            ClusterPosType &Cluster = (((ClusterPosType*)PS_Lists[PS_CLUSTERS_IDX].ObjectList)[Part.CLUSTER]);

            if (ppn.drawType == PSDT_POLY)
            {
                // if Already marked as IN Skip all checks
                if (Cluster.In or Cluster.Out) goto Skip;

                // 1st, check for Distance, if already out of visibility it's a CLUUSTER OUT OF FOV
                float Distance = TheDXEngine.DX2D_GetDistance((D3DXVECTOR3 *)&Part.pos);

                if (Distance > ppn.visibleDistance) Cluster.Out = true;

                // Check the whole cluster for light in range
                if (Distance < Part.Radius * LIGHT_SIZE_CX) Cluster.LightIn = true;

                // * DYNAMIC CLUSTERING *
                // Dynamic Cluster particles updates their extreme positions in the linked Cluster
                // to calulate the visibility in the FOV in PS_ClusterRun()
                if ( not Cluster.Static)
                {
                    // * DYNAMIC CLUSTERING *
                    // Setup XMM radius
                    XMMRadius.d3d.x = XMMRadius.d3d.y = XMMRadius.d3d.z = Part.Radius / 1.732f;
                    XMMPosMax.Xmm = _mm_add_ps(_mm_loadu_ps((float*)&Part.pos), XMMRadius.Xmm);
                    XMMPosMin.Xmm = _mm_sub_ps(_mm_loadu_ps((float*)&Part.pos), XMMRadius.Xmm);

                    // Setup min and Max Positions
                    _mm_store_ps((float*)&Cluster.TLFpos.Xmm, _mm_max_ps(_mm_load_ps((float*)&Cluster.TLFpos.Xmm), XMMPosMax.Xmm));
                    _mm_store_ps((float*)&Cluster.BRNpos.Xmm, _mm_min_ps(_mm_load_ps((float*)&Cluster.BRNpos.Xmm), XMMPosMax.Xmm));
                    _mm_store_ps((float*)&Cluster.TLFpos.Xmm, _mm_max_ps(_mm_load_ps((float*)&Cluster.TLFpos.Xmm), XMMPosMin.Xmm));
                    _mm_store_ps((float*)&Cluster.BRNpos.Xmm, _mm_min_ps(_mm_load_ps((float*)&Cluster.BRNpos.Xmm), XMMPosMin.Xmm));

                }
                else
                {
                    // STATIC clustering is calculated only once on 1st particle
                    // * STATIC CLUSTERING * only if yet assigned, JUST assign the position, radius is already in CLuster from PPN
                    if ( not Cluster.Alive)
                    {
                        Cluster.TLFpos.d3d.x = Part.pos.x, Cluster.TLFpos.d3d.y = Part.pos.y, Cluster.TLFpos.d3d.z = Part.pos.z;
                        // * STATIC CLUSTER , just use position and Radius and che Visibility
                        DWORD Visibility = TheDXEngine.ComputeSphereVisibility((D3DVECTOR*)&Part.pos, (float*)&Cluster.Radius);

                        if ( not Visibility) Cluster.In = true;

                        if (Visibility bitand D3DSTATUS_DEFAULT) Cluster.Out = true;
                    }
                }
            }

            // The cluster is used
            Cluster.Alive = true;
        }

        Skip:
        LastPtr = ptr;
        ptr = Part.NEXT;
    }

#ifdef DEBUG_NEW_PS_PARTICLES
    REPORT_VALUE("NEW PS PARTICLES", Count);
#endif
}



////////////////////// CLUSTERS MANAGEMENT \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void DrawableParticleSys::PS_ClustersReset(void)
{
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_CLUSTERS_IDX].ListEntry;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
        // Get the object
        ClusterPosType &Cluster = (((ClusterPosType*)PS_Lists[PS_CLUSTERS_IDX].ObjectList)[ptr]);

        // reset values
        Cluster.Alive = Cluster.In = Cluster.Out = Cluster.LightIn = false;
        Cluster.TLFpos.d3d.x = Cluster.TLFpos.d3d.y = Cluster.TLFpos.d3d.z = -99999999999.0f;
        *(float*)&Cluster.TLFpos.d3d.Flags = 0.0f; // Used as Radius
        Cluster.BRNpos.d3d.x = Cluster.BRNpos.d3d.y = Cluster.BRNpos.d3d.z = 99999999999.0f;

        ptr = Cluster.NEXT;

    }
}



// The CLUSTER Running fuction
void DrawableParticleSys::PS_ClustersRun(void)
{
    DWORD Visibility;
    XMMVector XMMPos, XMMDiv, XMMRadius;

    XMMDiv.d3d.x = XMMDiv.d3d.y = XMMDiv.d3d.z = 2.0f;

#ifdef DEBUG_NEW_PS_CLUSTERS
    DWORD Count = 0, ClusterIn = 0;
#endif
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_CLUSTERS_IDX].ListEntry, LastPtr = PS_NOPTR;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
        // Get the object
        ClusterPosType &Cluster = (((ClusterPosType*)PS_Lists[PS_CLUSTERS_IDX].ObjectList)[ptr]);

        if ( not Cluster.Alive)
        {
            ptr = PS_RemoveItem(PS_CLUSTERS_IDX, ptr, LastPtr);
            continue;
        }

        LastPtr = ptr;
        ptr = Cluster.NEXT;

#ifdef DEBUG_NEW_PS_CLUSTERS
        Count++;
#endif

        // if this cluster is already defined as Out or In, skip any calculation
#ifdef VISUALIZE_CLUSTERS

        if (Cluster.Out) continue;

        XMMPos = Cluster.TLFpos;
#else

        if (Cluster.Out or Cluster.In) continue;

#endif

        // * DYNAMIC CLUSTER, calculate radius and position
        if ( not Cluster.Static)
        {
            // calculate center of extreme coords
            XMMPos.Xmm = _mm_div_ps(_mm_add_ps(_mm_load_ps((float*)&Cluster.TLFpos), _mm_load_ps((float*)&Cluster.BRNpos)), XMMDiv.Xmm);
            // calculate the Radius
            XMMRadius.Xmm = _mm_sub_ps(_mm_load_ps((float*)&Cluster.TLFpos), XMMPos.Xmm);
            XMMRadius.Xmm = _mm_mul_ps(XMMRadius.Xmm, XMMRadius.Xmm);
            float Radius = sqrtf(XMMRadius.d3d.x + XMMRadius.d3d.y + XMMRadius.d3d.z);
            // check visibility
            Visibility = TheDXEngine.ComputeSphereVisibility((D3DVECTOR*)&XMMPos.d3d, &Radius);
#ifdef VISUALIZE_CLUSTERS
            Cluster.Radius = Radius;
#endif;
            // update results
            Cluster.In = Cluster.Out = false;

            if ( not Visibility) Cluster.In = true;

            if (Visibility bitand D3DSTATUS_DEFAULT) Cluster.Out = true;
        }



#ifdef VISUALIZE_CLUSTERS

        if ( not Cluster.Out) ClusterIn++;

        if (DrawableBSP::drawLabels)
        {
            D3DDYNVERTEX Quad[4];
            Quad[0].tu = Quad[0].tv = 0.0f;
            Quad[1].tu = Quad[1].tv = 0.0f;
            Quad[2].tu = Quad[2].tv = 0.0f;
            Quad[3].tu = Quad[3].tv = 0.0f;

            Quad[0].pos.x = Quad[1].pos.x = Quad[2].pos.x = Quad[3].pos.x = 0.f;
            Quad[0].pos.y = -Cluster.Radius;
            Quad[0].pos.z = -Cluster.Radius;
            Quad[1].pos.y = Cluster.Radius;
            Quad[1].pos.z = -Cluster.Radius;
            Quad[2].pos.y = Cluster.Radius;;
            Quad[2].pos.z = Cluster.Radius;
            Quad[3].pos.y = -Cluster.Radius;
            Quad[3].pos.z = Cluster.Radius;

            Quad[0].dwColour = Quad[1].dwColour = 0x40ff0000;
            Quad[2].dwColour = Quad[3].dwColour = 0x40ff0000;
            Quad[0].dwSpecular = Quad[1].dwSpecular = Quad[2].dwSpecular = Quad[3].dwSpecular = 0x40400000;
            // draw with BillBoard and declare as VISIBLE, the Visibility test was at function entry point ( DX2D_GetDistance()>0)
            TheDXEngine.DX2D_AddQuad(LAYER_AUTO, POLY_BB, (D3DXVECTOR3*)&XMMPos.d3d, Quad, Cluster.Radius, NULL);
        }

#endif
    }


#ifdef DEBUG_NEW_PS_CLUSTERS
    REPORT_VALUE("NEW PS CLUSTER", Count);
#endif
#ifdef VISUALIZE_CLUSTERS
    REPORT_VALUE("NEW PS CLUSTER IN", ClusterIn);
#endif
}


///////////////////////////////// TRAIL EMITTERS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// The Generating Function
TRAIL_HANDLE DrawableParticleSys::PS_AddTrail(int ID, Tpoint *Pos, PS_PTR OWNER, bool Run, float AlphaCx, float SizeCx)
{
    // Get a free slot
    PS_PTR ptr = PS_AddItem(PS_TRAILS_IDX);

    // security check
    if (ptr == PS_NOPTR) return NULL;

    // Get the pointer in the list
    TrailEmitterType &Trail = (((TrailEmitterType*)PS_Lists[PS_TRAILS_IDX].ObjectList)[ptr]);
    // Pointer to trail parametes
    PS_TPType &tpn = PS_TPN[ID];

    // Create the Handle
    TrailsHandle += PS_MAX_TRAILS;

    if ( not TrailsHandle) TrailsHandle = 1;

    Trail.Handle = TrailsHandle bitor ptr;
    // Thehandle can not be NULL

    // Assign Position
    Trail.Elapsed = Trail.Interval = tpn.Interval;
    Trail.Alive = Trail.StartUp = true;
    Trail.Run = Run;
    Trail.Split = Trail.Updated = false;
    Trail.CLUSTER = PS_NOPTR;
    Trail.OWNER = OWNER;
    Trail.ID = ID;
    Trail.LifeCx = tpn.LifeCx + PRANDFloat() * tpn.LifeCx * 0.4f;
    Trail.OffsetX = Trail.OffsetY = 0.0f;
    Trail.SizeCx = SizeCx, Trail.AlphaCx = AlphaCx;

    ParticleTextureNode *pt = tpn.SideTexture;
    DWORD SideTexHandle = pt->TexHandle;
    float Spare;
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 0, Trail.su[0], Spare);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 1, Trail.su[1], Spare);


    // Calculate max nodes from life of trail
    Trail.Nodes = (int)(tpn.LifeSpan / tpn.Interval);

    //always get even nodes number, for Odd/Even graphic stuff
    if (Trail.Nodes bitand 0x01) Trail.Nodes++;

    // allocate memory for such nodes
    Trail.TRAIL = (TrailSubPartType *)calloc(Trail.Nodes, sizeof(TrailSubPartType));
    // Start with node 0
    Trail.Entry = Trail.Last = 0;
    // Texture rating
    Trail.TexRate = tpn.TexRate;
    Trail.LastTexIndex = 0.0f;

    return Trail.Handle;
};


float Limit = 10;
float VLev = 0.2f;


#define DEBUG_NEW_PS_TRAILS

///////////////////////////////// TRAIL EMITTERS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void DrawableParticleSys::PS_TrailsClear(void)
{
    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_TRAILS_IDX].ListEntry, LastPtr = PS_NOPTR;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {
        TrailEmitterType &Trail = (((TrailEmitterType*)PS_Lists[PS_TRAILS_IDX].ObjectList)[ptr]);
        Trail.Handle = NULL;

        if (Trail.TRAIL) free(Trail.TRAIL);

        Trail.TRAIL = NULL;
        ptr = Trail.NEXT;
    }
}

// The running Function
void DrawableParticleSys::PS_TrailRun(void)
{
#ifdef DEBUG_NEW_PS_TRAILS
    DWORD TrailEmitters = 0;
#endif

    D3DXVECTOR3 Origin, SubPos;
    D3DXVECTOR3 Offset;
    D3DXVECTOR3 SV;

    // Get the List entry
    PS_PTR ptr = PS_Lists[PS_TRAILS_IDX].ListEntry, LastPtr = PS_NOPTR;

    // thru all the list
    while (ptr not_eq PS_NOPTR)
    {

#ifdef DEBUG_NEW_PS_TRAILS
        TrailEmitters ++;
#endif
        TrailEmitterType &Trail = (((TrailEmitterType*)PS_Lists[PS_TRAILS_IDX].ObjectList)[ptr]);
        PS_TPType &TPN = PS_TPN[Trail.ID];

        // If part is dead, remove it, all last stages have been already killed/exucuted
        if (Trail.OWNER not_eq PS_NOPTR and not (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[Trail.OWNER]).Alive) Trail.Alive = false;

        if ( not Trail.Run) goto Skip;

        // Check for emit time....
        Trail.Elapsed += PS_ElapsedTime;
        Trail.Wind = ((WeatherClass*)realWeather)->GetWindVector();


        if (Trail.Alive and Trail.OWNER not_eq PS_NOPTR)
        {
            Trail.Life +=  PS_ElapsedTime * Trail.LifeCx;

            if (Trail.Life >= 1.0f) Trail.Alive = false;
            else
            {
                ParticleNodeType &PN = (((ParticleNodeType*)PS_Lists[PS_PARTICLES_IDX].ObjectList)[Trail.OWNER]);
                Trail.Pos = PN.pos;
                float Life = max(Trail.Life, PN.life);
                Trail.Updated = true;
            }

        }

        if ( not PS_ElapsedTime) Trail.Updated = false;

        // THIS IS THE TRAIL ORIGIN, used in following calculations
        if (Trail.StartUp) Trail.Origin = *(D3DXVECTOR3*)&Trail.Pos;

        // Trail segments have position relative to the trail origin
        SubPos = *(D3DXVECTOR3*)&Trail.Pos - Trail.Origin;
        // Calculate present trail vector
        SV = *(D3DXVECTOR3*)&Trail.Pos - *(D3DXVECTOR3*)&Trail.LastPos;
        SV = RED_NormalizeVector(&SV);

        if (Trail.Alive and Trail.Updated and Trail.Elapsed >= Trail.Interval)
        {
            float Life = Trail.Life;


            float XRand, YRand;

            // * RANDOMIC STUFF *
            XRand = PRANDFloat() * TPN.RndLimit * TPN.RndStep;
            YRand = PRANDFloat() * TPN.RndLimit * TPN.RndStep;
            Trail.OffsetX += XRand;
            Trail.OffsetY += YRand;

            if (Trail.OffsetX >= TPN.RndLimit) Trail.OffsetX = TPN.RndLimit - XRand;

            if (Trail.OffsetY >= TPN.RndLimit) Trail.OffsetY = TPN.RndLimit - XRand;

            if (Trail.OffsetX <= -TPN.RndLimit) Trail.OffsetX = -TPN.RndLimit + XRand;

            if (Trail.OffsetY <= -TPN.RndLimit) Trail.OffsetY = -TPN.RndLimit + XRand;

            // The Offset based on the vector
            Offset.x = SV.y * Trail.OffsetX + SV.z * Trail.OffsetY;
            Offset.y = SV.z * Trail.OffsetX + SV.x * Trail.OffsetY;
            Offset.z = SV.x * Trail.OffsetX + SV.y * Trail.OffsetY;

            // EMIT THE TRAIL SUB PART
            PS_AddSubTrail(Trail.TRAIL[Trail.Last], Trail.ID, Trail.AlphaCx, Trail.SizeCx, (Tpoint*)&SubPos, &Trail.Wind, (Tpoint*)&Offset, Trail.CLUSTER);

            // Texture setting
            TrailSubPartType &Part = Trail.TRAIL[Trail.Last];

            // Get the texture coord lenght and divide it by the assigned texture rate for 100 feet
            float TexSeg = Trail.su[1] - Trail.su[0];

            // if the Trail StartUp ( 1st Node ), initialize a randomic texture start index
            if (Trail.StartUp or Trail.Split)
            {
                Trail.NextTexIndex = Trail.su[0] + TexSeg * PRANDFloatPos(), Trail.StartUp = false;;
                // ok, done
                Trail.Split = Trail.StartUp = false;
                // if coming from a Split or entry, mark the segment as entry
                Part.Entry = true;
            }

            Trail.TexStep = TexSeg / Trail.TexRate;

            // now get the extension for this segment + a 30% randomness
            Trail.TexStep *=  0.85f + 0.15f * PRANDFloat();
            // Setup a randomic direction
            Trail.TexStep *= (PRANDFloatPos() > 0.5f) ? 1.0f : -1.0f;


            // Assign Last Texture Coords
            Part.SideTexIndex = Trail.NextTexIndex;
            //Part.su[1] = Trail.NextTexIndex;
            Trail.LastTexIndex = Trail.NextTexIndex;

            // Ring the trail list
            Trail.Last++;

            if (Trail.Last >= Trail.Nodes) Trail.Last = 0;

            if (Trail.Entry == Trail.Last) Trail.Entry++;

            if (Trail.Entry >= Trail.Nodes) Trail.Entry = 0;

            Trail.Elapsed = 0;
            (*(D3DXVECTOR3*)&Trail.LastPos) = (*(D3DXVECTOR3*)&Trail.Pos);// + Offset;

            Trail.QuadListIndex = rand() % PS_MAXQUADRNDLIST;
            Trail.QuadListStep = rand() % 7 + 1;

        }

        // Get the header segment
        TrailSubPartType &Part = Trail.TRAIL[Trail.Last];


        // if here and Trail.Elapsed is more than twice the interval, means trail may have been
        // suspended and splitted...
        if ( not Trail.Split and Trail.Elapsed >= (Trail.Interval * 2))
        {
            // flag the split
            Part.Exit = Part.Split = true;
            // mark the trail for being in pause
            Trail.Split = true;
            // Ring the List
            Trail.Last++;

            if (Trail.Last >= Trail.Nodes) Trail.Last = 0;

            if (Trail.Entry == Trail.Last) Trail.Entry++;

            if (Trail.Entry >= Trail.Nodes) Trail.Entry = 0;
        }

        // Header - execute only if not suspended
        if ( not Trail.Split)
        {

            // The Offset based on the vector
            Offset.x = SV.y * Trail.OffsetX + SV.z * Trail.OffsetY;
            Offset.y = SV.z * Trail.OffsetX + SV.x * Trail.OffsetY;
            Offset.z = SV.x * Trail.OffsetX + SV.y * Trail.OffsetY;

            // UPDATE THE HEADER SUB PART
            if (Trail.Updated) PS_AddSubTrail(Trail.TRAIL[Trail.Last], Trail.ID, Trail.AlphaCx, Trail.SizeCx, (Tpoint*)&SubPos, &Trail.Wind, (Tpoint*)&Offset, Trail.CLUSTER);

            // Get the Last Emission distance
            float Distance = RED_Distance3D((D3DXVECTOR3*) &Trail.Pos, (D3DXVECTOR3*) &Trail.LastPos);

            // Calculate texture extension based on distance
            Trail.NextTexIndex = Trail.LastTexIndex + (Trail.TexStep * Distance / 100.0f);
            // Assign New Texture Coords
            Part.SideTexIndex = Trail.NextTexIndex;
            // Part.su[1] = Trail.NextTexIndex;
            Part.QuadListIndex = Trail.QuadListIndex;
            Part.QuadListStep = Trail.QuadListStep ;
            Part.Pos = *(Tpoint*)&SubPos;

            if ( not Trail.Alive) Part.Exit = true;
        }

        // Make origin Camera centric
        TheDXEngine.DX2D_MakeCameraSpace(&Origin, &Trail.Origin);

        // Update the Trail if something to draw
        if (Trail.Entry not_eq Trail.Last) PS_SubTrailRun(Trail.TRAIL, Origin, Trail.Entry, Trail.Last, Trail.Nodes, Trail.ID);

        Skip:
        Trail.Updated = false;

        // If trail is waiting to die, kill any possible reference to it
        if ( not Trail.Alive) Trail.Handle = NULL;

        // check for dead, can be dead also when the trail time has elapsed since last Sub Part geneation
        if (( not Trail.Alive and Trail.Entry == Trail.Last) or Trail.Elapsed >= TPN.LifeSpan)
        {
            // Release memory
            Trail.Handle = NULL;
            free(Trail.TRAIL);
            Trail.TRAIL = NULL;
            ptr = PS_RemoveItem(PS_TRAILS_IDX, ptr, LastPtr);
            continue;
        }

        // Go for next Trail
        LastPtr = ptr;
        ptr = Trail.NEXT;
    }

#ifdef DEBUG_NEW_PS_TRAILS
    //REPORT_VALUE("NEW PS Trail Emitters", TrailEmitters);
#endif
}



///////////////////////////////// TRAIL SUBPARTS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// The Generating Function
void DrawableParticleSys::PS_AddSubTrail(TrailSubPartType &Part, int ID, float AlphaCx, float SizeCx, Tpoint *Pos, Tpoint *Wind, Tpoint *Offset, PS_PTR CLUSTER)
{

    // Pointer to trail parametes
    // Part.TPN=ID;
    PS_TPType &TPN = PS_TPN[ID];

    // setup parameters
    Part.CLUSTER = CLUSTER;
    Part.Pos = *Pos;
    Part.Wind = *Wind;

    if (Offset) Part.Offset = *Offset;
    else Part.Offset.x = Part.Offset.y = Part.Offset.z = 0.0f;

    Part.Wind.z += TPN.Weight * (0.9f + PRANDFloatPos() * 0.1f);

    Part.SizeRnd = SizeCx + PRANDFloat() * 0.2f;
    Part.ColorRnd = 0.8f + PRANDFloat() * 0.2f;
    Part.AlphaRnd = AlphaCx;
    Part.Birth = PS_RunTime;
    Part.Split = Part.Entry = Part.Exit = false;

    ParticleTextureNode *pt = TPN.SideTexture;

    // check if depending on group texture
    if (TPN.GroupFlags bitand GRP_TEXTURE2)
        // if depending from a group, ask for a random texture node
        pt = (ParticleTextureNode*)(((ParticleGroupNode*)(TPN.SideTexture))->GetRandomArgument());

    Part.QuadListIndex = rand() % PS_MAXQUADRNDLIST;
    Part.QuadListStep = rand() % 7 + 1;

}




bool DebugTrail = false;
float Kx = 10.0f;



// Given a segment btw points A and B relative to origin 0,0,0, a cicle of center origin and RADIUS R
// and then a point K thru segment AB as contact btw the cicle and the segment itself...
// this function returns a negative number if no contact of the passsed AB segment is in radius of
// a circle center 0,0,0 relative to A and B and Radius R, eventually it passes back 0.0 if A inside the Circle,
// or a collision distance K from A thru AB if a collision with segment AB is present
float GetCollisionPoint(D3DXVECTOR3 A, D3DXVECTOR3 B, float R)
{
    D3DXVECTOR3 A_B = B - A;
    // AB lenght
    float AB = RED_Lenght3D(&A_B);
    // Get the distance from A
    float AC = RED_Lenght3D(&A);

    // if A inside radius, return 0 offset as contact from A
    if (AC <= R) return 0.0f;

    // else check if a contact with segment calculating nearest segment point distance as projecttion point P on AB
    // AB Vector
    // the AC Vector
    D3DXVECTOR3 A_C = -A;
    // the dot product AB*AC
    float ABxAC = D3DXVec3Dot(&A_C, &A_B);
    // the distance AP
    float AP = ABxAC / AB;

    // if AP more than AB, the P is out of segment AB, so, step is the full segment
    if (AP < 0) return AB;

    // the distance CP
    float CP = sqrtf(AC * AC - AP * AP);

    // if CP more than R, no contact, so, full segment
    if (CP > R) return AB;

    // ok, now calculate the AK distance thru AB vector to get contact point K with circle of radius R
    float AK = AP - sqrtf(R * R - CP * CP);
    //if(AK > AB) return AB;
    // return the result
    return AK;
}



void DrawableParticleSys::PS_SubTrailRun(TrailSubPartType *Trail, D3DXVECTOR3 &Origin, DWORD &Entry, DWORD &Exit, DWORD Elements, PS_PTR tpn)
{
    float Life, Alpha;
    XMMVector XMMPos, Div2;
    float TimeStep = PS_ElapsedTime, LastSize;
    bool TrailValid = false, EntrySegment = true, TrailCompleted = false, RecalcP1 = true, LineMode = false;
    DWORD Index = Entry;
    psRGBA Color, LastColor, ColorStep, ColorNew;

    // Get the Trail parameter Node
    PS_TPType &TPN = PS_TPN[tpn];
    float BaseSize = TPN.Size[0];
    float SizeRate = TPN.SizeRate;
    float LifeSpan = TPN.LifeSpan;
    float BiasCx = 1.0f ;//+ ( 1.0f / TheDXEngine.LODBiasCx()) / 10.0f;
    float FragRadius = TPN.FragRadius * BiasCx;
    float FragMixOut = FragRadius * 0.75f;
    float FragMixSpan = FragRadius - FragMixOut;
    float LineDistance = TPN.LineDistance * BiasCx;
    float OutDistance = TPN.VisibleDistance;
    float IntegrateDistance = TPN.IntegrateDistance;
    DWORD Integrate = 0;

    // * GEOMETRIC STUFF *
    D3DDYNVERTEX Quad[4];
    D3DDYNVERTEX Side[2];
    D3DXVECTOR3 S0, S1, ST, SL, V1, SVector;
    DWORD dwColor;
    float LastSU, LastSegAlpha, ST_Distance, S0_Distance, SegmentSize, S1_Distance;
    bool Flip = false;


    // Setup the Faces with default values
    memset(Quad, 0, sizeof(Quad));
    memset(Side, 0, sizeof(Side));

    // Link here the Texture and get its U/V Coord
    // The BB Surfaces texture
    ParticleTextureNode *pt = TPN.Texture;
    DWORD TexHandle = pt->TexHandle;
    float su[4], sv[4];
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 0, Quad[0].tu, Quad[0].tv);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 1, Quad[1].tu, Quad[1].tv);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 2, Quad[2].tu, Quad[2].tv);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 3, Quad[3].tu, Quad[3].tv);

    // The SIDE Texture
    pt = TPN.SideTexture;
    DWORD SideTexHandle = pt->TexHandle;
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 0, su[0], sv[0]);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 1, su[1], sv[1]);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 2, su[2], sv[2]);
    TheDXEngine.DX2D_GetTextureUV(pt->TexItem, 3, su[3], sv[3]);

    // The used divisor
    Div2.d3d.x = Div2.d3d.y = Div2.d3d.z = 2.0f;

    // thru all the list
    while ( not TrailCompleted /* and PS_SubTrails<14200*/)
    {

#ifdef DEBUG_NEW_PS_TRAILS
        PS_SubTrails++;
#endif

        // Check if last node, assert it
        if (Index == Exit) TrailCompleted = true;

        // Get the node data, and check for index rollover
        TrailSubPartType &Part = Trail[Index++];

        // * NOTE * Index points to the NEXT item in the segments list, not the present one
        if (Index >= Elements) Index = 0;

        // Calculate S0 Position
        S0 = *((D3DXVECTOR3*)&Part.Pos) + Origin;
        // The Distance from S0 ( AKA Pos )
        S0_Distance = RED_Lenght3D(&S0);

        // if out of View Distance, go to next node
        if (S0_Distance > OutDistance)
        {
            TrailValid = false;
            EntrySegment = false, RecalcP1 = true;
            continue;
        }



        //***************************** LIFE STUFF *****************************************

        // get elapsed time in mSeconds
        float LifeStep = (float)(PS_RunTime - Part.Birth) / 1000.0f;
        // Calculate Lifespan from 0.0 to 1.0
        Life =  LifeStep / LifeSpan;

        // check if life is over, if so, skip
        if (Life >= 1.0f)
        {
            if ( not TrailCompleted) Entry = Index;

            TrailValid = LineMode = false, RecalcP1 = true;
            Integrate = 0;
            goto Skip;
        }

        // the logarithmic life stuff...
        float InvLogLife = SizeArray[F_I32(((float)SIZE_ARRAY_ITEMS * Life))];
        // the Wind step applied to the segment
        float WindStep = LifeStep * InvLogLife;//LifeStep * min(1.0f, LifeStep/0.003f) ;

        //**********************************************************************************

        // Update Size
        float   Size = (BaseSize + SizeRate * InvLogLife) * Part.SizeRnd;

        // Update position...
        S0.x += Part.Offset.x * InvLogLife + Part.Wind.x * WindStep;
        S0.y += Part.Offset.y * InvLogLife + Part.Wind.y * WindStep;
        S0.z += Part.Offset.z * InvLogLife + Part.Wind.z * WindStep;

        // Update Color
        Color.r = (TPN.Color[0].r + TPN.ColorRate.r * InvLogLife * Part.ColorRnd) * PS_HiLightCx.r;
        Color.g = (TPN.Color[0].g + TPN.ColorRate.g * InvLogLife * Part.ColorRnd) * PS_HiLightCx.g;
        Color.b = (TPN.Color[0].b + TPN.ColorRate.b * InvLogLife * Part.ColorRnd) * PS_HiLightCx.b;

        Alpha = TPN.Alpha * Part.AlphaRnd * FadeArray[F_I32(((float)FADE_ARRAY_ITEMS * Life))];

        if (Alpha < 0.0f) Alpha = 0.0f;

        if (Alpha > 255.0f) Alpha = 255.0f;

        // Overrides for last point in list
        if (TrailCompleted) Size = BaseSize * Part.SizeRnd/*, Alpha=0.0f*/;

        // Overrides for 1st point in list
        if (Part.Entry or Part.Split or Part.Exit) Size = 0.0f, Alpha = 0.0f, Integrate = 0;

        // if entry, no trail is valid now
        if (Part.Entry) TrailValid = LineMode = false, RecalcP1 = true;

        Color.a = Alpha;


        // Execute only if a trail is Valid ( i.e. we have at least 2 nodes to draw a segment )
        if (TrailValid)
        {

            //*************************** START WITH VISIBILITY CHECKS ****************************************


            // if to be integrated, Skip the requested nodes
            /* if(Integrate and not TrailCompleted){
             // 1 less, as the index is alerady postincremented in upper function code
             Integrate-=1;
             // theck for Exit Index as limit
             if(Exit >= Index){
             // if exit has an higher value then Exit is the Limit
             Index += Integrate;
             } else {
             // if exit has a lower value then Elemts is the Limit
             Index += Integrate;
             if(Index>=Elements) Index-=Elements;
             }
            // Used to calculate the effective expected number of Segments
             PS_SubTrails+=Integrate;
             // Exit check, never close an integration on the EXIT segment, it has Alpha = 0
             // so there is a risk to get invisble segments
             if(Index>Exit) Index=Exit - 1;
             // correct any underflow
             if(Index==-1) Index = Elements-1;
             // reset skip value
             Integrate=0;
             // loop
             continue;
             }*/

            // if to be integrated, Skip the requested nodes
            if (Integrate and not TrailCompleted and Index not_eq Exit)
            {
                // 1 less, as the index is alerady postincremented in upper function code
                Integrate -= 1;
                // the new incoming index
                DWORD NewIndex = Index;

                // execute steps
                while (Integrate--)
                {
                    TrailSubPartType &Part = Trail[NewIndex];

                    // an integration can not end on trail ends
                    // if segment is a split/exit, then stop here and keep old Index as good value
                    if (Part.Split or Part.Exit or NewIndex == Exit) break;

                    // ok, the step
                    Index = NewIndex;
#ifdef DEBUG_NEW_PS_TRAILS
                    PS_SubTrails++;
#endif
                    // check new incoming segment
                    NewIndex++;

                    if (NewIndex >= Elements) NewIndex = 0;
                }

                // reset skip value in any case
                Integrate = 0;
                // loop
                continue;
            }

            // Calculate the next integration step
            if (S0_Distance > IntegrateDistance) Integrate = F_I32((S0_Distance - IntegrateDistance) / IntegrateDistance);

            // ******************************** CHECK FOR SEGMENT VISIBILITY *********************************

            // * CALCULATE THE TRAIL VECTOR *
            SVector = S0 - S1;
            // Get the Trail Lenght
            SegmentSize = RED_Lenght3D(&SVector);
            // Get the middle of present segment extremities, that is the position of the segment
            XMMPos.Xmm = _mm_div_ps(_mm_add_ps(_mm_loadu_ps((float*)&S0), _mm_loadu_ps((float*)&S1)), _mm_load_ps((float*)&Div2.Xmm));

            // Skip if the segment is not fully visible
            if ( not TheDXEngine.DX2D_GetVisibility((D3DXVECTOR3*)&XMMPos.d3d, SegmentSize / 2.0f, CAMERA_VERTICES))
            {
                RecalcP1 = true;
                goto Skip;
            }


            //******************************** CHECK IF JUST A LINE  ****************************************

            // Get the NORMALIZED angle btw View and Vector
            float XAngle = F_ABS(SVector.x * S0.x + SVector.y * S0.y + SVector.z + S0.z);
            XAngle = min(1.0f,  XAngle / (SegmentSize * S0_Distance));
            XAngle = PS_NORM_ASIN(XAngle);


            //******************************** LINE MODE DRAWING **********************************************
            // Check if a line is enough
            if (S0_Distance > LineDistance)
            {
                // Alpha is more intense is segment is seen frontally, and less intense with Distance
                Color.a = min(255.0f, Color.a * (1.0f + 1.0f * XAngle * LineDistance / S0_Distance) * ((OutDistance - S0_Distance) / (OutDistance - LineDistance)));

                if (DebugTrail)
                {
                    Flip xor_eq true;
                    Color.r = 255.0f;
                    Color.g = Flip ? 0.0f : 255.0f;
                    Color.b = Flip ? 0.0f : 255.0f;
                    Color.a = 255.0f;
                }

                // Draw only if any of the vertices has Alpha > 1
                if (Color.a > 1.0 or LastColor.a > 1.0f)
                {
                    // if not a Line already started
                    if ( not LineMode)
                    {
                        // Create the entry point for the line
                        Side[0].pos = *(D3DVECTOR*)&S1, Side[0].dwColour = F_TO_UARGB(LastColor.a, LastColor.r, LastColor.g, LastColor.b);
                        // Put the vertex in the Buffer
                        TheDXEngine.DX2D_AddSingle(LAYER_AUTO, CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE bitor POLY_LINE bitor TAPE_ENTRY, (D3DXVECTOR3*)&XMMPos.d3d, Side, 0, NULL);
                        // now we are in Line Mode
                        LineMode = true;
                    }

                    // Create the Next point for the line
                    Side[0].pos = *(D3DVECTOR*)&S0, Side[0].dwColour = F_TO_UARGB(Color.a, Color.r, Color.g, Color.b);
                    // Put the vertex in the Buffer
                    TheDXEngine.DX2D_AddSingle(LAYER_AUTO, CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE bitor POLY_LINE bitor POLY_TAPE, (D3DXVECTOR3*)&XMMPos.d3d, Side, 0, NULL);
                }

                // The go to next segment
                EntrySegment = false;
                RecalcP1 = true;
                goto Skip;
            }


            //******************************** POLY MODE DRAWING **********************************************
            // if here, we are no more in Line Mode;
            LineMode = false;


            //******************************** IF HERE, WE HAVE A TAPE/BB SURFACE ****************************

            /* // Assign BB Texture
             Quad[0].tu=tu[0], Quad[0].tv=tv[0];
             Quad[1].tu=tu[1], Quad[1].tv=tv[1];
             Quad[2].tu=tu[2], Quad[2].tv=tv[2];
             Quad[3].tu=tu[3], Quad[3].tv=tv[3];
            */

            float TexStep, NewTex, SizeStep, NewSize;
            float SegmentStep, SegmentDone;

            // Check if segment or part of it inside frag radius
            SegmentStep = GetCollisionPoint(S1, S0, FragRadius);
            SegmentDone = 0;

            // if not SegmentStep, means the S1 point is INSIDE RANGE, so not a 1st step needed
            // calculate the step by usual stepping rule
            if ( not SegmentStep) SegmentStep = Size / 2.0f + 0.5f; //min( Size / 2.0f + 0.5f, max( RED_Lenght3D(&S0) / FragRadius * 10, 1.0f));

            // if we are going to Step calculate Step advances for parameters
            if (SegmentStep < SegmentSize)
            {
                // Calculate Texture U Steps...
                TexStep = (Part.SideTexIndex - LastSU) / SegmentSize;
                SizeStep = (Size - LastSize) / SegmentSize;
                ColorStep.a = (Color.a - LastColor.a) / SegmentSize;
                ColorStep.r = (Color.r - LastColor.r) / SegmentSize;
                ColorStep.g = (Color.g - LastColor.g) / SegmentSize;
                ColorStep.b = (Color.b - LastColor.b) / SegmentSize;
            }

            DWORD Ix = 0;

            // we start stepping from S1 and its parameters
            ST = S1;
            NewTex = LastSU;
            NewSize = LastSize;
            ColorNew = LastColor;

            // Normalize the direction Vector
            SVector = RED_NormalizeVector(&SVector);

            // for each requested Segment
            while (SegmentDone < SegmentSize)
            {

                // update last point
                SL = ST;

                // if at the end, go forward to S0
                if ((SegmentDone + SegmentStep) >= SegmentSize)
                {
                    ST = S0;
                    ColorNew = Color;
                    NewSize = Size;
                    NewTex = Part.SideTexIndex;
                }
                else
                {
                    //calculate new position
                    ST.x += SVector.x * SegmentStep;
                    ST.y += SVector.y * SegmentStep;
                    ST.z += SVector.z * SegmentStep;
                    NewTex += TexStep * SegmentStep;
                    NewSize += SizeStep * SegmentStep;
                    ColorNew.a += ColorStep.a * SegmentStep;
                    ColorNew.r += ColorStep.r * SegmentStep;
                    ColorNew.g += ColorStep.g * SegmentStep;
                    ColorNew.b += ColorStep.b * SegmentStep;
                    // Step thru segment
                }


                // Update the done step
                SegmentDone += SegmentStep;


                ST_Distance = RED_Lenght3D(&ST);
                SegmentStep = (ST_Distance >= (FragRadius * 1.02f)) ? SegmentSize : SegmentStep = Size / 2.0f + 0.5f;  //min( Size / 2.0f + 0.5f, max( ST_Distance / FragRadius * 10, 1.0f));

                // NOW, SL HOLDS PREVIOUS POINT AND ST THE INCOMING ONE



                // Update Alpha based on distance
                float SegAlpha, AlphaMixCx;//, AlphaMixCx3;
                // Update the Alpha Mix CX
                AlphaMixCx = ((ST_Distance - FragMixOut) / FragMixSpan);
                // to be used Cubed
                //AlphaMixCx3 = AlphaMixCx * AlphaMixCx;
                // to be used squared
                AlphaMixCx *= AlphaMixCx;

                if (ST_Distance >= FragRadius) SegAlpha = min(255.0f, ColorNew.a * (1.0f + FragRadius / ST_Distance) * (1.0f + XAngle));
                else if (ST_Distance >= FragMixOut) SegAlpha = min(255.0f, ColorNew.a * 2 * AlphaMixCx * (1.0f + XAngle));

                //************************************* DRAW THE TAPE IF OUTSIDE MIXOUT DISTANCE *****************************************
                if (ST_Distance >= FragMixOut)
                {

                    if (DebugTrail)
                    {
                        Flip xor_eq true;
                        ColorNew.r = Flip ? 0.0f : 255.0f;
                        ColorNew.g = 255.0f;
                        ColorNew.b = Flip ? 0.0f : 255.0f;
                        LastColor = ColorNew;
                    }

                    // Get the middle of present segment extremities, that is the position of the segment
                    XMMPos.Xmm = _mm_div_ps(_mm_add_ps(_mm_loadu_ps((float*)&ST), _mm_loadu_ps((float*)&SL)), _mm_load_ps((float*)&Div2.Xmm));

                    // if this is the START OF A TAPE, Means, we have no previous node to link to and to get parameters from
                    // so, calculate if from scratch
                    if (RecalcP1)
                    {

                        RecalcP1 = false;

                        // if not entry segment, we already have an Alpha Value
                        if ( not EntrySegment) LastSegAlpha = LastColor.a;

                        // * CALCULATE THE NORMAL VECTOR BTW CAMERA ( the origin )AND THE SL POINT *
                        V1 = RED_Normal(SVector, SL);

                        // Assign directly parameters to the Segment end side
                        Side[0].pos.x = V1.x * LastSize + SL.x, Side[0].pos.y = V1.y * LastSize + SL.y, Side[0].pos.z = V1.z * LastSize + SL.z;
                        Side[1].pos.x = V1.x * -LastSize + SL.x, Side[1].pos.y = V1.y * -LastSize + SL.y, Side[1].pos.z = V1.z * -LastSize + SL.z;

                        Side[0].tu = LastSU, Side[0].tv = sv[1];
                        Side[1].tu = LastSU, Side[1].tv = sv[2];

                        Side[0].dwColour = Side[1].dwColour = F_TO_UARGB(LastSegAlpha, LastColor.r, LastColor.g, LastColor.b);

                        // Draw the 2 side surfaces
                        if (DebugTrail)
                        {
                            TheDXEngine.DX2D_AddBi(LAYER_AUTO, CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE bitor POLY_TAPE bitor TAPE_ENTRY, (D3DXVECTOR3*)&XMMPos.d3d, Side, SegmentSize, NULL);
                        }
                        else
                            TheDXEngine.DX2D_AddBi(LAYER_AUTO, CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE bitor POLY_TAPE bitor TAPE_ENTRY, (D3DXVECTOR3*)&XMMPos.d3d, Side, SegmentSize, SideTexHandle);

                    }

                    // ********************** SETUP PRESENT POINT ST PARAMETERS ****************************************************

                    // * CALCULATE THE NORMAL VECTOR BTW CAMERA ( the origin )AND THE ST POINT *
                    V1 = RED_Normal(SVector, ST);

                    Side[0].pos.x = V1.x * NewSize + ST.x;
                    Side[0].pos.y = V1.y * NewSize + ST.y;
                    Side[0].pos.z = V1.z * NewSize + ST.z;

                    Side[1].pos.x = V1.x * -NewSize + ST.x;
                    Side[1].pos.y = V1.y * -NewSize + ST.y;
                    Side[1].pos.z = V1.z * -NewSize + ST.z;

                    // Assign drawing data
                    Side[0].tu = NewTex, Side[0].tv = sv[0];
                    Side[1].tu = NewTex, Side[1].tv = sv[3];

                    dwColor = (DebugTrail) ? F_TO_UARGB(255.0f, ColorNew.r, ColorNew.g, ColorNew.b) : F_TO_UARGB(SegAlpha, ColorNew.r, ColorNew.g, ColorNew.b);

                    Side[0].dwColour = Side[1].dwColour = dwColor;

                    // Draw the 2 side surfaces
                    if (DebugTrail)
                    {
                        TheDXEngine.DX2D_AddBi(LAYER_AUTO, CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE bitor POLY_TAPE, (D3DXVECTOR3*)&XMMPos.d3d, Side, SegmentSize, NULL);
                    }
                    else
                        TheDXEngine.DX2D_AddBi(LAYER_AUTO, CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE bitor POLY_TAPE, (D3DXVECTOR3*)&XMMPos.d3d, Side, SegmentSize, SideTexHandle);
                }
                else
                    RecalcP1 = true;

                LastSegAlpha = SegAlpha;
                float Rcos, Rsin;
                DWORD QuadIdx = (Part.QuadListIndex + Ix * Part.QuadListStep) % PS_MAXQUADRNDLIST;


                if (ST_Distance <= FragRadius)
                {

                    if (ST_Distance <= FragMixOut) Alpha = ColorNew.a; //(ST_Distance > FragRadius) ? 0 : min(255, ColorNew.a * (2.0f - ST_Distance / FragRadius));
                    else Alpha = ColorNew.a * (1.0f  - AlphaMixCx/*3*/);

                    Rcos = PS_QuadRndList[QuadIdx].RotCx.cos * NewSize;
                    Rsin = PS_QuadRndList[QuadIdx].RotCx.sin * NewSize;

                    Quad[0].pos.y = Rcos;
                    Quad[0].pos.z = Rsin;
                    Quad[1].pos.y = Rsin;
                    Quad[1].pos.z = -Rcos;
                    Quad[2].pos.y = -Rcos;
                    Quad[2].pos.z = -Rsin;
                    Quad[3].pos.y = -Rsin;
                    Quad[3].pos.z = Rcos;

                    Quad[0].pos.x = Quad[1].pos.x = Quad[2].pos.x = Quad[3].pos.x = 0.0f;

                    Quad[0].dwColour = Quad[1].dwColour = Quad[2].dwColour = Quad[3].dwColour = DebugTrail ? F_TO_UARGB(255, 0, 0, 255) : F_TO_UARGB(Alpha, ColorNew.r, ColorNew.g, ColorNew.b);

                    // Draw the ROMB
                    if (DebugTrail)
                    {
                        TheDXEngine.DX2D_AddQuad(LAYER_AUTO, POLY_BB bitor CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE, &ST, Quad, NewSize, NULL);
                    }
                    else
                        TheDXEngine.DX2D_AddQuad(LAYER_AUTO, POLY_BB bitor CAMERA_VERTICES bitor POLY_VISIBLE bitor CALC_DISTANCE, &ST, Quad, NewSize, /*Part.*/TexHandle);
                }


                /* // ok, next is no more an entry
                 EntrySegment=false;*/
                // Color Alpha must be updated with segments one, as it is connected to next segment
                LastColor = ColorNew;
                // Copy Last Point data
                LastSU = NewTex, LastSize = NewSize, Ix++, LastColor = ColorNew;

            }

            LastSU = Part.SideTexIndex;
            Color = ColorNew;
        }

        // if here, still a trail node is valid
        TrailValid = true;

        Skip:
        // ok, next is no more an entry
        EntrySegment = false;
        S1_Distance = S0_Distance;
        S1 = S0;
        LastSU = Part.SideTexIndex;
        LastColor = Color;
        LastSize = Size;
    }



}






/////////////////////////////////////////////////// TRAILS PUBLIC CALLS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


// This function automatically updates or creates a trail
TRAIL_HANDLE DrawableParticleSys::PS_EmitTrail(TRAIL_HANDLE Handle, int TrailId, float x, float y, float z, float AlphaCx, float SizeCx)
{
    // RV - I-Hawk - CTD fix on vortex trails. probably caused by bad conversion from DWORD to int
    // So this will cause the emitter to cut and start another trail, but won't be noticed anyway...
    if (TrailId < 0 or TrailId > TRAIL_MAX - 1)
    {
        return NULL;
    }

    // Get the trail Number form the Handle, and the corresponding trails ID
    DWORD TrailNr = Handle bitand (PS_MAX_TRAILS - 1), ID = PS_TrailsID[TrailId];

    // Check it is an acceptable Trail Refernce
    if (Handle and TrailNr < PS_MAX_TRAILS)
    {
        // Get the trail
        TrailEmitterType &Trail = (((TrailEmitterType*)PS_Lists[PS_TRAILS_IDX].ObjectList)[TrailNr]);

        // if Handle ok Update the Emitter
        if (Trail.Handle == Handle)
        {
            Trail.Alive = true;
            Trail.Run = true;
            Trail.Updated = true;
            Trail.Pos.x = x, Trail.Pos.y = y, Trail.Pos.z = z;
            Trail.AlphaCx = AlphaCx;
            Trail.SizeCx = SizeCx;

            // Return the Handle
            return Handle;
        }
    }

    // if Here, we need to create a new trail
    Tpoint Pos = {x, y, z};
    // Return the Handle for the newly created Trail
    return PS_AddTrail(ID, &Pos, PS_NOPTR, false, AlphaCx, SizeCx);
}



void DrawableParticleSys::PS_KillTrail(TRAIL_HANDLE Handle)
{
    // Get the trail Number form the Handle, and the corresponding trails ID
    DWORD TrailNr = Handle bitand (PS_MAX_TRAILS - 1);

    // Check it is an acceptable Trail Refernce
    if (Handle and TrailNr < PS_MAX_TRAILS)
    {
        // Get the trail
        TrailEmitterType &Trail = (((TrailEmitterType*)PS_Lists[PS_TRAILS_IDX].ObjectList)[TrailNr]);

        // if same Handle, kill the Trail
        if (Trail.Handle == Handle)
        {
            Trail.Alive = false;
            Trail.Handle = NULL;
        }
    }
}





// THE PS PARSING MAIN FUNCTION
void DrawableParticleSys::PS_Exec(class RenderOTW *renderer)
{
    //START_PROFILE("New PS");

    PS_Renderer = renderer;

    // Update Timing Stuff
    PS_RunTime = TheTimeManager.GetClockTime();
    PS_ElapsedTime = (float)(PS_RunTime - PS_LastTime) * .001f;
    PS_LastTime = PS_RunTime;

    // Setup Colors
    TheTimeOfDay.GetTextureLightingColor(&PS_HiLightCx);

    // Setup type of rendering
    PS_NVG = PS_TV = false;

    switch (TheDXEngine.GetState())
    {
        case DX_TV:
                PS_TV = true;

            if ((g_bGreyMFD) and ( not bNVGmode))
                PS_HiLightCx.r = 0.2f, PS_HiLightCx.g = 0.2f, PS_HiLightCx.b = 0.2f, PS_LoLightCx = PS_HiLightCx;
            else
                PS_HiLightCx.r = 0.0f, PS_HiLightCx.g = 1.0f, PS_HiLightCx.b = 0.0f, PS_LoLightCx = PS_HiLightCx;

            break;

        case DX_NVG:
                PS_NVG = true;
            PS_HiLightCx.r = 0.0f, PS_HiLightCx.g = 1.0f, PS_HiLightCx.b = 0.0f, PS_LoLightCx = PS_HiLightCx;
            break;

        default:
                PS_NVG = PS_TV = false;
            PS_LoLightCx.r = PS_HiLightCx.r * 0.68f, PS_LoLightCx.g = PS_HiLightCx.g * 0.68f, PS_LoLightCx.b = PS_HiLightCx.b * 0.68f;
    }


    // Reset clusters Stuff
    PS_ClustersReset();

    // Parse for Patticles as 1st
    PS_ParticleRun();
#ifdef DEBUG_NEW_PS_TRAILS
    //START_PROFILE("NEW PS TRAILS RUN");
    PS_SubTrails = 0;
#endif
    // Trails
    PS_TrailRun();

    // PS_SubTrailRun();
#ifdef DEBUG_NEW_PS_TRAILS
    //REPORT_VALUE("NEW PS TRAILS", PS_SubTrails);
    //STOP_PROFILE("NEW PS TRAILS RUN");
#endif

    // Update cluster visibility
    PS_ClustersRun();

#ifdef DEBUG_NEW_PS_POLYS
    //START_PROFILE("NEW PS POLY TIME");
#endif
    // Polys
    PS_PolyRun();
#ifdef DEBUG_NEW_PS_POLYS
    //STOP_PROFILE("NEW PS POLY TIME");
#endif

    // Emitters
    PS_EmitterRun();

    // Lights
    PS_LightsRun();

    // Sounds
    PS_SoundRun();

    //STOP_PROFILE("New PS");
}











void DrawableParticleSys::ClearParticleList(void)
{
    DPS_Node *dps;

    paramList.Lock();
    dps = (DPS_Node *)dpsList.GetHead();

    while (dps)
    {
        dps->owner->ClearParticles();
        dps = (DPS_Node *)dps->GetSucc();
    }

    paramList.Unlock();
#ifdef USE_NEW_PS

    // Check if trails to release
    PS_TrailsClear();

    PS_ListsInit();
#endif

}

#if 1
// COBRA - RED - Returns False if any Failure
bool DrawableParticleSys::PS_LoadParameters(void)
{
    char buffer[1024];
    char path[_MAX_PATH];
    FILE *fp;

    // The PPN Pointer
    PS_PPType *ppn = NULL;
    PS_TPType *tpn = NULL;
    ParticleAnimationNode *pan = 0;
    ParticleGroupNode *GroupNode = NULL;
    DWORD LastPPN = 0, LastTPN = 0;


    int Counter;


    // COBRA - RED - Setup the LOG10 Array
    for (int a = 0; a < LOG10_ARRAY_ITEMS; a++)
    {
        Log10Array[a] = 1.0f - log10((float)a / LOG10_ARRAY_ITEMS * 9.0f + 1.0f);
    }

    for (float x = 0; x < FADE_ARRAY_ITEMS; x++)
    {
        FadeArray[(int)x] = log10(x + 10.0f) / (pow((x + 10), 2) / 1200 + 1.0f);
    }

    for (int a = 0; a < ASIN_ARRAY_ITEMS; a++)
    {
        ASinArray[a] = asinf((float)a / (ASIN_ARRAY_ITEMS - 1)) * 2 / PI;
    }

    for (float x = 0; x < SIZE_ARRAY_ITEMS; x++)
    {
        SizeArray[(int)x] = 1.0f - pow((((float)SIZE_ARRAY_ITEMS - x) / (float)SIZE_ARRAY_ITEMS), 9);
    }

    //  RED - Setup the Randomly rotate Quads
    for (int a = 0; a < PS_MAXQUADRNDLIST; a++)
    {
        PS_QuadRndList[a].RotRate = PRANDFloat() * PI;
        mlSinCos(&PS_QuadRndList[a].RotCx, PS_QuadRndList[a].RotRate);
        float RndSize = 1 + PRANDFloat() * 0.2f;
        PS_QuadRndList[a].RotCx.cos *= RndSize;
        PS_QuadRndList[a].RotCx.sin *= RndSize;
    }



    sprintf(path, "%s\\terrdata\\%s", FalconDataDirectory, TRAILFILE);  // MLR 12/14/2003 - This should probably be fixed
    fp = fopen(path, "r");

    // Initialize the Lists...
    memset(PS_Lists, 0, sizeof(PS_Lists));
    // Initialize all Lists
    PS_ListsInit();

    ppn = NULL;
    int currentemitter = -1;

    // Cobra - Use intrnal PS.ini
    int k = 0;

    while (1)
    {
        char *com, *ary, *arg;
        float  i; // time index

        if (fp == NULL)
        {
            if (g_bHighSFX)
            {
                while (strlen(PS_Data_High[k]) == 0)
                    k++;

                strcpy(buffer, PS_Data_High[k]);

                if (stricmp(PS_Data_High[k], "END") == 0)
                    break;

                k++;
            }
            else
            {
                while (strlen(PS_Data_Low[k]) == 0)
                    k++;

                strcpy(buffer, PS_Data_Low[k]);

                if (stricmp(PS_Data_Low[k], "END") == 0)
                    break;

                k++;
            }
        }
        else
        {
            if (fgets(buffer, sizeof buffer, fp) == 0)
            {
                fclose(fp);
                break;
            }
        }

        // end Cobra

        if (buffer[0] == '#' or buffer[0] == ';' or buffer[0] == '\n')
            continue;

        int b;

        for (b = 0; b < 1024 and (buffer[b] == ' ' or buffer[b] == '\t'); b++);

        com = strtok(&buffer[b], "=\n");
        arg = strtok(0, "\n\0");

        if (com[0] == '#' or com[0] == ';')
            continue;


        /* seperate command and array number 'command[arraynumber]*/
        com = strtok(com, "[");
        ary = strtok(0, "]");

        com = trim(com);
        ary = trim(ary);

        i = TokenF(ary, 0);

        /* kludge so that arg is the current string being parsed */
        SetTokenString(arg);

        if ( not com)continue;

#define On(s) if(stricmp(com,s)==0)

        // COBRA - RED - This section has been added of Animation Nodes
        // The token 'id' or 'animation' selects which node is pointed
        // and what commands are then evaluated referring that node type

        On("id")
        {
            pan = 0;
            GroupNode = NULL;
            tpn = NULL;
            char *n = TokenStr(0);

            if (n)
            {
                // if a new name must inherit from it
                char *i = TokenStr(0);

                if (i)
                {
                    // look for the name
                    DWORD c = 0;

                    // Check if already a PPN with same name
                    while (c < MAX_PARTICLE_PARAMETERS and strncmp(PS_PPN[c].name, i, PS_NAMESIZE)) c++;

                    // if found copy it's values and skip the rest
                    if (c < MAX_PARTICLE_PARAMETERS)
                    {
                        PS_PPN[LastPPN] = PS_PPN[c];
                        ppn = &PS_PPN[LastPPN++];
                        continue;
                    }
                }

                // New PPN, assign to pointer and count one more
                ppn = &PS_PPN[LastPPN++];
                // Default Values
                memset(ppn, 0, sizeof(*ppn));
                strncpy(ppn->name, n, PS_NAMESIZE);
                ppn->id = -1;
                ppn->accel[0].value = 1;
                ppn->alpha[0].value = 255.0f;
                ppn->velInherit = 1;
                ppn->drawType = PSDT_NONE;
                currentemitter = -1;
                ppn->sndVol[0].value = 0;
                ppn->sndPitch[0].value = 1;
                ppn->trailId = -1;
                ppn->visibleDistance = 10000;
                ppn->GroupFlags = GRP_NONE;
                ppn->TrailName[0] = 0;
                ppn->color[0].value.r = ppn->color[0].value.g = ppn->color[0].value.b = 255.0f;
                ppn->RotationRateMax = ppn->RotationRateMin = 0.0f;
                ppn->alpha[0].LogMode = false;
                ppn->color[0].LogMode = false;
                ppn->size[0].LogMode = false;
                ppn->ClusterMode = CHILD_CLUSTER; // Defaults
                ppn->ZPoly = false;
                ppn->EmitLight = false;
                ppn->LightRoot = false;
                ppn->WindAffected = true;
                ppn->WindFactor         = 1.0f; // RV - I-Hawk - default to 1.0
            }

            continue;
        }

        //////////////////////////// TRAILS PARAMETER INITIALIZATION \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

        On("trail")
        {
            GroupNode = NULL;
            ppn = NULL;
            char *n = TokenStr(0);

            if (n)
            {
                // if a new name must inherit from it
                char *i = TokenStr(0);

                if (i)
                {
                    // look for the name
                    DWORD c = 0;

                    // Check if already a PPN with same name
                    while (c < MAX_TRAIL_PARAMETERS and strncmp(PS_TPN[c].Name, i, PS_NAMESIZE)) c++;

                    // if found copy it's values and skip the rest
                    if (c < MAX_TRAIL_PARAMETERS)
                    {
                        PS_TPN[LastTPN] = PS_TPN[c];
                        tpn = &PS_TPN[LastTPN++];
                        continue;
                    }
                }

                // New PPN, assign to pointer and count one more
                tpn = &PS_TPN[LastTPN++];
                // Default Values
                memset(tpn, 0, sizeof(*tpn));
                strncpy(tpn->Name, n, PS_NAMESIZE);
                tpn->Id = -1;
                tpn->Color[0].a = 1.0f;
                tpn->GroupFlags = GRP_NONE;
            }

            continue;
        }


        On("group")
        {
            ppn = NULL;
            pan = NULL;
            tpn = NULL;
            Counter = 0;
            char *n = TokenStr(0);

            if (n) GroupNode = new ParticleGroupNode(n);

            if (LastGroup) LastGroup->Next = GroupNode;
            else Groups = GroupNode;

            LastGroup = GroupNode;
            continue;
        }

        On("animation")
        {
            ppn = NULL;
            GroupNode = NULL;
            char *n = TokenStr(0);

            if (n) pan = GetAnimationNode(n);

            continue;
        }

        // Texture File Spcification...
        On("texturefile")
        {
            char *n = TokenStr(0);
            TheDXEngine.LoadTexture(n);
            continue;
        }

        // we need a valid ppn or pan pointer
        if (( not tpn) and ( not ppn) and ( not pan) and ( not GroupNode))
        {
            continue;
        }



        ////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // COBRA - RED - This section is just evaluated if a PARTICLE PARAMETER NODE is active
        /////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

        if (ppn)  // * PARTICLE PARAMETER NODE *
        {

            On("cluster")
            {
                ppn->ClusterMode = TokenF(CHILD_CLUSTER);
                continue;
            }

            On("windoff")
            {
                ppn->WindAffected = false;
                continue;
            }

            On("windFactor")
            {
                ppn->WindFactor = TokenF(1);
                continue;
            }

            On("lifespan")
            {
                ppn->lifespan          = TokenF(1);
                ppn->lifespanvariation = TokenF(0);
                continue;
            }

            On("color")
            {
                // color[x]=0,1,0,0,1
                ppn->color[ppn->colorStages].value.r = TokenF(0) * 255.0f;
                ppn->color[ppn->colorStages].value.g = TokenF(0) * 255.0f;
                ppn->color[ppn->colorStages].value.b = TokenF(0) * 255.0f;
                ppn->color[ppn->colorStages].K.r = 1.0f;
                ppn->color[ppn->colorStages].K.g = 1.0f;
                ppn->color[ppn->colorStages].K.b = 1.0f;
                ppn->color[ppn->colorStages].time    = i;
                K_CALCRGB(ppn->color, ppn->colorStages, r);
                K_CALCRGB(ppn->color, ppn->colorStages, g);
                K_CALCRGB(ppn->color, ppn->colorStages, b);


                ppn->colorStages++;
                continue;
            }

            On("3dlight")
            {
                ppn->EmitLight = true;
                continue;
            }

            On("light")
            {
                ppn->light[ppn->lightStages].K = 1.0f;
                ppn->light[ppn->lightStages].value = TokenF(0);
                ppn->light[ppn->lightStages].time    = i;
                //if(ppn->light[ppn->lightStages].value>0.5) ppn->EmitLight=true;
                K_CALC(ppn->light, ppn->lightStages);
                ppn->lightStages++;
                continue;
            }

            On("size")
            {
                ppn->size[ppn->sizeStages].value = TokenF(0);
                ppn->size[ppn->sizeStages].time  = i;

                // COBRA - RED - The 1st Stage can define also a Size random CX
                if ( not ppn->sizeStages) ppn->SizeRandom = TokenF(0) / ppn->size[ppn->sizeStages].value;

                // K for this stage defaults at 1
                ppn->size[ppn->sizeStages].K = 1.0f;
                // Update CXes
                K_CALC(ppn->size, ppn->sizeStages);
                ppn->sizeStages++;
                continue;
            }

            On("flags")
            {
                ppn->flags = TokenFlags(0, PSF_CHARACTERS);
                continue;
            }


            On("animate")
            {
                ppn->Texture = NULL; // Disables Texture
                char FileName[PARTICLE_NAMES_LEN];
                strncpy(FileName, TokenStr(""), PS_NAMESIZE); // Sequence Name

                if (i > 1)i = 1; // Limit Check

                ppn->Animation[(int)i] = GetAnimationNode(FileName); // Assign it
                ppn->drawType = PSDT_POLY;
                continue;
            }


            On("texture")
            {
                strncpy(ppn->texFilename, arg, PS_NAMESIZE);
                // ok, check for a group with such a name
                ParticleGroupNode *gpn = FindGroupNode(ppn->texFilename);

                // if found
                if (gpn)
                {
                    // assing a link as texture
                    ppn->Texture = (ParticleTextureNode*)gpn;
                    // and signal the texture depends on a group
                    ppn->GroupFlags or_eq GRP_TEXTURE;
                    ppn->drawType = PSDT_POLY;
                    // end here
                    continue;
                }

                ParticleTextureNode *ptn;
                ptn = GetTextureNode(ppn->texFilename);
                ppn->Texture = ptn;
                ppn->drawType = PSDT_POLY;
                continue;
            }

            On("gravity")
            {
                ppn->gravity[ppn->gravityStages].value = TokenF(0);
                ppn->gravity[ppn->gravityStages].time  = i;
                // K for this stage defaults at 1
                ppn->gravity[ppn->gravityStages].K = 1.0f;
                // Update CXes
                K_CALC(ppn->gravity, ppn->gravityStages);
                ppn->gravityStages++;
                continue;
            }

            On("alpha")
            {
                ppn->alpha[ppn->alphaStages].value = TokenF(0) * 255.0f;
                ppn->alpha[ppn->alphaStages].time  = i;
                // K for this stage defaults at 1
                ppn->alpha[ppn->alphaStages].K = 1.0f;
                // Update CXes
                K_CALC(ppn->alpha, ppn->alphaStages);
                ppn->alphaStages++;
                continue;
            }

            On("drag")
            {
                ppn->simpleDrag = TokenF(0);
                continue;
            }

            On("bounce")
            {
                ppn->bounce = TokenF(0);
                continue;
            }

            if (currentemitter < 10)
            {
                On("addemitter")
                {
                    currentemitter++;
                    //Get run minimum Threshold
                    ppn->ParticleEmitterMin[currentemitter] = TokenF(0);
                    //Get run max Threshold
                    ppn->ParticleEmitterMax[currentemitter] = TokenF(1);
                    // setup zero time variation
                    ppn->emitter[currentemitter].TimeVariation  = 0.0f;
                    ppn->emitter[currentemitter].Light = false;
                    continue;
                }

                if (currentemitter >= 0)
                {
                    On("emissionid")
                    {
                        char *n;
                        n = TokenStr("none");
                        ppn->emitter[currentemitter].id   = -1;

                        if (n)
                            strncpy(ppn->emitter[currentemitter].name, n, PS_NAMESIZE);

                        continue;
                    }

                    On("emissionmode")
                    {
                        char *emitenums[] = {"EMITONCE", "EMITPERSEC", "EMITONIMPACT", "EMITONEARTHIMPACT", "EMITONWATERIMPACT", 0};
                        ppn->emitter[currentemitter].mode = (PSEmitterModeEnum)TokenEnum(emitenums, -1);

                        if (ppn->emitter[currentemitter].mode == -1)
                        {
                            char *emitenums[] = {"ONCE", "PERSEC", "IMPACT", "EARTHIMPACT", "WATERIMPACT", 0};
                            ppn->emitter[currentemitter].mode = (PSEmitterModeEnum)TokenEnum(emitenums, 0);
                        }

                        continue;
                    }
                    On("emissiondomain")
                    {
                        ppn->emitter[currentemitter].domain.Parse();
                        continue;
                    }

                    On("emissiontarget")
                    {
                        ppn->emitter[currentemitter].target.Parse();
                        continue;
                    }

                    On("emissionrate")
                    {
                        ppn->emitter[currentemitter].rate[ppn->emitter[currentemitter].stages].value = TokenF(0);
                        ppn->emitter[currentemitter].rate[ppn->emitter[currentemitter].stages].time  = i;
                        ppn->emitter[currentemitter].stages++;
                        continue;
                    }

                    On("emissionvelocity")
                    {
                        ppn->emitter[currentemitter].velocity = TokenF(0);
                        ppn->emitter[currentemitter].velVariation = TokenF(0);
                        continue;
                    }

                    On("emissionrnd")
                    {
                        ppn->emitter[currentemitter].TimeVariation = TokenF(0);
                        continue;
                    }

                }
            }

            On("acceleration")
            {
                ppn->accel[ppn->accelStages].value = TokenF(1);
                ppn->accel[ppn->accelStages].time  = i;
                // K for this stage defaults at 1
                ppn->accel[ppn->accelStages].K = 1.0f;
                // Update CXes
                K_CALC(ppn->accel, ppn->accelStages);
                ppn->accelStages++;
                continue;
            }

            On("inheritvelocity")
            {
                ppn->velInherit = TokenF(1);
                continue;
            }

            On("initialVelocity")
            {
                ppn->velInitial = TokenF(1);
                ppn->velVariation = TokenF(0);
                continue;
            }

            On("RotationRate")
            {
                ppn->RotationRateMax = TokenF(1) * PI / 180.0f;
                ppn->RotationRateMin = TokenF(0) * PI / 180.0f;
                continue;
            }


            On("drawtype")
            {
                char *n = TokenStr(0);

                if ( not strcmp(n, "poly"))
                {
                    ppn->drawType = PSDT_POLY;
                }

                if ( not strcmp(n, "zplane"))
                {
                    ppn->drawType = PSDT_POLY, ppn->ZPoly = true;
                }

                continue;
            }

            On("soundid")
            {
                ppn->sndId = TokenI(0);
                continue;
            }

            On("soundlooped")
            {
                ppn->sndLooped = TokenI(0);
                continue;
            }

            On("soundVolume")
            {
                ppn->sndVol[ppn->sndVolStages].value = (TokenF(0) - 1) * -10000;
                ppn->sndVol[ppn->sndVolStages].time  = i;
                // K for this stage defaults at 1
                ppn->sndVol[ppn->sndVolStages].K = 1.0f;
                // Update CXes
                K_CALC(ppn->sndVol, ppn->sndVolStages);
                ppn->sndVolStages++;
                continue;
            }

            On("soundPitch")
            {
                ppn->sndPitch[ppn->sndPitchStages].value = TokenF(1);
                ppn->sndPitch[ppn->sndPitchStages].time  = i;
                // K for this stage defaults at 1
                ppn->sndPitch[ppn->sndPitchStages].K = 1.0f;
                // Update CXes
                K_CALC(ppn->sndPitch, ppn->sndPitchStages);
                ppn->sndPitchStages++;
                continue;
            }

            On("trailid")
            {
                strncpy(ppn->TrailName, TokenStr(0), PS_NAMESIZE);
                continue;
            }

            On("groundfriction")
            {
                ppn->groundFriction = TokenF(0);
                continue;
            }

            On("visibledistance")
            {
                ppn->visibleDistance = TokenF(10000);
                continue;
            }

            On("dieonground")
            {
                ppn->dieOnGround = (float)TokenI(0);
                continue;
            }

            On("modelct")
            {
                ppn->bspCTID    = TokenI(0);
                ppn->bspVisType = TokenI(0);
                continue;
            }

            On("orientation")
            {
                char *enumstr[] = {"none", "movement", 0};
                ppn->orientation = (PSOrientation)TokenEnum(enumstr, 0);
                continue;
            }

            On("emitonerandomly")
            {
                ppn->emitOneRandomly = TokenI(0);
                continue;
            }
        } // * END OF PARTICLE PARAMETER NODE *



        ////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // COBRA - RED - This section is just evaluated if a PARTICLE ANIMATION NODE is active
        /////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

        if (pan)  // * PARTICLE ANIMATION NODE *
        {

            On("frames") // * FRAMES SEQUENCE DECLARATION *
            {
                // 'frames=n name'
                pan->NFrames = TokenI(0); // n = Number of Frames
                char FileName[PARTICLE_NAMES_LEN]; // name = BaseName of frames
                strncpy(FileName, TokenStr(""), PARTICLE_NAMES_LEN);
                pan->Sequence = (void*)GetFramesList(FileName, pan->NFrames);
                continue;
            }

            On("framerate") // * FRAME RATE DEFINITION *
            {
                // 'framerate=fps'
                pan->Fps = 1.0f / TokenF(0); // fps= Frames Per Second speed
                continue;
            }

            On("flags") // * FLAGS DECLARATION *
            {
                // 'flags=F1|F2...'
                pan->Flags = TokenFlags(0, ANIMATION_FLAGS);
                continue;
            }

        }

        ////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // COBRA - RED - This section is just evaluated if a GROUP NODE is active
        /////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

        if (GroupNode)
        {
            On("texture")
            {
                // Check for texture
                if (GroupNode->Type == GRP_NONE or GroupNode->Type == GRP_TEXTURE)
                {
                    char *n = TokenStr(0);

                    // if found and added
                    if (GroupNode->ptr[GroupNode->Items] = GetTextureNode(n))
                    {
                        GroupNode->Type = GRP_TEXTURE;
                        GroupNode->Items++;
                    }
                }

                continue;
            }
        }



        ////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // COBRA - RED - This section is just evaluated if a TRAIL PARAMETER NODE is active
        /////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


        if (tpn)  // * TRAIL PARAMETER NODE *
        {

            On("lifespan")
            {
                tpn->LifeSpan = TokenF(1);
                continue;
            }

            On("emittime")
            {
                tpn->LifeCx = 1.0f / TokenF(1) ;
                continue;
            }

            On("weight")
            {
                tpn->Weight = TokenF(0);
                continue;
            }


            On("color")
            {
                tpn->Color[0].r = TokenF(0) * 255.0f;
                tpn->Color[0].g = TokenF(0) * 255.0f;
                tpn->Color[0].b = TokenF(0) * 255.0f;
                tpn->Color[1].r = TokenF(0) * 255.0f;
                tpn->Color[1].g = TokenF(0) * 255.0f;
                tpn->Color[1].b = TokenF(0) * 255.0f;
                continue;
            }


            On("size")
            {
                tpn->Size[0] = TokenF(0);
                tpn->Size[1] = TokenF(0);
                continue;
            }


            On("texture")
            {
                // * SQUARE BILLBOARDED TEXTURE NAME *
                char *n = TokenStr(0);
                strncpy(tpn->TexFilename, n, PS_NAMESIZE);
                // ok, check for a group with such a name
                ParticleGroupNode *gpn = FindGroupNode(tpn->TexFilename);

                // if found
                if (gpn)
                {
                    // assing a link as texture
                    tpn->Texture = (ParticleTextureNode*)gpn;
                    // and signal the texture depends on a group
                    tpn->GroupFlags or_eq GRP_TEXTURE;
                    continue;
                }

                ParticleTextureNode *ptn;
                ptn = GetTextureNode(tpn->TexFilename);
                tpn->Texture = ptn;

                // * SEGMENT TEXTURE NAME *
                n = TokenStr(0);
                strncpy(tpn->SideTexFilename, n, PS_NAMESIZE);
                // ok, check for a group with such a name
                gpn = FindGroupNode(tpn->SideTexFilename);

                // if found
                if (gpn)
                {
                    // assing a link as texture
                    tpn->SideTexture = (ParticleTextureNode*)gpn;
                    // and signal the texture depends on a group
                    tpn->GroupFlags or_eq GRP_TEXTURE2;
                    continue;
                }

                ptn;
                ptn = GetTextureNode(tpn->SideTexFilename);
                tpn->SideTexture = ptn;

                // The segment Texture Rate
                tpn->TexRate = TokenF(1.0f);
                continue;
            }


            On("alpha")
            {
                tpn->Alpha = TokenF(1.0f) * 255.0f;
                continue;
            }

            On("trailnr")
            {
                tpn->Id = TokenI(-1);
                continue;
            }

            On("visibledistance")
            {
                tpn->VisibleDistance = TokenF(0.0f);
                continue;
            }

            On("rate")
            {
                tpn->Interval = TokenF(1.0f);
                continue;
            }


            On("fragradius")
            {
                tpn->FragRadius = TokenF(0.0f);
                // The integration distance has to be halved
                tpn->IntegrateDistance = TokenF(0.0f) / 2.0f;
                continue;
            }

            On("random")
            {
                tpn->RndStep = TokenF(0.0f), tpn->RndLimit = TokenF(0.0f), tpn->RndTime = TokenF(0.0f) ;
                continue;
            }

        }
    }

    /////////////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // END OF NODES EVALUATIONS
    /////////////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    /* id */
    int l, extra = 0;

    for (l = 0; l < nameListCount; l++)
    {
        // Assign IDs to Names for each possible PPN
        for (DWORD c = 0; c < MAX_PARTICLE_PARAMETERS; c++)
        {
            // if a name found
            if ( not stricmp(PS_PPN[c].name, nameList[l]))
            {
                PS_PPN[c].id = l;
                extra++;
                break;
            }
            else
            {
                if (PS_PPN[c].id not_eq -1)
                {
                    continue;
                }
                else
                    PS_PPN[c].id = -1;

                if (c == MAX_PARTICLE_PARAMETERS - 1)
                {
                    break;
                }
            }
        }
    }

    l = extra;

    // number all the nodes that don't match above;
    for (DWORD c = 0; c < MAX_PARTICLE_PARAMETERS; c++)
    {
        if (PS_PPN[c].id == -1)
        {
            PS_PPN[c].id = l++;
        }
    }



    // COBRA - RED - Going to check that all animations are configured the right way
    pan = (ParticleAnimationNode*)AnimationsList.GetHead();

    while (pan)
    {
        if (( not pan->Fps) or ( not pan->NFrames) or ( not pan->Sequence))
        {
            sprintf(ErrorMessage, "Animation %s Failed", pan->AnimationName); //*** CRASH MESSAGE ****
            OutputDebugString(ErrorMessage);
            ShiError(ErrorMessage);
            return(false);
        }
        else
        {
            pan = (ParticleAnimationNode*)pan->GetSucc();
        }
    }


    // copy pointers to PPN array here
    //PPN = (ParticleParamNode **)malloc(sizeof(ParticleParamNode *) * l);
    PPN = new ParticleParamNode * [l];
    PPNCount = l;

    for (int c = 0; c < l; c++)
    {
        PPN[PS_PPN[c].id] = (ParticleParamNode *)c;
    }

    /*ppn=(ParticleParamNode *)paramList.GetHead();
    while(ppn)
    {
     PPN[ppn->id]=ppn;
     ppn=(ParticleParamNode *)ppn->GetSucc();
    }*/




    // link the emmitter names to ids;
    // Check for any particle
    for (DWORD c = 0; c < MAX_PARTICLE_PARAMETERS; c++)
    {
        int t;

        // Check for any emitter in the particle
        for (t = 0; t < PSMAX_EMITTERS; t++)
        {
            if (PS_PPN[c].emitter[t].stages <= 0)
                continue;

            // Look thru all list of PPN for the right emitter
            for (DWORD n = 0; n < MAX_PARTICLE_PARAMETERS; n++)
            {
                // if found the right emitter, assign it's ID
                if (stricmp(PS_PPN[n].name, PS_PPN[c].emitter[t].name) == 0)
                {
                    PS_PPN[c].emitter[t].id = n;

                    if (PS_PPN[n].EmitLight)
                    {
                        PS_PPN[c].emitter[t].Light = true;
                    }
                }
            }
        }
    }

    // Get the light roots
    for (DWORD c = 0; c < MAX_PARTICLE_PARAMETERS; c++)
    {
        int t;

        // Check for any emitter in the particle
        for (t = 0; t < PSMAX_EMITTERS; t++)
        {
            if (PS_PPN[c].emitter[t].stages <= 0)
                continue;

            if (PS_PPN[c].emitter[t].Light)
            {
                PS_PPN[c].LightRoot = true;
                break;
            }
        }
    }


    ////////////////////////////////// SETUP TRAILS STUFF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // Setup conversion from Old Trail Numbers to New PS trails
    memset(PS_TrailsID, 0x00, sizeof(PS_TrailsID));

    for (DWORD a = 0; a < LastTPN; a++)
    {
        // start of names table
        const TrailRefsType *Table = TrailsTable;

        // check from start to end of table
        while (Table->Name[0])
        {
            // if trail name found in table, assign it
            if ( not strcmp(PS_TPN[a].Name, Table->Name))
            {
                PS_TrailsID[Table->Id] = a;
                break;
            }

            Table++;
        }
    }

    // Now compile trail parameters
    for (DWORD a = 0; a < LastTPN; a++)
    {
        // Get Life to make calculations
        float Life = PS_TPN[a].LifeSpan;

        // Calculate Colors Rate
        PS_TPN[a].ColorRate.r = (PS_TPN[a].Color[1].r - PS_TPN[a].Color[0].r) ;
        PS_TPN[a].ColorRate.g = (PS_TPN[a].Color[1].g - PS_TPN[a].Color[0].g) ;
        PS_TPN[a].ColorRate.b = (PS_TPN[a].Color[1].b - PS_TPN[a].Color[0].b) ;
        // Calculate Size Rate
        PS_TPN[a].SizeRate = (PS_TPN[a].Size[1] - PS_TPN[a].Size[0]) / 2.0f ;
        // Calculate Interval
        PS_TPN[a].Interval = (PS_TPN[a].Interval) ? 1 / PS_TPN[a].Interval : 999999.0f;
        // Calculate Density and Line Distance
        PS_TPN[a].LineDistance = PS_TPN[a].Size[1] * PS_TPN[a].Size[1] * 50.0f ;//* TRAIL_BIAS_CX;

        // Visible Distance
        if ( not PS_TPN[a].VisibleDistance)
        {
            PS_TPN[a].VisibleDistance = PS_TPN[a].LineDistance * 10.0f;
        }

        // The Frag Radius is half max size * 30 ( feet )
        if ( not PS_TPN[a].FragRadius)
        {
            PS_TPN[a].FragRadius = PS_TPN[a].Size[1] * 30.0f ;
        }

        if ( not PS_TPN[a].IntegrateDistance)
        {
            PS_TPN[a].IntegrateDistance = PS_TPN[a].FragRadius / 2.0f;
        }

    }


    // Update trail names for particles with right indexes
    for (DWORD c = 0; c < MAX_PARTICLE_PARAMETERS; c++)
    {
        // Look if a trail name present
        if (PS_PPN[c].TrailName[0])
        {
            // look for such name in Trails parameters
            for (DWORD a = 0; a < LastTPN; a++)
            {
                // if found same name, assign the ID to the Particle
                if ( not strcmp(PS_TPN[a].Name, PS_PPN[c].TrailName)) PS_PPN[c].trailId = a;
            }
        }
    }


    //////////////////////////////// SETUP TRAILS STUFF END \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


    //------------------
    fclose(fp);
    //------------------

    if (psContext)
    {
        DXContext *stored = psContext; // have to store it because release clears it.

        ReleaseTexturesOnDevice(stored);
        SetupTexturesOnDevice(stored);
    }

    return(true);
}
#endif
