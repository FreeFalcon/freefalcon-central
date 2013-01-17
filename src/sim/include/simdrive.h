#ifndef _SIMDRIVE_H
#define _SIMDRIVE_H

#include <map>

#include "falclib/include/f4vu.h"
#include "SimLoop.h"
#include "FalcSess.h"
#include "aircrft.h" 

#include "objectiv.h"		// RAS - 16Jan04 - include for traffic call code


#define MAX_IA_CAMP_UNIT 0x10000

class SimBaseClass;
class SimInitDataClass;
class FalconSessionEntity;
class CampBaseClass;
class TailInsertList;
class FalconPrivateOrderedList;
class FalconPrivateList;
class SimMoverClass;
class CampBaseClass;
class AircraftClass;
class SimFeatureClass;
class FlightClass;
class ObjectiveClass;
class UnitClass;

/** responsible for running sim entities (deaggregated). */
class SimulationDriver
{
public:
	SimulationDriver();
	~SimulationDriver();

	/** One time setup (at application start) */
	void Startup();
	/** One time shutdown (at application exit) */
	void Cleanup();
	/** One SIM cycle (could be multiple time steps) */
	void Cycle();
	/** Enter the SIM from the UI */
	void Enter();
	/** Set up sim for exiting */
	void Exit();

	// simple querries
	/** returns if in sim. */
	bool InSim() const                 { return SimulationLoopControl::InSim(); }
	/** are we running IA? */
	bool RunningInstantAction() const  { return FalconLocalGame->GetGameType() == game_InstantAction; }
	/** are we running DF? */
	bool RunningDogfight() const             { return FalconLocalGame->GetGameType() == game_Dogfight; }
	/** are we running TE? */
	bool RunningTactical() const             { return FalconLocalGame->GetGameType() == game_TacticalEngagement; }
	/** are we running CA? */
	bool RunningCampaign() const             { return FalconLocalGame->GetGameType() == game_Campaign; }
	/** are we running TE or CA? */
	bool RunningCampaignOrTactical() const   { return RunningCampaign() || RunningTactical(); }

	/** pause sim. */
	void Pause();
	/** remove pause */
	void NoPause();
	/** toggle pause */
	void TogglePause();

	// ??
	void SetFrameDescription(int mSecPerFrame, int numMinorFrames);

	/** sets the player entity in simdriver. This will ref the entity and unref old one if any. */
	void SetPlayerEntity(SimMoverClass* newObject);
	/** returns the player entity */
	SimMoverClass *GetPlayerEntity() const { return const_cast<SimMoverClass*>(playerEntity); }
    /** returns player entity aircraft IF it is an aircraft, otherwise return NULL */
	AircraftClass *GetPlayerAircraft() const;

	void UpdateIAStats(SimBaseClass*);

	// find functions
	SimBaseClass *FindNearestThreat(float*, float*, float*);
	SimBaseClass *FindNearestThreat(short *x, short *y, float *alt);
	SimBaseClass *FindNearestThreat(AircraftClass *aircraft, short *x, short *y, float *alt);
	SimBaseClass *FindNearestEnemyPlane(AircraftClass *aircraft, short *x, short *y, float *alt);	
	CampBaseClass *FindNearestCampThreat(AircraftClass *aircraft, short *x, short *y, float *alt);
	CampBaseClass *FindNearestCampEnemy(AircraftClass *aircraft, short *x, short *y, float *alt);
	SimBaseClass *FindFac(SimBaseClass*);
	FlightClass *FindTanker(SimBaseClass*);
	SimBaseClass *FindATC(VU_ID);
	SimBaseClass *FindNearestTraffic(AircraftClass* aircraft, ObjectiveClass *self, float* altitude);
	void FindTrafficConflict(SimBaseClass *traffic, AircraftClass *myaircraft, ObjectiveClass *self);

	// ??
	void UpdateRemoteData();

	int MotionOn(void) {return motionOn;};
	void SetMotion (int newFlag) {motionOn = newFlag;};
	int AVTROn (void) {return avtrOn;};
	void SetAVTR (int newFlag) {avtrOn = newFlag; };

	// add functions
	void AddToFeatureList(VuEntity* theObject);
	void AddToObjectList(VuEntity* theObject);
	void AddToCampUnitList(VuEntity* theObject);
	void AddToCampFeatList(VuEntity* theObject);
	void AddToCombUnitList(VuEntity* theObject);
	void AddToCombFeatList(VuEntity* theObject);

	// remove functions
	void RemoveFromFeatureList(VuEntity* theObject);
	void RemoveFromObjectList(VuEntity* theObject);
	void RemoveFromCampUnitList(VuEntity* theObject);
	void RemoveFromCampFeatList(VuEntity* theObject);

	// list access function, see list names for each meaning
	//FalconPrivateOrderedList * GetMoverList() const { 
	//	return const_cast<FalconPrivateOrderedList*>(moverList); 
	//}
	//FalconPrivateOrderedList * GetFeatureList() const {
	//	return const_cast<FalconPrivateOrderedList*>(featureList); 
	//}
	//FalconPrivateOrderedList * GetCampUnitList() const { 
	//	return const_cast<FalconPrivateOrderedList*>(campUnitList); 
	//}
	//FalconPrivateOrderedList * GetCampObjList() const { 
	//	return const_cast<FalconPrivateOrderedList*>(campObjList);
	//}
	//FalconPrivateOrderedList * GetCombinedList() const { 
	//	return const_cast<FalconPrivateOrderedList*>(combinedList);
	//}
	//FalconPrivateOrderedList * GetCombinedFeatureList() const { 
	//	return const_cast<FalconPrivateOrderedList*>(combinedFeatureList); 
	//} 
	//FalconPrivateList * GetObjsWithNoCampaignParentList() const { 
	//	return const_cast<FalconPrivateList*>(objsWithNoCampaignParentList); 
	//}
	//FalconPrivateList * GetFacList() const { 
	//	return const_cast<FalconPrivateList*>(facList) ;
	//}
	//VuLinkedList * GetAtcList() const { 
	//	return const_cast<VuLinkedList*>(atcList); 
	//}
	//VuLinkedList * GetTankerList() const { 
	//	return const_cast<VuLinkedList*>(tankerList);
	//}

	// others
	void InitACMIRecord();
	void POVKludgeFunction(DWORD);
	void InitializeSimMemoryPools();
	void ReleaseSimMemoryPools();
	void ShrinkSimMemoryPools();
	void WakeCampaignBase(int ctype, CampBaseClass *baseEntity, TailInsertList *components);
	void WakeObject(SimBaseClass* theObject);
	void SleepCampaignFlight(TailInsertList *flightList);
	void SleepObject(SimBaseClass* theObject);
	void NotifyExit(){ doExit = TRUE; };
	void NotifyGraphicsExit() { doGraphicsExit = TRUE; };
	/** sfr: @todo this is MLR hack for cycle again. should be gone. */
	void ObjAddedDuringCycle()	{ cycleAgain = 1; }

	/// sfr: @todo make private
	FalconPrivateOrderedList* objectList;		// List of locally deaggregated sim vehicles
	FalconPrivateOrderedList* featureList;		// List of locally deaggregated sim features
	FalconPrivateOrderedList* campUnitList;		// List of nearby aggregated campaign units
	FalconPrivateOrderedList* campObjList;		// List of nearby aggregated campaign objectives
	FalconPrivateOrderedList* combinedList;		// List of everything nearby
	FalconPrivateOrderedList* combinedFeatureList;	// List of everything nearby
	FalconPrivateList *ObjsWithNoCampaignParentList; // ??
	FalconPrivateList *facList; // ??
	VuFilteredList *atcList; // ??
	VuFilteredList *tankerList; // ??

	int doFile;
	int doEvent;
	uint eventReadPointer;
	unsigned long	lastRealTime;

	
private:
	// sfr: this is private now. I want more control of it
	// also, this is not an Aircraft (can be an ejected pilot)
	SimMoverClass* playerEntity;

	// SCR:  These used to be local to the loop function, but moved
	// here when the loop got broken out into a cycle per function call.
	// Some or all of these may be unnecessary, but I'll keep them for now...
	bool inCycle;
	bool cycleAgain;

	unsigned long	last_elapsedTime;
	int			lastFlyState;
	int			curFlyState;
	BOOL 		doExit;
	BOOL 		doGraphicsExit;

	/** responsible for getting vu messages directed to the simloop thread. */
	VuThread* vuThread;
	int curIALevel;
	char dataName[_MAX_PATH];
	int motionOn;
	int avtrOn;
	unsigned long nextATCTime;
	
	void UpdateEntityLists();
	void ReaggregateAllFlights();
	SimBaseClass* FindNearest(SimBaseClass*, VuLinkedList*);
	void UpdateATC(void);

};

// global declaration
extern SimulationDriver SimDriver;

#endif
