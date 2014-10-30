/***************************************************************************\
    RealWeather.cpp
    Miro "Jammer" Torrielli
    09Nov03

 - And then there was light
\***************************************************************************/
#include "d3d.h"
#include "polylib.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"
#include "TimeMgr.h"
#include "simdrive.h"
#include "aircrft.h"
#include "airframe.h"
#include "otwdrive.h"
#include "Rviewpnt.h"
#include "RenderOW.h"
#include "ObjList.h"
#include "TOD.h"
#include "TMap.h"
#include "radix.h"
#include "SoundFX.h"
#include "FakeRand.h"
#include "grinline.h"
#include "TimerThread.h"
#include "RealWeather.h"
#include "Real2DCloud.h"
#include "Real3DCloud.h"
#include "drawsgmt.h"
#include "CmpClass.h"
//#include "weather.h"
#include "dispopts.h"
#include "Urlmon.h"//Cobra added this so FreeFalcon connects to the net
//works with urlmon.lib which needs to be present for project to build.
#include "xmmintrin.h"

extern int g_nGfxFix;
extern int TimeOfDayGeneral();
extern bool g_bHearThunder;
Tcolor RealWeather::litCloudColor = { 0 };
float RealWeather::WeatherQuality, RealWeather::WeatherQualityRate;
DWORD RealWeather::WeatherQualityStep, RealWeather::WeatherQualityElapsed;
//extern Weather *TheWeather;


//static const float TEX_UV_LSB = 1.f/1024.f;
//static const float TEX_UV_MIN = TEX_UV_LSB;
//static const float TEX_UV_MAX = 1.f-TEX_UV_LSB;


// The Base indexes for Cirrus and Cirrcum
#define FIRST_CIRRUS_INDEX 0
#define FIRST_CIRCUM_INDEX 4

// COBRA - DX- These are the coordibnates for a 4x4 items Texture
const float UVCoords4X4[16][4][2] =
{

    {{  0.00f,  0.00f, }, { 0.25f, 0.00f }, { 0.25f, 0.25f}, {0.00f, 0.25f}},
    {{  0.25f,  0.00f, }, { 0.50f, 0.00f }, { 0.50f, 0.25f}, {0.25f, 0.25f}},
    {{  0.50f,  0.00f, }, { 0.75f, 0.00f }, { 0.75f, 0.25f}, {0.50f, 0.25f}},
    {{  0.75f,  0.00f, }, { 1.00f, 0.00f }, { 1.00f, 0.25f}, {0.75f, 0.25f}},

    {{  0.00f,  0.25f, }, { 0.25f, 0.25f }, { 0.25f, 0.50f}, {0.00f, 0.50f}},
    {{  0.25f,  0.25f, }, { 0.50f, 0.25f }, { 0.50f, 0.50f}, {0.25f, 0.50f}},
    {{  0.50f,  0.25f, }, { 0.75f, 0.25f }, { 0.75f, 0.50f}, {0.50f, 0.50f}},
    {{  0.75f,  0.25f, }, { 1.00f, 0.25f }, { 1.00f, 0.50f}, {0.75f, 0.50f}},

    {{  0.00f,  0.50f, }, { 0.25f, 0.50f }, { 0.25f, 0.75f}, {0.00f, 0.75f}},
    {{  0.25f,  0.50f, }, { 0.50f, 0.50f }, { 0.50f, 0.75f}, {0.25f, 0.75f}},
    {{  0.50f,  0.50f, }, { 0.75f, 0.50f }, { 0.75f, 0.75f}, {0.50f, 0.75f}},
    {{  0.75f,  0.50f, }, { 1.00f, 0.50f }, { 1.00f, 0.75f}, {0.75f, 0.75f}},

    {{  0.00f,  0.75f, }, { 0.25f, 0.75f }, { 0.25f, 1.00f}, {0.00f, 1.00f}},
    {{  0.25f,  0.75f, }, { 0.50f, 0.75f }, { 0.50f, 1.00f}, {0.25f, 1.00f}},
    {{  0.50f,  0.75f, }, { 0.75f, 0.75f }, { 0.75f, 1.00f}, {0.50f, 1.00f}},
    {{  0.75f,  0.75f, }, { 1.00f, 0.75f }, { 1.00f, 1.00f}, {0.75f, 1.00f}}
};


inline void RealWeather::DrawStratus(Tpoint *position, int txtIndex)
{
    if (weatherCondition == SUNNY) return;

    // Bad Weather and inside overcast
    if (InsideOvercast()) return;

    // COBRA - DX - Setup the Squares in the 2D DX Engine for clouds
    TheDXEngine.DX2D_SetupSquareCx(1.0f, 1.0f);
    // the Cloud vertices
    D3DDYNVERTEX Quad[4];

    if (realWeather->weatherCondition < FAIR) txtIndex += FIRST_CIRRUS_INDEX;
    else if ((realWeather->weatherCondition == FAIR) and (ShadingFactor < 5)) txtIndex += FIRST_CIRCUM_INDEX;

    // Assign textures Coord
    if (realWeather->weatherCondition < FAIR or ((realWeather->weatherCondition == FAIR) and (ShadingFactor < 5)))
    {
        Quad[0].tu = UVCoords4X4[txtIndex][0][0], Quad[0].tv = UVCoords4X4[txtIndex][0][1];
        Quad[1].tu = UVCoords4X4[txtIndex][1][0], Quad[1].tv = UVCoords4X4[txtIndex][1][1];
        Quad[2].tu = UVCoords4X4[txtIndex][2][0], Quad[2].tv = UVCoords4X4[txtIndex][2][1];
        Quad[3].tu = UVCoords4X4[txtIndex][3][0], Quad[3].tv = UVCoords4X4[txtIndex][3][1];
    }
    else
    {
        // Assign textures Coord
        Quad[0].tu = TEX_UV_MIN, Quad[0].tv = TEX_UV_MIN;
        Quad[1].tu = TEX_UV_MAX, Quad[1].tv = TEX_UV_MIN;
        Quad[2].tu = TEX_UV_MAX, Quad[2].tv = TEX_UV_MAX;
        Quad[3].tu = TEX_UV_MIN, Quad[3].tv = TEX_UV_MAX;
    }


    Quad[0].dwColour = Stratus1Color, Quad[0].dwSpecular = 0x000000;
    Quad[1].dwColour = Stratus1Color, Quad[1].dwSpecular = 0x000000;
    Quad[2].dwColour = Stratus1Color, Quad[2].dwSpecular = 0x000000;
    Quad[3].dwColour = Stratus1Color, Quad[3].dwSpecular = 0x000000;

    Quad[0].pos.x = -stratusRadius, Quad[0].pos.y = stratusRadius, Quad[0].pos.z = 0;
    Quad[1].pos.x = stratusRadius, Quad[1].pos.y = stratusRadius, Quad[1].pos.z = 0;
    Quad[2].pos.x = stratusRadius, Quad[2].pos.y = -stratusRadius, Quad[2].pos.z = 0;
    Quad[3].pos.x = -stratusRadius, Quad[3].pos.y = -stratusRadius, Quad[3].pos.z = 0;

    // Draw the Square
    if (realWeather->weatherCondition < FAIR or ((realWeather->weatherCondition == FAIR) and (ShadingFactor < 5)))
        TheDXEngine.DX2D_AddQuad(LAYER_STRATUS1, 0, (D3DXVECTOR3*)position, Quad, stratusRadius, CirrusCumTextures.TexHandle());
    else
        TheDXEngine.DX2D_AddQuad(LAYER_STRATUS1, 0, (D3DXVECTOR3*)position, Quad, stratusRadius, overcastTexture.TexHandle());
}

inline void RealWeather::DrawStratus2(Tpoint *position, int txtIndex)
{


    if (UnderOvercast()) return;

    // if stratus invisible, do not draw it
    if ( not (Stratus2Color bitand 0xff000000) or ShadingFactor < 2.0f) return;

    // COBRA - DX - Setup the Squares in the 2D DX Engine for clouds
    TheDXEngine.DX2D_SetupSquareCx(1.0f, 1.0f);

    // the Cloud vertices
    D3DDYNVERTEX Quad[4];

    // Offset for Cirrus Cumulus
    txtIndex += FIRST_CIRCUM_INDEX;

    // Assign textures Coord
    Quad[0].tu = UVCoords4X4[txtIndex][0][0], Quad[0].tv = UVCoords4X4[txtIndex][0][1];
    Quad[1].tu = UVCoords4X4[txtIndex][1][0], Quad[1].tv = UVCoords4X4[txtIndex][1][1];
    Quad[2].tu = UVCoords4X4[txtIndex][2][0], Quad[2].tv = UVCoords4X4[txtIndex][2][1];
    Quad[3].tu = UVCoords4X4[txtIndex][3][0], Quad[3].tv = UVCoords4X4[txtIndex][3][1];

    Quad[0].dwColour = Stratus2Color, Quad[0].dwSpecular = 0x000000;
    Quad[1].dwColour = Stratus2Color, Quad[1].dwSpecular = 0x000000;
    Quad[2].dwColour = Stratus2Color, Quad[2].dwSpecular = 0x000000;
    Quad[3].dwColour = Stratus2Color, Quad[3].dwSpecular = 0x000000;

    Quad[0].pos.x = -stratusRadius, Quad[0].pos.y = stratusRadius, Quad[0].pos.z = 0;
    Quad[1].pos.x = stratusRadius, Quad[1].pos.y = stratusRadius, Quad[1].pos.z = 0;
    Quad[2].pos.x = stratusRadius, Quad[2].pos.y = -stratusRadius, Quad[2].pos.z = 0;
    Quad[3].pos.x = -stratusRadius, Quad[3].pos.y = -stratusRadius, Quad[3].pos.z = 0;

    // Draw the Cloud
    TheDXEngine.DX2D_AddQuad(LAYER_STRATUS2, 0, (D3DXVECTOR3*)position, Quad, stratusRadius, CirrusCumTextures.TexHandle());
}



inline void RealWeather::DrawCumulus(Tpoint *position, int txtIndex, float Radius)
{
    if (weatherCondition not_eq FAIR) return;

    float minFog = 0.2f;

    // COBRA - DX - Setup the Squares in the 2D DX Engine for clouds
    // RV - I-Hawk - X and Y squares are same size

#if CLOUDS_FIX

    TheDXEngine.DX2D_SetupSquareCx(1.0f, 1.0f);

#else

    TheDXEngine.DX2D_SetupSquareCx(1.0f, 0.5f);

#endif

    // the Cloud vertices
    D3DDYNVERTEX Quad[4];

    // Assign textures Coord
    Quad[0].tu = UVCoords4X4[txtIndex][0][0], Quad[0].tv = UVCoords4X4[txtIndex][0][1];
    Quad[1].tu = UVCoords4X4[txtIndex][1][0], Quad[1].tv = UVCoords4X4[txtIndex][1][1];
    Quad[2].tu = UVCoords4X4[txtIndex][2][0], Quad[2].tv = UVCoords4X4[txtIndex][2][1];
    Quad[3].tu = UVCoords4X4[txtIndex][3][0], Quad[3].tv = UVCoords4X4[txtIndex][3][1];
    // Assign Colors
    Quad[0].dwColour = CloudHiColor, Quad[0].dwSpecular = 0x000000;
    Quad[1].dwColour = CloudHiColor, Quad[1].dwSpecular = 0x000000;
    Quad[2].dwColour = CloudLoColor, Quad[2].dwSpecular = 0x000000;
    Quad[3].dwColour = CloudLoColor, Quad[3].dwSpecular = 0x000000;

    // RV - I-Hawk - different positioning

#if CLOUDS_FIX

    Quad[0].pos.y = -puffRadius * 2.0f, Quad[0].pos.z = -puffRadius * 2.0f, Quad[0].pos.x = 0;
    Quad[1].pos.y = puffRadius * 2.0f, Quad[1].pos.z = -puffRadius * 2.0f, Quad[1].pos.x = 0;
    Quad[2].pos.y = puffRadius * 2.0f, Quad[2].pos.z = puffRadius * 2.0f, Quad[2].pos.x = 0;
    Quad[3].pos.y = -puffRadius * 2.0f, Quad[3].pos.z = puffRadius * 2.0f, Quad[3].pos.x = 0;

#else

    Quad[0].pos.y = -Radius * 3.0f, Quad[0].pos.z = -Radius * 1.5f, Quad[0].pos.x = 0;
    Quad[1].pos.y = Radius * 3.0f, Quad[1].pos.z = -Radius * 1.5f, Quad[1].pos.x = 0;
    Quad[2].pos.y = Radius * 3.0f, Quad[2].pos.z = Radius * 1.5f, Quad[2].pos.x = 0;
    Quad[3].pos.y = -Radius * 3.0f, Quad[3].pos.z = Radius * 1.5f, Quad[3].pos.x = 0;

#endif

    // Draw the Cloud
    TheDXEngine.DX2D_AddQuad(LAYER_GROUND, POLY_BB , (D3DXVECTOR3*)position, Quad, Radius, CumulusTextures.TexHandle());
}


// return the observer order
// LOW    = Ground < Z <= Stratus 1
// MIDDLE = Stratus 1 < Z <= Stratus 2
// HI     = Stratus 2 < Z
DWORD RealWeather::GetObserverOrder(float ZPosition)
{
    if (ZPosition >= stratusZ) return OBSERVER_LOW;

    if (ZPosition >= stratus2Z) return OBSERVER_MIDDLE;

    return OBSERVER_HI;
}



///////////////////////////////////////////////// VIEW ORDER TABLES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// RED - The various drawing orders absed on weather and viewver position
// WARNING  LAYER_TOP are the overall objects, IT CLOSES THE DRAWING, so MUST BE ALWAYS PRESENT

// * SUNNY VIEW ORDER *
const DWORD SunnyDrawOrder[MAX_OBSERVER_POSITIONS][MAX_2D_LAYERS] =
{
    {LAYER_ROOF, LAYER_STRATUS2, LAYER_MIDDLE, LAYER_STRATUS1, LAYER_GROUND, LAYER_TOP}, // LOW
    {LAYER_ROOF, LAYER_STRATUS2, LAYER_GROUND, LAYER_STRATUS1, LAYER_MIDDLE, LAYER_TOP}, // MIDDLE
    {LAYER_GROUND, LAYER_STRATUS1, LAYER_MIDDLE, LAYER_STRATUS2, LAYER_ROOF, LAYER_TOP}, // HI
};

// * POOR VIEW ORDER *
const DWORD PoorDrawOrder[MAX_OBSERVER_POSITIONS][MAX_2D_LAYERS] =
{
    {LAYER_STRATUS1, LAYER_GROUND, LAYER_TOP}, // LOW
    {LAYER_ROOF, LAYER_STRATUS2, LAYER_STRATUS1, LAYER_MIDDLE, LAYER_TOP}, // MIDDLE
    {LAYER_STRATUS1, LAYER_MIDDLE, LAYER_STRATUS2, LAYER_ROOF, LAYER_TOP}, // HI
};


// Function setting the Alpha/2D stuff deraw ordering, based on weather and observer Z position
void RealWeather::SetDrawingOrder(float ZPosition)
{
    // get the observer order
    DWORD Observer = GetObserverOrder(ZPosition);

    // check weather status, assing drawing order based on it
    switch (weatherCondition)
    {

            // Fair or good weather
        case SUNNY : // SUNNY AND FAIR USE SAME WEATHER TABLE
        case FAIR :
            TheDXEngine.DX2D_SetDrawOrder((DWORD*)&SunnyDrawOrder[Observer]);
            break;

        case POOR : // POOR / INCLEMENT USE SAME TABLE
        case INCLEMENT :
            TheDXEngine.DX2D_SetDrawOrder((DWORD*)&PoorDrawOrder[Observer]);
            break;


    }


}


RealWeather::RealWeather()
{
    metar = NULL;
    renderer = NULL;
    real2DClouds = NULL;
    real3DClouds = NULL;
    viewerZ = viewerX = viewerY = 0;
    rainX = rainY = rainZ = 0;
    ZeroMemory(&lightningPos, sizeof(Tpoint));
    belowLayer = insideLayer = greenMode = bSetup = FALSE;

    // Linear Fog stuff
    LinearFogStatus = false;
    LinearFogLimit = 10000.0f;
    // RV - Biker - Not used atm
}

RealWeather::~RealWeather()
{
    if (real3DClouds or real2DClouds)
    {
        Cleanup();
    }

    //Cobra this might not be necessary anymore
    if (metar)
        delete metar;
}

void RealWeather::Setup(ObjectDisplayList* cumulusList, ObjectDisplayList* stratusList)
{
    if (bSetup == TRUE)
    {
        return;
    }

    if ( not DisplayOptions.bZBuffering)
    {
        int i;

        if (cumulusList)
        {
            real3DClouds = new Real3DCloud[MAX_NUM_DRAWABLES];
        }

        if (stratusList)
        {
            real2DClouds = new Real2DCloud[MAX_NUM_DRAWABLES];
        }

        for (i = 0; i < MAX_NUM_DRAWABLES; i++)
        {
            real2DClouds[i].Setup(stratusList);
            real3DClouds[i].Setup(cumulusList);
        }
    }

    oldTimeMS = 0;
    startMS = SimLibElapsedTime - 200;
    intervalMS = max((rand() % 10) * SEC_TO_MSEC, 10 * SEC_TO_MSEC);

    bSetup = TRUE;
    didOnce = drawLightning = isLightning = FALSE;
}

void RealWeather::SetRenderer(RenderOTW *Renderer)
{
    renderer = Renderer;
}

void RealWeather::RefreshWeather(RenderOTW *Renderer)
{
    renderer = Renderer;

    viewerX = renderer->viewpoint->X();
    viewerY = renderer->viewpoint->Y();
    viewerZ = renderer->viewpoint->Z();

    TheTimeOfDay.SetScaleFactor(0);

    // RED - Update viewer / weather status
    if (weatherCondition > FAIR)
    {
        LoOvercast = stratusZ + stratusDepth / 2.0f;
        HiOvercast = stratusZ - stratusDepth / 2.0f;
        MidOvercast = stratusZ;
        // RED - default to inside Overcast, the check for conditions
        InsideOVCST = true, OverOVCST = false, UnderOVCST = false;

        if (viewerZ <= HiOvercast) InsideOVCST = UnderOVCST = false, OverOVCST = true;

        if (viewerZ >= LoOvercast) InsideOVCST = OverOVCST = false, UnderOVCST = true;
    }
    else
    {
        // No condition is true if nice weather
        InsideOVCST = UnderOVCST = OverOVCST = false;
    }

    if (UnderOvercast() or InsideOvercast())
    {
        float OvercastShading = .8f + ShadingFactor * 0.05f;

        // if under Overcast
        if (UnderOvercast())
        {
            TheTimeOfDay.SetScaleFactor(OvercastShading);
        }

        // if inside overcast
        if (InsideOvercast())
        {
            TheTimeOfDay.SetScaleFactor(OvercastShading - ((LoOvercast -  viewerZ) / stratusDepth) * OvercastShading);
        }
    }
    else
    {
        // The weather light, default to 0.0f
        TheTimeOfDay.SetScaleFactor(0.0f);
    }

    // update all the colors stuff
    TheTimeOfDay.UpdateWeatherColors(weatherCondition);
    TheTimeOfDay.GetTextureLightingColor(&litCloudColor);
    Drawable3DCloud::SetCloudColor(&litCloudColor);
    Drawable2DCloud::SetCloudColor(&litCloudColor);

    //REPORT_VALUE("Weather Quality :", (WeatherQuality * 1000.0f));
    //REPORT_VALUE("Shading Factor :", (ShadingFactor));
    UpdateCells();

    if ( not DisplayOptions.bZBuffering)
        UpdateDrawables();

    // Wather quality moving on every second
    if (TheTimeManager.GetClockTime() - WeatherQualityElapsed >= 1000)
    {
        // if Local game, update data
        if ( not vuLocalGame or vuLocalGame->IsLocal())
        {
            if (WeatherQualityStep) WeatherQualityStep--;
            else UpdateWeatherQuality();

            WeatherQuality += WeatherQualityRate;

            if (WeatherQuality > 1.0f) WeatherQuality = 1.0f;

            if (WeatherQuality < 0.0f) WeatherQuality = 0.0f;
        }

        WeatherQualityElapsed = TheTimeManager.GetClockTime();
    }


    // RED  - Update visible height if an overcasting is prsent
    // used by DrawableBSP to update its own visibility
    // if under the overcast layer
    if (weatherCondition > FAIR and viewerZ < stratusZ) VisibleHeight = stratusZ;
    // if weather fine or Viever under the overcast, give a default positive value ( negative is higher )
    // so that positive makes ALWAYS VISIBLE
    else VisibleHeight = 10000.0f;

    // Setup Overcast parameters
    float StratusHalf = stratusDepth / 2.0f;

    // Update fog evolution with weather
    if (weatherCondition == POOR) LinearFogLimit = -stratusZ * 4.0f;

    if (weatherCondition == INCLEMENT)
    {
        stratusZ = -15000.0f * WeatherQuality - 5000.0f;
        float sDistance = -stratusZ - (stratusDepth / 2.0f) ;
        LinearFogLimit = sDistance * sDistance / 5000.0f + 500.0f, ShadingFactor = 10.0f - 10.0f * WeatherQuality;
    }

    // this creates the randomic fog intensisty effect on movement
    if (LinearFogStatus)
    {
        // get the viewer Delta distance
        float dx = viewerX - LastViewPos.x, dy = viewerY - LastViewPos.y, dz = viewerZ - LastViewPos.z;
        float Distance = sqrtf(dx * dx + dy * dy + dz * dz);
        float ElapsedTime = (float)(TheTimeManager.GetClockTime() - LastTime) * .001f;

        // fog varies based on travelled distace / 1000 ( arbitrary ) + a little offset
        LinearFogDelta += PRANDFloat() * (Distance / 10.0f + 0.1f) * ElapsedTime;

        // consistency check
        if (LinearFogDelta > 1.0f)  LinearFogDelta = 1.0f;

        if (LinearFogDelta < 0.0f) LinearFogDelta = 0.0f;

        // update fields
        LastTime = TheTimeManager.GetClockTime();
        LastViewPos.x = viewerX, LastViewPos.y = viewerY, LastViewPos.z = viewerZ;

    }

    // Check for Fogdensity, if INSIDE Overcast Layer, the Fog range is lowered to
    // simulate clouds tickness
    if (InsideOvercast())
    {
        // get how much is into the clouds layer
        float DeltaCx = fabs(-viewerZ + stratusZ) / StratusHalf;
        // check if near layer lmits, and update fog...
        //LinearFogUsed = 20.0f + 1000.0f * LinearFogUsed;
        LinearFogUsed = 50.0f + 200.0f * (LinearFogDelta + DeltaCx);
    }
    else
        LinearFogUsed = LinearFogLimit, LinearFogDelta = 0.9f;


}

void RealWeather::UpdateLighting()
{
    Tpoint lv;
    float hyp, adj, opp;
    TheTimeOfDay.GetLightDirection(&lv);

    hyp = SqrtF(lv.x * lv.x + lv.y * lv.y + lv.z * lv.z);
    adj = -lv.z;
    opp = SqrtF(hyp * hyp - adj * adj);

    sunMag = opp * (-stratusZ) / adj / 10.f;
    sunAngle = Atan(opp, adj);
    sunYaw = glConvertToRadian(TheTimeOfDay.GetSunYaw()) + PI;

    if (sunYaw > 2.f * PI) sunYaw -= 2.f * PI;
}

// COBRA - DX - this function generates a cloud
void RealWeather::GenerateCloud(DWORD row, DWORD col)
{
    DWORD i;

    weatherCellArray[row][col].cloudPosX = (float)(row * cellSize + rand() % cellSize);
    weatherCellArray[row][col].cloudPosY = (float)(col * cellSize + rand() % cellSize);

    weatherCellArray[row][col].sTxtIndex = rand() % NUM_STRATUS_TEXTURES;

    for (i = 0; i < NUM_3DCLOUD_POLYS; i++)
        weatherCellArray[row][col].cTxtIndex[i] = rand() % NUM_CUMULUS_TEXTURES;

    for (i = 0; i < NUM_3DCLOUD_POLYS; i++)
        weatherCellArray[row][col].cPntIndex[i] = rand() % NUM_3DCLOUD_POINTS;

    // COBRA - DX - Random radius stuff
    // RV - I-Hawk - Using larger radius

#if CLOUDS_FIX

    weatherCellArray[row][col].Radius = 10000.f + 1000.f * PRANDFloatPos() * (float)ShadingFactor;

#else

    weatherCellArray[row][col].Radius = 3000.f + 300.f * PRANDFloatPos() * (float)ShadingFactor;

#endif

}

void RealWeather::GenerateClouds(bool bRandom)
{
    cloudRadius = 20000.f; // 10000.f; // Cobra - FRB - need larger radius due to z-fighting fix
    stratusRadius = 55000.f;

    // TODO - RED - Setup something for MP gaming
    if ( not bRandom)
    {
        FILE *fp;
        char fname[] = "campaign\\save\\mpwcells.bin";

        fp = fopen(fname, "rb");
        fread(&weatherCellArray, sizeof(WeatherCell)*MAX_NUM_CELLS * MAX_NUM_CELLS, 1, fp);
        fclose(fp);
    }
    else
    {
        int row, col;

        shadowCell = 2;
        drawableCell = 1;

        /*if(weatherCondition > FAIR
        and (-viewerZ) > (-stratusZ) and (-viewerZ) < (-stratusZ)+stratusDepth)
        {
         numCells = 5;
         cellSize = 7168;
         // COBRA - DX - Passed as single cell feature
         puffRadius = 3000.f + 500.f * PRANDFloatPos();
        }
        else*/
        {
            numCells = 9;
            cellSize = 57344;
            // RV - I-Hawk - using this value with the clouds fix
            puffRadius = 3000.f;
        }

        halfSize = cellSize / 2;
        halfCells = (numCells - 1) / 2;
        vpShift = halfCells * cellSize;


        weatherShiftX = (int)viewerX - vpShift + halfSize;
        weatherShiftY = (int)viewerY - vpShift + halfSize;

        ZeroMemory(weatherCellArray, sizeof(WeatherCell)*MAX_NUM_CELLS * MAX_NUM_CELLS);

        for (row = 0; row < numCells; row++)
        {
            for (col = 0; col < numCells; col++) GenerateCloud(row, col);
        }

        UpdateCondition();
        // RED - ENABLE IT TO SAVE AN MP SESSION WEATHER
        /*FILE *fp;
        char fname[] = "campaign\\save\\mpwcells.bin";

        fp = fopen(fname,"wb");
        fwrite(&weatherCellArray,sizeof(WeatherCell)*MAX_NUM_CELLS*MAX_NUM_CELLS,1,fp);
        fclose(fp);*/


    }
}


// to be called to rebuild shadings and so on
void RealWeather::UpdateCondition(void)
{
    // SUNNY
    if (weatherCondition < FAIR) ShadingFactor = PRANDFloatPos() * 3.0f;

    // FAIR OR WORST
    if (weatherCondition == FAIR) ShadingFactor = PRANDFloatPos() * 9.0f;

    // FAIR OR WORST
    if (weatherCondition > FAIR)
    {

        LinearFogStatus = true;
        ShadingFactor = PRANDFloatPos() * 5.0f + 5.0f;
        // Weather variance stuff
        UpdateWeatherQuality();

        // Update Weather Quality based on Stratus Z
        //WeatherQuality = PRANDFloatPos();
        //WeatherQuality = (stratusZ + 5000.0f) / -15000.0f;
        //I-Hawk - more randomized weatherQuality factor
        WeatherQuality = (((stratusZ + 5000.0f) / -15000.0f) + PRANDFloatPos()) * 0.5f  ;

        if (WeatherQuality > 1.0f) WeatherQuality = 1.0f;

        if (WeatherQuality < 0.0f) WeatherQuality = 0.0f;

        WeatherQualityElapsed = TheTimeManager.GetClockTime();

    }
    else
    {
        LinearFogStatus = false;
    }
}


void RealWeather::UpdateCells()
{
    if ( not renderer) return;


    //START_PROFILE("Clouds");

    DWORD timeMS;
    int row, col;
    Tpoint ep = { 0 };

    if (oldTimeMS)
    {
        timeMS = SimLibElapsedTime;
        ep.x = windSpeed * 0.9113f * (timeMS - oldTimeMS) / 100.f;
        RotatePoint(&ep, 0, 0, windHeading);
    }
    else
    {
        weatherShiftX = (int)viewerX - vpShift + halfSize;
        weatherShiftY = (int)viewerY - vpShift + halfSize;

        oldTimeMS = SimLibElapsedTime;

        return;
    }

    weatherShiftX += (int)ep.x;
    weatherShiftY += (int)ep.y;

    if ((viewerY - vpShift) - weatherShiftY > cellSize)
    {
        for (col = 0; col < numCells - 1; col++)
        {
            for (row = 0; row < numCells; row++)
            {
                weatherCellArray[row][col] = weatherCellArray[row][col + 1];
                weatherCellArray[row][col].cloudPosY -= cellSize;
            }
        }

        for (row = 0; row < numCells; row++) GenerateCloud(row, col);

        weatherShiftY += cellSize;
    }

    if (weatherShiftY - (viewerY - vpShift) > cellSize)
    {
        for (col = numCells - 1; col; col--)
        {
            for (row = 0; row < numCells; row++)
            {
                weatherCellArray[row][col] = weatherCellArray[row][col - 1];
                weatherCellArray[row][col].cloudPosY += cellSize;
            }
        }

        for (row = 0; row < numCells; row++)  GenerateCloud(row, col);

        weatherShiftY -= cellSize;
    }

    if ((viewerX - vpShift) - weatherShiftX > cellSize)
    {
        for (row = 0; row < numCells - 1; row++)
        {
            for (col = 0; col < numCells; col++)
            {
                weatherCellArray[row][col] = weatherCellArray[row + 1][col];
                weatherCellArray[row][col].cloudPosX -= cellSize;
            }
        }

        for (col = 0; col < numCells; col++)  GenerateCloud(row, col);

        weatherShiftX += cellSize;
    }


    if (weatherShiftX - (viewerX - vpShift) > cellSize)
    {
        for (row = numCells - 1; row; row--)
        {
            for (col = 0; col < numCells; col++)
            {
                weatherCellArray[row][col] = weatherCellArray[row - 1][col];
                weatherCellArray[row][col].cloudPosX += cellSize;
            }
        }

        for (col = 0; col < numCells; col++)  GenerateCloud(row, col);

        weatherShiftX -= cellSize;
    }

    oldTimeMS = SimLibElapsedTime;
    //STOP_PROFILE("Clouds");
}

void RealWeather::UpdateDrawables()
{
    bool DrawTheRain = false;

    if ( not renderer) return;

    //START_PROFILE("Clouds");

    // Get the Observer position order
    DWORD ObserverPos = GetObserverOrder(viewerZ);
    // get the original startus position
    float Stratus1Z = stratusZ;

    // if weather bad, position is upper or lower the stratus ( overcast ) layer
    if (weatherCondition > FAIR)
    {
        // Stratus 1 is mover lower or upper based on observer position
        if (ObserverPos == OBSERVER_LOW) Stratus1Z += stratusDepth / 2.0f;
        else Stratus1Z -= stratusDepth / 2.0f;
    }


    // COBRA - DX - setup the Hi bitand Lo colours for the Clouds
    // RV - I-Hawk - added alpha and lowered shading

#if CLOUDS_FIX

    float CloudShading = 1.0f - 0.025f * (float)ShadingFactor;
    float CloudAlpha =  0.9f + (float)ShadingFactor * 0.01f; //1.0f - (float)ShadingFactor * 0.02f;

#else

    float CloudShading = 0.95f - 0.05f * (float)ShadingFactor;
    float CloudAlpha = 0.5f + (float)ShadingFactor * 0.05f;

#endif

    //RV - I-Hawk - Added another factor for keeping the changes in cumulus shading as older code
    float stratusShadingFactor = 0.95f - 0.05f * (float)ShadingFactor;

    float Ambient = TheTimeOfDay.GetDiffuseValue();

    float Stratus1Alpha;
    float StratusShading;

    if (weatherCondition <= FAIR)
    {
        // Stratus 1 Alpha, depends on ShadingFactor and illumination
        Stratus1Alpha = max(0.0f, 1.0f - 0.4f * stratusShadingFactor - Ambient * 0.4f);
        StratusShading = 1.0f - 0.02f * (float)ShadingFactor;
    }
    else
    {
        // if Bad weather, Stratus1 more dense and darker...
        Stratus1Alpha = 0.7f + 0.02f * ShadingFactor;
        StratusShading = (ObserverPos == OBSERVER_LOW) ? 0.7f - 0.03f * (float)ShadingFactor : 0.9f;
    }

    // Stratus 2 Alpha, Depends on Shading and Sun pitch
    float   SunPitch = (float)TheTimeOfDay.GetSunPitch();

    if (SunPitch < 10.0f) SunPitch = 10.0f;

    float Stratus2Alpha = max(0.0f, 1.0f - 0.3f * stratusShadingFactor - SunPitch / 4000.0f);

    if (weatherCondition > FAIR) Stratus2Alpha /= 4.0f;

    CloudHiColor = F_TO_ARGB(CloudAlpha, litCloudColor.r, litCloudColor.g, litCloudColor.b);
    CloudLoColor = F_TO_ARGB(CloudAlpha, litCloudColor.r * CloudShading, litCloudColor.g * CloudShading, litCloudColor.b * CloudShading);

    Stratus2Color = F_TO_ARGB(Stratus2Alpha, litCloudColor.r, litCloudColor.g, litCloudColor.b);
    Stratus1Color = F_TO_ARGB(Stratus1Alpha, litCloudColor.r * StratusShading, litCloudColor.g * StratusShading, litCloudColor.b * StratusShading);

    DWORD clipFlag[4];
    Tpoint wp, vp[4], cumulusPos, stratusPos, shadowPos;
    int i, row, col, sTxtIndex, cTxtIndex, cPntIndex, p = 0, q = 0, r = 0;

    for (row = drawableCell; row < numCells - drawableCell; row++)
    {
        for (col = drawableCell; col < numCells - drawableCell; col++)
        {
            sTxtIndex = weatherCellArray[row][col].sTxtIndex;
            stratusPos.x = weatherCellArray[row][col].cloudPosX + weatherShiftX;
            stratusPos.y = weatherCellArray[row][col].cloudPosY + weatherShiftY;

            stratusPos.z = Stratus1Z;

            // RV - I-Hawk - Removed the lower stratus drawing
            // FRB - Daytime only
            if (TimeOfDayGeneral() == TOD_DAY)
                DrawStratus(&stratusPos, sTxtIndex);

            stratusPos.z = stratus2Z;
            DrawStratus2(&stratusPos, sTxtIndex);

            if (weatherCondition == FAIR or (weatherCondition > FAIR and InsideOvercast()))
            {
                for (i = 0; i < NUM_3DCLOUD_POLYS; i++)
                {
                    cPntIndex = weatherCellArray[row][col].cPntIndex[i];
                    cTxtIndex = weatherCellArray[row][col].cTxtIndex[i];

                    //RV - I-Hawk - the drawing order change
#if CLOUDS_FIX

                    // Do some fake random stuff to decide how to draw
                    if (i % 2)
                    {
                        float sideRandFactor, ZRandFactor;

                        if (i % 3)
                        {
                            sideRandFactor = ZRandFactor = 1;
                        }

                        else
                        {
                            sideRandFactor = -1;
                            ZRandFactor = 0;
                        }

                        // The regulare drawing, but with less space between the puffs
                        // make the clouds more 3D
                        cumulusPos.x = (stratusPos.x + (float)(i - 2) * /*puffRadius*/ weatherCellArray[row][col].Radius * 0.075f * sideRandFactor);
                        cumulusPos.y = (stratusPos.y + (float)(i - 2) * /*puffRadius*/ weatherCellArray[row][col].Radius * 0.075f * sideRandFactor);
                        cumulusPos.z = cumulusZ + 500.0f * ZRandFactor - 5000;
                    }

                    else
                    {
                        // The old-style BMS drawing method
                        cumulusPos.x = stratusPos.x + ((cloudPntList[cPntIndex][0] * 1.8f) / 30.f);
                        cumulusPos.y = stratusPos.y + ((cloudPntList[cPntIndex][2] * 1.8f) / 30.f);
                        cumulusPos.z = cumulusZ - ((cloudPntList[cPntIndex][1] * 1.8f) / 30.f) - 5000;
                    }

#else

                    cumulusPos.x = (stratusPos.x + (float)(i - 2) * weatherCellArray[row][col].Radius * 2.3f);
                    cumulusPos.y = (stratusPos.y + (float)(i - 2) * weatherCellArray[row][col].Radius * 2.3f);
                    cumulusPos.z = cumulusZ - 3000.0f;

#endif


                    if (cumulusPos.z <  stratusZ) cumulusPos.z = (stratusZ + 1500.f);

                    if (DisplayOptions.bZBuffering)
                        DrawCumulus(&cumulusPos, cTxtIndex, weatherCellArray[row][col].Radius);
                    else
                        real3DClouds[q].drawable3DClouds[r++].Update(&cumulusPos, cTxtIndex);

                    // Cobra - Raining under dark cummulus clouds
                    if ((weatherCondition == FAIR) and (ShadingFactor >= 5) and (viewerZ > cumulusPos.z))
                    {
                        float dx = viewerX - cumulusPos.x;
                        float dy = viewerY - cumulusPos.y;
                        float range = FabsF(SqrtF(dx * dx + dy * dy));

                        // Flag that rain is to be drawn
                        if (range < weatherCellArray[row][col].Radius * 0.6f) DrawTheRain = true;
                    }
                }

            }


            p++;
            q++;
            r = 0;

            if (PlayerOptions.ShadowsOn()
               and row >= shadowCell and row < numCells - shadowCell
               and col >= shadowCell and col < numCells - shadowCell)
            {
                shadowPos.x = sunMag;
                shadowPos.y = shadowPos.z = 0;
                RotatePoint(&shadowPos, 0, 0, sunYaw);
                // COBRA - RED - Restored to CloudRadius... too much large causes bad effects on ground...
                float Radius = cloudRadius; //weatherCellArray[row][col].Radius;
                shadowPos.x += weatherCellArray[row][col].cloudPosX + weatherShiftX;
                shadowPos.y += weatherCellArray[row][col].cloudPosY + weatherShiftY;
                shadowPos.z = renderer->viewpoint->GetGroundLevel(shadowPos.x, shadowPos.y);
                renderer->TransformPointToViewSwapped(&shadowPos, &weatherCellArray[row][col].shadowPos);

                wp.x = shadowPos.x + Radius;
                wp.y = shadowPos.y - Radius;
                wp.z = shadowPos.z;
                renderer->TransformPointToViewSwapped(&wp, &vp[0]);

                wp.x = shadowPos.x + Radius;
                wp.y = shadowPos.y + Radius;
                wp.z = shadowPos.z;
                renderer->TransformPointToViewSwapped(&wp, &vp[1]);

                wp.x = shadowPos.x - Radius;
                wp.y = shadowPos.y + Radius;
                wp.z = shadowPos.z;
                renderer->TransformPointToViewSwapped(&wp, &vp[2]);

                wp.x = shadowPos.x - Radius;
                wp.y = shadowPos.y - Radius;
                wp.z = shadowPos.z;
                renderer->TransformPointToViewSwapped(&wp, &vp[3]);

                for (i = 0; i < 4; i++)
                {
                    clipFlag[i]  = GetRangeClipFlags(vp[i].z, 0);
                    clipFlag[i] or_eq GetHorizontalClipFlags(vp[i].x, vp[i].z);
                    clipFlag[i] or_eq GetVerticalClipFlags(vp[i].y, vp[i].z);
                }

                weatherCellArray[row][col].onScreen = ( not clipFlag[0] or not clipFlag[1] or not clipFlag[2] or not clipFlag[3]);

                if ( not weatherCellArray[row][col].onScreen
                   and row >= shadowCell + 1 and row < numCells - shadowCell - 1
                   and col >= shadowCell + 1 and col < numCells - shadowCell - 1)
                {
                    float dx = viewerX - shadowPos.x;
                    float dy = viewerY - shadowPos.y;
                    float range = FabsF(SqrtF(dx * dx + dy * dy));

                    if (range < Radius * 2.f)
                    {
                        weatherCellArray[row][col].onScreen = TRUE;
                    }
                }
            }
        }
    }

    // if Flagged that rain is to be drawn, draw it
    if (DrawTheRain) DrawRain();

    //STOP_PROFILE("Clouds");
}

// RED - New stuff incoming
void RealWeather::Draw()
{
    // Update all clouds
    if (DisplayOptions.bZBuffering) UpdateDrawables();

    // Weather quality check under overcast
    if (weatherCondition == INCLEMENT and UnderOvercast())
    {
        // if worst than just lighting, rain
        if (WeatherQuality >= 0.1f) DrawRain(); // RV - I-Hawk - was 0.75

        // just before rain, lightning
        if (WeatherQuality >= 0.1f) // was 0.65
        {
            if ( not didOnce)
            {
                if ((SimLibElapsedTime - startMS) > intervalMS) didOnce = TRUE;
                else return;
            }
            else
                DoLightning();
        }
    }
}

void RealWeather::DrawRain()
{
#if 0 // FRB - Disable rain for now.  It looks really bad.
    int i;
    Tpoint p0, p1, p2, p3;
    float vel, pitch, yaw, brdsize;
    ThreeDVertex v0, v1, v2, v3;
    SimBaseClass *otwPlatform = OTWDriver.GraphicsOwnship();

    if (otwPlatform)
    {
        yaw = ((SimMoverClass *)otwPlatform)->Yaw();
        vel = min(((SimMoverClass *)otwPlatform)->GetKias(), 30.f);
        pitch = vel * (((SimMoverClass *)otwPlatform)->Pitch() - 90.f * DTR) / 30.f;
    }
    else
        yaw = vel = pitch = 0;

    brdsize = 80.0f;

    p0.x = p0.y = p0.z = 0;

    for (i = 0; i < 200; i++)
    {
        if (targetCompressionRatio)
        {
            rainX = PRANDFloat() * 50.f;
            rainY = PRANDFloat() * 50.f;
            rainZ = PRANDFloat() * 50.f;
        }

        if ( not (rainX > -40 and rainX < 40
             and rainY > -26 and rainY < 26
             and rainZ > -40 and rainZ < 40))
        {
            p0.x = -brdsize + rainX;
            p0.y =  0 + rainY;
            p0.z = -brdsize + rainZ;
            RotatePoint(&p0, pitch, 0, yaw);
            renderer->TransformCameraCentricPoint(&p0, &v0);

            p1.x =  brdsize + rainX;
            p1.y =  0 + rainY;
            p1.z = -brdsize + rainZ;
            RotatePoint(&p1, pitch, 0, yaw);
            renderer->TransformCameraCentricPoint(&p1, &v1);

            p2.x =  brdsize + rainX;
            p2.y =  0 + rainY;
            p2.z =  brdsize + rainZ;
            RotatePoint(&p2, pitch, 0, yaw);
            renderer->TransformCameraCentricPoint(&p2, &v2);

            p3.x = -brdsize + rainX;
            p3.y =  0 + rainY;
            p3.z =  brdsize + rainZ;
            RotatePoint(&p3, pitch, 0, yaw);
            renderer->TransformCameraCentricPoint(&p3, &v3);

            v0.u = TEX_UV_MIN, v0.v = TEX_UV_MIN, v0.q = v0.csZ * Q_SCALE;
            v1.u = TEX_UV_MAX, v1.v = TEX_UV_MIN, v1.q = v1.csZ * Q_SCALE;
            v2.u = TEX_UV_MAX, v2.v = TEX_UV_MAX, v2.q = v2.csZ * Q_SCALE;
            v3.u = TEX_UV_MIN, v3.v = TEX_UV_MAX, v3.q = v3.csZ * Q_SCALE;

            if (greenMode)
            {
                v0.r = v1.r = v2.r = v3.r = 0.f;
                v0.g = v1.g = v2.g = v3.g = .2f;
                v0.b = v1.b = v2.b = v3.b = 0.f;
            }
            else
            {
                v0.r = v1.r = (1.f * .2f) + (litCloudColor.r * .8f);
                v2.r = v3.r = (.4f * .2f) + (litCloudColor.r * .8f);

                v0.g = v1.g = (1.f * .2f) + (litCloudColor.g * .8f);
                v2.g = v3.g = (.4f * .2f) + (litCloudColor.g * .8f);

                v0.b = v1.b = (1.f * .2f) + (litCloudColor.b * .8f);
                v2.b = v3.b = (.4f * .2f) + (litCloudColor.b * .8f);
            }

            v0.a = v1.a = v2.a = v3.a = 1.f;

            renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);
            renderer->context.SelectTexture1(rainTexture.TexHandle());
            renderer->DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL,/*(g_nGfxFix > 0)*/ 0);
        }
    }

#endif
}

void RealWeather::DoLightning()
{
    int i, row, col;
    ThreeDVertex v0, v1, v2, v3;
    Tpoint pv, p0, p1, p2, p3, lp, lp0, lp2;
    float dx, dy, dz, adj, yoff, zoff, angle, sR, cR, distance;

    if ( not drawLightning)
    {
        intervalMS = max(((rand() % 30) + 20) * SEC_TO_MSEC, 50 * SEC_TO_MSEC);

        lDist = 0;
        isLightning = FALSE;

        row = rand() % (numCells - 1);
        col = rand() % (numCells - 1);

        lightningPos.x = weatherCellArray[row][col].cloudPosX + weatherShiftX;
        lightningPos.y = weatherCellArray[row][col].cloudPosY + weatherShiftY;
        lightningPos.z = stratusZ;

        dx = lightningPos.x - renderer->viewpoint->X();
        dy = lightningPos.y - renderer->viewpoint->Y();
        dz = (-lightningPos.z) - (-renderer->viewpoint->Z());
        distance = FabsF(SqrtF(dx * dx + dy * dy + dz * dz));

        lZM = -stratusZ / 128.f;
        lRad = distance * .005f;

        drawLightning = TRUE;
        startMS = SimLibElapsedTime;
    }
    else
    {
        if ((SimLibElapsedTime - startMS) <= 300)
        {
            if (-renderer->viewpoint->Z() < -stratusZ)
            {
                if ((SimLibElapsedTime - startMS) <= 100)
                {
                    isLightning = TRUE;
                    TheTimeOfDay.SetScaleFactor(0);
                    TheTimeManager.Refresh();
                }
                else if ((SimLibElapsedTime - startMS) <= 200)
                {
                    isLightning = FALSE;
                    TheTimeOfDay.SetScaleFactor(.99f);
                    TheTimeManager.Refresh();
                }
                else if ((SimLibElapsedTime - startMS) <= 300)
                {
                    isLightning = TRUE;
                    TheTimeOfDay.SetScaleFactor(0);
                    TheTimeManager.Refresh();
                }
            }

            if (isLightning)
            {
                renderer->TransformPointToView(&lightningPos, &pv);

                for (i = 0; i < NUM_LIGHTNING_POINTS - 1; i++)
                {
                    lp0.x = 0.f;
                    lp0.y = -lightningPosList[i][0] * 100.f;
                    lp0.z =  lightningPosList[i][1] * lZM;

                    lp2.x = 0.f;
                    lp2.y = -lightningPosList[i + 1][0] * 100.f;
                    lp2.z =  lightningPosList[i + 1][1] * lZM;

                    adj = (-lp0.z) - (-lp2.z);

                    if (adj)
                    {
                        angle = Atan(0.f, adj);
                        SinCos(angle - PI / 2.f, &sR, &cR);

                        yoff = sR * lRad;
                        zoff = cR * lRad;
                    }
                    else
                        yoff = zoff = 0;

                    p0 = lp0;
                    p2 = lp2;
                    p1 = p0;
                    p3 = p2;

                    p3.y += yoff;
                    p3.z += zoff;
                    p2.y -= yoff;
                    p2.z -= zoff;
                    p1.y -= yoff;
                    p1.z -= zoff;
                    p0.y += yoff;
                    p0.z += zoff;

                    renderer->TransformBillboardPoint(&p0, &pv, &v0);
                    renderer->TransformBillboardPoint(&p1, &pv, &v1);
                    renderer->TransformBillboardPoint(&p2, &pv, &v2);
                    renderer->TransformBillboardPoint(&p3, &pv, &v3);

                    v0.u = TEX_UV_MIN, v0.v = TEX_UV_MIN, v0.q = v0.csZ * Q_SCALE;
                    v1.u = TEX_UV_MAX, v1.v = TEX_UV_MIN, v1.q = v1.csZ * Q_SCALE;
                    v2.u = TEX_UV_MAX, v2.v = TEX_UV_MAX, v2.q = v2.csZ * Q_SCALE;
                    v3.u = TEX_UV_MIN, v3.v = TEX_UV_MAX, v3.q = v3.csZ * Q_SCALE;

                    if (greenMode)
                    {
                        v0.r = v1.r = v2.r = v3.r = 0.f;
                        v0.g = v1.g = v2.g = v3.g = 1.f;
                        v0.b = v1.b = v2.b = v3.b = 0.f;
                    }
                    else
                    {
                        v0.r = v1.r = v2.r = v3.r = 1.f;
                        v0.g = v1.g = v2.g = v3.g = 1.f;
                        v0.b = v1.b = v2.b = v3.b = 1.f;
                    }

                    v0.a = v1.a = v2.a = v3.a = 1.f;

                    if (v0.csZ > 50.f)
                        renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD);
                    else
                        renderer->context.RestoreState(STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE);

                    renderer->context.SelectTexture1(lightningTexture.TexHandle());
                    renderer->DrawSquare(&v0, &v1, &v2, &v3, CULL_ALLOW_ALL, (g_nGfxFix > 0));
                }

                memcpy(&lp, &lightningPos, sizeof(Tpoint));
                lp.z = renderer->viewpoint->GetGroundLevel(lp.x, lp.y);
                renderer->TransformPointToViewSwapped(&lp, &lightningGroundPos);
                lightningGroundPos.x += lp2.y;
            }
        }
        else
        {
            if (isLightning and g_bHearThunder)
            {
                static int uid = 0;
                F4SoundFXSetPos(SFX_THUNDER, TRUE, lightningPos.x, lightningPos.y, lightningPos.z, 1, 0, uid);
                // 7-11-04 version F4SoundFXSetPos(SFX_THUNDER,FALSE,lightningPos.x,lightningPos.y,lightningPos.z,1,0,0,0,0,uid,0);
                uid++;

                if (uid > 99)
                    uid = 0;

                TheTimeOfDay.SetScaleFactor(.99f);
                TheTimeManager.Refresh();
            }

            isLightning = FALSE;
        }

        if ((SimLibElapsedTime - startMS) > intervalMS)
        {
            drawLightning = FALSE;
        }
    }
}

void RealWeather::SetupTexturesOnDevice(DXContext *rc)
{
    char filename[_MAX_PATH];

    rainTexture.LoadAndCreate("RAIN.DDS", MPR_TI_DDS);
    overcastTexture.LoadAndCreate("CUMULUOV.DDS", MPR_TI_DDS);
    lightningTexture.LoadAndCreate("LIGHTNGB.DDS", MPR_TI_DDS);

    sprintf(filename, "CirrusCum.dds");
    CirrusCumTextures.LoadAndCreate(filename, MPR_TI_DDS);

    sprintf(filename, "Cumulus.dds");
    CumulusTextures.LoadAndCreate(filename, MPR_TI_DDS);

    TimeUpdateCallback(NULL);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, NULL);
}

void RealWeather::ReleaseTexturesOnDevice(DXContext *rc)
{

    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, NULL);

    rainTexture.FreeAll();
    overcastTexture.FreeAll();
    lightningTexture.FreeAll();

    CirrusCumTextures.FreeAll();

    CumulusTextures.FreeAll();
}

void RealWeather::Cleanup()
{
    int i;

    if ( not DisplayOptions.bZBuffering)
    {
        for (i = 0; i < MAX_NUM_DRAWABLES; i++)
        {
            if (real2DClouds)
                real2DClouds[i].Cleanup();

            if (real3DClouds)
                real3DClouds[i].Cleanup();
        }

        if (real2DClouds)
        {
            delete[] real2DClouds;
            real2DClouds = NULL;
        }

        if (real3DClouds)
        {
            delete[] real3DClouds;
            real3DClouds = NULL;
        }
    }

    bSetup = FALSE;
}

void RealWeather::SetGreenMode(BOOL state)
{
    greenMode = state;
    Drawable3DCloud::SetGreenMode(greenMode);
    Drawable2DCloud::SetGreenMode(greenMode);
#ifdef MLR_NEWTRAILCODE
    DrawableTrail::SetGreenMode(greenMode);
#endif
}

void RealWeather::UpdateWeatherQuality(void)
{
    WeatherQualityStep = F_I32(PRANDFloatPos() * MAX_WEATHER_Q_STEPS);
    WeatherQualityRate = PRANDFloat() * MIN_WEATHER_Q_STEP;

    if (fabs(WeatherQualityRate) < (MIN_WEATHER_Q_STEP / 10.0f)) realWeather->WeatherQualityRate = fabs(WeatherQualityRate) / WeatherQualityRate * MIN_WEATHER_Q_STEP;
}

void RealWeather::TimeUpdateCallback(void *)
{
    realWeather->UpdateLighting();

}
//Cobra
bool RealWeather::ReadWeather(void)
{

    FILE* fp;
    int i = 0;
    int cnt = 0;
    char file[1024];
    extern char FalconTerrainDataDir[];
    //char tmpChar[10];
    int tmp = 0;
    //char netFile[1024];
    //URLDownloadToFile(NULL, "http://weather.flightgear.org/~curt/WX/METAR.rwx", "c:\\metar.txt", 0, NULL);
    //int testthis = URLDownloadToFile(NULL, "http://www.microsoft.com/ms.htm", "c:\\ms.htm", 0, 0);

    sprintf(file, "C:\\metar.txt");

    if ( not (fp = fopen(file, "rt")))
        return FALSE;

    //Count
    int t = 0;
    int cntr = 0;
    char specMETAR[1024];

    while (fgets(file, 1024, fp))
    {
        if (file[0] == '\r' or file[0] == '#' or file[0] == ';' or file[0] == '\n')
        {
            t++;
            continue;
        }
        else
        {
            if (strncmp(file, "BIEG", 4) == 0)
            {
                strcpy(specMETAR, file);
            }
            else if (strncmp(file, "RKSS", 10) == 0)
            {
                strcpy(specMETAR, file);
            }

            continue;
        }
    }

    //Now we have a match.  Time to parse it.
    int tm = 0;
    int time = 0;
    int tempa = 0;
    int wndDir = 0;
    int wndSpd = 0;
    int vis = 0;
    int altim = 0;
    char cpy[10] = "";
    char cpy1[10] = "";
    char layer[5] = "";
    int cntLyr = 0;

    //char cpy2[10] = "";
    //KSTS 300653Z AUTO 10004KT 10SM CLR 10/08 A3014 RMK AO2 SLP200      T01000078 TSNO
    //METAR RKSS 300700Z 33006KT 5000 HZ FEW030 29/19 Q1002 NOSIG
    //KJKL 300705Z AUTO 00000KT 1 1/2SM +RA BR SCT003 OVC017 14/13 A2973
    if (specMETAR)
    {
        char * pch;

        pch = strtok(specMETAR, " ");

        while (pch not_eq NULL)
        {
            //Identify things by length
            if (strlen(pch) == 2 or (strlen(pch) == 3 and strncmp(pch, "-", 1))
                or (strlen(pch) == 3 and strncmp(pch, "+", 1)))
            {
                if (strlen(pch) == 3) //strip it down to two
                {
                    strncpy(cpy, pch + 1, 2);
                }
                else
                {
                    strcpy(cpy, pch);
                }

                if (strcmp(cpy, "RA") == 0 or strcmp(cpy, "DZ") == 0 or strcmp(cpy, "SN") == 0 or
                    strcmp(cpy, "GR") == 0 or strcmp(cpy, "GS") == 0 or strcmp(cpy, "PL") == 0 or
                    strcmp(cpy, "SG") == 0 or strcmp(cpy, "IC") == 0 or strcmp(cpy, "UP") == 0) //0/8
                {
                    //Some form of precip present
                    tm = 1;
                    memset(&cpy, 0, sizeof(cpy));
                }

                if (strcmp(cpy, "FG") == 0 or strcmp(cpy, "HZ") == 0 or strcmp(cpy, "FU") == 0 or
                    strcmp(cpy, "PY") == 0 or strcmp(cpy, "BR") == 0 or strcmp(cpy, "SA") == 0 or
                    strcmp(cpy, "DU") == 0 or strcmp(cpy, "VA") == 0) //0/8
                {
                    //Some form of visibility obstruction
                    tm = 1;
                    memset(&cpy, 0, sizeof(cpy));
                }

            }
            else if (strlen(pch) == 3)
            {
                //
                if (strcmp(pch, "CLR") == 0 or strcmp(pch, "NSC") == 0 or strcmp(pch, "SKC") == 0) //0/8
                {
                    tm = 1;
                }
                else if (strcmp(pch, "FEW") == 0) //0 - 2/8
                {
                    tm = 1;
                }
                else if (strcmp(pch, "SCT") == 0) //3/8-4/8
                {
                    tm = 1;
                }
                else if (strcmp(pch, "BKN") == 0) //5/8-7/8
                {
                    tm = 1;
                }
                else if (strcmp(pch, "OVC") == 0) //8/8
                {
                    tm = 1;
                }
                else if (strcmp(pch, "TCU") == 0) //Towering Cumulus
                {
                    tm = 1;
                }
            }
            else if (strlen(pch) == 4 or (strlen(pch) == 5 and strncmp(pch, "-", 1) == 0)
                     or (strlen(pch) == 5 and strncmp(pch, "+", 1) == 0))
            {
                if (strlen(pch) == 5) //strip it down to four
                {
                    strncpy(cpy, pch + 1, 4);
                }
                else if (strlen(pch) == 4)
                {
                    strcpy(cpy, pch);
                }


                if (strncmp(pch + 2, "SM", 2) == 0)
                {
                    strncpy(cpy, pch, 2);
                    vis = atoi(cpy);
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strcmp(cpy, "VCSH") == 0)
                {
                    //Showers
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strcmp(cpy, "TSRA") == 0)
                {
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strcmp(cpy, "9999") == 0)
                {
                    //Clear vis
                    memset(&cpy, 0, sizeof(cpy));
                }

            }
            else if (strlen(pch) == 5)
            {
                //
                if (strncmp(pch + 2, "/", 1) == 0)
                {
                    strncpy(cpy, pch, 2);
                    tempa = atoi(cpy);
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strncmp(pch, "A", 1) == 0)
                {
                    strncpy(cpy, pch + 1, 4);
                    altim = atoi(cpy);
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strncmp(pch, "Q", 1) == 0)
                {
                    strncpy(cpy, pch + 1, 4);
                    altim = atoi(cpy);
                    altim = FloatToInt32(altim * 0.02953f);
                    memset(&cpy, 0, sizeof(cpy));
                }
                /*Provided the visibility is >=10 km, AND the height of the lowest cloud (any amount) is >=5000 ft (or highest minimum sector altitude) AND there are no cumulonimbus clouds (CB, any height) within sight AND there is no significant weather (see list below), then the visibility and cloud part of the standard METAR is replaced by CAVOK (say "cav-oh-kay": 'Ceiling And Visibility OK'). (not used by certain countries, e.g. the United States)*/
                else if (strcmp(pch, "CAVOK") == 0) //Basically CLR
                {
                    tm = 1;
                }


            }
            else if (strlen(pch) == 6 or (strlen(pch) == 8 and (strncmp(pch + 6, "CB", 2) == 0))
                     or (strlen(pch) == 9 and (strncmp(pch + 6, "TCU", 3) == 0)))
            {
                //XYZ000
                if (strncmp(pch + 6, "CB", 2) == 0)
                {
                    //Set CB clouds
                    tm = 1;
                }
                else if (strncmp(pch + 7, "TCU", 3) == 0)
                {
                    //Set TCU
                    tm = 1;
                }

                if (strncmp(pch, "FEW", 3) == 0) //0 - 2/8
                {
                    tm = 1;
                    strncpy(cpy, pch + 3, 3);
                    cntLyr = atoi(cpy) * 100;
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strncmp(pch, "SCT", 3) == 0) //3/8-4/8
                {
                    tm = 2;
                    strncpy(cpy, pch + 3, 3);
                    cntLyr = atoi(cpy) * 100;
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strncmp(pch, "BKN", 3) == 0) //5/8-7/8
                {
                    tm = 3;
                    strncpy(cpy, pch + 3, 3);
                    cntLyr = atoi(cpy) * 100;
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strncmp(pch, "OVC", 3) == 0) //8/8
                {
                    tm = 4;
                    strncpy(cpy, pch + 3, 3);
                    cntLyr = atoi(cpy) * 100;
                    memset(&cpy, 0, sizeof(cpy));
                }

            }
            else if (strlen(pch) == 7 or strlen(pch) == 10)
            {
                //

                //Grab Time in ZULU
                if (strcmp(pch + 6, "Z") == 0)
                {
                    strncpy(cpy, pch + 2, 4);
                    time = atoi(cpy);
                    memset(&cpy, 0, sizeof(cpy));
                }
                else if (strcmp(pch + 5, "KT") == 0) //Grab wind direction and speed
                {
                    if (strncmp(pch, "VRB", 3) == 0) //catch variable wind
                    {
                        wndDir = 10;
                    }
                    else
                    {
                        strncpy(cpy1, pch, 3);
                        wndDir = atoi(cpy1);
                        memset(&cpy1, 0, sizeof(cpy1));
                    }

                    strncpy(cpy1, pch + 3, 2);
                    wndSpd = atoi(cpy1);
                    memset(&cpy1, 0, sizeof(cpy1));
                }
                else if (strcmp(pch + 8, "KT") == 0)
                {
                    strncpy(cpy1, pch, 3);
                    wndDir = atoi(cpy1);
                    memset(&cpy1, 0, sizeof(cpy1));
                    strncpy(cpy1, pch + 3, 2);
                    wndSpd = atoi(cpy1);
                    memset(&cpy1, 0, sizeof(cpy1));
                    //Add variable for gusts
                }

            }
            else if (strlen(pch) == 9)
            {
                if (strncmp(pch + 7, "TCU", 3) == 0)
                {
                    //We have TCU
                    tm = 1;
                }

            }

            MonoPrint("%s\n", pch);
            pch = strtok(NULL, " ,.");
        }
    }





    //This reads a file from weather folder (manual weather)
    /*sprintf(file,"%s\\weather\\RKSS.txt",FalconTerrainDataDir);

    if( not (fp=fopen(file,"rt")))
     {
     metar = NULL;
     return FALSE;
     }
    numMETARS = atoi(fgets(file,1024,fp));
    metar = new METAR[numMETARS];

    while (i<numMETARS)
    {
     fgets(file,1024,fp);
     if (file[0] == '\r' or file[0] == '#' or file[0] == ';' or file[0] == '\n')
     continue;

     switch (cnt)
     {
     case 0:
     //Station
     strcpy(tmpChar,file);
     strcpy(metar[i].station,tmpChar);
     break;
     case 1:
     //Time
     sscanf(file,"%6d",&tmp);
     metar[i].time = tmp;
     break;
     case 2:
     //#Wind Direction
     sscanf(file,"%3d",&tmp);
     metar[i].windDirection = tmp;
     break;
     case 3:
     //#Wind Speed (knots)
     sscanf(file,"%4d",&tmp);
     metar[i].windSpeed = tmp;
     break;
     case 4:
     //#Visibility (miles) (99 = Unlimited/Clr)
     sscanf(file,"%3d",&tmp);
     metar[i].visibility = tmp;
     break;
     case 5:
     //#Sky Coverage 1
     strcpy(tmpChar,file);
     strcpy(metar[i].skyCoverage1,tmpChar);
     break;
     case 6:
     //#Coverage Altitude (feet) 1
     sscanf(file,"%6d",&tmp);
     metar[i].skyCoverageAlt1 = tmp;
     break;
     case 7:
     //#sky Coverage 2 (FEW, SCT, BKN, OVC, CLR)
     strcpy(tmpChar,file);
     strcpy(metar[i].skyCoverage2,tmpChar);
     break;
     case 8:
     //#Coverage Altitude (feet) 2
     sscanf(file,"%6d",&tmp);
     metar[i].skyCoverageAlt2 = tmp;
     break;
     case 9:
     //#Weather Type (RA, TS, NONE)
     strcpy(tmpChar,file);
     strcpy(metar[i].weatherType,tmpChar);
     break;
     case 10:
     //#Altimeter
     sscanf(file,"%4d",&tmp);
     metar[i].altimeter = tmp;
     break;
     default:
     break;
     }
     cnt++;
     if (cnt == 11)
     {
     cnt = 0;
     i++;
     }
    }
    fclose(fp);*/

    return false;
}
