/***************************************************************************\
    Render2D.cpp
    Scott Randolph
    December 29, 1995

    This class provides 2D drawing functions.
\***************************************************************************/
#include <cISO646>
#include <math.h>
#include <stdio.h>
#include <windows.h>
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

    const float ox = (float)(int)OffsetX, oy = (float)(int)OffsetY;

    // #7 LINE THICKNESS under SSAA: Draw2DLine draws a LINESTRIP = always 1px in atlas pixels.
    // Under SSAA (atlas x g_rttFontScale) a 1px line gives 1/g on the panel = 'thin'. Draw g parallel
    // lines offset 1px along the perpendicular -> ~g px in the atlas = ~1px on the panel.
    // BUT thickening DEPENDS ON LENGTH: long lines (ASEC circle ~114px/segment, horizon, ladder)
    // are bold (x1.4, closer to BMS); short ones (speed/altitude scale ticks ~6px, FPM) are thin, else
    // thickening collapses a short tick into a 'dot' instead of a short bar (ex1.png). Outside RTT = 1px.
    extern bool g_rttBatchActive; extern float g_rttFontScale;

    float dx = x2 - x1, dy = y2 - y1;
    float len = (float)sqrt(dx * dx + dy * dy);   // length in atlas pixels

    int w = 1;
    if (g_rttBatchActive && g_rttFontScale > 1.5f)
        w = (len > 15.0f) ? (int)(g_rttFontScale * 1.4f + 0.5f)   // long: bold
                          : (int)(g_rttFontScale + 0.5f);          // short ticks: ~1px panel = thin bar

    if (w <= 1)
    {
        context.Draw2DLine(x1 + ox, y1 + oy, x2 + ox, y2 + oy);
        return;
    }

    float px = 0.0f, py = 0.0f;
    if (len > 0.0001f) { px = -dy / len; py = dx / len; }   // unit perpendicular

    float start = -(float)(w - 1) * 0.5f;
    for (int i = 0; i < w; ++i)
    {
        float off = start + (float)i;
        float sx = px * off, sy = py * off;
        context.Draw2DLine(x1 + ox + sx, y1 + oy + sy, x2 + ox + sx, y2 + oy + sy);
    }
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

    // #7: text size multiplier for the enlarged RTT atlas (1024). Active ONLY during
    // the displays' RTT pass (g_rttBatchActive); outside it = 1.0 (menus/2D untouched).
    // Scale glyph geometry (width/height/advance), leave UV untouched.
    extern bool g_rttBatchActive; extern float g_rttFontScale;
    float fS = g_rttBatchActive ? g_rttFontScale : 1.0f;

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
        vert[1].y = vert[0].y + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS - 1; //MI added -1
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
        vert[1].y = vert[0].y + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS - 1; //MI added -1
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

    // #7 DISPLAY TEXT BRIGHTNESS: glyph color FROM THE VERTEX (TexColorDiffuse: the font texture =
    // a mask only) -> uniform brightness = Color(). Otherwise STATE_TEXTURE_TEXT takes color FROM
    // the font TEXTURE (palette/AA) -> uneven/dim (DED unreadable).
    if (ForceAlpha)
    {
        // HUD (translucent): GOURAUD2 (x2 brightness, ADDR_WRAP -- the HUD font UVs stay within cells).
        context.RestoreState(STATE_CHROMA_TEXTURE_GOURAUD2); // COBRA - RED - Alpha Option
        context.TexColorDiffuse();
    }
    else if (g_rttBatchActive)
    {
        // RTT displays DED/MFD/RWR: mask (uniform brightness) BUT on STATE_TEXTURE_TEXT with
        // ADDR_CLAMP -- GOURAUD2 with ADDR_WRAP smeared glyphs vertically (sampling neighboring
        // font-atlas cells 12x9/16x12).
        context.RestoreState(STATE_TEXTURE_TEXT);
        context.TexColorDiffuse();
    }
    else context.RestoreState(STATE_TEXTURE_TEXT);   //JAM 18Oct03

    context.SelectTexture1(pFontSet->fontTexture[pFontSet->fontNum].TexHandle());

    TwoDVertex *pVtx = vert;

    int n = 0;

    while (*string)
    {
        // #7 PIXEL-SNAP: glyphs are placed in float (x accumulates by a fractional step, sizes width*256*fS
        // fractional) -> bilinear sampling of a fractional position gives DIFFERENT height/thickness between
        // glyphs ('font swims'). Snap the glyph quad corners to WHOLE atlas pixels ->
        // even baseline + equal thickness. Only for RTT displays (g_rttBatchActive).
        const float _gw = pFontSet->fontData[pFontSet->fontNum][*string].width * 256.0f * fS;
        const float _gh = pFontSet->fontData[pFontSet->fontNum][*string].pixelHeight * fS;
        // #7 PIXEL-SNAP TEMPORARILY OFF (test 'bare', closer to FF6/D3D7): glyphs in float, as in the original.
        const float sx  = x;
        const float sy  = y;
        const float sx2 = x + _gw;
        const float sy2 = y + _gh;

        // Top Left 1
        pVtx[0].x = sx;
        pVtx[0].y = sy;
        pVtx[0].r = r;
        pVtx[0].g = g;
        pVtx[0].b = b;
        pVtx[0].a = a;
        // pVtx[0].a = 1.0F;
        pVtx[0].u = pFontSet->fontData[pFontSet->fontNum][*string].left;
        pVtx[0].v = pFontSet->fontData[pFontSet->fontNum][*string].top;
        pVtx[0].q = 1.0F;

        // Top Right 1
        pVtx[1].x = sx2;
        pVtx[1].y = sy;
        pVtx[1].r = r;
        pVtx[1].g = g;
        pVtx[1].b = b;
        pVtx[1].a = a;
        // pVtx[1].a = 1.0F;
        pVtx[1].u = pFontSet->fontData[pFontSet->fontNum][*string].left + pFontSet->fontData[pFontSet->fontNum][*string].width;
        pVtx[1].v = pFontSet->fontData[pFontSet->fontNum][*string].top;
        pVtx[1].q = 1.0F;

        // Bottom Left 1
        pVtx[2].x = sx;
        pVtx[2].y = sy2;
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
        pVtx[5].x = sx2;
        pVtx[5].y = sy2;
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

        x += pFontSet->fontData[pFontSet->fontNum][*string].pixelWidth * fS;
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
        float y2 = yTop + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS;

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
        // Artscout - 2026: arrow tip depth proportional to the box height (was a flat -5 px) -> clear '<'.
        float bh = pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS;
        float x0 = (xLeft - 2.0F) - bh * 0.55F;
        float y0 = yTop + (pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS / 2);
        float x1 = xLeft - 2.0f;
        float y1 = yTop;
        float x2 = (float)(x + 1);
        float y2 = yTop + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS;

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
        // Artscout - 2026: arrow tip depth proportional to the box height (was a flat +4 px -> looked like a
        // straight edge at HUD scale). ~0.55x height gives a clear '>' like the real F-16 / BMS airspeed box.
        float bh = pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS;
        float x0 = float(x + 1) + bh * 0.55F;
        float y0 = yTop + (pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS / 2);
        float x1 = xLeft - 2.0f;
        float y1 = yTop;
        float x2 = (float)(x + 1);
        float y2 = yTop + pFontSet->fontData[pFontSet->fontNum][32].pixelHeight * fS;

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
        if (DisplayOptions.DispWidth == 848 or DisplayOptions.DispWidth == 1440 or DisplayOptions.DispWidth == 1680 or DisplayOptions.DispWidth == 1920 or DisplayOptions.DispWidth == 2560 or DisplayOptions.DispWidth == 3840)
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
        if (DisplayOptions.DispWidth == 848 or DisplayOptions.DispWidth == 1440 or DisplayOptions.DispWidth == 1680 or DisplayOptions.DispWidth == 1920 or DisplayOptions.DispWidth == 2560 or DisplayOptions.DispWidth == 3840)
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
            extern bool g_bUseD3D11;
            if (false /* #7 texel inset OFF: tradeoff height vs (left-edge clip/column gaps),
                         not cleanly solvable in this pipeline. The real fix is ROW PADDING in the .gif
                         font atlas (1px gap), then the bleed goes away without an inset. A content fix. */)
            {
                // #7 UNEVEN LETTER HEIGHT: font glyphs in the atlas are packed TIGHTLY in ROWS
                // (A in the row top=2, M/R at top=20). D3D11 without auto texel-bias -> the bilinear filter
                // mixes in the NEIGHBORING ROW at the glyph's top/bottom -> uneven height. Inset UV ONLY
                // VERTICALLY (top+0.5, height-1) removes row-bleed -> even height.
                // LEAVE HORIZONTAL ALONE: glyphs have DIFFERENT widths, and width-1 thinned them UNEVENLY
                // (narrow ones more) -> uneven thickness (test-confirmed). Leave pixelHeight/Width alone.
                // Shift +0.5 texel on BOTH axes (consistent -> equal sharpness for horiz/vert
                // strokes). Shrink (-1) ONLY the height (tightly packed rows there -> row-bleed);
                // do NOT shrink width (glyphs vary in width, -1 would thin unevenly).
                fontData[index][idx].top    = (top    + 0.5f) / 256.0f;
                fontData[index][idx].height = (height - 1.0f) / 256.0f;
                fontData[index][idx].left   = (left   + 0.5f) / 256.0f;
                fontData[index][idx].width  = width / 256.0f;
                fontData[index][idx].pixelHeight = (float)height;
                fontData[index][idx].pixelWidth = (float)(width + lead);
            }
            else if (g_bOldFontTexelFix) //Wombat778 4-01-04 complete fix in drawprimitive
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
