#ifndef _SIM_DOGFIGHT_H
#define _SIM_DOGFIGHT_H

#include "CampLib.h"

class FlightClass;
class FalconGameEntity;
class FalconSessionEntity;
class AircraftClass;
class TailInsertList;

// Dogfight flags
#define	DF_UNLIMITED_GUNS	0x01
#define DF_ECM_AVAIL		0x02
#define DF_GAME_OVER		0x04

// Local Dogfight flags
#define DF_VIEWED_SCORES	0x01			// User has viewed the scores, and is ready to reset
#define DF_PLAYER_REQ_REGEN	0x02			// The player has requested regeneration (by keystroke)

// Other defines
#define	MAX_DOGFIGHT_TEAMS	5

// Dogfight game types
enum DogfightType  { dog_Furball, dog_TeamFurball, dog_TeamMatchplay };

// GameStatus (Note: This is the local game status)
enum DogfightStatus { dog_Waiting, dog_Starting, dog_Flying, dog_EndRound };

// =====================================================================================
// KCK: This class will take care of all functionality associated with the various types
// of dogfight games.
// =====================================================================================

class DogfightClass
	{
	public:
		DogfightType		gameType;
		DogfightStatus		gameStatus;
		DogfightStatus		localGameStatus;
		CampaignTime		startTime;
		float				startRange;
		float				startAltitude;
		float				startX;
		float				startY;
		float				xRatio;
		float				yRatio;
		short				flags;
		short				localFlags;
		uchar				rounds;
		uchar				currentRound;
	// sfr: hiding these members 
	private:
		uchar				numRadarMissiles;
		uchar				numRearAspectMissiles;
		uchar				numAllAspectMissiles;
	public:
		static char			settings_filename[MAX_PATH];

	private:
		CampaignTime		lastUpdateTime;
		TailInsertList		*regenerationQueue;

	public:
		DogfightClass(void);
		~DogfightClass(void);

		// Game setup
		void SetGameType (DogfightType type)			{ gameType = type; };
		void SetGameStatus (DogfightStatus stat)		{ gameStatus = stat; };
		void SetRounds(uchar numr)						{ rounds=numr; }
		void SetStartRange (float newRange)				{ startRange = newRange * 0.5F;};
		void SetStartAltitude (float newAltitude)		{ startAltitude = -newAltitude;};
		void SetNumRadarMissiles (uchar newRadar)		{ numRadarMissiles = newRadar;};
		void SetNumRearAspectMissiles (uchar newRear)   { numRearAspectMissiles = newRear;};
		void SetNumAllAspectMissiles (uchar newAll)		{ numAllAspectMissiles = newAll;};
		// sfr getters for privates
		uchar GetNumRadarMissiles() const               { return numRadarMissiles;};
		uchar GetNumRearAspectMissiles() const          { return numRearAspectMissiles;};
		uchar GetNumAllAspectMissiles() const           { return numAllAspectMissiles;};

		void SetStartLocation (float newX, float newY)	{ startX = newX; startY = newY;};
		void SetFlag (int flag)							{ flags |= flag;};
		void UnSetFlag (int flag)						{ flags &= ~flag;};
		int  IsSetFlag (int flag)						{ return (flags & flag) ? 1 : 0;};
		void SetLocalFlag (int flag)					{ localFlags |= flag;};
		void UnSetLocalFlag (int flag)					{ localFlags &= ~flag;};
		int  IsSetLocalFlag (int flag)					{ return (localFlags & flag) ? 1 : 0;};
		
		DogfightType GetGameType (void)					{ return gameType; };
		DogfightStatus GetLocalGameStatus (void)		{ return localGameStatus; };
		DogfightStatus GetDogfightGameStatus (void)		{ return gameStatus; };
		int GameStarted (void)							{ if (gameStatus != dog_Waiting) return TRUE; return FALSE; };
		int GetRounds (void)							{ return rounds; };
		float StartX (void)								{ return startX;};
		float StartY (void)								{ return startY;};
		float StartZ (void)								{ return startAltitude;};

		// Functionality
		void ApplySettings (void);
		void SendSettings (FalconSessionEntity *target);
		void ReceiveSettings (DogfightClass *tmpSettings);
		void ApplySettingsToFlight (FlightClass *flight);
		void RequestSettings (FalconGameEntity *game);
		int ReadyToStart (void);

		void SetFilename (char *filename);
		void LoadSettings (void);
		void SaveSettings (char *filename);

		void UpdateDogfight (void);
		void UpdateGameStatus (void);
		void RegenerateAircraft (AircraftClass *aircraft);
		int AdjustClassId(int oldid, int team);
		void EndGame (void);

	private:
		// Private functions
		int GameOver (void);
		void RestartGame (void);
		int CheckRoundOver (void);
		void RoundOver (void);
		void EndRound(void);
		void ResetRound(void);
		void RegenerateAvailableAircraft(void);
	};

extern DogfightClass SimDogfight;

#endif
