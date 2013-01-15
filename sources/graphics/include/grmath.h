#ifndef _3DEJ_MATH_H_
#define _3DEJ_MATH_H_

#include "define.h"

GLdouble glGetSine (GLFixed0_14 angle);
GLdouble glGetCosine (GLFixed0_14 angle);
void	glGetSinCos (GLfloat *sinOut, GLfloat *cosOut, GLFixed0_14 angle);
void 	glGetSinCos (GLdouble *sinOut, GLdouble *cosOut, GLFixed0_14 angle);

GLFixed0_14	CalculateArcTan (GLfloat opposite, GLfloat adjacent);
GLFixed0_14	glCalculateAngle (GLfloat opposite, GLfloat adjacent);
#endif
