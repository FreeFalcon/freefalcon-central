#include "stdhdr.h"
#include "aircrft.h"
#include "debris.h"
#include "chaff.h"
#include "flare.h"
#include "camp2sim.h"
#include "hardpnt.h"
#include "entity.h"
#include "classtbl.h"
#include "PilotInputs.h"
#include "playerop.h"
#include "SimDrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "fack.h"
#include "playerrwr.h"
#include "falcsess.h"

/* ADDED BY S.G. FOR 'GetVehicleClassData' */#include "vehicle.h"
//MI for EWS stuff
#include "icp.h"
#include "cpmanager.h"
#include "soundfx.h"
#include "find.h"
#include "commands.h"
#include "airframe.h" // JPO
#include "IvibeData.h"

#include "Graphics/Include/drawbsp.h"

extern bool g_bRealisticAvionics;

//extern VuAntiDatabase *vuAntiDB;


static long	ChaffTime            = 2 * SEC_TO_MSEC;
static long	FlareTime            = 1 * SEC_TO_MSEC;
static long	ProgramDropDuration  = 5 * SEC_TO_MSEC;
// 2000-11-17 MODIFIED BY S.G. MEEDS TO BE TWO SECONDS AND NOT HALF A SECOND (TOO FAST)
// static long AutoProgramTiming    = SEC_TO_MSEC / 2;
static long AutoProgramTiming    = SEC_TO_MSEC * 2;

//MI
extern bool g_bMLU;

void AircraftClass::InitCountermeasures (void)
{
	//int type = GetClassID (
		//DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_BOMB_IRON, SPTYPE_MK82, VU_ANY, VU_ANY, VU_ANY) + 
		//VU_LAST_ENTITY_TYPE; // JB 010220
	int type = 
		GetClassID(
			DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY
		) + 
		VU_LAST_ENTITY_TYPE
	; // JB 010220
	
	// Add Chaff and flares
	// NOTE:  Since chaff and flares are created upon deployment, the use of a full hardpoint
	// is kinda wasteful.  The debris could probably be created upon need as well to save
	// a bunch of extra "bombs" hanging arround in memory.
	counterMeasureStation[FLARE_STATION].weaponCount = af->auxaeroData->nFlare;
	counterMeasureStation[FLARE_STATION].weaponPointer.reset();
	
	counterMeasureStation[CHAFF_STATION].weaponCount = af->auxaeroData->nChaff;
	counterMeasureStation[CHAFF_STATION].weaponPointer.reset();
	
	counterMeasureStation[DEBRIS_STATION].weaponPointer.reset();
	counterMeasureStation[DEBRIS_STATION].weaponCount = af->auxaeroData->nChaff;
}

void AircraftClass::DoCountermeasures (void)
{
	// 2000-11-17 ADDED BY S.G. SO AIRCRAFT HAVE A FLAG TELLING IF THEY CARRY CHAFFS/FLARES OR NOT
	if (!(GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE)->Flags & 0x40000000))
		return;
	// END OF ADDED SECTION

	if (mFaults && (mFaults->GetFault(FaultClass::cmds_fault) & FaultClass::bus))
		return;
	
	if (!IsSetFlag(ON_GROUND))
	{
		if (dropFlareCmd) {
			if (!(mFaults && (mFaults->GetFault(FaultClass::cmds_fault) & FaultClass::flar)))
			{
				DropFlare();
			}
			dropFlareCmd = FALSE;
		}
		else if (dropChaffCmd) {
			if (!(mFaults && (mFaults->GetFault(FaultClass::cmds_fault) & FaultClass::chaf)))
			{
				DropChaff();
			}
			dropChaffCmd = FALSE;
		}
		else if (dropProgrammedStep)
		{
			// Run the countdown to the next programmed event
			// 2000-09-02 S.G. dropProgrammedTimer IS int WHILE SimLibMajorFrameTime 
			// IS A float LESS THAN 1.0! NO WONDER IT'S NOT WORKING! ONLY USE int FOR NOW ON
			//if (dropProgrammedTimer > SimLibMajorFrameTime)
			if (dropProgrammedTimer + AutoProgramTiming < SimLibElapsedTime)
//			{
//				// Not time yet.  Just keep counting down
//				dropProgrammedTimer -= FloatToInt32(SimLibMajorFrameTime);
//			} 
//			else
			{
				// Time to do something
				switch (dropProgrammedStep) {
				  case 3:
					dropChaffCmd = TRUE;
					break;
				  case 2:
					dropFlareCmd = TRUE;
					break;
				  case 1:
					dropChaffCmd = TRUE;
					break;
				  default:
					ShiWarning( "Bad counter measures program step" );
					dropProgrammedStep = 1;
				}

				// Set the next state
				dropProgrammedStep--;
				// 2000-09-02 S.G. NEED TO SET IT TO SimLibElapsedTime INSTEAD
				//dropProgrammedTimer = AutoProgramTiming;
				dropProgrammedTimer = SimLibElapsedTime;
			}
		}
	}
	
	if (ChaffExpireTime() < SimLibElapsedTime)
	{
		SetChaffExpireTime( 0 );
		SetNewestChaffID( FalconNullId );
	}
	
	if (FlareExpireTime() < SimLibElapsedTime)
	{
//me123 for flare timing		SetFlareExpireTime( 0 );
		SetNewestFlareID( FalconNullId );
	}
}

void AircraftClass::DropChaff (void)
{
	vector		pos, posDelta;
	int		type;
	BombClass	*weapon;
	
	if (counterMeasureStation[CHAFF_STATION].weaponCount > 0)
	{

		if (this == FalconLocalSession->GetPlayerEntity())
		    g_intellivibeData.ChaffDropped++;
		/*
		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos();
		posDelta.x = XDelta() * 0.75F;
		posDelta.y = YDelta() * 0.75F;
		posDelta.z = ZDelta() * 0.75F;
		*/
		// new positional Dispensers 
		int NumToLaunch = 1;
		
		if(af->auxaeroData->Chaff.Sequence==2)
		{
			NumToLaunch=af->auxaeroData->Chaff.Count;
		}

		int i;
		for(i=0;i<NumToLaunch && counterMeasureStation[CHAFF_STATION].weaponCount > 0;i++)
		{
            counterMeasureStation[CHAFF_STATION].weaponCount--;
			Tpoint work;
			MatrixMult( &((DrawableBSP*)af->platform->drawPointer)->orientation, &af->auxaeroData->Chaff.Pos[chaffDispenser], &work ); 		
			pos.x=work.x + XPos();
			pos.y=work.y + YPos();
			pos.z=work.z + ZPos();

			MatrixMult( &((DrawableBSP*)af->platform->drawPointer)->orientation, &af->auxaeroData->Chaff.Vec[chaffDispenser], &work ); 		
			posDelta.x=work.x + XDelta();
			posDelta.y=work.y + YDelta();
			posDelta.z=work.z + ZDelta();


			switch(af->auxaeroData->Chaff.Sequence)
			{
			case 0: // alternate dispensers;
			case 2:
				chaffDispenser++;

				if(chaffDispenser>=af->auxaeroData->Chaff.Count)
				  chaffDispenser=0;
				break;
			case 1: // use 1 dispenser, then move to the next
			default:
				chaffUsed++;
				if(chaffUsed>=af->auxaeroData->Chaff.Decoys[chaffDispenser])
				{
					chaffUsed=0;
					chaffDispenser++;
					if(chaffDispenser>=af->auxaeroData->Chaff.Count)
					  chaffDispenser=0;
				}
				break;
			}


			// TODO:  Use a different (much higher drag) type for the chaff
			//type = GetClassID (DOMAIN_AIR, CLASS_SFX, TYPE_CHAFF, STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
			type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220

			weapon = new ChaffClass(type);
			weapon->Init();
			weapon->SetParent(this);
			weapon->Start(&pos, &posDelta, 0.2f);
			vuDatabase->/*Quick*/Insert(weapon);
			weapon->Wake();
		}

		SetChaffExpireTime( SimLibElapsedTime + ChaffTime );
		SetNewestChaffID( weapon->Id() );
	}
	//MI for EWS stuff
	if(g_bRealisticAvionics && this == FalconLocalSession->GetPlayerEntity())
	{
		if(counterMeasureStation[CHAFF_STATION].weaponCount == 0)
		{
			SoundPos.Sfx(af->auxaeroData->sndBBChaffFlareOut);
			//make sure we don't get here again, no sounds from now on
			counterMeasureStation[CHAFF_STATION].weaponCount--;
		}
		else if(OTWDriver.pCockpitManager->mpIcp->ChaffBingo == counterMeasureStation[CHAFF_STATION].weaponCount)
		{
			if(OTWDriver.pCockpitManager->mpIcp->EWS_BINGO_ON)
				SoundPos.Sfx(af->auxaeroData->sndBBChaffFlareLow);
		}
		//MI Moved further down
		/*if(counterMeasureStation[CHAFF_STATION].weaponCount > 0)
			F4SoundFXSetDist(SFX_BB_CHAFLARE, FALSE, 0.0f, 1.0f);*/
	}


	// If this is the player and they want unlimited chaff, let 'em have it
	if (IsSetFlag(MOTION_OWNSHIP) && PlayerOptions.UnlimitedChaff())
		counterMeasureStation[CHAFF_STATION].weaponCount++;
}

void AircraftClass::DropFlare (void)
{
	vector		pos, posDelta;
	int		type;
	BombClass	*weapon;
	
	if (counterMeasureStation[FLARE_STATION].weaponCount > 0)
	{
		if (this == FalconLocalSession->GetPlayerEntity())
		    g_intellivibeData.FlareDropped++;
		{
			static int chaffsid=0; // just need a fake id so multiple chaffs can play at once.
			chaffsid = (chaffsid + 1) & 0xf;
			SoundPos.Sfx( af->auxaeroData->sndBBFlare, chaffsid);
		}

/*
		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos();
		posDelta.x = XDelta() * 0.75F;
		posDelta.y = YDelta() * 0.75F;
		posDelta.z = ZDelta() * 0.75F;
*/
		// MLR 2003-11-16 New positional dispensers
		int NumToLaunch = 1;
		
		if(af->auxaeroData->Flare.Sequence==2)
		{
			NumToLaunch=af->auxaeroData->Flare.Count;
		}

		int i;
		for(i=0;i<NumToLaunch && counterMeasureStation[FLARE_STATION].weaponCount > 0;i++)
		{
			counterMeasureStation[FLARE_STATION].weaponCount--;

			Tpoint work;
			MatrixMult( &((DrawableBSP*)af->platform->drawPointer)->orientation, &af->auxaeroData->Flare.Pos[flareDispenser], &work ); 		
			pos.x=work.x + XPos();
			pos.y=work.y + YPos();
			pos.z=work.z + ZPos();

			MatrixMult( &((DrawableBSP*)af->platform->drawPointer)->orientation, &af->auxaeroData->Flare.Vec[flareDispenser], &work ); 		
			posDelta.x=work.x + XDelta();
			posDelta.y=work.y + YDelta();
			posDelta.z=work.z + ZDelta();


			switch(af->auxaeroData->Flare.Sequence)
			{
			case 0: // alternate dispensers;
			case 2:
				flareDispenser++;

				if(flareDispenser>=af->auxaeroData->Flare.Count)
				  flareDispenser=0;
				break;
			case 1: // use 1 dispenser, then move to the next
			default:
				flareUsed++;
				if(flareUsed>=af->auxaeroData->Flare.Decoys[flareDispenser])
				{
					flareUsed=0;
					flareDispenser++;
					if(flareDispenser>=af->auxaeroData->Flare.Count)
					  flareDispenser=0;
				}
				break;
			}

			//type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_BOMB_IRON, SPTYPE_MK82, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
			type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_FLARE1, SPTYPE_CHAFF1 + 1, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
			
			weapon = new FlareClass (type);
			weapon->Init();
			weapon->SetParent(this);
			weapon->Start(&pos, &posDelta, 0.2f);
			vuDatabase->/*Quick*/Insert(weapon);
			weapon->Wake();
		}
		SetFlareExpireTime( SimLibElapsedTime + FlareTime );
		SetNewestFlareID( weapon->Id() );
	}
	//MI for EWS stuff
	if(g_bRealisticAvionics && this == FalconLocalSession->GetPlayerEntity())
	{
		if(counterMeasureStation[FLARE_STATION].weaponCount == 0)
		{
			//F4SoundFXSetDist(af->auxaeroData->sndBBChaffFlareOut, TRUE, 0.0f, 1.0f);
			SoundPos.Sfx(af->auxaeroData->sndBBChaffFlareOut);
			//make sure we don't get here again, no sounds from now on
			counterMeasureStation[FLARE_STATION].weaponCount--;
		}
		else if(OTWDriver.pCockpitManager->mpIcp->FlareBingo == counterMeasureStation[FLARE_STATION].weaponCount)
		{
			if(OTWDriver.pCockpitManager->mpIcp->EWS_BINGO_ON)
				SoundPos.Sfx(af->auxaeroData->sndBBChaffFlareLow);
				//F4SoundFXSetDist( af->auxaeroData->sndBBChaffFlareLow, TRUE, 0.0f, 1.0f );
		}
		//MI moved further down
		/*else if(counterMeasureStation[FLARE_STATION].weaponCount > 0)
			F4SoundFXSetDist(SFX_BB_CHAFLARE, FALSE, 0.0f, 1.0f);*/
	}

	// If this is the player and they want unlimited chaff, let 'em have it
	if (IsSetFlag(MOTION_OWNSHIP) && PlayerOptions.UnlimitedChaff())
		counterMeasureStation[FLARE_STATION].weaponCount++;
}


void AircraftClass::CleanupCountermeasures (void)
{
}


void AircraftClass::DropProgramed (void)
{
	if (!dropProgrammedStep)
	{
		dropProgrammedStep  = 3;
		dropProgrammedTimer = 0;
	}
}
void AircraftClass::DropEWS()
{
	//Init some stuff
	ChaffCount = 0;
	FlareCount = 0;
	//make noise
	if((counterMeasureStation[FLARE_STATION].weaponCount > 0 || 
		counterMeasureStation[CHAFF_STATION].weaponCount > 0) &&
		(EWSPGM() == AircraftClass::EWSPGMSwitch::Man ||
		EWSPGM() == AircraftClass::EWSPGMSwitch::Semi ||
		EWSPGM() == AircraftClass::EWSPGMSwitch::Auto))
	{
		//F4SoundFXSetDist(af->auxaeroData->sndBBChaffFlare, FALSE, 0.0f, 1.0f);
		if(!SoundPos.IsPlaying(af->auxaeroData->sndBBChaffFlare))
		{
			SoundPos.Sfx(af->auxaeroData->sndBBChaffFlare); // MLR 5/16/2004 - 
		}
	}
	else if(counterMeasureStation[FLARE_STATION].weaponCount <= 0 && 
		counterMeasureStation[CHAFF_STATION].weaponCount <= 0)
	{
		//F4SoundFXSetDist(af->auxaeroData->sndBBChaffFlareOut, TRUE, 0.0f, 1.0f);
		SoundPos.Sfx(af->auxaeroData->sndBBChaffFlareOut); // MLR 5/16/2004 - 
	}
	ChaffBurstInterval = static_cast<VU_TIME>(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fCHAFF_BI[EWSProgNum] * CampaignSeconds));
	FlareBurstInterval = static_cast<VU_TIME>(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fFLARE_BI[EWSProgNum] * CampaignSeconds));
	FlareSalvoCount = OTWDriver.pCockpitManager->mpIcp->iFLARE_SQ[EWSProgNum] -1;
	ChaffSalvoCount = OTWDriver.pCockpitManager->mpIcp->iCHAFF_SQ[EWSProgNum] -1;		
}
void AircraftClass::EWSChaffBurst(void)
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

	if (mFaults && (mFaults->GetFault(FaultClass::cmds_fault) & FaultClass::bus))
		return;
	
	if(theRwr)
	{
		//RWR not on, no spikes!
		if(!theRwr->IsOn() || !HasPower(AircraftClass::EWSChaffPower))
			return;
	}
	else	//no RWR, return anyway
		return;

	//Set our next release time
	ChaffBurstInterval = static_cast<VU_TIME>(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fCHAFF_BI[EWSProgNum] * CampaignSeconds));
	//Drop one right now
	ChaffCount++;
	DropChaff();
}
void AircraftClass::EWSFlareBurst(void)
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

	if (mFaults && (mFaults->GetFault(FaultClass::cmds_fault) & FaultClass::bus))
		return;

	if(theRwr)
	{
		if(!theRwr->IsOn() || !HasPower(AircraftClass::EWSFlarePower))
			return;
	}
	else	//no RWR, return anyway
		return;

	//set our next release time
	FlareBurstInterval = static_cast<VU_TIME>(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fFLARE_BI[EWSProgNum] * CampaignSeconds));
	//Drop one right now
	FlareCount++;
	DropFlare();
}
void AircraftClass::ReleaseManualProgram(void)
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

	if(!theRwr)
		return;

	if(static_cast<unsigned int>(ChaffCount) >= OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[EWSProgNum] &&
		ChaffSalvoCount != 0 && !theRwr->ChaffCheck)
	{
		//Set our timer
		ChaffBurstInterval = static_cast<VU_TIME>(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fCHAFF_SI[EWSProgNum] * CampaignSeconds));
		//Reset our count
		ChaffCount = 0;
		//Mark us with one less to go
		ChaffSalvoCount--;
		//We've been here already
		theRwr->ChaffCheck = TRUE;							
	}
	if(FlareCount == OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[EWSProgNum] &&
		FlareSalvoCount != 0 && !theRwr->FlareCheck)
	{
		//Set our timer
		FlareBurstInterval = static_cast<VU_TIME>(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fFLARE_SI[EWSProgNum] * CampaignSeconds));
		//Reset our count
		FlareCount = 0;
		//Mark us with one less to go
		FlareSalvoCount--;
		//We've been here already
		theRwr->FlareCheck = TRUE;
	}
	if(SimLibElapsedTime >= ChaffBurstInterval && 
		(static_cast<unsigned int>(ChaffCount) < OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[EWSProgNum]))
	{					
		EWSChaffBurst();
		theRwr->ChaffCheck = FALSE;
	}
	if(SimLibElapsedTime >= FlareBurstInterval &&
		(static_cast<VU_TIME>(FlareCount) < OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[EWSProgNum]))
	{
		EWSFlareBurst();
		theRwr->FlareCheck = FALSE;
	}
	if(FlareCount == OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[EWSProgNum] &&
		ChaffCount == OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[EWSProgNum] &&
		ChaffSalvoCount <= 0 && FlareSalvoCount <= 0)
	{
		theRwr->ReleaseManual = FALSE;
	}

}
