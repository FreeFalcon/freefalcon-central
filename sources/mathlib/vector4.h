/*
vector4.h

	Author: Miro "Jammer" Torrielli
	Last Update: 21 April 2004 
*/

#ifndef _VECTOR4_H
#define _VECTOR4_H

#include "math.h"
#include <float.h>
#include "vector3.h"

struct vector4 
{
    enum component
    {
        X = (1<<0),
        Y = (1<<1),
        Z = (1<<2),
        W = (1<<3),
    };

	vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
	
	vector4(const float _x, const float _y, const float _z, const float _w) : x(_x), y(_y), z(_z), w(_w) { }
	
	vector4(const vector4& v) :	x(v.x), y(v.y), z(v.z), w(v.w) { }
	
	vector4(const vector3& v) : x(v.x), y(v.y), z(v.z), w(1.0f) { }
	
	void Set(const float _x, const float _y, const float _z, const float _w)
	{
	    x = _x;
	    y = _y;
	    z = _z;
	    w = _w;
	}
	
	void Set(const vector4& v)
	{
	    x = v.x;
	    y = v.y;
	    z = v.z;
	    w = v.w;
	}
	
	void Set(const vector3& v)
	{
	    x = v.x;
	    y = v.y;
	    z = v.z;
	    w = 1.0f;
	}
	
	float Length() const
	{
	    return sqrt(x*x + y*y + z*z + w*w);
	}
	
	void Normalize()
	{
	    float l = Length();
	    if(l > TINY) 
	    {
	        float oneDivL = 1.0f / l;
	        x *= oneDivL;
	        y *= oneDivL;
	        z *= oneDivL;
	        w *= oneDivL;
	    }
	}
	
	friend vector4 operator +(const vector4& v0, const vector4& v1) 
	{
	    return vector4(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w);
	}
	
	friend vector4 operator -(const vector4& v0, const vector4& v1) 
	{
	    return vector4(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w);
	}
	
	friend vector4 operator *(const vector4& v0, const float& s) 
	{
	    return vector4(v0.x * s, v0.y * s, v0.z * s, v0.w * s);
	}
	
	friend vector4 operator -(const vector4& v)
	{
	    return vector4(-v.x, -v.y, -v.z, -v.w);
	}
	
	void operator +=(const vector4& v)
	{
	    x += v.x; 
	    y += v.y; 
	    z += v.z; 
	    w += v.w;
	}
	
	void operator -=(const vector4& v)
	{
	    x -= v.x; 
	    y -= v.y; 
	    z -= v.z; 
	    w -= v.w;
	}
	
	void operator *=(const float s)
	{
	    x *= s; 
	    y *= s; 
	    z *= s; 
	    w *= s;
	}
	
	vector4& operator=(const vector3& v)
	{
	    this->Set(v);
	    return *this;
	}
	
	bool IsEqual(const vector4& v, float tol) const
	{
	    if(fabs(v.x - x) > tol)      return false;
	    else if(fabs(v.y - y) > tol) return false;
	    else if(fabs(v.z - z) > tol) return false;
	    else if(fabs(v.w - w) > tol) return false;
	    return true;
	}
	
	int Compare(const vector4& v, float tol) const
	{
	    if(fabs(v.x - x) > tol)      return (v.x > x) ? +1 : -1; 
	    else if(fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
	    else if(fabs(v.z - z) > tol) return (v.z > z) ? +1 : -1;
	    else if(fabs(v.w - w) > tol) return (v.w > w) ? +1 : -1;
	    else                         return 0;
	}
	
	void Minimum(const vector4& v)
	{
	    if(v.x < x) x = v.x;
	    if(v.y < y) y = v.y;
	    if(v.z < z) z = v.z;
	    if(v.w < w) w = v.w;
	}
	
	void Maximum(const vector4& v)
	{
	    if(v.x > x) x = v.x;
	    if(v.y > y) y = v.y;
	    if(v.z > z) z = v.z;
	    if(v.w > w) w = v.w;
	}
	
	void SetComp(float val, int mask)
	{
	    if(mask & X) x = val;
	    if(mask & Y) y = val;
	    if(mask & Z) z = val;
	    if(mask & W) w = val;
	}
	
	float GetComp(int mask)
	{
	    switch (mask)
	    {
	        case X:  return x;
	        case Y:  return y;
	        case Z:  return z;
	        default: return w;
	    }
	}
	
	int MinCompMask() const
	{
	    float minVal = x;
	    int minComp = X;

		if(y < minVal)
	    {
	        minComp = Y;
	        minVal  = y;
	    }
	    if(z < minVal) 
	    {
	        minComp = Z;
	        minVal  = z;
	    }
	    if(w < minVal) 
	    {
	        minComp = W;
	        minVal  = w;
	    }

	    return minComp;
	}
	
	void Lerp(const vector4& v0, float lerpVal)
	{
	    x = v0.x + ((x - v0.x) * lerpVal);
	    y = v0.y + ((y - v0.y) * lerpVal);
	    z = v0.z + ((z - v0.z) * lerpVal);
	    w = v0.w + ((w - v0.w) * lerpVal);
	}
	
	void Lerp(const vector4& v0, const vector4& v1, float lerpVal)
	{
	    x = v0.x + ((v1.x - v0.x) * lerpVal);
	    y = v0.y + ((v1.y - v0.y) * lerpVal);
	    z = v0.z + ((v1.z - v0.z) * lerpVal);
	    w = v0.w + ((v1.w - v0.w) * lerpVal);
	}
	
	
	void Saturate()
	{
	    x = Clamp(x);
	    y = Clamp(y);
	    z = Clamp(z);
	    w = Clamp(w);
	}
	
    float x, y, z, w;
};

#endif

