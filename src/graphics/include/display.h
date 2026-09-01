/***************************************************************************\
    Display.h
    Scott Randolph
    February 1, 1995

    This class provides basic 2D drawing functions in a device independent fashion.
\***************************************************************************/
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "ttypes.h"
#include "imagebuf.h" // ASSO:
#include "context.h" // ASSO:
#include "tex.h" // ASSO:

// Artscout - 2026: ODR/layout guard (see context.h). VirtualDisplay embeds a ContextMPR member
// (`context`); its offset must match in every translation unit, otherwise callers compute a
// wrong `this` for context.* (the mouse-cursor 2D-vanish bug). Pin packing to 8 so the member
// layout is identical regardless of any ambient "#pragma pack" leaking in via include order.
#pragma pack(push, 8)


//#define USE_ORIGINAL_FONT
//#define USE_STROKE_FONT
#define USE_TEXTURE_FONT

#ifdef USE_TEXTURE_FONT

#define NUM_FONT_RESOLUTIONS 4

typedef struct FontDataTag
{
    float top;
    float left;
    float width;
    float height;
    float pixelWidth;
    float pixelHeight;
} FontDataType;

// ASFO:
struct FontSet
{
    FontSet()
    {
        fontNum = 0;
        totalFont = 3;
    }
    ~FontSet()
    {
    }

    int ReadFontMetrics(int indx, char *fileName);
    Texture fontTexture[NUM_FONT_RESOLUTIONS];
    FontDataType fontData[NUM_FONT_RESOLUTIONS][256];
    //int fontSpacing[NUM_FONT_RESOLUTIONS];
    int fontNum;
    int totalFont;
};

//int  ReadFontMetrics(int indx, char*fileName);
//extern FontDataType pFontSet->fontData[NUM_FONT_RESOLUTIONS][256];
//extern int fontSpacing[NUM_FONT_RESOLUTIONS];

#endif

//extern int FontNum;
//extern int pFontSet->totalFont;


// Clipping flags.  Some of these are only used in 3D clipping, but I want to keep them together.
static const DWORD ON_SCREEN = 0x00;
static const DWORD CLIP_LEFT = 0x01;
static const DWORD CLIP_RIGHT = 0x02;
static const DWORD CLIP_TOP = 0x04;
static const DWORD CLIP_BOTTOM = 0x08;
static const DWORD CLIP_NEAR = 0x10;
static const DWORD CLIP_FAR = 0x20;
static const DWORD OFF_SCREEN = 0xFF;

static const int CircleStep = 4; // In units of degrees
static const int CircleSegments =
    360 / CircleStep + 1; // How many segments (plus one)?
extern float CircleX[];
extern float CircleY[];

struct DisplayMatrix // JPO - how a display is oriented
{
    float translationX, translationY;
    float rotation00, rotation01;
    float rotation10, rotation11;
};


class Render3D; // ASSO:


class VirtualDisplay
{
public:
    VirtualDisplay();
    virtual ~VirtualDisplay();

    // One time call to create inverse font
    static void InitializeFonts(void);

    // Parents Setup() must set xRes and yRes before call this...
    virtual void Setup(void);
    virtual void Cleanup(void);
    BOOL IsReady(void);

    virtual void StartDraw(void) = 0;
    virtual void ClearDraw(void) = 0;
    virtual void EndDraw(void) = 0;

    virtual void Point(float x1, float y1);
    virtual void Line(float x1, float y1, float x2, float y2);
    virtual void Line(float x1, float y1, float x2, float y2, float width);
    virtual void Tri(float x1, float y1, float x2, float y2, float x3,
                     float y3);
    virtual void Oval(float x, float y, float xRadius, float yRadius);
    virtual void OvalArc(float x, float y, float xRadius, float yRadius,
                         float start, float stop);
    virtual void Circle(float x, float y, float xRadius);
    virtual void Arc(float x, float y, float xRadius, float start, float stop);

    virtual void TextLeft(float x1, float y1, const char *string,
                          int boxed = 0);
    virtual void TextRight(float x1, float y1, const char *string,
                           int boxed = 0);
    virtual void TextLeftVertical(float x1, float y1, const char *string,
                                  int boxed = 0);
    virtual void TextRightVertical(float x1, float y1, const char *string,
                                   int boxed = 0);
    virtual void TextCenter(float x1, float y1, const char *string,
                            int boxed = 0);
    virtual void TextCenterVertical(float x1, float y1, const char *string,
                                    int boxed = 0);
    virtual int TextWrap(float h, float v, const char *string, float spacing,
                         float width);

    // NOTE:  These might need to be virtualized and overloaded by canvas3d (maybe???)
    virtual float TextWidth(char *string); // normalized screen space
    virtual float TextHeight(void); // normalized screen space

    //JAM 22Dec03
    virtual void SetColor(DWORD) = 0;
    virtual void SetBackground(DWORD) = 0;
    virtual void ScreenText(float x, float y, const char *string,
                            int boxed = 0) = 0;

    static int ScreenTextHeight(void);
    static int ScreenTextWidth(const char *string);

    static void SetFont(int newFont);
    static int CurFont(void); //JAM

    virtual void SetLineStyle(int);
    virtual DWORD Color(void);

    virtual void SetViewport(float leftSide, float topSide, float rightSide,
                             float bottomSide);
    virtual void SetViewportRelative(float leftSide, float topSide,
                                     float rightSide, float bottomSide);

    void AdjustOriginInViewport(float horizontal, float vertical);
    void AdjustRotationAboutOrigin(float angle);
    void CenterOriginInViewport(void);
    void ZeroRotationAboutOrigin(void);
    void SaveDisplayMatrix(DisplayMatrix *dm);
    void RestoreDisplayMatrix(DisplayMatrix *dm);

    int GetXRes(void);
    int GetYRes(void);

    void GetViewport(float *leftSide, float *topSide, float *rightSide,
                     float *bottomSide);

    float GetTopPixel(void);
    float GetBottomPixel(void);
    float GetLeftPixel(void);
    float GetRightPixel(void);

    float GetXOffset(void);
    float GetYOffset(void);

    // VR: temporarily render at a per-eye resolution. Caller follows with SetViewport()
    // (recomputes scaleX/scaleY) and SetFOV() (recomputes the projection), then restores.
    void VR_SetRes(int w, int h)
    {
        xRes = w;
        yRes = h;
        txRes = w;
        tyRes = h;
    }
    int VR_GetResX(void) const
    {
        return xRes;
    }
    int VR_GetResY(void) const
    {
        return yRes;
    }

    enum
    {
        DISPLAY_GENERAL = 0,
        DISPLAY_CANVAS
    } type;

    // Functions to convert from normalized coordinates to pixel coordinates
    // (assumes at this point that x is right and y is down)
    float viewportXtoPixel(float x);
    float viewportYtoPixel(float y);

protected:
    // Functions which must be provided by all derived classes
    virtual void Render2DPoint(float x1, float y1) = 0;
    virtual void Render2DLine(float x1, float y1, float x2, float y2) = 0;

    // Functions which should be provided by all derived classes
    virtual void Render2DTri(float x1, float y1, float x2, float y2, float x3,
                             float y3);

protected:
    // Store the currently selected resolution
    int xRes;
    int yRes;

    // The viewport properties in normalized screen space (-1 to 1)
    float left;
    float right;
    float top;
    float bottom;

    // The parameters required to get from normalized screen space to pixel space
    // TEMPORARILY PUBLIC TO GET THINGS GOING...
public:
    float scaleX;
    float scaleY;
    float shiftX;
    float shiftY;

protected:
    ImageBuffer *image;

    // Store the pixel space boundries of the current viewport
    // (top/right inclusive, bottom/left exclusive)
    float topPixel;
    float bottomPixel;
    float leftPixel;
    float rightPixel;

    // The 2D rotation/translation settings
    DisplayMatrix
        dmatrix; // JPO - now in a sub structure so you can save/restore
    //float translationX, translationY;
    //float rotation00, rotation01;
    //float rotation10, rotation11;

    // The font information for drawing text
    static const unsigned char FontLUT[256];
    static const unsigned char *Font[];
    static const unsigned int FontLength;
    static unsigned char InvFont[][8];
    BOOL ready;

public:
    bool ForceAlpha; // COBRA - RED - To force translucent displays
    // ASSO: BEGIN
    // Window and rendering context handles
    ContextMPR context;

    // ASFO:
    static FontSet Font2D;
    static FontSet Font3D;
    static FontSet *pFontSet;

    static bool SetupRttTarget(int tXres_, int tYres_, int tBpp_);
    static bool CleanupRttTarget();
    void StartRtt(Render3D *r3d_);
    void FinishRtt();
    // Artscout - 2026: re-bind the shared RTT atlas as the active D3D11 target WITHOUT the StartRtt
    // bookkeeping (no save/clear/rect reset). The GM radar's beam sub-render does EndDraw->BindBackBuffer
    // mid-batch, which unbinds the atlas; its 2D composite then leaked to the back buffer (GM drawn big
    // on screen, absent from the atlas). Call this to restore the atlas before the composite.
    void ReBindRttTarget();
    // Artscout - 2026: confine the D3D11 viewport to THIS display's atlas sub-zone (tLeft..tRight, the
    // same rect DrawRttQuad samples). A sensor scene's 3D OBJECTS (VS_Object, centred clip-NDC) otherwise
    // render at the FULL-atlas centre regardless of the display zone -> they leak into whatever other
    // display's zone covers the centre (TGP/Maverick target appearing on HUD/DED/RWR). Call before the
    // object flush; restore the full viewport afterwards (ReBindRttTarget). The terrain (screen-path,
    // full-atlas coords) is drawn earlier with the full viewport and is unaffected.
    void ConfineObjectViewportToZone();
    void SetRttCanvas(Tpoint *ul_, Tpoint *ur_, Tpoint *ll_, char blendMode_,
                      float alpha_);
    void SetRttRect(int tLeft_, int tTop_, int tRight_, int tBottom_,
                    bool rt_ = true);
    void AdjustRttViewport();
    void ResetRttViewport();
    void DrawRttQuad();
    // Artscout - 2026 (VR HUD 3D glass): draw a flat tinted semi-transparent quad over the RTT canvas
    // (the physical combiner-glass rectangle) so the glass plate reads as glass. Uses the SAME canvas
    // transform as DrawRttQuad (fixed in the cockpit world, NOT collimated), alpha-blended, no texture.
    void DrawGlassPlate(float r, float g, float b, float a);
    void
    DrawRttDebugOverlay(); // debug helper: draw the raw renderTexture into a screen corner
    int HasRttTarget();
    // Artscout - 2026 (sensor video): map the 2D screen path by THIS display's atlas-zone size (the
    // sensor scene's CPU spans arrive in zone pixels); the next atlas bind restores the atlas metric.
    void ConfineScreenMetricToZone();
    void GetRttCanvas(Tpoint *Canvas);

protected:
    int tLeft;
    int tTop;
    int tRight;
    int tBottom;
    int txRes;
    int tyRes;

    Tpoint canUL;
    Tpoint canUR;
    Tpoint canLL;
    int rttBlendMode;
    float rttAlpha;

    int oldXRes;
    int oldYRes;
    static float oldTop;
    static float oldLeft;
    static float oldBottom;
    static float oldRight;

    static TextureHandle *renderTexture;
    static IDirectDrawSurface7 *oldTarget;
    static Render3D *r3d;
    // ASSO: END
};

#pragma pack(                                                                  \
    pop) // Artscout - 2026: end ODR/layout packing guard (see top of file)


#endif // _DISPLAY_H_
