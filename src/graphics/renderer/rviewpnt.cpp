/***************************************************************************\
    RViewPnt.cpp
    Scott Randolph
    August 20, 1996

    Manages information about a world space location and keeps the weather,
 terrain, and object lists in synch.
\***************************************************************************/
#include "grmath.h"
#include "TimeMgr.h"
#include "TOD.h"
#include "DrawOvc.h"
#include "RViewPnt.h"
#include "context.h"
#include "FalcLib/include/dispopts.h" //JAM 04Oct03

//JAM 18Nov03
#include "RealWeather.h"

//#define _OLD_UPDATE_ // use scotts Update() funtion

/***************************************************************************\
 Setup the view point
\***************************************************************************/
void RViewPoint::Setup(float gndRange, int maxDetail, int minDetail, bool isZBuffer)
{
    int i;

    bZBuffering = isZBuffer; //JAM 13Dec03

    ShiAssert( not IsReady());

    // Initialize our sun and moon textures
    SetupTextures();

    // Determine how many object lists we'll need
    nObjectLists = _NUM_OBJECT_LISTS_; // 0=in terrain, 1=below cloud, 2=in cloud, 3=above clouds, 4= above roof

    // Allocate memory for the list of altitude segregated object lists

    objectLists = new ObjectListRecord[nObjectLists];
    ShiAssert(objectLists);

    // Initialize each display list -- update will set the top and base values
    for (i = 0; i < nObjectLists; i++)
    {
        objectLists[i].displayList.Setup();
        objectLists[i].Ztop  = -1e13f;
    }

    // Initialize the cloud display list
    cloudList.Setup();

    // Setup our base class's terrain information
    float *ranges = new float[minDetail + 1];

    for (i = minDetail; i >= 0; i--)
    {
        // Store this detail levels active range
        if (i == minDetail)
        {
            // Account for only drawing out to .707 of the lowest detail level
            ranges[i] = gndRange * 1.414f;
        }
        else
        {
            ranges[i] = gndRange;
        }

        // Make each cover half the distance of the one before
        gndRange /= 2.0f;
    }

    TViewPoint::Setup(maxDetail, minDetail, ranges);
    delete[] ranges;

    //JAM 12Dec03
    cloudBase = -(SKY_ROOF_HEIGHT - 0.1f);
    cloudTops = -SKY_ROOF_HEIGHT;
    roofHeight = -SKY_ROOF_HEIGHT;

    if (bZBuffering)
        realWeather->Setup();
    else
        realWeather->Setup(ObjectsBelowClouds(), Clouds());
}


/***************************************************************************\
 Clean up after ourselves
\***************************************************************************/
void RViewPoint::Cleanup(void)
{
    int i;

    ShiAssert(IsReady());

    // Release our sun and moon textures
    ReleaseTextures();

    // Cleanup all the display lists
    for (i = 0; i < nObjectLists; i++)
        objectLists[i].displayList.Cleanup();

    delete[] objectLists;
    objectLists = NULL;

    // Cleanup our bases class's terrain manager
    TViewPoint::Cleanup();
}



/***************************************************************************\
 Change the visible range and detail levels of the terrain
 NOTE:  This shouldn't be called until an update has been done so
 that the position is valid.
\***************************************************************************/
void RViewPoint::SetGroundRange(float gndRange, int maxDetail, int minDetail)
{
    TViewPoint tempViewPoint;
    int i;

    ShiAssert(IsReady());

    // Calculate the ranges we'll need at each LOD
    float *Ranges = new float[minDetail + 1];
    ShiAssert(Ranges);

    for (i = minDetail; i >= 0; i--)
    {
        Ranges[i] = gndRange;
        gndRange /= 2.0f;
    }

    delete[] Ranges;

    // Construct a temporary viewpoint with our new parameters and current position
    tempViewPoint.Setup(maxDetail, minDetail, Ranges);
    tempViewPoint.Update(&pos);

    // Reset and update our base class's terrain information
    TViewPoint::Cleanup();
    TViewPoint::Setup(maxDetail, minDetail, Ranges);
    TViewPoint::Update(&pos);

    // Cleanup the temporary viewpoint
    tempViewPoint.Cleanup();
}

//JAM 12Dec03
float defcloudop = 0.0;
/***************************************************************************\
 Update the state of the RViewPoint (time and location)
\***************************************************************************/
void RViewPoint::Update(const Tpoint *pos)
{
    int i;
    float previousTop;
    TransportStr transList;
    DrawableObject *first, *p;

    ShiAssert(IsReady());

    // Update the terrain center of attention
    TViewPoint::Update(pos);
    TViewPoint::GetAreaFloorAndCeiling(&terrainFloor, &terrainCeiling);

    roofHeight = -SKY_ROOF_HEIGHT;

    // Update the ceiling values of the object display lists
    objectLists[0].Ztop = terrainCeiling;
    objectLists[1].Ztop = realWeather->stratusZ + ((realWeather->stratusDepth) / 4.f);
    objectLists[2].Ztop = realWeather->stratusZ - ((realWeather->stratusDepth) / 4.f);
    objectLists[3].Ztop = roofHeight;

    previousTop = 1e12f;

    for (i = 0; i < nObjectLists; i++)
    {
        transList.list[i] = NULL;
        transList.top[i] = objectLists[i].Ztop;
        transList.bottom[i] = previousTop;
        previousTop = transList.top[i];
    }

    transList.top[nObjectLists - 1] = -1e12f;

    // Update the membership of the altitude segregated object lists
    previousTop = 1e12f;

    for (i = 0; i < nObjectLists; i++)
    {
        objectLists[i].displayList.UpdateMetrics(i, pos, &transList);
    }

    // Put any objects moved into their new list
    for (i = 0; i < nObjectLists; i++)
    {
        first = transList.list[i];

        while (first)
        {
            p = first;
            first = first->next;
            objectLists[i].displayList.InsertObject(p);
        }
    }

    //JAM 265Dec03
    if ( not bZBuffering)
    {
        for (i = 0; i < nObjectLists; i++)
            objectLists[i].displayList.SortForViewpoint();

        cloudList.UpdateMetrics(pos);
        cloudList.SortForViewpoint();
    }

    //DELME
    // cloudOpacity = defcloudop;

    TheTimeOfDay.GetVisColor(&cloudColor);
}
//JAM

/***************************************************************************\
 Insert an instance of an object into the active display lists
\***************************************************************************/
void RViewPoint::InsertObject(DrawableObject* object)
{
    int i;

    ShiAssert(object);

    if ( not object) // JB 010710 CTD?
        return;

    // Decide into which list to put the object
    for (i = 0; i < nObjectLists; i++)
    {
        if (object->position.z >= objectLists[i].Ztop)
        {
            objectLists[i].displayList.InsertObject(object);
            return;
        }
    }

    // We could only get here if the object was higher than the highest level
    ShiWarning("Object with way to much altitude");
}


/***************************************************************************\
 Remove an instance of an object from the active display lists
\***************************************************************************/
void RViewPoint::RemoveObject(DrawableObject* object)
{
    ShiAssert(object);
    ShiAssert(object->parentList);

    // Take the given object out of its parent list
    object->parentList->RemoveObject(object);
}


/***************************************************************************\
 Reset all the object lists to start with their most distance objects.
\***************************************************************************/
void RViewPoint::ResetObjectTraversal(void)
{
    ShiAssert(IsReady());

    for (int i = 0; i < nObjectLists; i++)
        objectLists[i].displayList.ResetTraversal();

    cloudList.ResetTraversal();
}


/***************************************************************************\
 Return the index of the object list which contains object at the given
 z value.
\***************************************************************************/
int RViewPoint::GetContainingList(float zValue)
{
    ShiAssert(IsReady());

    int i;

    for (i = 0; i < nObjectLists; i++)
        if (zValue > objectLists[i].Ztop)
            return i;

    // Can only get here if we got a really huge z value
    ShiWarning("Z way too big");
    return -1;
}


/***************************************************************************\
 Return TRUE if a line of sight exists between the two points with
 respect to both clouds and terrain
\***************************************************************************/
float RViewPoint::CompositLineOfSight(Tpoint *p1, Tpoint *p2)
{
    // if (LineOfSight( p1, p2 )) {
    // if (weather) {
    // return weather->LineOfSight( p1 ,p2 );
    // } else {
    return 1.0f;
    // }
    // } else {
    // return 0.0f;
    // }
}

/***************************************************************************\
 Return TRUE if a line of sight exists between the two points with
 respect to clouds
\***************************************************************************/
int RViewPoint::CloudLineOfSight(Tpoint *p1, Tpoint *p2)
{
    // if (weather) {
    // return FloatToInt32(weather->LineOfSight( p1 ,p2 ));
    // } else {
    return 1;
    // }
}


/***************************************************************************\
 Update the moon texture for the time of day and altitude.
\***************************************************************************/
void RViewPoint::UpdateMoon()
{
    if ( not TheTimeOfDay.ThereIsAMoon())
    {
        lastDay = 1;
        return;
    }

    if ( not lastDay) return;

    lastDay = 0; // do it only once when the moon appear

    TheTimeOfDay.CalculateMoonPhase();

    // Edit the image data for the texture to darken a portion of the moon
    TheTimeOfDay.CreateMoonPhase((unsigned char *)OriginalMoonTexture.imageData, (unsigned char *)MoonTexture.imageData);

    // Create the green moon texture based on the color version
    BYTE *texel = (BYTE*)MoonTexture.imageData;
    BYTE *dest = (BYTE*)GreenMoonTexture.imageData;
    BYTE *stopTexel = (BYTE*)MoonTexture.imageData + MoonTexture.dimensions * MoonTexture.dimensions;

    while (texel < stopTexel)
    {
        if (*texel not_eq 0)   // Don't touch the chromakeyed texels
        {
            *dest++ = (BYTE)((*texel++) bitor 128); // Use the "green" set of palette entries
        }
        else
        {
            texel++;
            *dest++ = 0;
        }
    }

    MoonTexture.UpdateMPR();
    GreenMoonTexture.UpdateMPR();
}

/***************************************************************************\
    Setup the celestial textures (sun and moon)
\***************************************************************************/
void RViewPoint::SetupTextures()
{
    DWORD *p, *stop;


    // Note that we haven't adapted for a particular day yet
    lastDay = -1;

    // Build the normal sun texture

    //JAM 04Oct03
    if (DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
        SunTexture.LoadAndCreate("sun.dds", MPR_TI_DDS);
    else
    {
        // Build the normal sun texture
        SunTexture.LoadAndCreate("sun5.apl", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE);
        SunTexture.FreeImage();

        // Now load the image to construct the green sun texture
        // (Could do without this, but this is easy and done only once...)
        if ( not GreenSunTexture.LoadImage("sun5.apl", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE))
        {
            ShiError("Failed to load sun texture(2)");
        }

        // Convert palette data to Green
        Palette *pal = GreenSunTexture.GetPalette();
        p = pal->paletteData;
        stop = p + 256;

        while (p < stop)
        {
            *p and_eq 0xFF00FF00;
            p++;
        }

        pal->UpdateMPR();

        // Convert chroma color to Green
        GreenSunTexture.chromaKey and_eq 0xFF00FF00;

        // Send the texture to MPR
        GreenSunTexture.CreateTexture();
        GreenSunTexture.FreeImage();
    }

    //JAM

    // Now setup the moon textures.  (We'll tweak them periodicaly in BuildMoon)
    OriginalMoonTexture.LoadAndCreate("moon.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE bitor MPR_TI_ALPHA);
    MoonTexture.LoadAndCreate("moon.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE bitor MPR_TI_ALPHA);
    GreenMoonTexture.SetPalette(MoonTexture.GetPalette());
    GreenMoonTexture.LoadAndCreate("moon.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE bitor MPR_TI_ALPHA);

    // build white moon with alpha
    Palette *moonPal = MoonTexture.GetPalette();
    moonPal->paletteData[0] = 0;
    unsigned int color;
    DWORD *src;
    DWORD *dst = &moonPal->paletteData[1];
    DWORD *end = &moonPal->paletteData[48];

    while (dst < end)
    {
        int a = *dst bitand 0xff;
        *dst++ = (a << 24) + 0xffffff;
    }

#ifdef USE_TRANSPARENT_MOON
    src = &MoonTexture.palette->paletteData[1];
    dst = &MoonTexture.palette->paletteData[1 + 128];
    end = &MoonTexture.palette->paletteData[128 + 48];
#else
    src = &moonPal->paletteData[1];
    dst = &moonPal->paletteData[1 + 48];
    end = &moonPal->paletteData[48 + 48];

    while (dst < end)
    {
        color = *src++;
        color = ((color >> 3) bitand 0xff000000) + 0xffffff;
        *dst++ = color;
    }

    src = &moonPal->paletteData[1];
    dst = &moonPal->paletteData[1 + 128];
    end = &moonPal->paletteData[128 + 48 + 48];
#endif

    // Setup the green portion of the moon palette
    while (dst < end)
    {
        color = *src++;
        *dst++ = (color bitand 0xff000000) + 0x00ff00;
    }

    // Update the moon palette
    moonPal->UpdateMPR();



    // Request time updates
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);
}


/***************************************************************************\
    Release the celestial textures (sun and moon)
\***************************************************************************/
void RViewPoint::ReleaseTextures(void)
{
    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);

    // Free our texture resources

    //JAM 04Oct03
    if (DisplayOptions.m_texMode not_eq DisplayOptionsClass::TEX_MODE_DDS)
        GreenSunTexture.FreeAll();

    //JAM

    OriginalMoonTexture.FreeAll();
    MoonTexture.FreeAll();
    GreenMoonTexture.FreeAll();
}


/***************************************************************************\
 Update the moon and sun properties based on the time of day
\***************************************************************************/
void RViewPoint::TimeUpdateCallback(void *self)
{
    ((RViewPoint*)self)->UpdateMoon();
}
