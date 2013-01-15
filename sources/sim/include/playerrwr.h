/***************************************************************************\
    PlayerRWR.h

    This provides the special abilities required for a player's RWR.  Most
	of the added functionality involves Display and Mode control.
\***************************************************************************/
#ifndef _PLAYERRWR_H_
#define _PLAYERRWR_H_

#include "vehrwr.h"

class SimBaseClass;

class PlayerRwrClass : public VehRwrClass
{
public :
	PlayerRwrClass (int idx, SimMoverClass* self);
	virtual ~PlayerRwrClass (void);

	virtual SimObjectType* Exec( SimObjectType* targetList );
	virtual void Display( VirtualDisplay *activeDisplay );
	virtual BOOL IsOn();

	void SetGridVisible(BOOL flag)				{mGridVisible = flag;};

	// State Access Functions
	int HasActivity (void)		{return detectionList[0].entity != NULL;};
	int LaunchIndication (void) {return missileActivity;};

	//MI EWS function
	void CheckEWS(void);
	bool InEWSLoop;
	bool SaidJammer;
	int ReleaseManual;
	bool LaunchDetected;
	bool ChaffCheck;
	bool FlareCheck;

	int IsPriority (void)		{return priorityMode;};
	int TargetSep (void)		{return targetSep;};
	int ShowUnknowns (void)		{return showUnknowns;};
	int ShowNaval (void)		{return showNaval;};
	int ShowSearch (void)		{return showSearch;};
	int ShowLowAltPriority (void)		{return lowAltPriority;};
	int IsFiltered (FalconEntity *);
	int ManualSelect (void);
	int LightSearch (void);
	int LightUnknowns (void);
	
	virtual void TogglePriority (void)			{priorityMode	= !priorityMode;};
	virtual void ToggleTargetSep (void);
	virtual void ToggleUnknowns (void)			{showUnknowns	= !showUnknowns;};
	virtual void ToggleNaval (void)				{showNaval		= !showNaval;};
	virtual void ToggleSearch (void)			{showSearch		= !showSearch;};
	virtual void SelectNextEmitter (void);
	virtual void ToggleLowAltPriority (void)	{lowAltPriority	= !lowAltPriority;};
	virtual void ToggleAutoDrop(void)			{};
	
  protected:
	int	flipFactor;		// -1 when AC is upside down, 1 when normal
	int	mGridVisible;
	
	// Button States
	int priorityMode;
	int targetSep;
	int showUnknowns;
	int showNaval;
	int showSearch;
   int missileActivity;
   int newGuy;

	// Select low or high altitude priority
	virtual void	AutoSelectAltitudePriority(void);

	// Output Functions
	void DrawStatusSymbol( int );
	void DrawGrid( void );
	void DrawContact( DetectListElement *record );

	void DoAudio( DetectListElement *record );
	void DoAudio(); // JB 010718
};

#endif // _PLAYERRWR_H_

