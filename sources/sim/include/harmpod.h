#ifndef _HARMPOD_H
#define _HARMPOD_H

#include "rwr.h"
#include "fcc.h"

class FalconPrivateList;
class ALR56Class;
class GroundListElement;

#define HTS_DISPLAY_RADIUS	1.4F
#define HAS_DISPLAY_RADIUS	0.75F
#define HTS_CURSOR_RATE		0.50F
#define HTS_Y_OFFSET		-0.5F

// RV - I-Hawk - added constants
#define	ALICSIDE	 0.75f
#define	ALICTOP		 1.05f
#define	ALICBOTTOM	 0.0f

#define MAX_POS_TARGETS 4
#define MAX_DTSB_TARGETS 5

class HarmTargetingPod : public RwrClass
{
  public:
	void Display (VirtualDisplay* activeDisplay);
	HarmTargetingPod(int idx, SimMoverClass* newPlatform);
	virtual ~HarmTargetingPod(void);
	virtual void  GetAGCenter (float* x, float* y);	// Center of rwr ground search

// Stuff isn't used anyway

//#if 0
//	class ListElement
//	{
//	  public:
//		ListElement(FalconEntity *newEntity);
//		~ListElement();
//		
//		FalconEntity*	BaseObject(void) {return baseObject;};
//		void			HandoffBaseObject(void);
//		
//		ListElement*	next;
//		int				flags;
//		int				symbol;
//		VU_TIME			lastHit;
//
//	  private:
//		FalconEntity*	baseObject;
//	};
//	
//	enum DisplayFlags {
//		Radiate    = 0x1,
//		Track      = 0x2,
//		Launch     = 0x4,
//      UnChecked  = 0x8,
//	};
//#endif
		
	float	cursorX, cursorY;
	float	HadOrigCursorX, HadOrigCursorY;
	float	HadOrigCursorX2, HadOrigCursorY2; 
	float	yawBackup, XPosBackup, YPosBackup;

	enum HASZoomMode { Wide = 0, Center, Right, Left };
	enum HADZoomMode { NORM = 0, EXP1, EXP2 };
	enum PreHandoffMode { Has = 0, Pos = 1 };
	enum Submode { HarmModeChooser = 0, HAS, Handoff, POS, FilterMode, HAD }; 

	// Threats filtering - All, High Priority threats, High Altitude threats, Low Altitude threats
	enum HASFilterMode { ALL = 0, HP, HA, LA };

	virtual SimObjectType*	Exec (SimObjectType* targetList);
	virtual void			HADDisplay (VirtualDisplay *newDisplay);
	virtual void			HADExpDisplay (VirtualDisplay *newDisplay);
	virtual void			HASDisplay (VirtualDisplay *newDisplay);
	virtual void			POSDisplay (VirtualDisplay *newDisplay);
	virtual void			HandoffDisplay (VirtualDisplay *newDisplay);

	virtual int	ObjectDetected (FalconEntity*, int trackType, int dummy = 0);

	Submode			GetSubMode ( void ) { return submode; }
	Submode			GetLastSubmode ( void ) { return lastSubmode; }
	void			SetSubMode ( Submode HarmMode ) { submode = HarmMode; }
	void			SetLastSubMode ( Submode HarmMode ) { lastSubmode = HarmMode; }
	int				GetRange (void)				{ return trueDisplayRange; };
	void			SetRange (int newRange)		{ displayRange = trueDisplayRange = newRange; };
	void			IncreaseRange (void);
	void			DecreaseRange (void);
	HASZoomMode     GetZoomMode(void) { return zoomMode; }
	void			ToggleZoomMode (void); 
	HADZoomMode     GetHADZoomMode(void) { return HadZoomMode; }
	void			ToggleHADZoomMode (void); 

	void			LockTargetUnderCursor(void);
	void			BoresightTarget(void);
	void			NextTarget(void);
	void			PrevTarget(void);
	virtual void	SetDesiredTarget( SimObjectType* newTarget );
    VU_ID			FindIDUnderCursor(void);
	GroundListElement*	FindTargetUnderCursor( void );
	GroundListElement*  FindHASTargetUnderCursor( void );
	GroundListElement*  FindPOSTarget( void );
	void			LockPOSTarget( void );
	PreHandoffMode  GetPreHandoffMode ( void ) { return preHandoffMode; }
	void			SetPOSTargetIndex ( int index );
	HASFilterMode	GetFilterMode ( void ) { return filterMode; }
	void			SetFilterMode ( HASFilterMode mode ) { filterMode = mode; }
	bool			GetHandedoff ( void ) { return handedoff; }
	void			SetHandedoff ( bool value ) { handedoff = value; }
	void			ResetHASTimer ( void ) { HASTimer = SimLibElapsedTime; HASNumTargets = 0;  }
	void			SaveHadCursorPos();
	void			ResetHadCursorPos();
	void			ResetCursor() { cursorX = 0.0f, cursorY = -HTS_Y_OFFSET; }

protected:
	bool			handedoff;
	static int		flash;			// Do we draw flashing things this frame?
	int				displayRange;
	// range to be displayed for labels, without the "zoom" in of displayRange
	int				trueDisplayRange;
	int				DTSBList[MAX_DTSB_TARGETS];
	int				POSTargetIndex;
	int				HASNumTargets;
	float			zoomFactor;
	float			HadZoomFactor;
	SIM_ULONG		handoffRefTime;
	SIM_ULONG		HASTimer;
	Submode			submode;
	Submode			lastSubmode;
	HASZoomMode		zoomMode;
	HADZoomMode		HadZoomMode;
	HASFilterMode	filterMode;
	GroundListElement* POSTargets[MAX_POS_TARGETS];
	WayPointClass*	POSTargetsWPs[MAX_POS_TARGETS];
	GroundListElement*		curTarget;
	PreHandoffMode  preHandoffMode;

	void			BuildPreplannedTargetList( void );
	GroundListElement*	FindEmmitter( FalconEntity *entity );
	void			LockListElement( GroundListElement* );
	void			DrawWEZ( class MissileClass *theMissile );
	void			DrawDTSBBox (void);
	void			UpdateDTSB (int symbol, float &displayX, float &displayY);
	void			ClearDTSB ( void ) { for ( int i = 0; i < MAX_DTSB_TARGETS; i++ ) DTSBList[i] = 0; };
	void			BoxTargetDTSB ( int symbol, float &displayX, float &displayY );
	void			ClearPOSTargets ( void );
	void			BuildPOSTargets ( void );
	int			    FindWaypointNum ( WayPointClass* theWP );
	bool			IsInsideALIC ( float &displayX, float &displayY );
	bool			IsInPriorityList ( int symbol );
};

#endif
