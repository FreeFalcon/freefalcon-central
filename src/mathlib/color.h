/*
color.h

 Author: Miro "Jammer" Torrielli
 Last Update: 21 April 2004
*/

#ifndef _COLOR_H
#define _COLOR_H

#include "math.h"

#define MAKERGB(r,g,b) ((r<<16)+(g<<8)+b)
#define MAKEARGB(a,r,g,b) ((a<<24)+(r<<16)+(g<<8)+b)

#define SMALL_NUMBER (1.e-8)
#define KINDA_SMALL_NUMBER (1.e-4)

struct color
{
    color()
    {
        r = g = b = h = s = v = X = Y = Z = x = y = L = 0.f;
    }

    color(float _r, float _g, float _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }

    void SetRGB(float _r, float _g, float _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }

    void SetHSV(float _h, float _s, float _v)
    {
        h = _h;
        s = _s;
        v = _v;
    }

    void SetXYZ(float _x, float _y, float _z)
    {
        X = _x;
        Y = _y;
        Z = _z;
    }

    void SetxyL(float _x, float _y, float _L)
    {
        x = _x;
        y = _y;
        L = _L;
    }

    void XYZtoRGB()
    {
        //CIE XYZ tristimulus to Rec. 709 (D65 Whitepoint) RGB
        r =  3.240479f * X - 1.537150f * Y - 0.498535f * Z;
        g = -0.969256f * X + 1.875991f * Y + 0.041556f * Z;
        b =  0.055648f * X - 0.204043f * Y + 1.057311f * Z;
    }

    void RGBtoXYZ()
    {
        X =  0.412453f * r + 0.357580f * g + 0.180423f * b;
        Y =  0.212671f * r + 0.715160f * g + 0.072169f * b;
        Z =  0.019334f * r + 0.119193f * g + 0.950227f * b;
    }

    void RGBtoHSV()
    {
        float max = MAXIMUM(r, g, b);
        float min = MINIMUM(r, g, b);
        v = max;
        s = (v != 0.f) ? ((max - min) / max) : 0.f;

        if (s == 0.f) h = 0.f;
        else
        {
            float delta = max - min;

            if (r == max)
                h = (g - b) / delta;
            else if (g == max)
                h = 2.f + (b - r) / delta;
            else if (b == max)
                h = 4.f + (r - g) / delta;

            h *= 60;

            if (h < 0.f) h += 360.f;

            h /= 360.f;
        }
    }

    void HSVtoRGB()
    {
        if (s == 0.f || h == -1.f)
        {
            r = g = b = v;
            return;
        }

        h *= 360.f;
        h /= 60.f;
        int i = (int)floorf(h);
        float f = h - i;
        float p = v * (1 - s);
        float q = v * (1 - s * f);
        float t = v * (1 - s * (1.f - f));

        switch (i)
        {
            case 0:
                r = v;
                g = t;
                b = p;
                break;

            case 1:
                r = q;
                g = v;
                b = p;
                break;

            case 2:
                r = p;
                g = v;
                b = t;
                break;

            case 3:
                r = p;
                g = q;
                b = v;
                break;

            case 4:
                r = t;
                g = p;
                b = v;
                break;

            default:
                r = v;
                g = p;
                b = q;
        }

    }

    void xyLtoXYZ()
    {
        X = x * (L / y);
        Y = L;
        Z = (1.f - x - y) * (Y / y);
    }

    void xyLtoRGB()
    {
        xyLtoXYZ();
        XYZtoRGB();
    }

    void XYZtoHSV()
    {
        XYZtoRGB();
        RGBtoHSV();
    }

    void GammaCorrectRGB(float R, float G, float B)
    {
        r = powf(r, 1.f / R);
        g = powf(g, 1.f / G);
        b = powf(b, 1.f / B);
    }

    void ExposureRGB(float exposure)
    {
        r = 1.f - expf(r * exposure);
        g = 1.f - expf(g * exposure);
        b = 1.f - expf(b * exposure);
    }

    void ExposureV(float exposure)
    {
        v = 1.f - expf(v * exposure);
    }

    void ExposureL(float exposure)
    {
        L = 1.f - expf(L * exposure);
    }

    void ClampRGB()
    {
        r = Clamp(r, 0.f, 1.f);
        g = Clamp(g, 0.f, 1.f);
        b = Clamp(b, 0.f, 1.f);
    }

    DWORD MakeARGB()
    {
        BYTE A = 0xFF;
        BYTE R = (BYTE)FloatToInt32(r * 255.9f);
        BYTE G = (BYTE)FloatToInt32(g * 255.9f);
        BYTE B = (BYTE)FloatToInt32(b * 255.9f);

        return MAKEARGB(A, R, G, B);
    }

    bool Normalize()
    {
        float squareSum = r * r + g * g + b * b;

        if (squareSum >= SMALL_NUMBER)
        {
            float scale = 1.f / (float)sqrt(squareSum);
            r *= scale;
            g *= scale;
            b *= scale;
            return 1;
        }
        else return 0;
    }

    friend color operator *(float Scale, const color& V)
    {
        return color(V.r * Scale, V.g * Scale, V.b * Scale);
    }

    color operator +(const color& V) const
    {
        return color(r + V.r, g + V.g, b + V.b);
    }

    color operator -(const color& V) const
    {
        return color(r - V.r, g - V.g, b - V.b);
    }

    color operator *(float Scale) const
    {
        return color(r * Scale, g * Scale, b * Scale);
    }

    color operator /(float Scale) const
    {
        float RScale = 1.f / Scale;
        return color(r * RScale, g * RScale, b * RScale);
    }

    color operator *(const color& V) const
    {
        return color(r * V.r, g * V.g, b * V.b);
    }

    color operator /(const color& V) const
    {
        return color(r / V.r, g / V.g, b / V.b);
    }

    bool operator ==(const color& V) const
    {
        return r == V.r && g == V.g && b == V.b;
    }

    bool operator !=(const color& V) const
    {
        return r != V.r || g != V.g || b != V.b;
    }

    color operator -() const
    {
        return color(-r, -g, -b);
    }

    color operator +=(const color& V)
    {
        r += V.r;
        g += V.g;
        b += V.b;
        return *this;
    }

    color operator -=(const color& V)
    {
        r -= V.r;
        g -= V.g;
        b -= V.b;
        return *this;
    }

    color operator *=(float Scale)
    {
        r *= Scale;
        g *= Scale;
        b *= Scale;
        return *this;
    }

    color operator /=(float V)
    {
        float RV = 1.f / V;
        r *= RV;
        g *= RV;
        b *= RV;
        return *this;
    }

    color operator *=(const color& V)
    {
        r *= V.r;
        g *= V.g;
        b *= V.b;
        return *this;
    }

    color operator /=(const color& V)
    {
        r /= V.r;
        g /= V.g;
        b /= V.b;
        return *this;
    }

    float r, g, b;
    float h, s, v;
    float X, Y, Z;
    float x, y, L;
};

#endif
