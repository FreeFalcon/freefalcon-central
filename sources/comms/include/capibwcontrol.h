#ifndef CAPIBWCONTROL_H
#define CAPIBWCONTROL_H

/** @file capibwcontrol.h
* CAPI bandwidth functions
* This is the local BW API for falcon
* Used only internally in ComAPI
* @author sfr
*/

#include "capibwstate.h"

#ifdef __cplusplus
extern "C" {
#endif

/** starts bandwidth control */
void start_bandwidth();

/** enters a given state, adjusting bandwidth */
void enter_state(bwstates);

/** called when a new player joins, adjusting bw */
void player_joined();

/** called when a player leaves, adjusting bw */
void player_left();

/** consume bandwidth
* @param size amount of bytes to consume
* @param isReliable indicates we are consuming reliable data. This means size will be adjusted,
* @param type of bandwidth being used
* since reliable consumes more
*/
void use_bandwidth (int size, int isReliable, int type);

/** checks if there is available bandwidth. if isRUDP, size is adjusted since RUDP consumes more 
* @param size amount of bytes
* @param isReliable indicates we are consuming RUDP, meaning size will be adjusted 
* @param type of bandwidth being used
* @return 0 if there is no BW available, different otherwise
*/
int check_bandwidth(int size, int isReliable, int type);

/** we call this when we want to stop sending (for example, when a send would block because of a full queue */
void cut_bandwidth();

/** gets status regarding BW usage 
* @param isReliable status for reliable connection? or regular?
* @return ok: 0, warning: 1, critical: 0
*/
int get_status(int isReliable);

#ifdef __cplusplus
}
#endif
#endif