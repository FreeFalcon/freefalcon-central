#ifndef _HDIGI_H
#define _HDIGI_H

#include "f4vu.h"
#include "simmath.h"
#include "simbrain.h"
#include "geometry.h"
#include "helimm.h"
#include "helo.h"

// Forward declarations for class pointers
class MissileClass;
class SimObjectType;
class SimObjectLocalData;
class SimVehicleClass;

#define CORNER_SPEED 200.0F

/*********************************************************/
/* include file for the digi aircraft                    */
/*********************************************************/

// #define CORNER_SPEED 100.0F

class HeliBrain 
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap fixed size pool
		void *operator new(size_t size) { ShiAssert( size == sizeof(HeliBrain) ); return MemAllocFS(pool);	};
		void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
		static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(HeliBrain), 20, 0 ); };
		static void ReleaseStorage()	{ MemPoolFree( pool ); };
		static MEM_POOL	pool;
#endif
	public:
		enum DigiMode {
			GroundAvoidMode, CollisionAvoidMode, GunsJinkMode, MissileDefeatMode, DefensiveModes,
			AirGroundBomb, AirGroundGuns, AirGroundMissile, GunsEngageMode, MissileEngageMode,
			RoopMode, OverBMode, WVREngageMode, BVREngageMode, AccelerateMode, OvershootMode,
			RunAwayMode, LoiterMode, SeparateMode, RTBMode, FollowOrdersMode,
			WingyMode, WaypointMode, LandingMode, GroundMnvrMode,
			LastValidMode, NoMode
		};
		enum {
			GunFireFlag = 0x1,
			MslFireFlag = 0x2
		};

		HelicopterClass* self;
		HeliMMClass* hf;
		int side;

		enum WaypointState {
			NotThereYet,
		  	Arrived,
			Stabalizing,
			OnStation,
			Landing,
			Landed,
			DropOff,
			PickUp,
			Departing
		};

		int campMission;
		int trackMode;

		// Mode Stuff
		int saveMode;
      	int waypointMode;
      	WaypointState onStation;
      	DigiMode curMode;
      	DigiMode nextMode;
      	DigiMode lastMode;

		// Targeting
		float rangedot;
		float lastAta, lastRange;
		SimObjectType* maxThreatPtr;
      	SimObjectType* maxTargetPtr;
      	SimObjectType* gunsJinkPtr;
      	SimObjectType* targetList;


		// Missile Engage
      	SimWeaponClass* curMissile;
      	int curMissileStation;
		int curMissileNum;
		int flyingMissileEid;
		int flyingMissileHid;
		float mslCheckTimer;
		float maxWpnRange;	// M.N.

		BOOL anyWeapons;

      	// Guns Engage
      	VU_TIME jinkTime;
      	VU_TIME waitingForShot;
      	float ataddot, rangeddot, mnverTime;
      	float newroll, pastAta, pastPstick, pastPipperAta;

		// Maneuvers
		int  fCount;
		float trackX, trackY, trackZ;

		float wpAlt;
		float lastMoveTime;
		void SetWPalt(float Alt) { wpAlt = Alt; };
		float GetWPalt() { return wpAlt; };

		void ResolveModeConflicts (void);
		void AddMode(DigiMode);
		void PrtMode(void);
		void PrintOnline(char *str);

		int  MissileEvade(void);
		int  MissileBeamManeuver(void);
		void MachHold (float, float, int);
		// RV - Biker - No need for PullUp
		// void PullUp (void);
		void RollAndPull(void);
		void PullToCollisionPoint(void);
		void PullToControlPoint(void);
		void MaintainClosure(void);
		void MissileDefeat(void);
		void MissileDragManeuver(void);
		void MissileLastDitch(void);
		void GunsEngage (void);
		void GunsJink (void);
		void MissileEngage(void);
		void FollowWaypoints(void);
		void Loiter(void);
		void LevelTurn(float loadFactor, float turnDir, int newTurn);
		void GammaHold (float desGamma);
		// RV - Biker - No more need for AltitudeHold
		// int AltitudeHold (float desAlt);
		float CollisionTime(void);
		void GoToCurrentWaypoint(void);
		void SelectNextWaypoint(void);
		void ChooseBrain(void);
		void RollOutOfPlane (void);
		void OverBank (float delta);
		void FindRunway (void);
		void LandMe (void);
		void SetupLanding(void);
		void CleanupLanding(void);

		float GunsAutoTrack (float targetX, float targetY, float targetZ,
							 float *elerr, float maxGs);
		void FineGunsTrack (float speed, float *lagAngle);
		void CoarseGunsTrack(float speed,float leadTof,float *newata);

		// Wingman Stuff
		int underOrders;
		int pointCounter;
		int curFormation, curOrder;
		float headingOrdered;
		float altitudeOrdered;
		void CheckOrders (void);
		void FollowOrders (void);
		void FollowLead(void);
		void CommandFlight (void);

		// Decision Stuff
		void Actions (void);
		void DecisionLogic(void);
		void WeaponSelection(void);
		void FireControl(void);
		void RunDecisionRoutines(void);
		// RV - Biker - No more need for GroundCheck
		// void GroundCheck(void);
		void GunsEngageCheck(void);
		void GunsJinkCheck(void);
		void CollisionCheck(void);
		void MissileDefeatCheck(void);
		void MissileEngageCheck(void);
		void WvrEngageCheck(void);
		void SensorFusion (void);
		void CollisionAvoid(void);
		float AutoTrack (float);
		float VectorTrack (float, int fineTrack = FALSE);
		int Stagnated (void);

	public:
		void TargetSelection( void );
		void ReceiveOrders (FalconEvent* theEvent);
		void JoinFlight (void);
		void SetLead (int flag);
		void FrameExec(SimObjectType*, SimObjectType*);
		HeliBrain (SimVehicleClass *myPlatform);
		virtual ~HeliBrain (void);

	// formerly in BaseBrain
	public:
		SimObjectType* targetPtr;
		SimObjectType* lastTarget;
		SimObjectLocalData* targetData;
		int flags;
		int isWing;
		float pStick, rStick, yPedal, throtl;
		virtual void Sleep (void) {ClearTarget();};
		void SetTarget (SimObjectType* newTarget);
		void SetTargetEntity( FalconEntity *obj );
		void ClearTarget (void);
		void SetFlag (int val) {flags |= val;};
		void ClearFlag (int val) {flags &= ~val;};
		int IsSetFlag (int val) {return (flags & val ? TRUE : FALSE);};

		// 2001-11-29 ADDED BY S.G. HELP FUNCTION TO SEARCH FOR A GROUND TARGET
		SimBaseClass *FindSimGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int startPos);
		// 2001-08-31 ADDED BY S.G. NEED TO KNOW THE LAST TWO GROUND TARGET AN AI TARGETED SO OTHER AI IN THE FLIGHT CAN SKIP THEM
		SimBaseClass *targetHistory[2];
		float nextTargetUpdate;

		// RV - Biker - Integrator for altitude PI-controler
		float powerI;
};
#endif /* _DIGI_H */
