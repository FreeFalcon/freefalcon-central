/** @file vudriver.cpp
* Implementation of classes which drive units in Vu
* Must not depend on any falclib file
* Rewrite: sfr
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "vu2.h"

/** number of update renews per second */
#define RENEWS_PER_SECOND (2)
/** maximum number of position updates sent per update per client by server */
#define MAX_SERVER_SENDS_PER_UPDATE (40)
/** maximum number of position updates sent per update per client by client */
#define MAX_CLIENT_SENDS_PER_UPDATE (5)

// tolerance for a position update generation
/** if unit moves more than this qty in any axis, an update can be generated. Unit used: feet */
#define MOVE_TOLERANCE (1.0f)
/** if unit turns more than this qty in any axis, update can be generated. In radians */
#define TURN_TOLERANCE (2*PI / 10.0f)


// useful constants
#define VU_TICS_PER_SEC_INV         (1.0f/VU_TICS_PER_SECOND)

// useful function
/** returns time interval in seconds between the 2 timestamps. Returns negative if last_timestamp is bigger */
inline double GetDT(VU_TIME timestamp, VU_TIME last_timestamp){
	// look out here, time is unsigned, hence the check
	if (timestamp > last_timestamp){
		return ((SM_SCALAR)(timestamp - last_timestamp))*VU_TICS_PER_SEC_INV;
	}
	else {
		return -(((SM_SCALAR)(last_timestamp - timestamp))*VU_TICS_PER_SEC_INV);
	}
}

//////////////
// VUDRIVER //
//////////////
VuDriver::VuDriver(VuEntity* entity) : entity_(entity){
	//lastUpdateGameTime_ = 0;
	lastUpdateGameTime_ = entity->LastUpdateTime();
}

VuDriver::~VuDriver(){
}

/*
void VuDriver::AlignTimeAdd(VU_TIME dt){
	lastUpdateGameTime_ += dt;
}

void VuDriver::AlignTimeSubtract(VU_TIME dt){
	lastUpdateGameTime_ -= dt;
}
*/

void VuDriver::ResetLastUpdateTime(VU_TIME time){
	lastUpdateGameTime_ = time;
	entity_->SetUpdateTime(time);
}

//////////////////
// VUDEADRECKON //
//////////////////
VuDeadReckon::VuDeadReckon(VuEntity* entity) : VuDriver(entity){
	VuDeadReckon::Reset();
	ResetLastUpdateTime(0);
}

VuDeadReckon::~VuDeadReckon(){
}

void VuDeadReckon::Reset(){
	// get unit position and heading
	d_drx_     = entity_->XDelta();
	d_dry_     = entity_->YDelta();
	d_drz_     = entity_->ZDelta();
	drx_       = entity_->XPos();
	dry_       = entity_->YPos();
	drz_       = entity_->ZPos();
	dryaw_     = entity_->Yaw();
	drpitch_   = entity_->Pitch();
	drroll_    = entity_->Roll();
}

void VuDeadReckon::NoExec(VU_TIME time){
	ResetLastUpdateTime(time);
}

void VuDeadReckon::ExecDR(VU_TIME timestamp){
	BIG_SCALAR dt = 
		(lastUpdateGameTime_ == 0) ? 0 : static_cast<BIG_SCALAR>(GetDT(timestamp, lastUpdateGameTime_))
	;

	// if update was not enough to change the value, we zero it...
	// this is a float precision problem
	BIG_SCALAR bu; // values before update
	BIG_SCALAR pval[3], dpval[3], tval[3], dtval[3]; // value and increment
	pval[0] = drx_;    pval[1] = dry_;     pval[2] = drz_;
	dpval[0] = d_drx_; dpval[1] = d_dry_;  dpval[2] = d_drz_;
	tval[0] = dryaw_;  tval[1] = drpitch_; tval[2] = drroll_;
	dtval[0] = d_dryaw_;  dtval[1] = d_drpitch_; dtval[2] = d_drroll_;

	for (unsigned int i=0;i<3;++i){
		// position
		BIG_SCALAR inc = dt*dpval[i];
		bu = pval[i];
		pval[i] += inc;
		if (bu == pval[i]){ dpval[i] = 0; } // if increment was not enough to update, set speed to 0
		// turn
		inc = dt*dtval[i];
		bu = tval[i];
		tval[i] += inc;
		if (bu == tval[i]){ dtval[i] = 0; } // if increment was not enough to update, set speed to 0

		// keeps turn between -PI and +PI
		if (tval[i] > VU_PI){ 
			tval[i] -= VU_TWOPI; 
		}
		else if (tval[i] < -VU_PI){
			tval[i] += VU_TWOPI; 
		}
	}
	drx_ = pval[0];     dry_ = pval[1];      drz_ = pval[2];
	d_drx_ = dpval[0];  d_dry_ = dpval[1];   d_drz_ = dpval[2];
	dryaw_ = tval[0];   drpitch_ = tval[1];  drroll_ = tval[2];
	d_dryaw_ = dtval[0];   d_drpitch_ = dtval[1];  d_drroll_ = dtval[2];

	entity_->SetPosition(drx_, dry_, drz_);
	entity_->SetDelta(d_drx_, d_dry_, d_drz_);
	entity_->SetYPR(dryaw_, drpitch_, drroll_);
	entity_->SetYPRDelta(d_dryaw_, d_drpitch_, d_drroll_);

/*
	val = bu = drx_;
	inc = dt*d_drx_;
	drx_ = val + inc;
	if (bu == drx_){ d_drx_ = 0; } // if increment was not enough to update, set speed to 0

	val = bu = dry_;
	inc = dt*d_dry_;
	dry_ = val + inc;
	if (bu == dry_){ d_dry_ = 0; }
	
	val = bu = drz_;
	inc = dt*d_drz_;
	drz_ = val + inc;
	if (bu == drz_){ d_drz_ = 0; }

	// sfr: we should use AGL instead of 0 here, since some places can be below 0...
	//if (drz_ > 0.0f){
	//	// z is inverted (- is up)
	//	drz_ = 0.0f;
	//}
	//entity update
	entity_->SetPosition(
		static_cast<SM_SCALAR>(drx_), 
		static_cast<SM_SCALAR>(dry_), 
		static_cast<SM_SCALAR>(drz_)
	);
	entity_->SetDelta(d_drx_, d_dry_, d_drz_);
	*/
	ResetLastUpdateTime(timestamp);
}

//////////////////
// VUDELAYSLAVE //
//////////////////
VuDelaySlave::VuDelaySlave(VuEntity *entity) : VuDeadReckon(entity){ 
	VuDelaySlave::Reset();
}

void VuDelaySlave::Reset(){
	VuDeadReckon::Reset();
	predictedTime_ = 0;//vuxGameTime;
	lastRemoteUpdateTime_ = 0;
}

void VuDelaySlave::NoExec(VU_TIME timestamp){
	VuDeadReckon::NoExec(timestamp);
}

void VuDelaySlave::Exec(VU_TIME timestamp){
	// check if prediction is recent
	if (predictedTime_ > timestamp){
		// speed is already set by handle, execute dead reckon
		ExecDR(timestamp);
	}
	// our prediction is far past, this prolly means we didnt receive
	// an update for this units for sometime
	else{
		VuDelaySlave::NoExec(timestamp);
	}
}

VU_ERRCODE VuDelaySlave::Handle(VuPositionUpdateEvent* event){
	// here we just set position
	// only accept events newer than last update
	if (event->updateTime_ < lastRemoteUpdateTime_){
		return VU_NO_OP;
	}

	// we do not receive any dYPR data
	entity_->SetPosition(event->x_, event->y_, event->z_);
	entity_->SetDelta(event->dx_, event->dy_, event->dz_);
	entity_->SetYPR(event->yaw_, event->pitch_, event->roll_);

	return VU_SUCCESS;
}

//////////////
// VUMASTER //
//////////////

// static variable
int VuMaster::toSend = 0;

/** returns maximum number of sends for entity. */
namespace {
	unsigned int MaxSends(){
		if (vuLocalGame->OwnerId() == vuLocalSession){
			return MAX_SERVER_SENDS_PER_UPDATE*(vuLocalGame->SessionCount()-1);
		}
		else {
			return MAX_CLIENT_SENDS_PER_UPDATE*(vuLocalGame->SessionCount()-1);
		}	
	}
}

void VuMaster::ResetToSendIfTime(){
	VU_TIME lastSend = 0;
	VU_TIME now = vuxGameTime;
	// every half second, renews position updates allowed
	if (now >= lastSend + (VU_TICS_PER_SECOND/RENEWS_PER_SECOND)){
		lastSend = now;
		toSend = static_cast<int>(MaxSends());
	}
}

unsigned int VuMaster::SendsPerPlayer(){
	int otherPlayers = vuLocalSessionEntity->Game()->SessionCount() - 1;
	// avoids division by zero
	return ((otherPlayers > 0) && (toSend > 0)) ? (MaxSends() / otherPlayers) : 0;
}

VuMaster::VuMaster(VuEntity* entity) : VuDeadReckon(entity){
	updateSentRealTime_ = 0;
	xsent_ = entity_->XPos();
	ysent_ = entity_->YPos();
	zsent_ = entity_->ZPos();
	dxsent_ = entity->XDelta();
	dxsent_ = entity->YDelta();
	dxsent_ = entity->ZDelta();
	yawsent_ = entity_->Yaw();
	pitchsent_ = entity_->Pitch();
	rollsent_ = entity_->Roll();
}

VuMaster::~VuMaster(){
}

VU_ERRCODE VuMaster::GeneratePositionUpdate(bool reliable, bool oob, VU_TIME time, VuSessionEntity *target){
	// send message
	VuPositionUpdateEvent *event = new VuPositionUpdateEvent(entity_, target);
	if (reliable){
		event->RequestReliableTransmit();
	}
	if (oob){
		event->RequestOutOfBandTransmit();
	}
	if (VuMessageQueue::PostVuMessage(event) <= 0) {
		return VU_ERROR; // send failed
	}
	// update sent
	xsent_ = entity_->XPos();
	ysent_ = entity_->YPos();
	zsent_ = entity_->ZPos();
	yawsent_ = entity_->Yaw();
	pitchsent_ = entity_->Pitch();
	rollsent_ = entity_->Roll();
	updateSentRealTime_ = vuxRealTime;
	return VU_SUCCESS;
}

inline bool VuMaster::ToleranceReached(){
	return 
		(abs(entity_->XPos() - xsent_) >= MOVE_TOLERANCE) ||
		(abs(entity_->YPos() - ysent_) >= MOVE_TOLERANCE) ||
		(abs(entity_->ZPos() - zsent_) >= MOVE_TOLERANCE) ||
		(abs(entity_->Yaw()   - yawsent_  ) >= TURN_TOLERANCE) ||
		(abs(entity_->Pitch() - pitchsent_) >= TURN_TOLERANCE) ||
		(abs(entity_->Roll()  - rollsent_ ) >= TURN_TOLERANCE)
	;
}

void VuMaster::Exec(VU_TIME timestamp){
	ResetToSendIfTime();

	// exec model, if fails, exec DR
	if (!ExecModel(timestamp)){
		ExecDR(timestamp);
	}

	// unit updated
	ResetLastUpdateTime(timestamp);

	//bool sent = false;
	VU_TIME timeDelta = vuxRealTime - updateSentRealTime_;
	VuSessionsIterator iter(vuLocalGame);
	for (VuSessionEntity *s=iter.GetFirst();s!=NULL;s=iter.GetNext()){
		// dont send to ourselves!
		if (s == vuLocalSessionEntity){
			continue;
		}
		// check if we should send position to this session
		SEND_SCORE score = SendScore(s, timeDelta);
		if (score.first >= SEND_OOB){
			s->EnqueueOobPositionUpdate(entity_);
			--toSend;
		}
		else if ((score.first == ENQUEUE_SEND) && (toSend > 0)){
			// enqeue send, dont decrement to sent (will be done during enqueued sends)
			// do this to optimize enqueued PU (they wont be sent if OOB)
			s->EnqueuePositionUpdate(score.second, entity_);
		}
	}
}

