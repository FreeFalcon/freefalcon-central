#ifndef _RADAR360_H_
#define _RADAR360_H_

#include "radar.h"


class Radar360Class : public RadarClass
{
  public :
	Radar360Class (int index, SimMoverClass* parentPlatform);
	virtual ~Radar360Class()					{};

	virtual	void ExecModes( int newDesignate, int newDrop );
	virtual void UpdateState( int cursorXCmd, int cursorYCmd );
	virtual SimObjectType* Exec( SimObjectType* targetList );
	virtual void Display( VirtualDisplay* activeDisplay );

	// State control functions
	virtual void SetEmitting( BOOL )		{ isEmitting = isOn; };		// Simple Radar is always active while powered
	virtual void PushButton (int idx, int mfd = 0);
	virtual void RangeStep( int cmd )			{ wantRange = (cmd>0) ? rangeNM*2.0f: rangeNM*0.5f; };

	virtual void NextTarget( void )				{ wantLock = NEXT; };
	virtual void PrevTarget( void )				{ wantLock = PREV; };

	virtual void DefaultAAMode( void )			{ wantMode = AA; };
	virtual void StepAAmode( void )				{ wantMode = AA; };
	virtual void SelectACMBore()				{ wantLock = BORE; };
	virtual void SetSRMOverride();
	virtual void SetMRMOverride();
	virtual void ClearOverride();

	virtual void	DefaultAGMode( void )		{ wantMode = GM; };
	virtual void	StepAGmode( void )			{ wantMode = GM; };

	virtual float	GetRange (void) {return rangeNM;};
	virtual void	GetCursorPosition (float* xPos, float* yPos) {*xPos = cursorY * rangeNM; *yPos = cursorX * rangeNM;};
	virtual void	GetAGCenter (float* x, float* y);
	virtual int		IsAG (void) { return mode == GM ? TRUE : FALSE;};
   virtual void SetMode (RadarMode cmd);

	// JB 011213
	void SetMaxRange(float inMaxRangeNM) {maxRangeNM = inMaxRangeNM;};
	float GetMaxRange() {return maxRangeNM;};
	void SetAWACSMode(bool inAWACSMode) {AWACSMode = inAWACSMode;};
	bool GetAWACSMode() {return AWACSMode;};

  protected:
	bool AWACSMode;
	float			maxRangeNM; // JB 011213 How far the radar can look out in NM
	float			rangeNM;			// How far are we looking in NM
	float			rangeFT;			// How far are we looking in FT for convienience
	float			invRangeFT;			// 1/how far we're looking for convienience

	float			prevRange;			// Indicate where we were before AA override

	typedef enum { NOCHANGE=0, AUTO, CURSOR, BORE, NEXT, PREV }	LockCommand;	
	LockCommand		lockCmd;			// Current desired target lock operation

	float			cursorX;			// radar cursor location in normalized display space
	float			cursorY;			// (ie:  -1.0 to 1.0)
	enum {CursorMoving = 0x1};
	int flags;

  protected:
	// Command queues -- These store commands until we're able to process them
	float		wantRange;
	RadarMode	wantMode;
	LockCommand	wantLock;

  protected:
	void	ExecAA( void );
	void	ExecAG( void );

	void NewRange( float rangeInNM );
	BOOL InAALockZone( SimObjectType *object, float x, float y );
	BOOL InAGLockZone( float cosATA, float x, float y );
	float CursorDelta( float x, float y );

	void DrawCursor( void );
	void DrawBullseyeData(void);
	void DrawButtons(void);
	void DisplayAATargets( float scaledSinYaw, float scaledCosYaw );
	void DisplayAGTargets( float scaledSinYaw, float scaledCosYaw );
};

#endif // _RADAR360_H_