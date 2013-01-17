#include "Graphics/Include/drawShdw.h"
#include "Graphics/Include/drawGuys.h"
#include "stdhdr.h"
#include "Classtbl.h"
#include "object.h"
#include "falcmesg.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "aircrft.h"
#include "weather.h"
#include "simeject.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/DeathMessage.h"
#include "campbase.h"
#include "simdrive.h"
#include "acmi/src/include/acmirec.h"
#include "camp2sim.h"
#include "otwdrive.h"
#include "falcsess.h"
#include "fsound.h"
#include "SimVuDrv.h"
#include "MsgInc/RadioChatterMsg.h"
#include "falcsnd/conv.h"
#include "simveh.h"
#include "airunit.h"
#include "rules.h"
#include "falcsnd/voicemanager.h"
#include "GameMgr.h"
#include "team.h"
#include "dofsnswitches.h"
#include "fakerand.h"
//sfr: added for checks
#include "InvalidBufferException.h"

// References.

void CalcTransformMatrix (SimBaseClass *theObject);
GridIndex SimToGrid (float x);

// Defines.
#define GRAVITY_ACCEL	32.2F			// ft/(sec*sec)
#define KG_TO_SLUGS		0.06852F		// kilogram to slugs conversion factor
#define AIR_DENSITY		0.0025026F	// air density at sea level in slugs/(ft*ft*ft)

#define DEBUG_EJECTION_SEQUENCE		FALSE

// Statics.
int		EjectedPilotClass::_classType = 0;
BOOL		EjectedPilotClass::_classTypeFound = FALSE;

// NOTE : The motion model is pretty unoptimized
//		Once it's fully tweaked, we can add the following optimizations (and probably more).
//		Get rid of run-time divides, by storing both mass and 1/mass for each stage of the sequence.
//		Get rid of some of the function call overhead, either by making functions inline or by
//			the handy old cut-n-paste job.
//
//		Physical and model data can be added for new ejection modes (for different planes, for instance).
//		To add a new mode, add a new #define for the mode in simeject.h, add a EP_PHYS_DATA structure 
//    (see F16Mode1PhysicalData below), and add an EP_MODEL_DATA structure (see F16ModelData below), and 
//		add a new case to the switch statement in SetMode(), to handle pointing at the data for the new 
//		ejection mode data.

// F-16 mode 1 player ejected pilot aero data.
EP_PHYS_DATA	F16Mode1PhysicalData =
{
	// Stage-dependent data.
	{
		// Jettison canopy stage.
		{
			// End stage time (in seconds).
			0.0F,
			// Drag factor (atmospheric density * cross-sectional area * drag coefficient).
			AIR_DENSITY * 8.0F * 1.0F,
			// Mass (in slugs).
			300.0F * KG_TO_SLUGS
		},
		// Eject seat stage
		{
			// End stage time (in seconds).
			1.5F,
			// Drag factor (atmospheric density * cross-sectional area * drag coefficient).
			AIR_DENSITY * 8.0F * 1.0F,
			// Mass (in slugs).
			300.0F * KG_TO_SLUGS
		},
		// Free fall stage with seat, chute closed
		{
			// End stage time (in seconds).
			1.5F,
			// Drag factor (atmospheric density * cross-sectional area * drag coefficient).
			AIR_DENSITY * 8.0F * 1.0F,
			// Mass (in slugs).
			300.0F * KG_TO_SLUGS
		},
		// Chute opening
		{
			// End stage time (in seconds).
			7.5F,
			// Drag factor (atmospheric density * cross-sectional area * drag coefficient).
			AIR_DENSITY * 70.0F * 1.2F,
			// Mass (in slugs).
			100.0F * KG_TO_SLUGS
		},
		// Free fall stage with open parachute, no seat
		{
			// End stage time (in seconds).
			10000.0F,
			// Drag factor (atmospheric density * cross-sectional area * drag coefficient).
			AIR_DENSITY * 150.0F * 50.2F,	//MI make chute slower, was *1.2F
			// Mass (in slugs).
			100.0F * KG_TO_SLUGS
		},
		// Free fall stage with collapsed parachute, no seat
		{
			// End stage time (in seconds).
			10000.0F,
			// Drag factor (atmospheric density * cross-sectional area * drag coefficient).
			AIR_DENSITY * 4.0F * 0.8F,
			// Mass (in slugs).
			100.0F * KG_TO_SLUGS
		}
	},
	// Speed of pilot at time of ejection (in ft/s).
	20.0F,
	// Acceleration of ejection seat from thrust (ft/(sec*sec))
	7.0F * GRAVITY_ACCEL,
	// Angle of ejection (in radians).
	PI/2.0F,
	// Pitch of pilot in free fall with chute.
	// This value must be negative!
	// -PI/10,
	0.0F,
	// Pitch decay of pilot in free fall with chute (radians/sec)
	// This value must be positive!
	0.0F, 
	//PI/200,
	// Delta yaw of pilot in free fall with chute (radians/sec)
	1.0F * DTR,
	// Seat offset from center of plane
	9.5F, 0.0F, 0.0F,
	// Player pilot end stage time adjustment.
	// This is added to the end stage time for the player pilot.
	0.5F
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

EP_MODEL_DATA	F16ModelData[MD_NUM_MODELS] =
{
	// pilot and seat.
	EP_MODEL_DATA
	(
		// bsp model id.
		VIS_EJECT2,
		// physical stage that this model is created in.
		PD_EJECT_SEAT,
		// camera mode.
		1,
		// focus point offset in model space.
		EP_VECTOR
		(
			0.0,
			0.0,
			0.0
		)
	),
	// pilot and open chute.
	EP_MODEL_DATA
	(
		// bsp model id.
		VIS_EJECT1,
		// physical stage that this model is created in.
		PD_CHUTE_OPENING,
		// camera mode.
		1,
		// focus point offset in model space.
		EP_VECTOR
		(
			0.0,
			0.0,
			70.0
		)
	),
	// pilot and collapsed chute.
	EP_MODEL_DATA
	(
		// bsp model id.
		VIS_EJECT3,
		// physical stage that this model is created in.
		PD_FREE_FALL_WITH_COLLAPSED_CHUTE,
		// camera mode.
		1,
		// focus point offset in model space.
		EP_VECTOR
		(
			0.0,
			0.0,
			70.0
		)
	),
	// Safe Landing
	EP_MODEL_DATA
	(
		// bsp model id.
		VIS_EJECT4,
		// physical stage that this model is created in.
		PD_SAFE_LANDING,
		// camera mode.
		2,
		// focus point offset in model space.
		EP_VECTOR
		(
			0.0,
			0.0,
			70.0
		)
	),
	// Crash Landing
	EP_MODEL_DATA
	(
		// bsp model id.
		VIS_EJECT5,
		// physical stage that this model is created in.
		PD_CRASH_LANDING,
		// camera mode.
		2,
		// focus point offset in model space.
		EP_VECTOR
		(
			0.0,
			0.0,
			70.0
		)
	)
};

EjectedPilotClass::EjectedPilotClass (VU_BYTE** stream, long *rem) : SimMoverClass(stream, rem)
{
   VU_ID airVuId;
   int ejMode;
   AircraftClass *parent;

   memcpychk (&airVuId, stream, sizeof (VU_ID), rem);
   memcpychk (&ejMode, stream, sizeof (int), rem);

   parent = (AircraftClass *)vuDatabase->Find( airVuId );

   InitLocalData( parent, ejMode, 0);
}



int EjectedPilotClass::SaveSize()
{
   return SimMoverClass::SaveSize() +
   		  sizeof (VU_ID) +
		  sizeof( int );
}

int EjectedPilotClass::Save(VU_BYTE **stream)
{
int saveSize = SimMoverClass::Save (stream);
int startMode = EM_F16_MODE1;

   memcpy (*stream, &_aircraftId, sizeof (VU_ID));
   *stream += sizeof (VU_ID);
   memcpy (*stream, &startMode, sizeof (int));
   *stream += sizeof (int);
   return (saveSize + sizeof(VU_ID) + sizeof (int));
}

EjectedPilotClass::EjectedPilotClass(AircraftClass *ac, int mode, int no) :
	SimMoverClass(EjectedPilotClass::ClassType())
{
	InitLocalData(ac, mode, no);
}

EjectedPilotClass::~EjectedPilotClass()
{
	CleanupLocalData();
}

void EjectedPilotClass::InitData(){
	SimMoverClass::InitData();
	InitLocalData(NULL, EM_F16_MODE1, 0);
}

void EjectedPilotClass::InitLocalData(AircraftClass *ac, int mode, int no){
	DrawableBSP		*acBSP;
	int				labelLen;

	_delayTime = SimLibElapsedTime + no * 2 * CampaignSeconds;
	// Initialize position, rotation, velocity, angular velocity.
	if (ac)
	{
		_pos = EP_VECTOR(ac->XPos(),ac->YPos(),ac->ZPos());

		_rot[I_ROLL] =	ac->Roll();
		_rot[I_PITCH] = ac->Pitch();
		_rot[I_YAW] = ac->Yaw();
		
		_vel = EP_VECTOR(ac->XDelta(),ac->YDelta(),ac->ZDelta());

		_aVel[I_ROLL] = ac->RollDelta();
		_aVel[I_PITCH] = ac->PitchDelta();
		_aVel[I_YAW] = ac->YawDelta();
	}
	else
	{

		_pos = EP_VECTOR(XPos(),YPos(),ZPos());

		_rot[I_ROLL] =	Roll();
		_rot[I_PITCH] = Pitch();
		_rot[I_YAW] = Yaw();
		
		_vel = EP_VECTOR(XDelta(),YDelta(),ZDelta());

		_aVel[I_ROLL] = RollDelta();
		_aVel[I_PITCH] = PitchDelta();
		_aVel[I_YAW] = YawDelta();
	}

	// Play with this value to change the signature of an
	// ejected pilot on the IR.
	SetPowerOutput(0);
	// sfr: not setters on this anymore
	//SetVt(0);
	//SetKias(0);

	// Initialize physical data.
	_pd = NULL;
	_stage = PD_START;

	// Initialize model data to NULL.
	_md = NULL;
	_model = MD_START;

	// Set the ejection mode.
	SetMode(mode);

	// Initialize run time and delta time.
	_runTime = 0.0;
	_deltaTime = 0.0;

	// We just set the type flag to "FalconSimEntity".
	SetTypeFlag(FalconEntity::FalconSimEntity);

	// Is it ourselves - Find out from the aircraft.
	if (ac && no == 0){
		_isPlayer = (SimDriver.GetPlayerEntity() == ac) ? TRUE : FALSE;
	}
	else {
		_isPlayer = FALSE;
	}

	// Is it a player - Find out from the aircraft.
	if (ac){
		_isDigital = ac->IsDigital() ? TRUE : FALSE;
	}
	else{
		_isDigital = TRUE;
	}

	// Set team/country
	if (ac){
      SetCountry (ac->GetCountry());
	}

	_endStageTimeAdjust = 
	(
		IsDigiPilot() ?
		0.0F :
		_pd->humanPilotEndStageTimeAdjust
	);
	
	// It hasn't hit the ground yet.
	_hitGround = FALSE;

	// The chute isn't collapsed yet.
	_collapseChute = FALSE;
	_chuteCollapsedTime = 1000000.0;

	// No death message yet.
	_deathMsg = NULL;

	// Update shared data.
	SetPosition(_pos[I_X], _pos[I_Y], _pos[I_Z]);
	SetDelta(_vel[I_X], _vel[I_Y], _vel[I_Z]);
	SetYPR(_rot[I_YAW], _rot[I_PITCH], _rot[I_ROLL]);
	SetYPRDelta(_aVel[I_YAW], _aVel[I_PITCH], _aVel[I_ROLL]);

	// Update matrices for geometry.
	CalcTransformMatrix((SimMoverClass *)this);

	// Set up our label.
	if (ac)
	{
		acBSP = (DrawableBSP *)ac->drawPointer;
		if(acBSP != NULL)
		{
			strncpy(_label, acBSP->Label(), 32);
			labelLen = strlen(acBSP->Label());
			if (no == 0){
			    strncat(_label, " Pilot", 32 - labelLen);
			}
			else {
			    char crewstr[20];
			    sprintf (crewstr, " Crew%d", no);
			    strncat(_label, crewstr, 32 - labelLen);
			}
			_label[31] = 0;
			_labelColor = acBSP->LabelColor();
		}
		else
		{
			_label[0] = 0;
			_labelColor = 0;
		}
	}
	else
	{
		strcpy(_label, "Pilot");
		labelLen = strlen(_label);
		_labelColor = 0;//acBSP->LabelColor();
	}
	
	_execCalledFromAircraft = FALSE;
	
	// Point to the aircraft that I ejected from.
	if (ac)
	{
		_aircraftId = ac->Id();
		_flightId = ac->GetCampaignObject()->Id();
	}

	// Update exec transfer synching data.
	_lastFrameCount = 0;
	//	_execCount = 0;

   // Act like a bomb, so nobody sees you
   // edg: yuck, we now have an eject pilot motion
   SetFlag(MOTION_BMB_AI);

   SetFlag(MOTION_EJECT_PILOT);

   if (IsLocal()) {
      SimVuDriver *drive = new SimVuDriver(this);
      drive->ExecDR(SimLibElapsedTime);
      SetDriver (drive);
   }
}

void EjectedPilotClass::CleanupData(){
	CleanupLocalData();
	SimMoverClass::CleanupData();
}

void EjectedPilotClass::CleanupLocalData(){
	_pd = NULL;
	_md = NULL;

	// Delete the death message if it still exists.
	if(_deathMsg != NULL)
	{
		delete _deathMsg;
		_deathMsg = NULL;
	}
}


void EjectedPilotClass::Init(SimInitDataClass *){
	SimMoverClass::Init(NULL);
}

int EjectedPilotClass::Wake (void)
{
	int retval = 0;

	if (IsAwake())
		return retval;

	return SimMoverClass::Wake();
}

int EjectedPilotClass::Sleep (void)
{
int retval = 0;

	if (!IsAwake())
		return retval;

	return SimMoverClass::Sleep();
}

int EjectedPilotClass::Exec()
{
   	ACMIGenPositionRecord genPos;
	ACMISwitchRecord acmiSwitch;

	SoundPos.UpdatePos(this);

	if (IsDead())
		return TRUE;
	
		// Call superclass Exec.
		SimMoverClass::Exec();

      if (!SimDriver.MotionOn())
         return IsLocal();

      if (_delayTime > SimLibElapsedTime) { // not time yet
		RunJettisonCanopy(); // stay with it
		return IsLocal();
      }

		// Advance time
		AdvanceTime();

		// Simulate the ejected pilot here.
		switch(_stage)
		{
			case PD_JETTISON_CANOPY :
			{
				RunJettisonCanopy();
				
				break;
			}
			case PD_EJECT_SEAT :
			{
				RunEjectSeat();

				break;
			}
			case PD_FREE_FALL_WITH_SEAT :
			{
				RunFreeFall();

				break;
			}
			case PD_CHUTE_OPENING :
			{
				RunFreeFall();

				// Here we run our little switch based animation...
				static const int NUM_FRAMES = 31;

				float percent = (_runTime             - StageEndTime(_stage-1)) /
					            (StageEndTime(_stage) - StageEndTime(_stage-1));
				int	frame = FloatToInt32(percent * (NUM_FRAMES-0.5f));

				if ( frame < 0 )
					frame = 0;
				else if ( frame > NUM_FRAMES )
					frame = NUM_FRAMES;

				percent = ((_runTime  - _deltaTime )           - StageEndTime(_stage-1)) /
					       (StageEndTime(_stage) - StageEndTime(_stage-1));

				int	prevframe = FloatToInt32(percent * ((float)NUM_FRAMES-0.5f));
				if ( prevframe < 0 )
					prevframe = 0;
				else if ( prevframe > NUM_FRAMES )
					prevframe = NUM_FRAMES;

				if ( gACMIRec.IsRecording() && prevframe != frame)
				{
						acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
						acmiSwitch.data.type = Type();
						acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(),NULL,0);//.num_;
						acmiSwitch.data.switchNum = 0;
						acmiSwitch.data.prevSwitchVal = 1<<prevframe;
						acmiSwitch.data.switchVal = 1<<frame;
						gACMIRec.SwitchRecord( &acmiSwitch );
				}
					
				if ( drawPointer )
					((DrawableBSP*)drawPointer)->SetSwitchMask( 0, 1<<frame );

				break;
			}
			case PD_FREE_FALL_WITH_OPEN_CHUTE :
			{
				RunFreeFallWithOpenChute();
	
				break;
			}
			case PD_FREE_FALL_WITH_COLLAPSED_CHUTE :
			{
				RunFreeFall();
	
				break;
			}
			case PD_SAFE_LANDING :
			{
				RunSafeLanding();

				_stageTimer += _deltaTime;

				static const int NUM_FRAMES = 13;
				float percent = _stageTimer/2.0f;
				int	frame = FloatToInt32(percent * ((float)NUM_FRAMES-0.5f));

				if ( frame < 0 )
					frame = 0;
				else if ( frame > NUM_FRAMES )
					frame = NUM_FRAMES;

				if ( drawPointer )
					((DrawableBSP*)drawPointer)->SetSwitchMask( 0, 1<<frame );
	
				break;
			}
			case PD_CRASH_LANDING :
			{
				RunCrashLanding();

				_stageTimer += _deltaTime;

				static const int NUM_FRAMES = 12;
				float percent = _stageTimer/2.0f;
				int	frame = FloatToInt32(percent * ((float)NUM_FRAMES-0.5f));

				if ( frame < 0 )
					frame = 0;
				else if ( frame > NUM_FRAMES )
					frame = NUM_FRAMES;

				if ( drawPointer )
					((DrawableBSP*)drawPointer)->SetSwitchMask( 0, 1<<frame );
	
				break;
			}
			default :
			{
				ShiWarning ("Bad Eject Mode");
			}
		}

		// Make sure all components of orientation are in range ( 0 <= n <= TWO_PI).
		FixOrientationRange();

		// Update shared data.
		SetPosition(_pos[I_X], _pos[I_Y], _pos[I_Z]);
		SetDelta(_vel[I_X], _vel[I_Y], _vel[I_Z]);
		SetYPR(_rot[I_YAW], _rot[I_PITCH], _rot[I_ROLL]);
		SetYPRDelta(_aVel[I_YAW], _aVel[I_PITCH], _aVel[I_ROLL]);
		if (gACMIRec.IsRecording() && (SimLibFrameCount & 3 ) == 0)
		{
			genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			genPos.data.type = Type();
			genPos.data.uniqueID = ACMIIDTable->Add(Id(),NULL,TeamInfo[GetTeam()]->GetColor());//.num_;
			genPos.data.x = XPos();
			genPos.data.y = YPos();
			genPos.data.z = ZPos();
			genPos.data.roll = Roll();
			genPos.data.pitch = Pitch();
			genPos.data.yaw = Yaw();
// remove			genPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();
			gACMIRec.GenPositionRecord( &genPos );
		}

		// Update matrices for geometry.
		CalcTransformMatrix((SimMoverClass *)this);

		// See if it hit the ground.
		if ( _hitGround == FALSE )
			_hitGround = HasHitGround();

		/*
		** We now do completion in the safe or crash landing stages
		** (by calling HitGround() )
		if (HasHitGround())
		{
			HitGround();
		}
		*/

		// Display some debug data.
#if DEBUG_EJECTION_SEQUENCE
		SpewDebugData();
#endif // DEBUG_EJECTION_SEQUENCE

	return IsLocal();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::ApplyDamage(FalconDamageMessage *damageMsg)
{
	if(_stage >= PD_CHUTE_OPENING)
	{
	   SimMoverClass::ApplyDamage(damageMsg);

		if(_collapseChute == FALSE)
		{
			// PlayRadioMessage (rcAIRMANDOWNB)
			// _flightId is the VU_ID of the flight the pilot ejected from
			
			Flight flight;
			flight = (Flight) vuDatabase->Find(_flightId);

			if(flight)
			{
				FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage( flight->Id(), FalconLocalSession );
				radioMessage->dataBlock.from = flight->Id();
				radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
				radioMessage->dataBlock.voice_id = flight->GetFlightLeadVoiceID();
				radioMessage->dataBlock.message = rcAIRMANDOWNB;
				//M.N. changed to 32767 -> flexibly use randomized values of max available eval indexes
				radioMessage->dataBlock.edata[0] = 32767;
				
				FalconSendMessage(radioMessage, FALSE);
			}

			F4Assert(_deathMsg == NULL);

			// Create death message. 
			_deathMsg = new FalconDeathMessage (Id(), FalconLocalGame);

			// ahhhhhhhhhhhhhhhhh
#ifdef MLR_NEWSNDCODE			
			SoundPos.Sfx( SFX_SCREAM, 0, 1, 0);
#else
			F4SoundFXSetPos( SFX_SCREAM, TRUE, XPos(), YPos(), ZPos(), 1.0f , 0 , XDelta(),YDelta(),ZDelta());
#endif
      
			_deathMsg->dataBlock.damageType = damageMsg->dataBlock.damageType;
			_deathMsg->dataBlock.dEntityID  = Id();
			_deathMsg->dataBlock.dCampID = 0;
			_deathMsg->dataBlock.dSide   = GetCountry();
			_deathMsg->dataBlock.dPilotID   = pilotSlot;
			_deathMsg->dataBlock.dIndex     = Type();

			_deathMsg->dataBlock.fEntityID  = damageMsg->dataBlock.fEntityID;
			_deathMsg->dataBlock.fCampID    = damageMsg->dataBlock.fCampID;
			_deathMsg->dataBlock.fSide      = damageMsg->dataBlock.fSide;
			_deathMsg->dataBlock.fPilotID   = damageMsg->dataBlock.fPilotID;
			_deathMsg->dataBlock.fIndex     = damageMsg->dataBlock.fIndex;
			_deathMsg->dataBlock.fWeaponID  = damageMsg->dataBlock.fWeaponID;
			_deathMsg->dataBlock.fWeaponUID = damageMsg->dataBlock.fWeaponUID;
		}

		_collapseChute = TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::SetDead(int flag)
{
	if (flag){
		if (IsPlayer()){
		//End the sim
			OTWDriver.EndFlight();
		}
	}
	SimMoverClass::SetDead(flag);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BOOL EjectedPilotClass::HasHitGround() const
{
	float testHeight;

	// We could linearly extrapolate based on the current frame time
	// and velocity to keep the pilot from going underground.

   testHeight = _pos[I_Z];

   if (_stage > PD_CHUTE_OPENING)
   {
	  if (drawPointer)
	  {
		testHeight += drawPointer->Radius() * 0.5f;
	  }
	  else
	  {
		  testHeight = 100.0f;
	  }
   }
   
	// TODO: do a cheaper ground height check 1st
	if (testHeight  >= OTWDriver.GetGroundLevel(_pos[I_X], _pos[I_Y])) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void EjectedPilotClass::HitGround()
{
	if(drawPointer){
		OTWDriver.RemoveObject(drawPointer, TRUE);
		drawPointer = NULL;
	}

	if (_stage != PD_SAFE_LANDING ){
		// No strength left.
		strength = 0.0;
		pctStrength = 0.0;

		// Send the death message
		if(_deathMsg != NULL)
		{
			FalconSendMessage (_deathMsg,FALSE);
			_deathMsg = NULL;
		}

		// Dead.  No countdown.  Just dead.
		dyingTimer = 0.0f;
	}

	// Right now, handle the same way whether dead or alive, just destroy
	// the object.
	SetDead(TRUE);
    SetExploding(TRUE);
}

void EjectedPilotClass::GetTransform(TransformMatrix tMat)
{
	memcpy (tMat, dmx, sizeof (TransformMatrix));
}

void EjectedPilotClass::GetFocusPoint(BIG_SCALAR &x, BIG_SCALAR &y, BIG_SCALAR &z){
	SIM_FLOAT startTime, totalTime;

	EP_VECTOR deltaFocusPoint, finalFocus, pos;

	Trotation rot; 

	Tpoint modelSpaceOffset, worldSpaceOffset;

	F4Assert(_model != MD_START);

	/*
	x = XPos() + _md[_model].focusOffset[I_X];
	y = YPos() + _md[_model].focusOffset[I_Y];
	z = ZPos() + _md[_model].focusOffset[I_Z];

	return;
	switch(_stage)
	{
		case PD_FREE_FALL_WITH_OPEN_CHUTE :
		case PD_FREE_FALL_WITH_COLLAPSED_CHUTE :
		case PD_SAFE_LANDING :
		case PD_CRASH_LANDING :
			x = XPos() + _md[_model].focusOffset[I_X];
			y = YPos() + _md[_model].focusOffset[I_Y];
			z = ZPos() + _md[_model].focusOffset[I_Z];
			break;
		case PD_JETTISON_CANOPY :
		case PD_EJECT_SEAT :
		case PD_FREE_FALL_WITH_SEAT :
		case PD_CHUTE_OPENING :
		default :
			x = XPos();
			y = YPos();
			z = ZPos();
			break;
	}

	return;
	*/

	// Get the model space offset here.  We should linearly interpolate
	// over time so that the focus point doesnt jump around.
	startTime = ModelCreateTime(_model);
	totalTime =
	(
		_model >= MD_PILOT_AND_OPEN_CHUTE ?
		5.0F :
		ModelCreateTime(_model + 1) - startTime
	);
	F4Assert(totalTime > 0.0F);
	finalFocus = 
	(
		_model >= MD_PILOT_AND_OPEN_CHUTE ?
		_md[_model].focusOffset :
		_md[_model + 1].focusOffset
	);
	if(_runTime > startTime + totalTime)
	{
		_focusPoint = finalFocus;			
	}
	else if(_runTime > startTime)
	{
//XX		deltaFocusPoint = finalFocus - _focusPoint;
		deltaFocusPoint = finalFocus;
		deltaFocusPoint -= _focusPoint;

		deltaFocusPoint *= (_runTime - startTime) / totalTime;
		_focusPoint += deltaFocusPoint;
	}
	_focusPoint.GetTpoint(modelSpaceOffset);

	// Transform the model space offset into a world space offset.
	_rot.GetTrotation(rot);
	MatrixMult
	(
		&rot,
		&modelSpaceOffset,
		&worldSpaceOffset
	);

	// Find the focus point in world space by adding the world space
	// offset to the position
	pos = EP_VECTOR(XPos(), YPos(), ZPos());
	pos += EP_VECTOR(worldSpaceOffset);

	// return our result
	x = (BIG_SCALAR)pos[I_X];
	y = (BIG_SCALAR)pos[I_Y];
	z = (BIG_SCALAR)pos[I_Z];
}

void EjectedPilotClass::SetMode(int mode)
{
	// Point us to the correct physical data.
	switch(mode){
		case EM_F16_MODE1: {
			_pd = &F16Mode1PhysicalData;
			_md = F16ModelData;

			break;
		}
		default: {
			ShiWarning("Bad Eject Mode");
		}
	}
}

int EjectedPilotClass::ClassType()
{
	if(_classTypeFound == FALSE)
	{
		_classType = GetClassID 
		(
			DOMAIN_AIR,
			CLASS_VEHICLE,
			TYPE_EJECT,
			STYPE_EJECT1,
			SPTYPE_ANY,
			VU_ANY,
			VU_ANY,
			VU_ANY
		) + VU_LAST_ENTITY_TYPE;
		_classTypeFound = TRUE;
	}

	return _classType;
}

AircraftClass* EjectedPilotClass::GetParentAircraft (void)
{
	return (AircraftClass*) vuDatabase->Find(_aircraftId); 
}

void EjectedPilotClass::AdvanceTime()
{
	// Get delta time.
	_deltaTime = 
	(
		_stage == PD_START ?
		0.0F :
		SimLibMajorFrameTime
	);

	// Update position.
//XX	_pos += _vel * _deltaTime;
		EP_VECTOR p;
		p = _vel;
		p *= _deltaTime;
		_pos += p;
	
	// Update orientation.
   // Set roll velocity to -roll angle, tilt back towards the sun
   // Set pitch velocity to -pitch angle, tilt back towards the sun
   _aVel[I_PITCH] = -_rot[I_PITCH]*1.5F;
   _aVel[I_ROLL] = -_rot[I_ROLL]*1.5F;
//XX	_rot += _aVel * _deltaTime;
   p = _aVel;
   p *= _deltaTime;
   _rot += p;
//MonoPrint ("Pitch %.2f rate %.2f\n", _rot[I_PITCH]*RTD, _aVel[I_PITCH]*RTD);
	
	// Increment total run time.
	_runTime += _deltaTime;
	
	// Advance stage if necessary.
	if(!_hitGround && _collapseChute && _stage != PD_FREE_FALL_WITH_COLLAPSED_CHUTE)
	{
		SetStage(PD_FREE_FALL_WITH_COLLAPSED_CHUTE);
		InitFreeFallWithCollapsedChute();
	}
	// advance when ground has been hit
	else if ( _hitGround )
	{
		if ( _stage < PD_SAFE_LANDING )
		{
			// only open chute results in safe landing
			if ( _stage == PD_FREE_FALL_WITH_OPEN_CHUTE )
			{
				SetStage(PD_SAFE_LANDING);
				InitSafeLanding();
			}
			else
			{
				SetStage(PD_CRASH_LANDING);
				InitCrashLanding();
			}
		}
	}
	else
	{
		while(_stage < PD_FREE_FALL_WITH_OPEN_CHUTE && _runTime >= StageEndTime())
		{
			switch(AdvanceStage())
			{
				case PD_JETTISON_CANOPY :
				{
					InitJettisonCanopy();
	
					break;
				}
				case PD_EJECT_SEAT :
				{
					InitEjectSeat();
	
					break;
				}
				case PD_FREE_FALL_WITH_SEAT :
				{
					InitFreeFallWithSeat();
	
					break;
				}
				case PD_CHUTE_OPENING :
				{
					InitChuteOpening();
	
					break;
				}
				case PD_FREE_FALL_WITH_OPEN_CHUTE :
				{
					InitFreeFallWithOpenChute();
	
					break;
				}
				default :
				{
					ShiWarning ("Bad Eject Mode");
				}
			}
		}

#if DEBUG_EJECTION_SEQUENCE
		SpewDebugData();
#endif // DEBUG_EJECTION_SEQUENCE
	}
}

void EjectedPilotClass::SetModel(int model){
	Trotation rot;
	Tpoint pos;
	// Destroy the current bsp.
	if(drawPointer)
	{
		drawPointer->GetPosition (&pos);
		OTWDriver.RemoveObject(drawPointer, TRUE);
		drawPointer = NULL;
	}
	else
	{
		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos();
	}

	// Set our new model.
	_model = model;
	
	// Create the new bsp.
	_rot.GetTrotation(rot);
	if ( _md[_model].bsp == VIS_GUYDIE || _md[_model].bsp == VIS_DOWN_PILOT )
	{
      if (pos.z > 0.0F)
         pos.z = 0.0F;
		drawPointer = new DrawableGuys(_md[_model].bsp, &pos, Yaw(), 1, 1.0 );
		((DrawableBSP *)drawPointer)->SetSwitchMask( 0, 1 );
		((DrawableBSP *)drawPointer)->SetSwitchMask( 1, 1 );
	}
	/*
	** edg: better to show nothing than the crappy oval shadow
	else if ( _md[_model].bsp == VIS_EJECT1  )
	{
		drawPointer = new DrawableShadowed(_md[_model].bsp, &pos, &rot, 1.0, VIS_PCHUTESH);
	}
	*/
	else 
	{
		drawPointer = new DrawableBSP(_md[_model].bsp, &pos, &rot, 1.0 );
	}
	F4Assert(drawPointer != NULL);

	// Set the label.
	if(drawPointer && strlen(_label) > 0)
	{
		drawPointer->SetLabel(_label, _labelColor);
		OTWDriver.InsertObject(drawPointer);
	}
}

void EjectedPilotClass::InitJettisonCanopy(){
	AircraftClass	*aircraft = (AircraftClass*) vuDatabase->Find(_aircraftId);

	_stageTimer = 0.0f;

	if (aircraft)
	{
		FalconSessionEntity *session = (FalconSessionEntity*) vuDatabase->Find(OwnerId());

		// Turn off the canopy.
		aircraft->SetSwitch(SIMP_CANOPY, FALSE);

		// Find and set the position of the seat in the cockpit.
		CalculateAndSetPositionAndOrientationInCockpit();

		// Create the drawable bsp here.
		// SCR 5/27/98  At this point a drawable was already created based on class table data in 
		// the OTWDriver::CreateVisualObject() function.  It might be okay to just use that if it
		// is always correcct, but for now I'll go ahead and let the existing draw pointer get 
		// deleted and a new one created in its place in the SetModel() call...
		SetModel(MD_PILOT_AND_SEAT);

		if(IsPlayerPilot() && session == FalconLocalSession)
		{
			// The sim driver now needs to know that the ejected pilot is the player entity.
			// KCK: We unfortunately need to do this outside of our exec loop, since it will
			// destroy our driver. So send a message which we'll handle later in the sim cycle.
			GameManager.AnnounceTransfer(aircraft,this);
		}
	}
}

void EjectedPilotClass::InitEjectSeat()
{
	EP_VECTOR
		ejectVec;

	// Find and set the position of the seat in the cockpit.
	CalculateAndSetPositionAndOrientationInCockpit();

	// Calculate the ejection vector
	CalculateEjectionVector(ejectVec);
	
	// Apply initial velocity of n ft/sec, at eject angle.
	// Add the velocity of the plane.
//XX	_vel += ejectVec * EjectSpeed();
	EP_VECTOR p;
	p = ejectVec;
	p *= EjectSpeed();
	_vel += p;

   F4Assert (_vel[I_X] <  10000.0F);
   F4Assert (_vel[I_X] > -10000.0F);
   F4Assert (_vel[I_Y] <  10000.0F);
   F4Assert (_vel[I_Y] > -10000.0F);
   F4Assert (_vel[I_Z] <  10000.0F);
   F4Assert (_vel[I_Z] > -10000.0F);
	
	// No angular velocity.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = 0.0;
	_aVel[I_YAW] = 0.0;
	_stageTimer = 0.0f;

	if(IsPlayerPilot())
	{
		OTWDriver.StartEjectCam(this);
	}
}

void EjectedPilotClass::InitFreeFallWithSeat()
{
   // PlayRadioMessage (rcAIRMANDOWNA)
   // PlayRadioMessage (rcAIRMANDOWNE)
   // PlayRadioMessage (rcAIRMANDOWNF)
   // _aircraft is pointer to the plane the pilot came from.
   // check _aircraft->flightPtr for someone in the flight to see the chute
   // Randomize these three (i think)
	Flight flight;
	flight = (Flight) vuDatabase->Find(_flightId);

	if(flight)
	{
		FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage( flight->Id(), FalconLocalGame );
		radioMessage->dataBlock.from = flight->Id();
		radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
		radioMessage->dataBlock.voice_id = flight->GetFlightLeadVoiceID();
		if(rand() %2)
		{
			radioMessage->dataBlock.message = rcAIRMANDOWNE;
			radioMessage->dataBlock.edata[0] = SimToGrid(YPos());// MN Fix - need SimToGrid, not FloatToInt32 as previoiusly..
			radioMessage->dataBlock.edata[1] = SimToGrid(XPos());
		}
		else
		{
			radioMessage->dataBlock.message = rcAIRMANDOWNA;
		}
		
		FalconSendMessage(radioMessage, FALSE);
	}

	// Pitch and roll should be zero.
	ZeroPitchAndRoll();
		
	// Set angular velocity to 0, except for yaw.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = 0.0;
	_aVel[I_YAW] = PI/6.0;
	_stageTimer = 0.0f;
}

void EjectedPilotClass::InitChuteOpening()
{
	// Set the new BSP.
	SetModel(MD_PILOT_AND_OPEN_CHUTE);
	
	// Set the orientation to upright
	_rot[I_ROLL] = 0.0;
	_rot[I_PITCH] = 0.0;
	_rot[I_YAW] = _vel.Heading();

	// Set angular velocity to 0.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = 0.0;
	_aVel[I_YAW] = 0.0;

	// play the sound
#ifdef MLR_NEWSNDCODE	
	SoundPos.Sfx( SFX_CHUTE, 0, 1.0f , 0 );
#else
	F4SoundFXSetPos( SFX_CHUTE, TRUE, XPos(), YPos(), ZPos(), 1.0f , 0 , XDelta(),YDelta(),ZDelta());
#endif

	_stageTimer = 0.0f;
}

void EjectedPilotClass::InitFreeFallWithOpenChute()
{
	// Give the pilot a slight pitch.
	ZeroPitchAndRoll();
	_rot[I_PITCH] = StartPitch();
	
	// Set angular velocity to 0, except for yaw.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = PitchDecay();
	_aVel[I_YAW] = YawSpeed();
	_stageTimer = 0.0f;
}


void EjectedPilotClass::InitSafeLanding()
{
	F4Assert(_md != NULL);

   // PlayRadioMessage (rcAIRMANDOWND)
   // _aircraft is pointer to the plane the pilot came from.
   // check _aircraft->flightPtr for someone in the flight to see the landing

	Flight flight;
	flight = (Flight) vuDatabase->Find(_flightId);

	if(flight)
	{
		FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage( flight->Id(), FalconLocalGame );
		radioMessage->dataBlock.from = flight->Id();
		radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
		radioMessage->dataBlock.voice_id = flight->GetFlightLeadVoiceID();
		radioMessage->dataBlock.message = rcAIRMANDOWND;
		radioMessage->dataBlock.edata[0] = flight->callsign_id;
		radioMessage->dataBlock.edata[1] = flight->GetFlightLeadCallNumber();
		radioMessage->dataBlock.edata[2] = SimToGrid(YPos()); // MN Fix - need SimToGrid, not FloatToInt32 as previoiusly..and reversed X/YPos
		radioMessage->dataBlock.edata[3] = SimToGrid(XPos());
		FalconSendMessage(radioMessage, FALSE);
	}

   // Set the new BSP.
	SetModel(MD_SAFE_LANDING);
		
	// Zero the pitch and roll.
	ZeroPitchAndRoll();
		
	// Set angular velocity to 0.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = 0.0;
	_aVel[I_YAW] = 0.0;

	// Set translational velocity to 0.
	_vel[I_X] = 0.0;
	_vel[I_Y] = 0.0;
	_vel[I_Z] = 0.0;

	// for new set Z on ground
	_pos[I_Z] = OTWDriver.GetGroundLevel(_pos[I_X], _pos[I_Y]) - 3.0f;

	// Set the time that we hit ground
	_hitGroundTime = _runTime;

	_stageTimer = 0.0f;
}

void EjectedPilotClass::InitCrashLanding()
{
	F4Assert(_md != NULL);

	// Set the new BSP.
	SetModel(MD_CRASH_LANDING);
		
	// Zero the pitch and roll.
	ZeroPitchAndRoll();
		
	// Set angular velocity to 0.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = 0.0;
	_aVel[I_YAW] = 0.0;

	// Set translational velocity to 0.
	_vel[I_X] = 0.0;
	_vel[I_Y] = 0.0;
	_vel[I_Z] = 0.0;

	// for new set Z on ground
	_pos[I_Z] = OTWDriver.GetGroundLevel(_pos[I_X], _pos[I_Y]) - 3.0f;

	// Set the time that we hit ground
	_hitGroundTime = _runTime;

	_stageTimer = 0.0f;
}

void EjectedPilotClass::InitFreeFallWithCollapsedChute(){
	F4Assert(_md != NULL);

	// Set the new BSP.
	SetModel(MD_PILOT_AND_COLLAPSED_CHUTE);
		
	// Zero the pitch and roll.
	ZeroPitchAndRoll();
		
	// Set angular velocity to 0.
	_aVel[I_ROLL] = 0.0;
	_aVel[I_PITCH] = 0.0;
	_aVel[I_YAW] = 0.0;

	// Set the time that the chute collapsed.
	_chuteCollapsedTime = _runTime;
	_stageTimer = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::RunJettisonCanopy()
{
	// Find and set the position of the seat in the cockpit.
	CalculateAndSetPositionAndOrientationInCockpit();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::RunEjectSeat()
{
	EP_VECTOR
		thrustVector,
		dragVector,
		accelVector;

	// Calculate all forces acting on the seat.
	CalculateGravityVector(accelVector);
	CalculateThrustVector(thrustVector);
	CalculateDragVector(dragVector);

	// Calculate the resultant acceleration on the seat.
	F4Assert(Mass() != 0.0);
	accelVector += thrustVector;
   dragVector *= _deltaTime/Mass();
	accelVector *= _deltaTime/Mass();

   // Don't cross the zero line
   if (dragVector[I_X] < - _vel[I_X])
      dragVector[I_X] = - _vel[I_X];
   else if (dragVector[I_X] > _vel[I_X])
      dragVector[I_X] = _vel[I_X];

   if (dragVector[I_Y] < - _vel[I_Y])
      dragVector[I_Y] = - _vel[I_Y];
   else if (dragVector[I_Y] > _vel[I_Y])
      dragVector[I_Y] = _vel[I_Y];

   if (dragVector[I_Z] < - _vel[I_Z])
      dragVector[I_Z] = - _vel[I_Z];
   else if (dragVector[I_Z] > _vel[I_Z])
      dragVector[I_Z] = _vel[I_Z];

	accelVector += dragVector;

	// Ajust the velocity by the acceleration.
	_vel += accelVector;
   F4Assert (_vel[I_X] <  10000.0F);
   F4Assert (_vel[I_X] > -10000.0F);
   F4Assert (_vel[I_Y] <  10000.0F);
   F4Assert (_vel[I_Y] > -10000.0F);
   F4Assert (_vel[I_Z] <  10000.0F);
   F4Assert (_vel[I_Z] > -10000.0F);

}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::RunSafeLanding()
{
	// all we do (thus far) is just time how long we stay around
	// after hitting the ground ( 5 secs )
	// once HitGround() is called we're finished
	if ( _runTime - _hitGroundTime > 10.0f )
		HitGround();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::RunCrashLanding()
{
	// all we do (thus far) is just time how long we stay around
	// after hitting the ground ( 5 secs )
	// once HitGround() is called we're finished
	if ( _runTime - _hitGroundTime > 10.0f )
		HitGround();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::RunFreeFallWithOpenChute()
{
	EP_VECTOR
		dragVector,
		accelVector;
	
	// Calculate all forces acting on the seat.
	CalculateGravityVector(accelVector);
	CalculateDragVector(dragVector);

	// Calculate the resultant acceleration on the seat.
	F4Assert(Mass() != 0.0);
   dragVector *= _deltaTime/Mass();
	accelVector *= _deltaTime/Mass();

   // Don't cross the zero line
   if (dragVector[I_X] < - _vel[I_X])
      dragVector[I_X] = - _vel[I_X];
   else if (dragVector[I_X] > _vel[I_X])
      dragVector[I_X] = _vel[I_X];

   if (dragVector[I_Y] < - _vel[I_Y])
      dragVector[I_Y] = - _vel[I_Y];
   else if (dragVector[I_Y] > _vel[I_Y])
      dragVector[I_Y] = _vel[I_Y];

   if (dragVector[I_Z] < - _vel[I_Z])
      dragVector[I_Z] = - _vel[I_Z];
   else if (dragVector[I_Z] > _vel[I_Z])
      dragVector[I_Z] = _vel[I_Z];

	accelVector += dragVector;
	
	// Ajust the velocity by the acceleration.
	_vel += accelVector;
   F4Assert (_vel[I_X] <  10000.0F);
   F4Assert (_vel[I_X] > -10000.0F);
   F4Assert (_vel[I_Y] <  10000.0F);
   F4Assert (_vel[I_Y] > -10000.0F);
   F4Assert (_vel[I_Z] <  10000.0F);
   F4Assert (_vel[I_Z] > -10000.0F);


	if(_rot[I_PITCH] > 0.0)
	{
		_rot[I_PITCH] = 0.0;
		_aVel[I_PITCH] = 0.0;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::RunFreeFall()
{
	EP_VECTOR
		dragVector,
		accelVector;
	
	// Calculate all forces acting on the seat.
	CalculateGravityVector(accelVector);
	CalculateDragVector(dragVector);

	// Calculate the resultant acceleration on the seat.
	F4Assert(Mass() != 0.0);
   dragVector *= _deltaTime/Mass();
	accelVector *= _deltaTime/Mass();

   // Don't cross the zero line
   if (dragVector[I_X] < - _vel[I_X])
      dragVector[I_X] = - _vel[I_X];
   else if (dragVector[I_X] > _vel[I_X])
      dragVector[I_X] = _vel[I_X];

   if (dragVector[I_Y] < - _vel[I_Y])
      dragVector[I_Y] = - _vel[I_Y];
   else if (dragVector[I_Y] > _vel[I_Y])
      dragVector[I_Y] = _vel[I_Y];

   if (dragVector[I_Z] < - _vel[I_Z])
      dragVector[I_Z] = - _vel[I_Z];
   else if (dragVector[I_Z] > _vel[I_Z])
      dragVector[I_Z] = _vel[I_Z];

	accelVector += dragVector;
	
	// Ajust the velocity by the acceleration.
	_vel += accelVector;
   F4Assert (_vel[I_X] <  10000.0F);
   F4Assert (_vel[I_X] > -10000.0F);
   F4Assert (_vel[I_Y] <  10000.0F);
   F4Assert (_vel[I_Y] > -10000.0F);
   F4Assert (_vel[I_Z] <  10000.0F);
   F4Assert (_vel[I_Z] > -10000.0F);

   if (_vel[I_X] > 10000.0F)
      _vel[I_X] = 10000.0F;
   else if (_vel[I_X] < -10000.0F)
      _vel[I_X] = -10000.0F;

   if (_vel[I_Y] > 10000.0F)
      _vel[I_Y] = 10000.0F;
   else if (_vel[I_Y] < -10000.0F)
      _vel[I_Y] = -10000.0F;

   if (_vel[I_Z] > 10000.0F)
      _vel[I_Z] = 10000.0F;
   else if (_vel[I_Z] < -10000.0F)
      _vel[I_Z] = -10000.0F;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::CalculateAndSetPositionAndOrientationInCockpit()
{
	Trotation
		rot;

	Tpoint
		modelOffset,
		worldOffset;

	AircraftClass	*aircraft = (AircraftClass*) vuDatabase->Find(_aircraftId);

	if (!aircraft)
		return;

	// Orient the seat the same way as the plane.
	_rot[I_ROLL] = aircraft->Roll();
	_rot[I_PITCH] = aircraft->Pitch();
	_rot[I_YAW] = aircraft->Yaw();

	// Get the seat offset in model space.
	SeatOffset().GetTpoint(modelOffset);

	// Transform to world space.
	_rot.GetTrotation(rot);
	MatrixMult(&rot, &modelOffset, &worldOffset);

	// Get position of aircraft + model space offset.
	_pos = EP_VECTOR
	(
		aircraft->XPos(),
		aircraft->YPos(),
		aircraft->ZPos()
	);
	_pos += worldOffset;

	// Velocity is the velocity of the plane.
	_vel = EP_VECTOR
	(
		aircraft->XDelta(),
		aircraft->YDelta(),
		aircraft->ZDelta()
	);

	// Angular velocity is the same as the plane.
	_aVel[I_ROLL] = aircraft->RollDelta();
	_aVel[I_PITCH] = aircraft->PitchDelta();
	_aVel[I_YAW] = aircraft->YawDelta();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::CalculateThrustVector(EP_VECTOR &result) const
{
	Trotation rot;

	Tpoint modelThrust, worldThrust;

	// Thrust is up.
	modelThrust.x = 0.0;
	modelThrust.y = 0.0;
	modelThrust.z = -1.0;

	// Transform to world space.
	_rot.GetTrotation(rot);
	MatrixMult(&rot, &modelThrust, &worldThrust);
	
	// Multiply it by the magnitude
	result = EP_VECTOR(worldThrust);
	result *= Mass() * SeatThrust();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::CalculateDragVector(EP_VECTOR &result) const
{
	SIM_FLOAT
		windHdg,
		relSpeed;

	EP_VECTOR
		windVelocity;
	
	 Tpoint			pos;
		  pos.x = _pos[I_X];
		  pos.y = _pos[I_Y];
		  pos.z = _pos[I_Z];
		 

	// Find the velocity vector for the wind.
	//JAM 24Nov03
	windHdg = ((WeatherClass*)realWeather)->windHeading;
	
	windVelocity = EP_VECTOR((float)cos(windHdg), (float)sin(windHdg), 0);
	windVelocity *= ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);

	// Subtract the velocity of the wind from the velocity of the seat before computing drag.
	result = _vel;
	result -= windVelocity;

	// Get the relative speed.
	relSpeed = result.Magnitude();

	// Normalize.
	if(relSpeed != 0.0)
	{
		result /= relSpeed;
	}
	else
	{
		result = EP_VECTOR(0.0, 0.0, 0.0);
	}

	// Invert the relative velocity.  Drag opposes the velocity vector.
	result.Invert();

	// Multiply it by the squared relative speed and drag factor.
	result *= (relSpeed * relSpeed) * DragFactor() / GRAVITY;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::CalculateGravityVector(EP_VECTOR &result) const
{
	// This one's easy.
	result[I_X] = 0;
	result[I_Y] = 0;
	result[I_Z] = GRAVITY_ACCEL * Mass();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::CalculateEjectionVector(EP_VECTOR &result) const
{
	Trotation
		rot;

	Tpoint
		modelEject,
		worldEject;

	EP_VECTOR
		dir;

	float fudge = 30 * PRANDFloat() * DTR;
	dir = 
	EP_VECTOR
	(
		(float)cos(EjectAngle()+fudge),
		0,
		-(float)sin(EjectAngle()+fudge)
	);

	_rot.GetTrotation(rot);
	dir.GetTpoint(modelEject);
	MatrixMult(&rot, &modelEject, &worldEject);

	result = EP_VECTOR(worldEject);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::ZeroPitchAndRoll()
{
	Trotation
		rot;

	EP_VECTOR
		hdg;

	_rot.GetTrotation(rot);

	_rot[I_ROLL] = 0.0;
	_rot[I_PITCH] = 0.0;

	// Find heading and set yaw to current heading.
	if(rot.M21 == 0.0)
	{
		_rot[I_YAW] = 0.0;
	}
	else
	{
		hdg = EP_VECTOR
		(
			rot.M11,
			rot.M21,
			0
		);
		hdg.Normalize();

		_rot[I_YAW] = (float)atan2(hdg[I_Y], hdg[I_X]);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::FixOrientationRange()
{
	// This can be done with math instead of iteratively.
	while(_rot[I_YAW] > TWO_PI)
	{
		_rot[I_YAW] -= TWO_PI;
	}
	while(_rot[I_YAW] < -TWO_PI)
	{
		_rot[I_YAW] += TWO_PI;
	}
	while(_rot[I_PITCH] > TWO_PI)
	{
		_rot[I_PITCH] -= TWO_PI;
	}
	while(_rot[I_PITCH] < -TWO_PI)
	{
		_rot[I_PITCH] += TWO_PI;
	}
	while(_rot[I_ROLL] > TWO_PI)
	{
		_rot[I_ROLL] -= TWO_PI;
	}
	while(_rot[I_ROLL] < -TWO_PI)
	{
		_rot[I_ROLL] += TWO_PI;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void EjectedPilotClass::SpewDebugData()
{
	return;
/*
	unsigned char
		monoX,
		monoY;

	if(_isDigital)
	{
		return;
	}

	monoX = 0;
	monoY = 0;

	// Output miscellaneous data for aircraft.
	MonoLocate(monoX, monoY++);
	MonoPrint("Ejection Stage: ");
	switch(_stage)
	{
		case PD_JETTISON_CANOPY :
		{
			MonoPrint("Jettison Canopy                                           \n");

			break;
		}
		case PD_EJECT_SEAT :
		{
			MonoPrint("Eject Seat                                                \n");
	
			break;
		}
		case PD_FREE_FALL_WITH_SEAT :
		{
			MonoPrint("Free Fall 1                                               \n");
			
			break;
		}
		case PD_CHUTE_OPENING :
		{
			MonoPrint("Chute Opening                                             \n");
			
			break;
		}
		case PD_FREE_FALL_WITH_OPEN_CHUTE :
		{
			MonoPrint("Free Fall 2                                               \n");
			
			break;
		}
		default :
		{
			MonoPrint("Unknown Stage                                             \n");
		}
	}

	MonoLocate(monoX, monoY++);
	MonoPrint
	(
		"Run Time: %14.4f Delta Time : %14.4f                          \n",
		_runTime,
		_deltaTime
	);
	MonoLocate(monoX, monoY++);
	MonoPrint
	(
		"Mass    : %14.4f Drag Factor: %14.4f                          \n",
		Mass(),
		DragFactor()
	);
	
	// Output motion data for aircraft.
	if(_aircraft != NULL)
	{
		MonoLocate(monoX, monoY++);
		MonoPrint("Aircraft                                                    \n");
		MonoLocate(monoX, monoY++);
		MonoPrint
		(
			"    location        : x:%14.4f  y:%14.4f  z:%14.4f\n",
			_aircraft->XPos(),
			_aircraft->YPos(),
			_aircraft->ZPos()
		);
		MonoLocate(monoX, monoY++);
		MonoPrint
		(
			"    velocity        : x:%14.4f  y:%14.4f  z:%14.4f\n",
			_aircraft->XDelta(),
			_aircraft->YDelta(),
			_aircraft->ZDelta()
		);
		MonoLocate(monoX, monoY++);
		MonoPrint
		(
			"    angle           : y:%14.10f  p:%14.10f  r:%14.10f\n",
			_aircraft->Roll(),
			_aircraft->Pitch(),
			_aircraft->Yaw()
		);
		MonoLocate(monoX, monoY++);
		MonoPrint
		(
			"    angular velocity: y:%14.10f  p:%14.10f  r:%14.10f\n",
			_aircraft->RollDelta(),
			_aircraft->PitchDelta(),
			_aircraft->YawDelta()
		);
	}

	// Output motion data for pilot.
	MonoLocate(monoX, monoY++);
	MonoPrint("Pilot                                                       \n");
	MonoLocate(monoX, monoY++);
	MonoPrint
	(
		"    location        : x:%14.4f  y:%14.4f  z:%14.4f\n",
		XPos(),
		YPos(),
		ZPos()
	);
	MonoLocate(monoX, monoY++);
	MonoPrint
	(
		"    velocity        : x:%14.4f  y:%14.4f  z:%14.4f\n",
		XDelta(),
		YDelta(),
		ZDelta()
	);
	MonoLocate(monoX, monoY++);
	MonoPrint
	(
		"    angle           : y:%14.10f  p:%14.10f  r:%14.10f\n",
		Roll(),
		Pitch(),
		Yaw()
	);
	MonoLocate(monoX, monoY++);
	MonoPrint
	(
		"    angular velocity: y:%14.10f  p:%14.10f  r:%14.10f\n",
		RollDelta(),
		PitchDelta(),
		YawDelta()
	);
	MonoLocate(monoX, monoY);
*/
}

