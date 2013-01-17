/***************************************************************************\
    VehRWR.h

    This provides the basic functionality of a Radar Warning Receiver.
	This class is used directly by AI aircraft.  The player's RWR
	derives from this class.
\***************************************************************************/
#ifndef _VEHRWR_MODEL_H
#define _VEHRWR_MODEL_H

#include "rwr.h"
struct RadarDataType;

class VehRwrClass : public RwrClass
{
  public:
	VehRwrClass (int idx, SimMoverClass* self);
	virtual ~VehRwrClass (void);

	virtual SimObjectType* Exec (SimObjectType* targetList);
	virtual void Display (VirtualDisplay*) {};

	virtual void SetPower (BOOL flag);
	virtual int ObjectDetected (FalconEntity*, int trackType, int radarMode = 0); // 2002-02-09 MODIFIED BY S.G. Added the radarMode var

	int  LowAltPriority (void)	{return lowAltPriority;};
	int  AutoDrop (void)		{return dropPattern;};
	
	// State Access Functions
	FalconEntity* CurSpike (FalconEntity *byHim = NULL, int *data = NULL);  // 2002-02-10 MODIFIED BY S.G. Added 'byHim' and 'data' which defaults to NULL if not passed

  protected:
	enum {None, Diamond, MissileLaunch, MissileActivity, NewDetection, Hat}; // JB 010727 added NewDetection MI added 010812 Hat
	enum {MaxRWRTracks = 16, PriorityContacts = 5};
/* S.G. I NEED IT */ public:	typedef struct DetectListElement	
	{
		FalconEntity*		entity;
		RadarDataType*		radarData;
		float				bearing;
		VU_TIME		lastHit;
		VU_TIME		lastPlayed;				// When was audio last played? (for player search tones)
		unsigned long		isLocked		 :  1,	// Is locked on us
							isAGLocked		 :  1, //Something to use for SAMs and DIGIs
							missileActivity  :  1,	// Is launching on us (active OR beam rider)
							missileLaunch    :  1,	// Is launching on us (active missile)
							newDetection     :  1,	// Have we played audio for this new hit yet? (only player cares)
							newDetectionDisplay : 1, // JB 010727
							selected		 :  1,	// Used to mark the "selected" emitter for playing audio
							previouslyLocked :  1,	// Used to decide about making budy spike calls
// JB 010727 RP5 RWR
// 2001-02-15 ADDED BY S.G. SO WHEN CAN'T PAINT, IT DOESN'T PLAY THE SOUND...
							cantPlay		 :  1,	// Used to decide if the radar can trigger a 'paint' signal on the RWR
// 2001-02-17 ADDED BY S.G. SO WHEN I CLICK 'Handoff' THE SOUND IS PLAYED NO MATTER WHAT
							playIt			 :  1;	// Used to decide if the radar can trigger a 'paint' signal on the RWR
		
		float				lethality;
		int					radarMode;				// 2002-02-09 ADDED BY S.G. The mode of the radar pinging us. Digis are too dumb at figuring this out from ping interval so they need a little help :-)
	} DetectListElement;
  protected:	
	// Data Elements
	DetectListElement detectionList[MaxRWRTracks];
	int  numContacts;
	int  lowAltPriority;	// TRUE means low altitude threats are considered more lethal
	char dropPattern;		// 0 = manual chaff/flare drop pattern, 1 = auto chaff/flare drop pattern
	
	// Helper functions
/* S.G. I NEED IT */ public:	DetectListElement* IsTracked (FalconEntity* object); /* S.G. */ protected:
	DetectListElement* AddTrack (FalconEntity* object, float lethality);
	void	DropTrack (int trackNum);
	void  SortDetectionList(void); // JB 010718
	void	ResortList (DetectListElement*);
	void	ReportBuddySpike (FalconEntity* theObject);
	virtual float	GetLethality (FalconEntity* theObject);
  public:
};

#endif

