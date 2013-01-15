#ifndef _ACMICAM_H_
#define _ACMICAM_H_

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define DETTACHED_CAM		0
#define ATTACHED_CAM			1

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define LOCAL_ROTATION		0
#define OBJECT_ROTATION		1

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define NUM_TRACKING_CAMS	2

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define NO_TRACKING			0
#define LOCAL_TRACKING		1
#define GLOBAL_TRACKING		2

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define NO_ACTION				0
#define ZOOM_IN				1
#define ZOOM_OUT				2
#define LOCAL_RIGHT_ROT		3
#define LOCAL_LEFT_ROT		4
#define LOCAL_UP_ROT			5
#define LOCAL_DOWN_ROT		6
#define OBJECT_RIGHT_ROT	7
#define OBJECT_LEFT_ROT		8
#define OBJECT_UP_ROT		9
#define OBJECT_DOWN_ROT		10
#define OBJECT_XRT_YUP_ROT	11
#define OBJECT_XLT_YDN_ROT	12
#define OBJECT_XRT_YDN_ROT	13
#define OBJECT_XLT_YUP_ROT	14
#define	NO_ROTATION			15
#define HOME					16
#define ACMI_PANNER			17

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define HOME_RANGE			-300.0F

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ACMICamera
{
public:
	// Constructors.
	ACMICamera();

	// Destructor.
	~ACMICamera();

	// Access.
	void SetType(int type);
	int  Type() const;

	void SetAction(int action);
	void SetAction(int action, float az, float el);
	int Action() const;

	void ToggleTracking();
	void SetTracking(int n);
	int Tracking() const;	

	void SetRotateType(int type);
	int RotateType() const;

	void SetPosition(const Tpoint &pos);
	void GetPosition(Tpoint &pos) const;
	const Tpoint &Position() const;

	void SetRotation(const Trotation &rot);
	void GetRotation(Trotation &rot) const;
	const Trotation &Rotation() const;

	void SetWorldPosition(const Tpoint &pos);

	void SetElDir(float diff);
	void SetAzDir(float diff);
	void SetObjectEl(float diff);
	void SetObjectAz(float diff);
	void SetObjectRoll(float diff);
	void SetPannerAz();
	void IncrementPannerAzEl(int currentAction, float az, float el);
	void SetLocalEl(float diff);
	void SetLocalAz(float diff);
	float El() const;
	float Az() const;
	void SetObjectRange(float diff, int instruction);
	float GetObjectRange( void )
	{
		return _objectRange;
	};
	void SetSlewRate(float diff);
	void TrackPoint(const Tpoint &trackingPt);

	// Update methods.
	void UpdatePosition();
	void UpdateChasePosition( float dT );
	void UpdatePannerPosition();

	void Tilt
	(
		float pitch,
		float roll,
		float yaw,
		Trotation *tilt
	);
	void Rotate
	(
		float pitch,
		float roll,
		float yaw,
		Trotation *viewRotation
	);
	void Translate
	(
		float x,
		float y,
		float z,
		Tpoint* camView
	);
	
	CRITICAL_SECTION    criticalSection;

	void SetChasePosition( Tpoint *pos )
	{
		_chasePos.x = pos->x;
		_chasePos.y = pos->y;
		_chasePos.z = pos->z;
	};

	void SetChaseRoll( float roll )
	{
		_chaseRoll = roll;
	};

private:
	// Detached or attached?
	int				_type;		
	
	// Detached or attached?
	int				_rotType;				

	// Camera position relative to object.
	Tpoint			_pos;

	// World coords of camera position.
	Tpoint			_worldPos;			

	// Camera rotation matrix.
	Trotation		_rot;

	float				_objectAz, _localAz, _pannerAz;			
	float				_objectEl, _localEl, _pannerEl;		
	float				_objectRoll;

	// For a dettached camera, set this value to 0.0F
	float				_objectRange;			

	// Used to rotate around self.
	float				_rotate;					

	// Used to rotate around object.
	float				_objectRotate;			

	float				_azDir;
	float				_elDir;
	float				_slewRate;
	int				_action;
	int				_tracking;
	Tpoint			_chasePos;
	Tpoint			_chaseObjPos;
	float			_chaseRoll;

	void DoAction();
};

#include "acmcminl.cpp"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // _ACMICAM_H_ 


