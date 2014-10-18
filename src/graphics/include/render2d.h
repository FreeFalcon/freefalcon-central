/***************************************************************************\
    Render2D.h
    Scott Randolph
    December 29, 1995

    This class provides 2D drawing functions.
\***************************************************************************/
#ifndef _RENDER2D_H_
#define _RENDER2D_H_

#include "Ttypes.h"
//#include "ImageBuf.h" // ASSO: moved to display.h so the RTT works correctly
//#include "Context.h" // ASSO: moved to display.h so the RTT works correctly
#include "Display.h"
//#include "Tex.h" // ASSO: moved to display.h so the RTT works correctly

typedef struct TwoDVertex: public MPRVtxTexClr_t
{
    /* MPRVtxTexClr_t provides:
     float x, y;
     float r, g, b, a;
     float u, v, q;

       Then I add:
    */
    DWORD clipFlag;
} TwoDVertex;


class Render2D : public VirtualDisplay
{
public:
    Render2D();
    virtual ~Render2D();
private:
    void IntersectTop(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v);
    void IntersectBottom(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v);
    void IntersectLeft(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v);
    void IntersectRight(TwoDVertex *v1, TwoDVertex *v2, TwoDVertex *v);
    float OffsetX, OffsetY;
public:
    virtual void Setup(ImageBuffer *imageBuffer);
    virtual void Cleanup(void);

    virtual void SetImageBuffer(ImageBuffer *imageBuffer);
    ImageBuffer* GetImageBuffer(void);

    virtual void StartDraw(void);
    virtual void ClearDraw(void);
    virtual void ClearZBuffer(void);
    virtual void EndDraw(void);

    virtual void SetViewport(float leftSide, float topSide, float rightSide, float bottomSide);

    //JAM 22Dec03 - These should not be here.
    virtual DWORD Color(void);
    virtual void SetColor(DWORD packedRGBA);
    virtual void SetBackground(DWORD packedRGBA);

    void Render2DPoint(float x1, float y1);
    void Render2DLine(float x1, float y1, float x2, float y2);
    void Render2DTri(float x1, float y1, float x2, float y2, float x3, float y3);
    void Render2DBitmap(int srcX, int srcY, int dstX, int dstY, int w, int h, int sourceWidth, DWORD *source, bool Fit = false);
    void Render2DBitmap(int srcX, int srcY, int dstX, int dstY, int w, int h, char *filename, bool Fit = false);
    void ScreenText(float x, float y, const char *string, int boxed = 0);
    void SetOffset(float x, float y);

    virtual void SetLineStyle(int);

    // Draw a fan with clipping (must set clip flags first)
    void SetClipFlags(TwoDVertex* vert);
    void ClipAndDraw2DFan(TwoDVertex** vertPointers, unsigned count, bool gifPicture = false);

    //JAM 22Dec03
    // ASFO:
    static void Load2DFontSet();
    static void Load3DFontSet();
    static void Release2DFontSet();
    static void Release3DFontSet();
    static void ChangeFontSet(FontSet* pFontSet_);

public:
    // Window and rendering context handles
    //ContextMPR context; // ASSO: moved to display.h so the RTT works correctly

protected:
    // ImageBuffer *image; // ASSO: moved to display.h so the RTT works correctly
};


#endif // _RENDER2D_H_
