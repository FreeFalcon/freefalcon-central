
#ifndef RULES_H
#define RULES_H

#include <tchar.h>
//#include "vutypes.h"
#include "PlayerOpDef.h"

enum{RUL_PW_LEN = 20};

typedef struct 
{
	_TCHAR				Password[RUL_PW_LEN];
	int					MaxPlayers;
	float				ObjMagnification;	
	int					SimFlags;					// Sim flags
	FlightModelType		SimFlightModel;			// Flight model type
	WeaponEffectType	SimWeaponEffect;
	AvionicsType		SimAvionicsType;
	AutopilotModeType   SimAutopilotType;
	RefuelModeType		SimAirRefuelingMode;				
	PadlockModeType		SimPadlockMode;	
	ulong				BumpTimer;
	ulong				AiPullTime;
	ulong				AiPatience;
	ulong				AtcPatience;
	short				GeneralFlags;		
}RulesStruct;

typedef enum{
	rINSTANT_ACTION,
	rDOGFIGHT,
	rTACTICAL_ENGAGEMENT,
	rCAMPAIGN,
	rNUM_MODES,
}RulesModes;

class RulesClass: public RulesStruct
{
	private:
		void EncryptPwd(void);

	public:
		RulesClass(void);
		void Initialize(void);
		int LoadRules(_TCHAR *filename = _T("default"));
		int SaveRules(_TCHAR *filename = _T("default"));
		void LoadRules(RulesStruct *rules);
		RulesStruct* GetRules(void)								{return (this);}

		int CheckPassword(_TCHAR *Pwd);
		int SetPassword(_TCHAR *Pwd);
		int GetPassword(_TCHAR *Pwd);
		
		float ObjectMagnification (void)						{ return ObjMagnification; }
		int GetFlightModelType (void)							{ return SimFlightModel; }
		int GetWeaponEffectiveness (void)						{ return SimWeaponEffect; }
		int GetAvionicsType (void)								{ return SimAvionicsType; }
		int GetAutopilotMode (void)								{ return SimAutopilotType; }
		int GetRefuelingMode (void)								{ return SimAirRefuelingMode; }
		int GetPadlockMode (void)								{ return SimPadlockMode; }
		int AutoTargetingOn (void)								{ return (SimFlags & SIM_AUTO_TARGET) && TRUE; }
		int BlackoutOn (void)									{ return !(SimFlags & SIM_NO_BLACKOUT); }
		int NoBlackout (void)									{ return (SimFlags & SIM_NO_BLACKOUT) && TRUE; }
		int UnlimitedFuel (void)								{ return (SimFlags & SIM_UNLIMITED_FUEL) && TRUE; }
		int UnlimitedAmmo (void)								{ return (SimFlags & SIM_UNLIMITED_AMMO) && TRUE; }
		int UnlimitedChaff (void)								{ return (SimFlags & SIM_UNLIMITED_CHAFF) && TRUE; }
		int CollisionsOn (void)									{ return !(SimFlags & SIM_NO_COLLISIONS); }
		int NoCollisions (void)									{ return (SimFlags & SIM_NO_COLLISIONS) && TRUE; }
		int NameTagsOn (void)									{ return (SimFlags & SIM_NAMETAGS) && TRUE; }
		int WeatherOn (void)									{ return !(GeneralFlags & GEN_NO_WEATHER); }
		int PadlockViewOn (void)								{ return (GeneralFlags & GEN_PADLOCK_VIEW) && TRUE; }
		int HawkeyeViewOn (void)								{ return (GeneralFlags & GEN_HAWKEYE_VIEW) && TRUE; }
		int ExternalViewOn (void)								{ return (GeneralFlags & GEN_EXTERNAL_VIEW) && TRUE; }
		int	InvulnerableOn(void)								{ return (SimFlags & SIM_INVULNERABLE) && TRUE; }

		void SetSimFlag (int flag)								{SimFlags |= flag;};
		void ClearSimFlag (int flag)							{SimFlags &= ~flag;};
		void SetGenFlag (int flag)								{GeneralFlags |= flag;};
		void ClearGenFlag (int flag)							{GeneralFlags &= ~flag;};
		void SetObjMagnification(float mag)						{ObjMagnification = mag;};

		void SetMaxPlayers(int num)								{if(num>0) MaxPlayers = num; }
		void SetSimFlightModel(FlightModelType FM)				{SimFlightModel = FM;};			
		void SetSimWeaponEffect(WeaponEffectType WE)			{SimWeaponEffect = WE;};
		void SetSimAvionicsType(AvionicsType AT)				{SimAvionicsType = AT;};
		void SetSimAutopilotType(AutopilotModeType AM)			{SimAutopilotType = AM;};
		void SetRefuelingMode(RefuelModeType RM)				{SimAirRefuelingMode = RM;};
		void SetPadlockMode(PadlockModeType PM)					{SimPadlockMode = PM;};
};

extern RulesClass gRules[rNUM_MODES];
extern RulesModes RuleMode;

int LoadAllRules(char *filename);
#endif