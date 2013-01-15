
#ifndef FALCGAME_H
#define FALCGAME_H

#include "vusessn.h"
#include "tchar.h"
#include "rules.h"

// ==========================================
// defines
// ==========================================

// ==========================================
// Externs
// ==========================================

// ==========================================
// Game types
// ==========================================

enum FalconGameType
{
	game_PlayerPool=0,
	game_InstantAction,
	game_Dogfight,
	game_TacticalEngagement,
	game_Campaign,
	game_MaxGameTypes, // This MUST be the last type (I use it as an array size) Please don't assign values individually
	// (Except for playerpool = 0)
};

// ==========================================
// Class shit
// ==========================================

class FalconGameEntity : public VuGameEntity
{
public:
	FalconGameType	gameType;
	RulesClass		rules;
		
public:
	// constructors & destructor
	FalconGameEntity(ulong domainMask, char *gameName);
	//sfr: added rem
	FalconGameEntity(VU_BYTE **stream, long *rem);
	FalconGameEntity(FILE *file);
	virtual ~FalconGameEntity();

	// encoders
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);
	virtual int Save(FILE *file);

	void DoFullUpdate(void);

	// accessors
	FalconGameType GetGameType (void);
	RulesClass *GetRules(void)					{ return &rules;	}

	// setters
	void SetGameType (FalconGameType type);
	void UpdateRules (RulesStruct *newrules);
	void EncipherPassword(char *pw,long size);
	long CheckPassword(_TCHAR *passwd);

	// event Handlers
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
	// Other crap
	virtual VU_ERRCODE Distribute(VuSessionEntity *sess);

private:
	int LocalSize(void) const; 
};

#endif

