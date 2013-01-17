////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Includes.
#pragma optimize( "", off )

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "Graphics/Include/grtypes.h"
#include "Graphics/Include/RenderWire.h"
#include "Graphics/Include/terrtex.h"
#include "Graphics/Include/Drawpole.h"
#include "Graphics/Include/TimeMgr.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/include/renderow.h"
#include "codelib/tools/lists/lists.h"
#include "ui95/chandler.h"
#include "ui95/cthook.h"
#include "vu2.h"
#include "sim/include/simbase.h"
#include "sim/include/phyconst.h"
#include "falcmesg.h"
//#include "dispcfg.h"
#include "acmiUI.h"
#include "ui/include/textids.h"
#include "acmitape.h"
#include "AcmiView.h"
#include "FalcLib/include/dispopts.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Defines and constants.

#define NSECTIONS			12
#define	S_FRAME_NUM			0

#define	ST_FRAME_NUM_DEC	0
#define	ST_FRAME_NUM_INC	1

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// References.

void CalcTransformMatrix(SimBaseClass* theObject);
extern float CalcKIAS( float, float );

/*
enum
{					
	ISO_CAM	=200165,
};
*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int	GLOBAL_WIRE_COCKPIT = 1; // default ON.

								
extern char FalconDataDirectory[_MAX_PATH];
extern char FalconPictureDirectory[_MAX_PATH]; // JB 010623
extern bool g_bNewAcmiHud;

extern C_Handler 
	*gMainHandler;

extern BOOL
	acmiDraw;

extern RECT
	acmiSrcRect,
	acmiDestRect;

extern ACMIView
	*acmiDrive;

extern int TESTBUTTONPUSH;
	
static DWORD
	frameStart,
	lastFrame,
	frameTime,
	fcount;

BOOL 
	camTrans = FALSE,
	frameAdvance = FALSE;



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::VectorTranslate(Tpoint*)
{	
	/*
	acmiCamPos.x += 
	(
		tVector->x * Camera()->Rotation().M11 + 
		tVector->y * Camera()->Rotation().M12 + 
		tVector->z * Camera()->Rotation().M13
	);

	acmiCamPos.y += 
	(
		tVector->x * Camera()->Rotation().M21 +
		tVector->y * Camera()->Rotation().M22 - 
		tVector->z * Camera()->Rotation().M23
	);

	acmiCamPos.z -=
	(
		-1 * tVector->x * Camera()->Rotation().M31
		+ tVector->y * Camera()->Rotation().M32 + 
		tVector->z * Camera()->Rotation().M33
	);
	*/

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::VectorToVectorTranslation(Tpoint*, Tpoint*)
{	
	/*
	offSetV->x += 
	(
		tVector->x * Camera()->Rotation().M11 +
		tVector->y * Camera()->Rotation().M12 + 
		tVector->z * Camera()->Rotation().M13
	);

	offSetV->y += 
	(
		tVector->x * Camera()->Rotation().M21 +
		tVector->y * Camera()->Rotation().M22 - 
		tVector->z * Camera()->Rotation().M23
	);

	offSetV->z -= 
	(
		-1 * tVector->x * Camera()->Rotation().M31 +
		tVector->y * Camera()->Rotation().M32 + 
		tVector->z * Camera()->Rotation().M33
	);
	*/
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::SelectCamera(long camSel)
{
	if ( camSel != FREE_CAM )
	{
		_camPos.x = -100.0f;
		_camPos.y = 0.0f;
		_camPos.z = 0.0f;
	}
	_camRange = -500.0f;
	_camRoll = 0.0f;

	switch(camSel)
	{
		case INTERNAL_CAM:			//internal
			GLOBAL_WIRE_COCKPIT = 1;
			_cameraState = INTERNAL_CAM;
			break;
		case EXTERNAL_CAM:			// orbit
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = EXTERNAL_CAM;
			break;
		case TRACKING_CAM:			// orbit
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = TRACKING_CAM;
			break;
		case CHASE_CAM:			//Chase
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = CHASE_CAM;
			break;
		case SAT_CAM:		// Satellite
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = SAT_CAM;

			// default sat view to looking straight down at 5000ft
			_camPitch = -90.0f * DTR;
			_camRange = -15000.0f;
			break;
		case ISO_CAM:		// ISOMETRIC
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = ISO_CAM;
			_camRange = -15000.0f;
			_camPitch =-25.0F * DTR;
			break;
		case FREE_CAM:			// Free
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = FREE_CAM;

			// default camera to last known position
			_camWorldPos.x += _camPos.x;
			_camWorldPos.y += _camPos.y;
			_camWorldPos.z += _camPos.z;

			break;
		default:
			GLOBAL_WIRE_COCKPIT = 0;
			_cameraState = EXTERNAL_CAM;
			break;
	};

	ResetPanner();
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::SetUIVector(Tpoint*) 
{ 
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::InitUIVector() 
{ 
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::SwitchCameraObject(long cameraObject) 
{ 
	int
		i,
		numEntities;

	F4Assert(Tape() != NULL);
	F4Assert(_entityUIMappings != NULL);

	numEntities = Tape()->NumEntities();
	for(i = 0; i < numEntities; i++)
	{
		if(cameraObject == _entityUIMappings[i].listboxId)
		{
			SetCameraObject(i);
			return;
		}
	}


}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::SwitchTrackingObject(long cameraObject) 
{ 
	int 
		i,
		numEntities;

	F4Assert(Tape() != NULL)
	F4Assert(_entityUIMappings != NULL);

	numEntities = Tape()->NumEntities();
	for(i = 0; i < numEntities; i++)
	{
		if(cameraObject == _entityUIMappings[i].listboxId)
		{
			SetTrackingObject(i);
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::Exec()
{
	Tpoint pos;

	if( !acmiDraw || !_tapeHasLoaded || Tape() == NULL || !Tape()->IsLoaded())
		return;
	
	if(fcount++ % 10 == 0)
	{
		frameStart = timeGetTime();
		frameTime = frameStart - lastFrame;
		lastFrame = frameStart;
	}


	//update the tape
	Tape()->Update( -1.0 );

	// set the camera's position and rotation matrix
	UpdateViewPosRot();
	

	pos.x = _camWorldPos.x + _camPos.x;
	pos.y = _camWorldPos.y + _camPos.y;
	pos.z = _camWorldPos.z + _camPos.z;

	// check view isn't below ground
	float groundZ= _renderer->viewpoint->GetGroundLevel( pos.x, pos.y );

	if ( pos.z > groundZ - 10.0f )
		pos.z = groundZ - 10.0f;


	Viewpoint()->Update(&pos);

	// draw the ACMI View
//	Draw();
	
	// ruurrr?
	TheTimeManager.SetTime(DWORD(Tape()->SimTime() * 1000));
											
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::Draw()
{
	static int lastButton = -1;
	Tpoint
		pos;

	Trotation
		rot;

	int i, numEntities;
	SimTapeEntity *ep;
	SimTapeEntity *targep;
	int targindex;

	TCHAR speedstring[20];
	TCHAR altitudestring[20];
	TCHAR headingstring[20];

	float ACMI_heading=0;
	float ACMI_altitude=0;
	float mph			= 0.0f;

	Tpoint posb;
	ThreeDVertex spos;

	if(TapeHasLoaded() && Tape() != NULL && Tape()->IsLoaded())
	{	
		gMainHandler->Unlock(); // Make surface available...

		// pos = _camPos;
		pos.x = 0.0f;
		pos.y = 0.0f;
		pos.z = 0.0f;
		rot = _camRot;

		// make sure the internal hud view object isn't displayed
		if((_cameraState == INTERNAL_CAM) )
			Tape()->RemoveEntityFromFrame( CameraObject() );

		// edg: ?????
		/*
		** Remove Bing-ism
		** causing a crash ....
		if(TESTBUTTONPUSH == 0 && TESTBUTTONPUSH != lastButton)
		{
			lastButton = TESTBUTTONPUSH;
			TheTerrTextures.SetOverrideTexture( wireTexture.TexHandle() );
			Viewpoint()->Update(&pos);
			_renderer->StartFrame();
			_renderer->DrawScene(&pos, &rot);
			_renderer->PostSceneCloudOcclusion();
			_renderer->FinishFrame();
			Viewpoint()->Update(&_camWorldPos);
		}
		*/

		//JAM 16Dec03
		if(DisplayOptions.bZBuffering)
			_renderer->context.SetZBuffering(TRUE);

		// start the render frame + Draw
		_renderer->context.StartFrame();
		_renderer->StartDraw();

		// render the 3d view
		_renderer->DrawScene(&pos, &rot);

		//JAM 12Dec03 - ZBUFFERING OFF
		if(DisplayOptions.bZBuffering)
			_renderer->context.FlushPolyLists();

//		_renderer->PostSceneCloudOcclusion();

		// now we do post-3d rendering
		// this includes putting in things like alt poles( shouldn't really
		// be done here), crapola hud, and some label stuff.  We need
		// to traverse the entity list on the tape in order to do this

		numEntities = Tape()->NumEntities();

		// 1st pass, turn off target boxes (bleck)
		for(i = 0; i < numEntities; i++)
		{
			// get the entity
			ep = Tape()->GetSimTapeEntity( i );
			// do these only for aircraft
			if ( ep->flags & ENTITY_FLAG_AIRCRAFT )
			{
				// default target box to off
				((DrawablePoled *)ep->objBase->drawPointer)->SetTarget( FALSE );
			}
		}

		for(i = 0; i < numEntities; i++)
		{
			// get the entity
			ep = Tape()->GetSimTapeEntity( i );

			// is it in existance at the moment?
			// also no poles or anything on chaff and flares
			if ( (ep->flags & ( ENTITY_FLAG_CHAFF | ENTITY_FLAG_FLARE ) ) ||
				 !Tape()->IsEntityInFrame( i ) )
			{
				continue;
			}

			// get world pos and screen pos of object
			ep->objBase->drawPointer->GetPosition(&pos);
			_renderer->TransformPoint( &pos, &spos);


			// radar target line
			if ( _doLockLine == 1 )
			{
				targindex = Tape()->GetEntityCurrentTarget( i );
				if ( targindex != -1 )
				{
					// get the target entity
					targep = Tape()->GetSimTapeEntity( targindex );
					ep->objBase->drawPointer->GetPosition(&pos);
					targep->objBase->drawPointer->GetPosition(&posb);

					rot = ((DrawableBSP *)ep->objBase->drawPointer)->orientation;

					// start line out in front of aircraft
					pos.x += rot.M11 * ep->objBase->drawPointer->Radius();
					pos.y += rot.M21 * ep->objBase->drawPointer->Radius();
					pos.z += rot.M31 * ep->objBase->drawPointer->Radius();


					// do target boxes and lines
					if ( _cameraState != FREE_CAM )//&& targindex == CameraObject() )//me123 we wanna see all lock lines
					{
						// current attached camera object is target
						if ( targep->flags & ENTITY_FLAG_AIRCRAFT ) 
						{
							((DrawablePoled *)targep->objBase->drawPointer)->SetTarget( TRUE );
							((DrawablePoled *)targep->objBase->drawPointer)->SetTargetBoxColor( 0xff00ffff );
						}
						
						_renderer->SetColor (0xff00ffff);
						_renderer->Render3DLine( &pos, &posb);

					} 
					else if ( _cameraState != FREE_CAM && i == CameraObject() )
					{
						// current attached camera object's target
						if ( targep->flags & ENTITY_FLAG_AIRCRAFT ) 
						{
							((DrawablePoled *)targep->objBase->drawPointer)->SetTarget( TRUE );
							((DrawablePoled *)targep->objBase->drawPointer)->SetTargetBoxColor( 0xffffffff );
						}
						_renderer->SetColor (0xffffffff);
						_renderer->Render3DLine( &pos, &posb);
					} 
					else if ( _cameraState == FREE_CAM )
					{
						_renderer->SetColor (0xff00ffff);
						_renderer->Render3DLine( &pos, &posb);
					} 

				} // if target
			} // if radar lines on


			// if we're in internal cam and we're the target object
			// display the "hud"
			if((_cameraState == INTERNAL_CAM) && i == CameraObject())
			{
			
				//HEADING
				ACMI_heading = ep->yaw * RTD;
				if ( ACMI_heading < 0.0f )
					ACMI_heading += 360.0f;
	
				// ALTITUDE
				ACMI_altitude = -(ep->z);
	
				// SPEED
				// mph=ep->aveSpeed * FTPSEC_TO_KNOTS;
				mph = CalcKIAS( ep->aveSpeed, -ep->z );
	
	
				// WIRE COCKPIT.
				if(GLOBAL_WIRE_COCKPIT == 1)
				{
					_renderer->SetColor (0xff00ff00);
					_renderer->Line( -.8f,-1.0f,-0.5f,-0.5f );
					_renderer->Line( -0.5f,-0.5f,0.5f,-0.5f );
					//_renderer->Line(  0.5f,-0.5f,1.0f,-1.3f );
					_renderer->Line(  0.5f,-0.5f,1.0f,-1.4f );
													
					_renderer->Line(  -0.5f,-0.5f,-.5f,.5f );
					_renderer->Line(  -.5f,.5f, .5f,.5f );
					_renderer->Line(  .5f,.5f, .5f,-.5f );
					if (g_bNewAcmiHud) {
						sprintf(speedstring,"%0.0f",mph);
						sprintf(altitudestring,"%0.0f",ACMI_altitude);
						sprintf(headingstring,"%0.0f",ACMI_heading);
						int ofont = _renderer->CurFont();
						_renderer->SetFont(2);
						_renderer->TextLeft(-0.48f, 0, speedstring);
						_renderer->TextRight(0.48f, 0, altitudestring);
						_renderer->TextCenter(0, 0.48f, headingstring);
						_renderer->TextCenter(0, -0.45f, ((DrawableBSP*)ep->objBase->drawPointer)->Label());
						_renderer->SetFont(ofont);
					} else {
						sprintf(speedstring,"%s: %0.0f",gStringMgr->GetString(TXT_AIRSPEED),mph);
						sprintf(altitudestring,"%s: %0.0f",gStringMgr->GetString(TXT_ALTITUDE),ACMI_altitude);
						sprintf(headingstring,"%s: %0.0f",gStringMgr->GetString(TXT_HEADING),ACMI_heading);
						_renderer->ScreenText(250.0f,160.0f,((DrawableBSP*)ep->objBase->drawPointer)->Label(),0);
						_renderer->ScreenText(250.0f,170.0f,speedstring,0);
						_renderer->ScreenText(250.0f,180.0f,altitudestring,0);
						_renderer->ScreenText(250.0f,190.0f,headingstring,0);
					}
					_renderer->SetColor (0xffff0000);
				}
			}
		}

		// tell renderer we're done
		_renderer->EndDraw();
		_renderer->context.FinishFrame(NULL);
		if(_takeScreenShot)
		{
			TakeScreenShot();
		}
		gMainHandler->Lock();

		// update the entities
		// MUST be done after render
		Tape()->UpdateSimTapeEntities();

	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::TakeScreenShot()
{
	char 
		fileName[_MAX_PATH];

	char
		tmpStr[_MAX_PATH];

	time_t
		ltime;

	struct tm*
		today;

   time(&ltime);
   // _takeScreenShot = FALSE;
   today = localtime(&ltime);

   strftime
	(
		tmpStr,
		_MAX_PATH-1,
      "%m.%d.%Y-%H.%M.%S",
		today 
	);

#if 0
   sprintf (fileName, "%s\\%s", FalconDataDirectory, tmpStr);
#else
   sprintf (fileName, "%s\\%s", FalconPictureDirectory, tmpStr);
#endif

	gMainHandler->GetFront()->BackBufferToRAW(fileName);
}


/*
** Name: UpdateViewPosRot
** Description:
**		Updates the view world position and rotation depending on
**		the camera setting and what we're tracking and panner positions
*/
void ACMIView::UpdateViewPosRot( void )
{
	int camObj;
	int trackObj;
	SimTapeEntity *camEnt;
	SimTapeEntity *trackEnt;
	Tpoint dPos;
	float dT;
	float dRoll;
	float dist;
	float objScale;

	// get any camera objects we might need
	camObj = CameraObject();
	trackObj = TrackingObject();
	camEnt = Tape()->GetSimTapeEntity( camObj );
	trackEnt = Tape()->GetSimTapeEntity( trackObj );

	// get current scaling
	objScale = Tape()->GetObjScale();


	// if we're not in free camera mode, our world position is
	// based on the camera object
	if ( _cameraState != FREE_CAM )
	{
		_camWorldPos.x = camEnt->x;
		_camWorldPos.y = camEnt->y;
		_camWorldPos.z = camEnt->z;
	}

	// first pass:
	// 	get yaw pitch and roll for camera
	switch(_cameraState)
	{
		case INTERNAL_CAM:			//internal
			_camYaw = camEnt->yaw;
			_camPitch = camEnt->pitch;
			_camRoll = camEnt->roll;
			break;
		case EXTERNAL_CAM:			// orbit
			_camYaw += _pannerAz;
			_camPitch += _pannerEl;
			_camRoll = 0.0f;
			_camRange += _pannerX;
			if ( _camRange > -50.0f )
				_camRange = -50.0f;
			break;
		case TRACKING_CAM:			// tracking
			_camYaw += _pannerAz;
			_camPitch += _pannerEl;
			_camRoll = 0.0f;
			_camRange += _pannerX;
			if ( _camRange > -50.0f )
				_camRange = -50.0f;
			break;
		case CHASE_CAM:			//Chase
			_camRange += _pannerX;
			if ( _camRange > -50.0f )
				_camRange = -50.0f;

			// where we want camera to be
			_chaseX = camEnt->objBase->dmx[0][0] * _camRange * objScale;
			_chaseY = camEnt->objBase->dmx[0][1] * _camRange * objScale;
			_chaseZ = camEnt->objBase->dmx[0][2] * _camRange * objScale;

			dT = Tape()->GetDeltaSimTime();
			if ( dT <= 0.0f )
				dT = 0.1f;

			// "spring" constants for camera roll and move
			#define KMOVE			0.29f
			#define KROLL			0.30f
		
			// convert frame loop time to secs from ms
			// dT = (float)frameTime * 0.001;
		
			// get the diff between desired and current camera pos
			dPos.x = _chaseX - _camPos.x;
			dPos.y = _chaseY - _camPos.y;
			dPos.z = _chaseZ - _camPos.z;
		
			// send the camera thataway
			_camPos.x += dPos.x * dT * KMOVE;
			_camPos.y += dPos.y * dT * KMOVE;
			_camPos.z += dPos.z * dT * KMOVE;
		
			// "look at" vector
			dPos.x = -_camPos.x;
			dPos.y = -_camPos.y;
			dPos.z = -_camPos.z;

			// get new camera roll
			dRoll = camEnt->roll - _camRoll;

			// roll in shortest direction
			if ( fabs( dRoll ) > 180.0f * DTR )
			{
				if ( dRoll < 0.0f )
					dRoll = 360.0f * DTR + camEnt->roll - _camRoll;
				else
					dRoll = -360.0f * DTR + camEnt->roll - _camRoll;
			}
		
			// apply roll
			_camRoll += dRoll * dT * KROLL;
		
			// keep chase cam roll with +/- 180
			if ( _camRoll > 1.0f * PI )
				_camRoll -= 2.0f * PI;
			else if ( _camRoll < -1.0f * PI )
				_camRoll += 2.0f * PI;
		
			// now get yaw and pitch based on look at vector
			dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
			_camPitch	= (float)-asin( dPos.z/dist );
			_camYaw	= (float)atan2( dPos.y, dPos.x );

			break;
		case SAT_CAM:		// Satellite
			_camYaw += _pannerAz;
			_camPitch += _pannerEl;
			if ( _camPitch > -45.0f * DTR )
				_camPitch = -45.0f * DTR;
			else if ( _camPitch < -90.0f * DTR )
				_camPitch = -90.0f * DTR;
			_camRoll = 0.0f;
			_camRange += _pannerX * 100.0f;
			if ( _camRange > -1000.0f )
				_camRange = -1000.0f;
			break;
		case ISO_CAM:		// ISOMETRIC
			_camYaw += _pannerAz;
			_camPitch += _pannerEl;
			if ( _camPitch > -15.0f * DTR )
				_camPitch = -15.0f * DTR;
			else if ( _camPitch < -60.0f * DTR )
				_camPitch = -60.0f * DTR;
			_camRoll = 0.0f;
			_camRange += _pannerX * 100.0f;
			if ( _camRange > -50.0f )
				_camRange = -50.0f;
			break;
		case FREE_CAM:			// Free
			_camYaw += _pannerAz;
			_camPitch += _pannerEl;
			_camRoll = 0.0f;
			_pannerX *= 20.0f;
			_pannerY *= 20.0f;
			_pannerZ *= 20.0f;
			// head in the direction we're currently facing (ie based on
			// current cam rotation
			_camWorldPos.x += _camRot.M11 * _pannerX + _camRot.M12 * _pannerY + _camRot.M13 * _pannerZ;
			_camWorldPos.y += _camRot.M21 * _pannerX + _camRot.M22 * _pannerY + _camRot.M23 * _pannerZ;
			_camWorldPos.z += _camRot.M31 * _pannerX + _camRot.M32 * _pannerY + _camRot.M33 * _pannerZ;
			break;
		default:
			_camYaw = camEnt->yaw;
			_camPitch = camEnt->pitch;
			_camRoll = camEnt->roll;
			break;
	};

	// second pass:
	//		create the rotation matrix
	float costha,sintha,cosphi,sinphi,cospsi,sinpsi;

	costha = (float)cos(_camPitch);
	sintha = (float)sin(_camPitch);
	cosphi = (float)cos(_camRoll);
	sinphi = (float)sin(_camRoll);
	cospsi = (float)cos(_camYaw);
	sinpsi = (float)sin(_camYaw);

	_camRot.M11 = cospsi*costha;
	_camRot.M21 = sinpsi*costha;
	_camRot.M31 = -sintha;

	_camRot.M12 = -sinpsi*cosphi + cospsi*sintha*sinphi;
	_camRot.M22 = cospsi*cosphi + sinpsi*sintha*sinphi;
	_camRot.M32 = costha*sinphi;

	_camRot.M13 = sinpsi*sinphi + cospsi*sintha*cosphi;
	_camRot.M23 = -cospsi*sinphi + sinpsi*sintha*cosphi;
	_camRot.M33 = costha*cosphi;

	// third pass:
	//		Set the relative camera positiion
	switch(_cameraState)
	{
		case INTERNAL_CAM:			//internal
			_camPos.x = 0.0f;
			_camPos.y = 0.0f;
			_camPos.z = 0.0f;
			break;
		case EXTERNAL_CAM:			// orbit
			_camPos.x = _camRot.M11 * _camRange * objScale;
			_camPos.y = _camRot.M21 * _camRange * objScale;
			_camPos.z = _camRot.M31 * _camRange * objScale;
			break;
		case TRACKING_CAM:			// orbit
			if ( camObj == trackObj )
			{
				_camPos.x = _camRot.M11 * _camRange * objScale;
				_camPos.y = _camRot.M21 * _camRange * objScale;
				_camPos.z = _camRot.M31 * _camRange * objScale;
			}
			else
			{
				// line up a vector between cam and track obj and place
				// camera on the vector, slightly up
				dPos.x = trackEnt->x - camEnt->x;
				dPos.y = trackEnt->y - camEnt->y;
				dPos.z = trackEnt->z - camEnt->z;
				dist = 1.0F/(float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
				_camPos.x = dPos.x * dist * _camRange * objScale;
				_camPos.y = dPos.y * dist * _camRange * objScale;
				_camPos.z = dPos.z * dist * _camRange *objScale + _camRange * 0.2f;
			}
			break;
		case CHASE_CAM:			//Chase
			break;
		case SAT_CAM:		// Satellite
			_camPos.x = _camRot.M11 * _camRange;
			_camPos.y = _camRot.M21 * _camRange;
			_camPos.z = _camRot.M31 * _camRange;
			break;
		case ISO_CAM:		// ISOMETRIC
			_camPos.x = _camRot.M11 * _camRange;
			_camPos.y = _camRot.M21 * _camRange;
			_camPos.z = _camRot.M31 * _camRange;
			break;
		case FREE_CAM:			// Free
			_camPos.x = 0.0f;
			_camPos.y = 0.0f;
			_camPos.z = 0.0f;
			break;
		default:
			_camPos.x = 0.0f;
			_camPos.y = 0.0f;
			_camPos.z = 0.0f;
			break;
	};

	// fourth pass
	//		if we're tracking an object we need to set a new rotation
	//		matrix
	// determine if we're tracking an object or not
	if (  _cameraState == TRACKING_CAM && camObj != trackObj )
	{
		// get the diff between desired and current camera pos
		// for look at vector
		dPos.x = trackEnt->x - (_camPos.x + _camWorldPos.x);
		dPos.y = trackEnt->y - (_camPos.y + _camWorldPos.y);
		dPos.z = trackEnt->z - (_camPos.z + _camWorldPos.z);

		// now get yaw and pitch based on look at vector
		dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
		float p	= (float)-asin( dPos.z/dist );
		float y	= (float)atan2( dPos.y, dPos.x );

		costha = (float)cos(p);
		sintha = (float)sin(p);
		cosphi = (float)(1.0f);
		sinphi = (float)(0.0f);
		cospsi = (float)cos(y);
		sinpsi = (float)sin(y);
	
		_camRot.M11 = cospsi*costha;
		_camRot.M21 = sinpsi*costha;
		_camRot.M31 = -sintha;
	
		_camRot.M12 = -sinpsi*cosphi + cospsi*sintha*sinphi;
		_camRot.M22 = cospsi*cosphi + sinpsi*sintha*sinphi;
		_camRot.M32 = costha*sinphi;
	
		_camRot.M13 = sinpsi*sinphi + cospsi*sintha*cosphi;
		_camRot.M23 = -cospsi*sinphi + sinpsi*sintha*cosphi;
		_camRot.M33 = costha*cosphi;
	}


	// reset panning
	ResetPanner();

}
