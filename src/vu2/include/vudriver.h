// vudriver.h - Copyright (c) Tue July 2 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VUDRIVER_H_
#define _VUDRIVER_H_

#include <utility>

// forward decls
class VuEntity;
class VuEvent;

/** returns the dt for position computations based on time variation */
inline double GetDT(VU_TIME timestamp, VU_TIME last_timestamp);

//////////////
// VUDRIVER //
//////////////
/** A vuDriver is responsible for driving an entity, ie, updating its position in 3d world.
* this is an interface for the other drivers, defining only most basic functions
*/
class VuDriver {
protected:
	VuDriver(VuEntity *entity);
public:
	virtual ~VuDriver();

	/** dont execute mode, stay put */
	virtual void NoExec(VU_TIME timestamp) = 0;
	/** exec model */
	virtual void Exec(VU_TIME timestamp) = 0;

	virtual VU_ERRCODE Handle(VuEvent *event) = 0;
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event) = 0;
	virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event) = 0;

	// sfr: not used
	//virtual void AlignTimeAdd(VU_TIME timeDelta);
	//virtual void AlignTimeSubtract(VU_TIME timeDelta);
	/** sets unit and driver update time */
	virtual void ResetLastUpdateTime(VU_TIME time);

	// returns entity driven by driver
	VuEntity *Entity() { return entity_; }

	// AI hooks
	/*
	virtual int AiType();
	virtual VU_BOOL AiExec();
	virtual void *AiPointer();*/

// Data
protected:
	/** last time driver updated unit, game time */
	VU_TIME lastUpdateGameTime_;

	/** entity driven */
	VuEntity *entity_;
};

//////////////////
// VUDEADRECKON //
//////////////////
/** a dead reckon driver updates units position based on its last position and velocity measured
* based on a time variation
* this class is used when we wait for a true position update (example: clients)
*/
class VuDeadReckon : public VuDriver {
public:
	VuDeadReckon(VuEntity *entity);
	virtual ~VuDeadReckon();

	// drive model
	/** sets driver DR variable to entity ones */
	virtual void NoExec(VU_TIME timestamp);
	/** execute dead reckon computations for entity */
	virtual void ExecDR(VU_TIME timestamp);
protected:
	/** resets DR data */
	void Reset();

// DATA
protected:
	// dead reckoning variables
	/** position */
	BIG_SCALAR drx_, dry_, drz_;
	/** received speed */
	SM_SCALAR d_drx_, d_dry_, d_drz_;
	/** turning */
	SM_SCALAR dryaw_, drpitch_, drroll_;
	/** received turning speed */
	SM_SCALAR d_dryaw_, d_drpitch_, d_drroll_;
};

//////////////////
// VUDELAYSLAVE //
//////////////////
/** a delayed slave is a slave which receives updates lately, so it must infer future positions
* Every unit in client world is driven by a DelayedSlave locally. Once positions are received 
* DR data is corrected for a more precise update.
* This causes an accumulated error, which must be reset once in a while
*/
class VuDelaySlave : public VuDeadReckon {
public:
	VuDelaySlave(VuEntity *entity);
	virtual ~VuDelaySlave(){}

	/** computes DR data based on past data */
	virtual void Exec(VU_TIME timestamp);
	/** resets all data and sets update time */
	virtual void NoExec(VU_TIME timestamp);
	/** resets all data */
	void Reset();

	// handle position update
	virtual VU_ERRCODE Handle(VuEvent *event){ return VU_NO_OP; }
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event){ return VU_NO_OP; }
	virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event); // just set position...

protected:
	/** last time an updated was received */
	VU_TIME lastRemoteUpdateTime_;
	// acceleration data
	SM_SCALAR dd_drx_, dd_dry_, dd_drz_;

	// predicted position for this unit
	//BIG_SCALAR px_, py_, pz_;
	// time upon which prediction was made
	VU_TIME predictedTime_;
};

//////////////
// VUMASTER //
//////////////
/** master is the driver which really drives and entity, applying AI to it 
* its also responsible for generating position updates of the unit driven
* by it
*/
class VuMaster : public VuDeadReckon {
public:
	VuMaster(VuEntity *entity);
	virtual ~VuMaster();

	/** does nothing */
	virtual void NoExec(VU_TIME timestamp){}
	/** executes masters model, calls ExecModel */
	virtual void Exec(VU_TIME timestamp);

	/** executes drivers model and returns if model was run
	* called by exec function
	*/
	virtual VU_BOOL ExecModel(VU_TIME timestamp) = 0;

	// master are source of data, no need to handle events (maybe a full update?)
	virtual VU_ERRCODE Handle(VuEvent *event){ return VU_NO_OP; }
	virtual VU_ERRCODE Handle(VuFullUpdateEvent *event){ return VU_NO_OP; }
	virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event){ return VU_NO_OP; }

private:
	/** resets toSend value */
	static void ResetToSendIfTime();
	
	/** indicates how many position updates can be sent yet */
	static int toSend;

public:
	/** returns how many sends can be made per player */
	static unsigned int SendsPerPlayer();

	/** generates a position update for this unit */
	VU_ERRCODE GeneratePositionUpdate(bool reliable, bool oob, VU_TIME timestamp, VuSessionEntity *e);


// DATA
protected:
	/** checks if tolerance value was reached to send pos update */
	bool ToleranceReached();

	/** send score enum */
	enum SCORE {
		DONT_SEND = 0,
		ENQUEUE_SEND,
		SEND_OOB
	};
	/** this type is returned by the send score function. The second member of the pair is the 
	* distance from the entity squared. Its used only if SEND_SCORE is ENQEUE_SEND. Otherwise its ignored.
	*/
	typedef std::pair<SCORE, SM_SCALAR> SEND_SCORE;

	/** computes the score for a position update to a session 
	* If SEND_OOB or SEND_RELIABLE, they will be sent immediately.
	* If ENQEUE_SEND, it will be sent if bw allows (selected by distance)
	* Otherwise, wont be sent
	*/
	virtual SEND_SCORE SendScore(const VuSessionEntity *s, VU_TIME timeDelta) = 0;

	/** time we sent a positional update in MP for this driver (real time) */
	VU_TIME updateSentRealTime_;
	/** position values we sent last time, used for checking tolerance values */
	BIG_SCALAR xsent_, ysent_, zsent_;
	/** position changes sent */
	SM_SCALAR dxsent_, dysent_, dzsent_;
	/** turn values sent in last update */
	SM_SCALAR yawsent_, pitchsent_, rollsent_;
};


#endif // _VUDRIVER_H_
