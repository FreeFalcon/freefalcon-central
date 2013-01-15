// Math Lib Trig functions
#ifndef _ML_TRIG_H
#define _ML_TRIG_H

typedef struct 
{
	Float32 sin;
	Float32 cos;
} mlTrig;

inline void mlSinCos(mlTrig* trig, Float32 angle)
{
#if defined(_MSC_VER)
	__asm 
	{
		__asm	mov     ecx, trig;
		__asm	fld     dword ptr [angle];
		__asm	fsincos;
		__asm	fstp    dword ptr [ecx]trig.cos;
		__asm	fstp    dword ptr [ecx]trig.sin;
	}
#else
	trig->sin = (Float32)sin(angle);
	trig->cos = (Float32)cos(angle);
#endif
}

// JB 010227
/*
atan2(y,x)=atan(y/x)       x>0
atan2(y,x)=atan(y/x)+pi    x<0, y>=0
atan2(y,x)=pi/2            x=0, y>0
atan2(y,x)=atan(y/x)-pi    x<0, y<0
atan2(y,x)=-pi/2           x=0, y<0
atan2(0,0) is undefined and should give an error.
*/
#ifndef DPI // JPO - work in double precision
#define DPI 3.1415926535897932384626433832795
#endif
#ifndef HALF_DPI
#define HALF_DPI	(DPI/2.0)
#endif
#pragma function(atan2)
#define atan2(x,y) checked_atan2(x,y)
inline double checked_atan2(double y, double x)
{
	double z;
	
	if (x == -0.0)
		x = 0.0;
	
	if (x != 0)
		z = atan(y / x);
	else if (y > 0)
		z = HALF_DPI;
	else
		z = -HALF_DPI;

	if (x < 0)
	{
		if (y < 0)
			z -= DPI;
		else
			z += DPI;
	}

	return z;
}

#endif
