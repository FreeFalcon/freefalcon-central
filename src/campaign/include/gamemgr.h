#ifndef _GAME_MANAGER_H
#define _GAME_MANAGER_H

#include "CampLib.h"
#include "SimBase.h"

class FalconSessionEntity;
class FalconGameEntity;
class FalconEntity;
class SimMoverClass;
class UnitClass;
class VuGameEntity;

class GameManagerClass 
	{
	private:

	public:
		GameManagerClass (void)			{};
		~GameManagerClass ()			{};

		int AllPlayersReady (VuGameEntity *game);
		int NoMorePlayers (VuGameEntity *game);
		int CheckPlayerStatus (FalconEntity *entity);
		void AnnounceEntry (void);
		void AnnounceExit (void);
		void AnnounceTransfer (SimBaseClass *oldObj, SimBaseClass *newObj);

		SimMoverClass* FindPlayerVehicle (UnitClass *campEntity, int vehSlot);
		SimMoverClass* AttachPlayerToVehicle (FalconSessionEntity *player, SimMoverClass *simEntity, int playerSlot);
		int DetachPlayerFromVehicle (FalconSessionEntity *player, SimMoverClass* simEntity);
		void ReassignPlayerVehicle (FalconSessionEntity *player, SimMoverClass *oldEntity, SimMoverClass *newEntity);

		void LockPlayer (FalconSessionEntity *player);
		void ReleasePlayer(FalconSessionEntity *player);
	};

extern GameManagerClass		GameManager;

#endif
