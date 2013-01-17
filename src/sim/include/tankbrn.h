#ifndef _TANKBRAIN_H
#define _TANKBRAIN_H

#include "digi.h"
#include "Graphics\Include\drawbsp.h"
#include "object.h"

class TailInsertList;
class SimVehicleClass;
//class SimObjectType;

enum{
	BOOM_AZIMUTH,
	BOOM_ELEVATION,
	BOOM_EXTENSION,
};

typedef struct BoomData{
	float			rx;				//relative to tanker center in ft
	float			ry;				//relative to tanker center in ft
	float			rz;				//relative to tanker center in ft
	DrawableBSP*	drawPointer;	//pointer to our boom
	float			az;				//in radians
	float			el;				//in radians
	float			ext;			//how far boom is extended in ft
}BoomData;

class TankerBrain : public DigitalBrain
{
public:
	enum TnkrType{
		TNKR_KC10,
		TNKR_KC135,
		TNKR_KCBOOM,
		TNKR_KCDROGUE,
		TNKR_KCBOTH,
		TNKR_IL78,
		TNKR_UNKNOWN
	};

   private:
      TailInsertList *thirstyQ;
	  HeadInsertList *waitQ;
      SimVehicleClass* curThirsty;
	  int		numBooms;	// FRB - number of booms/boom service	required
	  int		numDrogues;// FRB - number of drogues/drogue service	required
		float DrogueExt; // FRB - how far the basket extends
		Tpoint DrogueRFPos; // FRB - a/c postion adj.
		Tpoint BoomRFPos; // FRB - a/c postion adj.
		int DROGUE; // FRB - the drogue Slot to refuel from
		int BOOM; // FRB - the boom Slot to refuel from
	  DrawableBSP*	rack[5];
	  BoomData	boom[5];
	  TnkrType	type;	
    int stype; // FRB - subtype	(has direction lights - KC-135)
    int ServiceType; // FRB - A/C sevice type requirement (boom or drogue)
      int flags;
      float holdAlt;
      SimObjectType* tankingPtr;
      long lastStabalize;
      long lastBoomCommand;
	  int turnallow;	// M.N.
	  int directionsetup; // M.N.
	  int HeadsUp; // MN
	  float desSpeed; // MN
	  vector TrackPoints[4]; // 2002-03-13 MN
	  int currentTP; // 2002-03-13 MN
	  bool advancedirection; // if we go from TrackPoint 0->1->2->3 or 0->3->2->1 (latter is the case if tanker is outside min max tanker range envelope - would do a 180° turn in the other case)
	  bool reachedFirstTrackpoint; // when we reached the first trackpoint, we limit rStick and pStich in wingmnvers.cpp
	  float trackPointDistance; // contains closest distance at < 5 nm trackpoint distance

      void CallNext (void);
      void DriveBoom (void);
      void DriveLights (void);
      void BreakAway (void);
      void TurnTo (float newHeading);
      void FollowThirsty (void);
	  void TurnToTrackPoint (int trackPoint); // 2002-03-13 MN
      

   public:
	   
      enum TankerFlags {
         IsRefueling = 0x1,
         PatternDefined = 0x2,
         GivingGas = 0x4,
         PrecontactPos = 0x10,
         ContactPos = 0x20,
				 ClearingPlane = 0x40,
				 AIready = 0x80}; // 27NOV03 - FRB 
      TankerBrain (AircraftClass *myPlatform, AirframeClass* myAf);
      virtual ~TankerBrain (void);
      void FrameExec(SimObjectType*, SimObjectType*);
      int  AddToQ (SimVehicleClass* thirstyOne);
      void RemoveFromQ (SimVehicleClass* thirstyOne);
	  void AIReady (void); // 27NOV03 - FRB
	  int AddToWaitQ (SimVehicleClass* doneOne);
	  void PurgeWaitQ (void);
	  int TankingPosition(SimVehicleClass* thirstyOne);
	  void DoneRefueling (void);
	  int IsSet(int flag)				{return (flags & flag) && TRUE;}
	  SimObjectType* TankingPtr(void)	{return tankingPtr;}

	  void OptTankingPosition(Tpoint *pos);
	  void BoomWorldPosition(Tpoint *pos);
	  void ReceptorRelPosition(Tpoint *pos, SimVehicleClass *thirsty);
	  void BoomTipPosition(Tpoint *pos);
	  virtual int IsTanker(void)	{return TRUE;}
	  virtual void InitBoom(void);
	  virtual void CleanupBoom(void);
	  void SetInitial(void)	{ turnallow = 0; HeadsUp = 0; directionsetup = 1; currentTP = 0; }
	  float GetDesSpeed(void) {return desSpeed; }
	  bool ReachedFirstTrackPoint(void) {return reachedFirstTrackpoint; }

#ifdef USE_SH_POOLS
public:
	// Overload new/delete because our parent class does (and assumes a fixed size)
	void *operator new(size_t size) { return MemAllocPtr(pool, size, 0); };
	void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
};

#endif /* _FACBRAIN_H */
