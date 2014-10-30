/***************************************************************************\
    RenderOW.h
    Scott Randolph
    January 2, 1996

    This class provides 3D drawing functions specific to rendering out the
 window views including terrain.

    The implementation of this class is split into multiple .cpp files
 with names of the form OTW*.cpp
\***************************************************************************/
#ifndef _RENDEROW_H_
#define _RENDEROW_H_

#include "Edge.h"
#include "Render3D.h"
//#include "rviewpnt.h"


//#define ENABLE_LIGHTING // Define this to light the terrain when hazing is on
//#define TWO_D_MAP_AVAILABLE // Compile with this on to make the 2D debugging "map" view available
#ifdef TWO_D_MAP_AVAILABLE
extern BOOL twoDmode;
extern int  TWODSCALE;
#endif


typedef struct TerrainVertex: public ThreeDVertex
{
    /* ThreeDVertex provides:
     float x, y, z;  // screen space x bitand y, camera space z
     float q, u, v;
     float r, g, b, a;
     DWORD clipFlag;
     float csX, csY, csZ; // Camera space coordinates (csZ could go)

       Then I add:
    */
    Tpost *post;
    int RenderingStateHandle;
} TerrainVertex;


typedef struct SpanMinMax
{
    float insideEdge;
    float minEndPoint;
    float maxEndPoint;
    int startDraw;
    int stopDraw;
    int startXform;
    int stopXform;
} SpanMinMax;


typedef struct SpanListEntry
{
    int ring;
    int LOD;
    SpanMinMax Tsector;
    SpanMinMax Rsector;
    SpanMinMax Bsector;
    SpanMinMax Lsector;
} SpanListEntry;


// Used to construct the set of visible terrain posts
typedef struct BoundSegment
{
    Edge edge;
    float end;
} BoundSegment;


// Hold the rotated axes vectors and viewer location in units of level posts at each LOD
typedef struct LODdataBlock
{
    float Xstep[3];
    float Ystep[3];
    float Zstep[3];
    int availablePostRange;
    int centerRow;
    int centerCol;
    BOOL glueOnBottom;
    BOOL glueOnLeft;
    // int RenderingStateHandle;
} LODdataBlock;


// Used to pass information for drawing the sky among the various helper routines
typedef struct HorizonRecord
{
    float bandAngleUp; // Angular size (in radians) of sky haze band
    float hx; //    * Location of the horizon line in screen space
    float hy; //    *
    float vx; //    *
    float vy; //    ^
    float vxUp; //   * Location of the top of the haze band
    float vyUp; //    ^
    float vxDn; //   * Location of the bottom of the terrain filler band
    float vyDn; //    ^
    int horeffect; // Flag indicating type of sun effect on horizon
    float hazescale; // Strength of sun effect at sun location
    float rhazescale; // Strength of sun effect at right edge
    float lhazescale; // Strength of sun effect at left edge
    Tpoint sunEffectPos; // Sun position with respect to horizon
    Tcolor sunEffectColor; // Color of sun at horizon
} HorizonRecord;


class RenderOTW : public Render3D
{
public:
    RenderOTW();
    virtual ~RenderOTW();
protected:
    // Control states
    // BOOL smoothed; // Smooth shading state (on or off)
    BOOL dithered; // Dithering state (on or off)
    BOOL hazed; // Turns hazing on or off (TRUE => "On")
    BOOL skyRoof; // Turns the "roof" over the world on or off
    BOOL filtering; // Turns texture filtering on or off
    //    BOOL    alphaMode;      // Enables alpha blending in special effects

    // MPR state handles used to draw terrain at various distances
    DWORD state_fore; // Normally perspective corrected
    DWORD state_near; // Normally plain texture
    DWORD state_mid; // Normally fading texture
    DWORD state_far; // Normally gouraud shaded

    // Lighting angles
    float lightTheta;
    float lightPhi;

    // Sky properties
    Tcolor sky_color; // This is the color of the sky above the horizon
    Tcolor haze_ground_color; // This is the color distant terrain blends toward
    Tcolor  ground_color; // JAM 03Dec03
    Tcolor earth_end_color; // This is the color of the ground at the horizon
    Tcolor haze_sky_color; // This is the color at which the sky blend starts
    /*JAM 17Sep03 float haze_start; // The distance (in feet) from the viewer at which hazing starts
     float haze_depth;
     float blend_start;
     float blend_depth;*/
    float SunGlareValue;
    float SunGlareWashout;

    // weather properties
    float visibility; // what sort of vis there is.
    float rainFactor; // what sort of rain, 0 none, else 0-1 gives heaviness
    float snowFactor; // what sort of snow, 0 none, else 0-1 gives heaviness
    float brightness;   // how bright things are given the cloud thickness overhead
    bool thunderAndLightning; // is this likely....
    bool thunder; // set if we should hear thunder
    unsigned long thundertimer; // when to play
    bool Lightning; // only for nukes
    float lightningtimer; // for nuke lightning

    // Terrain drawing state
    int textureLevel;

    // How much of the players vision has been lost (normally 0s)
    float tunnelPercent;
    float tunnelAlphaWidth;
    float tunnelSolidWidth;
    DWORD tunnelColor;
    DWORD cloudColor;

    // Points which define the corners of the viewing volume
    float rightX1;
    float rightX2;
    float rightY1;
    float rightY2;
    float leftX1;
    float leftX2;
    float leftY1; 
    float leftY2;

    // Edges which define the viewing volume
    Edge front_edge;
    Edge back_edge;
    Edge left_edge;
    Edge right_edge;


    // The maximum expected length of any one span in total and measured outward from zero
    int maxSpanExtent;
    int maxSpanOffset;

    // The maximum expected number of rings we'll deal with
    int spanListMaxEntries;

    // Pointers to the memory we use to build the span lists
    SpanListEntry *spanList;
    SpanListEntry *firstEmptySpan;

    // An array of pointers to transformed vertex buffers - one per LOD
    // These point into the vertexMemory and are setup for + and - indexing
    TerrainVertex **vertexBuffer;

    // An array of transformed vertices (portions used by each LOD)
    TerrainVertex *vertexMemory;

    // Array which records the axes vectors and viewer location in units of level posts at each LOD
    LODdataBlock *LODdata;

    bool GreenMode;
public:
    // Setup and Cleanup need to have additions here, but still call the parent versions
    virtual void Setup(class ImageBuffer *imageBuffer, class RViewPoint *vp);
    virtual void Cleanup(void);

    // Overload this function to get extra work done at start frame
    virtual void StartDraw(void) ;
    virtual void EndDraw(void) ;

    // Select the amount of terrain texturing employed
    void SetTerrainTextureLevel(int level);
    // int  GetTerrainTextureLevel( void ) { return textureLevel; };

    // Set/get rendering settings
    void SetHazeMode(BOOL state);
    
    BOOL GetHazeMode(void);

    // alpha setting
    //JAM 01Dec03 - Deprecated
    // void SetAlphaMode( BOOL state )          { alphaMode = state; };
    // BOOL GetAlphaMode( void )         { return alphaMode; };

    void SetRoofMode(BOOL state);
    BOOL GetRoofMode(void);

    //JAM 01Dec03 - Deprecated
    // void SetSmoothShadingMode( BOOL state ) { smoothed = state; SetupStates(); };
    // BOOL GetSmoothShadingMode( void ) { return smoothed; };

    void SetDitheringMode(BOOL state);
    BOOL GetDitheringMode(void);

    void SetFilteringMode(BOOL state);
    BOOL GetFilteringMode(void);

    //JAM 26Dec03
    float GetRangeOnlyFog(float range);

    float GetValleyFog(float distance, float worldZ);
    Tcolor* GetFogColor(void);

    // Draw the out the window view, including terrain and all registered objects
    void DrawScene(const Tpoint *offset, const Trotation *orientation);
    // this function gets all needed objects round the scene
    void PreLoadScene(const Tpoint *offset, const Trotation *orientation);

    // Special calls used for tunnel vision and cloud occulsion effects
    float GetTunnelPercent(void);
    void SetTunnelPercent(float percent, DWORD color); // CALL _BEFORE_ DrawScene()
    void PostSceneCloudOcclusion(void); // CALL between DrawScene() and FinishFrame()
    void DrawTunnelBorder(); // CALL _AFTER_ FinishFrame()
    bool IsThunder();
    float RainFactor();
    float SnowFactor();
    float Visibility();
    void SetLightning(void);
    void ToggleGreenMode();
    void SetGreenMode(bool state);
    bool GetGreenMode();
    static void SetupTexturesOnDevice(DXContext *rc);
    static void ReleaseTexturesOnDevice(DXContext *rc);
protected:
    // Utility functions used within this class
    void SetupStates(void);

    void ComputeBounds(void);
    void BuildRingList(void);
    void ClipHorizontalSectors(void);
    void ClipVerticalSectors(void);
    void BuildCornerSet(void);
    void TrimCornerSet(void);

    void BuildVertexSet(void);
    void TransformVertexSet(void);
    void TransformRun(int row, int col, int stop, int LOD, BOOL do_row);

    //JAM 03Dec03
    virtual void ComputeVertexColor(TerrainVertex *vert, Tpost *post, float distance, float x = 0, float y = 0);

    void PreSceneCloudOcclusion(float percent, DWORD color);
    void DrawTerrainRing(SpanListEntry *span);
    void DrawConnectorRing(SpanListEntry *span);
    void DrawGapFiller(SpanListEntry *span);

    BOOL DrawSky(void);
    void DrawSkyNoRoof(void);
    void DrawSkyAbove(void);
    void DrawSkyBelow(void);
    void DrawGroundAndObjects(class ObjectDisplayList *objectList);
    void DrawCloudsAndObjects(class ObjectDisplayList *clouds, class ObjectDisplayList *objects);
    void DrawWeather(const Trotation *orientation);

    // These are overridden by the "green" displays (TV bitand IR)
    virtual void ProcessColor(Tcolor *color);
    virtual void DrawSun(void);
    virtual void DrawMoon(void);
    virtual void DrawStars(void);
    virtual void ComputeHorizonEffect(HorizonRecord *pHorizon);

    int DrawCelestialBody(Tpoint *center, float dist, float alpha = 1.f, float r = 1.f, float g = 1.f, float b = 1.f);  //JAM 30Sep03
    void DrawClearSky(HorizonRecord *pHorizon);
    void DrawSkyHazeBand(HorizonRecord *pHorizon);
    void DrawFillerToHorizon(HorizonRecord *pHorizon);
    void SetSpecularFog(float fogValue); //JAM 17Sep03
    void DrawTerrainSquare(int r, int c, int LOD);
    void DrawUpConnector(int r, int c, int LOD);
    void DrawRightConnector(int r, int c, int LOD);
    void DrawDownConnector(int r, int c, int LOD);
    void DrawLeftConnector(int r, int c, int LOD);

    // Handle time of day and lighting stuff
    virtual void SetTimeOfDayColor(void);
    virtual void AdjustSkyColor(void);
    static void TimeUpdateCallback(void *self);
public:
    //RViewPoint vtemp;
    class RViewPoint *viewpoint;
public:
    static const float PERSPECTIVE_RANGE;
    static const float NVG_TUNNEL_PERCENT;
    // static const float FOG_MIN;
    // static const float FOG_MAX;

    //JAM 24Sep03
    static const float DETAIL_RANGE;
    float haze_start;
    float haze_depth;
    static Texture NoiseFore;
    static Texture NoiseNear;
    //JAM

    static const float MOON_DIST;
    static const float SUN_DIST;

    static const float MOST_SUN_GLARE_DIST;
    static const float MIN_SUN_GLARE;

    static const float ROOF_REPEAT_COUNT;

    static const float HAZE_ALTITUDE_FACTOR;
    static const float GLARE_FACTOR;

    static class Texture texRoofTop;
    static class Texture texRoofBottom;

    static Texture ddsTest;
};


#endif // _RENDEROW_H_
