///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdhdr.h"
#include "dispcfg.h"
#include "Graphics/Include/Setup.h"
#include "Graphics/Include/RenderOW.h"
#include "otwdrive.h"
#include "Graphics/Include/image.h"
#include "Graphics/Include/device.h"
#include "RedMacros.h"

// RV - Biker - Theater switching stuff
extern char FalconSplashThrDirectory[];
int lastframe;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Properties of the animation image palette layout
static const int NUM_SPLASH_FRAMES = 5;
static const int PAL_FRAME_LENGTH = 32;

// location and dimensions of the radar thing (which we want to cover up)
static const int COVER_TOP = 246;
static const int COVER_LEFT = 159;

// Pointers to the global resources used while the loading screen is up
BYTE *originalImage = NULL;
unsigned long *originalPalette = NULL;
int origImageType = IMAGE_TYPE_UNKNOWN;

// Some data about the source image
static int originalWidth = 0;
static int originalHeight = 0;
static int coverWidth = 0;
static int coverHeight = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


/*
** Called to start showing splash screen
** This loads the loading screen data and show the initial frame
*/

void OTWDriverClass::SetupSplashScreen(void)
{
    int result;
    CImageFileMemory  texFile;
    char filename[MAX_PATH];
    FILE* tmpFile;


    lastframe = -1;

    // Make sure we recognize this file type
    //THW 2003-11-15 Allow 1600x1200 and 1280x1024 load screens
    if (OTWImage->targetXres() >= 1600)
    {
        // RV - Biker - Theater switching for splash files
        //filename = "art\\splash\\load16.gif";
        sprintf(filename, "%s\\%s", FalconSplashThrDirectory, "load16.gif");
        tmpFile = fopen(filename, "r");

        //Check if file does exist
        if (tmpFile)
        {
            fclose(tmpFile);
        }
        //Load theater soecific splash with 1024x768
        else
        {
            // RV - Biker
            //filename = "art\\splash\\load10.gif";
            sprintf(filename, "%s\\%s", FalconSplashThrDirectory, "load10.gif");
        }
    }
    else if (OTWImage->targetXres() >= 1280)
    {
        // RV - Biker
        //filename = "art\\splash\\load12.gif";
        sprintf(filename, "%s\\%s", FalconSplashThrDirectory, "load12.gif");
        tmpFile = fopen(filename, "r");

        if (tmpFile)
        {
            fclose(tmpFile);
        }
        else
        {
            // RV - Biker
            //filename = "art\\splash\\load10.gif";
            sprintf(filename, "%s\\%s", FalconSplashThrDirectory, "load10.gif");
        }
    }
    else if (OTWImage->targetXres() >= 1024)
    {
        // RV - Biker
        //filename = "art\\splash\\load10.gif";
        sprintf(filename, "%s\\%s", FalconSplashThrDirectory, "load10.gif");
    }
    // RV - Biker for such low res we don't do theater specific splash files
    else if (OTWImage->targetXres() >= 800)
    {
        //filename = "art\\splash\\load8.gif";
        sprintf(filename, "art\\splash\\load8.gif");
    }
    else
    {
        //filename = "art\\splash\\load6.gif";
        sprintf(filename, "art\\splash\\load6.gif");
    }

    // RV - Biker - Try to open file
    tmpFile = fopen(filename, "r");

    if (tmpFile)
    {
        fclose(tmpFile);
    }
    else
    {
        sprintf(filename, "art\\splash\\load10.gif");
    }

    texFile.imageType = CheckImageType(filename);

    if (texFile.imageType < 0)
    {
        ShiWarning("We failed to read a splash screen image");
        return;
    }

    // Open the input file
    result = texFile.glOpenFileMem(filename);

    if (result not_eq 1)
    {
        ShiWarning("We failed to read a splash screen image");
        return;
    }

    // Read the image data (note that ReadTextureImage will close texFile for us)
    texFile.glReadFileMem();
    result = ReadTextureImage(&texFile);

    if (result not_eq GOOD_READ)
    {
        ShiWarning("We failed to read a splash screen image");
        return;
    }

    // Store the image data
    originalWidth = texFile.image.width;
    originalHeight = texFile.image.height;
    originalImage = texFile.image.image;
    originalPalette = texFile.image.palette;
    origImageType = texFile.imageType;//XX
    ShiAssert(originalImage);
    ShiAssert(originalPalette);

    // Give a little delay, before next step, that will be Splash 0
    Sleep(1000);

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
** Called to finish showing splash screen
** This frees the loading screen data
*/

void OTWDriverClass::CleanupSplashScreen(void)
{
    // Release the original image data
    glReleaseMemory((char*)originalImage);
    glReleaseMemory((char*)originalPalette);
    originalImage = NULL;
    originalPalette = NULL;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
** This function is called periodically during startup to update the
** animation on the load screen
*/

void OTWDriverClass::SplashScreenUpdate(int frame)
{
    BYTE *imagePtr = NULL;
    void *buffer = NULL;
    int x, y;
    unsigned long tweakedPalette[256];
    unsigned long *srcPal = NULL, *dstPal = NULL;
    unsigned long *startLit = NULL, *stopLit = NULL, *startInvar = NULL, *stop = NULL;


    // RED - Consistency check
    if ( not OTWImage) return;

    // Validate our parameter
    if (frame == lastframe) // kill "go back a frame" bug
        return;

    if (frame == 3) // kill "go back a frame" bug
        lastframe = frame;

    //lastframe = frame;
    if (frame >= NUM_SPLASH_FRAMES)
    {
        ShiWarning("Bad frame requested for the splash screen");
        return;
    }

    if ( not originalImage)
    {
        return;
    }

    // RED - Allocating a buffer to work into, instead of Image Buffer
    // The buffer will be same size of the Original Image with 32bit Pixel Size
    DWORD *WorkBuffer = (DWORD*)malloc(originalHeight * originalWidth * sizeof(DWORD));

    // Clear the back buffer to black to erase anything that might already be there
    renderer->context.StartFrame();
    renderer->SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
    renderer->StartDraw();
    renderer->SetBackground(0xFF000000);
    renderer->ClearDraw();
    renderer->context.UpdateSpecularFog(0xff000000);


    // Darken all but the specified frame in the palette
    srcPal = originalPalette;
    dstPal = tweakedPalette;
    startLit = originalPalette + frame * PAL_FRAME_LENGTH;
    stopLit = startLit + PAL_FRAME_LENGTH;
    startInvar = originalPalette + NUM_SPLASH_FRAMES * PAL_FRAME_LENGTH;
    stop = originalPalette + 256;

    ShiAssert(srcPal <= startLit);
    ShiAssert(startLit <= stopLit);
    ShiAssert(stopLit <= startInvar);
    ShiAssert(startInvar <= stop);

    // Divide the dimmed color intensities by 4 (knock them down 2 bits in each channel)
    while (srcPal < startLit) *dstPal++ = (*srcPal++ bitand 0x00FCFCFC) >> 1;

    // Copy the "lit" color intensities
    while (srcPal < stopLit) *dstPal++ = *srcPal++;

    // Divide the dimmed color intensities by 4 (knock them down 2 bits in each channel)
    while (srcPal < startInvar) *dstPal++ = (*srcPal++ bitand 0x00FCFCFC) >> 1;

    // Copy the invariant high portion of the palette
    while (srcPal < stop) *dstPal++ = *srcPal++;

    // Point to the originalImage
    imagePtr = originalImage;

    DWORD *pixel;

    // Now, build image based on tweaked palette
    for (y = 0; y < originalHeight; y ++)
    {
        pixel = &WorkBuffer[ y * originalWidth ];

        for (x = 0; x < originalWidth and imagePtr ; x ++)
        {
            *pixel = tweakedPalette[*imagePtr];
            pixel ++;
            imagePtr ++;
        }
    }

    // Go, render it requesting a Fit To Screen
    renderer->Render2DBitmap(0, 0, 0, 0, originalWidth, originalHeight, originalWidth, WorkBuffer, true);
    renderer->EndDraw();
    renderer->context.FinishFrame(NULL);

    // flip the surface
    OTWImage->SwapBuffers(NULL);

    free(WorkBuffer);
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void OTWDriverClass::ShowSimpleWaitScreen(char *name)
{
    //int top, left;
    int width, height;
    char filename[MAX_PATH];
    CImageFileMemory  texFile; //THW 2003-11-15 Allow 1600x1200 and 1280x1024 exit screens
    FILE* tmpFile;

    // RV - RED - Rewritten with Image Scaling
    // Always look for 1600 x 1200 Image
    //sprintf (filename, "art\\splash\\%s16.gif", name);
    sprintf(filename, "%s\\%s16.gif", FalconSplashThrDirectory, name);

    // RV - Biker - Try to open file
    tmpFile = fopen(filename, "r");

    if (tmpFile)
    {
        fclose(tmpFile);
    }
    else
    {
        sprintf(filename, "art\\splash\\%s16.gif", name);
    }

    width = 1600;
    height = 1200;

    // Clear the back buffer to black to erase anything that might already be there
    renderer->context.StartFrame();
    renderer->SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
    renderer->StartDraw();
    renderer->SetBackground(0xFF000000);
    renderer->ClearDraw();
    // Go, render it requesting a Fit To Screen
    renderer->Render2DBitmap(0, 0, 0, 0, width, height, filename, true);
    renderer->EndDraw();

    renderer->context.FinishFrame(NULL);
    OTWImage->SwapBuffers(NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
