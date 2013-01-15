/*
vector2.h

	Author: Miro "Jammer" Torrielli
	Last Update: 21 April 2004 
*/

#ifndef _VECTOR2_H
#define _VECTOR2_H

#include "math.h"
#include <float.h>

struct vector2
{
    vector2();
    vector2(const float _x, const float _y);
    vector2(const vector2& vec);
    vector2(const float* p);
    void set(const float _x, const float _y);
    void set(const vector2& vec);
    void set(const float* p);
    float len() const;
    void norm();
    void operator+=(const vector2& v0);
    void operator-=(const vector2& v0);
    void operator*=(const float s);
    void operator/=(const float s);
    // fuzzy compare operator
    bool isequal(const vector2& v, const float tol) const;
    // fuzzy compare, returns -1, 0, +1
    int compare(const vector2& v, float tol) const;

    float x, y;
};

__forceinline vector2::vector2() : x(0.0f), y(0.0f) {}

__forceinline vector2::vector2(const float _x, const float _y) : x(_x), y(_y) {}

__forceinline vector2::vector2(const vector2& vec) : x(vec.x), y(vec.y) {}

__forceinline vector2::vector2(const float* p) : x(p[0]), y(p[1]) {}

__forceinline void vector2::set(const float _x, const float _y)
{
    x = _x;
    y = _y;
}

__forceinline void vector2::set(const vector2& v)
{
    x = v.x;
    y = v.y;
}

__forceinline void vector2::set(const float* p)
{
    x = p[0];
    y = p[1];
}

__forceinline float vector2::len() const
{
    return (float) sqrt(x * x + y * y);
}

__forceinline void vector2::norm()
{
    float l = len();
    if (l > TINY)
    {
        x /= l;
        y /= l;
    }
}

__forceinline void vector2::operator +=(const vector2& v0) 
{
    x += v0.x;
    y += v0.y;
}

__forceinline void vector2::operator -=(const vector2& v0) 
{
    x -= v0.x;
    y -= v0.y;
}

__forceinline void vector2::operator *=(const float s) 
{
    x *= s;
    y *= s;
}

__forceinline void vector2::operator /=(const float s) 
{
    x /= s;
    y /= s;
}

__forceinline bool vector2::isequal(const vector2& v, const float tol) const
{
    if (fabs(v.x - x) > tol)      return false;
    else if (fabs(v.y - y) > tol) return false;
    return true;
}

__forceinline int vector2::compare(const vector2& v, float tol) const
{
    if (fabs(v.x - x) > tol)      return (v.x > x) ? +1 : -1; 
    else if (fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
    else                          return 0;
}

static __forceinline vector2 operator +(const vector2& v0, const vector2& v1) 
{
    return vector2(v0.x + v1.x, v0.y + v1.y); 
}

static __forceinline vector2 operator -(const vector2& v0, const vector2& v1) 
{
    return vector2(v0.x - v1.x, v0.y - v1.y);
}

static __forceinline vector2 operator *(const vector2& v0, const float s) 
{
    return vector2(v0.x * s, v0.y * s);
}

static __forceinline vector2 operator -(const vector2& v) 
{
    return vector2(-v.x, -v.y);
}

#endif

