#include "stdhdr.h"
#include "bomb.h"
#include "camp2sim.h"
#include "hardpnt.h"
#include "entity.h"
#include "classtbl.h"
#include "pilotinputs.h"
#include "SimDrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "playerop.h"
#include "helo.h"


// edg:
// this shit's gone
#if 0

extern VuAntiDatabase *vuAntiDB;


static const long  ChaffTime			=  5 * 1000;
static const long  FlareTime			=  5 * 1000;
static const long  ProgramDropDuration	= 30 * 1000;


void HelicopterClass::InitCountermeasures (void)
{
	//short type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_BOMB_IRON, SPTYPE_MK82, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
	short type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
	
	// Add Chaff and flares
	// NOTE:  Since chaff and flares are created upon deployment, the use of a full hardpoint
	// is kinda wasteful.  The debris could probably be created upon need as well to save
	// a bunch of extra "bombs" hanging arround in memory.
	counterMeasureStation[FLARE_STATION].weaponCount = 30;
	counterMeasureStation[FLARE_STATION].weaponPointer = NULL;
	
	counterMeasureStation[CHAFF_STATION].weaponCount = 30;
	counterMeasureStation[CHAFF_STATION].weaponPointer = NULL;
	
	counterMeasureStation[DEBRIS_STATION].weaponPointer = new BombClass (type, BombClass::Debris);
	counterMeasureStation[DEBRIS_STATION].weaponPointer->Init();
	counterMeasureStation[DEBRIS_STATION].weaponCount = 60;
}

void HelicopterClass::DoCountermeasures (void)
{
	if (!IsSetFlag(ON_GROUND))
	{
		if (dropFlareCmd)
		{
			DropFlare();
			dropFlareCmd = FALSE;
		}
		else if (dropChaffCmd)
		{
			DropChaff();
			dropChaffCmd = FALSE;
		}
	}
	
	if (ChaffExpireTime() < SimLibElapsedTime)
	{
		SetChaffExpireTime( 0 );
		SetNewestChaffID( FalconNullId );
	}
	
	if (FlareExpireTime() < SimLibElapsedTime)
	{
		SetFlareExpireTime( 0 );
		SetNewestFlareID( FalconNullId );
	}
}

void HelicopterClass::DropChaff (void)
{
	vector		pos, posDelta;
	short		type;
	BombClass	*weapon;
	
	if (counterMeasureStation[CHAFF_STATION].weaponCount-- > 0)
	{
		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos();
		posDelta.x = XDelta() * 0.75F;
		posDelta.y = YDelta() * 0.75F;
		posDelta.z = ZDelta() * 0.75F;

		// TODO:  Use a different (much higher drag) type for the chaff
		//type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_BOMB_IRON, SPTYPE_MK82, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
		type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220

		weapon = new BombClass (type, BombClass::Chaff);
		weapon->Init();
		weapon->SetParent(this);
		weapon->Start(&pos, &posDelta, 0.2f);
		vuDatabase->Insert(weapon);
		weapon->Wake();

		SetChaffExpireTime( SimLibElapsedTime + ChaffTime );
		SetNewestChaffID( weapon->Id() );
	}

	// If this is the player and they want unlimited chaff, let 'em have it
	if (IsSetFlag(MOTION_OWNSHIP) && PlayerOptions.UnlimitedChaff())
		counterMeasureStation[CHAFF_STATION].weaponCount++;
}

void HelicopterClass::DropFlare (void)
{
	vector		pos, posDelta;
	short		type;
	BombClass	*weapon;
	
	if (counterMeasureStation[FLARE_STATION].weaponCount-- > 0)
	{
		//F4SoundFXSetPos( SFX_FLARE, TRUE, XPos(), YPos(), ZPos(), 1.0f, 0 , XDelta(),YDelta(),ZDelta() );
		SoundPos.Sfx( SFX_FLARE ); // MLR 5/16/2004 - 
		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos();
		posDelta.x = XDelta() * 0.75F;
		posDelta.y = YDelta() * 0.75F;
		posDelta.z = ZDelta() * 0.75F;

		//type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_BOMB_IRON, SPTYPE_MK82, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220
		type = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_BOMB, STYPE_FLARE1, SPTYPE_CHAFF1 + 1, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE; // JB 010220

		weapon = new BombClass (type, BombClass::Flare);
		weapon->Init();
		weapon->SetParent(this);
		weapon->Start(&pos, &posDelta, 0.2f);
		vuDatabase->Insert(weapon);
		weapon->Wake();

		SetFlareExpireTime( SimLibElapsedTime + ChaffTime );
		SetNewestFlareID( weapon->Id() );
	}

	// If this is the player and they want unlimited chaff, let 'em have it
	if (IsSetFlag(MOTION_OWNSHIP) && PlayerOptions.UnlimitedChaff())
		counterMeasureStation[FLARE_STATION].weaponCount++;
}

void HelicopterClass::CleanupCountermeasures (void)
{
   if (counterMeasureStation)
   {
//      vuAntiDB->Remove(counterMeasureStation[DEBRIS_STATION].weaponPointer);
      delete counterMeasureStation[DEBRIS_STATION].weaponPointer;
      counterMeasureStation[DEBRIS_STATION].weaponPointer = NULL;
   }
}
#endif
