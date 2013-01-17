#include "MsgInc/TrackMsg.h"
#include "Sim/Include/Sms.h"
#include "Sim/Include/Object.h"
#include "mesg.h"
#include "rwr.h"
#include "aircrft.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "flight.h"
#include "InvalidBufferException.h"

FalconTrackMessage::FalconTrackMessage(int reliable, VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (TrackMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	if (reliable == 1){
		RequestReliableTransmit ();
	}
	else if (reliable == 2){
		RequestOutOfBandTransmit ();
		RequestReliableTransmit ();
	}

		
}

FalconTrackMessage::FalconTrackMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (TrackMsg, FalconEvent::SimThread, senderid, target)
{
}

FalconTrackMessage::~FalconTrackMessage(void)
{
}

int FalconTrackMessage::Decode (VU_BYTE **buf, long *rem){
	long int init = *rem;
	FalconEvent::Decode (buf, rem);
	memcpychk(&dataBlock, buf, sizeof (DATA_BLOCK), rem);
	return init - *rem;
}


int FalconTrackMessage::Encode (VU_BYTE **buf)
{
	int size;
	size = FalconEvent::Encode (buf);
	memcpy (*buf, &dataBlock, sizeof (DATA_BLOCK));
	*buf += sizeof (DATA_BLOCK);
	size += sizeof (DATA_BLOCK);
	return size;
}


int FalconTrackMessage::Process(uchar autodisp)
{
	if (autodisp){
		return 0;
	}

	FalconEntity *theEntity = static_cast<FalconEntity*>(Entity());
	if (!theEntity){
		return 0;
	}
	FalconEntity *taggedEntity = static_cast<FalconEntity*>(vuDatabase->Find(dataBlock.id));

	// 2002-02-25 ADDED BY S.G. 'Entity' can be a campaign object if the missile being launch 
	// (ie, dataBlock.trackType is Launch) is a SARH and its platform is still aggregated since 
	// the platform becomes the launching object. Get in if it's a Mover class only...
	if (theEntity->IsMover()){
		SimMoverClass *mEntity = static_cast<SimMoverClass*>(theEntity);
		if (
			!mEntity->IsLocal () && 
			mEntity->numSensors > 1 && 
			// MN 2002-03-03 CTD
			mEntity->sensorArray[1] && 
			// JB 010604 CTD
			mEntity->IsAirplane()
		){
#if NO_REMOTE_BUGGED_TARGET
			// sfr: there should be a higher level function for doing this all...
			if (dataBlock.trackType == Track_Lock){
				SimObjectType *nt = new SimObjectType(taggedEntity);
				mEntity->sensorArray[1]->SetSensorTarget(nt);
				//mEntity->SetTarget(nt);
			}
			else if (dataBlock.trackType == Track_Unlock){
				mEntity->sensorArray[1]->SetSensorTarget(NULL);
				//mEntity->SetTarget(NULL);
			}
#else
			if (dataBlock.trackType == Track_Lock){
				mEntity->sensorArray[1]->RemoteBuggedTarget = taggedEntity;
			}
			else if (dataBlock.trackType == Track_Unlock){
				mEntity->sensorArray[1]->RemoteBuggedTarget = NULL ;
			}
#endif
		}
	}

	if (taggedEntity){
		// HACK HACK HACK!  This should be status info on the aircraft concerned...
		// check for smoke on/off messages
		if (dataBlock.trackType == Track_SmokeOn){
			((AircraftClass *)taggedEntity)->playerSmokeOn = TRUE;
		}
		else if (dataBlock.trackType == Track_SmokeOff){
			((AircraftClass *)taggedEntity)->playerSmokeOn = FALSE;
		}
		else if (dataBlock.trackType == Track_JettisonAll){
			if ((!IsLocal ()) && (taggedEntity->IsAirplane ())){
				//MonoPrint ("JettisonAll %08x\n", dataBlock.id, dataBlock.hardpoint);
				((AircraftClass*)taggedEntity)->Sms->EmergencyJettison ();
			}
		}
		else if (dataBlock.trackType == Track_JettisonWeapon){
			if ((!IsLocal ()) && (taggedEntity->IsAirplane ())){
				//MonoPrint ("Jettison Weapon %08x %d\n", dataBlock.id, dataBlock.hardpoint);
				((AircraftClass*)taggedEntity)->Sms->JettisonWeapon (dataBlock.hardpoint);
			}
		}
		else if (dataBlock.trackType == Track_RemoveWeapon){
			if ((!IsLocal ()) && (taggedEntity->IsAirplane ())){
				//MonoPrint ("RemoveWeapon %08x %d\n", dataBlock.id, dataBlock.hardpoint);
				((AircraftClass*)taggedEntity)->Sms->RemoveWeapon (dataBlock.hardpoint);
			}
		}
		else if (taggedEntity->IsLocal()){
			// 2002-02-09 COMMENT BY S.G. 
			// In here, hardpoint will be used to specify the kind of radar mode the radar was in when.
			if (taggedEntity->IsAirplane()){
				// Find RWR and/or HTS
				for (int j=0; j<((SimVehicleClass*)taggedEntity)->numSensors; j++){
					if (((SimVehicleClass*)taggedEntity)->sensorArray[j]->Type() == SensorClass::RWR){
						((RwrClass*)((SimVehicleClass*)taggedEntity)->sensorArray[j])->ObjectDetected(
							theEntity, dataBlock.trackType, dataBlock.hardpoint
						); // 2002-02-09 MODIFIED BY S.G. Added hardpoint to ObjectDetected
					}
					if (((SimVehicleClass*)taggedEntity)->sensorArray[j]->Type() == SensorClass::HTS){
						((RwrClass*)((SimVehicleClass*)taggedEntity)->sensorArray[j])->ObjectDetected(
							theEntity, dataBlock.trackType, dataBlock.hardpoint
						);  // 2002-02-09 MODIFIED BY S.G. Added hardpoint to ObjectDetected
					}
				}
			}
			else if (taggedEntity->IsFlight()){
				if ((dataBlock.trackType == Track_Lock) || (dataBlock.trackType == Track_Launch)){
					// Tell the campaign entity that it's being locked on to.
					((Flight)taggedEntity)->RegisterLock(theEntity);
				}
			}
		}
	}
	return 1;
}

