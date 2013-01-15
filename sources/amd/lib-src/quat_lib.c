/*****************************************************************************
 *
 * Copyright (c) 1996-1999 ADVANCED MICRO DEVICES, INC. All Rights reserved.
 *
 * This software is unpublished and contains the trade secrets and
 * confidential proprietary information of AMD. Unless otherwise
 * provided in the Software Agreement associated herewith, it is
 * licensed in confidence "AS IS" and is not to be reproduced in
 * whole or part by any means except for backup. Use, duplication,
 * or disclosure by the Government is subject to the restrictions
 * in paragraph(b)(3)(B)of the Rights in Technical Data and
 * Computer Software clause in DFAR 52.227-7013(a)(Oct 1988).
 * Software owned by Advanced Micro Devices Inc., One AMD Place
 * P.O. Box 3453, Sunnyvale, CA 94088-3453.
 *
 *****************************************************************************
 *
 * QUAT_LIB.C
 *
 * AMD3D 3D library code: Quaternion math
 *	These routines provide a portable quaternion math library.  3DNow!
 *  accelerated versions of these routines can be found in QUAT.ASM.
 *
 *  Loosly based on the quaternion library presented by Jeff Lander.
 *  Adapted to 3DNow! implementation and library conventions.
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#include <math.h>
#include <amath.h>
#include <amd3dx.h>
#include "quat.h"

#define M_PI 3.1415927f
#define HALF_PI (M_PI*0.5f)
#define DEG2RAD (M_PI / (360.0f / 2.0f))
#define RAD2DEG ((360.0f / 2.0f) / M_PI)


/* add_quat - add the elements of two quaternions together
 *      a, b    - input quaternions
 *      dest    - the resultant quaternion
 */
void add_quat (Quat dest, const Quat a, const Quat b)
{
	dest[0] = a[0] + b[0];
	dest[1] = a[1] + b[1];
	dest[2] = a[2] + b[2];
	dest[3] = a[3] + b[3];
}

/* sub_quat - subtract the elements of 'b' from 'a'
 *      a, b    - input quaternions
 *      dest    - the resultant quaternion
 */
void sub_quat (Quat dest, const Quat a, const Quat b)
{
	dest[0] = a[0] - b[0];
	dest[1] = a[1] - b[1];
	dest[2] = a[2] - b[2];
	dest[3] = a[3] - b[3];
}


/* mat2quat - convert the input 4x4 rotation matrix into a quaternion.
 *      m       - the rotation matrix
 *      quat    - the resultant quaternion
 */
void mat2quat(const float m[4][4], Quat quat)
{
	static int nxt[3] = {1, 2, 0};
	float  tr, s, q[4];
	int    i, j, k;

	tr = m[0][0] + m[1][1] + m[2][2];

	// check the diagonal
	if (tr > 0.0)
	{
		s = (float)sqrt (tr + 1.0f);
		quat[3] = s * 0.5f;
		s = 0.5f / s;
		quat[0] = (m[1][2] - m[2][1]) * s;
		quat[1] = (m[2][0] - m[0][2]) * s;
		quat[2] = (m[0][1] - m[1][0]) * s;
	}
	else
	{
		// diagonal is negative
		i = 0;
		if (m[1][1] > m[0][0]) i = 1;
		if (m[2][2] > m[i][i]) i = 2;
		j = nxt[i];
		k = nxt[j];

		s = (float)sqrt ((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);

		q[i] = s * 0.5f;

		if (s != 0.0)
			s = 0.5f / s;

		q[3] = (m[j][k] - m[k][j]) * s;
		q[j] = (m[i][j] + m[j][i]) * s;
		q[k] = (m[i][k] + m[k][i]) * s;

		quat[0] = q[0];
		quat[1] = q[1];
		quat[2] = q[2];
		quat[3] = q[3];
	}
}


/* quat2mat - construct a 4x4 rotation matrix from a quaternion
 *      quat    - input quaternion
 *      m       - the resultant matrix
 */
void quat2mat(const Quat quat, float m[4][4])
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	// calculate coefficients
	x2 = quat[0] + quat[0];
	y2 = quat[1] + quat[1]; 
	z2 = quat[2] + quat[2];

	xx = quat[0] * x2;   xy = quat[0] * y2;   xz = quat[0] * z2;
	yy = quat[1] * y2;   yz = quat[1] * z2;   zz = quat[2] * z2;
	wx = quat[3] * x2;   wy = quat[3] * y2;   wz = quat[3] * z2;

	m[0][0] = 1.0f - (yy + xx);
	m[0][1] = xy - wz;
	m[0][2] = xz + wy;
	m[0][3] = 0.0f;
	m[1][0] = xy + wz;
	m[1][1] = 1.0f - (xx + zz);
	m[1][2] = yz - wx;
	m[1][3] = 0.0f;
	m[2][0] = xz - wy;
	m[2][1] = yz + wx;
	m[2][2] = 1.0f - (xx + yy);
	m[2][3] = 0.0f;
	m[3][0] =
	m[3][1] =
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}


/* mult_quat - compute the product of two quaternions
 *      q1, q2  - input quaternions
 *      dst     - the resultant quaternion
 */
void mult_quat (Quat dst, const Quat q1, const Quat q2)
{
	// Do this to a temporary variable in case the output aliases an input
	Quat	tmp;
	tmp[0] = q1[0] * q2[3] + q2[0] * q1[3] + q1[1] * q2[2] - q1[2] * q2[1];
	tmp[1] = q1[1] * q2[3] + q2[1] * q1[3] + q1[2] * q2[0] - q1[0] * q2[2];
	tmp[2] = q1[2] * q2[3] + q2[2] * q1[3] + q1[0] * q2[1] - q1[1] * q2[0];
	tmp[3] = q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2];

	dst[0] = tmp[0];
	dst[1] = tmp[1];
	dst[2] = tmp[2];
	dst[3] = tmp[3];
}


/* norm_quat - put the quaternion in normal form (x^2 + y^2 + z^2 + w^2 = 1)
 *      quat    - input quaternions
 *      dst     - the resultant quaternion
 */
void norm_quat (Quat dst, const Quat quat)
{
	const float quatx = quat[0],
				quaty = quat[1],
				quatz = quat[2],
				quatw = quat[3];
	const float magnitude = 1.0f / ((quatx * quatx) +
									(quaty * quaty) +
									(quatz * quatz) +
									(quatw * quatw));

	dst[0] = quatx * magnitude;
	dst[1] = quaty * magnitude;
	dst[2] = quatz * magnitude;
	dst[3] = quatw * magnitude;
}


/* euler2quat - construct a quaternion by applying the given euler angles
 *			in X-Y-Z order (pitch-yaw-roll)
 *      rot     - the euler angles (3 elements)
 *      quat    - the resultant quaternion
 */
void euler2quat (const float *rot, Quat quat)
{
	float csx[2],csy[2],csz[2],cc,cs,sc,ss, cy, sy;

	// Convert angles to radians/2, construct the quat axes
	if (rot[0] == 0.0f)
	{
		csx[0] = 1.0f;
		csx[1] = 0.0f;
	}
	else
	{
		const float deg = rot[0] * (DEG2RAD * 0.5f);
		csx[0] = (float)cos (deg);
		csx[1] = (float)sin (deg);
	}

	if (rot[2] == 0.0f)
	{
		cc = csx[0];
		ss = 0.0f;
		cs = 0.0f;
		sc = csx[1];
	}
	else
	{
		const float deg = rot[2] * (DEG2RAD * 0.5f);
		csz[0] = (float)cos (deg);
		csz[1] = (float)sin (deg);
		cc = csx[0] * csz[0];
		ss = csx[1] * csz[1];
		cs = csx[0] * csz[1];
		sc = csx[1] * csz[0];
	}

	if (rot[1] == 0.0f)
	{
		quat[0] = sc;
		quat[1] = ss;
		quat[2] = cs;
		quat[3] = cc;
	}
	else
	{
		const float deg = rot[1] * (DEG2RAD * 0.5f);
		cy = csy[0] = (float)cos (deg);
		sy = csy[1] = (float)sin (deg);
		quat[0] = (cy * sc) - (sy * cs);
		quat[1] = (cy * ss) + (sy * cc);
		quat[2] = (cy * cs) - (sy * sc);
		quat[3] = (cy * cc) + (sy * ss);
	}

	// should be normal, if sin and cos are accurate enough
}

void _euler2quat (const float *rot, Quat quat)
{
	float csx[2],csy[2],csz[2],cc,cs,sc,ss, cy, sy;

	// Convert angles to radians/2, construct the quat axes
	if (rot[0] == 0.0f)
	{
		csx[0] = 1.0f;
		csx[1] = 0.0f;
	}
	else
	{
		const float deg = rot[0] * (DEG2RAD * 0.5f);
		_sincos (deg, csx);
	}

	if (rot[2] == 0.0f)
	{
		cc = csx[0];
		ss = 0.0f;
		cs = 0.0f;
		sc = csx[1];
	}
	else
	{
		const float deg = rot[2] * (DEG2RAD * 0.5f);
		_sincos (deg, csz);
		cc = csx[0] * csz[0];
		ss = csx[1] * csz[1];
		cs = csx[0] * csz[1];
		sc = csx[1] * csz[0];
	}

	if (rot[1] == 0.0f)
	{
		quat[0] = sc;
		quat[1] = ss;
		quat[2] = cs;
		quat[3] = cc;
	}
	else
	{
		const float deg = rot[1] * (DEG2RAD * 0.5f);
		_sincos (deg, csy);
		cy = csy[0];
		sy = csy[1];
		quat[0] = (cy * sc) - (sy * cs);
		quat[1] = (cy * ss) + (sy * cc);
		quat[2] = (cy * cs) - (sy * sc);
		quat[3] = (cy * cc) + (sy * ss);
	}

	// should be normal, if sin and cos are accurate enough
}

/* euler2quat2 - construct a quaternion by applying the given euler angles
 *			in X-Y-Z order (pitch-yaw-roll).
 *			Less efficient than euler2quat(), but more easily modified for
 *			different rotation orders.
 *      rot     - the euler angles (3 elements)
 *      quat    - the resultant quaternion
 */
void euler2quat2 (const float *rot, Quat quat)
{
	Quat	qx, qy, qz, qf;
	float	deg;

	// Convert angles to radians (and half-angles), and compute partial quats
	deg = rot[0] * 0.5f * DEG2RAD;
	qx[0] = (float)sin (deg);
	qx[1] = 0.0f; 
	qx[2] = 0.0f; 
	qx[3] = (float)cos (deg);

	deg = rot[1] * 0.5f * DEG2RAD;
	qy[0] = 0.0f; 
	qy[1] = (float)sin (deg);
	qy[2] = 0.0f;
	qy[3] = (float)cos (deg);

	deg = rot[2] * 0.5f * DEG2RAD;
	qz[0] = 0.0f;
	qz[1] = 0.0f;
	qz[2] = (float)sin (deg);
	qz[3] = (float)cos (deg);

	mult_quat (qf, qx, qy);
	mult_quat (quat, qz, qf);

	// should be normal, if _sincos and quat_mult are accurate enough
}


/* quat2axis_angle - construct the axis-angle representation of the quaternion
 *      quat        - the quaternion
 *      axisAngle	- 4 element array to hold the <x,y,z> axis and w angle
 */
void quat2axis_angle (const Quat quat, float *axisAngle)
{
	const float tw = (float)acos (quat[3]);
	const float scale = 1.0f / (float)sin (tw);

	axisAngle[0] = quat[0] * scale;
	axisAngle[1] = quat[1] * scale;
	axisAngle[2] = quat[2] * scale;

	// NOW CONVERT THE ANGLE OF ROTATION BACK TO DEGREES
	axisAngle[3] = tw * 2.0f * RAD2DEG;
}

void _quat2axis_angle (const Quat quat, float *axisAngle)
{
	const float tw = _acos (quat[3]);
	const float scale = 1.0f / _sin (tw);

	axisAngle[0] = quat[0] * scale;
	axisAngle[1] = quat[1] * scale;
	axisAngle[2] = quat[2] * scale;

	// NOW CONVERT THE ANGLE OF ROTATION BACK TO DEGREES
	axisAngle[3] = tw * 2.0f * RAD2DEG;
}


/* axis_angle2quat - construct the quaternion representation of the given axis&angle
 *      axisAngle	- 4 element array of the <x,y,z> axis and w angle
 *      quat		- the resultant quaternion
 */
void axis_angle2quat (const float *axisAngle, Quat quat)
{
	const float deg = axisAngle[3] / (2.0f * RAD2DEG);
	const float cs = (float)cos (deg);
	quat[3] = (float)sin (deg);
	quat[0] = cs * axisAngle[0];
	quat[1] = cs * axisAngle[1];
	quat[2] = cs * axisAngle[2];
}


#define DELTA	0.0001		// DIFFERENCE AT WHICH TO LERP INSTEAD OF SLERP
/* slerp_quat - spherically interpolate between quat1 and quat2
 *		quat1, quat2	- points to interpolate through
 *		slerp			- interpolation factor, 0 = quat1, 1 = quat2.
 *		result			- the resultant quaternion
 */
void slerp_quat (const Quat quat1, const Quat quat2, float slerp, Quat result)
{
	double omega, cosom, isinom;
	float scale0, scale1;
	float q2x, q2y, q2z, q2w;

	// DOT the quats to get the cosine of the angle between them
	cosom =	quat1[0] * quat2[0] +
			quat1[1] * quat2[1] +
			quat1[2] * quat2[2] +
			quat1[3] * quat2[3];

	// Two special cases:
	// Quats are exactly opposite, within DELTA?
	if (cosom > DELTA - 1.0) {
		// make sure they are different enough to avoid a divide by 0
		if (cosom < 1.0 - DELTA) {
			// SLERP away
			omega = acos (cosom);
			isinom = 1.0 / sin (omega);
			scale0 = (float)(sin ((1.0 - slerp) * omega) * isinom);
			scale1 = (float)(sin (slerp * omega) * isinom);
		} else {
			// LERP is good enough at this distance
			scale0 = 1.0f - slerp;
			scale1 = slerp;
		}

		q2x = quat2[0] * scale1;
		q2y = quat2[1] * scale1;
		q2z = quat2[2] * scale1;
		q2w = quat2[3] * scale1;
	} else {
		// SLERP towards a perpendicular quat
		// Set slerp parameters
		scale0 = (float)sin ((1.0f - slerp) * HALF_PI);
		scale1 = (float)sin (slerp * HALF_PI);

		q2x = -quat2[1] * scale1;
		q2y = quat2[0] * scale1;
		q2z = -quat2[3] * scale1;
		q2w = quat2[2] * scale1;
	}

	// Compute the result
	result[0] = scale0 * quat1[0] + q2x;
	result[1] = scale0 * quat1[1] + q2y;
	result[2] = scale0 * quat1[2] + q2z;
	result[3] = scale0 * quat1[3] + q2w;
}


/* trans_quat - rotate a point with a quat.  Note that this is equivalent
 *			to using quat2mat to make a rotation matrix, and then multiplying
 *			the vector by the matrix.  This form is more compact, and equally
 *			efficient when only transforming a single vector.  For other cases,
 *			it is advisable to construct the rotation matrix.
 *      result	- the resultant point (must point to 3 float array)
 *      q		- the quaternion for rotation
 *		v		- the vector/point to rotate (must have at least 3 elements)
 */
void trans_quat (float *result, const Quat q, const float *v)
{
	// result = av + bq + c(q.v CROSS v)
	// where
	//	a = q.w^2 - (q.v DOT q.v)
	//	b = 2 * (q.v DOT v)
	//	c = 2q.w
	float	w = q[3];	// just a convenience name
	float	a = w * w - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
	float   b = 2.0f * (q[0] * v[0] + q[1] * v[1] + q[2] * v[2]);
	float	c = 2.0f * w;

	// Must store this, because result may alias v
	float cross[3];	// q.v CROSS v
	cross[0] = q[1] * v[2] - q[2] * v[1];
	cross[1] = q[2] * v[0] - q[0] * v[2];
	cross[2] = q[0] * v[1] - q[1] * v[0];

	result[0] = a * v[0] + b * q[0] + c * cross[0];
	result[1] = a * v[1] + b * q[1] + c * cross[1];
	result[2] = a * v[2] + b * q[2] + c * cross[2];
}


// eof - quat_lib.c
