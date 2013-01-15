#ifndef _IVIBE_DATA_H
#define _IVIBE_DATA_H
/*
landing gear status
wheel brake status
air brakes status
game paused status (when game is not live, we should get a paused signal)
ejection? for this, we can reneder some cool effects
drop bombs?
firing guns?
firing missile? (use the + counter bytes for each type, this is the best
way)
type?
sidewinder
amraam
maverick.. probably more?

exiting the sim (you know the graphical fly by it does when you exit?  for
that we can render a flyby to match the F16 passing the viewer, so if
there's a "distance from viewpoint center" value, that would be great to
know)

For the X,Y,Z coordinates, when having an external view, we can do
some really cool things if we are able to, for example, subtract the values
and come up with the distance from the center of the screen.  Is that
possible?  It's the viewer flyby that will work best, so as the plane
approaces and whizzes by, we can make all the feedback rise and fall exactly
in sync, so it might feel like a real ^16 "buzzed the tower" so to speak...
should be cool, but there's only one way to find out! :)

escape key?
resume sim?

flare dropped?
chaff dropped?  again maybe the +1 byte counters are best?

rpm
yaw
pitch
roll
air speed
takeoff (are we on the ground, off the ground)
roll
gforce (lateral and front to back if you got 'em)
overspeed
weapons
afterburner
*/
class IntellivibeData {
public:
	IntellivibeData() { AAMissileFired = 0; AGMissileFired = 0; BombDropped = 0; FlareDropped = 0;
		ChaffDropped = 0; BulletsFired = 0; CollisionCounter = 0;
		IsFiringGun = false; IsEndFlight = false; IsEjecting = false; In3D = false;	IsPaused = false; 
		IsFrozen = false; IsOverG = false; IsOnGround = false; IsExitGame = false;
		Gforce = 1.0;
		eyex = 0.0; eyey = 0.0; eyez = 0.0; lastdamage = 0; damageforce = 0.0; whendamage = 0; }

  unsigned char AAMissileFired; // how many AA missiles fired.
  unsigned char AGMissileFired; // how many maveric/rockets fired
  unsigned char BombDropped; // how many bombs dropped
  unsigned char FlareDropped; // how many flares dropped
  unsigned char ChaffDropped; // how many chaff dropped
  unsigned char BulletsFired; // how many bullets shot
	int CollisionCounter; // Collisions
  bool IsFiringGun; // gun is firing
	bool IsEndFlight; // Ending the flight from 3d
  bool IsEjecting; // we've ejected
	bool In3D; // In 3D?
  bool IsPaused; // sim paused?
	bool IsFrozen; // sim frozen?
  bool IsOverG; // are G limits being exceeded?
  bool IsOnGround; // are we on the ground
	bool IsExitGame; // Did we exit Falcon?
  float Gforce; // what gforce we are feeling
  float eyex, eyey, eyez; // where the eye is in relationship to the plane
  int lastdamage; // 1 to 8 depending on quadrant. Make this into an enum later
  float damageforce; // how big the hit was.
  int whendamage;
};

extern IntellivibeData g_intellivibeData;
#endif