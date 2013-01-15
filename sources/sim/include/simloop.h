/***************************************************************************
	SimLoop.h
    Scott Randolph
    June 18, 1998

	This class is the master loop for the simulation and graphics.
	It starts and stops each as appropriate during transitions between
	the SIM and UI.
***************************************************************************/
#ifndef _SIMLOOP_H_
#define _SIMLOOP_H_
#include "stdhdr.h"

class SimluationDriver;

class SimulationLoopControl {
public:
	static void StartSim(void);
	static void StopSim(void);

	static void StartGraphics(void);
	static void StopGraphics(void);

	static void Loop(void);
	static void StartLoop(void);

	static bool InSim(void)			{ return currentMode == RunningGraphics; }
	static int  GetSimTick(void)	{ return sim_tick; }

	static HANDLE wait_for_sim_cleanup;
	static HANDLE wait_for_graphics_cleanup;

protected:
	static enum SimLoopControlMode { 
		Stopped,
		StartingSim, 
		RunningSim, 
		StartingGraphics, 
		Step2, 
		StartRunningGraphics, 
		RunningGraphics, 
		StoppingGraphics,
		Step5,
		StoppingSim,
	} currentMode;

	static HANDLE wait_for_start_graphics;
	static HANDLE wait_for_stop_graphics;
	static int sim_tick;
};


#endif // _SIMLOOP_H_
