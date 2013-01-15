/*
matrix33.h

	Author: Miro "Jammer" Torrielli
	Last Update: 21 April 2004 
*/

#ifndef _MATRIX33_H
#define _MATRIX33_H

#include <memory.h>
#include "vector2.h"
#include "vector3.h"
#include "quaternion.h"
#include "euler.h"
#include "matrixdefs.h"

static float matrix33_ident[9] = 
{
    1.f, 0.f, 0.f,
    0.f, 1.f, 0.f,
    0.f, 0.f, 1.f,
};

struct matrix33 
{

	matrix33() 
	{
	    memcpy(&(m[0][0]), matrix33_ident, sizeof(matrix33_ident));
	}
	
	matrix33(const vector3& v0, const vector3& v1, const vector3& v2) 
	{
	    m11=v0.x; m12=v0.y; m13=v0.z;
	    m21=v1.x; m22=v1.y; m23=v1.z;
	    m31=v2.x; m32=v2.y; m33=v2.z;
	}
	
	matrix33(const matrix33& m1) 
	{
	    memcpy(m, &(m1.m[0][0]), 9*sizeof(float));
	}
	
	matrix33(float _m11, float _m12, float _m13,
	                     float _m21, float _m22, float _m23,
	                     float _m31, float _m32, float _m33)
	{
	    m11=_m11; m12=_m12; m13=_m13;
	    m21=_m21; m22=_m22; m23=_m23;
	    m31=_m31; m32=_m32; m33=_m33;
	}
	
	matrix33(const quaternion& q) 
	{
	    float xx = q.x*q.x; float yy = q.y*q.y; float zz = q.z*q.z;
	    float xy = q.x*q.y; float xz = q.x*q.z; float yz = q.y*q.z;
	    float wx = q.w*q.x; float wy = q.w*q.y; float wz = q.w*q.z;
	
	    m[0][0] = 1.f - 2.f * (yy + zz);
	    m[1][0] =        2.f * (xy - wz);
	    m[2][0] =        2.f * (xz + wy);
	
	    m[0][1] =        2.f * (xy + wz);
	    m[1][1] = 1.f - 2.f * (xx + zz);
	    m[2][1] =        2.f * (yz - wx);
	
	    m[0][2] =        2.f * (xz - wy);
	    m[1][2] =        2.f * (yz + wx);
	    m[2][2] = 1.f - 2.f * (xx + yy);
	}
	
	friend matrix33 operator *(const matrix33& m0, const matrix33& m1) 
	{
	    matrix33 m2(
	        m0.m[0][0]*m1.m[0][0] + m0.m[0][1]*m1.m[1][0] + m0.m[0][2]*m1.m[2][0],
	        m0.m[0][0]*m1.m[0][1] + m0.m[0][1]*m1.m[1][1] + m0.m[0][2]*m1.m[2][1],
	        m0.m[0][0]*m1.m[0][2] + m0.m[0][1]*m1.m[1][2] + m0.m[0][2]*m1.m[2][2],
	
	        m0.m[1][0]*m1.m[0][0] + m0.m[1][1]*m1.m[1][0] + m0.m[1][2]*m1.m[2][0],
	        m0.m[1][0]*m1.m[0][1] + m0.m[1][1]*m1.m[1][1] + m0.m[1][2]*m1.m[2][1],
	        m0.m[1][0]*m1.m[0][2] + m0.m[1][1]*m1.m[1][2] + m0.m[1][2]*m1.m[2][2],
	
	        m0.m[2][0]*m1.m[0][0] + m0.m[2][1]*m1.m[1][0] + m0.m[2][2]*m1.m[2][0],
	        m0.m[2][0]*m1.m[0][1] + m0.m[2][1]*m1.m[1][1] + m0.m[2][2]*m1.m[2][1],
	        m0.m[2][0]*m1.m[0][2] + m0.m[2][1]*m1.m[1][2] + m0.m[2][2]*m1.m[2][2]
	    );
	    return m2;
	}
	
	friend vector3 operator *(const matrix33& m, const vector3& v)
	{
	    return vector3(
	        m.m11*v.x + m.m21*v.y + m.m31*v.z,
	        m.m12*v.x + m.m22*v.y + m.m32*v.z,
	        m.m13*v.x + m.m23*v.y + m.m33*v.z);
	};
	
	quaternion GetQuaternion() const
	{
	    float qa[4];
	    float tr = m[0][0] + m[1][1] + m[2][2];
	    if (tr > 0.f) 
	    {
	        float s = sqrt (tr + 1.f);
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
	        float s = sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.f);
	        qa[i] = s * 0.5f;
	        s = 0.5f / s;
	        qa[3] = (m[j][k] - m[k][j])* s;
	        qa[j] = (m[i][j] + m[j][i]) * s;
	        qa[k] = (m[i][k] + m[k][i]) * s;
	    }
	    quaternion q(qa[0],qa[1],qa[2],qa[3]);
	    return q;
	}
	
	vector3 ToEuler() const
	{    
	    vector3 ea;
	    
	    // work on matrix with flipped row/columns
	    matrix33 tmp(*this);
	    tmp.Transpose();
	
	    int i,j,k,h,n,s,f;
	    EulGetOrd(EulOrdXYZs,i,j,k,h,n,s,f);
	    if (s==EulRepYes) 
	    {
	        double sy = (float) sqrt(tmp.m12 * tmp.m12 + tmp.m13 * tmp.m13);
	        if (sy > 16*FLT_EPSILON) 
	        {
	            ea.x = (float) atan2(tmp.m12, tmp.m13);
	            ea.y = (float) atan2((float)sy, tmp.m11);
	            ea.z = (float) atan2(tmp.m21, -tmp.m31);
	        } else {
	            ea.x = (float) atan2(-tmp.m23, tmp.m22);
	            ea.y = (float) atan2((float)sy, tmp.m11);
	            ea.z = 0;
	        }
	    } 
	    else 
	    {
	        double cy = sqrt(tmp.m11 * tmp.m11 + tmp.m21 * tmp.m21);
	        if (cy > 16*FLT_EPSILON) 
	        {
	            ea.x = (float) atan2(tmp.m32, tmp.m33);
	            ea.y = (float) atan2(-tmp.m31, (float)cy);
	            ea.z = (float) atan2(tmp.m21, tmp.m11);
	        } 
	        else 
	        {
	            ea.x = (float) atan2(-tmp.m23, tmp.m22);
	            ea.y = (float) atan2(-tmp.m31, (float)cy);
	            ea.z = 0;
	        }
	    }
	    if (n==EulParOdd) {ea.x = -ea.x; ea.y = - ea.y; ea.z = -ea.z;}
	    if (f==EulFrmR) {float t = ea.x; ea.x = ea.z; ea.z = t;}
	
	    return ea;
	}
	
	void FromEuler(const vector3& ea) 
	{
	    vector3 tea = ea;
	    double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
	    int i,j,k,h,n,s,f;
	    EulGetOrd(EulOrdXYZs,i,j,k,h,n,s,f);
	    if (f==EulFrmR) {float t = ea.x; tea.x = ea.z; tea.z = t;}
	    if (n==EulParOdd) {tea.x = -ea.x; tea.y = -ea.y; tea.z = -ea.z;}
	    ti = tea.x;   tj = tea.y;   th = tea.z;
	    ci = cos(ti); cj = cos(tj); ch = cos(th);
	    si = sin(ti); sj = sin(tj); sh = sin(th);
	    cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;
	    if (s==EulRepYes) 
	    {
	        m11 = (float)(cj);     m12 = (float)(sj*si);     m13 = (float)(sj*ci);
	        m21 = (float)(sj*sh);  m22 = (float)(-cj*ss+cc); m23 = (float)(-cj*cs-sc);
	        m31 = (float)(-sj*ch); m23 = (float)( cj*sc+cs); m33 = (float)( cj*cc-ss);
	    } 
	    else 
	    {
	        m11 = (float)(cj*ch); m12 = (float)(sj*sc-cs); m13 = (float)(sj*cc+ss);
	        m21 = (float)(cj*sh); m22 = (float)(sj*ss+cc); m23 = (float)(sj*cs-sc);
	        m31 = (float)(-sj);   m32 = (float)(cj*si);    m33 = (float)(cj*ci);
	    }
	
	    // flip row/column
	    this->Transpose();
	}
	
	void LookAt(const vector3& from, const vector3& to, const vector3& up) 
	{
	    vector3 z(from - to);
	    z.Normalize();
	    vector3 x(up * z);   // x = y cross z
	    x.Normalize();
	    vector3 y = z * x;   // y = z cross x
	
	    m11=x.x;  m12=x.y;  m13=x.z;
	    m21=y.x;  m22=y.y;  m23=y.z;
	    m31=z.x;  m32=z.y;  m33=z.z;
	}
	
	void Billboard(const vector3& from, const vector3& to, const vector3& up)
	{
	    vector3 z(from - to);
	    z.Normalize();
	    vector3 y(up);
	    y.Normalize();
	    vector3 x(y * z);
	    z = x * y;
	
	    m11=x.x;  m12=x.y;  m13=x.z;
	    m21=y.x;  m22=y.y;  m23=y.z;
	    m31=z.x;  m32=z.y;  m33=z.z;
	}
	
	void Set(float _m11, float _m12, float _m13,
	               float _m21, float _m22, float _m23,
	               float _m31, float _m32, float _m33) 
	{
	    m11=_m11; m12=_m12; m13=_m13;
	    m21=_m21; m22=_m22; m23=_m23;
	    m31=_m31; m32=_m32; m33=_m33;
	}
	
	void Set(const vector3& v0, const vector3& v1, const vector3& v2) 
	{
	    m11=v0.x; m12=v0.y; m13=v0.z;
	    m21=v1.x; m22=v1.y; m23=v1.z;
	    m31=v2.x; m32=v2.y; m33=v2.z;
	}
	
	void Set(const matrix33& m1) 
	{
	    memcpy(m, &(m1.m), 9*sizeof(float));
	}
	
	void Ident() 
	{
	    memcpy(&(m[0][0]), matrix33_ident, sizeof(matrix33_ident));
	}
	
	void Transpose() 
	{
	    #undef _swap
	    #define _swap(x,y) { float t=x; x=y; y=t; }
	    _swap(m[0][1],m[1][0]);
	    _swap(m[0][2],m[2][0]);
	    _swap(m[1][2],m[2][1]);
	}
	
	bool Orthonorm(float limit) 
	{
	    if (((m11*m21+m12*m22+m13*m23)<limit) &&
	        ((m11*m31+m12*m32+m13*m33)<limit) &&
	        ((m31*m21+m32*m22+m33*m23)<limit) &&
	        ((m11*m11+m12*m12+m13*m13)>(1.0-limit)) &&
	        ((m11*m11+m12*m12+m13*m13)<(1.0+limit)) &&
	        ((m21*m21+m22*m22+m23*m23)>(1.0-limit)) &&
	        ((m21*m21+m22*m22+m23*m23)<(1.0+limit)) &&
	        ((m31*m31+m32*m32+m33*m33)>(1.0-limit)) &&
	        ((m31*m31+m32*m32+m33*m33)<(1.0+limit)))
	        return true;
	    else
	        return false;
	}
	
	void Scale(const vector3& s)
	{
	    int i;
	    for (i=0; i<3; i++) {
	        m[i][0] *= s.x;
	        m[i][1] *= s.y;
	        m[i][2] *= s.z;
	    }
	}
	
	void RotateX(const float a)
	{
	    float c = cosf(a);
	    float s = sinf(a);
	    int i;
	    for (i=0; i<3; i++)
	    {
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
	    for (i=0; i<3; i++)
	    {
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
	    for (i=0; i<3; i++)
	    {
	        float mi0 = m[i][0];
	        float mi1 = m[i][1];
	        m[i][0] = mi0*c + mi1*-s;
	        m[i][1] = mi0*s + mi1*c;
	    }
	}
	
	void RotateLocalX(const float a)
	{
	    matrix33 rotM;  // initialized as identity matrix
		rotM.m22 = (float) cosf(a); rotM.m23 = -(float) sinf(a);
		rotM.m32 = (float) sinf(a); rotM.m33 =  (float) cosf(a);
	
		(*this) = rotM * (*this); 
	}
	
	void RotateLocalY(const float a)
	{
	    matrix33 rotM;  // initialized as identity matrix
		rotM.m11 = (float) cosf(a);  rotM.m13 = (float) sinf(a);
	    rotM.m31 = -(float) sinf(a); rotM.m33 = (float) cosf(a);
	
		(*this) = rotM * (*this); 
	}
	
	void RotateLocalZ(const float a)
	{
	    matrix33 rotM;  // initialized as identity matrix
	    rotM.m11 = (float) cosf(a); rotM.m12 = -(float) sinf(a);
		rotM.m21 = (float) sinf(a); rotM.m22 =  (float) cosf(a);
	
		(*this) = rotM * (*this); 
	}
	
	void Rotate(const vector3& vec, float a)
	{
	    vector3 v(vec);
	    v.Normalize();
	    float sa = (float) sinf(a);
	    float ca = (float) cosf(a);
	
		matrix33 rotM;
		rotM.m11 = ca + (1.f - ca) * v.x * v.x;
		rotM.m12 = (1.f - ca) * v.x * v.y - sa * v.z;
		rotM.m13 = (1.f - ca) * v.z * v.x + sa * v.y;
		rotM.m21 = (1.f - ca) * v.x * v.y + sa * v.z;
		rotM.m22 = ca + (1.f - ca) * v.y * v.y;
		rotM.m23 = (1.f - ca) * v.y * v.z - sa * v.x;
		rotM.m31 = (1.f - ca) * v.z * v.x - sa * v.y;
		rotM.m32 = (1.f - ca) * v.y * v.z + sa * v.x;
		rotM.m33 = ca + (1.f - ca) * v.z * v.z;
		
		(*this) = (*this) * rotM;
	}
	
	vector3 XComponent() const
	{
	    vector3 v(m11,m12,m13);
	    return v;
	}
	
	vector3 YComponent(void) const
	{
	    vector3 v(m21,m22,m23);
	    return v;
	}
	
	vector3 ZComponent(void) const 
	{
	    vector3 v(m31,m32,m33);
	    return v;
	};
	
	void operator *=(const matrix33& m1) 
	{
	    int i;
	    for (i=0; i<3; i++) {
	        float mi0 = m[i][0];
	        float mi1 = m[i][1];
	        float mi2 = m[i][2];
	        m[i][0] = mi0*m1.m[0][0] + mi1*m1.m[1][0] + mi2*m1.m[2][0];
	        m[i][1] = mi0*m1.m[0][1] + mi1*m1.m[1][1] + mi2*m1.m[2][1];
	        m[i][2] = mi0*m1.m[0][2] + mi1*m1.m[1][2] + mi2*m1.m[2][2];
	    };
	}
	
	/* Multiply source vector with matrix and store in destination vector
	this eliminates the construction of a temp vector3 object */
	void Mult(const vector3& src, vector3& dst) const
	{
	    dst.x = m11*src.x + m21*src.y + m31*src.z;
	    dst.y = m12*src.x + m22*src.y + m32*src.z;
	    dst.z = m13*src.x + m23*src.y + m33*src.z;
	}
	
	void Translate(const vector2& t)
	{
	    m31 += t.x;
	    m32 += t.y;
	}
	
    float m[3][3];
};

#endif
