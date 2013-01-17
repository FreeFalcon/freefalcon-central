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
 * AQUAT.H
 *
 * AMD3D 3D library code: Quaternion Math
 *
 *      BETA RELEASE
 *
 *****************************************************************************/

#ifndef __QUAT_H__
#define __QUAT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef float Quat[4];

// C quaternion functions
void add_quat (Quat dest, const Quat a, const Quat b);
void sub_quat (Quat dest, const Quat a, const Quat b);
void mat2quat (const float m[4][4], Quat quat);
void quat2mat (const Quat quat, float m[4][4]);
void mult_quat(Quat dst, const Quat q1, const Quat q2);
void norm_quat(Quat dst, const Quat quat);
void euler2quat  (const float *rot, Quat quat);
void euler2quat2 (const float *rot, Quat quat);
void quat2axis_angle (const Quat quat, float *axisAngle);
void axis_angle2quat (const float *axisAngle, Quat quat);
void slerp_quat (const Quat quat1, const Quat quat2, float slerp, Quat result);
void trans_quat (float *result, const Quat q, const float *v);

// 3DNow! accelerated quat functions
void _add_quat (Quat dest, const Quat a, const Quat b);
void _sub_quat (Quat dest, const Quat a, const Quat b);
void _mat2quat (const float m[4][4], Quat quat);
void _quat2mat (const Quat quat, float m[4][4]);
void _mult_quat(Quat dst, const Quat q1, const Quat q2);
void _norm_quat(Quat dst, const Quat quat);
void _euler2quat (const float *rot, Quat quat);
void _quat2axis_angle (const Quat quat, float *axisAngle);
void _axis_angle2quat (const float *axisAngle, Quat quat);
void _slerp_quat (const Quat quat1, const Quat quat2, float slerp, Quat result);
void _trans_quat (float *result, const Quat q, const float *v);

#ifdef __cplusplus
}
#endif

#endif

// eof