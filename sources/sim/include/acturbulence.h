#ifndef AC_TURBULENCE
#define AC_TURBULENCE

//sfr: changed filename to ACTurbulence (avoids name collision)
// also changed place

#include "AList.h"
//#include "mathlib/vector3.h"

class TurbulanceList : public ProtectedAList
{
public:
	TurbulanceList() {};
	~TurbulanceList();
};

class AircraftTurbulence : public ANode
{
public:
	enum TurbType { WAKE, LVORTEX, RVORTEX };
	TurbType type;

	AircraftTurbulence();
	~AircraftTurbulence();
	void Release(void); // mark for self deletion
	void RecordPosition(float Strength, float X, float Y, float Z);
	static float GetTurbulence(float X, float Y, float Z, float Yaw, float Pitch, float Roll, float &WakeEffect, float &YawEffect, float &PitchEffect, float &RollEffect);
	void BreakRecord(void) {breakRecord = 1;}

	float startRadius;
	float lifeSpan;
	float growthRate;

	static void Draw( class RenderOTW *renderer ); // debug useage

private:
	static unsigned long lastPurgeTime;
	float RetieveTurbulence(struct RetrieveTurbulanceParams &rtp);
	int locked;
	AList turbRecordList;
	int counter;
	int breakRecord;
};

#endif
