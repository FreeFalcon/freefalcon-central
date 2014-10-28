// sfr: math constants
#define _USE_MATH_DEFINES
#include <math.h>

#include "stdhdr.h"
#include "airframe.h"
#include "simbase.h"
#include "aircrft.h"
#include "otwdrive.h"
#include "fakerand.h"
#include "Graphics/Include/tmap.h"
#include "Graphics/Include/rviewpnt.h"  // to get ground type
#include "vutypes.h"
#include "PilotInputs.h"
#include "limiters.h"
#include "fack.h"
#include "falcsess.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/LandingMessage.h"
#include "campbase.h"
#include "fsound.h"
#include "soundfx.h"
#include "playerop.h"
#include "simio.h"
#include "weather.h"
#include "objectiv.h"
#include "find.h"
#include "atcbrain.h"
#include "Graphics/Include/terrtex.h"
#include "ffeedbk.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
//Retro_dead 15Jan2004 #define DIRECTINPUT_VERSION 0x0700
//Retro_dead 15Jan2004 #include "dinput.h"

#include "flight.h"
#include "simdrive.h"
#include "digi.h"
#include "ptdata.h"
#include "dofsnswitches.h"
#include "Graphics/Include/drawbsp.h"
#include "classtbl.h"
//#include <crtdbg.h> // JPO debug

extern VU_TIME vuxGameTime;

// All headers cut bitand paste from eom.cpp
// this code provides the animations for gear strut compression and rolling wheels.
void AirframeClass::RunLandingGear(void)
{
    if (auxaeroData->animWheelRadius[0] and platform->drawPointer)
    {
        // MLR 2003-10-04 code to support spinning landing wheels
        // the idea here is to set the Radians/Sec rotation of the wheel
        // while in contact with the ground.
        // code in the surface.cpp file actually rotates the wheel, and
        // gradually bleeds off the RPS one the gear is no longer grounded
        // (which means this code is no longer running).
        int i;
        float cgloc = GetAeroData(AeroDataSet::CGLoc);

        // sfr: distance moved in frame
        SM_SCALAR speed = platform->GetVt();
        SM_SCALAR dist = (speed * SimLibMajorFrameTime);

        for (i = 0; i < NumGear(); i++)
        {
            Tpoint PtWorldPos;
            Tpoint PtRelPos;

            PtRelPos.x = cgloc - GetAeroData(AeroDataSet::NosGearX + i * 4);
            PtRelPos.y = GetAeroData(AeroDataSet::NosGearY + i * 4);
            PtRelPos.z = GetAeroData(AeroDataSet::NosGearZ + i * 4);

            MatrixMult(&((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos);

            PtWorldPos.x += x;
            PtWorldPos.y += y;
            PtWorldPos.z += z;


            // MLR 2003-10-14 animate the landing gear strut
            {
                //gear[i].StrutExtension = OTWDriver.GetGroundLevel(PtWorldPos.x, PtWorldPos.y) - ( PtWorldPos.z + z)
                gear[i].StrutExtension = groundZ - (PtWorldPos.z);

                // limit to values from auxaerodata
                // these will get applied to a DOF or Translator to make it look like
                // the gear is working.

                if (gear[i].StrutExtension < -auxaeroData->animGearMaxComp[i])
                {
                    gear[i].StrutExtension = -auxaeroData->animGearMaxComp[i];
                }

                gear[i].WheelRPS *= (1 - .4f * SimLibMajorFrameTime); // slows wheel down

                if (gear[i].StrutExtension > auxaeroData->animGearMaxExt[i])
                {
                    // wheel off ground
                    gear[i].StrutExtension = auxaeroData->animGearMaxExt[i];
                    // sfr: was in run gear function
                    // deaccel gear, since its not in touch with ground
                    // compute new angle
                    gear[i].WheelAngle += gear[i].WheelRPS * SimLibMajorFrameTime;
                }
                else if (SimLibMajorFrameTime and auxaeroData->animWheelRadius[i])
                {
                    // sfr: using plane speed now
                    // we need this for the case above,
                    // when gear is not in touch with ground anymore
                    gear[i].WheelRPS = speed;
                    // wheel rotation increment
                    SM_SCALAR rInc = dist / auxaeroData->animWheelRadius[i];
                    gear[i].WheelAngle += rInc;
                }

                // can be more than 2pi
                gear[i].WheelAngle = fmod(gear[i].WheelAngle, ((float)(M_PI)) * 2.0f);
            }
        }
    }

    //error - need to have sound played when the radii are 0
}
