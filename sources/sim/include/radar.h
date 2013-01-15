#ifndef _RADAR_H_
#define _RADAR_H_

#include "sensclas.h"
#include "radarData.h"

class RadarClass : public SensorClass
{
  public:
      enum RadarMode {
			OFF, STBY, RWS, LRS, TWS, VS, ACM_30x20, ACM_SLEW, ACM_BORE, ACM_10x60,
			SAM, SAM_AUTO_MODE, SAM_MANUAL_MODE, STT, GM, FTT, GMT, SEA, AA, BCN, AGR
	  };
      enum DigiRadarMode {DigiSTT, DigiSAM, DigiTWS, DigiRWS, DigiOFF};  // 2002-02-09 ADDED BY S.G. The different radar mode.
	  enum RadarFlag { FirstSweep = 0x00000001 }; // 2002-03-10 S.G.
  protected:
	UInt32					lastTargetLockSend;		// When did we last send a lock message?
	BOOL					isEmitting;				// Though the radar may be "on" is it radiating?
/* S.G. */ public:	struct RadarDataType	*radarData;	protected:			// Pointer into the class table data for this type of radar
	static const float		CursorRate;				// How fast do the radar cursors move?
	virtual void SetSensorTarget( SimObjectType* newTarget );		// Used for things in the target list

/* S.G. */public:	virtual float ReturnStrength( SimObjectType* target );	protected: // > 1.0 means I see it.  < 1.0 means I don't
															// Considers look down, jamming, range, RCS,
															// Doppler notch by default.
	RadarMode		mode;				// Current radar mode
	RadarMode		prevMode;			// Indicate where we were before AA override
	RadarMode		Missovrradarmode ;//me123
	RadarMode		Dogfovrradarmode;//me123
	RadarMode		noovrradarmode;//me123
	int				flag; // 2002-03-10 S.G.
	int Overridemode ;//me123
	//MI
	RadarMode		LastAAMode, LastNAVMode, LastAGMode;
	RadarDataSet *radarDatFile;
  public :
	RadarClass (int type, SimMoverClass* parentPlatform);
	virtual ~RadarClass()	{};
	

// Methods inherited from SensorClass (commented out here since we aren't doing anything)
//	virtual void ExecModes(int designateCmd, int dropTrackCmd)		{};
//	virtual void UpdateState(int cursorXCmd, int cursorYCmd)		{};
//	virtual void PushButton( int whichButton, int whichMFD = 0)		{};

	// This does the basic radar modeling
	virtual SimObjectType* Exec( SimObjectType* targetList ) = 0;

	// Drawing functions
	virtual void DisplayInit (ImageBuffer* newImage);

	// Target notification function (triggers RWR and Digi responses)
	virtual void SendTrackMsg (SimObjectType* tgtptr, unsigned int trackType, unsigned int hardpoint = 0);		// Send a track message

	// State control functions (callable from any thread)
	virtual void SetPower( BOOL state );
	virtual void SetEmitting( BOOL state );
	virtual void RangeStep( int )			{};	// Step up/down in range

	virtual void NextTarget( void )				{};	// Step to next available target
	virtual void PrevTarget( void )				{};	// Step to prev available target
	virtual void SetDesiredTarget( SimObjectType* newTarget );
	virtual void ClearSensorTarget ( void );		// 2002-02-10 ADDED BY S.G. Need to deal with digiRadarMode before calling SensorClass::ClearSensorTarget

	virtual void DefaultAAMode( void )			{};	// If not in AA go to default AA mode, otherwise nothing
	virtual void StepAAmode( void )				{};	// Enter or step AA mode
	virtual void SetSRMOverride()				{};	// Temporary pop into short range AA mode
	virtual void SetMRMOverride()				{};	// Temporary pop into medium range AA mode
	virtual void ClearOverride()				{};	// Return to previous mode

	virtual void SelectACMVertical()			{};	// Go directly to the named
	virtual void SelectACMBore()				{};	// AA mode.
	virtual void SelectACMSlew()				{};	// (in hard mode, at least, this only works
	virtual void SelectACM30x20()				{};	//  when already in an ACM mode.)

	virtual void StepAAscanHeight()				{};	// Adjust the width and height of the
	virtual void StepAAscanWidth()				{};	// radar scan volume.
	virtual void StepAAelvation( int )		{};	// Adjust the lookup/lookdown angle of the radar (cmd=0 means center it)

	virtual void DefaultAGMode( void )			{};	// If not in AG go to default AG mode, otherwise nothing
	virtual void StepAGmode( void )				{};	// Enter or step AA mode

	virtual void StepAGfov()					{};	// Select the degree of "zoom" on GM radar
	virtual void StepAGgain( int )			{};	// Select gain on GM radar
   virtual void SetMode(RadarMode )       {};   // Select new mode

	virtual void ToggleAGfreeze()				{};	// Freeze radar image
	virtual void ToggleAGsnowPlow()				{};	// Look a fixed distance ahead of the AC
	virtual void SetAGSnowPlow(int )				{};	// Look a fixed distance ahead of the AC
	virtual void ToggleAGcursorZero()			{};	// Remove pilot cursor offsets
	virtual void SetGroundPoint (float, float, float) {};	//Set center of ground map radar

	int IsEmitting (void)						{ return isEmitting; };
	virtual int   IsAG (void)					{ return FALSE; };		// Is radar in A/G mode
	virtual void  GetAGCenter (float* x, float* y)	{*x = 0; *y = 0;};	// Center of radar ground search
	virtual float GetRange(void)				{ return 10.0F; };		// Display range in NM
	virtual float GetVolume (void)				{ return 1.0472F; };	// Default to 60 degree cone
	virtual void  GetCursorPosition (float*, float*) {};
	virtual int GetBuggedData (float *x, float *y, float *dir, float *speed) { return FALSE; }; // info about bugged target
  public:
	static const UInt32		TrackUpdateTime;		// How long between lock message retransmits?
	DigiRadarMode	digiRadarMode; // 2002-02-09 ADDED BY S.G. The current radar mode of the AI (STT, TWS, RWS or OFF)

	//MI
	virtual void ClearModeDesiredCmd(void)	{};

	// 2002-02-12 ADDED BY S.G. Need them outside RadarClass
	RadarDataType *GetRadarData(void)			{ return radarData; };
	RadarDataSet *GetRadarDatFile(void)			{ return radarDatFile; };

	// 2002-03-29 ADDED BY S.G. Need them outside RadarClass
	void SetFlag(RadarFlag val)					{ flag |= val; };
	void ClearFlag(RadarFlag val)				{ flag &= ~val; };
	int GetFlag(void)						{ return flag; };
	virtual RadarMode	GetRadarModeR(void)	{return mode;};
};

#endif // _RADAR_H_

