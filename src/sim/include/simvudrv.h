#ifndef _SIM_VUDRIVER_H
#define _SIM_VUDRIVER_H

/** @file simvudrv.h
* higher level drivers
* implement base driver virtual functions
* sfr: rewrite
*/

#include "f4vu.h"
#include "vudriver.h"
#include "vuevent.h"


// sfr: merged TargetSpotDriver and GroundSpotDriver into SpotDriver
// sfr: moved these drivers to here
// FUCK these are duplicates classes...
// TODO need to get rid of it (create a SpotDriver)
// and WTH are they for??
// TODO2 also check if this really does not need sending
// if it does, we make this super of SimVuDriver
class SpotDriver : public VuMaster
{
    VU_TIME last_time;
    SM_SCALAR lx;
    SM_SCALAR ly;
    SM_SCALAR lz;
public:
    SpotDriver(VuEntity *entity);
    virtual void Exec(VU_TIME timestamp);
    virtual VU_BOOL ExecModel(VU_TIME timestamp);
    virtual VuMaster::SEND_SCORE SendScore(const VuSessionEntity *vs, VU_TIME timeDelta)
    {
        return SEND_SCORE(DONT_SEND, 0.0f);
    }
};


/** this is a master driver for a unit in game. It implements VuMaster virtual functions */
class SimVuDriver : public VuMaster
{
public:

    // constructors and destructors
    SimVuDriver(VuEntity* theEnt) : VuMaster(theEnt) {};
    ~SimVuDriver(void) {};

    /** this calls the AI model. Base Exec calls this */
    virtual VU_BOOL ExecModel(VU_TIME timestamp);

    /** determines if and how unit update will be sent to other clients. Pure virtual in base class */
    virtual VuMaster::SEND_SCORE SimVuDriver::SendScore(const VuSessionEntity *vs, VU_TIME timeDelta);
};

/** Slave driver. Any unit which is driver by a non-local session has a slave driver.
* Implements VuDelaySlave virtual functions
*/
class SimVuSlave : public VuDelaySlave
{
public:
    // constructors and destructors
    SimVuSlave(VuEntity *entity) : VuDelaySlave(entity) {};
    virtual ~SimVuSlave() {};

    /** create values for this unit so that it may appear to move fluid. Usually, calls deadreckon */
    virtual void Exec(VU_TIME timestamp);
    /** handle a position update, predicting new position */
    virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);
};

#endif
