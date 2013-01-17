#include "stdhdr.h"
#include "aircrft.h"
#include "airframe.h"
#include "simbrain.h"
#include "simdrive.h"

#define BLACKOUT_RATE      2.0F
#define REDOUT_RATE        5.0F
#define UNREDDEN_RATE      0.075F

// JB 000807
//#define AWAKEN_RATE        0.167F
//#define GREYOUT_ONSET      36.0F//-
//#define BLACKOUT_ONSET     190.0F//-
// 2000-11-24 BY S.G. JB, I'LL USE THE VALUE FROM RP4 IF THAT'S OK WITH YOU
//#define AWAKEN_RATE        0.012F
//#define GREYOUT_ONSET      155.0F
//#define BLACKOUT_ONSET     195.0F
// JB 000807
//#define AWAKEN_RATE        (1.0F/30.0F) // Awakening is too slow with the new values below.
//#define GREYOUT_ONSET      200.0F		// 27 seconds before starting to blackout
//#define BLACKOUT_ONSET     283.0F		// to 33 seconds for a complete blackout
//#define G0Plus             5.5F
// END OF MODIFIED SECTION
//#define REDOUT_ONSET       -55.0F
//#define PINKOUT_ONSET      -15.0F
//#define G0Neg              -1.5F

// RV - Biker - Make blackout/redout faster
#define AWAKEN_RATE        (1.0F/30.0F)
#define GREYOUT_ONSET      100.0F
#define BLACKOUT_ONSET     150.0F
#define G0Plus               3.4F
#define REDOUT_ONSET       -40.0F
#define PINKOUT_ONSET      -15.0F
#define G0Neg               -1.5F

float AircraftClass::GlocPrediction(void)
{
	float gFact;
	float gs = af->nzcgb;
	float skillFactor = (theBrain->SkillLevel() + 2.0F)/4.0F;

	if (this == SimDriver.GetPlayerEntity()) {
		skillFactor = 1.1F;
	}

	if (SimDriver.MotionOn()) {
		if (gLoadSeconds >= 0.0F) {
			if (gs > G0Plus+skillFactor) {
				gLoadSeconds += (BLACKOUT_RATE * (gs-(G0Plus+skillFactor)) * SimLibMajorFrameTime);
			}
			else if (gs > G0Neg) {
				gLoadSeconds *= (1.0F - AWAKEN_RATE  * skillFactor * SimLibMajorFrameTime);
			}
			else {
				gLoadSeconds *= (1.0F - AWAKEN_RATE  * skillFactor * SimLibMajorFrameTime);
				gLoadSeconds -= (REDOUT_RATE * (G0Neg - gs) * SimLibMajorFrameTime);
			}

			gFact = (gLoadSeconds - GREYOUT_ONSET * skillFactor) / ((BLACKOUT_ONSET - GREYOUT_ONSET) * skillFactor);
			gFact = min ( max (gFact, 0.0F), 1.0F);
		}
		else {
			if (gs <= G0Neg) {
				gLoadSeconds += (REDOUT_RATE * (gs - G0Neg)  * SimLibMajorFrameTime);
			}
			else if (gs < G0Plus+skillFactor) {
				gLoadSeconds *= (1.0F - UNREDDEN_RATE * skillFactor * SimLibMajorFrameTime);
			}
			else {
				gLoadSeconds *= (1.0F - UNREDDEN_RATE * skillFactor * SimLibMajorFrameTime);
				gLoadSeconds += (BLACKOUT_RATE * (gs - (G0Plus+skillFactor)) * SimLibMajorFrameTime);
			}
			gFact = (gLoadSeconds - PINKOUT_ONSET * skillFactor) / ((PINKOUT_ONSET - REDOUT_ONSET) * skillFactor);
			gFact = min ( max (gFact, -1.0F), 0.0F);
		}
	}
	else {
		if (gLoadSeconds >= 0.0F) {
			gFact = (gLoadSeconds - GREYOUT_ONSET * skillFactor) / ((BLACKOUT_ONSET - GREYOUT_ONSET) * skillFactor);
			gFact = min ( max (gFact, 0.0F), 1.0F);
		}
		else {
			gFact = (gLoadSeconds - PINKOUT_ONSET * skillFactor) / ((PINKOUT_ONSET - REDOUT_ONSET) * skillFactor);
			gFact = min ( max (gFact, -1.0F), 0.0F);
		}
	}

	return (gFact);
}
