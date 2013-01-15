#ifndef _IRSTDATA_H
#define _IRSTDATA_H

typedef struct IRSTDataType
{
	float		NominalRange;					// Detection range against F16 sized target
	float		FOVHalfAngle;					// radians (degrees in file)
	float		GimbalLimitHalfAngle;			// radians (degrees in file)
	float		GroundFactor;					// Range multiplier applied for ground targets
	float		FlareChance;					// Base probability a flare will work
} IRSTDataType;

extern IRSTDataType*	 IRSTDataTable;
extern short NumIRSTEntries;

#endif
