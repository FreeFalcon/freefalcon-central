#ifndef REDPROFILER_H
#define REDPROFILER_H

//****************************************************************************
//* RED Profiler Definitions and Macros                                      *
//****************************************************************************


#define	_USE_RED_PROFILER_

#define	USE_NEW_PS												// Enables New PS System
#define	DEBUG_NEW_PS											// Enabled profiler of NEW PS
#define	DEBUG_2D_ENGINE

#ifdef	DEBUG_NEW_PS

	// NEW PS SUB PARTS DEBUG DEFINITIONS
	//#define	DEBUG_NEW_PS_LIGHTS
	//#define	DEBUG_NEW_PS_EMITTERS
	//#define	DEBUG_NEW_PS_POLYS
	#define	DEBUG_NEW_PS_TRAILS

	//#define	DEBUG_NEW_PS_SOUNDS
	//#define	DEBUG_NEW_PS_PARTICLESS
	//#define	DEBUG_NEW_PS_CLUSTERS
#ifdef	DEBUG_NEW_PS_CLUSTERS
	#define	VISUALIZE_CLUSTERS
#endif

#endif



//#define	DEBUG_PS_ID												// Enables PS name-on-screen feature


//#define	LIGHT_ENGINE_DEBUG										// Enabled LIGHT ENGINE DEBUG REPORTS - 
																// GOT ONLY SENSE ITH 1 SPOT LIGHT IN THE SCENE

//#define	DEBUG_AI_BEHAVIOUR									// Enables DEBUG MESSAGES ABOUT AI BEHAVIOUR

// IF DEFINED, THIS WILL ENABLE LOD ID INSTEAD OF LABELS ON ALL OBJECTS
//#define	DEBUG_LOD_ID


#define	TIME_OUT		5										// Default Time On display in Seconds
#define	MAX_PROFILES	16										// Max Allowable Profiles at same time
#define	MAX_MESSAGES	16
typedef	enum	{ EMPTY_PROF=0, TIME_PROF, COUNT_PROF, VALUE_PROF, MAIN_PROF, CUMULATIVE_PROF } ProfileType;

typedef	struct {
	ProfileType	Type;
	__int64	Start, Stop, Average, Total;
	char	ProName[128];
	int		TimeOut;
	int		Level;
} RedProfItemType;

typedef	struct {
	ProfileType	Type;
	__int64	Result;
	char	ProName[128];
	int		TimeOut;
} RedDisplayType;

#ifdef	_USE_RED_PROFILER_
extern	RedProfItemType	RedProfiles[MAX_PROFILES];
#define	MAIN_PROFILE(x)		StartMainProfile(x)
#define	START_PROFILE(x)	StartProfile(x)
#define	STOP_PROFILE(x)		StopProfile(x)
#define	REPORT_PROFILE(n, x)	ReportProfile(n, x)
#define	REPORT_PROFILE_NR(n, x)	ReportProfileNr(n, x)
#define	COUNT_PROFILE(x)	ProfileCount(x)
#define	REPORT_VALUE(x,y)	ReportValue(x,(__int64)y)
#define	LIST_PROFILES		ProfileBuildList(); FixMainProfile();

#define	DEBUG_MESSAGE(Slot, Message)	ProfileMex(Slot, Message)
#define	REPORT_MESSAGE(Slot, dest)		ReportMessageNr(Slot, dest)
	
void	ProfileBuildList(void);
void	StartProfile(char *name);
void	StartMainProfile(char *name);
void	StopProfile(char *name);
void	ProfileCount(char *name);
void	ReportProfile(char *name, char *dest);
bool	ReportProfileNr(int Num, char *dest);
void	ReportValue(char *name, __int64 Value);
void	FixMainProfile(void);
void	ReportMessageNr(int  Num, char *dest );
void	ProfileMex(int Slot, const char *Message);

#else	
#define	MAIN_PROFILE(x)
#define	START_PROFILE(x)
#define	COUNT_PROFILE(x)	
#define	STOP_PROFILE(x)	
#define	REPORT_PROFILE(n, x)
#define	REPORT_PROFILE_NR(n, x)
#define	REPORT_VALUE(x,y)
#define	LIST_PROFILES	
#define	DEBUG_MESSAGE(Slot, Message)
#define	REPORT_MESSAGE(Slot, dest)
#endif
	

#ifdef	DEBUG_AI_BEHAVIOUR
#define	AI_MESSAGE(s,x)	DEBUG_MESSAGE(s,x)
#else
#define	AI_MESSAGE(s,x)
#endif


#define	DX_ENGINE_PROF	"*** DX  ENGINE"
#define	BSP_ENGINE_PROF	">>> BSP ENGINE"



#endif