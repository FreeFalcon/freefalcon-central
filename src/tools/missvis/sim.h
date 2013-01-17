/**********************s*****************************************************\
	sim.h
	Scott Randolph
	November 25, 1994

	Provide the simulations calcuations and interaction with the user.
\***************************************************************************/
#ifndef _SIM_H_
#define _SIM_H_

#include "Utils\Types.h"
#include "Utils\Matrix.h"


class SimClass {
  public:
  	SimClass()	{ ready = FALSE; };
	~SimClass()	{ if (ready)  Cleanup(); };

  protected:
	DWORD		lastTime;
	BOOL		ready;
	BOOL		haveJoystick;
	JOYINFOEX	joyInfoEx;
	JOYCAPS		joyCaps;
	float		e1;               
	float		e2;              	
	float		e3;              	
	float		e4;

  public:                              
	Tpoint		position;
	Trotation	rotation;

	float		speed;
	float		pitchRate;
	float		yawRate;
	float		rollRate;

	float		headPitchRate;
	float		headYawRate;
	float		headPitch;
	float		headYaw;

	Tpoint		headPos;
	Trotation	headMatrix;

	DWORD		buttons;

	void  Setup( void );  
	void  Cleanup( void );                 
	DWORD Update( DWORD time );

	void  Save( char *filename );
	void  Restore( char *filename );

	void Up( float distance )		{ position.z -= distance; };
	void Down( float distance )		{ position.z += distance; };
#if 0
	// The following are appoximations that don't work if the axis of motion is near vertical
	void Forward( float d )		{ position.x += d * rotation.M11;	position.y += d * rotation.M21; };
	void Backward( float d )	{ position.x -= d * rotation.M11;	position.y -= d * rotation.M21; };
	void Rightward( float d )	{ position.x += d * rotation.M12;	position.y += d * rotation.M22; };
	void Leftward( float d )	{ position.x -= d * rotation.M12;	position.y -= d * rotation.M22; };
#else
	void Forward( float d )		{ position.x += d; };
	void Backward( float d )	{ position.x -= d; };
	void Rightward( float d )	{ position.y += d; };
	void Leftward( float d )	{ position.y -= d; };
#endif

	BOOL IsReady( void )	{ return ready; };
	
  private:
	void UpdateRotation( float rollRate, float pitchRate, float yawRate );
};

#endif /* _SIM_H_ */
