#include "cmpglobl.h"
#include "campcell.h"
#include "campterr.h"
#include "listadt.h"
#include "find.h"
#include "cmpradar.h"

int RadarRangeClass::CanDetect(float dx, float dy, float dz)
{
    int oct;

    oct = OctantTo(0.0F, 0.0F, dx, dy);

    if ((dz * dz) >
        ((dx * dx) + (dy * dy)) * (detect_ratio[oct] * detect_ratio[oct]))
        return 1;

    return 0;
}

float RadarRangeClass::GetRadarRange(float dx, float dy, float dz)
{
    int oct;

    oct = OctantTo(0.0F, 0.0F, dx, dy);
    return dz / detect_ratio[oct];
}
