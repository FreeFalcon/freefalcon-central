/**********************s*****************************************************\
	JoyInput.h
	Scott Randolph
	February 18, 1998

	Provide interaction through a joystick.
\***************************************************************************/
#ifndef _JOYINPUT_H_
#define _JOYINPUT_H_

#include <windows.h>
#include <Mmsystem.h>

#include "grTypes.h"
#include "Matrix.h"


extern class JoyInputClass	TheJoystick;


class JoyInputClass {
  public:
  	JoyInputClass()		{ ready = FALSE; haveJoystick = 0; };
	~JoyInputClass()	{ if (ready)  Cleanup(); };

  protected:
	DWORD		lastTime;
	BOOL		ready;
	BOOL		haveJoystick;
	JOYINFOEX	joyInfoEx;
	JOYCAPS		joyCaps;

  public:                              
	float		throttleIIR;
	float		throttle;

	float		pitchRate;
	float		yawRate;
	float		rollRate;
	Trotation	deltaMatrix;

	float		headPitchRate;
	float		headYawRate;
	Trotation	headDeltaMatrix;

	DWORD		buttons;

	void Setup( void );  
	void Cleanup( void );                 
	void Update( DWORD time );

	BOOL IsReady( void )	{ return ready; };
	BOOL HaveJoystick (void)  { return haveJoystick; };
	
  private:
	void ConstructDeltaMatrix( float p, float r, float y, Trotation *T );
};

#endif /* _JOYINPUT_H_ */
