#ifndef FALCON_DAMAGE_H
#define FALCON_DAMAGE_H

class FalconDamageType
{
public:
   enum {
      BulletDamage,
      MissileDamage,
      CollisionDamage,
      BombDamage,
      FODDamage,
      GroundCollisionDamage,
      ObjectCollisionDamage,
      FeatureCollisionDamage,
      DebrisDamage,
      ProximityDamage,
	  OtherDamage,		// KCK: Use if you don't want any messages/scoring to occur
   };
};

#endif
