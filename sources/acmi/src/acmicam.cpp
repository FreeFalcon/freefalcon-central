#pragma optimize( "", off )
/* ------------------------------------------------------------------------

    AcmiCam.cpp

    ACMI Camera data and movement


    Version 0.01

    Written by Jim DiZoglio(x257)      (c) 1997 Spectrum Holobyte

   ------------------------------------------------------------------------ */

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <math.h>

#include "Graphics\Include\grTypes.h"
#include "codelib\tools\lists\lists.h"
#include "debuggr.h"
#include "AcmiCam.h"

#define TRACKING_OFFSET	0.15F
//FILE *debugData = NULL;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ACMICamera::ACMICamera()
{
	_pos.x = 0.0F; _pos.y = 0.0F; _pos.z =  0.0F;

	_rot.M11 = 1.0F; _rot.M12 = 0.0F; _rot.M13 = 0.0F;
	_rot.M21 = 0.0F; _rot.M22 = 1.0F; _rot.M23 = 0.0F;
	_rot.M31 = 0.0F; _rot.M32 = 0.0F; _rot.M33 = 1.0F;
	_objectAz = _pannerAz = 0.0f;
	_objectEl = _pannerEl = 0.0f;

//	debugData = fopen("debugData.txt", "wb");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ACMICamera::~ACMICamera()
{
//	fclose(debugData);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::SetType(int type)
{
	_type = type;
	_objectAz = 0.0F;
	_objectEl = 0.0F;
	_objectRoll = 0.0F;
	_localAz = 0.0F;
	_localEl = 0.0F;
	_objectRange = HOME_RANGE;
	_rotate = 0.0F;
	_azDir = 0.0F;
	_elDir = 0.0F;
	_slewRate = 0.1F;
	_tracking = NO_TRACKING;

	switch(_type)
	{
		case ATTACHED_CAM:
		{
			_rotType = OBJECT_ROTATION;
			break;
		}
		case DETTACHED_CAM:
		{
			_rotType = LOCAL_ROTATION;
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::DoAction()
{
	switch(_type)
	{
		case ATTACHED_CAM:
		{
			switch(_action)
			{
				case NO_ACTION:
				{
					SetAzDir(0.0F);
					SetElDir(0.0F);
					SetRotateType(OBJECT_ROTATION);
					break;
				}
				case ZOOM_IN:
				{
					SetObjectRange(0.0F, ZOOM_IN);
					break;
				}
				case ZOOM_OUT:
				{
					SetObjectRange(10.0F, ZOOM_OUT);
					break;
				}
				case LOCAL_RIGHT_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetAzDir(1.0F);
					break;
				}
				case LOCAL_LEFT_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetAzDir(-1.0F);
					break;
				}
				case LOCAL_UP_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetElDir(1.0F);
					break;
				}
				case LOCAL_DOWN_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetElDir(-1.0F);
					break;
				}
				case OBJECT_RIGHT_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetAzDir(-1.0F);
					break;
				}
				case OBJECT_LEFT_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetAzDir(1.0F);
					break;
				}
				case OBJECT_UP_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetElDir(1.0F);
					break;
				}
				case OBJECT_DOWN_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetElDir(-1.0F);
					break;
				}
				case OBJECT_XRT_YUP_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetAzDir(-1.0F);
					SetElDir(1.0F);
					break;
				}
				case OBJECT_XLT_YDN_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetAzDir(1.0F);
					SetElDir(-1.0F);
					break;
				}
				case OBJECT_XRT_YDN_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetAzDir(-1.0F);
					SetElDir(-1.0F);
					break;
				}
				case OBJECT_XLT_YUP_ROT:
				{
					SetRotateType(OBJECT_ROTATION);
					SetAzDir(1.0F);
					SetElDir(1.0F);
					break;
				}
				case ACMI_PANNER:
				{
					SetRotateType(OBJECT_ROTATION);
					break;
				}
			}
			break;
		}
		case DETTACHED_CAM:
		{
			switch(_action)
			{
				case NO_ACTION:
				{
					SetAzDir(0.0F);
					SetElDir(0.0F);
			//		SetRotateType(OBJECT_ROTATION);
					break;
				}
/*				case ZOOM_IN:
				{
			//		SetObjectRange(0.0F, ZOOM_IN);
					SetRotateType(NO_ROTATION);
					break;
				}
				case ZOOM_OUT:
				{
			//		SetObjectRange(10.0F, ZOOM_OUT);
					SetRotateType(NO_ROTATION);
					break;
				}
				case LOCAL_RIGHT_ROT:
				{
					SetRotateType(NO_ROTATION);
			//		SetAzDir(1.0F);
					break;
				}
				case LOCAL_LEFT_ROT:
				{
					SetRotateType(NO_ROTATION);
		//			SetAzDir(-1.0F);
					break;
				}
				case LOCAL_UP_ROT:
				{
					SetRotateType(NO_ROTATION);
		//			SetElDir(1.0F);
					break;
				}
				case LOCAL_DOWN_ROT:
				{
					SetRotateType(NO_ROTATION);
		//			SetElDir(-1.0F);
					break;
				} */
				case OBJECT_RIGHT_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetAzDir(-1.0F);
					break;
				}
				case OBJECT_LEFT_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetAzDir(1.0F);
					break;
				}
				case OBJECT_UP_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetElDir(1.0F);
					break;
				}
				case OBJECT_DOWN_ROT:
				{
					SetRotateType(LOCAL_ROTATION);
					SetElDir(-1.0F);
					break;
				}
			}
			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::UpdatePosition()
{
	Trotation tilt;

	DoAction();

	if(_rotType == LOCAL_ROTATION)
	{
		_localAz += _azDir * _slewRate;
		_localEl += _elDir * _slewRate;
		Rotate(_objectEl + _localEl, 0.0F, _objectAz + _localAz, &_rot);
	}
	else
	{
		// Asmth and elevation angles
		_objectAz += _azDir * _slewRate;
		_objectEl += _elDir * _slewRate;
	}

	// Be around ownship, looking at it
	Tilt(_objectEl, 0.0F, _objectAz, &tilt);
	Translate(_objectRange * tilt.M11, _objectRange * tilt.M12, _objectRange * tilt.M13, &_pos);

	if ( _tracking )
		Rotate(_localEl, 0.0F, _localAz, &_rot);
	else
		Rotate(_objectEl, _objectRoll, _objectAz, &_rot);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::UpdatePannerPosition()
{
	Trotation 
		tilt;

	Rotate(_objectEl, 0.0F, _objectAz, &_rot);

	// Be around ownship, looking at it
	Tilt(_objectEl, 0.0F, _objectAz, &tilt);
	Translate(_objectRange * tilt.M11, _objectRange * tilt.M12, _objectRange * tilt.M13, &_pos);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::Tilt(float pitch, float, float yaw, Trotation *tilt)
{

	float costha, sintha, cospsi, sinpsi;

	// Be around ownship, looking at it
	costha = (float)cos(pitch);
	sintha = (float)sin(pitch);
	cospsi = (float)cos(yaw);
	sinpsi = (float)sin(yaw);

	tilt->M11 = cospsi * costha;
	tilt->M12 = sinpsi * costha;
	tilt->M13 = -sintha;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::Rotate(float pitch, float roll, float yaw, Trotation* viewRotation)
{
	float costha,sintha,cosphi,sinphi,cospsi,sinpsi;

	costha = (float)cos(pitch);
	sintha = (float)sin(pitch);
	cosphi = (float)cos(roll);
	sinphi = (float)sin(roll);
	cospsi = (float)cos(yaw);
	sinpsi = (float)sin(yaw);

	viewRotation->M11 = cospsi*costha;
	viewRotation->M21 = sinpsi*costha;
	viewRotation->M31 = -sintha;

	viewRotation->M12 = -sinpsi*cosphi + cospsi*sintha*sinphi;
	viewRotation->M22 = cospsi*cosphi + sinpsi*sintha*sinphi;
	viewRotation->M32 = costha*sinphi;

	viewRotation->M13 = sinpsi*sinphi + cospsi*sintha*cosphi;
	viewRotation->M23 = -cospsi*sinphi + sinpsi*sintha*cosphi;
	viewRotation->M33 = costha*cosphi;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::IncrementPannerAzEl(int currentAction, float az, float el)
{
	_action = currentAction;
	_objectAz = _pannerAz + az;
	_objectEl = _pannerEl + el;

	MonoPrint("PannA: %lf\n", _pannerAz);
	MonoPrint("PannE: %lf\n", _pannerEl);
	MonoPrint("ObjeA: %lf\n", _objectAz);
	MonoPrint("ObjeE: %lf\n", _objectEl);

	SetAzDir(0.0F);
	SetElDir(0.0F);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::SetObjectRange(float diff, int instruction)
{
	switch(instruction)
	{
		case ZOOM_IN:
			_objectRange = min(-50.0F, _objectRange + 50.0F);
			break;
		case ZOOM_OUT:
			_objectRange -= diff;
			break;
		case HOME:
			_objectRange = diff;
			break;
	}
	
	SetRotateType(OBJECT_ROTATION);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::TrackPoint(const Tpoint &trackingPt)
{
	float
		trackingAz,
		trackingEl;

	float  
		deltaX,
		deltaY,
		deltaZ,
		deltaRange;

	if(_tracking == NO_TRACKING)
		return;

	deltaX = trackingPt.x - (_worldPos.x + _pos.x );
	deltaY = trackingPt.y - (_worldPos.y + _pos.y );

   if(deltaX)
		trackingAz = (float)atan2(deltaY, deltaX);
	else
		trackingAz = 0.0F;

	deltaZ = (_worldPos.z + _pos.z ) - trackingPt.z;

#if 1
	deltaRange = 
	(
		(float)sqrt
		(
			deltaX * deltaX +
			deltaY * deltaY + 
			deltaZ * deltaZ
		)
	);

	if(deltaRange != 0.0F)
	{
		deltaRange = (deltaZ / deltaRange);
	}
	
	trackingEl = (float)asin(deltaRange);
#else
	trackingEl = (float)atan2(sqrt(deltaX*deltaX+deltaY*deltaY), deltaZ);
#endif

	/*
	switch(_tracking)
	{
		case LOCAL_TRACKING:
		{
			SetLocalAz((float)trackingAz);
			SetLocalEl((float)trackingEl);
			break;
		}
		case GLOBAL_TRACKING:
		{
			SetObjectAz((float)trackingAz);
			SetObjectEl((float)trackingEl - TRACKING_OFFSET);
			break;
		}
	}
	*/
	SetLocalAz((float)trackingAz);
	SetLocalEl((float)trackingEl);
	// SetObjectAz((float)trackingAz);
	// SetObjectEl((float)trackingEl);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMICamera::UpdateChasePosition( float dT )
{
	Tpoint dPos;
	float dRoll;
	float dist;

	DoAction();

	if ( dT < 0.0f )
		dT = -dT;
	else if ( dT > 0.0f )
		dT = dT;

	if(_rotType == LOCAL_ROTATION)
	{
		_localAz += _azDir * _slewRate;
		_localEl += _elDir * _slewRate;
	}
	else
	{
		// Asmth and elevation angles
		_objectAz += _azDir * _slewRate;
		_objectEl += _elDir * _slewRate;
	}

	// Be around ownship, looking at it
	// Tilt(_objectEl, 0.0F, _objectAz, &tilt);
	// Translate(_objectRange * tilt.M11, _objectRange * tilt.M12, _objectRange * tilt.M13, &_pos);

	/*
	if ( _tracking )
		Rotate(_localEl, 0.0F, _localAz, &_rot);
	else
		Rotate(_objectEl, _objectRoll, _objectAz, &_rot);
	*/

	 // "spring" constants for camera roll and move
	 #define KMOVE			0.29f
	 #define KROLL			0.30f

	 // convert frame loop time to secs from ms
	 // dT = (float)frameTime * 0.001;

	 // get the diff between desired and current camera pos
	 dPos.x = _chasePos.x - _pos.x;
	 dPos.y = _chasePos.y - _pos.y;
	 dPos.z = _chasePos.z - _pos.z;

	 // send the camera thataway
	 _pos.x += dPos.x * dT * KMOVE;
	 _pos.y += dPos.y * dT * KMOVE;
	 _pos.z += dPos.z * dT * KMOVE;

	 // "look at" vector
	 dPos.x = -_pos.x;
	 dPos.y = -_pos.y;
	 dPos.z = -_pos.z;

	 // get new camera roll
	 if ( _objectRoll < -0.5f * PI && _chaseRoll > 0.5f * PI )
	 {
		 dRoll = _objectRoll + (2.0f * PI) - _chaseRoll;
	 }
	 else if ( _objectRoll > 0.5f * PI && _chaseRoll < -0.5f * PI )
	 {
		 dRoll = _objectRoll - (0.5f * PI) - _chaseRoll;
	 }
	 else // same sign
	 {
		dRoll = _objectRoll - _chaseRoll;
	 }

	 // apply roll
	 _chaseRoll += dRoll * dT * KROLL;

	 // keep chase cam roll with +/- 180
	 if ( _chaseRoll > 1.0f * PI )
		_chaseRoll -= 2.0f * PI;
	 else if ( _chaseRoll < -1.0f * PI )
		_chaseRoll += 2.0f * PI;

	 // now get yaw and pitch based on look at vector
	 dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
	 _objectEl	= (float)-asin( dPos.z/dist );
	 _objectAz	= (float)atan2( dPos.y, dPos.x );
	 Rotate(_objectEl, _chaseRoll, _objectAz, &_rot);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
