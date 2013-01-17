#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "Find.h"
#include "CmpRadar.h"

int RadarRangeClass::CanDetect (float dx, float dy, float dz)
	{
	int		oct;

	oct = OctantTo(0.0F,0.0F,dx,dy);
	if ((dz * dz) > ((dx*dx) + (dy*dy)) * (detect_ratio[oct]*detect_ratio[oct]))
		return 1;
	return 0;
	}

float RadarRangeClass::GetRadarRange (float dx, float dy, float dz)
	{
	int		oct;

	oct = OctantTo(0.0F,0.0F,dx,dy);
	return dz / detect_ratio[oct];
	}
