/** @file capibwfsm.cpp
* BwFSM implementation
* @author sfr
*/
#include <stdexcept>
#include <sstream>
#include "capibwfsm.h"

using namespace std;

// minimum bw for each state in bytes per second, setting min 32kbps for each
//	LOBBY_ST = 0,      // lobby
//	CAS_ST   = 1,      // inside CA or TE as server
//	CAC_ST   = 2,      // inside CA or TE as client
//	DF_ST    = 3,      // in dogfight
int BwFSM::minBw[NOSTATE_ST] = {
	6000, 8000, 6000, 6000
};

// bw increment for each state, in bytes per second
int BwFSM::stBw[NOSTATE_ST] = {
	300, 8000, 300, 300
};

void BwFSM::ComputeBW(){
	bw = stBw[mac_state] * num_players;
	if (bw < minBw[mac_state]){
		bw = minBw[mac_state];
	}
}

BwFSM::BwFSM(){
	Reset();
}

BwFSM::~BwFSM(){
	// nothing to do here
}

void BwFSM::Reset(){
	EnterState(LOBBY_ST);
}

void BwFSM::EnterState(bwstates st){
	num_players = 0;
	mac_state = st;
	ComputeBW();
}

void BwFSM::PlayerJoined(){
	++num_players;
	ComputeBW();
}

void BwFSM::PlayerLeft(){
	if (num_players == 0){
		// can happen when we are alone and leave game
		return;
	}
	--num_players;
	ComputeBW();
}

int BwFSM::GetBandwidth(){
	return bw;
}
