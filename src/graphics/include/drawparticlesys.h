/***************************************************************************\
DrawParticleSys.h
MLR

\***************************************************************************/
#ifndef _DRAWPARTICLESYS_H_
#define _DRAWPARTICLESYS_H_

#include "DrawObj.h"
#include "context.h"
#include "Tex.h"
#include "falclib/include/alist.h"
#include "Graphics/Include/Drawbsp.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"
#include "FakeRand.h"
#include "falclib/include/fsound.h"

#include "context.h"
#include "mltrig.h"

#define PARTICLE_NAMES_LEN 32
#define LOG10_ARRAY_ITEMS 500
#define ASIN_ARRAY_ITEMS 500
#define FADE_ARRAY_ITEMS 500
#define SIZE_ARRAY_ITEMS 500
#define MAX_GROUP_ITEMS 32
#define PS_NAMESIZE 64
#define LIGHT_SIZE_CX 15



////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//                              COBRA - RED - THE PS REWRITING                                     \\
////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\





#define MAX_PARTICLE_PARAMETERS 1024 // The max particle definitions supported
#define MAX_EMITTERS_PARAMETERS 4096 // The max emitters definitions supported
#define MAX_TRAIL_PARAMETERS 256 // The max Trails definitions supported
#define PS_MAX_PARTICLES 32768 // The PARTICLES Supported
#define PS_MAX_POLYS PS_MAX_PARTICLES // The Max POLY Subparts supported
#define PS_MAX_EMITTERS PS_MAX_PARTICLES*4 // The EMITTERS Supported
#define PS_MAX_SOUNDS PS_MAX_PARTICLES // The Max SOUND Subparts supported
#define PS_MAX_LIGHTS PS_MAX_PARTICLES/32 // The Max SOUND Subparts supported
#define PS_MAX_CLUSTERS PS_MAX_PARTICLES/2 // The Max CLUSTERS supported
#define PS_MAX_TRAILS 256 // The Max TRAILS supported * WARNING - MUST BE A POWER OF 2 FOR Handle calculations
//#define PS_MAX_TRAILPARTS (PS_MAX_TRAILS * 128) // The Max TRAIL PARTS supported
#define PS_PTR DWORD // the pointers size for PS stuff
#define PS_NOPTR 0xffffffff // the null value for a PS PTR
#define PS_RECALC_DELTA 300 // Delta position change of a particle to cause some parameters recalcs
#define PS_MAXQUADRNDLIST 1024 // Max precalculated Quads for use

#define TRAIL_NODES_X_SEG 128 // Number of nodes for each segment trail
#define TRAIL_MAX_SEGMENTS 256 // Number of available Trail Segmments

#define PS_LISTS_NR 8 // Number of supported PS LISTS
#define PS_PARTICLES_IDX 0 // The Index of List
#define PS_POLYS_IDX 1 // The Index of List
#define PS_EMITTERS_IDX 2 // The Index of List
#define PS_SOUNDS_IDX 3 // The Index of List
#define PS_LIGHTS_IDX 4 // The Index of List
#define PS_TRAILS_IDX 5 // The Index of List
//#define PS_TRAILPARTS_IDX 6 // The Index of List
#define PS_CLUSTERS_IDX 7 // The Index of List


// The (0 / 2PI) normalized ASIN macro
#define PS_NORM_ASIN(x) (ASinArray[min( ASIN_ARRAY_ITEMS - 1, F_I32(x * ASIN_ARRAY_ITEMS))])
// The Lod Bias CX of the zoom, exagerates trails details with distance, this CX is used to reduce to
// a reasonable value
#define TRAIL_BIAS_CX    0.2F

///////////////////////////// ENUMS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

typedef DWORD TRAIL_HANDLE;

typedef enum { PSEM_ONCE = 0, PSEM_PERSEC = 1, PSEM_IMPACT = 2, PSEM_EARTHIMPACT = 3, PSEM_WATERIMPACT = 4 } PSEmitterModeEnum;

typedef enum { PSD_SPHERE = 0, PSD_PLANE = 1, PSD_BOX = 2, PSD_BLOB = 3, PSD_CYLINDER = 4, PSD_CONE = 5, PSD_TRIANGLE = 6, PSD_RECTANGLE = 7,
                PSD_DISC = 8, PSD_LINE = 9, PSD_POINT = 10
             }  PSDomainEnum;

typedef enum { PSDT_NONE = 0, PSDT_POLY  = 1, PSDT_POINT = 2, PSDT_LINE  = 3 } PSDrawType;

typedef enum { PSO_NONE = 0, PSO_MOVEMENT  = 1 } PSOrientation;

enum PSType { PST_NONE = 0, PST_SPARKS = 1, PST_EXPLOSION_SMALL = 2, PST_NAPALM = 3 };


////////////////////// BASIC STRUCTURES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

struct Point
{
    float x, y, z;
};

struct psRGBA
{
    float   r, g, b, a;
};

struct timedRGBA
{
    bool LogMode;
    float   time;
    psRGBA  value;
    psRGBA K;
};

struct psRGB
{
    float   r, g, b;
};


/****** Timed data structs ******/
struct timedRGB
{
    bool LogMode;
    float   time;
    psRGB value;
    psRGB K;
};

struct timedFloat
{
    bool LogMode;
    float   time;
    float   value;
    float K;
};


/******************************/
struct TextureLink
{
    DWORD TexHandle;
    CTextureItem *TexItem;
};


///////////////////////////// PARTICLES STRUCTURES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


// The Default clustering mode, depends on owner
#define CHILD_CLUSTER -99999.0f
#define NO_CLUSTERING -1.0f

// The Cluster position... used to make a FOV Macro calculation of a PS cluster

#pragma pack( push, Clusterpack, 16)
typedef struct
{

#pragma pack( push, ClusterData, 16)
    struct
    {
        PS_PTR NEXT; // the Next Cluster node

        struct
        {
            char Alive : 1; // Tells this cluster is used by some particle
            char Out : 1; // Result of visibility check, Full Out of FOV or Out of Visibile Distance
            char In : 1; // Result of visibility check, Full in FOV
            char LightIn : 1; // Within Light Range
            char Static : 1; // 1 = Static Cluster, Dimension is from a Fixed Radius, position is TLF
            // 0 = Dynamic Cluster, Radius calculated from  TLF - BRN points
        };

        float Radius;
    };
#pragma pack( pop, ClusterData)
    XMMVector TLFpos, BRNpos; // Top/Left/Far and Bottom/Right/Near positions
} ClusterPosType;
#pragma pack( pop, Clusterpack)



// The Particle Node structure
typedef struct
{

    PS_PTR NEXT; // the Next Particle node
    PS_PTR PPN; // the Parameter node
    PS_PTR CLUSTER; // Cluster Position stuff
    PS_PTR LIGHT; // The Light

    int birthTime; //
    int lastTime; // last time we were updated
    float lifespan; // in seconds
    float life; // normalized

    // I'll live forever...
    struct
    {
        char Alive : 1;
        char Cluster : 1;
        char WindAffected: 1;
    };

    float elapsedTime;
    Trotation *rotation; // runtime computed, shared
    float plife; // life value for the previous exec

    int AccelStage, GravityStage;

    Tpoint pos; //
    float Radius; //

    Tpoint vel, LastCalcPos, Wind;
    float GroundLevel, VelRnd;

    int SizeStage;
    // The texture Item
    float SizeRandom;

} ParticleNodeType;


// This is the poly Subpart structure
typedef struct
{
    // Next in List
    PS_PTR NEXT;
    // The Owner
    PS_PTR OWNER;
    // The Parameter node
    PS_PTR PPN;
    // The Stages Counters
    int AlphaStage;
    int ColorStage;
    int LightStage;
    // The textuer Handle
    DWORD TexHandle;
    // The Size random CX
    //CTextureItem *TexItem;
    // the Quad UV vertices
    float tu[4], tv[4];
    // Rotation Stff
    float Rotation, RotationRate;
} PolySubPartType;


// This is an Emitter structure
typedef struct
{
    // Next in List
    PS_PTR NEXT;
    // The Owner
    PS_PTR OWNER;
    // The Light
    PS_PTR LIGHT;
    // the rest from generation
    float  rollover;
    // Time Stuff
    float  RndTime, RndTimeCx;
    // Generation stages
    int LastStage;
    // The Pointer to the PEP
    struct ParticleEmitterParam *PEP;
}  EmitterPartType;


// This is a Light structure
typedef struct
{
    // Next in List
    PS_PTR NEXT;
    // the  Color
    D3DCOLORVALUE Color;
    // the light level
    float Light;
    // The Radius
    float Radius;
    // Alive
    struct
    {
        char Alive : 1;
        char LightIn : 1;
    };
    // position
    Tpoint Pos;
}  LightPartType;



typedef struct
{
    // Next in List
    PS_PTR NEXT;
    // The Owner
    PS_PTR OWNER;
    // The Parameter node
    PS_PTR PPN;

    struct
    {
        char Play : 1;
        char Looped : 1;
        char Started : 1;
    };
    int VolumeStage, PitchStage, SoundId;
    F4SoundPos *SoundPos;
} SoundSubPartType;




typedef struct
{
    PS_PTR CLUSTER; // Cluster Position stuff

    // Life
    DWORD Birth;
    struct
    {
        DWORD Entry : 1; // This is the entry of a segment
        DWORD Exit : 1; // this is the exit of a segment
        DWORD Split : 1; // This is a split of a segment
    };
    // Position bitand Wind
    Tpoint Pos, Wind, Offset;
    // Randomic CX
    float SizeRnd, ColorRnd, AlphaRnd;

    // The textuer Handle
    DWORD QuadListIndex, QuadListStep;
    float SideTexIndex;
} TrailSubPartType;




typedef struct
{
    float RotRate;
    mlTrig RotCx;
} QuadRndType;



typedef struct
{
    // Next in List
    PS_PTR NEXT;
    PS_PTR OWNER; // The Owner
    PS_PTR TPN; // Trail parameter Node
    PS_PTR CLUSTER; // Cluster Position stuff

    TRAIL_HANDLE Handle;

    TrailSubPartType *TRAIL; // The Trail
    DWORD Nodes, Entry, Last;

    // Position, Wind, Drag
    Tpoint Pos, LastPos, Wind;
    D3DXVECTOR3 Origin, LastSV;
    float Elapsed, Interval, LifeCx, Life, ClusterRadius;
    float TexRate, LastTexIndex, TexStep, NextTexIndex, OffsetX, OffsetY;
    float SizeCx, AlphaCx;
    DWORD ID;
    DWORD QuadListIndex, QuadListStep;
    float su[2];

    struct
    {
        char Alive : 1;
        char StartUp : 1;
        char Run : 1;
        char Updated : 1;
        char Split : 1;
    };
} TrailEmitterType;




// This is the item pointing to a PS List of objects
typedef struct
{
    DWORD ListSize;
    void *ObjectList;
    PS_PTR *IndexList;
    PS_PTR ListEntry, ObjectIn, ObjectOut, ListEnd;
} PS_ListType;

#define PS_INIT_LIST(i, nr, type) PS_ListInit(i, nr, sizeof(type))



// Used to draw segmented trails (like missile trails)

class ParticleParam;
class ParticleNode;
class ParticleTextureNode;
class ParticleGroupNode;
class ParticleAnimationNode;

enum GroupType
{
    GRP_NONE = 0,
    GRP_TEXTURE = 0x00000001,
    GRP_TEXTURE2 = 0x00000002,
    GRP_PARTICLE = 0x00000004,
};

class DPS_Node : public ANode
{
    // this is used to track particle objects for various internal needs
public:
    class DrawableParticleSys *owner;
};


//*********************************************************************************************************

class ParticleDomain
{
public:
    PSDomainEnum type;
    void GetRandomDirection(Tpoint *p);
    void GetRandomPosition(Tpoint *p);
    union
    {
        float  param[9];
        struct
        {
            Point pos, size;
        } sphere;
        struct
        {
            float a, b, c, d;
        } plane;
        struct
        {
            Point cornerA, cornerB;
        } box;
        struct
        {
            Point pos, size;
            float  deviation;
        } blob;
    };
    void Parse(void);
};

struct   ParticleEmitterParam
{
    PSEmitterModeEnum  mode;
    ParticleDomain    domain;
    ParticleDomain     target;
    float    param[9];
    int    id;
    char    name[PS_NAMESIZE];
    int    stages;
    timedFloat rate[10];
    float velocity, velVariation;
    float TimeVariation;
    bool Light;
};

#define PS_PEPType  ParticleEmitterParam


// The emitter structure
typedef struct
{
    ParticleEmitterParam *pep;
    float  rollover;
    float  RndTime, RndTimeCx;
    int LastStage;
} EmitterSubPartType;






typedef struct
{
    int id;
    char    name[PS_NAMESIZE];

    int     particleType; // which paritcle class to use for particles????

    float lifespan, lifespanvariation; // how long a particle lasts in seconds
    int flags;
    float lodFactor;
    DWORD GroupFlags;
    PSDrawType drawType;
    int colorStages;
    timedRGB color[10];
    int lightStages;
    timedFloat light[10];
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
    char TrailName[PS_NAMESIZE];
    int trailId;
    float velInitial, velVariation, velInherit; // inherit from emitter
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
    float ClusterMode; // the Cluster Mode
    struct
    {
        char ZPoly : 1;
        char EmitLight : 1;
        char LightRoot : 1;
        char WindAffected: 1;
    };
    float VelRnd;
    float   WindFactor; // RV - I-Hawk - Multiplier wind factor for some effects

} PS_PPType;



// COBRA - RED - The Animations List pointer
// This class is used to keep track of a sequence of animated frames
class ParticleAnimationNode : public ANode
{
public:
    char AnimationName[PARTICLE_NAMES_LEN]; // Name of the Sequence
    int NFrames; // frames of the Sequence
    float Fps; // Frames Per second Playing speed
    void *Sequence; // pointer to the Frame List pointers in memory
    int Flags;

    GLint Run(int &Frame, float &TimeRest, float Elapsed, Tpoint &pos, float &alpha);

};

#define ANIMATION_FLAGS "LUD" // COBRA - RED - Animations Flag
// L = Loop Animation
// U = UP View Full Animation
// D = DOWN View Full Animation

#define ANIM_LOOPING 0x01
#define ANIM_UPVIEW 0x02
#define ANIM_DNVIEW 0x04

typedef struct
{
    int Id;
    char    Name[PS_NAMESIZE];
    float LifeSpan, Interval, LifeCx;
    float Size[2], SizeRate, VisibleDistance, IntegrateDistance;
    psRGBA Color[2], ColorRate;
    float Alpha, Weight;
    DWORD GroupFlags;
    char    TexFilename[PS_NAMESIZE];
    char    SideTexFilename[PS_NAMESIZE];
    float TexRate;
    float LineDistance, FragRadius;
    float RndLimit, RndStep, RndTime;


    ParticleTextureNode *Texture;
    ParticleTextureNode *SideTexture;

} PS_TPType;


typedef struct
{
    char    Name[PS_NAMESIZE];
    DWORD Id;
} TrailRefsType;

#define TRAILREF(trail) { "TRAIL_"#trail, TRAIL_##trail }


//*********************************************************************************************************


class DrawableParticleSys : public DrawableObject
{
public:
    DrawableParticleSys(int ParticleSysType, float scale = 1.0f);
    virtual ~DrawableParticleSys();

    void ReleaseToSfx(); // gives object to SfxClass object so that the parent object can be terminated whilst still leaving the effect running

    void AddParticle(int ID, Tpoint *p , Tpoint *v = 0);
    void AddParticle(Tpoint *p, Tpoint *v = 0);

    void Exec(void);
    virtual void Draw(class RenderOTW *renderer, int LOD);
    int  HasParticles(void);

    static bool LoadParameters(void);
    static void UnloadParameters(void);
    static void SetGreenMode(BOOL state);
    static void SetCloudColor(Tcolor *color);
    static void ClearParticleList(void);
    static char *GetErrorMessage(void);

    static float Log10Array[LOG10_ARRAY_ITEMS];
    static float FadeArray[FADE_ARRAY_ITEMS];
    static float ASinArray[ASIN_ARRAY_ITEMS];
    static float SizeArray[SIZE_ARRAY_ITEMS];
    static void  PS_AddParticleEx(int ID, Tpoint *Pos, Tpoint *Vel);


    void    SetHeadVelocity(Tpoint *FPS);

protected:
    static BOOL greenMode;
    static Tcolor litCloudColor;

    int type;
private:
    AList particleList;
    ParticleParam *param;
    int  Something;
    Tpoint headFPS;
    Tpoint position;

    DPS_Node dpsNode;
    static AList dpsList;
    void ClearParticles(void);

    static ProtectedAList paramList; // anytime we access the paramList (or a PPN) we must lock this list.

    static AList AnimationsList;
    static AList textureList;
    static ParticleTextureNode *FindTextureNode(char *fn);
    static ParticleTextureNode *GetTextureNode(char *fn);
    static ParticleTextureNode *GetFramesList(char *fn, int Frames);
    static ParticleAnimationNode *FindAnimationNode(char *fn);
    static ParticleAnimationNode *GetAnimationNode(char *fn);
    static ParticleGroupNode  *Groups, *LastGroup;
    static ParticleGroupNode *FindGroupNode(char *fn);

    static QuadRndType PS_QuadRndList[PS_MAXQUADRNDLIST];
    static class RenderOTW *PS_Renderer;
    static PS_ListType PS_Lists[PS_LISTS_NR];
    static PS_PPType *PS_PPN;
    static PS_PEPType *PS_PEP;
    static PS_TPType *PS_TPN;
    static DWORD PS_PPNNr;
    static DWORD PS_TPNNr;
    static DWORD PS_PEPNr;
    static TRAIL_HANDLE TrailsHandle;
    static DWORD PS_TrailsID[MAX_TRAIL_PARAMETERS], PS_SubTrails;
    static void PS_ListInit(DWORD i, DWORD nr, DWORD size);
    static void PS_ListsInit(void);
    static void PS_ListsReset(void);
    static void PS_ListsRelease(void);
    static PS_PTR PS_AddItem(DWORD ListIdx);
    static PS_PTR  PS_RemoveItem(DWORD ListIdx, PS_PTR Item, PS_PTR Prev);
    static bool PS_LoadParameters(void);

    static void PS_AddParticle(int ID, Tpoint *Pos, Tpoint *Vel = 0, Tpoint *Aim = 0, float fRotationRate = 0, PS_PTR Cluster = PS_NOPTR, PS_PTR LIGHT = PS_NOPTR);
    static void PS_ParticleRun(void);

    static TRAIL_HANDLE PS_AddTrail(int ID, Tpoint *Pos, PS_PTR OWNER = PS_NOPTR, bool run = true, float AlphaCx = 1.0f, float SizeCx = 1.0f);
    static void PS_TrailRun(void);
    static void PS_TrailsClear(void);

    static void PS_AddSubTrail(TrailSubPartType &Part, int ID, float AlphaCx, float SizeCx, Tpoint *Pos, Tpoint *Wind, Tpoint *Offset, PS_PTR CLUSTER);
    static void PS_SubTrailRun(TrailSubPartType *Trail, D3DXVECTOR3 &Origin, DWORD &Entry, DWORD &Exit, DWORD Elements, PS_PTR tpn);
    static void PS_SubTrailDraw(void);

    static void PS_AddSound(PS_PTR owner, PS_PTR ID);
    static void PS_SoundRun(void);

    static void PS_LightsRun(void);

    static void PS_AddPoly(PS_PTR owner, PS_PTR ppn);
    static void PS_PolyRun(void);

    static void PS_ClustersReset(void);
    static void PS_ClustersRun(void);

    static void PS_EmitterRun(void);
    static void PS_GenerateEmitters(PS_PTR owner, PS_PPType &PPN);
    static void PS_AddEmitter(PS_PTR owner, ParticleEmitterParam *PEP, PS_PTR Light);

    static float PS_EvalTimedLinLogFloat(float life, int &LastStage, int Count, timedFloat *input);
    static float PS_EvalTimedFloat(float life,  int &LastStage, int Count, timedFloat *input);
    static psRGBA PS_EvalTimedRGBA(float life, int &LastStage, int Count, timedRGBA  *input);
    static psRGB PS_EvalTimedRGB(float life, int &LastStage, int Count, timedRGB   *input);

    static float PS_ElapsedTime;
    static DWORD PS_RunTime;
    static DWORD PS_LastTime;
    static Tcolor PS_HiLightCx, PS_LoLightCx;
    static char *nameList[];
    static int nameListCount;

    static bool PS_NVG, PS_TV;

public:
    static void SetupTexturesOnDevice(DXContext *rc);
    static void ReleaseTexturesOnDevice(DXContext *rc);
    static int IsValidPSId(int id);
    static int GetNameId(char *name);
    static void PS_Exec(class RenderOTW *renderer);

    static float groundLevel;
    static float cameraDistance;
    static int   reloadParameters;
    static float winddx, winddy;

    // static PS_PTR PS_AddTrail(int TrailId, float x, float y, float z);
    static void PS_KillTrail(TRAIL_HANDLE Handle);
    // static void PS_UpdateTrail(PS_PTR Trail, float x, float y, float z);
    static TRAIL_HANDLE PS_EmitTrail(TRAIL_HANDLE Handle, int TrailId, float x, float y, float z, float AlphaCx = 1.0f, float SizeCx = 1.0f);

};


#endif // _DRAWSGMT_H_

