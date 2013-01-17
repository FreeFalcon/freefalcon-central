#ifndef _FFEEDBK_H
#define _FFEEDBK_H

int JoystickPlayEffect(int effectNum, int data);
void JoystickStopEffect(int effectNum);

enum {
   JoyFireEffect = 0,
   JoyHitEffect = 1,
   JoyAutoCenter = 2,
   JoyRunwayRumble1 = 3,
   JoyRunwayRumble2 = 4,
   JoyLeftDrop = 5,
   JoyRightDrop = 6,
   JoyStall1 = 7,
   JoyStall2 = 8,
};
#endif