#include "stdhdr.h"
#include "hdigi.h"
#include "sensors.h"
#include "object.h"
#include "simveh.h"

/*--------------------------*/
/* last ditch maneuver time */
/*--------------------------*/
#define LD_TIME  4.5               
#define MISSILE_LETHAL_CONE    45.0F

void HeliBrain::MissileDefeatCheck(void)
{
}

void HeliBrain::MissileDefeat(void)
{
}

int HeliBrain::MissileBeamManeuver(void)
{
	return 0;
}

void HeliBrain::MissileDragManeuver(void)
{
}

void HeliBrain::MissileLastDitch(void)
{
}

int HeliBrain::MissileEvade (void)
{
	return 0;
}
