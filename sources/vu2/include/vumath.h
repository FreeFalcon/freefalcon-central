#ifndef _VUMATH_H_
#define _VUMATH_H_

#define _USE_MATH_DEFINES
#include "math.h"

#define VU_PI		((SM_SCALAR)3.1415927)
#define VU_TWOPI	((SM_SCALAR)6.2831853)

typedef float BIG_SCALAR;
typedef float SM_SCALAR;

typedef SM_SCALAR VU_QUAT[4];
typedef SM_SCALAR VU_VECT[3];

#ifdef __cplusplus
inline float vuabs(float val)
{
  return (float)fabs(val);
}
#else
#define vuabs(val) ((val) > 0 ? (val) : -(val))
#endif

#ifdef __cplusplus
inline float vumax(float val1, float val2)
{
	return (val1 > val2 ? val1 : val2);
}
inline float vumin(float val1, float val2)
{
	return (val1 < val2 ? val1 : val2);
}
#else
#define vumax(val1, val2) ((val1) > (val2) ? (val1) : (val2))
#define vumin(val1, val2) ((val1) < (val2) ? (val1) : (val2))
#endif

#endif // _VUMATH_H_
