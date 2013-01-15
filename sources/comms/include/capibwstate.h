#ifndef CAPIBWSTATE_H
#define CAPIBWSTATE_H

/** state definition for bandwidth 
* @author sfr
*/

/** these are possible states for bw control 
* WARNING: if changed here, change also capi.h
*/
typedef enum {
	LOBBY_ST = 0,      ///< lobby
	CAS_ST   = 1,      ///< inside CA or TE as server
	CAC_ST   = 2,      ///< inside CA or TE as client
	DF_ST    = 3,      ///< in dogfight
	NOSTATE_ST         ///< number of states
} bwstates;

#endif