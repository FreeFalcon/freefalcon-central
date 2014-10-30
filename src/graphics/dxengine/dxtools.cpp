#include <cISO646>
#include <math.h>
#include "../include/TimeMgr.h"
#include "../include/ObjectInstance.h"
#include "dxdefines.h"
#include "DxTools.h"
#include "../../falclib/include/mltrig.h"
#include "dxengine.h"

#ifndef DEBUG_ENGINE

// This function just assign a PMatrix object to a DX Compliant Matrix
void AssignPmatrixToD3DXMATRIX(D3DXMATRIX *d, Pmatrix *s)
{
    d->m00 = s->M11;
    d->m01 = s->M21;
    d->m02 = s->M31;
    d->m03 = 0.0f;
    d->m10 = s->M12;
    d->m11 = s->M22;
    d->m12 = s->M32;
    d->m13 = 0.0f;
    d->m20 = s->M13;
    d->m21 = s->M23;
    d->m22 = s->M33;
    d->m23 = 0.0f;
    d->m30 = 0.0f;
    d->m31 = 0.0f;
    d->m32 = 0.0f;
    d->m33 = 1.0f;
}


void AssignD3DXMATRIXToPmatrix(Pmatrix *d, D3DXMATRIX *s)
{
    d->M11 = s->m00;
    d->M21 = s->m01;
    d->M31 = s->m02;
    d->M12 = s->m10;
    d->M22 = s->m11;
    d->M32 = s->m12;
    d->M13 = s->m20;
    d->M23 = s->m21;
    d->M33 = s->m22;
}

#endif



//////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
///////////////////////////////////////////// SCRIPTS MANAGEMENT \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

static const float Seconds = 1.0f / 1000.0f;
static const float Minutes = Seconds / (60.0f);
static const float Hours = Minutes / (60.0f);
static const float Degrees = (PI / 180.0f);
static const float DegreesPerSecond = Degrees * Seconds;

// the boolean return value is used by the caller to understand if the next script in the list is to be
// processed, false means no more script processing

bool DXScript_None(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    return true;
}


/////////////////////////////////////////////////////////////////////////////////
// The Cycling animation, max 32 frames, timed
// Argument 0 = SwitchNr to apply the animation
// Argument 1 = Delay btw frames in mSecs
// Argument 2 = Number of Frames
bool DXScript_Animate(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    // Consistency check
    if (obj->ParentObject->nSwitches <= 0) return true;

    if (Argument[0]  >= (WORD) obj->ParentObject->nSwitches) return true;

#ifdef DEBUG_ENGINE
    // Get the timings
    DWORD Delta = GetTickCount();
#else
    // Get the timings
    DWORD Delta = TheTimeManager.GetClockTime();
#endif
    // Scale it by the delay
    Delta /= Argument[1];
    //Set accordingly the switch
    obj->SetSwitch(Argument[0], 1 << (Delta % Argument[2]));

    return true;
}



/////////////////////////////////////////////////////////////////////////////////
// The Rotating animation
// Argument 0 = Starting Dof
// Argument 1 = Nr of DOFS to apply rotation
// Argument 2 = FLOAT, Degrees per second of rotation
bool DXScript_Rotate(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    // Consistency check
    if (obj->ParentObject->nDOFs <= 0) return true;

    // Get the Starting DOF
    DWORD Dof = Argument[0];
    // get the number of Dofs to apply the rotation
    DWORD Count = Argument[1];

    // consistency check and Limitation
    if (Dof >= (WORD) obj->ParentObject->nDOFs) return true;

    if ((Dof + Count) >= (WORD) obj->ParentObject->nDOFs) Count = obj->ParentObject->nDOFs - Dof - 1;

#ifdef DEBUG_ENGINE
    // Get the timings
    float Delta = GetTickCount() * ((float*)Argument)[2] * DegreesPerSecond;
#else
    // Get the timings
    float Delta = TheTimeManager.GetClockTime() * ((float*)Argument)[2] * DegreesPerSecond;
#endif

    // for each DOF
    while (Count--)
    {
        obj->DOFValues[Dof++].rotation = Delta;
    }

    return true;
}




/////////////////////////////////////////////////////////////////////////////////
// The Helicopter aniamtion, up to 4 DOFs rotated with 4 different CXs
// Argument 0 = Starting Dof
// Argument 1 = Nr of DOFS to apply rotation
// Argument 2 = FLOAT, Degrees per second of rotation
bool DXScript_HelyRotate(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    // Consistency check
    if (obj->ParentObject->nDOFs <= 0) return true;

    // Get the Starting DOF
    DWORD Dof = Argument[0];
    // get the number of Dofs to apply the rotation
    DWORD Count = Argument[1];

    // consistency check and Limitation
    if (Dof >= (WORD) obj->ParentObject->nDOFs) return true;

    if ((Dof + Count) >= (WORD) obj->ParentObject->nDOFs) Count = obj->ParentObject->nDOFs - Dof - 1;

#ifdef DEBUG_ENGINE
    // Get the timings
    float Delta = GetTickCount() * ((float*)Argument)[2] * DegreesPerSecond;
#else
    // Get the timings
    float Delta = TheTimeManager.GetClockTime() * ((float*)Argument)[2] * DegreesPerSecond;
#endif

    // for each DOF
    if (Count--)obj->DOFValues[Dof++].rotation = Delta;

    if (Count--)obj->DOFValues[Dof++].rotation = Delta * 1.6f;

    if (Count--)obj->DOFValues[Dof++].rotation = Delta * 2.1f;

    if (Count--)obj->DOFValues[Dof++].rotation = Delta * 0.1f;

    return true;
}


float TestAngle;


// Military rotating beacon script
// Assume model has green pointing north, and two whites 30 degrees apart pointing southward
// switch bits:
// 1 0   degree green dim
// 2 0   degree green flash
// 3 0   degree green has flashed flag
// 5 165 degree white dim
// 6 165 degree white flash
// 7 165 degree white has flashed flag
// 9 195 degree white dim
// 10 195 degree white flash
// 11 195 degree white has flashed flag
bool DXScript_Beacon(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    ShiAssert(obj->ParentObject->nSwitches > 1);
    ShiAssert(obj->ParentObject->nDOFs > 0);

    if (obj->ParentObject->nDOFs <= 0) return true;

    if (obj->ParentObject->nSwitches <= 1) return true;

    DWORD sw = obj->SwitchValues[1];

#ifdef DEBUG_ENGINE
    // Get the timings
    float Delta = GetTickCount() * 36.0f * DegreesPerSecond;
#else
    // Get the timings
    float Delta = TheTimeManager.GetClockTime() * 36.0f * DegreesPerSecond;
#endif

    float RelAngle;

    // get the angular difference btw camera and object ALWAYS POSITIVE ( + 2PI )
    if (pos->y) RelAngle = (float)atan2(pos->x, pos->y);

    // calculate the Beacon World Transformation
    D3DXVECTOR3 BeaconWorldDir(1, 0, 0);
    // get the Beacon world transformation
    D3DXMATRIX WorldVect = TheDXEngine.AppliedState;
    // kill any world translation, just keep rotation
    WorldVect.m30 = WorldVect.m31 = WorldVect.m32 = 0.0f;
    WorldVect.m33 = 1.0f;
    // transform a coordinate of x=1,y=0
    D3DXVec3TransformCoord(&BeaconWorldDir, &BeaconWorldDir, &WorldVect);
    // calculate rotation in WORLD SPACE ALWAYS POSITIVE ( + 2PI )
    float Br = Delta - (float)atan2(BeaconWorldDir.x, BeaconWorldDir.y);
    // subract viever relative angle and keep it ALWAYS POSITIVE
    Br = Br + RelAngle + PI * 2;
    // get the Degrees fomr the 0 < r <2PI range )
    RelAngle = (fmod(Br, PI * 2) - PI) / Degrees;


    // All flashes OFF
    sw and_eq 0xFFFFF000; // All off

    /////// 0 degree green light

    if (fabs(RelAngle) <= 3.0f) sw or_eq 0x7; // Flash on, has flashed, visible

    if (RelAngle >= 162.0f and RelAngle <= 168.0f) sw or_eq 0x700; // Flash on, has flashed, visible

    if (RelAngle >= -168.0f and RelAngle <= -162.0f) sw or_eq 0x70; // Flash on, has flashed, visible

    // Now store the computed results
    obj->DOFValues[0].rotation = (float)fmod(Delta, 2.0f * PI);
    //obj->DOFValues[0].rotation = TestAngle*Degrees;
    obj->SwitchValues[1] = sw;

    return true;
}



// Approach angle apprpriate VASI light indications (FAR set)
bool DXScript_VASIF(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{

    ShiAssert(obj->ParentObject->nSwitches > 0);

    if (obj->ParentObject->nSwitches <= 0) return true;

    float angle = (float)atan2(pos->z, sqrtf(pos->y * pos->y + pos->x * pos->x)) / Degrees;

    if (angle > 4.0f) obj->SetSwitch(0, 2); // White
    else obj->SetSwitch(0, 1); // Red

    return true;
}

// Approach angle apprpriate VASI light indications (NEAR set)
bool DXScript_VASIN(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    ShiAssert(obj->ParentObject->nSwitches > 0);

    if (obj->ParentObject->nSwitches <= 0) return true;

    float angle = (float)atan2(pos->z, sqrtf(pos->y * pos->y + pos->x * pos->x)) / Degrees;

    if (angle > 2.0f) obj->SetSwitch(0, 2); // White
    else obj->SetSwitch(0, 1); // Red

    return true;
}


#define GS (3.0f)
#define NANGLES (sizeof(angles)/sizeof(float))

const float angles[] = {GS + 2.3f, GS + 2, GS + 1.7f, GS + 1.3f, GS + 1, GS + 0.7F, GS + 0.3f, GS,
                          GS - 0.3f, GS - 0.7f, GS - 1, GS - 1.3f, GS - 1.7f
                       };


// Approach angle for Carrier MeatBall - 13 switches (0-12) for vertical Glide Slope
bool DXScript_MeatBall(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    float angle = (float)atan2(pos->z, sqrtf(pos->y * pos->y + pos->x * pos->x)) / Degrees;

    ShiAssert(obj->ParentObject->nSwitches >= NANGLES - 2);

    if (obj->ParentObject->nSwitches <= NANGLES - 2) return true;

    for (int i = 0; i < NANGLES - 1; i++)
    {
        if (angle < angles[i] and angle > angles[i + 1]) obj->SetSwitch(i, 1);
        else obj->SetSwitch(i, 0);
    }

    return true;
}


// Chaff animation
// TODO:  Should run at 10hz, not frame rate (ie be time based not frame based)
bool DXScript_Chaff(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
#ifdef DEBUG_ENGINE
    // Get the timings
    DWORD Delta = (GetTickCount() bitand 0xffffff) / 100;
#else
    // Get the timings
    DWORD Delta = (TheTimeManager.GetClockTime() bitand 0xffffff) / 100;
#endif

    // consistency check
    if (obj->ParentObject->nSwitches <= 0) return true;

    // if 1st frame set the starting time
    if ((obj->SwitchValues[0] bitand 0xffff0000) == 0x0000) obj->SwitchValues[0] = (Delta << 16) bitand 0xffff0000;

    // update frame number every 100 mSec
    if ( not (obj->SwitchValues[0] bitand 0x8000))
        obj->SwitchValues[0] = (obj->SwitchValues[0] bitand 0xffff0000) bitor (1 << ((Delta bitand 0xffff) - (obj->SwitchValues[0] >> 16) bitand 0x00ffff));

    return true;
}


bool DXScript_CollapseChute(D3DVECTOR *pos, ObjectInstance *obj, DWORD *Argument)
{
    ShiAssert(obj->ParentObject->nSwitches > 0);

    if (obj->SwitchValues[0] == 0)
    {
        obj->SwitchValues[0] = 1;
    }
    else if (obj->SwitchValues[0] < 0x40)
    {
        obj->SwitchValues[0] <<= 1;
    }
    else if (obj->SwitchValues[0] == 0x40)
    {
        obj->SwitchValues[0] = 0x10000020;
    }
    else if (obj->SwitchValues[0] == 0x10000020)
    {
        obj->SwitchValues[0] = 0x10000010;
    }
    else
    {
        obj->SwitchValues[0] = 0x00000008;
    }

    return true;
}



bool (*DXScriptArray[])(D3DVECTOR *pos, ObjectInstance*, DWORD*) =
{
    DXScript_None,
    DXScript_Animate,
    DXScript_Rotate,
    DXScript_HelyRotate,
    DXScript_Beacon,
    DXScript_VASIF,
    DXScript_VASIN,
    DXScript_MeatBall,
    DXScript_Chaff,
    DXScript_CollapseChute,

};


