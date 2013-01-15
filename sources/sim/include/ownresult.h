#ifndef _OWNSHIP_RESULTS_H
#define _OWNSHIP_RESULTS_H

class OwnshipResultsClass
{
   public:
      enum WeaponTypes {
         ShortRangeMissile,
         LongRangeMissile,
         AirGroundMissile,
         Rockets,
         IronBombs,
         Bullets,
         Special,
         NumWeaponTypes};

      OwnshipResultsClass(void);
      ~OwnshipResultsClass (void);
      int NumUsed (int type) {return numWeaponsUsed[type];};
      int EnemyAirKills (void) {return numEnemyAirKills;};
      int EnemyGroundKills (void) {return numEnemyGroundKills;};
      int FriendlyAirKills (void) {return numFriendlyAirKills;};
      int FriendlyGroundKills (void) {return numFriendlyGroundKills;};
      int TargetStatus (void) {return targetStatus;};
      int EscortStatus (void) {return escortStatus;};
      int EndStatus (void) {return endStatus;};
      int FireFlag (void) {return didFire;};
      float TOT (void) { return tot;};
      float PlannedTOT (void) {return plannedTOT;};

      void SetNumUsed (int type, int val) {numWeaponsUsed[type] = val;};
      void SetEnemyAirKills (int val) {numEnemyAirKills = val;};
      void SetEnemyGroundKills (int val) {numEnemyGroundKills = val;};
      void SetFriendlyAirKills (int val) {numFriendlyAirKills = val;};
      void SetFriendlyGroundKills (int val) {numFriendlyGroundKills = val;};
      void SetTargetStatus (int val) {targetStatus = val;};
      void SetEscortStatus (int val) {escortStatus = val;};
      void SetEndStatus (int val) {endStatus = val;};
      void SetFireFlag (int val) {didFire = val;};
      void SetTOT (float val) { tot = val;};
      void SetPlannedTOT (float val) {plannedTOT = val;};

      void ClearData (void);
         
   private:
      int numWeaponsUsed[NumWeaponTypes];
      int numEnemyAirKills;
      int numEnemyGroundKills;
      int numFriendlyAirKills;
      int numFriendlyGroundKills;
      int targetStatus;
      int escortStatus;
      int didFire;
      int endStatus;
      float tot;
      float plannedTOT;
};

extern OwnshipResultsClass OwnResults;

#endif
