/*
vector3.h

	Author: Miro "Jammer" Torrielli
	Last Update: 21 April 2004 
*/

#ifndef _VECTOR3_H
#define _VECTOR3_H

//#include "math.h"
#include <math.h>
#include <float.h>
#undef  PI
#define PI (3.1415926535897932384626433832795028841971693993751f)
#define PI_FRAC (PI / 180.0)
#define TINY (0.0000001)
/*
static inline float Clamp(float val)
{
    if(val < 0.f) return 0.f;
    else if(val > 1.f) return 1.f;
    else return val;
}
*/
// Base vector3 with empty default constructor
struct vector3
{
	vector3() { x = y = z = 0.f; };

	vector3(const float _x, const float _y, const float _z) : x(_x), y(_y), z(_z) { }

	vector3(const vector3& v) : x(v.x), y(v.y), z(v.z) { }

	void Set(const float _x, const float _y, const float _z)
	{
	    x = _x;
	    y = _y;
	    z = _z;
	}

	void Set(const vector3& vec)
	{
	    x = vec.x;
	    y = vec.y;
	    z = vec.z;
	}

	float Length() const
	{
	    return sqrt(x*x + y*y + z*z);
	}

	float LengthSquared() const
	{
	    return x * x + y * y + z * z;
	}

	void Normalize()
	{
	    float l = Length();
	    if (l > TINY) 
	    {
	        x /= l;
	        y /= l;
	        z /= l;
	    }
	}

	friend vector3 operator +(const vector3& v0, const vector3& v1) 
	{
	    return vector3(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
	}

	friend vector3 operator -(const vector3& v0, const vector3& v1) 
	{
	    return vector3(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
	}

	friend vector3 operator *(const vector3& v0, const float s) 
	{
	    return vector3(v0.x * s, v0.y * s, v0.z * s);
	}

	friend vector3 operator *(const float s, const vector3& v0) 
	{
	    return vector3(v0.x * s, v0.y * s, v0.z * s);
	}

	friend vector3 operator /(const vector3& v0, const float s)
	{
	    float one_over_s = 1.f/s;
	    return vector3(v0.x*one_over_s, v0.y*one_over_s, v0.z*one_over_s);
	}

	friend vector3 operator -(const vector3& v) 
	{
	    return vector3(-v.x, -v.y, -v.z);
	}

	// Dot product.
	friend float operator %(const vector3& v0, const vector3& v1)
	{
	    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
	}

	// Cross product.
	friend vector3 operator *(const vector3& v0, const vector3& v1) 
	{
	    return vector3(v0.y * v1.z - v0.z * v1.y,
						v0.z * v1.x - v0.x * v1.z,
	                    v0.x * v1.y - v0.y * v1.x);
	}

	void operator +=(const vector3& v0)
	{
	    x += v0.x;
	    y += v0.y;
	    z += v0.z;
	}

	void operator -=(const vector3& v0)
	{
	    x -= v0.x;
	    y -= v0.y;
	    z -= v0.z;
	}

	void operator *=(float s)
	{
	    x *= s;
	    y *= s;
	    z *= s;
	}

	void operator /=(float s)
	{
	    x /= s;
	    y /= s;
	    z /= s;
	}

	bool IsEqual(const vector3& v, float tol) const
	{
	    if (fabs(v.x - x) > tol)      return false;
	    else if (fabs(v.y - y) > tol) return false;
	    else if (fabs(v.z - z) > tol) return false;
	    return true;
	}

	int Compare(const vector3& v, float tol) const
	{
	    if (fabs(v.x - x) > tol)      return (v.x > x) ? +1 : -1; 
	    else if (fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
	    else if (fabs(v.z - z) > tol) return (v.z > z) ? +1 : -1;
	    else                          return 0;
	}

	void Rotate(const vector3& axis, float angle)
	{
	    // rotates this one around given vector. We do
	    // rotation with matrices, but these aren't defined yet!
	    float rotM[9];
	    float sa, ca;
	
	    sa = sinf(angle);
	    ca = cosf(angle);
	
	    // build a rotation matrix
	    rotM[0] = ca + (1 - ca) * axis.x * axis.x;
	    rotM[1] = (1 - ca) * axis.x * axis.y - sa * axis.z;
	    rotM[2] = (1 - ca) * axis.z * axis.x + sa * axis.y;
	    rotM[3] = (1 - ca) * axis.x * axis.y + sa * axis.z;
	    rotM[4] = ca + (1 - ca) * axis.y * axis.y;
	    rotM[5] = (1 - ca) * axis.y * axis.z - sa * axis.x;
	    rotM[6] = (1 - ca) * axis.z * axis.x - sa * axis.y;
	    rotM[7] = (1 - ca) * axis.y * axis.z + sa * axis.x;
	    rotM[8] = ca + (1 - ca) * axis.z * axis.z;
	
	    // "handmade" multiplication
	    vector3 help(rotM[0] * this->x + rotM[1] * this->y + rotM[2] * this->z,
	                  rotM[3] * this->x + rotM[4] * this->y + rotM[5] * this->z,
	                  rotM[6] * this->x + rotM[7] * this->y + rotM[8] * this->z);
	    *this = help;
	}

	void Lerp(const vector3& v0, float lerpVal)
	{
	    x = v0.x + ((x - v0.x) * lerpVal);
	    y = v0.y + ((y - v0.y) * lerpVal);
	    z = v0.z + ((z - v0.z) * lerpVal);
	}

	void Lerp(const vector3& v0, const vector3& v1, float lerpVal)
	{
	    x = v0.x + ((v1.x - v0.x) * lerpVal);
	    y = v0.y + ((v1.y - v0.y) * lerpVal);
	    z = v0.z + ((v1.z - v0.z) * lerpVal);
	}

	void Saturate()
	{
	    x = Clamp(x);
	    y = Clamp(y);
	    z = Clamp(z);
	}

	// Find a vector that is orthogonal to self. Self should not be (0,0,0). Return value is not normalized.
	vector3 FindOrtho() const
	{
	    if(0.f != x)
	        return vector3((-y - z)/x,1.f,1.f);
	    else if(0.f != y)
	        return vector3(1.f,(-x - z)/y,1.f);
	    else if(0.f != z)
	        return vector3(1.f,1.f,(-x - y)/z);
	    else
	        return vector3(0.f,0.f,0.f);
	}

	float x,y,z;
};

#endif
