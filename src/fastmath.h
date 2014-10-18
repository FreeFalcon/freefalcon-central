/** Fast Math Functions **/

// Float to 32-bit integer
inline DWORD F_I32(float x)
{
    DWORD r;

    _asm
    {
        fld x
        fistp r
    }

    return r;
}

// Absolute value
inline float F_ABS(float x)
{
    float r;

    _asm
    {
        fld x
        fabs
        fstp r
    }

    return r;
}
