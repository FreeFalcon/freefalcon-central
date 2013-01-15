#pragma optimize( "", off )

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <tchar.h>

#include "Graphics\Include\Setup.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawtrcr.h"
#include "Graphics\Include\rViewpnt.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\drawBSP.h"
#include "Graphics\Include\drawpole.h"
#include "Graphics\Include\drawplat.h"
#include "Graphics\Include\drawbrdg.h"
#include "Graphics\Include\renderow.h"
#include "falclib.h"
#include "fsound.h"
#include "Graphics\Include\grTypes.h"									

#include "codelib\tools\lists\lists.h"
#include "debuggr.h"
#include "AcmiTape.h"
#include "sim\include\misctemp.h"		// for Clamp function.
#include "sim\include\simbase.h"
#include "sim\include\otwdrive.h"
#include "sim\include\sfx.h"
#include "acmirec.h"
#include "sim\include\simfeat.h"
#include "Campaign\include\CmpGlobl.h"
#include "Campaign\include\evtparse.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "ui\include\events.h"
#include "f4vu.h"
#include "feature.h"
#include "campstr.h"
#include "team.h"
#include "acmihash.h"

//////////////////////////////////////////////////////////////////////////
/// 3-23 BING
#include "AcmiView.h"
#include "AcmiUI.h"
				
extern ACMIView			*acmiView;


long tempTarget; // for missile lock.
				

//////////////////////////////////////////////////////////////////////////


void CalcTransformMatrix(SimBaseClass* theObject);
void CreateDrawable (SimBaseClass* theObject, float objectScale);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// these are for raw data import
LIST *importEntityList;
LIST *importFeatList;
LIST *importPosList;
LIST *importEventList;
LIST *importEntEventList;
LIST *importEntityListEnd;
LIST *importFeatListEnd;
LIST *importPosListEnd;
LIST *importEventListEnd;
LIST *importEntEventListEnd;
LIST *importFeatEventList;
LIST *importFeatEventListEnd;
int importNumPos;
int importNumEnt;
int importNumFeat;
int importNumFeatEvents;
int importNumEvents;
int importNumEntEvents;
int importEntOffset;
int importFeatOffset;
int importFeatEventOffset;
int importPosOffset;
int importEventOffset;
int importEntEventOffset;
ACMIEventTrailer *importEventTrailerList;

extern long TeamSimColorList[NUM_TEAMS];

LIST * AppendToEndOfList( LIST * list, LIST **end, void * node );
void DestroyTheList( LIST * list );
extern float CalcKIAS( float, float );

ACMI_CallRec *ACMI_Callsigns=NULL;
ACMI_CallRec *Import_Callsigns=NULL;
long import_count=0;

//extern GLOBAL_SPEED;
//extern GLOBAL_ALTITUDE;
//extern GLOBAL_HEADING;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DefaultForwardACMIGeneralEventCallback
(
	ACMITape *,
	EventIdData eventId,
	void *,
	void *
)
{
	MonoPrint
	(
		"General event occured in forward ACMI Tape play --> event type: %d.\n",
		eventId.type
	);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DefaultReverseACMIGeneralEventCallback
(
	ACMITape *,
	EventIdData eventId,
	void *,
	void *
)
{
	MonoPrint
	(
		"General event occured in reverse ACMI Tape play --> event type: %d.\n",
		eventId.type
	);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
** Callback compare function from qsort.
*/
int CompareEventTrailer( const void *p1, const void *p2 )
{
	ACMIEventTrailer *t1 = (ACMIEventTrailer *)p1;
	ACMIEventTrailer *t2 = (ACMIEventTrailer *)p2;

	if ( t1->timeEnd < t2->timeEnd )
		return -1;
	else if ( t1->timeEnd > t2->timeEnd )
		return 1;
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DestroyACMIRawPositionDataList(LIST *list)
{
	// LIST_DESTROY (list, (PFV)DeleteACMIRawPositionData);
	DestroyTheList (list);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DeleteACMIRawPositionData(ACMIRawPositionData* rawPositionData)
{
	delete rawPositionData;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DeleteACMIEntityData(ACMIEntityData *data)
{
	delete data;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DeleteACMIEventHeader(ACMIEventHeader *data)
{
	delete data;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DeleteACMIEntityPositionData(ACMIEntityPositionData *data)
{
	delete data;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void DeleteACMIFeatEventImportData(ACMIFeatEventImportData *data)
{
	delete data;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ACMITape::ACMITape(char *name, RenderOTW *renderer, RViewPoint *viewPoint )
{
	int i, numEntities;
	char fullName[MAX_PATH];
	ACMIEntityData *e;
	long length=0;
	char *callsigns=NULL;
	long numcalls=0;


	// initialize storage for drawable poled objects
	#ifdef USE_SH_POOLS
	DrawablePoled::InitializeStorage();
	#endif

	F4Assert(name != NULL);

	_tape = NULL;
	_entityReadHeads = NULL;
	_simTapeEntities = NULL;
	_simTapeFeatures = NULL;
	_activeEventHead = NULL;
	_eventList = NULL;
	_screenCapturing = FALSE;
	_wingTrails = FALSE;
	_tapeObjScale = 1.0f;

	// set our render and viewpoint
	_renderer = renderer;
	_viewPoint = viewPoint;
	
	Init();

	// Open up a map file with the given name.

	// edg note on hack: right now, ALWAYS do an import from the acmi.flt
	// file to convert to a tape file.  Later we'll probably want to import
	// right after an ACMIU record session to get into .vhs format
	//strcpy( fullName, "campaign\\save\\fltfiles\\" );
	strcpy( fullName, "acmibin\\" );
	strcat( fullName, name );

	// commented out if statement for quick testing....
 	// if ( Import( fullName ) )
	{
		// create the memory mapping
		length=OpenTapeFile( fullName );

		// just test
		if ( IsLoaded() )
		{
			numEntities = NumEntities();

			for ( i= 0; i < numEntities; i++ )
			{
				e = EntityData( i );
				MonoPrint( "Entity %d: Type = %d, Id = %d, Offset = %d\n",
							i,
							e->type,
							e->uniqueID,
							e->firstPositionDataOffset );
	
			
			}

			// CloseTapeFile();
		}
		else
		{
			MonoPrint( "Unable to test memory mapped tape file\n" );
		}
	}

	// If it loaded, do any additional setup.
	if(IsLoaded())
	{
		// Setup Callsigns...
		callsigns=(char*)GetCallsignList(&numcalls);
		if(((char *)callsigns - (char *)_tape) < length && numcalls > 0) // there are callsigns...
		{
			ACMI_Callsigns=new ACMI_CallRec[numcalls];
			memcpy(ACMI_Callsigns,callsigns,sizeof(ACMI_CallRec)*numcalls);
		}

		numEntities = NumEntities();

		// Setup entity event callbacks. and read heads
		_entityReadHeads = new ACMIEntityReadHead[numEntities];
		F4Assert(_entityReadHeads != NULL);

		for(i = 0; i < numEntities; i++)
		{
			// set the read heads to the first position
			e = EntityData( i );
			_entityReadHeads[i].positionDataOffset = e->firstPositionDataOffset;
			_entityReadHeads[i].eventDataOffset = e->firstEventDataOffset;
		}

		// Setup general event callbacks.
		SetGeneralEventCallbacks
		(
			DefaultForwardACMIGeneralEventCallback,
			DefaultReverseACMIGeneralEventCallback,
			NULL
		);

		// setup the sim tape entities
		SetupSimTapeEntities();

		// create an array of ActiveEvent pointers -- 1 for every event
		_eventList = new ActiveEvent * [ _tapeHdr.numEvents ];
		// make sure they're null
		memset( _eventList, 0, sizeof( ActiveEvent * ) * _tapeHdr.numEvents );

		// set the first and last event trailer pointers
		if ( _tapeHdr.numEvents == 0 )
		{
			_firstEventTrailer = NULL;
			_lastEventTrailer = NULL;
		}
		else
		{
			_firstEventTrailer = (ACMIEventTrailer *)( (char *)_tape + _tapeHdr.firstEventTrailerOffset );
			_lastEventTrailer = _firstEventTrailer + (_tapeHdr.numEvents - 1);
		}

		_generalEventReadHeadTrailer = _firstEventTrailer;

		if ( _tapeHdr.numFeatEvents == 0 )
		{
			_firstFeatEvent = NULL;
			_lastFeatEvent = NULL;
		}
		else
		{
			_firstFeatEvent = (ACMIFeatEvent *)( (char *)_tape + _tapeHdr.firstFeatEventOffset );
			_lastFeatEvent = _firstFeatEvent + (_tapeHdr.numFeatEvents - 1);
		}

		_featEventReadHead = _firstFeatEvent;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ACMITape::~ACMITape()
{
	// Delete Callsigns
	if(ACMI_Callsigns)
	{
		delete ACMI_Callsigns;
		ACMI_Callsigns=NULL;
	}
	Init();

	#ifdef USE_SH_POOLS
	DrawablePoled::ReleaseStorage();
	#endif
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::Init()
{

	if(_entityReadHeads)
	{
		delete [] _entityReadHeads;
		_entityReadHeads = NULL;
	}

	if(_simTapeEntities)
	{
		CleanupSimTapeEntities();
	}

	if ( _eventList )
	{
		CleanupEventList( );
	}

	SetGeneralEventCallbacks
	(
		NULL,
		NULL,
		NULL
	);

	if(_tape)
	{
		// close file mapping.
		CloseTapeFile();
	}

	_playVelocity = 0.0;
	_playAcceleration = 0.0;
	_maxPlaySpeed = 4.0;	

	_simTime = 0.0;
	_stepTrail = 0.0;

	_lastRealTime = 0.0;


	_unpause = FALSE;
	_paused = TRUE;
	_simulateOnly = FALSE;

	_generalEventReadHeadHeader = 0;
	_featEventReadHead = NULL;
	_generalEventReadHeadTrailer = NULL;


}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BOOL ReadRawACMIPositionData
(
	FILE *flightFile,
	ACMIRawPositionData &rawPositionData
)
{
	int
		result;

	fscanf
	(
		flightFile,
		"%d %d",
		&rawPositionData.type,
		&rawPositionData.uniqueID
	);

	
	// We don't need to check the status of our last two fscanf calls, because
	// if they fail, this one will too.
	result = fscanf
	(
		flightFile,
		"%f %f %f %f %f %f\n",
		&rawPositionData.entityPosData.posData.x,
		&rawPositionData.entityPosData.posData.y,
		&rawPositionData.entityPosData.posData.z,
		&rawPositionData.entityPosData.posData.pitch,
		&rawPositionData.entityPosData.posData.roll,
		&rawPositionData.entityPosData.posData.yaw
	);

	// insure pitch roll and yaw are positive (edg:?)
	// or in 0 - 2PI range
	/* nah, this ain't right....  need to fix songy's stuff
	if ( rawPositionData.entityPosData.pitch < 0.0f )
		rawPositionData.entityPosData.pitch += 2.0f * PI;
	if ( rawPositionData.entityPosData.roll < 0.0f )
		rawPositionData.entityPosData.roll += 2.0f * PI;
	if ( rawPositionData.entityPosData.yaw < 0.0f )
		rawPositionData.entityPosData.yaw += 2.0f * PI;
	*/

	return (!result || result == EOF ? FALSE : TRUE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CleanupACMIImportPositionData
(
	FILE *flightFile,
	ACMIRawPositionData *rawPositionData
)
{
	if(flightFile != NULL)
	{
		fclose(flightFile);
	}

	if(rawPositionData != NULL)
	{
		delete rawPositionData;
	}

	if ( importEntityList != NULL )
	{
		DestroyTheList (importEntityList);
		importEntityList = NULL;
	}

	if ( importFeatList != NULL )
	{
		DestroyTheList (importFeatList);
		importFeatList = NULL;
	}

	if ( importPosList != NULL )
	{
		DestroyTheList (importPosList );
		importPosList = NULL;
	}

	if ( importEntEventList != NULL )
	{
		DestroyTheList (importEntEventList );
		importEntEventList = NULL;
	}

	if ( importEventList != NULL )
	{
		DestroyTheList (importEventList );
		importEventList = NULL;
	}

	if ( importEventTrailerList != NULL )
	{
		delete [] importEventTrailerList;
		importEventTrailerList = NULL;
	}

	if ( importFeatEventList != NULL )
	{
		DestroyTheList (importFeatEventList );
		importFeatEventList = NULL;
	}
	if(Import_Callsigns)
	{
		delete Import_Callsigns;
		Import_Callsigns=NULL;
		import_count=0;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BOOL ACMITape::Import(char *inFltFile, char *outTapeFileName)
{
	FILE
		*flightFile;

	ACMIRawPositionData
		*rawPositionData = NULL;

	ACMIEventHeader
		*ehdr = NULL;

	ACMIFeatEventImportData
		*fedata = NULL;

	float
		begTime,
		endTime;

	ACMITapeHeader tapeHdr;
	ACMIRecHeader  hdr;
	ACMIGenPositionData genpos;
	ACMIFeaturePositionData featpos;
	ACMITracerStartData tracer;
	ACMIStationarySfxData sfx;
	ACMIMovingSfxData msfx;
	ACMISwitchData sd;
	ACMIDOFData dd;
	ACMIFeatureStatusData fs;
		

	// zero our counters
	importNumFeat = 0;
	importNumPos = 0;
	importNumEnt = 0;
	importNumEvents = 0;
	importNumFeatEvents = 0;
	importNumEntEvents = 0;

	// zero out position list
	importFeatList = NULL;
	importFeatEventList = NULL;
	importPosList = NULL;
	importEventList = NULL;
	importEntEventList = NULL;
	importEventTrailerList = NULL;

	// this value comes from tod type record
	tapeHdr.todOffset =  0.0f;


	// Load flight file for positional data.
	//flightFile = fopen("campaign\\save\\fltfiles\\acmi.flt", "rb");
	flightFile = fopen(inFltFile, "rb");
						
	if (flightFile == NULL)
	{
		MonoPrint("Error opening acmi flight file");
		return FALSE;
	}

	begTime = -1.0;
	endTime = 0.0;
	MonoPrint("ACMITape Import: Reading Raw Data ....\n");
	while( fread(&hdr, sizeof( ACMIRecHeader ), 1, flightFile ) )
	{
		// now read in the rest of the record depending on type
		switch( hdr.type )
		{
			case ACMIRecTodOffset:
				tapeHdr.todOffset =  hdr.time;
				break;

			case ACMIRecGenPosition:
			case ACMIRecMissilePosition:
			case ACMIRecChaffPosition:
			case ACMIRecFlarePosition:
			case ACMIRecAircraftPosition:
		
				// Read the data
				if ( !fread( &genpos, sizeof( ACMIGenPositionData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}
				if (hdr.type == ACMIRecAircraftPosition)
					fread(&tempTarget, sizeof(tempTarget),1,flightFile);
				else
					tempTarget = -1;
				// Allocate a new data node.
				F4Assert(rawPositionData == NULL);
				rawPositionData = new ACMIRawPositionData;
				F4Assert(rawPositionData != NULL);
		
				// fill in raw position data
				rawPositionData->uniqueID = genpos.uniqueID;
				rawPositionData->type = genpos.type;
				if ( hdr.type == ACMIRecMissilePosition )
					rawPositionData->flags = ENTITY_FLAG_MISSILE;
				else if ( hdr.type == ACMIRecAircraftPosition )
					rawPositionData->flags = ENTITY_FLAG_AIRCRAFT;
				else if ( hdr.type == ACMIRecChaffPosition )
					rawPositionData->flags = ENTITY_FLAG_CHAFF;
				else if ( hdr.type == ACMIRecFlarePosition )
					rawPositionData->flags = ENTITY_FLAG_FLARE;
				else
					rawPositionData->flags = 0;

				rawPositionData->entityPosData.time = hdr.time;
				rawPositionData->entityPosData.type = PosTypePos;
// remove				rawPositionData->entityPosData.teamColor = genpos.teamColor;
// remove				strcpy((char*)rawPositionData->entityPosData.label, (char*)genpos.label);
				rawPositionData->entityPosData.posData.x = genpos.x;
				rawPositionData->entityPosData.posData.y = genpos.y;
				rawPositionData->entityPosData.posData.z = genpos.z;
				rawPositionData->entityPosData.posData.roll = genpos.roll;
				rawPositionData->entityPosData.posData.pitch = genpos.pitch;
				rawPositionData->entityPosData.posData.yaw = genpos.yaw;
				rawPositionData->entityPosData.posData.radarTarget= tempTarget;

																																
				// Append our new position data.
				importPosList = AppendToEndOfList(importPosList, &importPosListEnd, rawPositionData);
				rawPositionData = NULL;
		
				// bump counter
				importNumPos++;

				break;
			case ACMIRecTracerStart:

				// Read the data
				if ( !fread( &tracer, sizeof( ACMITracerStartData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(ehdr == NULL);
				ehdr = new ACMIEventHeader;
				F4Assert(ehdr != NULL);

				// fill in data
				ehdr->eventType = hdr.type;
				ehdr->time = hdr.time;
				ehdr->timeEnd = hdr.time + 2.5F;
				ehdr->index = importNumEvents;
				ehdr->x = tracer.x;
				ehdr->y = tracer.y;
				ehdr->z = tracer.z;
				ehdr->dx = tracer.dx;
				ehdr->dy = tracer.dy;
				ehdr->dz = tracer.dz;

				
				// Append our new data.
				importEventList = AppendToEndOfList(importEventList, &importEventListEnd, ehdr );
				ehdr = NULL;
		
				// bump counter
				importNumEvents++;
				break;
			case ACMIRecStationarySfx:
				// Read the data
				if ( !fread( &sfx, sizeof( ACMIStationarySfxData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(ehdr == NULL);
				ehdr = new ACMIEventHeader;
				F4Assert(ehdr != NULL);

				// fill in data
				ehdr->eventType = hdr.type;
				ehdr->index = importNumEvents;
				ehdr->time = hdr.time;
				ehdr->timeEnd = hdr.time + sfx.timeToLive;
				ehdr->x = sfx.x;
				ehdr->y = sfx.y;
				ehdr->z = sfx.z;
				ehdr->type = sfx.type;
				ehdr->scale = sfx.scale;

				
				// Append our new data.
				importEventList = AppendToEndOfList(importEventList, &importEventListEnd, ehdr );
				ehdr = NULL;
		
				// bump counter
				importNumEvents++;
				break;

			case ACMIRecFeatureStatus:
				// Read the data
				if ( !fread( &fs, sizeof( ACMIFeatureStatusData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(fedata == NULL);
				fedata = new ACMIFeatEventImportData;
				F4Assert(fedata != NULL);

				// fill in data
				fedata->uniqueID = fs.uniqueID;
				fedata->data.index = -1;	// will be filled in later
				fedata->data.time = hdr.time;
				fedata->data.newStatus = fs.newStatus;
				fedata->data.prevStatus = fs.prevStatus;

				
				// Append our new data.
				importFeatEventList = AppendToEndOfList(importFeatEventList, &importFeatEventListEnd, fedata );
				fedata = NULL;
		
				// bump counter
				importNumFeatEvents++;
				break;

			// not ready for these yet
			case ACMIRecMovingSfx:
				// Read the data
				if ( !fread( &msfx, sizeof( ACMIMovingSfxData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(ehdr == NULL);
				ehdr = new ACMIEventHeader;
				F4Assert(ehdr != NULL);

				// fill in data
				ehdr->eventType = hdr.type;
				ehdr->index = importNumEvents;
				ehdr->time = hdr.time;
				ehdr->timeEnd = hdr.time + msfx.timeToLive;
				ehdr->x = msfx.x;
				ehdr->y = msfx.y;
				ehdr->z = msfx.z;
				ehdr->dx = msfx.dx;
				ehdr->dy = msfx.dy;
				ehdr->dz = msfx.dz;
				ehdr->flags = msfx.flags;
				ehdr->user = msfx.user;
				ehdr->type = msfx.type;
				ehdr->scale = msfx.scale;

				
				// Append our new data.
				importEventList = AppendToEndOfList(importEventList, &importEventListEnd, ehdr );
				ehdr = NULL;
		
				// bump counter
				importNumEvents++;
				break;

			case ACMIRecSwitch:
		
				// Read the data
				if ( !fread( &sd, sizeof( ACMISwitchData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(rawPositionData == NULL);
				rawPositionData = new ACMIRawPositionData;
				F4Assert(rawPositionData != NULL);
		
				// fill in raw position data
				rawPositionData->uniqueID = sd.uniqueID;
				rawPositionData->type = sd.type;
				rawPositionData->flags = 0;


				rawPositionData->entityPosData.time = hdr.time;
				rawPositionData->entityPosData.type = PosTypeSwitch;
				rawPositionData->entityPosData.switchData.switchNum = sd.switchNum;
				rawPositionData->entityPosData.switchData.switchVal = sd.switchVal;
				rawPositionData->entityPosData.switchData.prevSwitchVal = sd.prevSwitchVal;
				
				// Append our new position data.
				importEntEventList = AppendToEndOfList(importEntEventList, &importEntEventListEnd, rawPositionData);
				rawPositionData = NULL;
		
				// bump counter
				importNumEntEvents++;

				break;

			case ACMIRecDOF:
		
				// Read the data
				if ( !fread( &dd, sizeof( ACMIDOFData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(rawPositionData == NULL);
				rawPositionData = new ACMIRawPositionData;
				F4Assert(rawPositionData != NULL);
		
				// fill in raw position data
				rawPositionData->uniqueID = dd.uniqueID;
				rawPositionData->type = dd.type;
				rawPositionData->flags = 0;


				rawPositionData->entityPosData.time = hdr.time;
				rawPositionData->entityPosData.type = PosTypeDOF;
				rawPositionData->entityPosData.dofData.DOFNum = dd.DOFNum;
				rawPositionData->entityPosData.dofData.DOFVal = dd.DOFVal;
				rawPositionData->entityPosData.dofData.prevDOFVal = dd.prevDOFVal;
				
				// Append our new position data.
				importEntEventList = AppendToEndOfList(importEntEventList, &importEntEventListEnd, rawPositionData);
				rawPositionData = NULL;
		
				// bump counter
				importNumEntEvents++;

				break;

			case ACMIRecFeaturePosition:
		
				// Read the data
				if ( !fread( &featpos, sizeof( ACMIFeaturePositionData ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				// Allocate a new data node.
				F4Assert(rawPositionData == NULL);
				rawPositionData = new ACMIRawPositionData;
				F4Assert(rawPositionData != NULL);
		
				// fill in raw position data
				rawPositionData->uniqueID = featpos.uniqueID;
				rawPositionData->leadIndex = featpos.leadUniqueID;
				rawPositionData->specialFlags = featpos.specialFlags;
				rawPositionData->slot = featpos.slot;
				rawPositionData->type = featpos.type;
				rawPositionData->flags = ENTITY_FLAG_FEATURE;

				rawPositionData->entityPosData.time = hdr.time;
				rawPositionData->entityPosData.type = PosTypePos;
				rawPositionData->entityPosData.posData.x = featpos.x;
				rawPositionData->entityPosData.posData.y = featpos.y;
				rawPositionData->entityPosData.posData.z = featpos.z;
				rawPositionData->entityPosData.posData.roll = featpos.roll;
				rawPositionData->entityPosData.posData.pitch = featpos.pitch;
				rawPositionData->entityPosData.posData.yaw = featpos.yaw;
				
				// Append our new position data.
				importPosList = AppendToEndOfList(importPosList, &importPosListEnd, rawPositionData);
				rawPositionData = NULL;
		
				// bump counter
				importNumPos++;

				break;
			case ACMICallsignList:
		
				// Read the data
				if ( !fread( &import_count, sizeof( long ), 1, flightFile ) )
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}

				F4Assert(Import_Callsigns == NULL);
				Import_Callsigns=new ACMI_CallRec[import_count];
				F4Assert(Import_Callsigns != NULL);

				if(!fread(Import_Callsigns,import_count * sizeof(ACMI_CallRec),1,flightFile))
				{
					CleanupACMIImportPositionData ( flightFile, rawPositionData );
					return FALSE;
				}
				break;

			default:
				// KCK: I was hitting this repeatidly.. So I'm making it a ShiAssert (and therefore ignorable)
//				ShiAssert(0);
				break;
		}

		// save begin and end times
		if ( hdr.type != ACMIRecTodOffset )
		{
			if ( begTime < 0.0 )
				begTime = hdr.time;
			if ( hdr.time > endTime )
				endTime = hdr.time;
		}
	}

	// build the importEntityList
	MonoPrint("ACMITape Import: Parsing Entities ....\n");
	ParseEntities();

	// setup the tape header
	tapeHdr.fileID = 'TAPE';
	tapeHdr.numEntities = importNumEnt;
	tapeHdr.numFeat = importNumFeat;
	tapeHdr.entityBlockOffset = sizeof( ACMITapeHeader );
	tapeHdr.featBlockOffset = tapeHdr.entityBlockOffset +
								  sizeof( ACMIEntityData ) * importNumEnt;
	tapeHdr.timelineBlockOffset = tapeHdr.featBlockOffset +
								  sizeof( ACMIEntityData ) * importNumFeat;
	tapeHdr.firstEntEventOffset = tapeHdr.timelineBlockOffset +
								  sizeof( ACMIEntityPositionData ) * importNumPos;
	tapeHdr.firstGeneralEventOffset = tapeHdr.firstEntEventOffset +
								  sizeof( ACMIEntityPositionData ) * importNumEntEvents;
	tapeHdr.firstEventTrailerOffset = tapeHdr.firstGeneralEventOffset +
								  sizeof( ACMIEventHeader ) * importNumEvents;
	tapeHdr.firstFeatEventOffset = tapeHdr.firstEventTrailerOffset +
								  sizeof( ACMIEventTrailer ) * importNumEvents;
	tapeHdr.firstTextEventOffset = tapeHdr.firstFeatEventOffset +
								  sizeof( ACMIFeatEvent ) * importNumFeatEvents;
	tapeHdr.numEntityPositions = importNumPos;
	tapeHdr.numEvents = importNumEvents;
	tapeHdr.numFeatEvents = importNumFeatEvents;
	tapeHdr.numEntEvents = importNumEntEvents;
	tapeHdr.totPlayTime = endTime - begTime;
	tapeHdr.startTime =  begTime;


	// set up the chain offsets of entity positions
	MonoPrint("ACMITape Import: Threading Positions ....\n");
	ThreadEntityPositions( &tapeHdr );

	// set up the chain offsets of entity events
	MonoPrint("ACMITape Import: Threading Entity Events ....\n");
	ThreadEntityEvents( &tapeHdr );

	// Calculate size of .vhs file.
	tapeHdr.fileSize = tapeHdr.timelineBlockOffset +
					   sizeof( ACMIEntityPositionData ) * importNumPos +
					   sizeof( ACMIEntityPositionData ) * importNumEntEvents +
					   sizeof( ACMIEventHeader ) * importNumEvents +
					   sizeof( ACMIFeatEvent ) * importNumFeatEvents +
					   sizeof( ACMIEventTrailer ) * importNumEvents;

	// Open a writecopy file mapping.
	// Write out file in .vhs format.
	MonoPrint("ACMITape Import: Writing Tape File ....\n");
	WriteTapeFile( outTapeFileName, &tapeHdr );

	// Cleanup import data.
	CleanupACMIImportPositionData ( flightFile, rawPositionData );

	// now delete the acmi.flt file
	//remove("campaign\\save\\fltfiles\\acmi.flt");
	remove(inFltFile);
				
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::ParseEntities ( void )
{
	int
		i = 0,
		count = 0;
	
	LIST			
		*entityPtr,
		*rawList;

	ACMIRawPositionData
		*entityType;

	ACMIEntityData
		*importEntityInfo;

	importEntityList = NULL;

	rawList = importPosList;
	for (count = 0; count < importNumPos; count++)
	{
		// rawList = LIST_NTH(importPosList, count);
		entityType = (ACMIRawPositionData *)rawList->node;

		if ( entityType->flags & ENTITY_FLAG_FEATURE )
		{
			// look for existing entity
			entityPtr = importFeatList;
			for (i = 0; i < importNumFeat; i++)
			{
				// entityPtr = LIST_NTH(importEntityList, i);
				importEntityInfo = ( ACMIEntityData * )entityPtr->node;
				if(entityType->uniqueID == importEntityInfo->uniqueID)
				{
					break;
				}
	
				entityPtr = entityPtr->next;
			}
	
			// create new import entity record
			if(i == importNumFeat)
			{
				importEntityInfo = new ACMIEntityData;
				importEntityInfo->count =0;

				F4Assert( importEntityInfo );
				importEntityInfo->uniqueID = entityType->uniqueID;
				importEntityInfo->type = entityType->type;
				importEntityInfo->flags = entityType->flags;
				importEntityInfo->leadIndex = entityType->leadIndex;
				importEntityInfo->specialFlags = entityType->specialFlags;
				importEntityInfo->slot = entityType->slot;
				importFeatList = AppendToEndOfList(importFeatList, &importFeatListEnd, importEntityInfo);
				importNumFeat++;
			}
		}
		else
		{
			// not a feature

			// look for existing entity
			entityPtr = importEntityList;

			for (i = 0; i < importNumEnt; i++)
			{
				// entityPtr = LIST_NTH(importEntityList, i);
				importEntityInfo = ( ACMIEntityData * )entityPtr->node;
				if(entityType->uniqueID == importEntityInfo->uniqueID)
				{
					break;
				}
	
				entityPtr = entityPtr->next;
			}
	
			// create new import entity record
			if(i == importNumEnt)
			{
				importEntityInfo = new ACMIEntityData;
				importEntityInfo->count =0;

				F4Assert( importEntityInfo );
				importEntityInfo->uniqueID = entityType->uniqueID;
				importEntityInfo->type = entityType->type;
				importEntityInfo->flags = entityType->flags;
// remove				importEntityInfo->teamColor = entityType->entityPosData.teamColor;
// remove				strcpy((importEntityInfo->label), (char*) entityType->entityPosData.label);

				importEntityList = AppendToEndOfList(importEntityList, &importEntityListEnd, importEntityInfo);
				importNumEnt++;
			}
		}

		rawList = rawList->next;
	}

	// Count instances of each unique type
	LIST* list1 = importEntityList;
	LIST* list2;
	ACMIEntityData* thing1;
	ACMIEntityData* thing2;
	int objCount;

	while (list1)
	{
		thing1 = (ACMIEntityData*)list1->node;
		if (thing1->count == 0)
		{
			thing1->count = 1;
			objCount = 2;
			list2 = list1->next;
			while (list2)
			{
				thing2 = (ACMIEntityData*)list2->node;
				if (thing2->type == thing1->type && thing2->count == 0)
				{
					thing2->count = objCount;
					objCount ++;
				}
				list2 = list2->next;
			}
		}
		list1 = list1->next;
	}



}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
** Description:
**		At this point importEntList and importPosList should be populated.
**		Now, we're going to have to setup the offset pointers to do the
**		file mapping.  Each entity chains back and forth thru its position
**		list.
*/
void ACMITape::ThreadEntityPositions ( ACMITapeHeader *tapeHdr )
{
	int i, j;
	long prevOffset;
	LIST *entityListPtr, *posListPtr, *featListPtr;
	ACMIEntityData *entityPtr, *featPtr;
	ACMIRawPositionData *posPtr;
	ACMIRawPositionData *prevPosPtr;
	ACMIFeatEventImportData *fePtr;
	BOOL foundFirst;
	long currOffset;

	// we run an outer and inner loop here.
	// the outer loops steps thru each entity
	// the inner loop searches each position update for one owned by the
	// entity and chains them together

	entityListPtr = importEntityList;
	for ( i = 0; i < importNumEnt; i++ )
	{
		// entityListPtr = LIST_NTH(importEntityList, i);
		entityPtr = (ACMIEntityData *)entityListPtr->node;
		foundFirst = FALSE;
		prevOffset = 0;
		prevPosPtr = NULL;
		entityPtr->firstPositionDataOffset = 0;

		posListPtr = importPosList;
		for ( j = 0; j < importNumPos; j++ )
		{
			// posListPtr = LIST_NTH(importPosList, j);
			posPtr = (ACMIRawPositionData *)posListPtr->node;

			// check the id to see if this position belongs to the entity
			if ( posPtr->uniqueID != entityPtr->uniqueID )
			{
				// nope
				posListPtr = posListPtr->next;
				continue;
			}

			// calculate the offset of this positional record
			currOffset = tapeHdr->timelineBlockOffset +
					   sizeof( ACMIEntityPositionData ) * j;

			// if it's the 1st in the chain, set the offset to it in
			// the entity's record
			if ( foundFirst == FALSE )
			{
				entityPtr->firstPositionDataOffset = currOffset;
				foundFirst = TRUE;
			}

			// thread current to previous
			posPtr->entityPosData.prevPositionUpdateOffset = prevOffset;
			posPtr->entityPosData.nextPositionUpdateOffset = 0;

			// thread previous to current
			if ( prevPosPtr )
			{
				prevPosPtr->entityPosData.nextPositionUpdateOffset = currOffset;
			}

			// set vals for next time thru loop
			prevOffset = currOffset;
			prevPosPtr = posPtr;

			// next in list
			posListPtr = posListPtr->next;

		} // end for position loop

		entityListPtr = entityListPtr->next;
	} // end for entity loop

	entityListPtr = importFeatList;
	for ( i = 0; i < importNumFeat; i++ )
	{
		entityPtr = (ACMIEntityData *)entityListPtr->node;
		foundFirst = FALSE;
		prevOffset = 0;
		prevPosPtr = NULL;
		entityPtr->firstPositionDataOffset = 0;

		posListPtr = importPosList;
		for ( j = 0; j < importNumPos; j++ )
		{
			// posListPtr = LIST_NTH(importPosList, j);
			posPtr = (ACMIRawPositionData *)posListPtr->node;

			// check the id to see if this position belongs to the entity
			if ( posPtr->uniqueID != entityPtr->uniqueID )
			{
				// nope
				posListPtr = posListPtr->next;
				continue;
			}

			// calculate the offset of this positional record
			currOffset = tapeHdr->timelineBlockOffset +
					   sizeof( ACMIEntityPositionData ) * j;

			// if it's the 1st in the chain, set the offset to it in
			// the entity's record
			if ( foundFirst == FALSE )
			{
				entityPtr->firstPositionDataOffset = currOffset;
				foundFirst = TRUE;
			}

			// thread current to previous
			posPtr->entityPosData.prevPositionUpdateOffset = prevOffset;
			posPtr->entityPosData.nextPositionUpdateOffset = 0;

			// thread previous to current
			if ( prevPosPtr )
			{
				prevPosPtr->entityPosData.nextPositionUpdateOffset = currOffset;
			}

			// set vals for next time thru loop
			prevOffset = currOffset;
			prevPosPtr = posPtr;

			// next in list
			posListPtr = posListPtr->next;

		} // end for position loop

		// while we're doing the features, for each one, go thru the
		// feature event list looking for our unique ID in the events
		// and setting the index value of our feature in the event
		posListPtr = importFeatEventList;
		for ( j = 0; j < importNumFeatEvents; j++ )
		{
			// posListPtr = LIST_NTH(importPosList, j);
			fePtr = (ACMIFeatEventImportData *)posListPtr->node;

			// check the id to see if this event belongs to the entity
			if ( fePtr->uniqueID == entityPtr->uniqueID )
			{
				fePtr->data.index = i;
			}

			// next in list
			posListPtr = posListPtr->next;

		} // end for feature event loop

		// now go thru the feature list again and find lead unique ID's and
		// change them to indices into the list

		// actually NOW, go through and just make sure they exist... otherwise, clear
		if ( entityPtr->leadIndex != -1)
		{
			featListPtr = importFeatList;
			for ( j = 0; j < importNumFeat; j++ )
			{
				// we don't compare ourselves
				if ( j != i )
				{
					featPtr = (ACMIEntityData *)featListPtr->node;
					if ( entityPtr->leadIndex == featPtr->uniqueID )
					{
						entityPtr->leadIndex = j;
						break;
					}
	
				}
				// next in list
				featListPtr = featListPtr->next;
			}

			// if we're gone thru the whole list and haven't found
			// a lead index, we're in trouble.  To protect, set the
			// lead index to -1
			if ( j == importNumFeat )
			{
				entityPtr->leadIndex = -1;
			}
		}

		entityListPtr = entityListPtr->next;
	} // end for feature entity loop


}

/*
** Description:
**		At this point importEntList and importPosList should be populated.
**		Now, we're going to have to setup the offset pointers to do the
**		file mapping.  Each entity chains back and forth thru its position
**		list.
*/
void ACMITape::ThreadEntityEvents ( ACMITapeHeader *tapeHdr )
{
	int i, j;
	long prevOffset;
	LIST *entityListPtr, *posListPtr;
	ACMIEntityData *entityPtr;
	ACMIRawPositionData *posPtr;
	ACMIRawPositionData *prevPosPtr;
	BOOL foundFirst;
	long currOffset;

	// we run an outer and inner loop here.
	// the outer loops steps thru each entity
	// the inner loop searches each position update for one owned by the
	// entity and chains them together

	entityListPtr = importEntityList;
	for ( i = 0; i < importNumEnt; i++ )
	{
		// entityListPtr = LIST_NTH(importEntityList, i);
		entityPtr = (ACMIEntityData *)entityListPtr->node;
		foundFirst = FALSE;
		prevOffset = 0;
		prevPosPtr = NULL;
		entityPtr->firstEventDataOffset = 0;

		posListPtr = importEntEventList;
		for ( j = 0; j < importNumEntEvents; j++ )
		{
			// posListPtr = LIST_NTH(importPosList, j);
			posPtr = (ACMIRawPositionData *)posListPtr->node;

			// check the id to see if this position belongs to the entity
			if ( posPtr->uniqueID != entityPtr->uniqueID )
			{
				// nope
				posListPtr = posListPtr->next;
				continue;
			}

			// calculate the offset of this positional record
			currOffset = tapeHdr->firstEntEventOffset +
					   sizeof( ACMIEntityPositionData ) * j;

			// if it's the 1st in the chain, set the offset to it in
			// the entity's record
			if ( foundFirst == FALSE )
			{
				entityPtr->firstEventDataOffset = currOffset;
				foundFirst = TRUE;
			}

			// thread current to previous
			posPtr->entityPosData.prevPositionUpdateOffset = prevOffset;
			posPtr->entityPosData.nextPositionUpdateOffset = 0;

			// thread previous to current
			if ( prevPosPtr )
			{
				prevPosPtr->entityPosData.nextPositionUpdateOffset = currOffset;
			}

			// set vals for next time thru loop
			prevOffset = currOffset;
			prevPosPtr = posPtr;

			// next in list
			posListPtr = posListPtr->next;

		} // end for position loop

		entityListPtr = entityListPtr->next;
	} // end for entity loop


}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
** Description:
**		At this point importEntList and importPosList should be populated.
**		Also the entities and positions are now threaded
**		write out the file
*/
void ACMITape::WriteTapeFile ( char *fname, ACMITapeHeader *tapeHdr )
{
	int i,j;
	LIST *entityListPtr, *posListPtr, *eventListPtr;
	ACMIEntityData *entityPtr;
	ACMIEventHeader *eventPtr;
	ACMIRawPositionData *posPtr;
	ACMIFeatEventImportData *fePtr;
	FILE *tapeFile;
	long ret;

	tapeFile = fopen(fname, "wb");
	if (tapeFile == NULL)
	{
		MonoPrint("Error opening new tape file\n");
		return;
	}

	// write the header
	ret = fwrite( tapeHdr, sizeof( ACMITapeHeader ), 1, tapeFile );
	if ( !ret )
	 	goto error_exit;


	// write out the entities
	entityListPtr = importEntityList;
	for ( i = 0; i < importNumEnt; i++ )
	{
		// entityListPtr = LIST_NTH(importEntityList, i);
		entityPtr = (ACMIEntityData *)entityListPtr->node;

		ret = fwrite( entityPtr, sizeof( ACMIEntityData ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;
		entityListPtr = entityListPtr->next;
	} // end for entity loop

	// write out the features
	entityListPtr = importFeatList;
	for ( i = 0; i < importNumFeat; i++ )
	{
		// entityListPtr = LIST_NTH(importEntityList, i);
		entityPtr = (ACMIEntityData *)entityListPtr->node;

		ret = fwrite( entityPtr, sizeof( ACMIEntityData ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;
		entityListPtr = entityListPtr->next;
	} // end for entity loop

	// write out the entitiy positions
	posListPtr = importPosList;
	for ( i = 0; i < importNumPos; i++ )
	{
		// posListPtr = LIST_NTH(importPosList, i);
		posPtr = (ACMIRawPositionData *)posListPtr->node;

		// we now want to do a "fixup" of the radar targets.  These are
		// currently in "uniqueIDs" and we want to convert them into
		// an index into the entity list
		if ( posPtr->entityPosData.posData.radarTarget != -1 )
		{
			entityListPtr = importEntityList;
			for ( j = 0; j < importNumEnt; j++ )
			{
				entityPtr = (ACMIEntityData *)entityListPtr->node;

				if ( posPtr->entityPosData.posData.radarTarget == entityPtr->uniqueID )
				{
					posPtr->entityPosData.posData.radarTarget = j;
					break;
				}

				entityListPtr = entityListPtr->next;
			} // end for entity loop

			// did we find it?
			if ( j == importNumEnt )
			{
				// nope
				posPtr->entityPosData.posData.radarTarget = -1;
			}
		} // end if there's a radar target

		ret = fwrite( &posPtr->entityPosData, sizeof( ACMIEntityPositionData ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;

		posListPtr = posListPtr->next;
	}

	// write out the entitiy events
	posListPtr = importEntEventList;
	for ( i = 0; i < importNumEntEvents; i++ )
	{
		// posListPtr = LIST_NTH(importPosList, i);
		posPtr = (ACMIRawPositionData *)posListPtr->node;

		ret = fwrite( &posPtr->entityPosData, sizeof( ACMIEntityPositionData ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;

		posListPtr = posListPtr->next;
	}


	// allocate the trailer list
	importEventTrailerList = new ACMIEventTrailer[importNumEvents];
	F4Assert( importEventTrailerList );

	// write out the events
	eventListPtr = importEventList;
	for ( i = 0; i < importNumEvents; i++ )
	{
		// eventListPtr = LIST_NTH(importEventList, i);
		eventPtr = (ACMIEventHeader *)eventListPtr->node;

		// set the trailer data
		importEventTrailerList[i].index = i;
		importEventTrailerList[i].timeEnd = eventPtr->timeEnd;

		ret = fwrite( eventPtr, sizeof( ACMIEventHeader ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;

		eventListPtr = eventListPtr->next;

	} // end for events loop

	// now sort the trailers in ascending order by endTime and
	// write them out
	qsort( importEventTrailerList,
		   importNumEvents,
		   sizeof( ACMIEventTrailer ),
		   CompareEventTrailer );

	for ( i = 0; i < importNumEvents; i++ )
	{
		ret = fwrite( &importEventTrailerList[i], sizeof( ACMIEventTrailer ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;

	} // end for events loop

	// write out the feature events
	posListPtr = importFeatEventList;
	for ( i = 0; i < importNumFeatEvents; i++ )
	{
		// posListPtr = LIST_NTH(importPosList, i);
		fePtr = (ACMIFeatEventImportData *)posListPtr->node;

		ret = fwrite( &fePtr->data, sizeof( ACMIFeatEvent ), 1, tapeFile );
		if ( !ret )
	 		goto error_exit;

		posListPtr = posListPtr->next;
	}

	// finally import and write out the text events
	ImportTextEventList( tapeFile, tapeHdr );

	// normal exit
	fclose( tapeFile );
	return;

error_exit:
	MonoPrint("Error writing new tape file\n");
	if ( tapeFile )
		fclose( tapeFile );
	return;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BOOL ACMITape::GetEntityPosition
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
)
{
	float
		deltaTime;

	float dx, dy, dz;
	float dx1, dy1, dz1;

	ACMIEntityPositionData
		*pos1,
		*pos2,
		*pos3;

	// init speed to 0.0
	speed = 0.0f;
	turnrate = 0.0f;
	turnradius = 0.0f;

	F4Assert(index >= 0 && index < NumEntities());

	pos1 = CurrentEntityPositionHead(index);

	// If there is not at least 1 positional update, the entity doesn't exist.
	F4Assert(pos1 != NULL);

	if(pos1->time > _simTime)
	{
		x = pos1->posData.x;
		y = pos1->posData.y;
		z = pos1->posData.z;
		yaw = pos1->posData.yaw;
		pitch = pos1->posData.pitch;
		roll = pos1->posData.roll;
		return FALSE;
	}

	pos2 = HeadNext(pos1);
	if(pos2 == NULL)
	{
		x = pos1->posData.x;
		y = pos1->posData.y;
		z = pos1->posData.z;
		yaw = pos1->posData.yaw;
		pitch = pos1->posData.pitch;
		roll = pos1->posData.roll;
		return FALSE;		
	}
	else
	{
   	pos3 = HeadPrev(pos1);
		F4Assert(pos1->time <= _simTime);
		F4Assert(pos2->time > _simTime);

		dx = pos2->posData.x - pos1->posData.x;
		dy = pos2->posData.y - pos1->posData.y;
		dz = pos2->posData.z - pos1->posData.z;

		// Interpolate.
		deltaTime = 
		(
			(_simTime - pos1->time) /
			(pos2->time - pos1->time)
		);

		x = 
		(
			pos1->posData.x + dx * deltaTime
		);

		y = 
		(
			pos1->posData.y + dy * deltaTime
		);

		z = 
		(
			pos1->posData.z + dz * deltaTime
		);

		yaw = AngleInterp( pos1->posData.yaw, pos2->posData.yaw, deltaTime );
		pitch = AngleInterp( pos1->posData.pitch, pos2->posData.pitch, deltaTime );
		roll = AngleInterp( pos1->posData.roll, pos2->posData.roll, deltaTime );

		// get the average speed
		speed = (float)sqrt( dx * dx + dy * dy + dz * dz ) / ( pos2->time - pos1->time );
		float dAng = pos2->posData.yaw - pos1->posData.yaw;
		if ( fabs( dAng ) > 180.0f * DTR )
		{
			if ( dAng >= 0.0f )
				dAng -= 360.0f * DTR;
			else
				dAng += 360.0f * DTR;

		}

      if (pos3)
      {
		   dx1 = pos1->posData.x - pos3->posData.x;
		   dy1 = pos1->posData.y - pos3->posData.y;
		   dz1 = pos1->posData.z - pos3->posData.z;

         // Turn rate = solid angle delta between velocity vectors
         turnrate = (float)acos ((dx*dx1 + dy*dy1 + dz*dz1)/
            (float)sqrt((dx*dx + dy*dy + dz*dz) * (dx1*dx1 + dy1*dy1 + dz1*dz1)));
         turnrate *= RTD / ( pos2->time - pos1->time );
//		   turnrate = RTD * fabs( dAng ) / ( pos2->time - pos1->time );

		   if ( turnrate != 0.0f )
		   {
			   // sec to turn 360 deg
			   float secs = 360.0f/turnrate;

			   // get circumference
			   float circum = speed * secs;

			   // now we get turn radius ( circum = 2 * PI * R )
			   turnradius = circum/( 2.0f * PI );
		   }
      }
      else
      {
         turnrate = 0.0F;
         turnradius = 0.0F;
      }
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::SetGeneralEventCallbacks
(
	ACMI_GENERAL_EVENT_CALLBACK forwardEventCallback,
	ACMI_GENERAL_EVENT_CALLBACK reverseEventCallback,
	void *userData
)
{
	_generalEventCallbacks.forwardCallback = forwardEventCallback;
	_generalEventCallbacks.reverseCallback = reverseEventCallback;
	_generalEventCallbacks.userData = userData;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::SetHeadPosition(float t)
{
	float newSimTime;

	// t should be normalized from 0 to 1
	newSimTime = _tapeHdr.startTime + (_tapeHdr.totPlayTime - 0.1f) * t;

	// run the update cycle until we've reached the new sim time
	while( _simTime != newSimTime )
		Update( newSimTime );

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

float ACMITape::GetNewSimTime(float t)
{
	float newSimTime;

	// t should be normalized from 0 to 1
	newSimTime = _tapeHdr.startTime + (_tapeHdr.totPlayTime - 0.1f) * t;

	return newSimTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

float ACMITape::HeadPosition()
{
	return _simTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::Update( float newSimTime )
{
	float
		realTime,
		deltaRealTime;
	float
		deltaLimit;

	if ( _screenCapturing )
		deltaLimit = 0.0625f;
	else
		deltaLimit = 0.2f;

	// Update active events
	UpdateActiveEvents();

	// Calculate delta time and unpause us if necessary.
	realTime = timeGetTime() * 0.001F;

	// if new sim time is not negative, we are trying to
	// reach a new play position that the user has set with the slider
	// we'll be going in steps until _simTime = newSimTime
	_simulateOnly = FALSE;
	if ( newSimTime >= 0.0f )
	{
		_simulateOnly = TRUE;
		deltaLimit = newSimTime - _simTime;
		deltaRealTime = 0.2f;
		if ( deltaLimit > 0.0f )
		{
			_playVelocity = 1.0f;
		}
		else if ( deltaLimit < 0.0f )
		{
			_playVelocity = -1.0f;
		}
		else
		{
			_playVelocity = 0.0f;
		}
		if ( deltaLimit < 0.0f )
			deltaLimit = -deltaLimit;
		
		if ( deltaRealTime > deltaLimit )
			deltaRealTime = deltaLimit;

	}
	else if(_unpause && _paused)
	{
		deltaRealTime = 0.0;
		_paused = FALSE;
	}
	else if(_paused)
	{
		deltaRealTime = 0.0;
	}
	else
	{
		deltaRealTime = realTime - _lastRealTime;

		// for debugger stops, make sure delta never is larger
		// than 1/5 second
		if ( deltaRealTime < -deltaLimit )
			deltaRealTime = -deltaLimit;
		if ( deltaRealTime > deltaLimit )
			deltaRealTime = deltaLimit;
	}
	_lastRealTime = realTime;
	_unpause = FALSE;

	// Advance time.
	_deltaSimTime = _playVelocity * deltaRealTime;
	_simTime += _deltaSimTime;

	// sanity check -- don't allow the head to go beyond tape ends
	if ( _simTime < _tapeHdr.startTime )
		_simTime = _tapeHdr.startTime;
	if ( _simTime > _tapeHdr.startTime + _tapeHdr.totPlayTime - 0.1f)
		_simTime = _tapeHdr.startTime + _tapeHdr.totPlayTime - 0.1f;

	_playVelocity += _playAcceleration * deltaRealTime;
	Clamp((float)(-_maxPlaySpeed), _playVelocity, (float)_maxPlaySpeed);

	// Advance all entity read heads.
	AdvanceAllHeads();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::AdvanceEntityPositionHead(int index)
{
	ACMIEntityPositionData
		*curr,
		*next,
		*prev;

	F4Assert(index >= 0 && index < NumEntities());

	// Backward.
	curr = CurrentEntityPositionHead(index);
	if(curr == NULL) return;
	while(_simTime < curr->time)
	{
		prev = HeadPrev(curr);
		if(prev == NULL) return;

		// Advance the head.
		_entityReadHeads[index].positionDataOffset = curr->prevPositionUpdateOffset;
		curr = prev;
	}
	
	// Forward.
	next = HeadNext(curr);
	if(next == NULL) return;
	while(_simTime >= next->time)
	{
		// Advance the head.
		_entityReadHeads[index].positionDataOffset = curr->nextPositionUpdateOffset;
		curr = next;

		next = HeadNext(curr);
		if(next == NULL) return;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::AdvanceEntityEventHead(int index)
{
	ACMIEntityPositionData
		*curr,
		*next,
		*prev;
	SimTapeEntity *e;

	F4Assert(index >= 0 && index < NumEntities());

	// get the entity if we need to change switch settings
	e = &_simTapeEntities[ index ];

	// Backward.
	curr = CurrentEntityEventHead(index);
	if(curr == NULL) return;
	while(_simTime < curr->time)
	{
		prev = HeadPrev(curr);
		if(prev == NULL) return;

		// handle switch settings
		if ( curr->type == PosTypeSwitch )
		{
			((DrawableBSP *)e->objBase->drawPointer)->SetSwitchMask( curr->switchData.switchNum, curr->switchData.prevSwitchVal );
		}
		else if ( curr->type == PosTypeDOF )
		{
			((DrawableBSP *)e->objBase->drawPointer)->SetDOFangle( curr->dofData.DOFNum, curr->dofData.prevDOFVal );
		}
		
		// Advance the head.
		_entityReadHeads[index].eventDataOffset = curr->prevPositionUpdateOffset;
		curr = prev;
	}
	
	// Forward.
	next = HeadNext(curr);
	if(next == NULL) return;
	while(_simTime >= next->time)
	{
		// Advance the head.
		_entityReadHeads[index].eventDataOffset = curr->nextPositionUpdateOffset;

		// handle switch settings
		if ( curr->type == PosTypeSwitch )
		{
			((DrawableBSP *)e->objBase->drawPointer)->SetSwitchMask( curr->switchData.switchNum, curr->switchData.switchVal );
		}
		else if ( curr->type == PosTypeDOF )
		{
			((DrawableBSP *)e->objBase->drawPointer)->SetDOFangle( curr->dofData.DOFNum, curr->dofData.DOFVal );
		}

		curr = next;
		next = HeadNext(curr);
		if(next == NULL) return;
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::AdvanceGeneralEventHeadHeader( void )
{
	ACMIEventHeader
		*curr,
		*next,
		*prev;

	// Reverse.
	curr = GeneralEventData();
	if(curr == NULL) return;
	while(_simTime < curr->time)
	{
		prev = Prev(curr);
		if(prev == NULL) return;

		if ( _eventList[ curr->index ] )
			RemoveActiveEvent( &_eventList[ curr->index ] );


		// Advance the head.
		curr = prev;
		_generalEventReadHeadHeader = curr->index;
	}
	
	// Forward.
	next = Next(curr);
	if(next == NULL) return;
	while(_simTime >= next->time)
	{

		// Advance the head.
		curr = next;
		_generalEventReadHeadHeader = curr->index;

		if ( !_eventList[ curr->index ] )
		{
			_eventList[curr->index] = InsertActiveEvent( curr, _simTime - curr->time  );
		}


		next = Next(curr);
		if(next == NULL) return;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::AdvanceGeneralEventHeadTrailer( void )
{
	ACMIEventHeader *e;
	ACMIEventTrailer
		*curr,
		*next,
		*prev;

	// Reverse.
	curr = GeneralEventTrailer();
	if(curr == NULL) return;
	while(_simTime < curr->timeEnd)
	{
		prev = Prev(curr);
		if(prev == NULL) return;

		if ( !_eventList[ curr->index ] )
		{
			e = GetGeneralEventData( curr->index );
			_eventList[curr->index] = InsertActiveEvent( e, _simTime - e->time  );
		}


		// Advance the head.
		curr = prev;
		_generalEventReadHeadTrailer = curr;
	}
	
	// Forward.
	next = Next(curr);
	if(next == NULL) return;
	while(_simTime >= next->timeEnd)
	{

		// Advance the head.
		curr = next;
		_generalEventReadHeadTrailer = curr;

		if ( _eventList[ curr->index ] )
			RemoveActiveEvent( &_eventList[ curr->index ] );


		next = Next(curr);
		if(next == NULL) return;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::AdvanceFeatEventHead( void )
{
	ACMIFeatEvent
		*curr,
		*next,
		*prev;
	SimTapeEntity *feat;

	// Reverse.
	curr = CurrFeatEvent();
	if(curr == NULL) return;
	while(_simTime < curr->time)
	{
		prev = Prev(curr);
		if(prev == NULL) return;

		// do stuff

		// sanity check that we have the right index
		if ( curr->index >= 0 )
		{
			// get the feature entity
			feat = &_simTapeFeatures[ curr->index ];

			// create the new drawable object
			// set new status
			feat->objBase->ClearStatusBit( VIS_TYPE_MASK );
			feat->objBase->SetStatusBit( curr->prevStatus );

			// this function now handles inserts and removes from
			// drawlist
       		CreateFeatureDrawable ( feat );
			F4Assert(feat->objBase->drawPointer != NULL);
			if ( curr->prevStatus == VIS_DAMAGED )
			{
				((DrawableBSP *)feat->objBase->drawPointer)->SetTextureSet( 1 );
			}

			// remove old from display and delete
			/*
			if(feat->objBase->drawPointer->InDisplayList())
			{
				_viewPoint->RemoveObject(feat->objBase->drawPointer);
			}
			delete feat->objBase->drawPointer;
			feat->objBase->drawPointer = NULL;

			// set new status
			feat->objBase->ClearStatusBit( VIS_TYPE_MASK );
			feat->objBase->SetStatusBit( curr->prevStatus );

       		CreateDrawable ( feat->objBase, 1.0F);
			F4Assert(feat->objBase->drawPointer != NULL);

			// set damaged texture set if needed
			if ( curr->prevStatus == VIS_DAMAGED )
			{
				((DrawableBSP *)feat->objBase->drawPointer)->SetTextureSet( 1 );
			}

			// features get put into draw list and positioned here.
			_viewPoint->InsertObject( feat->objBase->drawPointer );
			*/
		}

		// Advance the head.
		curr = prev;
		_featEventReadHead = curr;
	}
	
	// Forward.
	next = Next(curr);
	if(next == NULL) return;
	if (F4IsBadReadPtr(curr, sizeof(ACMIFeatEvent)))
		return;

	while(_simTime >= next->time)
	{
		if (F4IsBadReadPtr(next, sizeof(ACMIFeatEvent)))
			return;

		// Advance the head.
		curr = next;
		_featEventReadHead = curr;

		// do stuff

		// sanity check that we have the right index
		if ( curr->index >= 0 )
		{
			// get the feature entity
			feat = &_simTapeFeatures[ curr->index ];

			// create the new drawable object
			// set new status
			feat->objBase->ClearStatusBit( VIS_TYPE_MASK );
			feat->objBase->SetStatusBit( curr->newStatus );

       		CreateFeatureDrawable ( feat );
			F4Assert(feat->objBase->drawPointer != NULL);
			// set damaged texture set if needed
			if ( curr->newStatus == VIS_DAMAGED )
			{
				((DrawableBSP *)feat->objBase->drawPointer)->SetTextureSet( 1 );
			}

			/*
			// remove old from display and delete
			if(feat->objBase->drawPointer->InDisplayList())
			{
				_viewPoint->RemoveObject(feat->objBase->drawPointer);
			}
			delete feat->objBase->drawPointer;
			feat->objBase->drawPointer = NULL;

			// set new status
			feat->objBase->ClearStatusBit( VIS_TYPE_MASK );
			feat->objBase->SetStatusBit( curr->newStatus );

       		CreateDrawable ( feat->objBase, 1.0F);
			F4Assert(feat->objBase->drawPointer != NULL);

			// set damaged texture set if needed
			if ( curr->newStatus == VIS_DAMAGED )
			{
				((DrawableBSP *)feat->objBase->drawPointer)->SetTextureSet( 1 );
			}

			// features get put into draw list and positioned here.
			_viewPoint->InsertObject( feat->objBase->drawPointer );
			*/
		}

		next = Next(curr);
		if(next == NULL) return;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
** Description:
**		Opens the passed in tape file name and sets up the memory mapped
**		stuff
*/
long ACMITape::OpenTapeFile ( char *fname )
{
	FILE *fd;
	long length=0;

	// 1st get the header info and check it out
	fd = fopen( fname, "rb" );
	if ( fd == NULL )
	{
		MonoPrint( "Unable to Open Tape File\n");
		return(0);
	}

	// read in the tape header
	if ( !fread( &_tapeHdr, sizeof( ACMITapeHeader), 1, fd ) )
	{
		MonoPrint( "Unable to to read tape header\n");
		fclose( fd );
		return(0);
	}

	// close the file
	fclose( fd );

	// check that we've got a valid file
	if ( _tapeHdr.fileID != 'TAPE' )
	{
		MonoPrint( "Invalid Tape File\n");
		return(0);
	}

	// Set up memory mapping

	// open the tape file
	_tapeFileHandle = CreateFile(
						fname,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
						NULL );
	if ( _tapeFileHandle == INVALID_HANDLE_VALUE )
	{
		MonoPrint( "CreateFile failed on tape open\n" );
		return(0);
	}
	length=GetFileSize(_tapeFileHandle,NULL);

	// create file mapping
	_tapeMapHandle = CreateFileMapping(
						_tapeFileHandle,
						NULL,
						PAGE_READONLY,
						0,
						0,
						NULL );
	if ( _tapeMapHandle == NULL )
	{
		MonoPrint( "CreateFileMapping failed on tape open\n" );
		CloseHandle( _tapeFileHandle );
		return(0);
	}

	// map view of file
	_tape = MapViewOfFile(
						_tapeMapHandle,
						FILE_MAP_READ,
						0,
						0,
						0 );
	if ( _tape == NULL )
	{
		MonoPrint( "MapViewOfFile failed on tape open\n" );
		CloseHandle( _tapeMapHandle );
		CloseHandle( _tapeFileHandle );
		return(0);
	}

	// hunky dory
	return(length);

}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
** Description:
**		Releases system stuff done in OpenTapeFile
*/
void ACMITape::CloseTapeFile ( void )
{
	UnmapViewOfFile( _tape );
	CloseHandle( _tapeMapHandle );
	CloseHandle( _tapeFileHandle );
	_tape = NULL;
	return;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
** Description:
**		Interpolates between 2 angles.  dT is the relative dist (0-1)
**		between begin and end.
*/
float ACMITape::AngleInterp ( float begAng, float endAng, float dT )
{
	float dAng;

	// get the delta angle
	dAng = endAng - begAng;

	// always rotate in shortest direction (ie when delta > 180 deg in mag)
	if ( fabs( dAng ) > 180.0f * DTR )
	{
		if ( dAng >= 0.0f )
			endAng -= 360.0f * DTR;
		else
			endAng += 360.0f * DTR;

		dAng = endAng - begAng;
	}


	/*
	if ( endAng < -0.5f * PI && begAng > 0.5f * PI )
		dAng = endAng + ( 2.0f * PI ) - begAng;
	else if ( endAng > 0.5f * PI && begAng < -0.5f * PI )
		dAng = endAng - ( 2.0f * PI ) - begAng;
	else
		dAng = endAng - begAng;
	*/

	return (float)(begAng + dAng * dT);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ACMITape::AdvanceAllHeads( void )
{
	int
		i,
		numEntities;

	// Advance all entity read heads.
	numEntities = NumEntities();
	for(i = 0; i < numEntities; i++)
	{
		// Advance entity position head.
		AdvanceEntityPositionHead(i);
		AdvanceEntityEventHead(i);
	}

	// Advance general event head, and apply events that are encountered.
	AdvanceGeneralEventHeadHeader();
	AdvanceGeneralEventHeadTrailer();

	// advance head for any feature events
	AdvanceFeatEventHead();
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		Reads the entities off of the tape and sets up the in-memory
**		sim and graphics representation for them so that they can be
**		manipulated and moved
*/
void ACMITape::SetupSimTapeEntities()
{
	int				
		i,
		numEntities;
	Tpoint pos;
	ACMIEntityData *e;
	ACMIEntityPositionData *p;


	Tpoint origin = { 0.0f, 0.0f, 0.0f };

	F4Assert(_simTapeEntities == NULL);
	F4Assert(_tape != NULL );

	// create array of SimTapeEntity
	numEntities = NumEntities();
	_simTapeEntities = new SimTapeEntity [numEntities];
	F4Assert(_simTapeEntities != NULL);

	// for each entity, create it's object stuff....
	for(i = 0; i < numEntities; i++)
	{

		// get the tape entity data
		e = EntityData( i );

		// get the 1st position data for the entity
		p = CurrentEntityPositionHead( i );

		_simTapeEntities[i].flags = e->flags;

		if ( e->flags & ENTITY_FLAG_FEATURE )
		{
			// edg: I believe this code path should no longer be in use
			// since features are in a different list
			_simTapeEntities[i].objBase = new SimFeatureClass(EntityType(i));
			_simTapeEntities[i].objBase->SetDelta( 0.0f, 0.0f, 0.0f );
			_simTapeEntities[i].objBase->SetYPRDelta( 0.0f, 0.0f, 0.0f );
		}
		else
		{
			// create the base class
			_simTapeEntities[i].objBase = new SimStaticClass(EntityType(i));// new SimBaseClass(EntityType(i));
			GetEntityPosition ( i,
								_simTapeEntities[i].x,
								_simTapeEntities[i].y,
								_simTapeEntities[i].z,
								_simTapeEntities[i].yaw,
								_simTapeEntities[i].pitch,
								_simTapeEntities[i].roll,
								_simTapeEntities[i].aveSpeed,
								_simTapeEntities[i].aveTurnRate,
								_simTapeEntities[i].aveTurnRadius );
		}

		// set the matrix
		_simTapeEntities[i].objBase->SetYPR( p->posData.yaw, p->posData.pitch, p->posData.roll );
		_simTapeEntities[i].objBase->SetPosition( p->posData.x, p->posData.y, p->posData.z);
		CalcTransformMatrix(_simTapeEntities[i].objBase);

		// create thge drawable object
		// special case chaff and flares
		if ( e->flags & ENTITY_FLAG_CHAFF )
		{
			pos.x = p->posData.x;
			pos.y = p->posData.y;
			pos.z = p->posData.z;
       		_simTapeEntities[i].objBase->drawPointer =
				 new DrawableBSP( MapVisId(VIS_CHAFF), &pos, &IMatrix, 1.0f );
       		((DrawableBSP *)_simTapeEntities[i].objBase->drawPointer)->SetLabel("", 0 );
		}
		else if ( e->flags & ENTITY_FLAG_FLARE )
		{

			pos.x = p->posData.x;
			pos.y = p->posData.y;
			pos.z = p->posData.z;
       		_simTapeEntities[i].objBase->drawPointer =
				new Drawable2D( DRAW2D_FLARE, 2.0f, &pos );
		}
		// aircraft use special Drawable Poled class
		else if ( e->flags & ENTITY_FLAG_AIRCRAFT )
		{
			short visType;

			SimBaseClass *theObject = _simTapeEntities[i].objBase;
			// get the class pointer
			Falcon4EntityClassType* classPtr =
				(Falcon4EntityClassType*)theObject->EntityType();
			pos.x = p->posData.x;
			pos.y = p->posData.y;
			pos.z = p->posData.z;
			visType = classPtr->visType[theObject->Status() & VIS_TYPE_MASK];
			theObject->drawPointer = new DrawablePoled( visType, &pos, &IMatrix, 1.0f );

			F4Assert( theObject->drawPointer != NULL );
			
			if(ACMI_Callsigns) // we have callsigns
				theObject->drawPointer->SetLabel(ACMI_Callsigns[e->uniqueID].label, TeamSimColorList[ACMI_Callsigns[e->uniqueID].teamColor]);
		}
		else
		{
			CreateDrawable ( _simTapeEntities[i].objBase, 1.0F);
			if(ACMI_Callsigns)
				((DrawableBSP*)_simTapeEntities[i].objBase->drawPointer)->SetLabel(((DrawableBSP*)_simTapeEntities[i].objBase->drawPointer)->Label(),TeamSimColorList[ACMI_Callsigns[e->uniqueID].teamColor]);
		}


		// sigh.  hack for ejections
		if (_simTapeEntities[i].objBase->drawPointer == NULL)
		{
			short visType;

			SimBaseClass *theObject = _simTapeEntities[i].objBase;
			// get the class pointer
			Falcon4EntityClassType* classPtr =
				(Falcon4EntityClassType*)theObject->EntityType();
			if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_EJECT)
			{
				pos.x = p->posData.x;
				pos.y = p->posData.y;
				pos.z = p->posData.z;
				visType = classPtr->visType[theObject->Status() & VIS_TYPE_MASK];
				theObject->drawPointer = new DrawableBSP( MapVisId(VIS_EJECT1), &pos, &IMatrix, 1.0f );
       			((DrawableBSP *)theObject->drawPointer)->SetLabel("Ejected Pilot", 0x0000FF00 );
			}
			F4Assert( theObject->drawPointer != NULL );
		}

		// features get put into draw list and positioned here.
		if ( e->flags & ENTITY_FLAG_FEATURE )
		{
			// edg: I believe this code path should no longer be in use
			// since features are in a different list
			_viewPoint->InsertObject( _simTapeEntities[i].objBase->drawPointer );
		}


		// create other stuff as needed by the object
		// (ie, if this is missile, create the drawable trail for it...)
		_simTapeEntities[i].objTrail = NULL;
		_simTapeEntities[i].objBsp1 = NULL;
		_simTapeEntities[i].objBsp2 = NULL;
		_simTapeEntities[i].obj2d = NULL;
		_simTapeEntities[i].wlTrail = NULL;
		_simTapeEntities[i].wrTrail = NULL;
		_simTapeEntities[i].wtLength = 0;


		// a missile -- it needs drawable trail set up
		if ( (e->flags & ENTITY_FLAG_MISSILE) )
		{
			_simTapeEntities[i].objTrail = new DrawableTrail(TRAIL_SAM);
			_simTapeEntities[i].objBsp1 = new DrawableBSP( MapVisId(VIS_MFLAME_L), &origin, (struct Trotation *)&IMatrix, 1.0f );
			_simTapeEntities[i].objTrail->KeepStaleSegs( TRUE );
			_simTapeEntities[i].trailStartTime = p->time;
			_simTapeEntities[i].trailEndTime = p->time + 120.0F; //me123 to 30 we wanna see the trials in acmi// trail lasts 3 sec
		}
		// a flare -- it needs drawable trail set up and a glow sphere
		else if ( (e->flags & ENTITY_FLAG_FLARE) )
		{
			_simTapeEntities[i].objTrail = new DrawableTrail(TRAIL_SAM);
			_simTapeEntities[i].objTrail->KeepStaleSegs( TRUE );
			_simTapeEntities[i].trailStartTime = p->time;
			_simTapeEntities[i].trailEndTime = p->time + 3.0F; // trail lasts 3 sec
			_simTapeEntities[i].obj2d =
				new Drawable2D( DRAW2D_EXPLCIRC_GLOW, 8.0f, &origin );
		}

		// a aircraft -- it needs drawable wing trails set up
		else if ( (e->flags & ENTITY_FLAG_AIRCRAFT) )
		{
			_simTapeEntities[i].wlTrail = new DrawableTrail(TRAIL_LWING);
			_simTapeEntities[i].wlTrail->KeepStaleSegs( true ); // MLR 12/14/2003 - 
			_simTapeEntities[i].wrTrail = new DrawableTrail(TRAIL_RWING);
			_simTapeEntities[i].wrTrail->KeepStaleSegs( true ); // MLR 12/14/2003 - 
			//_simTapeEntities[i].wlTrail = new DrawableTrail(TRAIL_AIM120);
			//_simTapeEntities[i].wrTrail = new DrawableTrail(TRAIL_MAVERICK);
																
		}

	}

	F4Assert(_simTapeFeatures == NULL);


	// create array of SimTapeEntity
	if ( _tapeHdr.numFeat == 0 )
		return;
	_simTapeFeatures = new SimTapeEntity [_tapeHdr.numFeat];
	F4Assert(_simTapeFeatures != NULL);

	// for each feature, create it's object stuff....
	for(i = 0; i < _tapeHdr.numFeat; i++)
	{
		// get the tape entity data
		e = FeatureData( i );

		// get the 1st position data for the entity
		p = CurrentFeaturePositionHead( i );

		_simTapeFeatures[i].flags = e->flags;

		// protect against bad indices(!?)
		// edg: I'm not sure what's going on but it seems like we
		// occasionally get a bad lead VU ID.  The imported can't find
		// it.  This is protected for in Import.  Also protect here...
		if ( e->leadIndex >= _tapeHdr.numFeat )
			_simTapeFeatures[i].leadIndex = -1;
		else
			_simTapeFeatures[i].leadIndex = e->leadIndex;
		_simTapeFeatures[i].slot = e->slot;

		_simTapeFeatures[i].objBase = new SimFeatureClass(e->type);
		F4Assert(_simTapeFeatures[i].objBase != NULL);
		_simTapeFeatures[i].objBase->SetDelta( 0.0f, 0.0f, 0.0f );
		_simTapeFeatures[i].objBase->SetYPRDelta( 0.0f, 0.0f, 0.0f );
		_simTapeFeatures[i].objBase->SetYPR( p->posData.yaw, p->posData.pitch, p->posData.roll );
		_simTapeFeatures[i].objBase->SetPosition( p->posData.x, p->posData.y, p->posData.z);
		((SimFeatureClass *)_simTapeFeatures[i].objBase)->featureFlags = e->specialFlags;

		// get the class pointer
		Falcon4EntityClassType* classPtr =
			(Falcon4EntityClassType*)_simTapeFeatures[i].objBase->EntityType();
		// get the feature class data
		FeatureClassDataType *fc = (FeatureClassDataType *)classPtr->dataPtr;
		_simTapeFeatures[i].objBase->SetCampaignFlag( fc->Flags );
		CalcTransformMatrix(_simTapeFeatures[i].objBase);



		// create other stuff as needed by the object
		// (ie, if this is missile, create the drawable trail for it...)
		_simTapeFeatures[i].objTrail = NULL;
		_simTapeFeatures[i].objBsp1 = NULL;
		_simTapeFeatures[i].objBsp2 = NULL;
		_simTapeFeatures[i].obj2d = NULL;

	}

	// for each feature, create it's drawable object
	// we do this in a different loop because we want to make sure all the
	// objbase's are created first since some feature objects are actually
	// subcomponents of others and have thier drawpointer inserted into
	// the drawpointer list of another rather than the main drawlist
	for(i = 0; i < _tapeHdr.numFeat; i++)
	{
		// create the drawable object
       	CreateFeatureDrawable ( &_simTapeFeatures[i] );
		F4Assert(_simTapeFeatures[i].objBase->drawPointer != NULL);

		// NOTE: insertion into draw list should now be done in
		// CreateFeatureDrawable
		// features get put into draw list and positioned here.
		// _viewPoint->InsertObject( _simTapeFeatures[i].objBase->drawPointer );
	}

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		Cleans up the _simTapeEntities list freeing objects and removing
**		drawables from lists
*/
void ACMITape::CleanupSimTapeEntities( void )
{
	int				
		i,
		numEntities;
	SimTapeEntity *ep;

	F4Assert(_simTapeEntities != NULL);

	// create array of SimTapeEntity
	numEntities = NumEntities();

	// for each entity, create it's object stuff....
	for(i = 0; i < numEntities; i++)
	{
		// get pointer
		ep = &_simTapeEntities[i];

		// if there's a base object...
		if ( ep->objBase )
		{
			// remove from display
			if(ep->objBase->drawPointer->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->objBase->drawPointer);
			}
			delete ep->objBase->drawPointer;
			ep->objBase->drawPointer = NULL;
			delete ep->objBase;
			ep->objBase = NULL;
		}

		// if there's a trail object...
		if ( ep->objTrail )
		{
			// remove from display
			if(ep->objTrail->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->objTrail);
			}
			delete ep->objTrail;
			ep->objTrail = NULL;
		}

		// if there's a 2d object...
		if ( ep->obj2d )
		{
			// remove from display
			if(ep->obj2d->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->obj2d);
			}
			delete ep->obj2d;
			ep->obj2d = NULL;
		}

		// if there's a bsp object...
		if ( ep->objBsp1 )
		{
			// remove from display
			if(ep->objBsp1->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->objBsp1);
			}
			delete ep->objBsp1;
			ep->objBsp1 = NULL;
		}

		// if there's a bsp object...
		if ( ep->objBsp2 )
		{
			// remove from display
			if(ep->objBsp2->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->objBsp2);
			}
			delete ep->objBsp2;
			ep->objBsp2 = NULL;
		}

		// if there's a trail object...
		if ( ep->wlTrail )
		{
			// remove from display
			if(ep->wlTrail->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->wlTrail);
			}
			delete ep->wlTrail;
			ep->wlTrail = NULL;
		}

		// if there's a trail object...
		if ( ep->wrTrail )
		{
			// remove from display
			if(ep->wrTrail->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->wrTrail);
			}
			delete ep->wrTrail;
			ep->wrTrail = NULL;
		}
	}

	delete [] _simTapeEntities;
	_simTapeEntities = NULL;

	// for each feature, remove its object stuff
	// 1st pass do drawpointer
	for(i = 0; i < _tapeHdr.numFeat; i++)
	{
		// get pointer
		ep = &_simTapeFeatures[i];

		// if there's a base object...
		if ( ep->objBase )
		{
			// remove from display
			if(ep->objBase->drawPointer->InDisplayList())
			{
				_viewPoint->RemoveObject(ep->objBase->drawPointer);
			}
			delete ep->objBase->drawPointer;
			ep->objBase->drawPointer = NULL;
		}

	}

	// 2nd pass delete baseObj pointer and objBase
	for(i = 0; i < _tapeHdr.numFeat; i++)
	{
		// get pointer
		ep = &_simTapeFeatures[i];

		// if there's a base object...
		if ( ep->objBase )
		{
			// remove from display
			if( ((SimFeatureClass *)ep->objBase)->baseObject)
			{
				if( ((SimFeatureClass *)ep->objBase)->baseObject->InDisplayList())
				{
					_viewPoint->RemoveObject( ((SimFeatureClass*)ep->objBase)->baseObject);
				}
				delete ((SimFeatureClass *)ep->objBase)->baseObject;
				((SimFeatureClass *)ep->objBase)->baseObject = NULL;
			}

			delete ep->objBase;
			ep->objBase = NULL;
		}

	}

	delete [] _simTapeFeatures;
	_simTapeFeatures = NULL;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		Access simtape entity by index
*/
SimTapeEntity * ACMITape::GetSimTapeEntity( int index )
{
	F4Assert(_simTapeEntities != NULL);
	F4Assert(index < NumEntities() );

	return &_simTapeEntities[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		Returns if entity exists in current frame or not
*/
BOOL ACMITape::IsEntityInFrame( int index )
{
	ACMIEntityPositionData
		*pos1,
		*pos2;

	F4Assert(index >= 0 && index < NumEntities());

	pos1 = CurrentEntityPositionHead(index);

	// If there is not at least 1 positional update, the entity doesn't exist.
	F4Assert(pos1 != NULL);

	if(pos1->time > _simTime)
	{
		return FALSE;
	}

	pos2 = HeadNext(pos1);
	if(pos2 == NULL || pos2->time < _simTime )
	{
		return FALSE;		
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		returns the index of the entity targeted by the entity who's
**		index is passed in.  -1 is no target.
*/
int ACMITape::GetEntityCurrentTarget( int index )
{
	ACMIEntityPositionData
		*pos1,
		*pos2;

	F4Assert(index >= 0 && index < NumEntities());

	pos1 = CurrentEntityPositionHead(index);

	// If there is not at least 1 positional update, the entity doesn't exist.
	F4Assert(pos1 != NULL);

	if(pos1->time > _simTime)
	{
		return -1;
	}

	pos2 = HeadNext(pos1);
	if(pos2 == NULL || pos2->time < _simTime )
	{
		return -1;
	}

	return pos1->posData.radarTarget;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**	Puts the entity into darw list
*/
void ACMITape::InsertEntityInFrame( int index )
{
	F4Assert(_simTapeEntities != NULL);
	F4Assert(index < NumEntities() );

	if (_simTapeEntities[index].objBase->drawPointer->InDisplayList() )
		return;

	_viewPoint->InsertObject( _simTapeEntities[index].objBase->drawPointer );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**	removes the entity from darw list
*/
void ACMITape::RemoveEntityFromFrame( int index )
{
	F4Assert(_simTapeEntities != NULL);
	F4Assert(index < NumEntities() );

	if (!_simTapeEntities[index].objBase->drawPointer->InDisplayList() )
		return;

	_viewPoint->RemoveObject( _simTapeEntities[index].objBase->drawPointer );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		Runs thru the entity list and determines if the entities
**		should be inserted/removed from draw and/or positioned in the frame.
*/
void ACMITape::UpdateSimTapeEntities( void )
{
	int				
		i,
		numEntities;

	SimTapeEntity *ep;

	Tpoint pos;	
	Tpoint wtpos;
	Trotation rot;
	Tpoint newPoint;




	F4Assert(_simTapeEntities != NULL);

	// create array of SimTapeEntity
	numEntities = NumEntities();
	_renderer->SetColor (0xffff0000);

	// for each entity, create it's object stuff....
	for(i = 0; i < numEntities; i++)
	{
		// get pointer
		ep = &_simTapeEntities[i];


		if ( GetEntityPosition ( i, ep->x, ep->y, ep->z, ep->yaw, ep->pitch, ep->roll, ep->aveSpeed, ep->aveTurnRate, ep->aveTurnRadius  ) == FALSE )
		{
			// make sure we remove from draw list
			if ( ep->objBase->drawPointer->InDisplayList() )
			{
				_viewPoint->RemoveObject( ep->objBase->drawPointer );
				// remove trail too
				if ( ep->objTrail && ep->objTrail->InDisplayList() && _simTime < ep->trailStartTime )
				{
					ep->objTrail->TrimTrail( 0 );
					_viewPoint->RemoveObject( ep->objTrail );
				}
				// remove Bsp1 too
				if ( ep->objBsp1 && ep->objBsp1->InDisplayList() && _simTime < ep->trailStartTime )
				{
					_viewPoint->RemoveObject( ep->objBsp1 );
				}
				// remove Bsp2 too
				if ( ep->objBsp2 && ep->objBsp2->InDisplayList() && _simTime < ep->trailStartTime )
				{
					_viewPoint->RemoveObject( ep->objBsp2 );
				}
				// remove 2d too
				if ( ep->obj2d && ep->obj2d->InDisplayList() )
				{
					_viewPoint->RemoveObject( ep->obj2d );
				}
			}
			continue;
		}

					
		////////////////////////////////////////////////////////////////////////
										
		

		////////////////////////////////////////////////////////////


		// set the scalar value...// put check in if Isometric view
		ep->objBase->drawPointer->SetScale(_tapeObjScale);

		// if the entity is an aircraft we set its labels
		if (ep->flags & ENTITY_FLAG_AIRCRAFT )
		{
			float tmp;
			char tmpstr[32];

			DrawablePoled *dp = (DrawablePoled *)ep->objBase->drawPointer;
				
			
			// LOCK RANGES
			// get the target entity
			Tpoint posb;
			int tgt = GetEntityCurrentTarget(i);
			float distance;
			if (tgt >0)
			{
			SimTapeEntity *targep = GetSimTapeEntity(tgt);
			ep->objBase->drawPointer->GetPosition(&pos);
			targep->objBase->drawPointer->GetPosition(&posb);
			distance =  (float)(FT_TO_NM * (float)sqrt(((pos.x-posb.x)*(pos.x-posb.x) + (pos.y-posb.y)*(pos.y-posb.y))));
			}
			else distance = 0;

			tmp = distance;
			sprintf(tmpstr,"%0.0f Rng",tmp);
			dp->SetDataLabel( DP_LABEL_LOCK_RANGE, tmpstr );
				
				

				{
			// heading
			tmp = ep->yaw * RTD;
			if ( tmp < 0.0f )
				tmp += 360.0f;
			sprintf(tmpstr,"%0.0f Deg",tmp);
			dp->SetDataLabel( DP_LABEL_HEADING, tmpstr );
				}
			// alt
			sprintf(tmpstr,"%0.0f ft", -ep->z);
			dp->SetDataLabel( DP_LABEL_ALT, tmpstr );

			// speed
			// tmp=ep->aveSpeed * FTPSEC_TO_KNOTS;
			tmp=CalcKIAS( ep->aveSpeed, -ep->z );
			sprintf(tmpstr,"%0.0f Kts", tmp);
			dp->SetDataLabel( DP_LABEL_SPEED, tmpstr );

			// turn rate
			tmp=ep->aveTurnRate;
			sprintf(tmpstr,"%0.0f deg/s", tmp);
			dp->SetDataLabel( DP_LABEL_TURNRATE, tmpstr );

			// turn radius
			tmp=ep->aveTurnRadius;
			sprintf(tmpstr,"%0.0f ft", tmp);
			dp->SetDataLabel( DP_LABEL_TURNRADIUS, tmpstr );
		}


		// update object's position
		ep->objBase->SetPosition
		(
			ep->x,
			ep->y,
			ep->z
		);
		ep->objBase->SetYPR
		(
			ep->yaw,
			ep->pitch,
			ep->roll
		);
		// just to make sure....
		ep->objBase->SetYPRDelta
		(
			0.0f,
			0.0f,
			0.0f
		);

		// set the matrix
		CalcTransformMatrix(ep->objBase);

		// get position and matrix for drawable BSP
		ObjectSetData( ep->objBase, &pos, &rot );

		// update the BSP
		if ( ep->flags & ENTITY_FLAG_FLARE )
			((Drawable2D *)ep->objBase->drawPointer)->SetPosition( &pos );
		else
			((DrawableBSP *)ep->objBase->drawPointer)->Update( &pos, &rot );

		////////////////////////////////////////////////////////////////////////////////////

		// BING 3-20-98
		// TURN ON LABELS FOR ENTITIES.
		//	acmiView->GetObjectName(acmiView->Tape()->GetSimTapeEntity(i)->objBase,tmpStr);
	//		((DrawableBSP *)ep->objBase->drawPointer)->SetLabel(ep->name , labelColor );
																																	
		/////////////////////////////////////////////////////////////////////////////////////
			
			
		// entity is in the frame .....
		// make sure we tell draw loop to draw it
		if ( !ep->objBase->drawPointer->InDisplayList() )
		{
			_viewPoint->InsertObject( ep->objBase->drawPointer );
		}

		// likewise for 2d portion
		if ( ep->obj2d )
		{
			if ( !ep->obj2d->InDisplayList() )
				_viewPoint->InsertObject( ep->obj2d );

			ep->obj2d->SetPosition( &pos );
		}

		// do the wing trails if turned on
		// edg: partial hack.  For regen in dogfight the wingtrails are
		// continue from dead pos to new position.  Since we don't have the
		// info to detect a regen, if we see that the airspeed is too high
		// trim the trails back to 0
		if ( _wingTrails && (ep->flags & ENTITY_FLAG_AIRCRAFT ) && CalcKIAS( ep->aveSpeed, -ep->z ) > 1100.0f )
		{
			ep->wrTrail->TrimTrail( 0 );
			ep->wlTrail->TrimTrail( 0 );
			ep->wtLength = 0;
		}
		else if ( _wingTrails && (ep->flags & ENTITY_FLAG_AIRCRAFT ) )
		{
			if ( _playVelocity < 0.0f && (!_paused || _simulateOnly) )
			{
				ep->wtLength -= ep->wrTrail->RewindTrail( (DWORD)(_simTime * 1000) );
				ep->wlTrail->RewindTrail( (DWORD)(_simTime * 1000) );
			}
			else if ( _playVelocity > 0.0f && (!_paused || _simulateOnly) )
			{
				ep->wtLength++;
				wtpos.x = ep->objBase->dmx[1][0] * -20.0f * _tapeObjScale+ ep->x;
				wtpos.y = ep->objBase->dmx[1][1] * -20.0f * _tapeObjScale+ ep->y;
				wtpos.z = ep->objBase->dmx[1][2] * -20.0f * _tapeObjScale+ ep->z;
				ep->wlTrail->AddPointAtHead( &wtpos, (DWORD)(_simTime * 1000) );
				wtpos.x = ep->objBase->dmx[1][0] * 20.0f * _tapeObjScale+ ep->x;
				wtpos.y = ep->objBase->dmx[1][1] * 20.0f * _tapeObjScale+ ep->y;
				wtpos.z = ep->objBase->dmx[1][2] * 20.0f * _tapeObjScale+ ep->z;
				ep->wrTrail->AddPointAtHead( &wtpos, (DWORD)(_simTime * 1000) );

/*				ep->wtLength++;
				wtpos.x = ep->objBase->dmx[1][0] * -40.0f + ep->x;
				wtpos.y = ep->objBase->dmx[1][1] * -40.0f + ep->y;
				wtpos.z = ep->objBase->dmx[1][2] * -40.0f + ep->z;
				ep->wlTrail->AddPointAtHead( &wtpos, (DWORD)(_simTime * 1000) );
				wtpos.x = ep->objBase->dmx[1][0] * 40.0f + ep->x;
				wtpos.y = ep->objBase->dmx[1][1] * 40.0f + ep->y;
				wtpos.z = ep->objBase->dmx[1][2] * 40.0f + ep->z;
				ep->wrTrail->AddPointAtHead( &wtpos, (DWORD)(_simTime * 1000) );
*/		
			
			
			
			}
			else if ( _stepTrail < 0.0f )
			{
				ep->wtLength -= ep->wrTrail->RewindTrail( (DWORD)(_simTime * 1000) );
				ep->wlTrail->RewindTrail( (DWORD)(_simTime * 1000) );
			}
			else if ( _stepTrail > 0.0f )
			{
				ep->wtLength++;
				wtpos.x = ep->objBase->dmx[1][0] * -20.0f * _tapeObjScale+ ep->x;
				wtpos.y = ep->objBase->dmx[1][1] * -20.0f * _tapeObjScale+ ep->y;
				wtpos.z = ep->objBase->dmx[1][2] * -20.0f * _tapeObjScale+ ep->z;
				ep->wlTrail->AddPointAtHead( &wtpos, (DWORD)(_simTime * 1000) );
				wtpos.x = ep->objBase->dmx[1][0] * 20.0f * _tapeObjScale+ ep->x;
				wtpos.y = ep->objBase->dmx[1][1] * 20.0f * _tapeObjScale+ ep->y;
				wtpos.z = ep->objBase->dmx[1][2] * 20.0f * _tapeObjScale+ ep->z;
				ep->wrTrail->AddPointAtHead( &wtpos, (DWORD)(_simTime * 1000) );

			
			}

			if ( ep->wtLength != _wtMaxLength ) // MLR 12/14/2003 - 
			{
				ep->wrTrail->TrimTrail( _wtMaxLength );
				ep->wlTrail->TrimTrail( _wtMaxLength );
				ep->wtLength = _wtMaxLength;
			}
		}

		// check for trail
		if ( !ep->objTrail )
			continue;

		// we need to deal with the trail....

		// if the trail end time is before the read head, we do nothing
		if ( _simTime > ep->trailEndTime )
		{
			// remove Bsp1 too
			if ( ep->objBsp1 && ep->objBsp1->InDisplayList() )
			{
				_viewPoint->RemoveObject( ep->objBsp1 );
			}
			// remove Bsp2 too
			if ( ep->objBsp2 && ep->objBsp2->InDisplayList() )
			{
				_viewPoint->RemoveObject( ep->objBsp2 );
			}
			continue;
		}

		// if the trail start time is after the current read head,
		// make sure trail is no longer in display list
		if ( _simTime < ep->trailStartTime )
		{
			if ( ep->objTrail->InDisplayList() )
			{
				ep->objTrail->TrimTrail( 0 );
				_viewPoint->RemoveObject( ep->objTrail );
			}
			// remove Bsp1 too
			if ( ep->objBsp1 && ep->objBsp1->InDisplayList() )
			{
				_viewPoint->RemoveObject( ep->objBsp1 );
			}
			// remove Bsp2 too
			if ( ep->objBsp2 && ep->objBsp2->InDisplayList() )
			{
				_viewPoint->RemoveObject( ep->objBsp2 );
			}
			continue;
		}

		// the read head is between trail start and end times
		// we need need to determine if we're moving forwards or
		// backwards in time, if back we rewind the trail, otherwise
		// add a new point
		if ( !ep->objTrail->InDisplayList() )
		{
			_viewPoint->InsertObject( ep->objTrail );
		}
		// insert Bsp1 too
		if ( ep->objBsp1 && !ep->objBsp1->InDisplayList() )
		{
			_viewPoint->InsertObject( ep->objBsp1 );
		}
		// insert Bsp2 too
		if ( ep->objBsp2 && !ep->objBsp2->InDisplayList() )
		{
			_viewPoint->InsertObject( ep->objBsp2 );
		}

		// update the BSPs
	    // placement a bit behind the missile
		newPoint = pos;
    	newPoint.x += ep->objBase->dmx[0][0]*-7.0f;
    	newPoint.y += ep->objBase->dmx[0][1]*-7.0f;
    	newPoint.z += ep->objBase->dmx[0][2]*-7.0f;
		if ( ep->objBsp1 )
			ep->objBsp1->Update( &newPoint, &rot  );
		if ( ep->objBsp2 )
			ep->objBsp2->Update( &newPoint, &rot  );
    	newPoint.x += ep->objBase->dmx[0][0]*-30.0f;
    	newPoint.y += ep->objBase->dmx[0][1]*-30.0f;
    	newPoint.z += ep->objBase->dmx[0][2]*-30.0f;

					

		if ( _playVelocity < 0.0f && (!_paused || _simulateOnly) )
		{
			ep->objTrail->RewindTrail( (DWORD)(_simTime * 1000) );
		}
		else if ( _playVelocity > 0.0f && (!_paused || _simulateOnly ) )
		{
			ep->objTrail->AddPointAtHead( &newPoint, (DWORD)(_simTime * 1000) );
		}
		else if ( _stepTrail < 0.0f )
		{
			ep->objTrail->RewindTrail( (DWORD)(_simTime * 1000) );
		}
		else if ( _stepTrail > 0.0f )
		{
			ep->objTrail->AddPointAtHead( &newPoint, (DWORD)(_simTime * 1000) );
		}


	}
	_stepTrail = 0.0;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMITape::ObjectSetData(SimBaseClass *obj, Tpoint *simView, Trotation *viewRotation)
{
   viewRotation->M11 = obj->dmx[0][0];
   viewRotation->M21 = obj->dmx[0][1];
   viewRotation->M31 = obj->dmx[0][2];

   viewRotation->M12 = obj->dmx[1][0];
   viewRotation->M22 = obj->dmx[1][1];
   viewRotation->M32 = obj->dmx[1][2];

   viewRotation->M13 = obj->dmx[2][0];
   viewRotation->M23 = obj->dmx[2][1];
   viewRotation->M33 = obj->dmx[2][2];

   // Update object position
   simView->x     = obj->XPos();
   simView->y     = obj->YPos();
   simView->z     = obj->ZPos();

}

/*
** Makes sure all memory used by events is free'd up
*/
void ACMITape::CleanupEventList( void )
{
	int i;

	F4Assert( _eventList );

	for ( i = 0; i < _tapeHdr.numEvents; i++ )
	{
		if ( _eventList[i] )
		{
			RemoveActiveEvent( &_eventList[i] );
		}
	}

	delete [] _eventList;
	_eventList = NULL;
}

/*
**	Takes the acmi info passed in from the tape and creates an
**	active event record for it, sets up the objects, inserts them
**	into the display list(if needed) and chains it to the active list head
**	dT should be the time delta between start time for event and read head
*/
ActiveEvent *
ACMITape::InsertActiveEvent( ACMIEventHeader *eh, float dT )
{
	ActiveEvent *event = NULL;
	TracerEventData *td = NULL;
	SfxClass *sfx = NULL;
	SimBaseClass *simBase;
	Tpoint pos;
	Tpoint vec;

	// don't insert if passed end time
	if ( eh->time + dT > eh->timeEnd )
		return NULL;

	// creation based on type
	switch( eh->eventType )
	{
		case ACMIRecTracerStart:
			// create new event record
			event = new ActiveEvent;
			F4Assert( event );
			event->eventType = eh->eventType;
			event->index = eh->index;
			event->time = eh->time;
			event->timeEnd = eh->timeEnd;

			// create new tracer event record
			td = new TracerEventData;
			F4Assert( td );
			event->eventData = (void *)td;

			// init tracer data
			td->x = eh->x;
			td->y = eh->y;
			td->z = eh->z;
			td->dx = eh->dx;
			td->dy = eh->dy;
			td->dz = eh->dz;
			// create tracer
			td->objTracer = new DrawableTracer( 1.3f );
			td->objTracer->SetAlpha( 0.8f );
			td->objTracer->SetRGB( 1.0f, 1.0f, 0.2f );

			UpdateTracerEvent( td, dT );

			// put it into the draw list
			_viewPoint->InsertObject( td->objTracer );

			break;

		case ACMIRecStationarySfx:
			// create new event record
			event = new ActiveEvent;
			F4Assert( event );
			event->eventType = eh->eventType;
			event->index = eh->index;
			event->time = eh->time;
			event->timeEnd = eh->timeEnd;

			pos.x = eh->x;
			pos.y = eh->y;
			pos.z = eh->z;

			// create new tracer event record
			sfx = new SfxClass( eh->type,
								&pos,
								(float)(eh->timeEnd - eh->time),
								eh->scale );

			F4Assert( sfx );
			event->eventData = (void *)sfx;

			sfx->ACMIStart( _viewPoint, event->time, _simTime );

			break;

		case ACMIRecMovingSfx:
			// create new event record
			event = new ActiveEvent;
			F4Assert( event );
			event->eventType = eh->eventType;
			event->index = eh->index;
			event->time = eh->time;
			event->timeEnd = eh->timeEnd;

			pos.x = eh->x;
			pos.y = eh->y;
			pos.z = eh->z;
			vec.x = eh->dx;
			vec.y = eh->dy;
			vec.z = eh->dz;

			// create new sfx 
			if ( eh->user < 0 )
			{
				sfx = new SfxClass( eh->type,
								eh->flags,
								&pos,
								&vec,
								(float)(eh->timeEnd - eh->time),
								eh->scale );
			}
			else {
				// we need to build a base obj first
				simBase = new SimStaticClass(0);// SimBaseClass( 0 );
				simBase->drawPointer = new DrawableBSP( eh->user, &pos, &IMatrix, 1.0f );
				simBase->SetPosition(pos.x, pos.y, pos.z);
				simBase->SetDelta(vec.x, vec.y, vec.z);
				simBase->SetYPR(0.0f, 0.0f, 0.0f);
				simBase->SetYPRDelta(0.0f, 0.0f, 0.0f);
				sfx = new SfxClass( eh->type, eh->flags, simBase, (float)(eh->timeEnd - eh->time), eh->scale);
			}

			F4Assert( sfx );
			event->eventData = (void *)sfx;

			sfx->ACMIStart( _viewPoint, event->time, _simTime );

			break;

		// current don't handle anything else
		default:
			return NULL;
	}

	// now insert it into the active list
	event->prev = NULL;
	if ( _activeEventHead )
	{
		_activeEventHead->prev = event;
	}

	event->next = _activeEventHead;
	_activeEventHead = event;

	return event;
}


/*
**	Removes objects from display lists
**	Frees memory for any objects.
**	Frees memory for ActiveEvent and event data
*/
void
ACMITape::RemoveActiveEvent( ActiveEvent **eptrptr )
{
	ActiveEvent *event = *eptrptr;
	TracerEventData *td = NULL;
	SfxClass *sfx;

	// deletion based on type
	switch( event->eventType )
	{
		case ACMIRecTracerStart:

			// cast eventData to appropriate type
			td = (TracerEventData *)event->eventData;

			// remove from draw list
			if ( td->objTracer->InDisplayList() )
				_viewPoint->RemoveObject( td->objTracer );

			// free data memory
			delete td->objTracer;
			delete td;

			break;

		case ACMIRecMovingSfx:
		case ACMIRecStationarySfx:

			// cast eventData to appropriate type
			sfx = (SfxClass *)event->eventData;

			// free data memory
			delete sfx;

			break;

		// current don't handle anything else
		default:
			return;
	}

	// take event out of active Event List
	if ( event->prev )
		event->prev->next = event->next;
	else
		_activeEventHead = event->next;

	if ( event->next )
		event->next->prev = event->prev;

	// delete event data and set caller's pointer to NULL
	delete event;
	*eptrptr =NULL;
}

/*
** Update tracer info based on delta Time
*/
void
ACMITape::UpdateTracerEvent( TracerEventData *td, float dT )
{
	Tpoint pos, end;

	pos.x = td->x + td->dx * dT;
	pos.y = td->y + td->dy * dT;
	pos.z = td->z + td->dz * dT;

	end.x = pos.x - td->dx * 0.05f;
	end.y = pos.y - td->dy * 0.05f;
	end.z = pos.z - td->dz * 0.05f;

	td->objTracer->Update( &pos, &end );
}


/*
**	Run the update cycle for all active events
*/
void
ACMITape::UpdateActiveEvents( void )
{
	ActiveEvent *event = NULL;
	TracerEventData *td = NULL;
	SfxClass *sfx;

	event = _activeEventHead;

	while( event )
	{

		// handle based on type
		switch( event->eventType )
		{
			case ACMIRecTracerStart:

				// deref eventData
				td = (TracerEventData *)event->eventData;
				UpdateTracerEvent( td, _simTime - event->time );
	
				// remove from display list if event no longer exists
				// blech, this is a very less than optimal solution
				// the active event list is going to bloat over time
				/*
				if ( _simTime > event->timeEnd || event->time > _simTime )
				{
					if ( td->objTracer->InDisplayList() )
						_viewPoint->RemoveObject( td->objTracer );
				}
				else
				{
					// the event is active....
					if ( !td->objTracer->InDisplayList() )
						_viewPoint->InsertObject( td->objTracer );
					UpdateTracerEvent( td, _simTime - event->time );
				}
				*/
				break;
		case ACMIRecMovingSfx:
		case ACMIRecStationarySfx:

				// deref eventData
				sfx = (SfxClass *)event->eventData;
				sfx->ACMIExec( _simTime );

				break;
	
			// currently don't handle anything else
			default:
				break;
		}

		event = event->next;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
** Description:
**		Sets the wing trails
*/
void ACMITape::SetWingTrails( BOOL turnOn )
{
	int				
		i,
		numEntities;

	SimTapeEntity *ep;

	F4Assert(_simTapeEntities != NULL);
	F4Assert(_tape != NULL );

	// set flag
	_wingTrails = turnOn;

	// create array of SimTapeEntity
	numEntities = NumEntities();

	// for each entity, create it's object stuff....
	for(i = 0; i < numEntities; i++)
	{
		// get the tape entity data
		ep = &_simTapeEntities[i];

		if ( !(ep->flags & ENTITY_FLAG_AIRCRAFT) )
		{
			continue;
		}

		if ( turnOn )
		{
			// turn trails on
			if ( !ep->wrTrail->InDisplayList() )
				_viewPoint->InsertObject( ep->wrTrail );
			if ( !ep->wlTrail->InDisplayList() )
				_viewPoint->InsertObject( ep->wlTrail );
		}
		else
		{
			// turn trails off
			if ( ep->wrTrail->InDisplayList() )
				_viewPoint->RemoveObject( ep->wrTrail );
			if ( ep->wlTrail->InDisplayList() )
				_viewPoint->RemoveObject( ep->wlTrail );
		}

		ep->wrTrail->TrimTrail( 0 );
		ep->wlTrail->TrimTrail( 0 );
		ep->wtLength = 0;

	}

}

/*
 * append new node to end of list
 * caller should cast returned value to appropriate type
 */

LIST *
AppendToEndOfList( LIST * list, LIST **end, void * node )
{
   LIST * newnode;

   newnode = new LIST;

   newnode -> node = node;
   newnode -> next = NULL;

   /* list was null */
   if ( !list ) 
   {
     list = newnode;
	 *end = list;
   }
   else 
   {
      /* chain in at end */
      (*end) -> next = newnode;
	  *end = newnode;
   }

   return( list );
}


/*
 * destroy a list
 * optionally free the data pointed to by node, using supplied destructor fn
 * If destructor is NULL, node data not affected, only list nodes get freed
 */

void 
DestroyTheList( LIST * list )
{
   LIST * prev,
        * curr;

   if ( !list )
      return;

   prev = list;
   curr = list -> next;

   while ( curr )
   {
      // if ( destructor )
      //    (*destructor)(prev -> node);

	  delete prev->node;

      prev -> next = NULL;

      delete prev;

      prev = curr;
      curr = curr -> next;
   }

   // if( destructor )
   //    (*destructor)( prev -> node );

   delete prev->node;

   prev -> next = NULL;

   delete prev;

   //ListGlobalPack();
}

extern EventElement *ProcessEventListForACMI( void );
extern void ClearSortedEventList( void );

/*
** Description:
**		Reads the event file and writes out associated text events with
**		the tape.
*/
void
ACMITape::ImportTextEventList( FILE *fd, ACMITapeHeader *tapeHdr )
{
	EventElement *cur;
	long ret;
	ACMITextEvent te;
	char timestr[20];

	tapeHdr->numTextEvents = 0;

	cur = ProcessEventListForACMI();

	memset(&te,0,sizeof(ACMITextEvent));

	// PJW Totally rewrote event debriefing stuff... thus the new code
	while ( cur )
	{
		te.intTime = cur->eventTime;
		GetTimeString(cur->eventTime, timestr);

		_tcscpy(te.timeStr,timestr + 3);
		_tcscpy(te.msgStr,cur->eventString);

		// KCK: Edit out some script info which is used in debreiefings
		_TCHAR	*strptr = _tcschr(te.msgStr,'@');
		if (strptr)
		{
			strptr[0] = ' ';
			strptr[1] = '-';
			strptr[2] = ' ';
		}

		ret = fwrite( &te, sizeof( ACMITextEvent ), 1, fd );
		if ( !ret )
		{
			MonoPrint( "Error writing TAPE event element\n" );
			break;
		}
		tapeHdr->numTextEvents++;

		// next one
		cur = cur->next;

	} // end for events loop


	// write callsign list
	if(Import_Callsigns)
	{
		ret = fwrite(&import_count, sizeof(long),1, fd );
		if ( !ret )
	 		goto error_exit;

		ret = fwrite(Import_Callsigns, import_count * sizeof(ACMI_CallRec),1, fd );
		if ( !ret )
	 		goto error_exit;
	}

	// write the header again (bleck)
	ret = fseek( fd, 0, SEEK_SET );
	if ( ret )
	{
		MonoPrint( "Error seeking TAPE start\n" );
		goto error_exit;
	}
	ret = fwrite( tapeHdr, sizeof( ACMITapeHeader ), 1, fd );
	if ( !ret )
	{
		MonoPrint( "Error writing TAPE header again\n" );
	}

error_exit:
	// free up mem
	// DisposeEventList(evList);
	ClearSortedEventList();

}


/*
** Description:
**		returns pointer to 1st text event element and the count of
**		elements
*/
void *
ACMITape::GetTextEvents( int *count )
{
	if (_tapeHdr.numTextEvents > 1048576) // Sanity check
	{
		count = 0;
		return NULL;
	}

	*count = _tapeHdr.numTextEvents;
	return (void *)((char *)_tape + _tapeHdr.firstTextEventOffset);
}

void *ACMITape::GetCallsignList(long *count)
{
	if (_tapeHdr.numTextEvents > 1048576) // Sanity check
	{
		count = 0;
		return NULL;
	}

	*count = (long)(*(long*)((char*)_tape + _tapeHdr.firstTextEventOffset + _tapeHdr.numTextEvents * sizeof(ACMITextEvent)));
	return((void *)((char*)_tape + _tapeHdr.firstTextEventOffset + _tapeHdr.numTextEvents * sizeof(ACMITextEvent)+sizeof(long)));
}

/*
** Name: CreateFeatureDrawable
** Description:
**		Creates and/or updates the drawpointer for a feature.
**		This function will also insert the drawpointer in the viewpoint
**		display list or into the drawpointer of another object if it's
**		a composite object like a bridge or airbase.
**		Rewritten from CreateDrawable in OTWDriver.
*/
void ACMITape::CreateFeatureDrawable ( SimTapeEntity *feat )
{
	short    visType = -1;
	Tpoint    simView;
	Trotation viewRotation;
	SimBaseClass* baseObject;
	DrawableObject* lastPointer = NULL;
	

	// get the object and pointer to its classtbl entry
	SimBaseClass* theObject = feat->objBase;
	Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)theObject->EntityType();

	// Set position and orientations
	viewRotation.M11 = theObject->dmx[0][0];
	viewRotation.M21 = theObject->dmx[0][1];
	viewRotation.M31 = theObject->dmx[0][2];

	viewRotation.M12 = theObject->dmx[1][0];
	viewRotation.M22 = theObject->dmx[1][1];
	viewRotation.M32 = theObject->dmx[1][2];

	viewRotation.M13 = theObject->dmx[2][0];
	viewRotation.M23 = theObject->dmx[2][1];
	viewRotation.M33 = theObject->dmx[2][2];

	// Update object position
	simView.x     = theObject->XPos();
	simView.y     = theObject->YPos();
	simView.z     = theObject->ZPos();

	visType = classPtr->visType[theObject->Status() & VIS_TYPE_MASK];

	// make sure things are sane
	F4Assert(visType >= 0 || theObject->drawPointer);
	F4Assert(classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND );
	F4Assert(classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_FEATURE);

	// A feature thingy..
	SimBaseClass	*prevObj = NULL, *nextObj = NULL;

	// In many cases, our visType should be modified by our neighbors.
	if ((theObject->Status() & VIS_TYPE_MASK) != VIS_DESTROYED &&
	     (((SimFeatureClass*)theObject)->featureFlags & FEAT_NEXT_NORM ||
		 ((SimFeatureClass*)theObject)->featureFlags & FEAT_PREV_NORM))
	{
		int	idx = feat->slot;

		prevObj = FindComponentFeature(feat->leadIndex, idx - 1);
		nextObj = FindComponentFeature(feat->leadIndex, idx + 1);
		if (prevObj &&
		   (((SimFeatureClass*)theObject)->featureFlags & FEAT_PREV_NORM) &&
		   (prevObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED)
		{
			if (nextObj &&
			   (((SimFeatureClass*)theObject)->featureFlags & FEAT_NEXT_NORM) &&
			   (nextObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED)
			{
				visType = classPtr->visType[VIS_BOTH_DEST];
			}
			else
			{
				visType = classPtr->visType[VIS_LEFT_DEST];
			}
		}
		else if (nextObj &&
		      (((SimFeatureClass*)theObject)->featureFlags & FEAT_NEXT_NORM) &&
			  (nextObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED)
		{
			visType = classPtr->visType[VIS_RIGHT_DEST];
		}
	}

	// Check for change - and don't bother if there is none.
	if (theObject->drawPointer &&
	   ((DrawableBSP*)theObject->drawPointer)->GetID() == visType)
		return;

	if (theObject->drawPointer && theObject->drawPointer->InDisplayList() )
	{
		// KCK: In some cases we still need this pointer (specifically
		// when we replace bridge segments), so let's save it here - we'll
		// toss it out after we're done.
		lastPointer = theObject->drawPointer;
		theObject->drawPointer = NULL;
	}

	// get the lead baseobject if any
	// otherwise set base object to ourself
	if ( feat->leadIndex >= 0 )
		baseObject = _simTapeFeatures[ feat->leadIndex ].objBase;
	else
		baseObject = theObject;

	// Some things require Base Objects (like bridges and airbases)
	if (!((SimFeatureClass*)baseObject)->baseObject)
	{
		// Is this a bridge?
		if (theObject->IsSetCampaignFlag(FEAT_ELEV_CONTAINER))
		{
			// baseObject is the "container" object for all parts of the bridge
			// There is only one container for the entire bridge, stored in the lead element
			((SimFeatureClass*)baseObject)->baseObject = new DrawableBridge (1.0F);

			// Insert only the bridge drawable.
			_viewPoint->InsertObject (((SimFeatureClass*)baseObject)->baseObject);
		}
		// Is this a big flat thing with things on it (like an airbase?)
		else if (theObject->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
		{
			// baseObject is the "container" object for all parts of the platform
			// There is only one container for the entire platform, stored in the 
			// lead element.
			((SimFeatureClass*)baseObject)->baseObject = new DrawablePlatform (1.0F);

			// Insert only the platform drawable.
			_viewPoint->InsertObject (((SimFeatureClass*)baseObject)->baseObject);
		}
	}

	// Add another building to this grouping of buildings, or replace the drawable
	// of one which is here.
	// Is the container a bridge?
	if (baseObject->IsSetCampaignFlag(FEAT_ELEV_CONTAINER))
	{
		// Make the new BRIDGE object
		if (visType)
		{
			if (theObject->IsSetCampaignFlag(FEAT_NEXT_IS_TOP) && theObject->Status() != VIS_DESTROYED)
				theObject->drawPointer = new DrawableRoadbed(visType, visType+1, &simView, theObject->Yaw(), 10.0f, (float)atan(20.0f/280.0f) );
			else
				theObject->drawPointer = new DrawableRoadbed(visType, -1, &simView, theObject->Yaw(), 10.0f, (float)atan(20.0f/280.0f) );
		}
		else
			theObject->drawPointer = NULL;

		// Check for replacement
		if (lastPointer)
		{
			ShiAssert(lastPointer->GetClass() == DrawableObject::Roadbed);
			ShiAssert(theObject->drawPointer->GetClass() == DrawableObject::Roadbed);
			((DrawableBridge*)(((SimFeatureClass*)baseObject)->baseObject))->ReplacePiece ((DrawableRoadbed*)(lastPointer), (DrawableRoadbed*)(theObject->drawPointer));
		}
		else if (theObject->drawPointer)
		{
			ShiAssert(theObject->drawPointer->GetClass() == DrawableObject::Roadbed);
			((DrawableBridge*)(((SimFeatureClass*)baseObject)->baseObject))->AddSegment((DrawableRoadbed*)(theObject->drawPointer));
		}
	}
	// Is the container a big flat thing (airbase)?
	else if (baseObject->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
	{
		// Everything on a platform is a Building
		// That means it sticks straight up the -Z axis
		theObject->drawPointer = new DrawableBuilding(visType, &simView, theObject->Yaw(), 1.0F);

		// Am I Flat (can things drive across it)?
		if (theObject->IsSetCampaignFlag((FEAT_FLAT_CONTAINER | FEAT_ELEV_CONTAINER)))
			((DrawablePlatform*)((SimFeatureClass*)baseObject)->baseObject)->InsertStaticSurface (((DrawableBuilding*)theObject->drawPointer));
		else
			((DrawablePlatform*)((SimFeatureClass*)baseObject)->baseObject)->InsertStaticObject (theObject->drawPointer);
	}
	else 
	{
		// if we get here then this is just a loose collection of buildings, like a
		// village or city, with no big flat objects between them
		theObject->drawPointer = new DrawableBuilding(visType, &simView, theObject->Yaw(), 1.0F);

		// Insert the object
		_viewPoint->InsertObject(((SimFeatureClass*)theObject)->drawPointer);
	}
	// KCK: Remove any previous drawable object
	if (lastPointer)
	{
		if ( lastPointer->InDisplayList() )
			_viewPoint->RemoveObject( lastPointer );
		delete lastPointer;
	}
}
 
/*
** Name: FindComponentFeature
** Description:
**		Tries to find the feature with the leadindex and slot
**		Passed in.
*/
SimBaseClass *ACMITape::FindComponentFeature ( long leadIndex, int slot)
{
	int i;

	if ( leadIndex < 0 || slot < 0 )
		return NULL;

	for(i = 0; i < _tapeHdr.numFeat; i++)
	{
		if ( _simTapeFeatures[i].leadIndex == leadIndex &&
			 _simTapeFeatures[i].slot == slot )
		{
			return _simTapeFeatures[i].objBase;
		}
	}
	return NULL;
}
