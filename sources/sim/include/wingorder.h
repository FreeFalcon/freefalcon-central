#ifndef _WINGORDER_H
#define _WINGORDER_H

#include "aircrft.h"
#include "msginc\wingmanmsg.h"
#include "msginc\radiochattermsg.h"


#define CALLSIGN_NUM_OFFSET 36


enum AiPosition {
	AiFlightLead,
	AiFirstWing,
	AiElementLead,
	AiSecondWing,
	AiTotalPositions
};

enum AiExtent {
	AiWingman,
	AiElement,
	AiFlight,
	AiPackage,
	AiLeader,
	AiAllButSender,
	AiNoExtent,
	AiTotalExtent
};

enum AiDesignate {
	AiTarget,
	AiGroup,
	AiNone,
	AiNoChange,
	AiTotatlDesignate
};

extern char* gpAiExtentStr[];


// The following can be called publicly

BOOL	AiIsFullResponse		( int, int ); 

void	AiCreateRadioMsgs		( SimBaseClass*, FalconRadioChatterMessage** );
void	AiCustomizeRadioMsg	( SimBaseClass* p_sender, int extent, FalconRadioChatterMessage** pp_radioMsgs, int command, VU_ID targetid);
void	AiFillCallsign			(SimBaseClass* p_sender, int extent, FalconRadioChatterMessage** pp_radioMsgs, BOOL fillCallName);
void	AiMakeRadioCall		( SimBaseClass*, int, int, VU_ID targetid );
void	AiMakeRadioResponse	( SimBaseClass* p_sender, int message, short* p_edata );
void	AiMakeCommandMsg		(SimBaseClass* p_sender, int command, int extent, VU_ID targetid);
void	AiSendCommand			(SimBaseClass* p_sender, int command, int extent, VU_ID targetid = FalconNullId);
void	AiSendPlayerCommand	( int, int, VU_ID targetid = FalconNullId );
void	AiRespondLongCallSign( AircraftClass* p_aircraft );
void	AiRespondShortCallSign( AircraftClass* p_aircraft );

VU_ID AiCheckForThreat		( AircraftClass* paircraft, char domain, int position, float* az = NULL );
VU_ID AiDesignateTarget		( AircraftClass* paircraft );

#endif