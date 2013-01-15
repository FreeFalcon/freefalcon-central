/*
 * Machine Generated include file for message "Radio Chatter Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 20-November-1997 at 16:32:51
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _RADIOCHATTERMSG_H
#define _RADIOCHATTERMSG_H

#ifndef FALCSESS_H
#include "FalcSess.h"
#endif

#ifndef _AIRCRAFT_CLASS_H
#include "aircrft.h"
#endif
/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "falcsnd\conv.h"
#include "FalcMesg.h"

#pragma pack (1)

#define MESSAGE_FOR_WORLD		0
#define MESSAGE_FOR_TEAM		1
#define MESSAGE_FOR_PACKAGE		2
#define MESSAGE_FOR_FLIGHT		3
#define MESSAGE_FOR_AIRCRAFT	4
#define MESSAGE_FOR_ENEMY		5

extern uchar GetDefaultAwacsVoice();
extern void ResetDefaultAwacsVoice();
extern short gDefaultAWACSCallSign;
extern short gDefaultAWACSFlightNum;

#define MAX_EVALS_PER_RADIO_MESSAGE		10
extern float MAX_RADIO_RANGE;	// Radio range, in feet (300nm)(maximum range at which you hear any calls)

/*
 * Message Type Radio Chatter Message
 */
class FalconRadioChatterMessage : public FalconEvent
{
   public:
      FalconRadioChatterMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconRadioChatterMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconRadioChatterMessage(void);

      virtual int Size() const;
		//sfr: changed to long *
	  virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      struct DATA_BLOCK
      {
		  VU_ID	from;
		  ulong	time_to_play;		// Time to play (from now)
		  uchar	to;
		  uchar	voice_id;
		  short	message;
		  short	edata[MAX_EVALS_PER_RADIO_MESSAGE];
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

// ==============================
// Conversion functions
// ==============================

extern short ConvertFlightNumberToCallNumber (int flight_num);
extern short ConvertWingNumberToCallNumber (int wing_num);
extern short ConvertToCallNumber (int flight_num, int wing_num);

// This will send a simple call from ATC (ie: a call with only the callsigns as evals)
FalconRadioChatterMessage* CreateCallFromATC (Objective airbase, AircraftClass* aircraft, short call, VuTargetEntity *target = FalconLocalSession);
void SendCallFromATC (Objective airbase, AircraftClass* aircraft, short call, VuTargetEntity *target = FalconLocalSession);

// This will send a simple call to ATC (ie: a call with only the callsigns as evals)
FalconRadioChatterMessage* CreateCallToATC (AircraftClass* aircraft, short call, VuTargetEntity *target = FalconLocalSession);
void SendCallToATC (AircraftClass* aircraft, short call, VuTargetEntity *target = FalconLocalSession);

// This will send a simple call to ATC (ie: a call with only the callsigns as evals)
void SendCallToATC (AircraftClass* aircraft, VU_ID airbaseID, short call, VuTargetEntity *target);
FalconRadioChatterMessage* CreateCallToATC (AircraftClass* aircraft, VU_ID airbaseID, short call, VuTargetEntity *target);

// This will send a simple call to AWACS/FAC (ie: a call with only the callsigns as evals)
extern void SendCallToAWACS (AircraftClass* aircraft, short call, VuTargetEntity *target = FalconLocalSession);
extern FalconRadioChatterMessage* CreateCallToAWACS (AircraftClass* aircraft, short call, VuTargetEntity *target = FalconLocalSession);

// This will send a simple call to AWACS/FAC (ie: a call with only the callsigns as evals)
extern void SendCallToAWACS (Flight flight, short call, VuTargetEntity *target = FalconLocalSession);
extern FalconRadioChatterMessage* CreateCallToAWACS (Flight flight, short call, VuTargetEntity *target = FalconLocalSession);

// This will send a simple call FROM AWACS/FAC (ie: a call with only the callsigns as evals)
extern void SendCallFromAwacs (Flight flight, short call, VuTargetEntity *target = FalconLocalSession);
extern FalconRadioChatterMessage* CreateCallFromAwacs (Flight flight, short call, VuTargetEntity *target = FalconLocalSession);
extern FalconRadioChatterMessage* CreateCallFromAwacsPlane (AircraftClass* plane, short call, VuTargetEntity *target = FalconLocalSession);

// This will send a simple call FROM from (ie: a call with only the callsigns as evals)
void SendCallToPlane (AircraftClass* aircraft, FalconEntity *from, short call, VuTargetEntity *target = FalconLocalSession);
FalconRadioChatterMessage* CreateCallToPlane (AircraftClass* aircraft, FalconEntity *from, short call, VuTargetEntity *target = FalconLocalSession);

void SendRogerToPlane (AircraftClass* aircraft, FalconEntity *from, VuTargetEntity *target = FalconLocalSession);

// This will send a simple call FROM from (ie: a call with only the callsigns as evals)
void SendCallToFlight (Flight flight, FalconEntity *from, short call, VuTargetEntity *target = FalconLocalSession);
FalconRadioChatterMessage* CreateCallToFlight (Flight flight, FalconEntity *from, short call, VuTargetEntity *target = FalconLocalSession);

FalconRadioChatterMessage* CreateCallToWing (Flight flight, int from_position, int to_position, short call, VuTargetEntity *target = FalconLocalSession);
void SendCallToWing (Flight flight, int from_position, int to_position, short call, VuTargetEntity *target = FalconLocalSession);


#pragma pack ()

#endif
