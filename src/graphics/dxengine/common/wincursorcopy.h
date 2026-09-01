// wincursorcopy.h -- Artscout - 2026: the live Windows cursor as RGBA pixels.
// The OS never composites its cursor into a headset, so VR panels have to draw
// their own; this hands them the real desktop shape instead of a crosshair.
#ifndef _WINCURSORCOPY_H_
#define _WINCURSORCOPY_H_

#ifdef _WIN32

#include <windows.h>
#include <vector>

// Returns the current cursor as top-down RGBA (alpha premultiplied by nothing),
// plus its hotspot. The result is cached and only rebuilt when the OS cursor
// changes shape. False = no cursor available; the caller keeps its fallback.
inline bool FF_WinCursorRGBA(const unsigned char** outPixels, int* outW,
                             int* outH, int* outHotX, int* outHotY)
{
    static std::vector<unsigned char> s_pixels;
    static HCURSOR s_src = NULL;
    static int s_w = 0, s_h = 0, s_hotX = 0, s_hotY = 0;

    CURSORINFO ci;
    ci.cbSize = sizeof(ci);

    if (!GetCursorInfo(&ci) || ci.hCursor == NULL)
        return false;

    if (ci.hCursor != s_src || s_pixels.empty())
    {
        ICONINFO ii;
        ZeroMemory(&ii, sizeof(ii));

        if (!GetIconInfo(ci.hCursor, &ii))
            return false;

        BITMAP bm;
        const HBITMAP shape = ii.hbmColor ? ii.hbmColor : ii.hbmMask;
        bool built = false;

        if (GetObject(shape, sizeof(bm), &bm) && bm.bmWidth > 0)
        {
            // A monochrome cursor packs AND over XOR in one double-height mask.
            const int w = bm.bmWidth;
            const int h = ii.hbmColor ? bm.bmHeight : (bm.bmHeight / 2);
            const int maskRows = bm.bmHeight;

            std::vector<DWORD> color((size_t)w * h, 0);
            std::vector<DWORD> mask((size_t)w * maskRows, 0);

            BITMAPINFO bi;
            ZeroMemory(&bi, sizeof(bi));
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = w;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;

            HDC dc = GetDC(NULL);

            if (dc)
            {
                if (ii.hbmColor)
                {
                    bi.bmiHeader.biHeight = -h; // negative = top-down rows
                    GetDIBits(dc, ii.hbmColor, 0, h, &color[0], &bi,
                              DIB_RGB_COLORS);
                }

                bi.bmiHeader.biHeight = -maskRows;
                GetDIBits(dc, ii.hbmMask, 0, maskRows, &mask[0], &bi,
                          DIB_RGB_COLORS);
                ReleaseDC(NULL, dc);

                // Modern cursors carry alpha; legacy ones leave it zero and the
                // AND mask decides (bit set = transparent).
                bool hasAlpha = false;

                if (ii.hbmColor)
                {
                    for (size_t k = 0; k < color.size(); ++k)
                    {
                        if ((color[k] & 0xFF000000u) != 0)
                        {
                            hasAlpha = true;
                            break;
                        }
                    }
                }

                s_pixels.assign((size_t)w * h * 4, 0);

                for (int r = 0; r < h; ++r)
                {
                    for (int c = 0; c < w; ++c)
                    {
                        const size_t k = (size_t)r * w + c;
                        const bool andSet = (mask[k] & 0x00FFFFFFu) != 0;
                        DWORD px;

                        if (ii.hbmColor)
                        {
                            px = color[k];

                            if (!hasAlpha)
                                px = andSet ? 0u : (px | 0xFF000000u);
                        }
                        else
                        {
                            // XOR half carries the colour, AND the coverage.
                            const DWORD xorPx =
                                mask[(size_t)(r + h) * w + c] & 0x00FFFFFFu;
                            px = andSet ? 0u : (xorPx | 0xFF000000u);
                        }

                        // GetDIBits gives BGRA; the panels want RGBA.
                        unsigned char* o = &s_pixels[k * 4];
                        o[0] = (unsigned char)((px >> 16) & 0xFF);
                        o[1] = (unsigned char)((px >> 8) & 0xFF);
                        o[2] = (unsigned char)(px & 0xFF);
                        o[3] = (unsigned char)((px >> 24) & 0xFF);
                    }
                }

                s_w = w;
                s_h = h;
                s_hotX = (int)ii.xHotspot;
                s_hotY = (int)ii.yHotspot;
                s_src = ci.hCursor;
                built = true;
            }
        }

        if (ii.hbmColor)
            DeleteObject(ii.hbmColor);

        if (ii.hbmMask)
            DeleteObject(ii.hbmMask);

        if (!built)
            return false;
    }

    if (s_pixels.empty() || s_w <= 0 || s_h <= 0)
        return false;

    if (outPixels)
        *outPixels = &s_pixels[0];
    if (outW)
        *outW = s_w;
    if (outH)
        *outH = s_h;
    if (outHotX)
        *outHotX = s_hotX;
    if (outHotY)
        *outHotY = s_hotY;

    return true;
}

// Alpha-blends the cursor into a top-down RGBA buffer at (cx,cy), hotspot
// applied. rowPitch <= 0 means tightly packed. False = no copy available.
inline bool FF_BlitWinCursorRGBA(unsigned char* dst, int dstW, int dstH, int cx,
                                 int cy, int rowPitch = 0)
{
    const unsigned char* px = 0;
    int w = 0, h = 0, hotX = 0, hotY = 0;

    if (!dst || dstW <= 0 || dstH <= 0)
        return false;

    const size_t pitch =
        (rowPitch > 0) ? (size_t)rowPitch : ((size_t)dstW * 4);

    if (!FF_WinCursorRGBA(&px, &w, &h, &hotX, &hotY))
        return false;

    const int x0 = cx - hotX, y0 = cy - hotY;

    for (int r = 0; r < h; ++r)
    {
        const int y = y0 + r;

        if (y < 0 || y >= dstH)
            continue;

        for (int c = 0; c < w; ++c)
        {
            const int x = x0 + c;

            if (x < 0 || x >= dstW)
                continue;

            const unsigned char* s = px + ((size_t)r * w + c) * 4;
            const unsigned a = s[3];

            if (!a)
                continue;

            unsigned char* o = dst + (size_t)y * pitch + (size_t)x * 4;
            o[0] = (unsigned char)((s[0] * a + o[0] * (255 - a)) / 255);
            o[1] = (unsigned char)((s[1] * a + o[1] * (255 - a)) / 255);
            o[2] = (unsigned char)((s[2] * a + o[2] * (255 - a)) / 255);
            o[3] = 255;
        }
    }

    return true;
}

#endif // _WIN32
#endif // _WINCURSORCOPY_H_
