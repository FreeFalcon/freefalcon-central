#ifndef _ACMITAPE_H_
#define _ACMITAPE_H_

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "FalcMesg.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define		MAX_ENTITY_CAMS	50

#define ACMI_VERSION 2
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ACMITape;
class SimBaseClass;
class DrawableTrail;
class DrawableBSP;
class Drawable2D;
class RViewPoint;
class RenderOTW;
class DrawableTracer;
class SfxClass;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#define ACMI_LABEL_LEN 15

// this structure will hold the in-memory, Sim representation for the
// entity.  IOW base class, drawable objs, etc...
typedef struct 
{
	SimBaseClass *objBase;
	DrawableTrail *objTrail;

	int			flags;

	// object orientation
	float		x;
	float		y;
	float		z;
	float		yaw;
	float		pitch;
	float		roll;

	// average speed between 2 positions
	float		aveSpeed;
	float		aveTurnRate;
	float		aveTurnRadius;

	// for trails, the start and end times
	float		trailStartTime;
	float		trailEndTime;

	// missiles need engine glow drawables
	DrawableBSP  *objBsp1;
	DrawableBSP  *objBsp2;

	// for flare need a glowing sphere
	Drawable2D  *obj2d;

	// for wing tip trails
	int			  wtLength;
	DrawableTrail *wlTrail;
	DrawableTrail *wrTrail;

	// for features we may need an index to the lead component and
	// the slot # that was in the camp component list (for bridges, bases...)
	long		  leadIndex;
	int			  slot;

} SimTapeEntity;

/*
** This struct holds info necessary for handling active tracer events
*/
typedef struct
{
	float		x;
	float		y;
	float		z;
	float		dx;
	float		dy;
	float		dz;
	DrawableTracer *objTracer;
} TracerEventData;


/*
** Each active event will have one of these in a chain
*/
typedef struct _ActiveEvent
{
	long		eventType;
	long		index;
	float		time;
	float		timeEnd;
	void		*eventData;
	struct _ActiveEvent *next;
	struct _ActiveEvent *prev;
} ActiveEvent;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// These are the headers and data that are used internally for the .vhs format.
// These use offsets instead of pointers so that we can memory map them.
// All offsets are from the start of the file!!!.

////////////////////////////////////////////////////////////////////////////////
//
// Header for the tape file.

#pragma pack (push, pack1, 1)
typedef struct 
{
	long		fileID;
	long		fileSize;
	long		numEntities;
	long		numFeat;
	long 		entityBlockOffset;
	long 		featBlockOffset;
	long		numEntityPositions;
	long		timelineBlockOffset;
	long		firstEntEventOffset;
	long		firstGeneralEventOffset;
	long		firstEventTrailerOffset;
	long		firstTextEventOffset;
	long		firstFeatEventOffset;
	long		numEvents;
	long		numEntEvents;
	long		numTextEvents;
	long		numFeatEvents;
	float		startTime;
	float		totPlayTime;
	float 		todOffset;
} ACMITapeHeader;
#pragma pack (pop, pack1)

////////////////////////////////////////////////////////////////////////////////
//
// Entity data.

#pragma pack (push, pack1, 1)
typedef struct 
{
	long		uniqueID;
	long		type;
	long		count;
	long		flags;
	
		#define		ENTITY_FLAG_MISSILE			0x00000001
		#define		ENTITY_FLAG_FEATURE			0x00000002
		#define		ENTITY_FLAG_AIRCRAFT		0x00000004
		#define		ENTITY_FLAG_CHAFF			0x00000008
		#define		ENTITY_FLAG_FLARE			0x00000010

	// for features we may need an index to the lead component and
	// the slot # that was in the camp component list (for bridges, bases...)
	long		leadIndex;
	int			slot;
	int			specialFlags;


	// Offset from the start of the file to the start of my positional data.
	long 		firstPositionDataOffset;
	long 		firstEventDataOffset;

} ACMIEntityData;
#pragma pack (pop, pack1)

////////////////////////////////////////////////////////////////////////////////
//
// Entity position data.

// enum types for position
enum
{
	PosTypePos = 0,
	PosTypeSwitch,
	PosTypeDOF,
};

#pragma pack (push, pack1, 1)
typedef struct
{
	// Time stamp for the positional data
	float		time;
	BYTE		type;

	// dereference based on type
	union
	{
		// Positional data.
		struct posTag
		{
			float		x;
			float		y;
			float		z;
			float		pitch;
			float		roll;
			float		yaw;
			long	    radarTarget;
		} posData;
		// switch change
		struct switchTag
		{
			int			switchNum;
			int			switchVal;
			int			prevSwitchVal;
		} switchData;
		// DOF change
		struct dofTag
		{
			int			DOFNum;
			float		DOFVal;
			float		prevDOFVal;
		} dofData;
	};

	// Although position data is a fixed size, we still want
	// this so that we can organize the data to be friendly for
	// paging.
	long		nextPositionUpdateOffset;
	long		prevPositionUpdateOffset;
} ACMIEntityPositionData;
#pragma pack (pop, pack1)

//
// This raw format is used by the position/event/sfx bundler to
// create a .vhs file (dig that extension), which is the ACMITape playback format.
// This is the format stored in the flight file.

typedef struct 
{
	int			type;			// type of object
	long		uniqueID;		// A unique ID for the object. Many to One correlation to Falcon Entities
	int			flags;			// side

	// for features we may need an index to the lead component and
	// the slot # that was in the camp component list (for bridges, bases...)
	long		leadIndex;
	int			slot;
	int			specialFlags;
	ACMIEntityPositionData entityPosData;
} ACMIRawPositionData;

////////////////////////////////////////////////////////////////////////////////
//
// Header for event data.

#pragma pack (push, pack1, 1)
typedef struct
{
	// type of event this is
	BYTE		eventType;
	long 		index;

	// Time stamp for this event.
	float		time;
	float		timeEnd;

	// data specific to type of event
	long		type;
	long		user;
	long		flags;
	float		scale;
	float		x, y, z;
	float		dx, dy, dz;
	float		roll, pitch, yaw;

} ACMIEventHeader;
#pragma pack (pop, pack1)

//
// Trailer for event data.
//

#pragma pack (push, pack1, 1)
typedef struct
{
	float		timeEnd;
	long 		index;		// into EventHeader
} ACMIEventTrailer;
#pragma pack (pop, pack1)

////////////////////////////////////////////////////////////////////////////////
//
// Feature Status Event

#pragma pack (push, pack1, 1)
typedef struct
{
	// Time stamp for this event.
	float		time;

	// index of feature on tape
	long 		index;

	// data specific to type of event
	long		newStatus;
	long		prevStatus;

} ACMIFeatEvent;
#pragma pack (pop, pack1)

typedef struct
{
	long		uniqueID;
	ACMIFeatEvent data;
} ACMIFeatEventImportData;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// .vhs file format:
//
// |                        |                 |                 |
// |        header          | entity block    | timeline block  |
// | sizeof(ACMITapeHeader) | (variable size) | (variable size) |
//
////////////////////////////////////////////////////////////////////////////////
//
// entity block:
//
// |                    |                                       |
// | number of entities |              entities                 |    
// |  sizeof(long)      | num entities * sizeof(ACMIEntityData) |
//
// entity:
//
// |                        |
// |      ACMIEntityData    |
// | sizeof(ACMIEntityData) |
//
////////////////////////////////////////////////////////////////////////////////
//
// timeline block:
//
// |                              |                    |                     |
// | entity position update block | entity event block | general event block |   
// |     (variable size)          |  (variable size)   |    (variable size)  |
//
// The entity position update block contains all entity position updates.
// The position updates are threaded on a per-entity basis, with a separate doubly linked list
// for each entity.
// The position updates should be chronologically sorted.
// There should be a position update read-head for each entity for traversing its linked list
// of position updates.
//
// The entity event block contains all events which are relevant to entities.
// The events are threaded on a per-entity basis, with a separate doubly linked list
// for each entity.
// The events should be chronologically sorted.  
// There should be an event read-head for each entity for traversing its linked list of events.
//
// The general event block contains all events which are not relevant to a specific entity.
// The events are threaded in doubly linked list.
// The events should be chronologically sorted.
// There should be an event read-head for traversing the linked list of events.
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
	void DestroyACMIRawPositionDataList(LIST* _frameList);
	void DeleteACMIRawPositionData(ACMIRawPositionData* rawPositionData);
	void DeleteACMIEntityPositionData(ACMIEntityPositionData *data);
	void DeleteACMIEntityData(ACMIEntityData *data);
	void DeleteACMIEventHeader(ACMIEventHeader *data);
	void DeleteACMIFeatEventImportData(ACMIFeatEventImportData *data);
	int CompareEventTrailer( const void *t1, const void *t2 );
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Callback for events that are not entity-specific.
// The EventIdData parameter is the event id.
// The first void * parameter points to the event data, which
// can be decoded with the event id.
// The second void * parameter is for user data.

typedef void (*ACMI_GENERAL_EVENT_CALLBACK) (ACMITape *, EventIdData, void *, void *);

typedef struct
{
	ACMI_GENERAL_EVENT_CALLBACK		forwardCallback;
	ACMI_GENERAL_EVENT_CALLBACK		reverseCallback;
	void									*userData;
} ACMIGeneralEventCallback;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct
{

	long			positionDataOffset;
	long			eventDataOffset;
} ACMIEntityReadHead;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ACMITape
{
public:
	
	// Constructors.
	// Do not put the extension with name.
	// This should be the name of the desired .vcr file.
	ACMITape(char *name, RenderOTW *renderer, RViewPoint *viewPoint);

	// Destructor.
	~ACMITape();

	// Import the current positional, event, and sfx data.
	// The filenames of these files will always be the same 
	// so we don't have to pass them in.
	static BOOL Import(char *inFltFile, char *outTapeFileName);
	static void WriteTapeFile ( char *fname, ACMITapeHeader *tapeHdr );
	
	// Time-independent entity access.
	int NumEntities() ;
	int EntityId(int index);
	int EntityType(int index);

	// Time-dependent entity access.
	BOOL GetEntityPosition
	(
		int index,
		float &x,
		float &y,
		float &z,
		float &yaw,
		float &pitch,
		float &roll,
		float &speed,
		float &turnrate,
		float &turnradius
	);


	// Prototype of an ACMI_GENERAL_EVENT_CALLBACK:
	// void PoopooCB(ACMITape *tape, EventIdData id, void *eventData, void *userData);
	void SetGeneralEventCallbacks
	(
		ACMI_GENERAL_EVENT_CALLBACK forwardEventCallback,
		ACMI_GENERAL_EVENT_CALLBACK reverseEventCallback,
		void *userData
	);

	// Was the tape file found and opened successfully?
	BOOL IsLoaded();

	// Is the tape paused?
	BOOL IsPaused() ;

	// Playback controls.
	void Play();
	void Pause();
	
	// Step in sim time.
	void StepTime(float numSeconds);

	// return a sim time based on pct into tape parm is
	float GetNewSimTime( float pct );
	
	// Play speed controls.
	// This is a ratio of sim time / real time.
	void SetPlayVelocity(float n);
	float PlayVelocity() ;

	// Increase in play velocity per second of real time.
	void SetPlayAcceleration(float n);
	float PlayAcceleration() ;

	// This will be used to clamp play velocity.
	// It will be clamped to (-fabs(speed) <= velocity <= fabs(speed));
	void SetMaxPlaySpeed(float n);
	float MaxPlaySpeed() ;

	// Set the read head position.  This should be a number 
	// from 0 to 1 (0 = beginning of tape, 1 = end of tape).
	// The input value will be clamped to fit this range.
	void SetHeadPosition(float t);
	float HeadPosition() ;

	// This gives the current simulation time.
	float SimTime() ;
	float GetTapePercent() ;

	void Update( float newSimTime );

	// YPR interpolation
	float AngleInterp( float begAng, float endAng, float dT );

	// access function for sim tape entity
	SimTapeEntity *GetSimTapeEntity( int index );

	// does entity exist at current read head
	BOOL IsEntityInFrame( int index );
	void InsertEntityInFrame( int index );
	void RemoveEntityFromFrame( int index );

	// get the entity's current radar target (entity index returned)
	int GetEntityCurrentTarget( int index );

	// update sim tape Entities for this frame
	void UpdateSimTapeEntities( void );

	// sets the draw position and matrix for bsp update
	void ObjectSetData(SimBaseClass*, Tpoint*, Trotation*);

	void SetScreenCapturing( BOOL val )
	{
		_screenCapturing = val;
	};

	void SetObjScale( float val )
	{
		_tapeObjScale = val;
	};

	float GetObjScale( void )
	{
		return _tapeObjScale;
	};

	float GetDeltaSimTime( void )
	{
		return _deltaSimTime;
	};

	void SetWingTrails( BOOL val );

	void SetWingTrailLength( int val )
	{
		_wtMaxLength = val;
	};

	void * GetTextEvents( int *count );
	void * GetCallsignList(long *count);


	// list of sim entities from the tape that are manipulated and drawn
	SimTapeEntity						*_simTapeEntities;
	ACMIEntityData *EntityData(int index);

	float GetTodOffset( void )
	{
		return _tapeHdr.todOffset;
	};
	
private:

	void Init();

	// These are used for importation.
	static void ParseEntities ( void );
	static void ThreadEntityPositions( ACMITapeHeader *tapeHdr );
	static void ThreadEntityEvents( ACMITapeHeader *tapeHdr );
	static void ImportTextEventList( FILE *fd, ACMITapeHeader *tapeHdr );

	// Get at the entity data.
	ACMIEntityData *FeatureData(int index);
	ACMIEntityPositionData *CurrentFeaturePositionHead(int i);

	// Traverse an entity's position update thread.
	ACMIEntityPositionData *CurrentEntityPositionHead(int i);
	ACMIEntityPositionData *CurrentEntityEventHead(int i);
	ACMIEntityPositionData *HeadNext(ACMIEntityPositionData *current);
	ACMIEntityPositionData *HeadPrev(ACMIEntityPositionData *current);

	// Traverse an event thread.
	ACMIEventHeader *GeneralEventData(void);
	ACMIEventHeader *GetGeneralEventData(int i);
	ACMIEventHeader *Next(ACMIEventHeader *current);
	ACMIEventHeader *Prev(ACMIEventHeader *current);

	ACMIEventTrailer *GeneralEventTrailer(void);
	ACMIEventTrailer *Next(ACMIEventTrailer *current);
	ACMIEventTrailer *Prev(ACMIEventTrailer *current);

	ACMIFeatEvent *CurrFeatEvent(void);
	ACMIFeatEvent *Next(ACMIFeatEvent *current);
	ACMIFeatEvent *Prev(ACMIFeatEvent *current);

	// Advance heads to current sim time.
	void AdvanceEntityPositionHead(int index);
	void AdvanceEntityEventHead(int index);
	void AdvanceGeneralEventHead( void );
	void AdvanceGeneralEventHeadHeader( void );
	void AdvanceGeneralEventHeadTrailer( void );
	void AdvanceFeatEventHead( void );
	void AdvanceAllHeads( void );

	// Entity setup and cleanup
	void SetupSimTapeEntities( void );
	void CleanupSimTapeEntities( void );

	// open the tape file and setup memory mapping
	long OpenTapeFile( char *fname ); // returns tape length
	void CloseTapeFile( void );

	// event list related functions
	void CleanupEventList( void );
	ActiveEvent *InsertActiveEvent( ACMIEventHeader *, float dT );
	void RemoveActiveEvent( ActiveEvent ** );
	void UpdateActiveEvents( void );

	// create/update feature drawables
	void CreateFeatureDrawable( SimTapeEntity *feat );
	SimBaseClass *FindComponentFeature( long leadIndex, int slot );

	// update tracer data
	void UpdateTracerEvent( TracerEventData *td, float dT );

	// tape header
	ACMITapeHeader						_tapeHdr;

	BOOL								_screenCapturing;

	// system info for tape file access
	HANDLE								_tapeFileHandle;
	HANDLE								_tapeMapHandle;

	// sim time / real time
	float								_playVelocity;
	float								_playAcceleration;
	float								_maxPlaySpeed;

	// Current sim time
	float								_simTime;
	float								_stepTrail;
	float								_deltaSimTime;

	// list of sim entities from the tape that are manipulated and drawn
	SimTapeEntity						*_simTapeFeatures;

	// viewpoint and renderer objs from acmiview
	RViewPoint							*_viewPoint;
	RenderOTW							*_renderer;

	// Current real time
	float								_lastRealTime;

	BOOL									_simulateOnly;
	BOOL									_paused;
	BOOL									_unpause;

	BOOL									_wingTrails;
	int										_wtMaxLength;

	// Base memory address of the file mapping
	// for the tape data.
	void									*_tape;
	ACMIEntityReadHead				*_entityReadHeads;
	long							_generalEventReadHeadHeader;
	ACMIEventTrailer				*_generalEventReadHeadTrailer;
	ACMIGeneralEventCallback		_generalEventCallbacks;
	ACMIFeatEvent					*_featEventReadHead;

	// events
	ActiveEvent						**_eventList;
	ActiveEvent						*_activeEventHead;
	ACMIEventTrailer				*_firstEventTrailer;
	ACMIEventTrailer				*_lastEventTrailer;
	ACMIFeatEvent					*_firstFeatEvent;
	ACMIFeatEvent					*_lastFeatEvent;

	// for scaling objects
	float							_tapeObjScale;
};


#include "acmtpinl.cpp"

#endif  // _ACMITAPE_H_

