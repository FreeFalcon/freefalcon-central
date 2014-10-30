/*
+---------------------------------------------------------------------------+
|    Math.c                                                                 |
+---------------------------------------------------------------------------+
|    Description:  This file contains general math library                  |
+---------------------------------------------------------------------------+
|    Created by Erick Jap                              November 22, 1993    |
+---------------------------------------------------------------------------+
                 Copyright (c) 1993  Spectrum Holobyte, Inc.
*/

/*
Note:
----
New Matrix --> Old Matrix ==> new M(i),(j) = old M(i-1),(j-1)
new M11 = old M33   new M12 = old M31   new M13 = old M32
new M21 = old M13   new M22 = old M11   new M23 = old M12
new M31 = old M23   new M32 = old M21   new M33 = old M22

Inverse matrix:
New Matrix --> Old Matrix ==> new M(i),(j) = old M(i+1),(j+1)
*/

//___________________________________________________________________________

#include "stdafx.h"
#include "grmath.h"
#include "grinline.h"
#include "costable.h"


//___________________________________________________________________________


/*** Let's get up to speed in the Pentium and Pentium-Pro world - MBR ***/

#define glArcCos(NUM) atan2(sqrt(1-(NUM)*(NUM)), (NUM))
#define glArcSin(NUM) atan2((NUM), sqrt(1-(NUM)*(NUM)))

/************************************************************************/




/*
+---------------------------------------------------------------------------+
|    glGetSinCos                                                            |
+---------------------------------------------------------------------------+
|    Description:  calculate sin and cos of angle                           |
|                                                                           |
|    Input:        angle (0-16384 --> 4096 = 90 degrees)                    |
|                                                                           |
|    Output:       sinOut = offset to sin                                   |
|                  cosOut = offset to cos                                   |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                                May 27, 1993    |
+---------------------------------------------------------------------------+
*/
void
glGetSinCos(GLfloat *sinOut, GLfloat *cosOut, GLFixed0_14 angle)
{
    angle and_eq 0x3fff;

    if (angle bitand 0x1000)
    {
        // angle between 270-360
        if (angle bitand 0x2000)
        {
            *sinOut = (GLfloat) - CosineTable[angle - 0x3000];
            *cosOut = (GLfloat) CosineTable[0x4000 - angle];
        }
        // angle between 90-180
        else
        {
            *sinOut = (GLfloat) CosineTable[angle - 0x1000];
            *cosOut = (GLfloat) - CosineTable[0x2000 - angle];
        }
    }
    else
    {
        if (angle bitand 0x2000)
        {
            // angle between 180-270
            *sinOut = (GLfloat) - CosineTable[0x3000 - angle];
            *cosOut = (GLfloat) - CosineTable[angle - 0x2000];
        }
        else
        {
            // angle between 0-90
            *sinOut = (GLfloat) CosineTable[0x1000 - angle];
            *cosOut = (GLfloat) CosineTable[angle];
        }
    }
}

void
glGetSinCos(GLdouble *sinOut, GLdouble *cosOut, GLFixed0_14 angle)
{
    angle and_eq 0x3fff;

    if (angle bitand 0x1000)
    {
        // angle between 270-360
        if (angle bitand 0x2000)
        {
            *sinOut = -CosineTable[angle - 0x3000];
            *cosOut = CosineTable[0x4000 - angle];
        }
        // angle between 90-180
        else
        {
            *sinOut = CosineTable[angle - 0x1000];
            *cosOut = -CosineTable[0x2000 - angle];
        }
    }
    else
    {
        if (angle bitand 0x2000)
        {
            // angle between 180-270
            *sinOut = -CosineTable[0x3000 - angle];
            *cosOut = -CosineTable[angle - 0x2000];
        }
        else
        {
            // angle between 0-90
            *sinOut = CosineTable[0x1000 - angle];
            *cosOut = CosineTable[angle];
        }
    }
} /* glGetSinCos */



GLdouble
glGetSine(GLFixed0_14 angle)
{
    angle and_eq 0x3fff;

    if (angle bitand 0x1000)
    {
        // angle between 270-360
        if (angle bitand 0x2000)
        {
            return -CosineTable[angle - 0x3000];
        }
        // angle between 90-180
        else
        {
            return CosineTable[angle - 0x1000];
        }
    }
    else
    {
        if (angle bitand 0x2000)
        {
            // angle between 180-270
            return -CosineTable[0x3000 - angle];
        }
        else
        {
            // angle between 0-90
            return CosineTable[0x1000 - angle];
        }
    }
} /* glGetSine */



GLdouble
glGetCosine(GLFixed0_14 angle)
{
    angle and_eq 0x3fff;

    if (angle bitand 0x1000)
    {
        // angle between 270-360
        if (angle bitand 0x2000)
        {
            return CosineTable[0x4000 - angle];
        }
        // angle between 90-180
        else
        {
            return -CosineTable[0x2000 - angle];
        }
    }
    else
    {
        if (angle bitand 0x2000)
        {
            // angle between 180-270
            return -CosineTable[angle - 0x2000];
        }
        else
        {
            // angle between 0-90
            return CosineTable[angle];
        }
    }
} /* glGetCosine */


/*
+---------------------------------------------------------------------------+
|    CalculateArcTan                                                        |
+---------------------------------------------------------------------------+
|    Description:  Calculate arc tangent of two vectors                     |
|                                                                           |
|    Parameters:   opposite = vector at opposite                            |
|                  adjacent = vector at adjacent                            |
|                                                                           |
|    Returns:      arc tangent (4096 = 90 degrees)                          |
|                                                                           |
|    Note:         Y = opposite                                             |
|                  X = adjacent                                             |
|    Formula:      if (Y < X) {                                             |
|                     A = Y/X * 16384;                                      |
|                     A = A + (16384-A)*A/(128*8*45);                       |
|                     A = A/8;                                              |
|                  }                                                        |
|                  else {                                                   |
|                     A = X/Y * 16384;                                      |
|                     A = A + (16384-A)*A/(128*8*45);                       |
|                     A = 4096 - A/8;                                       |
|                  }                                                        |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                           November 29, 1993    |
+---------------------------------------------------------------------------+
*/

GLFixed0_14
CalculateArcTan(GLfloat opposite,
                GLfloat adjacent)
{
    GLFixed0_14 angle;

    if (opposite < adjacent)
    {
        angle = FloatToInt32((opposite * 16384.0f) / adjacent);
        angle += (((16384 - angle) * angle) / 46080l);
        angle >>= 3;
    }
    else
    {
        angle = FloatToInt32((adjacent * 16384.0f) / opposite);
        angle += (((16384 - angle) * angle) / 46080l);
        angle = 4096 - (angle >> 3);
    }

    return (angle);
} /* CalculateArcTan */



/*
+---------------------------------------------------------------------------+
|    glCalculateAngle                                                       |
+---------------------------------------------------------------------------+
|    Description:  Calculate angle between two vectors                      |
|                                                                           |
|    Parameters:   opposite = vector at opposite                            |
|                  adjacent = vector at adjacent                            |
|                                                                           |
|    Returns:      angle (16384 = 360 degrees)                              |
|                                                                           |
|    Note:                      bitor 90    /|                                  |
|                               bitor  I  /  bitor                                  |
|                       II      bitor   /    bitor opp                              |
|                               bitor / adj  bitor                                  |
|               180 ------------+------------ 0                             |
|                               bitor                                           |
|                       III     bitor    IV                                     |
|                               bitor                                           |
|                               bitor 270                                       |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                           November 29, 1993    |
+---------------------------------------------------------------------------+
*/

GLFixed0_14
glCalculateAngle(GLfloat opposite,
                 GLfloat adjacent)
{
    GLint       sign_opp, sign_adj;
    GLFixed0_14 angle;

    if ( not opposite)
    {
        if (adjacent < 0.0f) return (8192); // 180 degree

        return(0);
    }

    if ( not adjacent)
    {
        if (opposite < 0.0f) return (12288); // 270 degree

        return(4096); // 90 degree
    }

    if (opposite < 0.0f)
    {
        sign_opp = 1;
        opposite = -opposite;
    }
    else sign_opp = 0;

    if (adjacent < 0.0f)
    {
        sign_adj = 1;
        adjacent = -adjacent;
    }
    else sign_adj = 0;

    angle = CalculateArcTan(opposite, adjacent);

    if ( not angle)   // either 0 or 180
    {
        if (sign_adj) return (8192); // 180 degree

        return(0);
    }

    if (sign_opp)   // quadrant 3 or 4
    {
        if (sign_adj) angle += 8192; // quadrant 3
        else angle = 16384 - angle; // quadrant 4
    }
    else if (sign_adj) angle = 8192 - angle; // quadrant 2

    return (angle); // quadrant 1
} /* glCalculateAngle */
