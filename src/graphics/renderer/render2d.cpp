/***************************************************************************\
    Render2D.cpp
    Scott Randolph
    December 29, 1995

    This class provides 2D drawing functions.
\***************************************************************************/
#include <cISO646>
#include <math.h>
#include "falclib/include/debuggr.h"
#include "Image.h"
#include "Device.h"
#include "Render2D.h"
#include "GraphicsRes.h"
#include "Tex.h"
#include "GraphicsRes.h"
#include "falclib/include/dispopts.h" //Wombat778 12-12-2003

//#ifdef USE_TEXTURE_FONT
//Texture Font2D.fontTexture[NUM_FONT_RESOLUTIONS];
//#endif

extern bool g_bAutoScaleFonts; //Wombat778 12-12-2003
extern bool g_bOldFontTexelFix; //Wombat777 4-01-04

int FindBestResolution(); //Wombat778 4-03-04

//Texture Render2D::Font.FontTexture[NUM_FONT_RESOLUTIONS]; //JAM 22Dec03


Render2D::Render2D()
{
}


Render2D::~Render2D()
{
}


ImageBuffer* Render2D::GetImageBuffer()
{
    return image;
}


void Render2D::ClearDraw()
{
    context.ClearBuffers(MPR_CI_DRAW_BUFFER);
}


void Render2D::ClearZBuffer()
{
    context.ClearBuffers(MPR_CI_ZBUFFER);
}


DWORD Render2D::Color()
{
    return context.CurrentForegroundColor();
}


void Render2D::SetColor(DWORD packedRGBA)
{
    context.RestoreState(STATE_SOLID);
    context.SelectForegroundColor(packedRGBA);
}


void Render2D::SetBackground(DWORD packedRGBA)
{
    context.SetState(MPR_STA_BG_COLOR, packedRGBA);
}


void Render2D::SetOffset(float x, float y)
{
    OffsetX = x;
    OffsetY = y;
}


void Render2D::SetLineStyle(int)
{
}


/***************************************************************************\
 Setup the rendering context for this display
\***************************************************************************/
void Render2D::Setup(ImageBuffer *imageBuffer)
{
    BOOL result;

    image = imageBuffer;

    // Create the MPR rendering context (frame buffer, etc.)

    // OW
    //result = context.Setup( (DWORD)imageBuffer->targetSurface(), (DWORD)imageBuffer->GetDisplayDevice()->GetMPRdevice());
    result = context.Setup(imageBuffer, imageBuffer->GetDisplayDevice()->GetDefaultRC());

    if ( not result)
    {
        ShiError("Failed to setup rendering context");
    }

    // Store key properties of our target buffer
    xRes = image->targetXres();
    yRes = image->targetYres();
    OffsetX = OffsetY = 0;

    // ASSO: reset adjusted RTT viewport
    tLeft = 0;
    tTop = 0;
    tRight = xRes;
    tBottom = yRes;
    txRes = xRes;
    tyRes = yRes;

    // Set the renderer's default foreground and background colors
    SetColor(0xFFFFFFFF);
    SetBackground(0xFF000000);

    // Call our base classes setup function (must come AFTER xRes and Yres have been set)
    VirtualDisplay::Setup();
}



/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void Render2D::Cleanup(void)
{
    context.Cleanup();

    VirtualDisplay::Cleanup();
}



/***************************************************************************\
    Replace the image buffer used by this renderer
\***************************************************************************/
void Render2D::SetImageBuffer(ImageBuffer *imageBuffer)
{
    // Remember who our new image buffer is, and tell MPR about the change
    image = imageBuffer;
    context.NewImageBuffer((DWORD)imageBuffer->targetSurface());

    // This shouldn't be required, but _might_ be
    // context.InvalidateState();
    context.RestoreState(STATE_SOLID);

    // Store key properties of our target buffer
    xRes = image->targetXres();
    yRes = image->targetYres();

    // ASSO: reset adjusted RTT viewport
    tLeft = 0;
    tTop = 0;
    tRight = xRes;
    tBottom = yRes;
    txRes = xRes;
    tyRes = yRes;


    // Setup the default viewport
    SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);

    // Setup the default offset and rotation
    CenterOriginInViewport();
    ZeroRotationAboutOrigin();
}


/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void Render2D::StartDraw(void)
{
    // DX - YELLOW BUG FIX - RED
    // ShiAssert(image);
    context.StartDraw();
}

/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void Render2D::EndDraw(void)
{
    // DX - YELLOW BUG FIX - RED
    // ShiAssert(image);
    context.EndDraw();
}

/***************************************************************************\
 Set the dimensions and location of the viewport.
\***************************************************************************/
void Render2D::SetViewport(float l, float t, float r, float b)
{
    // First call the base classes version of this function
    VirtualDisplay::SetViewport(l, t, r, b);

    // Send the new clipping region to MPR
    // (top/right inclusive, bottom/left exclusive)
    context.SetState(MPR_STA_ENABLES, MPR_SE_SCISSORING);
    context.SetState(MPR_STA_SCISSOR_TOP, FloatToInt32((float)floor(topPixel)));
    context.SetState(MPR_STA_SCISSOR_LEFT, FloatToInt32((float)floor(leftPixel)));
    context.SetState(MPR_STA_SCISSOR_RIGHT, FloatToInt32((float)ceil(rightPixel)));
    context.SetState(MPR_STA_SCISSOR_BOTTOM, FloatToInt32((float)ceil(bottomPixel)));
}



/***************************************************************************\
 Put a pixel on the display.
\***************************************************************************/
void Render2D::Render2DPoint(float x1, float y1)
{
    if (ForceAlpha) context.RestoreState(STATE_ALPHA_SOLID);

    context.Draw2DPoint(x1 + (int)OffsetX, y1 + (int)OffsetY);
}



/***************************************************************************\
 Put a straight line on the display.
\***************************************************************************/
void Render2D::Render2DLine(float x1, float y1, float x2, float y2)
{
    if (ForceAlpha) context.RestoreState(STATE_CHROMA_TEXTURE_GOURAUD2); // COBRA - RED - Alpha Option

    context.Draw2DLine(x1 + (int)OffsetX, y1 + (int)OffsetY, x2 + (int)OffsetX, y2 + (int)OffsetY);
}



/***************************************************************************\
 Put a mono-colored screen space triangle on the display.
\***************************************************************************/
void Render2D::Render2DTri(float x1, float y1, float x2, float y2, float x3, float y3)
{
    MPRVtx_t verts[3];

    //Clip test
    if (
        (max(max(x1, x2), x3) > rightPixel) or
        (min(min(x1, x2), x3) < leftPixel) or
        (max(max(y1, y2), y3) > bottomPixel) or
        (min(min(y1, y2), y3) < topPixel)
    )
        return;

    // Package up the tri's coordinates
    verts[0].x = x1 + OffsetX;
    verts[0].y = y1 + OffsetY;
    verts[1].x = x2 + OffsetX;
    verts[1].y = y2 + OffsetY;
    verts[2].x = x3 + OffsetX;
    verts[2].y = y3 + OffsetY;

    // Draw the triangle
    // context.RestoreState( STATE_ALPHA_SOLID );
    if (ForceAlpha) context.RestoreState(STATE_CHROMA_TEXTURE_GOURAUD2); // COBRA - RED - Alpha Option

    context.DrawPrimitive(MPR_PRM_TRIANGLES, 0, 3, verts, sizeof(verts[0]));
}



/***************************************************************************\
 Put a portion of a caller supplied 32 bit bitmap on the display.
 The pixels should be of the form 0x00BBGGRR
 Chroma keying is not supported
\***************************************************************************/
void Render2D::Render2DBitmap(int sX, int sY, int dX, int dY, int w, int h, int totalWidth, DWORD *source, bool Fit)
{
    if (ForceAlpha) context.RestoreState(STATE_ALPHA_SOLID); // COBRA - RED - Alpha Option

    context.Render2DBitmap(sX, sY, dX + (int)OffsetX, dY + (int)OffsetY, w, h, totalWidth, source, Fit);
}



/***************************************************************************\
 Put a portion of a bitmap from a file on disk on the display.
 Chroma keying is not supported
\***************************************************************************/
void Render2D::Render2DBitmap(int sX, int sY, int dX, int dY, int w, int h, char *filename, bool Fit)
{
    int result;
    CImageFileMemory  texFile;
    int totalWidth;
    DWORD *dataptr;


    // Make sure we recognize this file type
    texFile.imageType = CheckImageType(filename);
    ShiAssert(texFile.imageType not_eq IMAGE_TYPE_UNKNOWN);

    // Open the input file
    result = texFile.glOpenFileMem(filename);
    ShiAssert(result == 1);

    // Read the image data (note that ReadTextureImage will close texFile for us)
    texFile.glReadFileMem();
    result = ReadTextureImage(&texFile);

    if (result not_eq GOOD_READ)
    {
        ShiError("Failed to read bitmap.  CD Error?");
    }

    // Store the image size (check it in debug mode)
    totalWidth = texFile.image.width;
    ShiAssert(sX + w <= texFile.image.width);
    ShiAssert(sY + h <= texFile.image.height);

    // Force the data into 32 bit color
    dataptr = (DWORD*)ConvertImage(&texFile.image, COLOR_16M, NULL);
    ShiAssert(dataptr);

    // Release the unconverted image data
    // edg: I've seen palette be NULL
    if (texFile.image.palette)
        glReleaseMemory((char*)texFile.image.palette);

    glReleaseMemory((char*)texFile.image.image);

    // Pass the bitmap data into the bitmap display function
    if (ForceAlpha) context.RestoreState(STATE_ALPHA_SOLID); // COBRA - RED - Alpha Option

    Render2DBitmap(sX, sY, dX + (int)OffsetX, dY + (int)OffsetY, w, h, totalWidth, dataptr, Fit);

    // Release the converted image data
    glReleaseMemory(dataptr);
}

/***************************************************************************\
 Put a mono-colored string of text on the display in screen space.
 (The location given is used as the upper left corner of the text in units of pixels)
\***************************************************************************/
//JAM 22Dec03 - Don't they teach people how to format code?
void Render2D::ScreenText(float xLeft, float yTop, const char *string, int boxed)
{
    int color;
    float x, y;
    float r, g, b, a;
    TwoDVertex vert[4 * 256];

    //JAM 15Dec03
    BOOL bToggle = FALSE;

    if (context.bZBuffering and DisplayOptions.bZBuffering)
    {
        bToggle = TRUE;
        context.SetZBuffering(FALSE);
        context.SetState(MPR_STA_DISABLES, MPR_SE_Z_BUFFERING);
    }

    //JAM

    x = (float)floor(xLeft) + OffsetX;
    y = (float)floor(yTop) + OffsetY;

    // Select font texture here

    color = Color();

    // COBRA - RED - is forced alpha get it from the color else dafaults to 1
    if (ForceAlpha) a = ((color bitand 0xFF000000) >> 24) / 255.0F;
    else a = 1.0f;

    // Draw two tris to make a square;
    if (boxed not_eq 2)
    {
        r = (color bitand 0xFF) / 255.0F;
        g = ((color bitand 0xFF00) >> 8) / 255.0F;
        b = ((color bitand 0xFF0000) >> 16) / 255.0F;
    }
    else
    {
        // boxed == 2 means inverse text, so draw the square
        r = 0.0F;
        g = 0.0F;
        b = 0.0F;
        vert[0].x = x - 1.8F; //MI changed from - 2.0F
        vert[0].y = y;
        vert[1].x = vert[0].x;
        vert[1].y = vert[0].y + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight - 1; //MI added -1
        vert[2].x = vert[0].x + ScreenTextWidth(string) + 1.8F; //MI changed from +4.0F
        vert[2].y = vert[1].y;
        vert[3].x = vert[2].x;
        vert[3].y = vert[0].y;

        if (ForceAlpha) context.RestoreState(STATE_ALPHA_SOLID); // COBRA - RED - Alpha Option
        else context.RestoreState(STATE_SOLID);

        context.DrawPrimitive(MPR_PRM_TRIFAN, 0, 4, vert, sizeof(vert[0]));
    }

    //Wombat778 3-09-2004 Added to allow colored text on a black background
    if (boxed == 3)
    {

        DWORD tempcolor = Color();
        SetColor(0x00000000);

        vert[0].x = x - 1.8F; //MI changed from - 2.0F
        vert[0].y = y;
        vert[1].x = vert[0].x;
        vert[1].y = vert[0].y + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight - 1; //MI added -1
        vert[2].x = vert[0].x + ScreenTextWidth(string) + 1.8F; //MI changed from +4.0F
        vert[2].y = vert[1].y;
        vert[3].x = vert[2].x;
        vert[3].y = vert[0].y;

        if (ForceAlpha) context.RestoreState(STATE_ALPHA_SOLID); // COBRA - RED - Alpha Option
        else context.RestoreState(STATE_SOLID);

        context.DrawPrimitive(MPR_PRM_TRIFAN, 0, 4, vert, sizeof(vert[0]));

        SetColor(tempcolor);
    }


    SetColor(color);

    if (ForceAlpha)
    {
        // force the Hud mode bitand text gets color from the Vertices
        context.RestoreState(STATE_CHROMA_TEXTURE_GOURAUD2); // COBRA - RED - Alpha Option
        context.TexColorDiffuse();
    }
    else context.RestoreState(STATE_TEXTURE_TEXT);   //JAM 18Oct03

    context.SelectTexture1(pFontSet->fontTexture[pFontSet->fontNum].TexHandle());

    TwoDVertex *pVtx = vert;

    int n = 0;

    while (*string)
    {
        // Top Left 1
        pVtx[0].x = x;
        pVtx[0].y = y;
        pVtx[0].r = r;
        pVtx[0].g = g;
        pVtx[0].b = b;
        pVtx[0].a = a;
        // pVtx[0].a = 1.0F;
        pVtx[0].u = pFontSet->fontData[pFontSet->fontNum][*string].left;
        pVtx[0].v = pFontSet->fontData[pFontSet->fontNum][*string].top;
        pVtx[0].q = 1.0F;

        // Top Right 1
        pVtx[1].x = x + (pFontSet->fontData[pFontSet->fontNum][*string].width * 256.0f);
        pVtx[1].y = y;
        pVtx[1].r = r;
        pVtx[1].g = g;
        pVtx[1].b = b;
        pVtx[1].a = a;
        // pVtx[1].a = 1.0F;
        pVtx[1].u = pFontSet->fontData[pFontSet->fontNum][*string].left + pFontSet->fontData[pFontSet->fontNum][*string].width;
        pVtx[1].v = pFontSet->fontData[pFontSet->fontNum][*string].top;
        pVtx[1].q = 1.0F;

        // Bottom Left 1
        pVtx[2].x = x;
        pVtx[2].y = y + pFontSet->fontData[pFontSet->fontNum][*string].pixelHeight;
        pVtx[2].r = r;
        pVtx[2].g = g;
        pVtx[2].b = b;
        pVtx[2].a = a;
        // pVtx[2].a = 1.0F;
        pVtx[2].u = pFontSet->fontData[pFontSet->fontNum][*string].left;
        pVtx[2].v = pFontSet->fontData[pFontSet->fontNum][*string].top + pFontSet->fontData[pFontSet->fontNum][*string].height;
        pVtx[2].q = 1.0F;

        // Bottom Left 2
        pVtx[3] = pVtx[2];

        // Top Right 2
        pVtx[4] = pVtx[1];

        // Bottom Right 2
        pVtx[5].x = x + (pFontSet->fontData[pFontSet->fontNum][*string].width * 256.0f);
        pVtx[5].y = y + pFontSet->fontData[pFontSet->fontNum][*string].pixelHeight;
        pVtx[5].r = r;
        pVtx[5].g = g;
        pVtx[5].b = b;
        pVtx[5].a = a;
        // pVtx[5].a = 1.0F;
        pVtx[5].u = pFontSet->fontData[pFontSet->fontNum][*string].left + pFontSet->fontData[pFontSet->fontNum][*string].width;
        pVtx[5].v = pFontSet->fontData[pFontSet->fontNum][*string].top + pFontSet->fontData[pFontSet->fontNum][*string].height;
        pVtx[5].q = 1.0F;

        // Do a block clip
        if ( not (pVtx[0].x <= rightPixel and pVtx[0].x >= leftPixel and pVtx[0].y <= bottomPixel and pVtx[0].y >= topPixel))
            break;

        if ( not (pVtx[1].x <= rightPixel and pVtx[1].x >= leftPixel and pVtx[1].y <= bottomPixel and pVtx[1].y >= topPixel))
            break;

        if ( not (pVtx[2].x <= rightPixel and pVtx[2].x >= leftPixel and pVtx[2].y <= bottomPixel and pVtx[2].y >= topPixel))
            break;

        if ( not (pVtx[5].x <= rightPixel and pVtx[5].x >= leftPixel and pVtx[5].y <= bottomPixel and pVtx[5].y >= topPixel))
            break;

        x += pFontSet->fontData[pFontSet->fontNum][*string].pixelWidth;
        string++;
        n++;
        pVtx += 6;
    }

    ShiAssert(n < 256);

    if (n)
        context.DrawPrimitive(MPR_PRM_TRIANGLES, MPR_VI_COLOR bitor MPR_VI_TEXTURE, n * 6, vert, sizeof(vert[0]));

    if (ForceAlpha) context.RestoreState(STATE_ALPHA_SOLID); // COBRA - RED - Alpha Option
    else context.RestoreState(STATE_SOLID);

    // Go back and box the string if necessary
    if (boxed == 1)
    {
        float x1 = xLeft - 2.0f;
        float y1 = yTop  - 2.0f;
        float x2 = (float)(x + 1);
        float y2 = yTop + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight;

        // Only draw the box if it is entirely on screen
        if ((x1 > leftPixel) and (x2 < rightPixel) and (y1 > topPixel) and (y2 < bottomPixel))
        {
            Render2DLine(x1, y1, x2, y1);
            Render2DLine(x2, y1, x2, y2);
            Render2DLine(x2, y2, x1, y2);
            Render2DLine(x1, y2, x1, y1);
        }
    }

    // Left Arrow
    if (boxed == 0x4)
    {
        float x0 = xLeft - 5.0F;
        float y0 = yTop + (pFontSet->fontData[pFontSet->fontNum][32].pixelHeight / 2);
        float x1 = xLeft - 2.0f;
        float y1 = yTop;
        float x2 = (float)(x + 1);
        float y2 = yTop + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight;

        x1 = max(x1, leftPixel + 1.0F);

        // Only draw the box if it is entirely on screen
        if ((x1 > leftPixel) and (x2 < rightPixel) and (y1 > topPixel) and (y2 < bottomPixel))
        {
            Render2DLine(x0, y0, x1, y1);
            Render2DLine(x0, y0, x1, y2);
            Render2DLine(x1, y1, x2, y1);
            Render2DLine(x2, y1, x2, y2);
            Render2DLine(x2, y2, x1, y2);
        }
    }

    // Right Arrow
    if (boxed == 0x8)
    {
        float x0 = float(x + 4);
        float y0 = yTop + (pFontSet->fontData[pFontSet->fontNum][32].pixelHeight / 2);
        float x1 = xLeft - 2.0f;
        float y1 = yTop;
        float x2 = (float)(x + 1);
        float y2 = yTop + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight;

        // Only draw the box if it is entirely on screen
        x1 = max(x1, leftPixel + 1.0F);

        if ((x1 > leftPixel) and (x2 < rightPixel) and (y1 > topPixel) and (y2 < bottomPixel))
        {
            Render2DLine(x0, y0, x2, y1);
            Render2DLine(x0, y0, x2, y2);
            Render2DLine(x1, y1, x2, y1);
            Render2DLine(x1, y1, x1, y2);
            Render2DLine(x2, y2, x1, y2);
        }
    }

    //JAM 15Dec03
    if (bToggle and DisplayOptions.bZBuffering)
    {
        context.SetZBuffering(TRUE);
        context.SetState(MPR_STA_ENABLES, MPR_SE_Z_BUFFERING);
    }
}

// Global Texture mapped font stuff
//JAM 22Dec03 - Now a static R2D func
void Render2D::Load2DFontSet()
{
#ifdef USE_TEXTURE_FONT

    //Wombat778 12-12-2003 Added to allow fonts to be chosen based on the current resolution (code that runs under not g_bAutoScaleFonts is the original)
    if ( not g_bAutoScaleFonts)
    {
        if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
            Font2D.fontTexture[0].CreateTexture("6x4font.gif");

        if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
            Font2D.fontTexture[1].CreateTexture("8x6font.gif");

        if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
            Font2D.fontTexture[2].CreateTexture("10x7font.gif");

        Font2D.ReadFontMetrics(0, "art\\ckptart\\6x4font.rct");
        Font2D.ReadFontMetrics(1, "art\\ckptart\\8x6font.rct");
        Font2D.ReadFontMetrics(2, "art\\ckptart\\10x7font.rct");
        Font2D.totalFont = 3; // JPO new font.

        if (Font2D.ReadFontMetrics(3, "art\\ckptart\\warn_font.rct") and 
            Font2D.fontTexture[3].LoadImage("art\\ckptart\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
        {
            Font2D.fontTexture[3].CreateTexture("warn_font.gif");
            Font2D.totalFont = 4;
        }
    }
    else
    {
        //Wombat778 12-15-2003 Moved the new fonts to the autofont directory...
        //This should ensure that old setups arent broken if scaling is disabled

        // RV - Biker - Check for widescreen resolutions
        if (DisplayOptions.DispWidth == 848 or DisplayOptions.DispWidth == 1440 or DisplayOptions.DispWidth == 1680 or DisplayOptions.DispWidth == 1920)
        {
            switch (DisplayOptions.DispWidth)
            {
                case 848:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("6x4font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;

                case 1440:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("8x6font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("10x7font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("12x9font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\8x6font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\10x7font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\12x9font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\12warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\12warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("12warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;

                default:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("10x7font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("12x9font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\16x12font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("16x12font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\10x7font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\12x9font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\16x12font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\16warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\16warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("16warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;
            }
        }

        else
        {
            //Wombat778 4-04-04 Reorganized and added support for 640 and 800.  Because there are no fonts lower than 640, just fill with duplicates of 640.
            switch (FindBestResolution())
            {
                case 640:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("6x4font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;

                case 800:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("8x6font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\8x6font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;

                case 1024:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("8x6font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("10x7font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\6x4font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\8x6font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\10x7font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;

                case 1280:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("8x6font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("10x7font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("12x9font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\8x6font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\10x7font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\12x9font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\12warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\12warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("12warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;

                case 1600:
                    if (Font2D.fontTexture[0].LoadImage("art\\ckptart\\autofont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[0].CreateTexture("10x7font.gif");

                    if (Font2D.fontTexture[1].LoadImage("art\\ckptart\\autofont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[1].CreateTexture("12x9font.gif");

                    if (Font2D.fontTexture[2].LoadImage("art\\ckptart\\autofont\\16x12font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font2D.fontTexture[2].CreateTexture("16x12font.gif");

                    Font2D.ReadFontMetrics(0, "art\\ckptart\\autofont\\10x7font.rct");
                    Font2D.ReadFontMetrics(1, "art\\ckptart\\autofont\\12x9font.rct");
                    Font2D.ReadFontMetrics(2, "art\\ckptart\\autofont\\16x12font.rct");
                    Font2D.totalFont = 3; // JPO new font.

                    if (Font2D.ReadFontMetrics(3, "art\\ckptart\\autofont\\16warn_font.rct") and 
                        Font2D.fontTexture[3].LoadImage("art\\ckptart\\autofont\\16warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font2D.fontTexture[3].CreateTexture("16warn_font.gif");
                        Font2D.totalFont = 4;
                    }

                    break;
            }
        }
    }

    //WOmbat778 12-12-2003 end of changed code

#endif
}

//JAM 22Dec03 - Now a static R2D func
void Render2D::Release2DFontSet()
{
#ifdef USE_TEXTURE_FONT
    Font2D.fontTexture[0].FreeAll();
    Font2D.fontTexture[1].FreeAll();
    Font2D.fontTexture[2].FreeAll();

    if (Font2D.totalFont > 3)
        Font2D.fontTexture[3].FreeAll();

#endif
}

// ASFO:
void Render2D::Load3DFontSet()
{
#ifdef USE_TEXTURE_FONT

    if ( not g_bAutoScaleFonts)
    {
        if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
            Font3D.fontTexture[0].CreateTexture("6x4font.gif");

        if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
            Font3D.fontTexture[1].CreateTexture("8x6font.gif");

        if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
            Font3D.fontTexture[2].CreateTexture("10x7font.gif");

        Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\6x4font.rct");
        Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\8x6font.rct");
        Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\10x7font.rct");
        Font3D.totalFont = 3; // JPO new font.

        if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\warn_font.rct") and 
            Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
        {
            Font3D.fontTexture[3].CreateTexture("warn_font.gif");
            Font3D.totalFont = 4;
        }
    }
    else
    {
        // RV - Biker - Check for widescreen resolutions
        if (DisplayOptions.DispWidth == 848 or DisplayOptions.DispWidth == 1440 or DisplayOptions.DispWidth == 1680 or DisplayOptions.DispWidth == 1920)
        {
            switch (DisplayOptions.DispWidth)
            {
                case 848:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("6x4font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;

                case 1440:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("8x6font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("10x7font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("12x9font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\8x6font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\10x7font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\12x9font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\12warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\12warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("12warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;

                default:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("10x7font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("12x9font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\16x12font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("16x12font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\10x7font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\12x9font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\16x12font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\16warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\16warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("16warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;
            }
        }
        else
        {
            switch (FindBestResolution())
            {
                case 640:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("6x4font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;

                case 800:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("8x6font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\8x6font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;

                case 1024:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\6x4font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("6x4font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("8x6font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("10x7font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\6x4font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\8x6font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\10x7font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;

                case 1280:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\8x6font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("8x6font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("10x7font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("12x9font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\8x6font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\10x7font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\12x9font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\12warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\12warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("12warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;

                case 1600:
                    if (Font3D.fontTexture[0].LoadImage("art\\ckptart\\3dfont\\10x7font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[0].CreateTexture("10x7font.gif");

                    if (Font3D.fontTexture[1].LoadImage("art\\ckptart\\3dfont\\12x9font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[1].CreateTexture("12x9font.gif");

                    if (Font3D.fontTexture[2].LoadImage("art\\ckptart\\3dfont\\16x12font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                        Font3D.fontTexture[2].CreateTexture("16x12font.gif");

                    Font3D.ReadFontMetrics(0, "art\\ckptart\\3dfont\\10x7font.rct");
                    Font3D.ReadFontMetrics(1, "art\\ckptart\\3dfont\\12x9font.rct");
                    Font3D.ReadFontMetrics(2, "art\\ckptart\\3dfont\\16x12font.rct");
                    Font3D.totalFont = 3; // JPO new font.

                    if (Font3D.ReadFontMetrics(3, "art\\ckptart\\3dfont\\16warn_font.rct") and 
                        Font3D.fontTexture[3].LoadImage("art\\ckptart\\3dfont\\16warn_font.gif", MPR_TI_CHROMAKEY bitor MPR_TI_PALETTE, FALSE))
                    {
                        Font3D.fontTexture[3].CreateTexture("16warn_font.gif");
                        Font3D.totalFont = 4;
                    }

                    break;
            }
        }
    }

#endif
}

// ASFO:
void Render2D::Release3DFontSet()
{
#ifdef USE_TEXTURE_FONT
    Font3D.fontTexture[0].FreeAll();
    Font3D.fontTexture[1].FreeAll();
    Font3D.fontTexture[2].FreeAll();

    if (Font3D.totalFont > 3)
        Font3D.fontTexture[3].FreeAll();

#endif
}

// ASFO:
void Render2D::ChangeFontSet(FontSet* pFontSet_)
{
    pFontSet = pFontSet_;
    //paFontTexture = static_cast<Texture(*)[NUM_FONT_RESOLUTIONS]>(aFontTexture_);
    //paFontData = static_cast<FontDataType(*)[NUM_FONT_RESOLUTIONS][256]>(aFontData
}

#ifdef USE_TEXTURE_FONT

extern FILE *ResFOpen(char *, char *);

int FontSet::ReadFontMetrics(int index, char*fileName) // JPO return status
{
    int
    file,
    size,
    idx,
    top,
    left,
    width,
    height,
    lead,
    trail;

    char
    *str;

    static char
    buffer[16000];

    ShiAssert(index < NUM_FONT_RESOLUTIONS and index >= 0);
    ShiAssert(FALSE == IsBadStringPtr(fileName, _MAX_PATH));

    file = GR_OPEN(fileName, O_RDONLY);

    if (file >= 0)
    {
        size = GR_READ(file, buffer, 15999);

        buffer[size] = 0;

        str = buffer;

        while (str and *str)
        {
            int n = sscanf(str, "%d %d %d %d %d %d %d", &idx, &left, &top, &width, &height, &lead, &trail);
            ShiAssert(n == 7);

            //JAM 22Dec03 - Not anymore, all modern video cards do automatic biasing.
            //TODO: Add global cfg variable for older cards.
            // if(DisplayOptions.bFontTexelAlignment)
            if (g_bOldFontTexelFix) //Wombat778 4-01-04 complete fix in drawprimitive
            {
                // OW: shift u,v by a half texel. if you dont do that and the card filters it fetches the wrong texels
                // because if you specify 1.0 you're saying that you want the far-right edge of this texel
                // and not the center of the texel like you thought
                float fOffset = (1.0f / 256.0F) / 2.0f;

                fontData[index][idx].top = top / 256.0F;
                fontData[index][idx].top += fOffset;

                fontData[index][idx].left = left / 256.0F;
                fontData[index][idx].left += fOffset;

                fontData[index][idx].width = width / 256.0F;
                fontData[index][idx].width -= fOffset;

                fontData[index][idx].height = height / 256.0F;
                fontData[index][idx].height -= fOffset;

                fontData[index][idx].pixelHeight = (float)height;
                fontData[index][idx].pixelWidth = (float)(width + lead);
            }
            else
            {
                fontData[index][idx].top = top / 256.0F;
                fontData[index][idx].left = left / 256.0F;
                fontData[index][idx].width = width / 256.0F;
                fontData[index][idx].height = height / 256.0F;
                fontData[index][idx].pixelHeight = (float)height;
                fontData[index][idx].pixelWidth = (float)(width + lead);
            }

            str = strstr(str, "\n");

            while ((str) and ((*str == '\n') or (*str == '\r')))
            {
                str ++;
            }
        }

        GR_CLOSE(file);
        return 1;
    }

    return 0;
}
#endif
