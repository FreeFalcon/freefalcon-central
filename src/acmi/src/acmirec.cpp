/*
** Name: ACMIREC.CPP
** Description:
**		Member functions and globals for the ACMIRecorder Class
** History:
**		13-oct-97 (edg)
**			We go dancing in.....
*/
#pragma optimize( "", off )
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

//#include "stdhdr.h"
#include "vu2.h"
#include "acmirec.h"
#include "sim\include\simdrive.h"
#include "timerthread.h"
//#include "simlib.h"
//#include "otwdrive.h"
#include "playerop.h"


// the global recorder
ACMIRecorder		gACMIRec;

// no more recording once an error is hit
BOOL gACMIRecError = FALSE;

// visible display in otwdrive for pct tape full
extern char gAcmiStr[11];

void InitACMIIDTable();
void CleanupACMIIDTable();

/*
** The constructor
*/
ACMIRecorder::ACMIRecorder( void )
{
	_fd = NULL;
	_csect = F4CreateCriticalSection("acmi");
	_recording = FALSE;

	// edg: We Need to get this from player options!!!!
	// at the moment there doesn't seem to be a value for this in the class
	// default to 5 meg
	_maxBytesToWrite = (float)PlayerOptions.AcmiFileSize()*1024*1024;

// OW BC
#if 1
	HANDLE handle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindFileData; 
	char	path[MAX_PATH] = "";

	handle = FindFirstFile("acmibin\\*.flt", &FindFileData);

	if(handle != INVALID_HANDLE_VALUE) {
		strcpy(path, "acmibin\\");
		strcat(path, FindFileData.cFileName);
		DeleteFile(path);

		while(FindNextFile(handle,  &FindFileData)) {
			strcpy(path, "acmibin\\");
			strcat(path, FindFileData.cFileName);
			DeleteFile(path);
		}
		FindClose(handle);
	}
#else
	HANDLE handle = INVALID_HANDLE_VALUE;
	LPWIN32_FIND_DATA lpFindFileData = NULL; 
	char	path[MAX_PATH] = "";

	handle = FindFirstFile("acmibin\\*.flt", lpFindFileData);

	if(handle != INVALID_HANDLE_VALUE) {
		strcpy(path, "acmibin\\");
		strcat(path, lpFindFileData->cFileName);
		DeleteFile(path);

		while(FindNextFile(handle,  lpFindFileData)) {
			strcpy(path, "acmibin\\");
			strcat(path, lpFindFileData->cFileName);
			DeleteFile(path);
		}
		FindClose(handle);
	}
#endif
}

/*
** The destructer
*/
ACMIRecorder::~ACMIRecorder( )
{
	if ( _fd )
		fclose( _fd );
	_fd = NULL;
	F4DestroyCriticalSection(_csect);
	_csect = NULL; // JB 010108
}

/*
** StartRecording
*/
void
ACMIRecorder::StartRecording( void )
{
	char fname[MAX_PATH];
	int y;
	FILE *fp;

	InitACMIIDTable();

	// init the display string
	strcpy( gAcmiStr, "----------" );

	// if we're hit a write error, no more recording...
	if ( gACMIRecError == TRUE )
		return;

	// set our max file size now
	_maxBytesToWrite = 1000000.0f * PlayerOptions.ACMIFileSize;

	// find a suitable name for flight file
	for ( y = 0; y < 10000; y++ )
	{
		sprintf( fname, "acmibin\\acmi%04d.flt", y );

		fp = fopen( fname, "r" );
		if ( !fp )
		{
			break;
		}
		else
		{
			fclose( fp );
		}
	}

	_fd = fopen( fname, "wb" );

	if (_fd)
	{
		// initialize the bytes written
		_bytesWritten = 0.0f;

		// this is where a call to simdriver needs to go to initialize
		// objects
		SimDriver.InitACMIRecord();

		_recording = TRUE;

		MonoPrint( "ACMI Recording, File Size = %f\n", _maxBytesToWrite );
	}
}

/*
** StopRecording
*/
void
ACMIRecorder::StopRecording( void )
{
	long i,count;
	unsigned long idx;
	ACMI_HASHNODE *rec;
	ACMI_CallRec *list;
	ACMIRecHeader  hdr;


	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		_recording = FALSE;
		F4LeaveCriticalSection( _csect );
		return;
	}

	// Write out the Callsign/Color list

	count=ACMIIDTable->GetLastID();
	if(count > 0)
	{
		list=new ACMI_CallRec[count];
		memset(list,0,sizeof(ACMI_CallRec)*count);

		hdr.type=ACMICallsignList;
		hdr.time=0.0f;
		fwrite(&hdr,sizeof(ACMIRecHeader),1,_fd);

		fwrite(&count,sizeof(long),1,_fd);

		i=ACMIIDTable->GetFirst(&rec,&idx);
		while(rec && i >=0 && i < count)
		{
			strncpy(list[i].label,rec->label,15);
			list[i].teamColor=rec->color;

			i=ACMIIDTable->GetNext(&rec,&idx);
		}
		fwrite(list,sizeof(ACMI_CallRec)*count,1,_fd);
	}

	fclose( _fd );
	_fd = NULL;
	_recording = FALSE;

	CleanupACMIIDTable();

	F4LeaveCriticalSection( _csect );

	MonoPrint( "ACMI Stopped Recording\n" );
}

/*
** Write a tracer start record
*/
void
ACMIRecorder::TracerRecord( ACMITracerStartRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecTracerStart;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;


	if (!fwrite( recp, sizeof( ACMITracerStartRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMITracerStartRecord ));

	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a General Position record
*/
void
ACMIRecorder::GenPositionRecord( ACMIGenPositionRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecGenPosition;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

// FIX	*(recp->data.label) = NULL;

	if (!fwrite( recp, sizeof( ACMIGenPositionRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIGenPositionRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a General Position record
*/
void
ACMIRecorder::FeaturePositionRecord( ACMIFeaturePositionRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecFeaturePosition;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMIFeaturePositionRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIFeaturePositionRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a Feature Status record
*/
void
ACMIRecorder::FeatureStatusRecord( ACMIFeatureStatusRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecFeatureStatus;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMIFeatureStatusRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIFeatureStatusRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}


/*
** Write a General Position record
*/
void
ACMIRecorder::MissilePositionRecord( ACMIMissilePositionRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecMissilePosition;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

// FIX	*(recp->data.label) = NULL;

	if (!fwrite( recp, sizeof( ACMIMissilePositionRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}


	_bytesWritten += (float)(sizeof( ACMIMissilePositionRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a Stationary Sfx record
*/
void
ACMIRecorder::StationarySfxRecord( ACMIStationarySfxRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecStationarySfx;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMIStationarySfxRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIStationarySfxRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a Moving Sfx record
*/
void
ACMIRecorder::MovingSfxRecord( ACMIMovingSfxRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecMovingSfx;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMIMovingSfxRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIMovingSfxRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a switch data record
*/
void
ACMIRecorder::SwitchRecord( ACMISwitchRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecSwitch;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMISwitchRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMISwitchRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a DOF data record
*/
void
ACMIRecorder::DOFRecord( ACMIDOFRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecDOF;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMIDOFRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIDOFRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}


/*
** Write a General Position record
*/
void
ACMIRecorder::AircraftPositionRecord( ACMIAircraftPositionRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecAircraftPosition;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

	if (!fwrite( recp, sizeof( ACMIAircraftPositionRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIAircraftPositionRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** ToggleRecording
*/
void
ACMIRecorder::ToggleRecording( void )
{
	F4EnterCriticalSection( _csect );
	if ( IsRecording() )
		StopRecording();
	else
		StartRecording();
	F4LeaveCriticalSection( _csect );
}

/*
** PercentTapeFull
**		Returns a number in the 0 - 10 range
*/
int
ACMIRecorder::PercentTapeFull( void )
{
	return (int)(10.0f * ( _bytesWritten/_maxBytesToWrite ));
}

/*
** Write a General Position record
*/
void
ACMIRecorder::ChaffPositionRecord( ACMIChaffPositionRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecChaffPosition;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

// FIX	*(recp->data.label) = NULL;
// FIX	recp->data.teamColor = 0x0;

	if (!fwrite( recp, sizeof( ACMIChaffPositionRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIChaffPositionRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a General Position record
*/
void
ACMIRecorder::FlarePositionRecord( ACMIFlarePositionRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecFlarePosition;
	// recp->hdr.time = (float)(vuxGameTime/1000) + OTWDriver.todOffset;

// FIX	*(recp->data.label) = NULL;
// FIX	recp->data.teamColor = 0x0;
	
	if (!fwrite( recp, sizeof( ACMIFlarePositionRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMIFlarePositionRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}

/*
** Write a General Position record
*/
void
ACMIRecorder::TodOffsetRecord( ACMITodOffsetRecord *recp )
{
	F4EnterCriticalSection( _csect );
	if ( !_fd )
	{
		F4LeaveCriticalSection( _csect );
		return;
	}

	recp->hdr.type = (BYTE)ACMIRecTodOffset;

	if (!fwrite( recp, sizeof( ACMITodOffsetRecord ), 1, _fd ) )
	{
		StopRecording();
		gACMIRecError = TRUE;
	}

	_bytesWritten += (float)(sizeof( ACMITodOffsetRecord ));
	// check our file size and automaticly start a new recording
	if ( _bytesWritten >= _maxBytesToWrite )
	{
		StopRecording();
		// this tells simdrive to toggle recording at the appropriate time
		SimDriver.doFile = TRUE;
		MonoPrint( "ACMI Recording starting new tape\n" );
	}

	F4LeaveCriticalSection( _csect );
}
