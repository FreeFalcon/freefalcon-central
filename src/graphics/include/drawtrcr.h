/***************************************************************************\
    DrawTrcr.h
    Derived class to handle drawing tracers (bullets)
\***************************************************************************/
#ifndef _DRAWTRCR_H_
#define _DRAWTRCR_H_

#include "drawobj.h"
#define TRACER_VISIBLE_DISTANCE 20000.0f


class DrawableTracer : public DrawableObject
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(DrawableTracer));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem)
            MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(DrawableTracer), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
public:
    DrawableTracer(void);
    DrawableTracer(float w);
    DrawableTracer(Tpoint *p, float w);
    virtual ~DrawableTracer();

    virtual void Update(Tpoint *head, Tpoint *tail);

    virtual void Draw(class RenderOTW *renderer, int LOD);

    virtual void SetAlpha(float a)
    {
        alpha = a;
    };
    virtual float GetAlpha(void)
    {
        return alpha;
    };
    virtual void GetRGB(float *R, float *G, float *B)
    {
        *R = r;
        *G = g;
        *B = b;
    };
    virtual void SetWidth(float w)
    {
        width = w;
    };
    virtual void SetType(int t)
    {
        type = t;
    };
    virtual void SetRGB(float R, float G, float B)
    {
        r = R;
        g = G;
        b = B;
    };

protected:
    BOOL ConstructWidth(RenderOTW *renderer, Tpoint *start, Tpoint *end,
                        struct ThreeDVertex *xformLeft,
                        struct ThreeDVertex *xformRight,
                        struct ThreeDVertex *xformLefte,
                        struct ThreeDVertex *xformRighte);
    Tpoint tailEnd;
    float width;
    float alpha;
    float r, g, b;
    int type;
    Tpoint LastPos;
    // Artscout - 2026: wall-clock (ms) of the last time this tracer actually moved. The "frozen
    // tracer" cull used to fire on the FIRST render frame where the position hadn't changed since
    // the previous draw. Tracers are driven by the gun Exec (sim rate); at high render FPS many
    // frames elapse between sim ticks, so live tracers were culled almost immediately -> the stream
    // looked sparse/faint in Release but fine in (slow) Debug. We now only cull after the position
    // has been stale for STALE_MS of real time, and keep drawing it meanwhile.
    DWORD lastMoveMs;
#define TRACER_TYPE_TRACER 0
#define TRACER_TYPE_BALL 1
};


//////////////////////////////// THE DX VERSION OF THE DRAWABLE TRACER \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

class DXDrawableTracer : public DrawableTracer
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(DXDrawableTracer));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem)
            MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(DXDrawableTracer), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
public:
    DXDrawableTracer(void);
    DXDrawableTracer(float w);
    DXDrawableTracer(Tpoint *p, float w);
    virtual ~DXDrawableTracer();

    void Update(Tpoint *head, Tpoint *tail);

    virtual void Draw(class RenderOTW *renderer, int LOD);

    void SetAlpha(float a)
    {
        alpha = a;
    };
    float GetAlpha(void)
    {
        return alpha;
    };
    void GetRGB(float *R, float *G, float *B)
    {
        *R = r;
        *G = g;
        *B = b;
    };
    void SetWidth(float w)
    {
        width = w;
    };
    void SetType(int t)
    {
        type = t;
    };
    void SetRGB(float R, float G, float B)
    {
        r = R;
        g = G;
        b = B;
    };

protected:
    BOOL ConstructWidth(RenderOTW *renderer, Tpoint *start, Tpoint *end,
                        struct ThreeDVertex *xformLeft,
                        struct ThreeDVertex *xformRight,
                        struct ThreeDVertex *xformLefte,
                        struct ThreeDVertex *xformRighte);
    Tpoint tailEnd;
    float width;
    float alpha;
    float r, g, b;
    Tpoint LastPos;
    int type;
#define TRACER_TYPE_TRACER 0
#define TRACER_TYPE_BALL 1
};

////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#endif // _DRAWTRCR_H_
