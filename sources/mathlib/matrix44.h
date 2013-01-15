/*
matrix44.h

	Author: Miro "Jammer" Torrielli
	Last Update: 21 April 2004 
*/

#ifndef _MATRIX44_H
#define _MATRIX44_H

#include <memory.h>
#include "vector4.h"
#include "vector3.h"
#include "quaternion.h"
#include "euler.h"
#include "matrixdefs.h"

static float matrix44_ident[16] = 
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

struct matrix44 
{
	matrix44()
	{
	    memcpy(&(m[0][0]), matrix44_ident, sizeof(matrix44_ident));
	}
	
	matrix44(const vector4& v0, const vector4& v1, const vector4& v2, const vector4& v3)
	{
	    m11 = v0.x; m12 = v0.y; m13 = v0.z; m14 = v0.w;
	    m21 = v1.x; m22 = v1.y; m23 = v1.z; m24 = v1.w;
	    m31 = v2.x; m32 = v2.y; m33 = v2.z; m34 = v2.w;
	    m41 = v3.x; m42 = v3.y; m43 = v3.z; m44 = v3.w;
	}
	
	matrix44(const matrix44& m1) 
	{
	    memcpy(m, &(m1.m[0][0]), 16 * sizeof(float));
	}
	
	matrix44(float _m11, float _m12, float _m13, float _m14,
	                     float _m21, float _m22, float _m23, float _m24,
	                     float _m31, float _m32, float _m33, float _m34,
	                     float _m41, float _m42, float _m43, float _m44)
	{
	    m11 = _m11; m12 = _m12; m13 = _m13; m14 = _m14;
	    m21 = _m21; m22 = _m22; m23 = _m23; m24 = _m24;
	    m31 = _m31; m32 = _m32; m33 = _m33; m34 = _m34;
	    m41 = _m41; m42 = _m42; m43 = _m43; m44 = _m44;
	}
	
	matrix44(const quaternion& q) 
	{
	    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
	    x2 = q.x + q.x; y2 = q.y + q.y; z2 = q.z + q.z;
	    xx = q.x * x2;   xy = q.x * y2;   xz = q.x * z2;
	    yy = q.y * y2;   yz = q.y * z2;   zz = q.z * z2;
	    wx = q.w * x2;   wy = q.w * y2;   wz = q.w * z2;
	
	    m[0][0] = 1.0f - (yy + zz);
	    m[1][0] = xy - wz;
	    m[2][0] = xz + wy;
	
	    m[0][1] = xy + wz;
	    m[1][1] = 1.0f - (xx + zz);
	    m[2][1] = yz - wx;
	
	    m[0][2] = xz - wy;
	    m[1][2] = yz + wx;
	    m[2][2] = 1.0f - (xx + yy);
	
	    m[3][0] = m[3][1] = m[3][2] = 0.0f;
	    m[0][3] = m[1][3] = m[2][3] = 0.0f;
	    m[3][3] = 1.0f;
	}
	
	// Convert orientation of 4x4 matrix into quaterion, 4x4 matrix must not be scaled!
	quaternion GetQuaternion() const
	{
	    float qa[4];
	    float tr = m[0][0] + m[1][1] + m[2][2];
	    if (tr > 0.0f) 
	    {
	        float s = sqrt (tr + 1.0f);
	        qa[3] = s * 0.5f;
	        s = 0.5f / s;
	        qa[0] = (m[1][2] - m[2][1]) * s;
	        qa[1] = (m[2][0] - m[0][2]) * s;
	        qa[2] = (m[0][1] - m[1][0]) * s;
	    } 
	    else 
	    {
	        int i, j, k, nxt[3] = {1,2,0};
	        i = 0;
	        if (m[1][1] > m[0][0]) i=1;
	        if (m[2][2] > m[i][i]) i=2;
	        j = nxt[i];
	        k = nxt[j];
	        float s = sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
	        qa[i] = s * 0.5f;
	        s = 0.5f / s;
	        qa[3] = (m[j][k] - m[k][j])* s;
	        qa[j] = (m[i][j] + m[j][i]) * s;
	        qa[k] = (m[i][k] + m[k][i]) * s;
	    }

	    quaternion q(qa[0],qa[1],qa[2],qa[3]);
	    return q;
	}
	
	void Set(const vector4& v0, const vector4& v1, const vector4& v2, const vector4& v3) 
	{
	    m11=v0.x; m12=v0.y; m13=v0.z, m14=v0.w;
	    m21=v1.x; m22=v1.y; m23=v1.z; m24=v1.w;
	    m31=v2.x; m32=v2.y; m33=v2.z; m34=v2.w;
	    m41=v3.x; m42=v3.y; m43=v3.z; m44=v3.w;
	}
	
	void Set(const matrix44& m1) 
	{
	    memcpy(m, &(m1.m[0][0]), 16*sizeof(float));
	}
	
	void Set(float _m11, float _m12, float _m13, float _m14,
	               float _m21, float _m22, float _m23, float _m24,
	               float _m31, float _m32, float _m33, float _m34,
	               float _m41, float _m42, float _m43, float _m44)
	{
	    m11=_m11; m12=_m12; m13=_m13; m14=_m14;
	    m21=_m21; m22=_m22; m23=_m23; m24=_m24;
	    m31=_m31; m32=_m32; m33=_m33; m34=_m34;
	    m41=_m41; m42=_m42; m43=_m43; m44=_m44;
	}
	
	void Set(const quaternion& q) 
	{
	    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
	    x2 = q.x + q.x; y2 = q.y + q.y; z2 = q.z + q.z;
	    xx = q.x * x2;   xy = q.x * y2;   xz = q.x * z2;
	    yy = q.y * y2;   yz = q.y * z2;   zz = q.z * z2;
	    wx = q.w * x2;   wy = q.w * y2;   wz = q.w * z2;
	
	    m[0][0] = 1.0f - (yy + zz);
	    m[1][0] = xy - wz;
	    m[2][0] = xz + wy;
	
	    m[0][1] = xy + wz;
	    m[1][1] = 1.0f - (xx + zz);
	    m[2][1] = yz - wx;
	
	    m[0][2] = xz - wy;
	    m[1][2] = yz + wx;
	    m[2][2] = 1.0f - (xx + yy);
	
	    m[3][0] = m[3][1] = m[3][2] = 0.0f;
	    m[0][3] = m[1][3] = m[2][3] = 0.0f;
	    m[3][3] = 1.0f;
	}
	
	void Ident() 
	{
	    memcpy(&(m[0][0]), matrix44_ident, sizeof(matrix44_ident));
	}
	
	void Transpose() 
	{
	    #undef _swap
	    #define _swap(x,y) { float t=x; x=y; y=t; }
	    _swap(m12, m21);
	    _swap(m13, m31);
	    _swap(m14, m41);
	    _swap(m23, m32);
	    _swap(m24, m42);
	    _swap(m34, m43);
	}
	
	float Det() 
	{
	    return
	        (m11 * m22 - m12 * m21) * (m33 * m44 - m34 * m43)
	       -(m11 * m23 - m13 * m21) * (m32 * m44 - m34 * m42)
	       +(m11 * m24 - m14 * m21) * (m32 * m43 - m33 * m42)
	       +(m12 * m23 - m13 * m22) * (m31 * m44 - m34 * m41)
	       -(m12 * m24 - m14 * m22) * (m31 * m43 - m33 * m41)
	       +(m13 * m24 - m14 * m23) * (m31 * m42 - m32 * m41);
	}
	
	void Invert() 
	{
	    float s = Det();
	    if (s == 0.0) return;
	    s = 1/s;
	    this->Set(
	        s*(m22*(m33*m44 - m34*m43) + m23*(m34*m42 - m32*m44) + m24*(m32*m43 - m33*m42)),
	        s*(m32*(m13*m44 - m14*m43) + m33*(m14*m42 - m12*m44) + m34*(m12*m43 - m13*m42)),
	        s*(m42*(m13*m24 - m14*m23) + m43*(m14*m22 - m12*m24) + m44*(m12*m23 - m13*m22)),
	        s*(m12*(m24*m33 - m23*m34) + m13*(m22*m34 - m24*m32) + m14*(m23*m32 - m22*m33)),
	        s*(m23*(m31*m44 - m34*m41) + m24*(m33*m41 - m31*m43) + m21*(m34*m43 - m33*m44)),
	        s*(m33*(m11*m44 - m14*m41) + m34*(m13*m41 - m11*m43) + m31*(m14*m43 - m13*m44)),
	        s*(m43*(m11*m24 - m14*m21) + m44*(m13*m21 - m11*m23) + m41*(m14*m23 - m13*m24)),
	        s*(m13*(m24*m31 - m21*m34) + m14*(m21*m33 - m23*m31) + m11*(m23*m34 - m24*m33)),
	        s*(m24*(m31*m42 - m32*m41) + m21*(m32*m44 - m34*m42) + m22*(m34*m41 - m31*m44)),
	        s*(m34*(m11*m42 - m12*m41) + m31*(m12*m44 - m14*m42) + m32*(m14*m41 - m11*m44)),
	        s*(m44*(m11*m22 - m12*m21) + m41*(m12*m24 - m14*m22) + m42*(m14*m21 - m11*m24)),
	        s*(m14*(m22*m31 - m21*m32) + m11*(m24*m32 - m22*m34) + m12*(m21*m34 - m24*m31)),
	        s*(m21*(m33*m42 - m32*m43) + m22*(m31*m43 - m33*m41) + m23*(m32*m41 - m31*m42)),
	        s*(m31*(m13*m42 - m12*m43) + m32*(m11*m43 - m13*m41) + m33*(m12*m41 - m11*m42)),
	        s*(m41*(m13*m22 - m12*m23) + m42*(m11*m23 - m13*m21) + m43*(m12*m21 - m11*m22)),
	        s*(m11*(m22*m33 - m23*m32) + m12*(m23*m31 - m21*m33) + m13*(m21*m32 - m22*m31)));
	}
	
	/* Inverts a 4x4 matrix consisting of a 3x3 rotation matrix and a translation (eg. everything
	that has [0,0,0,1] as the rightmost column) MUCH cheaper then a real 4x4 inversion */
	void InvertSimple() 
	{
	    float s = Det();
	    if (s == 0.0f) return;
	    s = 1.0f/s;
	    this->Set(
	        s * ((m22 * m33) - (m23 * m32)),
	        s * ((m32 * m13) - (m33 * m12)),
	        s * ((m12 * m23) - (m13 * m22)),
	        0.0f,
	        s * ((m23 * m31) - (m21 * m33)),
	        s * ((m33 * m11) - (m31 * m13)),
	        s * ((m13 * m21) - (m11 * m23)),
	        0.0f,
	        s * ((m21 * m32) - (m22 * m31)),
	        s * ((m31 * m12) - (m32 * m11)),
	        s * ((m11 * m22) - (m12 * m21)),
	        0.0f,
	        s * (m21*(m33*m42 - m32*m43) + m22*(m31*m43 - m33*m41) + m23*(m32*m41 - m31*m42)),
	        s * (m31*(m13*m42 - m12*m43) + m32*(m11*m43 - m13*m41) + m33*(m12*m41 - m11*m42)),
	        s * (m41*(m13*m22 - m12*m23) + m42*(m11*m23 - m13*m21) + m43*(m12*m21 - m11*m22)),
	        1.0f);
	}
	
	// Optimized multiplication, assumes that M14==M24==M34==0 AND M44==1
	void MultSimple(const matrix44& m1) 
	{
	    int i;
	    for (i=0; i<4; i++) 
	    {
	        float mi0 = m[i][0];
	        float mi1 = m[i][1];
	        float mi2 = m[i][2];
	        m[i][0] = mi0*m1.m[0][0] + mi1*m1.m[1][0] + mi2*m1.m[2][0];
	        m[i][1] = mi0*m1.m[0][1] + mi1*m1.m[1][1] + mi2*m1.m[2][1];
	        m[i][2] = mi0*m1.m[0][2] + mi1*m1.m[1][2] + mi2*m1.m[2][2];
	    }
	    m[3][0] += m1.m[3][0];
	    m[3][1] += m1.m[3][1];
	    m[3][2] += m1.m[3][2];
	    m[0][3] = 0.0f;
	    m[1][3] = 0.0f;
	    m[2][3] = 0.0f;
	    m[3][3] = 1.0f;
	}
	
	// Transforms a vector by the matrix, projecting the result back into w=1.
	vector3 TransformCoord(const vector3& v) const
	{
	    float d = 1.0f / (m14*v.x + m24*v.y + m34*v.z + m44);
	    return vector3(
	        (m11*v.x + m21*v.y + m31*v.z + m41) * d,
	        (m12*v.x + m22*v.y + m32*v.z + m42) * d,
	        (m13*v.x + m23*v.y + m33*v.z + m43) * d);
	}
	
	vector3 XComponent() const
	{
	    vector3 v(m11,m12,m13);
	    return v;
	}
	
	vector3 YComponent() const
	{
	    vector3 v(m21,m22,m23);
	    return v;
	}
	
	vector3 ZComponent() const 
	{
	    vector3 v(m31,m32,m33);
	    return v;
	}
	
	vector3 PosComponent() const 
	{
	    vector3 v(m41,m42,m43);
	    return v;
	}
	
	void RotateX(const float a) 
	{
	    float c = cosf(a);
	    float s = sinf(a);
	    int i;
	    for (i=0; i<4; i++) {
	        float mi1 = m[i][1];
	        float mi2 = m[i][2];
	        m[i][1] = mi1*c + mi2*-s;
	        m[i][2] = mi1*s + mi2*c;
	    }
	}
	
	void RotateY(const float a) 
	{
	    float c = cosf(a);
	    float s = sinf(a);
	    int i;
	    for (i=0; i<4; i++) {
	        float mi0 = m[i][0];
	        float mi2 = m[i][2];
	        m[i][0] = mi0*c + mi2*s;
	        m[i][2] = mi0*-s + mi2*c;
	    }
	}
	
	void RotateZ(const float a) 
	{
	    float c = cosf(a);
	    float s = sinf(a);
	    int i;
	    for (i=0; i<4; i++) {
	        float mi0 = m[i][0];
	        float mi1 = m[i][1];
	        m[i][0] = mi0*c + mi1*-s;
	        m[i][1] = mi0*s + mi1*c;
	    }
	}
	
	void Translate(const vector3& t) 
	{
	    m41 += t.x;
	    m42 += t.y;
	    m43 += t.z;
	}
	
	void Scale(const vector3& s) 
	{
	    int i;
	    for (i=0; i<4; i++) 
	    {
	        m[i][0] *= s.x;
	        m[i][1] *= s.y;
	        m[i][2] *= s.z;
	    }
	}
	
	void LookatRh(const vector3& at, const vector3& up) 
	{
	    vector3 eye(m41, m42, m43);
	    vector3 zaxis = eye - at;
	    zaxis.Normalize();
	    vector3 xaxis = up * zaxis;
	    xaxis.Normalize();
	    vector3 yaxis = zaxis * xaxis;
	    m11 = xaxis.x;  m12 = xaxis.y;  m13 = xaxis.z;  m14 = 0.0f;
	    m21 = yaxis.x;  m22 = yaxis.y;  m23 = yaxis.z;  m24 = 0.0f;
	    m31 = zaxis.x;  m32 = zaxis.y;  m33 = zaxis.z;  m34 = 0.0f;
	}
	
	void LookatLh(const vector3& at, const vector3& up) 
	{
	    vector3 eye(m41, m42, m43);
	    vector3 zaxis = at - eye;
	    zaxis.Normalize();
	    vector3 xaxis = up * zaxis;
	    xaxis.Normalize();
	    vector3 yaxis = zaxis * xaxis;
	    m11 = xaxis.x;  m12 = yaxis.x;  m13 = zaxis.x;  m14 = 0.0f;
	    m21 = xaxis.y;  m22 = yaxis.y;  m23 = zaxis.y;  m24 = 0.0f;
	    m31 = xaxis.z;  m32 = yaxis.z;  m33 = zaxis.z;  m34 = 0.0f;
	}
	
	void PerspFovLh(float fovY, float aspect, float zn, float zf)
	{
	    float h = float(1.0 / tan(fovY * 0.5f));
	    float w = h / aspect;
	    m11 = w;    m12 = 0.0f; m13 = 0.0f;                   m14 = 0.0f;
	    m21 = 0.0f; m22 = h;    m23 = 0.0f;                   m24 = 0.0f;
	    m31 = 0.0f; m32 = 0.0f; m33 = zf / (zf - zn);         m34 = 1.0f;
	    m41 = 0.0f; m42 = 0.0f; m43 = -zn * (zf / (zf - zn)); m44 = 0.0f;
	}
	
	void PerspFovRh(float fovY, float aspect, float zn, float zf)
	{
	    float h = float(1.0 / tan(fovY * 0.5f));
	    float w = h / aspect;
	    m11 = w;    m12 = 0.0f; m13 = 0.0f;                  m14 = 0.0f;
	    m21 = 0.0f; m22 = h;    m23 = 0.0f;                  m24 = 0.0f;
	    m31 = 0.0f; m32 = 0.0f; m33 = zf / (zn - zf);        m34 = -1.0f;
	    m41 = 0.0f; m42 = 0.0f; m43 = zn * (zf / (zn - zf)); m44 = 0.0f;
	}
	
	void OrthoLh(float w, float h, float zn, float zf)
	{
	    m11 = 2.0f / w; m12 = 0.0f;     m13 = 0.0f;             m14 = 0.0f;
	    m21 = 0.0f;     m22 = 2.0f / h; m23 = 0.0f;             m24 = 0.0f;
	    m31 = 0.0f;     m32 = 0.0f;     m33 = 1.0f / (zf - zn); m34 = 0.0f;
	    m41 = 0.0f;     m42 = 0.0f;     m43 = zn / (zn - zf);   m44 = 1.0f;
	}
	
	void OrthoRh(float w, float h, float zn, float zf)
	{
	    m11 = 2.0f / w; m12 = 0.0f;     m13 = 0.0f;             m14 = 0.0f;
	    m21 = 0.0f;     m22 = 2.0f / h; m23 = 0.0f;             m24 = 0.0f;
	    m31 = 0.0f;     m32 = 0.0f;     m33 = 1.0f / (zn - zf); m34 = 0.0f;
	    m41 = 0.0f;     m42 = 0.0f;     m43 = zn / (zn - zf);   m44 = 1.0f;
	}
	
	void Billboard(const vector3& to, const vector3& up)
	{
	    vector3 from(m41, m42, m43);
	    vector3 z(from - to);
	    z.Normalize();
	    vector3 y(up);
	    y.Normalize();
	    vector3 x(y * z);
	    z = x * y;       
	
	    m11=x.x;  m12=x.y;  m13=x.z;  m14=0.0f;
	    m21=y.x;  m22=y.y;  m23=y.z;  m24=0.0f;
	    m31=z.x;  m32=z.y;  m33=z.z;  m34=0.0f;
	}
	
	void operator *=(const matrix44& m1) 
	{
	    int i;
	    for (i=0; i<4; i++) 
	    {
	        float mi0 = m[i][0];
	        float mi1 = m[i][1];
	        float mi2 = m[i][2];
	        float mi3 = m[i][3];
	        m[i][0] = mi0*m1.m[0][0] + mi1*m1.m[1][0] + mi2*m1.m[2][0] + mi3*m1.m[3][0];
	        m[i][1] = mi0*m1.m[0][1] + mi1*m1.m[1][1] + mi2*m1.m[2][1] + mi3*m1.m[3][1];
	        m[i][2] = mi0*m1.m[0][2] + mi1*m1.m[1][2] + mi2*m1.m[2][2] + mi3*m1.m[3][2];
	        m[i][3] = mi0*m1.m[0][3] + mi1*m1.m[1][3] + mi2*m1.m[2][3] + mi3*m1.m[3][3];
	    }
	}
	
	void Rotate(const vector3& vec, float a)
	{
	    vector3 v(vec);
	    v.Normalize();
	    float sa = (float) sinf(a);
	    float ca = (float) cosf(a);
	
		matrix44 rotM;
		rotM.m11 = ca + (1.0f - ca) * v.x * v.x;
		rotM.m12 = (1.0f - ca) * v.x * v.y - sa * v.z;
		rotM.m13 = (1.0f - ca) * v.z * v.x + sa * v.y;
		rotM.m21 = (1.0f - ca) * v.x * v.y + sa * v.z;
		rotM.m22 = ca + (1.0f - ca) * v.y * v.y;
		rotM.m23 = (1.0f - ca) * v.y * v.z - sa * v.x;
		rotM.m31 = (1.0f - ca) * v.z * v.x - sa * v.y;
		rotM.m32 = (1.0f - ca) * v.y * v.z + sa * v.x;
		rotM.m33 = ca + (1.0f - ca) * v.z * v.z;
		
		(*this) *= rotM;
	}
	
	void Mult(const vector4& src, vector4& dst) const
	{
	    dst.x = m11*src.x + m21*src.y + m31*src.z + m41*src.w;
	    dst.y = m12*src.x + m22*src.y + m32*src.z + m42*src.w;
	    dst.z = m13*src.x + m23*src.y + m33*src.z + m43*src.w;
	    dst.w = m14*src.x + m24*src.y + m34*src.z + m44*src.w;
	}
	
	void Mult(const vector3& src, vector3& dst) const
	{
	    dst.x = m11*src.x + m21*src.y + m31*src.z + m41;
	    dst.y = m12*src.x + m22*src.y + m32*src.z + m42;
	    dst.z = m13*src.x + m23*src.y + m33*src.z + m43;
	}
	
	void SetProjection(float hFOV, float vFOV, float fNearPlane, float fFarPlane)
	{
		if(Abs(fFarPlane-fNearPlane) < .01f ) return;

		float w = Cot(hFOV*.5f);
		float h = Cot(vFOV*.5f);
		float Q = fFarPlane/(fFarPlane - fNearPlane);

		Ident();
		m[0][0] = w;
		m[1][1] = h;
		m[2][2] = Q;
		m[2][3] = 1.0f;
		m[3][2] = -Q*fNearPlane;
	}

	void SetView(vector3& vFrom, vector3& vAt, vector3& vWorldUp)
	{
		// Get the z basis vector, which points straight ahead. This is the difference from the eyepoint to the lookat point.
		vector3 vView = vAt - vFrom;
		float fLength = vView.Length();
		if(fLength < 1e-6f) return;
		vView /= fLength; // Normalize the z basis vector

		// Get the dot product, and calculate the projection of the z basis vector onto the up vector. The projection is the y basis vector.
		float fDotProduct = vWorldUp % vView;
		vector3 vUp = vWorldUp - fDotProduct * vView;

		// If this vector has near-zero length because the input specified a bogus up vector, let's try a default up vector
		if(1e-6f > (fLength = vUp.Length()))
		{
			vUp = vector3(0.f,1.f,0.f ) - vView.y * vView;
			if(1e-6f > (fLength = vUp.Length())) // If we still have near-zero length, resort to a different axis.
			{
				vUp = vector3(0.f,0.f,1.f ) - vView.z * vView;
				if(1e-6f > (fLength = vUp.Length())) return;
			}
		}

		vUp /= fLength; // Normalize the y basis vector

		// The x basis vector is found simply with the cross product of the y and z basis vectors
		vector3 vRight = vUp * vView;

		// Start building the matrix. The first three rows contains the basis vectors used to rotate the view to point at the lookat point
		Ident();
		m[0][0] = vRight.x; m[0][1] = vUp.x; m[0][2] = vView.x;
		m[1][0] = vRight.y; m[1][1] = vUp.y; m[1][2] = vView.y;
		m[2][0] = vRight.z; m[2][1] = vUp.z; m[2][2] = vView.z;

		// Do the translation values (rotations are still about the eyepoint)
		m[3][0] = -(vFrom % vRight);
		m[3][1] = -(vFrom % vUp);
		m[3][2] = -(vFrom % vView);
	}

	friend matrix44 operator *(const matrix44& m0, const matrix44& m1) 
	{
	    matrix44 m2(
	        m0.m[0][0]*m1.m[0][0] + m0.m[0][1]*m1.m[1][0] + m0.m[0][2]*m1.m[2][0] + m0.m[0][3]*m1.m[3][0],
	        m0.m[0][0]*m1.m[0][1] + m0.m[0][1]*m1.m[1][1] + m0.m[0][2]*m1.m[2][1] + m0.m[0][3]*m1.m[3][1],
	        m0.m[0][0]*m1.m[0][2] + m0.m[0][1]*m1.m[1][2] + m0.m[0][2]*m1.m[2][2] + m0.m[0][3]*m1.m[3][2],
	        m0.m[0][0]*m1.m[0][3] + m0.m[0][1]*m1.m[1][3] + m0.m[0][2]*m1.m[2][3] + m0.m[0][3]*m1.m[3][3],
	
	        m0.m[1][0]*m1.m[0][0] + m0.m[1][1]*m1.m[1][0] + m0.m[1][2]*m1.m[2][0] + m0.m[1][3]*m1.m[3][0],
	        m0.m[1][0]*m1.m[0][1] + m0.m[1][1]*m1.m[1][1] + m0.m[1][2]*m1.m[2][1] + m0.m[1][3]*m1.m[3][1],
	        m0.m[1][0]*m1.m[0][2] + m0.m[1][1]*m1.m[1][2] + m0.m[1][2]*m1.m[2][2] + m0.m[1][3]*m1.m[3][2],
	        m0.m[1][0]*m1.m[0][3] + m0.m[1][1]*m1.m[1][3] + m0.m[1][2]*m1.m[2][3] + m0.m[1][3]*m1.m[3][3],
	
	        m0.m[2][0]*m1.m[0][0] + m0.m[2][1]*m1.m[1][0] + m0.m[2][2]*m1.m[2][0] + m0.m[2][3]*m1.m[3][0],
	        m0.m[2][0]*m1.m[0][1] + m0.m[2][1]*m1.m[1][1] + m0.m[2][2]*m1.m[2][1] + m0.m[2][3]*m1.m[3][1],
	        m0.m[2][0]*m1.m[0][2] + m0.m[2][1]*m1.m[1][2] + m0.m[2][2]*m1.m[2][2] + m0.m[2][3]*m1.m[3][2],
	        m0.m[2][0]*m1.m[0][3] + m0.m[2][1]*m1.m[1][3] + m0.m[2][2]*m1.m[2][3] + m0.m[2][3]*m1.m[3][3],
	
	        m0.m[3][0]*m1.m[0][0] + m0.m[3][1]*m1.m[1][0] + m0.m[3][2]*m1.m[2][0] + m0.m[3][3]*m1.m[3][0],
	        m0.m[3][0]*m1.m[0][1] + m0.m[3][1]*m1.m[1][1] + m0.m[3][2]*m1.m[2][1] + m0.m[3][3]*m1.m[3][1],
	        m0.m[3][0]*m1.m[0][2] + m0.m[3][1]*m1.m[1][2] + m0.m[3][2]*m1.m[2][2] + m0.m[3][3]*m1.m[3][2],
	        m0.m[3][0]*m1.m[0][3] + m0.m[3][1]*m1.m[1][3] + m0.m[3][2]*m1.m[2][3] + m0.m[3][3]*m1.m[3][3]);
	    return m2;
	}
	
	friend vector3 operator *(const matrix44& m, const vector3& v)
	{
	    return vector3(
	        m.m11*v.x + m.m21*v.y + m.m31*v.z + m.m41,
	        m.m12*v.x + m.m22*v.y + m.m32*v.z + m.m42,
	        m.m13*v.x + m.m23*v.y + m.m33*v.z + m.m43);
	}
	
	friend vector4 operator *(const matrix44& m, const vector4& v)
	{
	    return vector4(
	        m.m11*v.x + m.m21*v.y + m.m31*v.z + m.m41*v.w,
	        m.m12*v.x + m.m22*v.y + m.m32*v.z + m.m42*v.w,
	        m.m13*v.x + m.m23*v.y + m.m33*v.z + m.m43*v.w,
	        m.m14*v.x + m.m24*v.y + m.m34*v.z + m.m44*v.w);
	};
	
    float m[4][4];
};

#endif
