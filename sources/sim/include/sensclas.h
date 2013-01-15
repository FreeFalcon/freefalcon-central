#ifndef _SENSOR_CLASS_H
#define _SENSOR_CLASS_H

// sfr: Im removing the RemoteBuggedTarget, since it doest matter if entity is local or not
#define NO_REMOTE_BUGGED_TARGET 1

#include "mfd.h"

class SimObjectType;
class SimMoverClass;

class SensorClass : public MfdDrawable {
#if NO_REMOTE_BUGGED_TARGET
	// sfr: should we make SetSensorTarget public?
	friend class FalconTrackMessage;
#endif
public:
	SensorClass(SimMoverClass* newPlatform);
	virtual ~SensorClass(void);

public:
	enum SensorType {IRST, Radar, RWR, Visual, HTS, TargetingPod, RadarHoming, NumSensorTypes};
	enum TrackTypes {NoTrack, Detection, UnreliableTrack, SensorTrack};
	enum SensorData {NoInfo, LooseBearing, RangeAndBearing, ExactPosition, PositionAndOrientation};
	typedef struct{
		float az, el, ata, droll;
		float azFrom, elFrom, ataFrom;
		float x, y, z;
		float range, vt;
	} SensorUpdateType;

	SensorData		dataProvided;
	SimMoverClass*	platform;

public:
	SensorType	Type(void)			{ return sensorType; }
	virtual void	SetType(SensorType senstype)		{ sensorType = senstype; }

	virtual void	SetPower (BOOL state)	{ isOn = state; if (!isOn) ClearSensorTarget(); }
	virtual BOOL			IsOn(void)				{ return isOn; }

	// AI Uses this to command a lock
	virtual void SetDesiredTarget( SimObjectType* newTarget );
	// Drop our target lock (if any)
	virtual void ClearSensorTarget ( void );						
	SimObjectType* CurrentTarget(void)	{ return lockedTarget; };

	virtual SimObjectType* Exec (SimObjectType* curTargetList) = 0;
	virtual void ExecModes(int, int)						{}
	virtual void UpdateState(int, int)						{}
	virtual void SetSeekerPos (float newAz, float newEl)	{ seekerAzCenter = newAz; seekerElCenter = newEl; }
	// ID of the thing under the cursor, or FalconNullId
	virtual VU_ID TargetUnderCursor (void)                  { return targetUnderCursor; }

	VirtualDisplay*	GetDisplay(void)                        { return privateDisplay; }
	float			SeekerAz(void)                          { return seekerAzCenter; }
	float			SeekerEl(void)							{ return seekerElCenter; }

#if !NO_REMOTE_BUGGED_TARGET
	FalconEntity *RemoteBuggedTarget;//me123
#endif

protected:
	// Used for things in the target list
	virtual void SetSensorTarget( SimObjectType* newTarget );
	// Creates a "dummy" SimObjectType whose rel geom isn't updated
	virtual void SetSensorTargetHack( FalconEntity* newTarget );	
	// Handle sim/camp handoff and target death
	virtual void CheckLockedTarget( void );							

	SimObjectType	*FindNextDetected(SimObjectType*);
	SimObjectType	*FindPrevDetected(SimObjectType*);
	// ID of the target under the player's cursors
	VU_ID					targetUnderCursor;
	// target under the player's cursors last scan
	SimObjectType*		lasttargetUnderCursor;

	// Targeting
	// Current "primary" target -- who we're locked onto
	SimObjectType	*lockedTarget;			
	// Is this sensor turned on?
	int				isOn;					
	// Sensor state
	float		seekerAzCenter, seekerElCenter,oldseekerElCenter;
	SensorType	sensorType;
};

SensorClass* FindSensor (SimMoverClass* theObject, int sensorType);

#endif

