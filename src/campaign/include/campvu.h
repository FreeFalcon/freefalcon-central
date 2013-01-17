#ifndef CAMPVU_H
#define cAMPVU_H

#include "vu.h"
//#include "unit.h"
//#include "command.h"
//#include "hq.h"
#include "objective.h"

#ifdef __cplusplus
extern "C" {
#endif

#define U_AVAILABLE	0x1

/*
typedef struct {
	CdbStaticHeader				entity;
	UnitTransmitDataType			transmitData;
	UnitClass						*UnitData;
	} FalconUnitType;

typedef struct {
	CdbStaticHeader				entity;
	CommandTransmitDataType		transmitData;
	CommandClass					*CommandData;
	} FalconCommandType;

typedef struct {
	CdbStaticHeader				entity;
	HQTransmitDataType			transmitData;
	HQClass							*HQData;
	} FalconHQType;

// OR...

typedef struct {
	CdbStaticHeader				entity;
	TransmitDataType				transmitData;
	union	{
		UnitClass						*UnitData;
		CommandClass					*CommandData;
		HQClass							*HQData;
		};
	} FalconUnitType;

*/

typedef struct {
	CdbStaticHeader				entity;
	ObjectiveTransmitDataType	transmitData;
	ObjectiveClass					*ObjectiveData;
	} FalconObjectiveType;

#ifdef __cplusplus
#endif
#endif
