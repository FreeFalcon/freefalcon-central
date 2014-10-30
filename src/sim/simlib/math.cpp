/******************************************************************************/
/*                                                                            */
/*  Unit Name : math.cpp                                                      */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the SIMLIB_MATH_CLASS. */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "simmath.h"

/*----------------------------------------------------*/
/* Memory Allocation for externals declared elsewhere */
/*----------------------------------------------------*/
SIM_INT SimLibErrno;
SIMLIB_MATH_CLASS Math;

/*-------------------*/
/* Private Functions */
/*-------------------*/

/*------------------*/
/* Public Functions */
/*------------------*/

/********************************************************************/
/*                                                                  */
/* Routine: SIM_SHORT  SIMLIB_MATH_CLASS::Limit (SIM_SHORT,         */
/*          SIM_SHORT, SIM_SHORT)                                   */
/*                                                                  */
/* Description:                                                     */
/*    Limit the input to be between the minimum and the maximum.    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_SHORT input - Value to be limited                         */
/*    SIM_SHORT min_val   - Mimimum possible value                  */
/*    SIM_SHORT max_val   - Maximum possible value                  */
/*                                                                  */
/* Outputs:                                                         */
/*    Input limited such that min_val <= input <= max_val           */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_SHORT  SIMLIB_MATH_CLASS::Limit(SIM_SHORT input, SIM_SHORT min_val,
                                    SIM_SHORT max_val)
{
    return (min(max(input, min_val), max_val));
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT  SIMLIB_MATH_CLASS::Limit (SIM_INT,             */
/*          SIM_INT, SIM_INT)                                       */
/*                                                                  */
/* Description:                                                     */
/*    Limit the input to be between the minimum and the maximum.    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT input - Value to be limited                           */
/*    SIM_INT min_val   - Mimimum possible value                    */
/*    SIM_INT max_val   - Maximum possible value                    */
/*                                                                  */
/* Outputs:                                                         */
/*    Input limited such that min_val <= input <= max_val           */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SIMLIB_MATH_CLASS::Limit(SIM_INT input, SIM_INT min_val,
                                 SIM_INT max_val)
{
    return (min(max(input, min_val), max_val));
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_LONG  SIMLIB_MATH_CLASS::Limit (SIM_LONG,           */
/*          SIM_LONG, SIM_LONG)                                     */
/*                                                                  */
/* Description:                                                     */
/*    Limit the input to be between the minimum and the maximum.    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_LONG input - Value to be limited                          */
/*    SIM_LONG min_val   - Mimimum possible value                   */
/*    SIM_LONG max_val   - Maximum possible value                   */
/*                                                                  */
/* Outputs:                                                         */
/*    Input limited such that min_val <= input <= max_val           */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_LONG   SIMLIB_MATH_CLASS::Limit(SIM_LONG input, SIM_LONG min_val,
                                    SIM_LONG max_val)
{
    return (min(max(input, min_val), max_val));
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT  SIMLIB_MATH_CLASS::Limit (SIM_FLOAT,         */
/*          SIM_FLOAT, SIM_FLOAT)                                   */
/*                                                                  */
/* Description:                                                     */
/*    Limit the input to be between the minimum and the maximum.    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT input - Value to be limited                         */
/*    SIM_FLOAT min_val   - Mimimum possible value                  */
/*    SIM_FLOAT max_val   - Maximum possible value                  */
/*                                                                  */
/* Outputs:                                                         */
/*    Input limited such that min_val <= input <= max_val           */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT  SIMLIB_MATH_CLASS::Limit(SIM_FLOAT input, SIM_FLOAT min_val,
                                    SIM_FLOAT max_val)
{
    return (min(max(input, min_val), max_val));
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_DOUBLE  SIMLIB_MATH_CLASS::Limit (SIM_DOUBLE,       */
/*          SIM_DOUBLE, SIM_DOUBLE)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Limit the input to be between the minimum and the maximum.    */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_DOUBLE input - Value to be limited                        */
/*    SIM_DOUBLE min_val   - Mimimum possible value                 */
/*    SIM_DOUBLE max_val   - Maximum possible value                 */
/*                                                                  */
/* Outputs:                                                         */
/*    Input limited such that min_val <= input <= max_val           */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_DOUBLE SIMLIB_MATH_CLASS::Limit(SIM_DOUBLE input, SIM_DOUBLE min_val,
                                    SIM_DOUBLE max_val)
{
    return (min(max(input, min_val), max_val));
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT  SIMLIB_MATH_CLASS::RateLimit (SIM_FLOAT,     */
/*                 SIM_FLOAT, SIM_FLOAT, SIM_FLOAT)                 */
/*                                                                  */
/* Description:                                                     */
/*    Limit a position based on current position and maximum rate.  */
/*    The rate used is set.                                         */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT input    - Desired position                         */
/*    SIM_FLOAT cur      - Current position                         */
/*    SIM_FLOAT max_rate - Max rate allowed                         */
/*    SIM_FLOAT *rate    - rate actually achieved                   */
/*                                                                  */
/* Outputs:                                                         */
/*    New position limited be rate.                                 */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT  SIMLIB_MATH_CLASS::RateLimit(SIM_FLOAT input, SIM_FLOAT cur,
                                        SIM_FLOAT max_rate, SIM_FLOAT *rate, SIM_FLOAT delt)
{
    *rate = (input - cur) / delt;
    *rate = Limit(*rate, -max_rate, max_rate);
    return (cur + *rate * delt);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_DOUBLE  SIMLIB_MATH_CLASS::RateLimit (SIM_DOUBLE,   */
/*                 SIM_DOUBLE, SIM_DOUBLE, SIM_DOUBLE)              */
/*                                                                  */
/* Description:                                                     */
/*    Limit a position based on current position and maximum rate.  */
/*    The rate used is set.                                         */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_DOUBLE input    - Desired position                        */
/*    SIM_DOUBLE cur      - Current position                        */
/*    SIM_DOUBLE max_rate - Max rate allowed                        */
/*    SIM_DOUBLE *rate    - rate actually achieved                  */
/*                                                                  */
/* Outputs:                                                         */
/*    New position limited be rate.                                 */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_DOUBLE SIMLIB_MATH_CLASS::RateLimit(SIM_DOUBLE input, SIM_DOUBLE cur,
                                        SIM_DOUBLE max_rate, SIM_DOUBLE *rate, SIM_DOUBLE delt)
{
    *rate = (input - cur) / delt;
    *rate = Limit(*rate, -max_rate, max_rate);
    return (cur + *rate * delt);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::DeadBand (SIM_FLOAT,       */
/*            SIM_FLOAT, SIM_FLOAT)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Add a breakout value to an input.                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT input        - Current input                        */
/*    SIM_FLOAT min_breakout - Minimum breakout value               */
/*    SIM_FLOAT max_breakout - Maximum breakout value               */
/*                                                                  */
/* Outputs:                                                         */
/*    Input subjected to breakout                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::DeadBand(SIM_FLOAT input, SIM_FLOAT min_breakout,
                                      SIM_FLOAT max_breakout)
{
    if (input > max_breakout)
        input -= max_breakout;
    else if (input < min_breakout)
        input -= min_breakout;
    else
        input = 0.0F;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_DOUBLE SIMLIB_MATH_CLASS::DeadBand (SIM_DOUBLE,     */
/*            SIM_DOUBLE, SIM_DOUBLE)                               */
/*                                                                  */
/* Description:                                                     */
/*    Add a breakout value to an input.                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_DOUBLE input        - Current input                       */
/*    SIM_DOUBLE min_breakout - Minimum breakout value              */
/*    SIM_DOUBLE max_breakout - Maximum breakout value              */
/*                                                                  */
/* Outputs:                                                         */
/*    Input subjected to breakout                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_DOUBLE SIMLIB_MATH_CLASS::DeadBand(SIM_DOUBLE input, SIM_DOUBLE min_breakout,
                                       SIM_DOUBLE max_breakout)
{
    if (input > max_breakout)
        input -= max_breakout;
    else if (input < min_breakout)
        input -= min_breakout;
    else
        input = 0.0;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT  SIMLIB_MATH_CLASS::Resolve (SIM_FLOAT,       */
/*              SIM_FLOAT)                                          */
/*                                                                  */
/* Description:                                                     */
/*    Mod an angle such that the result is between +- max.          */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT input     - Real angle                              */
/*    SIM_FLOAT max_angle - Limiting value                          */
/*                                                                  */
/* Outputs:                                                         */
/*    Returns the equvalent angle such that -max_angle <= return <= */
/*      max_angle                                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT  SIMLIB_MATH_CLASS::Resolve(SIM_FLOAT input, SIM_FLOAT max_angle)
{
    register SIM_FLOAT delta;

    delta = 2.0F * max_angle;

    while (input >= max_angle)
        input -= delta;

    while (input < -max_angle)
        input += delta;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_DOUBLE  SIMLIB_MATH_CLASS::Resolve (SIM_DOUBLE,     */
/*              SIM_DOUBLE)                                         */
/*                                                                  */
/* Description:                                                     */
/*    Mod an angle such that the result is between +- max.          */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_DOUBLE input     - Real angle                             */
/*    SIM_DOUBLE max_angle - Limiting value                         */
/*                                                                  */
/* Outputs:                                                         */
/*    Returns the equvalent angle such that -max_angle <= return <= */
/*      max_angle                                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_DOUBLE SIMLIB_MATH_CLASS::Resolve(SIM_DOUBLE input, SIM_DOUBLE max_angle)
{
    register SIM_DOUBLE delta;

    delta = 2.0F * max_angle;

    while (input >= max_angle)
        input -= delta;

    while (input < -max_angle)
        input += delta;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT  SIMLIB_MATH_CLASS::Resolve0 (SIM_FLOAT,      */
/*              SIM_FLOAT)                                          */
/*                                                                  */
/* Description:                                                     */
/*    Mod an angle such that the result is between 0.0 and max      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT input     - Real angle                              */
/*    SIM_FLOAT max_angle - Limiting value                          */
/*                                                                  */
/* Outputs:                                                         */
/*    Returns the equvalent angle such that 0.0 <= return <=        */
/*      max_angle                                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT  SIMLIB_MATH_CLASS::Resolve0(SIM_FLOAT input, SIM_FLOAT max_angle)
{
    while (input >= max_angle)
        input -= max_angle;

    while (input < 0.0F)
        input += max_angle;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_DOUBLE  SIMLIB_MATH_CLASS::Resolve0 (SIM_DOUBLE,    */
/*              SIM_DOUBLE)                                         */
/*                                                                  */
/* Description:                                                     */
/*    Mod an angle such that the result is between 0.0 and max      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_DOUBLE input     - Real angle                             */
/*    SIM_DOUBLE max_angle - Limiting value                         */
/*                                                                  */
/* Outputs:                                                         */
/*    Returns the equvalent angle such that 0.0 <= return <=        */
/*      max_angle                                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_DOUBLE SIMLIB_MATH_CLASS::Resolve0(SIM_DOUBLE input, SIM_DOUBLE max_angle)
{
    while (input >= max_angle)
        input -= max_angle;

    while (input < 0.0F)
        input += max_angle;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT  SIMLIB_MATH_CLASS::ResolveWrap (SIM_FLOAT,   */
/*              SIM_FLOAT)                                          */
/*                                                                  */
/* Description:                                                     */
/*    Mod an angle such that the result is between +- max, with a   */
/*    bounce at the end (such as Theta)                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT input     - Real angle                              */
/*    SIM_FLOAT max_angle - Limiting value                          */
/*                                                                  */
/* Outputs:                                                         */
/*    Returns the equvalent angle such that -max_angle <= return <= */
/*      max_angle                                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::ResolveWrap(SIM_FLOAT input, SIM_FLOAT max_angle)
{
    input = Resolve(input, 2.0F * max_angle);

    if (input > max_angle)
        input = 2.0F * max_angle - input;
    else if (input < -max_angle)
        input = -2.0F * max_angle - input;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_DOUBLE  SIMLIB_MATH_CLASS::ResolveWrap (SIM_DOUBLE, */
/*              SIM_DOUBLE)                                         */
/*                                                                  */
/* Description:                                                     */
/*    Mod an angle such that the result is between +- max, with a   */
/*    bounce at the end (such as Theta)                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_DOUBLE input     - Real angle                             */
/*    SIM_DOUBLE max_angle - Limiting value                         */
/*                                                                  */
/* Outputs:                                                         */
/*    Returns the equvalent angle such that -max_angle <= return <= */
/*      max_angle                                                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_DOUBLE SIMLIB_MATH_CLASS::ResolveWrap(SIM_DOUBLE input, SIM_DOUBLE max_angle)
{
    input = Resolve(input, 2.0 * max_angle);

    if (input > max_angle)
        input -= max_angle;
    else if (input < -max_angle)
        input += max_angle;

    return (input);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F1Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT,            */
/*           SAVE_ARRAY, SIM_INT *)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Notch filter routine                                          */
/*     y(s)    s**2 + 2*zeta1*omega1*s + omega1**2                  */
/*     ---  =  ------------------------------------                 */
/*     u(s)    s**2 + 2*zeta2*omega2*s + omega2**2                  */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in     - Current Input                              */
/*    SIM_FLOAT zeta1  - Low Damping                                */
/*    SIM_FLOAT omega1 - Low Frequency                              */
/*    SIM_FLOAT zeta2  - Hi Damping                                 */
/*    SIM_FLOAT omega2 - Hi Frequency                               */
/*    SIM_FLOAT delt   - Frame Rate (Sec)                           */
/*    SAVE_ARRAY save  - Data History                               */
/*    SIM_INT *jstart  - Frame count                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Notch filtered input.                                         */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F1Tust(SIM_FLOAT in, SIM_FLOAT zeta1, SIM_FLOAT omega1,
                                    SIM_FLOAT zeta2, SIM_FLOAT omega2, SIM_FLOAT delt, SAVE_ARRAY save, SIM_INT *jstart)
{
    SIM_FLOAT x1, x2, y1, y2, a1, a2, b1, b2, k;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    x1 = zeta1 * omega1;
    x2 = zeta2 * omega2;
    y1 = omega1 * (SIM_FLOAT)sqrt(1 - zeta1 * zeta1);
    y2 = omega2 * (SIM_FLOAT)sqrt(1 - zeta2 * zeta2);

    a1 = 2 * (SIM_FLOAT)exp(-x1 * delt) * (SIM_FLOAT)cos(y1 * delt);
    a2 = 2 * (SIM_FLOAT)exp(-x2 * delt) * (SIM_FLOAT)cos(y2 * delt);
    b1 = (SIM_FLOAT)exp(-2 * x1 * delt);
    b2 = (SIM_FLOAT)exp(-2 * x2 * delt);
    k  = (1 - a2 + b2) / (1 - a1 + b1) * (omega1 / omega2) * (omega1 / omega2);

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[4] = k * (save[5] - a1 * save[4] + b1 * save[3]) + a2 * save[1] - b2 * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    if (*jstart >= 2)
    {
        save[0] = save[1];
        save[3] = save[4];
    }

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    if (*jstart >= 1)
    {
        save[1] = save[2];
        save[4] = save[5];
    }

    *jstart = *jstart + 1;

    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F2Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)           */
/*                                                                  */
/* Description:                                                     */
/*    Lead-lag filter routine:                                      */
/*                                                                  */
/*    y(s)   tau1*s + 1                                             */
/*    ---- = ----------                                             */
/*    u(s)   tau2*s + 1                                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau1  - Lead time constant                          */
/*    SIM_FLOAT tau2  - Lag time constant                           */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F2Tust(SIM_FLOAT in, SIM_FLOAT tau1, SIM_FLOAT tau2,
                                    SIM_FLOAT delt, SAVE_ARRAY save)
{
    SIM_FLOAT k1, k2, k;

    /*-----------------------*/
    /* equation coefficients */
    /*-----------------------*/
    k1 = (SIM_FLOAT)exp(-delt / tau1);
    k2 = (SIM_FLOAT)exp(-delt / tau2);
    k  = (1 - k2) / (1 - k1);

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[3] = in;
    save[1] = save[0] * k2 + k * (save[3] - save[2] * k1);

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[0] = save[1];
    save[2] = save[3];

    return((SIM_FLOAT)save[1]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F3Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)           */
/*                                                                  */
/* Description:                                                     */
/*    Second order filter routine:                                  */
/*                                                                  */
/*    y(s)               omega**2                                   */
/*    ---  =  --------------------------------                      */
/*    u(s)    s**2 + 2*zeta*omega*s + omega**2                      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT zeta  - Damping constant                            */
/*    SIM_FLOAT omega - Frequency (Hz)                              */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F3Tust(SIM_FLOAT in, SIM_FLOAT zeta, SIM_FLOAT omega,
                                    SIM_FLOAT delt, SAVE_ARRAY save)
{
    SIM_FLOAT a, b, c, d, k;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    a = zeta * omega;
    b = omega * (SIM_FLOAT)sqrt(1 - zeta * zeta);
    c = 2 * (SIM_FLOAT)exp(-a * delt) * (SIM_FLOAT)cos(b * delt);
    d = (SIM_FLOAT)exp(-2 * a * delt);
    k  = (1 - c + d) / 4.0F;

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[2] = k * (save[5] + 2.0F * save[4] + save[3]) + c * save[1] - d * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    save[0] = save[1];
    save[3] = save[4];

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[1] = save[2];
    save[4] = save[5];

    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F4Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT,            */
/*           SAVE_ARRAY, SIM_INT *)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Second order filter routine:                                  */
/*                                                                  */
/*    y(s)          omega**2(tau*s + 1)                             */
/*    ---  =  --------------------------------                      */
/*    u(s)    s**2 + 2*zeta*omega*s + omega**2                      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau   - Time constant                               */
/*    SIM_FLOAT zeta  - Damping constant                            */
/*    SIM_FLOAT omega - Frequency (Hz)                              */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*    SIM_INT *jstart - Frame count                                 */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F4Tust(SIM_FLOAT in, SIM_FLOAT tau, SIM_FLOAT zeta,
                                    SIM_FLOAT omega, SIM_FLOAT delt, SAVE_ARRAY save, SIM_INT *jstart)
{
    SIM_FLOAT a, b, c, d, e, k;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    a = zeta * omega;
    b = omega * (SIM_FLOAT)sqrt(1 - zeta * zeta);
    c = 2 * (SIM_FLOAT)exp(-a * delt) * (SIM_FLOAT)cos(b * delt);
    d = (SIM_FLOAT)exp(-2 * a * delt);
    e = (SIM_FLOAT)exp(-delt / tau);
    k  = (1 - c + d) / (2.0F * (1 - e));

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[2] = k * (save[5] + (1 - e) * save[4] - e * save[3]) + c * save[1] - d * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    if (*jstart >= 2)
    {
        save[0] = save[1];
        save[3] = save[4];
    }

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    if (*jstart >= 1)
    {
        save[1] = save[2];
        save[4] = save[5];
    }

    *jstart = *jstart + 1;
    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F5Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT,            */
/*           SAVE_ARRAY, SIM_INT *)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Second order filter routine:                                  */
/*                                                                  */
/*    y(s)         (1/tau)*s*(tau*s + 1)                            */
/*    ---  =  --------------------------------                      */
/*    u(s)    s**2 + 2*zeta*omega*s + omega**2                      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau   - Time constant                               */
/*    SIM_FLOAT zeta  - Damping constant                            */
/*    SIM_FLOAT omega - Frequency (Hz)                              */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*    SIM_INT *jstart - Frame count                                 */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F5Tust(SIM_FLOAT in, SIM_FLOAT tau, SIM_FLOAT zeta,
                                    SIM_FLOAT omega, SIM_FLOAT delt, SAVE_ARRAY save, SIM_INT *jstart)
{
    SIM_FLOAT a, b, c, d, e, k;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    a = zeta * omega;
    b = omega * (SIM_FLOAT)sqrt(1 - zeta * zeta);
    c = 2 * (SIM_FLOAT)exp(-a * delt) * (SIM_FLOAT)cos(b * delt);
    d = (SIM_FLOAT)exp(-2 * a * delt);
    e = (SIM_FLOAT)exp(-delt / tau);
    k  = (1 + c + d) / (2.0F * (1 + e));

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[2] = k * (save[5] - (1 + e) * save[4] + e * save[1]) + c * save[1] - d * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    if (*jstart >= 2)
    {
        save[0] = save[1];
        save[3] = save[4];
    }

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    if (*jstart >= 1)
    {
        save[1] = save[2];
        save[4] = save[5];
    }

    *jstart = *jstart + 1;
    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F6Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT,            */
/*           SAVE_ARRAY, SIM_INT *)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Second order filter routine:                                  */
/*                                                                  */
/*    y(s)              s*omega**2                                  */
/*    ---  =  --------------------------------                      */
/*    u(s)    s**2 + 2*zeta*omega*s + omega**2                      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT zeta  - Damping constant                            */
/*    SIM_FLOAT omega - Frequency (Hz)                              */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*    SIM_INT *jstart - Frame count                                 */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F6Tust(SIM_FLOAT in, SIM_FLOAT zeta, SIM_FLOAT omega,
                                    SIM_FLOAT delt, SAVE_ARRAY save, SIM_INT *jstart)
{
    SIM_FLOAT a1, b1, c1, d1, e1, a, b, c, d;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    a1 =  1.0F + delt * zeta * omega + 0.25F * (omega * delt) * (omega * delt);
    b1 = -2.0F + 0.5F * (omega * delt) * (omega * delt);
    c1 =  1.0F - delt * zeta * omega + 0.25F * (omega * delt) * (omega * delt);
    d1 =  0.5F * delt * omega * omega;
    e1 = -0.5F * delt * omega * omega;
    a  = b1 / a1;
    b  = c1 / a1;
    c  = d1 / a1;
    d  = e1 / a1;

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[2] = c * save[5] + d * save[3] - a * save[1] - b * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    if (*jstart >= 2)
    {
        save[0] = save[1];
        save[3] = save[4];
    }

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    if (*jstart >= 1)
    {
        save[1] = save[2];
        save[4] = save[5];
    }

    *jstart = *jstart + 1;
    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F7Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)*/
/*                                                                  */
/* Description:                                                     */
/*    Second order filter routine:                                  */
/*                                                                  */
/*    y(s)           (tau*s + 1)                                    */
/*    ---  =  -------------------------                             */
/*    u(s)    (tau2*s + 1)*(tau3*s + 1)                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau1  - Lead time constant                          */
/*    SIM_FLOAT tau2  - Lag1 time constant                          */
/*    SIM_FLOAT tau3  - Lag2 time constant                          */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*    SIM_INT *jstart - Frame count                                 */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F7Tust(SIM_FLOAT in, SIM_FLOAT tau1, SIM_FLOAT tau2,
                                    SIM_FLOAT tau3, SIM_FLOAT delt, SAVE_ARRAY save, SIM_INT *jstart)
{
    SIM_FLOAT a, b, c, d, k;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    a = -((SIM_FLOAT)exp(-delt / tau2) + (SIM_FLOAT)exp(-delt / tau3));
    b = (SIM_FLOAT)exp(-delt * (1 / tau2 + 1 / tau3));
    c =   1.0F - (SIM_FLOAT)exp(-delt / tau1);
    d =  -(SIM_FLOAT)exp(-delt / tau1);

    if (1.0F + c + d == 0.0F)
        k = 0.0F;
    else
        k = (1.0F + a + b) / (1.0F + c + d);


    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[2] = k * (save[5] + c * save[4] + d * save[3]) -  a * save[1] - b * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    if (*jstart >= 2)
    {
        save[0] = save[1];
        save[3] = save[4];
    }

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    if (*jstart >= 1)
    {
        save[1] = save[2];
        save[4] = save[5];
    }

    *jstart = *jstart + 1;
    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::F8Tust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)*/
/*                                                                  */
/* Description:                                                     */
/*    Second order filter routine:                                  */
/*                                                                  */
/*    y(s)          s*(tau*s + 1)                                   */
/*    ---  =  -------------------------                             */
/*    u(s)    (tau2*s + 1)*(tau3*s + 1)                             */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau1  - Lead time constant                          */
/*    SIM_FLOAT tau2  - Lag1 time constant                          */
/*    SIM_FLOAT tau3  - Lag2 time constant                          */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*    SIM_INT *jstart - Frame count                                 */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::F8Tust(SIM_FLOAT in, SIM_FLOAT tau1, SIM_FLOAT tau2,
                                    SIM_FLOAT tau3, SIM_FLOAT delt, SAVE_ARRAY save, SIM_INT *jstart)
{
    SIM_FLOAT a, b, c, d, k;

    /*----------------------------------*/
    /* compute z-transform coefficients */
    /*----------------------------------*/
    a = -((SIM_FLOAT)exp(-delt / tau2) + (SIM_FLOAT)exp(-delt / tau3));
    b = (SIM_FLOAT)exp(-delt * (1 / tau2 + 1 / tau3));
    c = -(1 + (SIM_FLOAT)exp(-delt / tau1));
    d = (SIM_FLOAT)exp(-delt / tau1);
    k = (1 - a + b) / (1 - c + d) * tau1 / (tau2 * tau3);

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[5] = in;
    save[2] = k * (save[5] + c * save[4] + d * save[3]) -  a * save[1] - b * save[0];

    /*----------------------------------*/
    /* save values from past two frames */
    /*----------------------------------*/
    if (*jstart >= 2)
    {
        save[0] = save[1];
        save[3] = save[4];
    }

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    if (*jstart >= 1)
    {
        save[1] = save[2];
        save[4] = save[5];
    }

    *jstart = *jstart + 1;
    return((SIM_FLOAT)save[2]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::FLTust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)                      */
/*                                                                  */
/* Description:                                                     */
/*    Lag filter routine:                                           */
/*                                                                  */
/*    y(s)       1                                                  */
/*    ---- = ---------                                              */
/*    u(s)   tau*s + 1                                              */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau   - Lag time constant                           */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::FLTust(SIM_FLOAT in, SIM_FLOAT tau, SIM_FLOAT delt,
                                    SAVE_ARRAY save)
{
    SIM_FLOAT k1, k2;

    /*-----------------------*/
    /* equation coefficients */
    /*-----------------------*/
    k1 = (float)exp(-delt / tau);
    k2 = (1 - k1) / 2.0F;

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[3] = in;
    save[1] = save[0] * k1 + k2 * (save[3] + save[2]);

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[0] = save[1];
    save[2] = save[3];

    return((SIM_FLOAT)save[1]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::FLeadTust( SIM_FLOAT,      */
/*           SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)                      */
/*                                                                  */
/* Description:                                                     */
/*    Lead filter routine:                                          */
/*                                                                  */
/*    y(s)   tau*s + 1                                              */
/*    ---- = ---------                                              */
/*    u(s)       1                                                  */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau   - Lead time constant                          */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::FLeadTust(SIM_FLOAT in, SIM_FLOAT tau, SIM_FLOAT delt,
                                       SAVE_ARRAY save)
{
    SIM_FLOAT k1, k2;

    /*-----------------------*/
    /* equation coefficients */
    /*-----------------------*/
    k1 = (float)exp(-delt / tau);
    k2 = 2.0F / (1.0F - k1);

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[3] = in;
    save[1] = save[0] * k1 + k2 * (save[3] - save[2]);

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[0] = save[1];
    save[2] = save[3];

    return((SIM_FLOAT)save[1]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::FITust( SIM_FLOAT,         */
/*           SIM_FLOAT, SAVE_ARRAY)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Integration routine:                                          */
/*                                                                  */
/*    y(s)   1                                                      */
/*    ---  = -                                                      */
/*    u(s)   s                                                      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::FITust(SIM_FLOAT in, SIM_FLOAT delt, SAVE_ARRAY save)
{
    /*----------------*/
    /* compute output */
    /*----------------*/
    save[3] = in;
    save[1] = save[0] + delt * (save[3] + save[2]) * 0.5F;

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[0] = save[1];
    save[2] = save[3];

    return((SIM_FLOAT)save[1]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::FIAdamsBash( SIM_FLOAT,         */
/*           SIM_FLOAT, SAVE_ARRAY)                                 */
/*                                                                  */
/* Description:                                                     */
/*    Integration routine:                                          */
/*                                                                  */
/*    y(s)   1                                                      */
/*    ---  = -                                                      */
/*    u(s)   s                                                      */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  29-Oct-98 DP                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::FIAdamsBash(SIM_FLOAT in, SIM_FLOAT delt, SAVE_ARRAY save)
{
    /*----------------*/
    /* compute output */
    /*----------------*/
    save[3] = in;
    save[1] = save[1] + delt * (save[3] * 3.0F - save[2]) * 0.5F;

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[0] = save[1];
    save[2] = save[3];

    return((SIM_FLOAT)save[1]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_FLOAT SIMLIB_MATH_CLASS::FWTust( SIM_FLOAT,         */
/*           SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY)                      */
/*                                                                  */
/* Description:                                                     */
/*    Integration routine:                                          */
/*                                                                  */
/*    y(s)     tau*s                                                */
/*    ---- = ---------                                              */
/*    u(s)   tau*s + 1                                              */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FLOAT in    - Current Input                               */
/*    SIM_FLOAT tau   - Time constant                               */
/*    SIM_FLOAT delt  - Frame Rate (Sec)                            */
/*    SAVE_ARRAY save - Data History                                */
/*                                                                  */
/* Outputs:                                                         */
/*    Filtered input                                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::FWTust(SIM_FLOAT in, SIM_FLOAT tau, SIM_FLOAT delt,
                                    SAVE_ARRAY save)
{
    SIM_FLOAT k1, k2;

    /*-----------------------*/
    /* equation coefficients */
    /*-----------------------*/
    k1 = (float)exp(-delt / tau);
    k2 = (1 + k1) / 2.0F;

    /*----------------*/
    /* compute output */
    /*----------------*/
    save[3] = in;
    save[1] = save[0] * k1 + k2 * (save[3] - save[2]);

    /*---------------------------------*/
    /* save values from the last frame */
    /*---------------------------------*/
    save[0] = save[1];
    save[2] = save[3];

    return((SIM_FLOAT)save[1]);
}
/*****************************************************************************/
/* ONED_INTERP                                                               */
/*                                                                           */
/* DESCRIPTION                                                               */
/* ------------------------------------------------------------------------- */
/*   one independent variable interpolation                                   */
/*                                                                           */
/* USED BY: manned, digi                                                     */
/*                                                                           */
/* PARAMETERS                     DESCRIPTION                                */
/* ------------------------------------------------------------------------- */
/*   IN: float x                  independent variable                       */
/*       float *xarry             pointer to the independ variable array     */
/*       float *data              pointer to 1-d array as a function of x    */
/*       int   numx               number of x breakpoints                    */
/*       int   *lastx             pointer to most recent x bkpt to speed     */
/*                                up the interpolation.                      */
/*  OUT: returns interpolated value                                          */
/*                                                                           */
/* MODIFICATIONS    DATE          DESCRIPTION                                */
/* ------------------------------------------------------------------------- */
/*     JC           1/2/92        initial write                              */
/*                                                                           */
/*****************************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::OnedInterp(SIM_FLOAT x, const SIM_FLOAT *xarray,
                                        const SIM_FLOAT *data, SIM_INT numx, SIM_INT *lastx)
{
    int i = 0;
    float x0 = 0.0F, dx = 0.0F, ddata = 0.0F;
    float xinpt = 0.0F, xmin = 0.0F, xmax = 0.0F;
    float final_val = static_cast<float>(*lastx);

    /*-----------------------*/
    /* limit to within table */
    /*-----------------------*/
    xmin = xarray[0];
    xmax = xarray[numx - 1];
    xinpt = min(max(x, xmin), xmax);

    /*-------------------------------------------------*/
    /* If there is a last x use it as a starting point */
    /*-------------------------------------------------*/
    if (*lastx not_eq 0)
    {
        if (xinpt >= xarray[*lastx] and xinpt <= xarray[*lastx + 1])
        {
            x0 = xarray[*lastx];
            dx = xarray[*lastx + 1] - x0;
            ddata = data[*lastx + 1] - data[*lastx];
            final_val =  data[*lastx] + (xinpt - x0) * ddata / dx;
        }
        else if (xinpt <= xarray[*lastx])
        {
            /*-----------------------*/
            /* Look down in the data */
            /*-----------------------*/
            for (i = *lastx; i > 0; i--)
            {
                if (xinpt <= xarray[i] and xinpt >= xarray[i - 1])
                {
                    *lastx = i - 1;
                    x0 = xarray[i - 1];
                    dx = xarray[i] - x0;
                    ddata = data[i] - data[i - 1];
                    final_val =  data[i - 1] + (xinpt - x0) * ddata / dx;
                    break;
                }
            }
        }
        else
        {
            for (i = *lastx + 1; i < numx - 1; i++)
            {
                if (xinpt >= xarray[i] and xinpt <= xarray[i + 1])
                {
                    *lastx = i;
                    x0 = xarray[i];
                    dx = xarray[i + 1] - x0;
                    ddata = data[i + 1] - data[i];
                    final_val = data[i] + (xinpt - x0) * ddata / dx;
                    break;
                }
            }
        }
    }
    else
    {
        /*--------------------------------------------*/
        /* No Previous value so start at the begining */
        /*--------------------------------------------*/
        for (i = 0; i < numx - 1; i++)
        {
            if (xinpt >= xarray[i] and xinpt <= xarray[i + 1])
            {
                *lastx = i;
                x0 = xarray[i];
                dx = xarray[i + 1] - x0;
                ddata = data[i + 1] - data[i];
                final_val =  data[i] + (xinpt - x0) * ddata / dx;
                break;
            }
        }
    }

    return (final_val);
}

/*****************************************************************************/
/* TWOD_INTERP                                                               */
/*                                                                           */
/* DESCRIPTION                                                               */
/* ------------------------------------------------------------------------- */
/*   two independent variable interpolation                                   */
/*                                                                           */
/* USED BY: manned, digi                                                     */
/*                                                                           */
/* PARAMETERS                     DESCRIPTION                                */
/* ------------------------------------------------------------------------- */
/*   IN: float x                  independent variable                       */
/*       float y                  independent variable                       */
/*       float *xarry             pointer to the independ variable array     */
/*       float *yarry             pointer to the independ variable array     */
/*       float *data              pointer to 2-d array as a function of x,y  */
/*       int   numx               number of x breakpoints                    */
/*       int   numy               number of y breakpoints                    */
/*       int   *lastx             pointer to most recent x bkpt to speed     */
/*                                up the interpolation.                      */
/*       int   *lasty             pointer to most recent y bkpt to speed     */
/*                                up the interpolation.                      */
/*  OUT: returns interpolated value                                          */
/*                                                                           */
/* MODIFICATIONS    DATE          DESCRIPTION                                */
/* ------------------------------------------------------------------------- */
/*     JC           1/2/92        initial write                              */
/*                                                                           */
/*****************************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::TwodInterp(SIM_FLOAT x, SIM_FLOAT y,
                                        const SIM_FLOAT *xarray, const SIM_FLOAT *yarray, const SIM_FLOAT *data, SIM_INT numx, SIM_INT numy,
                                        SIM_INT *lastx,  SIM_INT *lasty)
{
    float xinpt, yinpt;
    float x1, x2, dx;
    float y1, y2, dy;
    float ddata, ddata1, ddata2, data1, data2;
    float interp;
    int   ix, iy;

    ShiAssert(FALSE == F4IsBadReadPtr(xarray, sizeof * xarray * numx));
    ShiAssert(FALSE == F4IsBadReadPtr(yarray, sizeof * yarray * numy))
    /*-------------------------------------------------*/
    /* limit independent variables to table boundaries */
    /*-------------------------------------------------*/
    numx --;
    numy --;

    //if (numx >= 0 and numy >= 0) // JB 010220 CTD
    if (numx >= 0 and numy >= 0 and 
        xarray and // not F4IsBadReadPtr(xarray, sizeof(SIM_FLOAT)) and // (too much CPU)
        yarray // not F4IsBadReadPtr(yarray, sizeof(SIM_FLOAT)) // JB 010318 CTD (too much CPU)
       )
    {
        xinpt = min(max(x, xarray[0]), xarray[numx]);
        yinpt = min(max(y, yarray[0]), yarray[numy]);
    }

    /*-------------------------------------------------*/
    /* If there is a last x use it as a starting point */
    /*-------------------------------------------------*/
    ix = *lastx;
    ShiAssert(ix >= 0 and ix < numx); // JPO CTD

    if (ix not_eq 0)
    {
        if (xinpt <= xarray[ix])
        {
            /*-----------------------*/
            /* Look down in the data */
            /*-----------------------*/
            while ((ix > 0) and (xinpt < xarray[ix]))
                ix --;
        }
        else if (xinpt > xarray[ix + 1])
        {
            /*---------------------*/
            /* Look up in the data */
            /*---------------------*/
            while ((ix < numx) and (xinpt > xarray[ix + 1]))
                ix ++;
        }
    }
    else
    {
        /*--------------------------------------------*/
        /* No Previous value so start at the begining */
        /*--------------------------------------------*/
        while ((ix < numx) and (xinpt > xarray[ix + 1]))
            ix ++;
    }

    ShiAssert(ix >= 0 and ix < numx); // JPO CTD  check again

    if (ix < 0 or ix >= numx)
        return 0.0f;

    *lastx = ix;
    x1 = xarray[ix];
    x2 = xarray[ix + 1];
    dx = 1.0F / (x2 - x1);

    /*-------------------------------------------------*/
    /* If there is a last y use it as a starting point */
    /*-------------------------------------------------*/
    iy = *lasty;
    ShiAssert(iy >= 0 and iy < numy); // JPO CTD

    if (iy not_eq 0)
    {
        if (yinpt <= yarray[iy])
        {
            /*-----------------------*/
            /* Look down in the data */
            /*-----------------------*/
            while ((iy > 0) and (yinpt < yarray[iy]))
                iy --;
        }
        else if (yinpt > yarray[iy + 1])
        {
            /*---------------------*/
            /* Look up in the data */
            /*---------------------*/
            while ((iy < numy) and (yinpt > yarray[iy + 1]))
                iy ++;
        }
    }
    else
    {
        /*--------------------------------------------*/
        /* No Previous value so start at the begining */
        /*--------------------------------------------*/
        while ((iy < numy) and (yinpt > yarray[iy + 1]))
            iy ++;
    }

    ShiAssert(iy >= 0 and iy < numy); // JPO CTD  check again
    *lasty = iy;
    y1 = yarray[iy];
    y2 = yarray[iy + 1];
    dy = 1.0F / (y2 - y1);

    /*----------------------*/
    /* xarray interpolation */
    /*----------------------*/
    numy ++;
    ddata1 = data[(ix + 1) * numy + iy]     - data[ix * numy + iy];
    ddata2 = data[(ix + 1) * numy + iy + 1] - data[ix * numy + iy + 1];

    data1  = data[ix * numy + iy]     + (xinpt - x1) * ddata1 * dx;
    data2  = data[ix * numy + iy + 1] + (xinpt - x1) * ddata2 * dx;

    /*----------------------*/
    /* yarray interpolation */
    /*----------------------*/
    ddata    = data2 - data1;

    interp = data1 + (yinpt - y1) * ddata * dy;

    return (interp);
}

/*****************************************************************************/
/* THREED_INTERP                                                             */
/*                                                                           */
/* DESCRIPTION                                                               */
/* ------------------------------------------------------------------------- */
/* Three independent variable interpolation                                   */
/*                                                                           */
/* USED BY: manned, digi                                                     */
/*                                                                           */
/* PARAMETERS                     DESCRIPTION                                */
/* ------------------------------------------------------------------------- */
/*   IN: float x                  independent variable                       */
/*       float y                  independent variable                       */
/*       float z                  independent variable                       */
/*       float *xarry             pointer to the independ variable array     */
/*       float *yarry             pointer to the independ variable array     */
/*       float *zarry             pointer to the independ variable array     */
/*       float *data              ptr to 3-d array as a function of x,y,z    */
/*       int   numx               number of x breakpoints                    */
/*       int   numy               number of y breakpoints                    */
/*       int   numz               number of z breakpoints                    */
/*       int   *lastx             pointer to most recent x bkpt to speed     */
/*                                up the interpolation.                      */
/*       int   *lasty             pointer to most recent y bkpt to speed     */
/*                                up the interpolation.                      */
/*       int   *lastz             pointer to most recent z bkpt to speed     */
/*                                up the interpolation.                      */
/*  OUT: returns interpolated value                                          */
/*                                                                           */
/* MODIFICATIONS    DATE          DESCRIPTION                                */
/* ------------------------------------------------------------------------- */
/*     JC           1/2/92        initial write                              */
/*                                                                           */
/*****************************************************************************/
SIM_FLOAT SIMLIB_MATH_CLASS::ThreedInterp(SIM_FLOAT x, SIM_FLOAT y, SIM_FLOAT z,
        const SIM_FLOAT *xarray, const SIM_FLOAT *yarray, const SIM_FLOAT *zarray, const
        SIM_FLOAT *data, SIM_INT numx, SIM_INT numy, SIM_INT numz, SIM_INT *lastx,
        SIM_INT *lasty, SIM_INT *lastz)
{
    float xmin, xmax, ymin, ymax, zmin, zmax;
    float xinpt = 0.0F, yinpt = 0.0F, zinpt = 0.0F;
    float d1 = 0.0F, d2 = 0.0F, dd1 = 0.0F, dd2 = 0.0F, dd = 0.0F, de = 0.0F;
    float e1 = 0.0F, e2 = 0.0F;
    float x1 = static_cast<float>(*lastx), x2 = static_cast<float>(*lastx), dx = 0.0F;
    float y1 = static_cast<float>(*lasty), y2 = static_cast<float>(*lasty), dy = 0.0F;
    float z1 = static_cast<float>(*lastz), z2 = static_cast<float>(*lastz), dz = 0.0F;
    float interp = 0.0F;
    int i = 0, ix = 0, iy = 0, iz = 0;
    int index1 = 0, index2 = 0;
    int index3 = 0, index4 = 0;

    xmax = xarray[numx - 1];
    xmin = xarray[   0];
    ymax = yarray[numy - 1];
    ymin = yarray[   0];
    zmax = zarray[numz - 1];
    zmin = zarray[   0];

    /*-------------------------------------------------*/
    /* LIMIT INDEPENDENT VARIABLES TO TABLE BOUNDARIES */
    /*-------------------------------------------------*/
    xinpt = min(max(x, xmin), xmax);
    yinpt = min(max(y, ymin), ymax);
    zinpt = min(max(z, zmin), zmax);

    /*-------------------------------------------------*/
    /* If there is a last x use it as a starting point */
    /*-------------------------------------------------*/
    if (*lastx not_eq 0)
    {
        if (xinpt >= xarray[*lastx] and xinpt <= xarray[*lastx + 1])
        {
            ix = *lastx;
            x1 = xarray[*lastx];
            x2 = xarray[*lastx + 1];
        }
        else if (xinpt <= xarray[*lastx])
        {
            /*-----------------------*/
            /* Look down in the data */
            /*-----------------------*/
            for (i = *lastx; i > 0; i--)
            {
                if (xinpt <= xarray[i] and xinpt >= xarray[i - 1])
                {
                    *lastx = i - 1;
                    ix = i - 1;
                    x1 = xarray[i - 1];
                    x2 = xarray[i];
                    break;
                }
            }
        }
        else
        {
            for (i = *lastx + 1; i < numx - 1; i++)
            {
                if (xinpt >= xarray[i] and xinpt <= xarray[i + 1])
                {
                    *lastx = i;
                    ix = i;
                    x1 = xarray[i];
                    x2 = xarray[i + 1];
                    break;
                }
            }
        }
    }
    else
    {
        /*--------------------------------------------*/
        /* No Previous value so start at the begining */
        /*--------------------------------------------*/
        for (i = 0; i < numx - 1; i++)
        {
            if (xinpt >= xarray[i] and xinpt <= xarray[i + 1])
            {
                *lastx = i;
                ix = i;
                x1 = xarray[i];
                x2 = xarray[i + 1];
                break;
            }
        }

    }

    dx = x2 - x1;

    if ( not dx) dx = 1; //me123 incase theres only one number in the dat file

    /*-------------------------------------------------*/
    /* If there is a last y use it as a starting point */
    /*-------------------------------------------------*/
    if (*lasty not_eq 0)
    {
        if (yinpt >= yarray[*lasty] and yinpt <= yarray[*lasty + 1])
        {
            iy = *lasty;
            y1 = yarray[*lasty];
            y2 = yarray[*lasty + 1];
        }
        else if (yinpt <= yarray[*lasty])
        {
            /*-----------------------*/
            /* Look down in the data */
            /*-----------------------*/
            for (i = *lasty; i > 0; i--)
            {
                if (yinpt <= yarray[i] and yinpt >= yarray[i - 1])
                {
                    *lasty = i - 1;
                    iy = i - 1;
                    y1 = yarray[i - 1];
                    y2 = yarray[i];
                    break;
                }
            }
        }
        else
        {
            for (i = *lasty + 1; i < numy - 1; i++)
            {
                if (yinpt >= yarray[i] and yinpt <= yarray[i + 1])
                {
                    *lasty = i;
                    iy = i;
                    y1 = yarray[i];
                    y2 = yarray[i + 1];
                    break;
                }
            }
        }
    }
    else
    {
        /*--------------------------------------------*/
        /* No Previous value so start at the begining */
        /*--------------------------------------------*/
        for (i = 0; i < numy - 1; i++)
        {
            if (yinpt >= yarray[i] and yinpt <= yarray[i + 1])
            {
                *lasty = i;
                iy = i;
                y1 = yarray[i];
                y2 = yarray[i + 1];
                break;
            }
        }

    }

    dy = y2 - y1;

    if ( not dy) dy = 1; //me123 incase theres only one number in the dat file

    /*-------------------------------------------------*/
    /* If there is a last z use it as a starting point */
    /*-------------------------------------------------*/
    if (*lastz not_eq 0)
    {
        if (zinpt >= zarray[*lastz] and zinpt <= zarray[*lastz + 1])
        {
            iz = *lastz;
            z1 = zarray[*lastz];
            z2 = zarray[*lastz + 1];
        }
        else if (zinpt <= zarray[*lastz])
        {
            /*-----------------------*/
            /* Look down in the data */
            /*-----------------------*/
            for (i = *lastz; i > 0; i--)
            {
                if (zinpt <= zarray[i] and zinpt >= zarray[i - 1])
                {
                    *lastz = i - 1;
                    iz = i - 1;
                    z1 = zarray[i - 1];
                    z2 = zarray[i];
                    break;
                }
            }
        }
        else
        {
            for (i = *lastz + 1; i < numz - 1; i++)
            {
                if (zinpt >= zarray[i] and zinpt <= zarray[i + 1])
                {
                    *lastz = i;
                    iz = i;
                    z1 = zarray[i];
                    z2 = zarray[i + 1];
                    break;
                }
            }
        }
    }
    else
    {
        /*--------------------------------------------*/
        /* No Previous value so start at the begining */
        /*--------------------------------------------*/
        for (i = 0; i < numz - 1; i++)
        {
            if (zinpt >= zarray[i] and zinpt <= zarray[i + 1])
            {
                *lastz = i;
                iz = i;
                z1 = zarray[i];
                z2 = zarray[i + 1];
                break;
            }
        }

    }

    dz = z2 - z1;

    if ( not dz) dz = 1; //me123 incase theres only one number in the dat file

    /*----------------------------------*/
    /* x INTERPOLATION AT z BREAKPOINT  */
    /*----------------------------------*/
    index1 = (ix + 1) * numy * numz + iy * numz + iz;
    index2 = ix * numy * numz + iy * numz + iz;
    index3 = (ix + 1) * numy * numz + (iy + 1) * numz + iz;
    index4 = ix * numy * numz + (iy + 1) * numz + iz;

    dd1 = data[index1] - data[index2];

    dd2 = data[index3] - data[index4];

    d1  = data[index2] + (xinpt - x1) * dd1 / dx;

    d2  = data[index4] + (xinpt - x1) * dd2 / dx;

    /*---------------------------------*/
    /* y INTERPOLATION AT z BREAKPOINT */
    /*---------------------------------*/
    dd  = d2 - d1;
    e1  = d1 + (yinpt - y1) * dd / dy;

    /*-----------------------------------*/
    /* x INTERPOLATION AT z+1 BREAKPOINT */
    /*-----------------------------------*/
    index1 = (ix + 1) * numy * numz + iy * numz + iz + 1;
    index2 = ix * numy * numz + iy * numz + iz + 1;
    index3 = (ix + 1) * numy * numz + (iy + 1) * numz + iz + 1;
    index4 = ix * numy * numz + (iy + 1) * numz + iz + 1;

    dd1 = data[index1] - data[index2];

    dd2 = data[index3] - data[index4];

    d1  = data[index2] + (xinpt - x1) * dd1 / dx;

    d2  = data[index4] + (xinpt - x1) * dd2 / dx;

    /*-----------------------------------*/
    /* y INTERPOLATION AT z+1 BREAKPOINT */
    /*-----------------------------------*/
    dd  = d2 - d1;
    e2  = d1 + (yinpt - y1) * dd / dy;

    /*-----------------*/
    /* z INTERPOLATION */
    /*-----------------*/
    de    = e2 - e1;
    interp = e1 + (zinpt - z1) * de / dz;
    return (interp);
}
