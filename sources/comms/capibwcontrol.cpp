/** @file capibwcontrol.cpp
* I moved all bandwidth related functions here
* trying to make this modular 
* @author sfr
*/

#include <assert.h>
#include <time.h>
#include "capibwcontrol.h"
#include "capibwfsm.h"

// define to instrument BW usage
#define INSTRUMENT_BW 0
#if INSTRUMENT_BW
#include "RedProfiler.h"
#endif

////////////
// MACROS //
////////////

/** transforms clock to microseconds */
#define CLOCKS_TO_MSEC(clocks) (clocks * 1000 / CLOCKS_PER_SEC )
/** number of renews per second */
#define RENEWS_PER_SEC (2)
/** we renew bandiwidth every COMP_INTERVAL_MSEC microseconds */
#define RENEW_INTERVAL_MSEC (1000 / RENEWS_PER_SEC)
/** reliable modifier for each message */
#define RELIABLE_MOD (1.0f)
// percentages by type of bandwidth
// common gets the rest
// Right now Im not reserving anything...
/** percentage of total bw reserved for positional updates */
#define BW_POSITIONAL_RESERVED (0.0f)
/** percentage of total bw reserved for dirty updates */
#define BW_DIRTY_RESERVED      (0.0f)
/** percentage reserved for other stuff */
#define BW_OTHER_RESERVED      (0.0f)

///////////////////////
// BW STATE VARIABLE //
///////////////////////
/** class holding bandwidth information */
class BW {
public:
	/** this structure holds BW types. If changed here, also adjust ComAPI (look for BWTYPE) */
	typedef enum {
		BW_POSITIONAL = 0,   ///< positional update (MUST BE ZERO!!)
		BW_DIRTY,            ///< dirty data
		BW_OTHER,            ///< other stuff
		BW_COMMON,           ///< common pool
		BW_NUM_TYPES         ///< number of types
	} BWTypes;

private:
	/** percentages each type gets, common gets the rest (between 0 and 1) */
	float percentages_by_type[BW_NUM_TYPES-1];

	/** total bandwidth available, in bytes, by type */
	int bytes_by_type[BW_NUM_TYPES];
	/** bw used, by type */
	int used_by_type[BW_NUM_TYPES];

	/** last time bandwidth was renewed */
	clock_t last_renew;
	/** bw finite state machine */
	BwFSM fsm;

	// private functions
	/** sets total bw, bytes per second */
	void Set(int bw_sec){
		// gets amount per interval
		int bw = bw_sec / RENEWS_PER_SEC;
		int total = 0;
		for (int i=0;i<BW_NUM_TYPES-1;++i){
			bytes_by_type[i] = (int)(bw * percentages_by_type[i]);
			total += bytes_by_type[i];
		}
		bytes_by_type[BW_COMMON] = bw - total;
	}

	/** renew bandwidth */
	void Renew(){
		for (int i=0;i<BW_NUM_TYPES;++i){
			//used_by_type[i] = (used_by_type[i] > bytes_by_type[i]) ? (used_by_type[i] - bytes_by_type[i]) : 0;
			// lets be more flexible here...
			used_by_type[i] = 0;
		}
		last_renew = clock();
	}

#if INSTRUMENT_BW
	/** computes the total of all BW types. Used in instrumentation */
	int ComputeTotalBW() const{
		int total = 0;
		for (int i=0;i<BW_NUM_TYPES;++i){
			total += bytes_by_type[i];
		}
		return total;
	}

	/** computes the total of all BW types considering usage. Used in instrumentation */
	int ComputeTotalUsedBW() const{
		int total = 0;
		for (int i=0;i<BW_NUM_TYPES;++i){
			total += used_by_type[i];
		}
		return total;
	}
#endif

	/** checks if time for renew is reached, if so, renew */
	void RenewIfTime(){
		clock_t now = clock();
		// clock can wrap around, in this case renew bw immediately
		clock_t interval_msec = now < last_renew ? RENEW_INTERVAL_MSEC : CLOCKS_TO_MSEC(now - last_renew);
		if (interval_msec >= RENEW_INTERVAL_MSEC){
#if INSTRUMENT_BW
			// percentage used
			unsigned int total = ComputeTotalBW();
			unsigned int used = ComputeTotalUsedBW(); 
			REPORT_VALUE("BW total (per renew)", total);
			REPORT_VALUE("BW percentage", (used * 100) / total);
#endif
			Renew();
		}
	}

public:
	// pub functions
	/** constructor: resets fsm and set bandwidth */
	BW(){
		percentages_by_type[BW_POSITIONAL] = BW_POSITIONAL_RESERVED;
		percentages_by_type[BW_DIRTY] = BW_DIRTY_RESERVED;
		percentages_by_type[BW_OTHER] = BW_OTHER_RESERVED;
		Reset();
	}

	/** resets BW object */
	void Reset(){
		fsm.Reset();
		Set(fsm.GetBandwidth());
	}

	/** checks available bandwidth, by type */
	bool Check(int s_, bool isReliable, BWTypes type){
		RenewIfTime();
		// size adjustment for reliable
		int size = (int)((isReliable) ? s_*RELIABLE_MOD : s_);

		// we are flexible here
		// if we have any spare bw of that type, ok
		// this avoids starvation for low bw connections
		if (
			(used_by_type[type] < bytes_by_type[type]) ||
			(used_by_type[BW_COMMON] < bytes_by_type[BW_COMMON])
		){
			return true;
		}
		else {
			return false;
		}
	}

	/** uses bandwidth */
	void Use(int s_, bool isReliable, int type){
		RenewIfTime();
		// size adjustment for reliable
		int size = (int)((isReliable) ? s_*RELIABLE_MOD : s_);
		// subtract from reserve first
		used_by_type[type] += size;
		// see if used exceed amount for that type
		if ((used_by_type[type] > bytes_by_type[type]) && (type != BW_COMMON)){
			// consume common
			int common = used_by_type[type] - bytes_by_type[type];
			used_by_type[type] = bytes_by_type[type];
			used_by_type[BW_COMMON] += common;
		}
	}

	/** cuts all bandwidth until next renew */
	void Cut(){
		for (int i=0;i<BW_NUM_TYPES;++i){
			used_by_type[i] = bytes_by_type[i];
		}
		last_renew = clock();
	}

	// fsm functions
	/** changes fsm state */
	void EnterState(bwstates st){
		fsm.EnterState(st);
		Set(fsm.GetBandwidth());
	}

	/** called when player joins */
	void PlayerJoined(){
		fsm.PlayerJoined();
		Set(fsm.GetBandwidth());
	}

	/** called when player leaves */
	void PlayerLeft(){
		fsm.PlayerLeft();
		Set(fsm.GetBandwidth());
	}

};
/** object holding bw information */
static BW bw_object;


////////////////////////
// EXPORTED FUNCTIONS //
////////////////////////
void start_bandwidth(){
	bw_object.Reset();	
}

void enter_state(bwstates st){
	bw_object.EnterState(st);
}

void player_joined(){
	bw_object.PlayerJoined();
}

void player_left(){
	bw_object.PlayerLeft();
}

void use_bandwidth(int size, int isReliable, int type){
	bw_object.Use(size, isReliable ? true : false, type);
}

int check_bandwidth(int size, int isReliable, int type){
	return bw_object.Check(size, isReliable ? true : false, (BW::BWTypes)type);
}

void cut_bandwidth(void){
	bw_object.Cut();
}

