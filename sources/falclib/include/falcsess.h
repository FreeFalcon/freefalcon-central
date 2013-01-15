
#ifndef FALCSESS_H
#define FALCSESS_H

#include <list>

#include "tchar.h"
#include "FalcGame.h"
//sfr: added for lengths
#include "UI/INCLUDE/logbook.h"

/** sfr: maximum number of entities a session can have fine interest
* MUST be smaller than 256
*/
#define FALCSESS_MAX_FINE_INTEREST 5
#if FALCSESS_MAX_FINE_INTEREST > 255
#error "FALCSESS_MAX_FINE_INTEREST greater than 255"
#endif
// sfr: fine interest define
#define FINE_INT 1


// ==========================================
// Fly states
// ==========================================
#define FLYSTATE_IN_UI				0					// Sitting in the UI
#define	FLYSTATE_LOADING			1					// Loading the sim data
#define FLYSTATE_WAITING			2					// Waiting for other players
#define FLYSTATE_FLYING				3					// Flying
#define FLYSTATE_DEAD				4					// We're now dead, waiting for respawn

// ==========================================
// Other defines
// ==========================================
#define FS_MAXBLK					320					// Maximum number of data blocks

// ==========================================
// Externs
// ==========================================
extern uchar GetTeam(uchar country);

class SquadronClass;
class FlightClass;
class FalconEntity;

// ==========================================
// Class 
// ==========================================
class FalconSessionEntity : public VuSessionEntity
{
public:
	enum {
		_AIR_KILLS_=0,
		_GROUND_KILLS_,
		_NAVAL_KILLS_,
		_STATIC_KILLS_,
		_KILL_CATS_,

		_VS_AI_		= 1,
		_VS_HUMAN_	= 2,
	};

private:
	//sfr: for some reason, game is raising trying to delete name, allocate statically
	_TCHAR			name[_NAME_LEN_+1];
	_TCHAR			callSign[_CALLSIGN_LEN_+1];
	VU_ID			playerSquadron;
	VU_ID			playerFlight;
	VU_ID			playerEntity;
	// sfr: using smartpointers
	VuBin<SquadronClass> playerSquadronPtr;
	VuBin<FlightClass> playerFlightPtr;
	VuBin<FalconEntity>	playerEntityPtr;
	float			AceFactor;			// Player Ace Factor
	float			initAceFactor;		// AceFactor at beginning of match
	float			bubbleRatio;		// This session's multiplier for the player bubble size
	ushort			kills[_KILL_CATS_]; // Player kills - can't keep in log book
	ushort			missions;			// Player missions flown
	uchar			country;			// Country or Team player is on
	uchar			aircraftNum;		// Which aircraft in a flight we're using (0-3)
	uchar			pilotSlot;			// Which pilot slot we've been assigned to
	uchar			flyState;			// What we're doing (sitting in UI, waiting to load, flying)
	short			reqCompression;		// Requested compression rate
	ulong			latency;			// Current latency estimate for this session
	uchar			samples;			// # of samples in current latency calculation
	uchar			rating;				// Player rating (averaged)
	uchar			voiceID;			// Player's voice of choice

	uchar			assignedAircraftNum;
	uchar			assignedPilotSlot;
	FlightClass		*assignedPlayerFlightPtr;
#if FINE_INT
	// sfr: fine interest object list
	typedef VuBin<FalconEntity> FalconEntityBin;
	typedef std::list<FalconEntityBin> FalconEntityList;
	FalconEntityList fineInterestList;
#endif



public:
	uchar*			unitDataSendBuffer;		// Unit data the local session is sending to this session
	short			unitDataSendSet;
	long			unitDataSendSize; 
	uchar*			unitDataReceiveBuffer;	// Unit data the local session is receiving from this session
	short			unitDataReceiveSet;
	uchar			unitDataReceived[FS_MAXBLK/8];
	uchar*			objDataSendBuffer;		// Objective data the local session is sending to this session
	short			objDataSendSet;
	long			objDataSendSize; 
	uchar*			objDataReceiveBuffer;	// Objective data the local session is receiving from this session
	short			objDataReceiveSet;
	uchar			objDataReceived[FS_MAXBLK/8];

public:
	// constructors & destructor
	FalconSessionEntity(ulong domainMask, char *callsign);
	//sfr: added rem
	FalconSessionEntity(VU_BYTE **stream, long *rem);
	FalconSessionEntity(FILE *file);
	virtual VU_ERRCODE InsertionCallback(void);
	virtual ~FalconSessionEntity();

	// encoders
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);
	void DoFullUpdate(void);

	// accessors
	_TCHAR* GetPlayerName(void)                   { return name; }
	_TCHAR* GetPlayerCallsign(void)               { return callSign; }
	VU_ID GetPlayerSquadronID(void) const         { return playerSquadron; }
	VU_ID GetPlayerFlightID(void) const           { return playerFlight; }
	VU_ID GetPlayerEntityID(void) const           { return playerEntity; }
	// sfr: changed to FalconEntity
	//VuEntityEntity* GetPlayerEntity(void) const     { return playerEntityPtr.get(); }
	FalconEntity* GetPlayerEntity(void) const     { return playerEntityPtr.get(); }
	FlightClass* GetPlayerFlight(void) const      { return playerFlightPtr.get(); }
	SquadronClass* GetPlayerSquadron(void)	const { return playerSquadronPtr.get(); }
	float GetAceFactor() const                    { return(AceFactor); }
	float GetInitAceFactor() const                { return(initAceFactor); }
	uchar GetTeam (void);
	uchar GetCountry (void) const                 { return country; }
	uchar GetAircraftNum (void) const             { return aircraftNum; }
	uchar GetPilotSlot (void) const               { return pilotSlot; }
	uchar GetFlyState (void) const                { return flyState; }
	short GetReqCompression (void) const          { return reqCompression; }
	float GetBubbleRatio (void) const             { return bubbleRatio; }
	ushort GetKill(ushort CAT) const              { if(CAT < _KILL_CATS_) return(kills[CAT]); return(0); }
	ushort GetMissions() const                    { return missions; }
	uchar GetRating() const                       { return rating; }
	uchar GetVoiceID() const                      { return voiceID; }
	FalconGameEntity* GetGame (void)              { return (FalconGameEntity*)VuSessionEntity::Game(); }

	uchar GetAssignedAircraftNum (void) const     { return assignedAircraftNum; }
	uchar GetAssignedPilotSlot (void) const       { return assignedPilotSlot; }
	FlightClass* GetAssignedPlayerFlight(void) const { return assignedPlayerFlightPtr; }

	// setters
	void SetPlayerName (_TCHAR* pname);
	void SetPlayerCallsign (_TCHAR* pcallsign);

	void SetPlayerSquadron (SquadronClass* ent);		
	void SetPlayerSquadronID (VU_ID id);		
	void SetPlayerFlight (FlightClass* ent);		
	void SetPlayerFlightID (VU_ID id);
	// sfr: changed to falcon entity
	//void SetPlayerEntity(VuEntity* ent);			
	void SetPlayerEntity(FalconEntity* ent);			
	void SetPlayerEntityID (VU_ID id);			
	void UpdatePlayer (void);

	void SetAceFactor (float af);
	void SetInitAceFactor (float af);
	void SetCountry (uchar c);
	void SetAircraftNum (uchar an);
	void SetPilotSlot (uchar ps);
	void SetFlyState (uchar fs);
	void SetReqCompression (short rc);
	void SetVuStateAccess (VU_MEM state)	{ SetVuState(state); }
	void SetBubbleRatio (float ratio)	{ bubbleRatio = ratio;}
	void SetKill(ushort CAT,ushort kill){ if(CAT < _KILL_CATS_) kills[CAT]=kill; }
	void SetMissions(ushort count)		{ missions = count; }
	void SetRating(uchar newrat)		{ rating = newrat; }
	void SetVoiceID (uchar v)			{ voiceID = v; }

	void SetAssignedAircraftNum (uchar an);
	void SetAssignedPilotSlot (uchar ps);
	void SetAssignedPlayerFlight (FlightClass* ent);		

	// Calculate new ace factor
	void SetAceFactorKill(float opponent);
	void SetAceFactorDeath(float opponent);
	// queries
	int InSessionBubble (FalconEntity* ent, float bubble_multiplier = 1.0F);

	// event Handlers
	//		virtual VU_ERRCODE Handle(VuEvent *event);
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
	//		virtual VU_ERRCODE Handle(VuSessionEvent *event);

#if FINE_INT
	// sfr
	/* add a unit to this session fine interest list. This unit will be updated in fine fashion */
	bool AddToFineInterest(FalconEntity *entity, bool silent = true);
	/** removes unit from fine interest list. Will be updated normally */
	bool RemoveFromFineInterest(const FalconEntity *entity, bool silent = true);
	/** clears fine interest list */
	void ClearFineInterest(bool silent = true);
	/** checks if a unit is in fine interest list */
	bool HasFineInterest(const FalconEntity *entity) const ;
#endif

};

// Some conversion equivalencies between vuLocalSession and FalconLocalSession
#define FalconLocalSession ((FalconSessionEntity*)vuLocalSessionEntity.get())
#define FalconLocalSessionId vuLocalSession
#define FalconLocalGame (vuLocalSessionEntity ?\
	((FalconSessionEntity*)vuLocalSessionEntity.get())->GetGame() : NULL)
#define FalconLocalGameId (vuLocalSessionEntity->GameId())

#endif

