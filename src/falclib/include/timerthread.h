#ifndef     _TIMER_THREAD_H_
#define     _TIMER_THREAD_H_

#include    "Falclib.h"

#define NO_TIMER_THREAD	1

#define MAX_TIME_DELTA				500			// Never process more than 500 ms per frame
#define RESYNC_TIME					2000		// Resync time every 2 seconds
#define COMPRESSION_TIME			4000		// Change compression ratio every 4 seconds
#define REMOTE_LOOKAHEAD_TIME		300000		// Lookahead time on clients

#define REMOTE_REQUEST_PAUSE		0x01
#define REMOTE_REQUEST_2			0x02
#define REMOTE_REQUEST_4			0x04
#define REMOTE_REQUEST_8			0x08
#define REMOTE_REQUEST_16			0x10
#define REMOTE_REQUEST_32			0x20
#define REMOTE_REQUEST_64			0x40
#define REMOTE_REQUEST_128		0x80
#define REMOTE_REQUEST_256		0x100
#define REMOTE_REQUEST_512		0x200
#define REMOTE_REQUEST_1024		0x400

// external functions and variables
extern void             RealTimeFunction( unsigned long, void * );
extern void				SetTimeCompressionFromRemote( int );
extern void             SetTimeCompression( int );
extern void             SetTemporaryCompression( int );
extern void             SetTime( unsigned long );

extern ulong            gCompressTillTime;
extern unsigned long    vuxRealTime;
extern unsigned long    vuxGameTime;
extern unsigned long    vuxTargetGameTime;
extern unsigned long    vuxLastTargetGameTime;
extern unsigned long    vuxDeadReconTime;
extern unsigned long	lastTimingMessage;
extern unsigned long    lastStartTime;
extern int              gameCompressionRatio;
extern int				targetGameCompressionRatio;
extern int              targetCompressionRatio;
extern int				remoteCompressionRequests;

#ifndef NO_TIMER_THREAD
#define                 THREAD_TIME_SLICE       20

extern void             beginTimer( void );
extern void             endTimer( void );
#endif		// NO_TIMER_THREAD

extern void SetTimeCompression( int newComp );
extern void SetOnlineTimeCompression( int newComp );
extern void SetTemporaryCompression( int newComp );
extern void SetTime( unsigned long currentTime );

#endif      // _TIMER_THREAD_H_
