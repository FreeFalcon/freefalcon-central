#ifndef CAPIBWFSM_HPP
#define CAPIBWFSM_HPP

/** @file capibwfsm.hpp
* Header file for bandwidth finite state machine. See bw.png for diagram
* @author sfr
*/

#include "capibwstate.h"

/** this is the finite state machine declaration for bandwidth */
class BwFSM {
public:
	/** constructor initializes data and put FSM in start state */
	BwFSM();
	/** destructor finalizes FSM data */
	~BwFSM();
	/** puts FSM in start position */
	void Reset();

	/** enters a given state */
	void EnterState(bwstates);

	/** called when a player joins, state not changed, but bandwidth is adjusted according to state */
	void PlayerJoined();
	/** called when a player leaves, state not changed, but bandwidth is adjusted according to state */
	void PlayerLeft();
	
	/** returns bandwidth for this state, bytes per second */
	int GetBandwidth();	
	
private:
	/** computes bw for machine, based on state */
	void ComputeBW();

	/** number of players connected. Does not include ourselves */
	unsigned int num_players;
	/** bandwidth allocated */
	int bw;
	/** state machine is in */
	bwstates mac_state;

	// useful constants
	/** these are the minimum BW for each state */
	static int minBw[NOSTATE_ST];
	/** bw increment for each client joined */
	static int stBw[NOSTATE_ST];
};

#endif
