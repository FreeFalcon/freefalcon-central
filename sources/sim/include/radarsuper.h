#ifndef _RADARSUPER_H_
#define _RADARSUPER_H_

#include "radar.h"


class RadarSuperClass : public RadarClass
{
  public :
	RadarSuperClass( int index, SimMoverClass* parentPlatform );
	virtual ~RadarSuperClass()					{};

	virtual	void ExecModes( int newDesignate, int newDrop );
	virtual void UpdateState( int cursorXCmd, int cursorYCmd );
	virtual SimObjectType* Exec( SimObjectType* targetList );
	virtual void Display( VirtualDisplay* activeDisplay );

	// State control functions
	virtual void PushButton (int idx, int mfd = 0);
	virtual void RangeStep( int cmd )			{ wantRange = (cmd>0) ? rangeNM*2.0f: rangeNM*0.5f; };

	virtual void NextTarget( void )				{ wantLock = NEXT; };
	virtual void PrevTarget( void )				{ wantLock = PREV; };

	virtual void DefaultAAMode( void )			{ wantMode = AA; };
	virtual void StepAAmode( void );
	virtual void SetSRMOverride( void );
	virtual void SetMRMOverride( void );
	virtual void ClearOverride( void );

	virtual void SelectACMBore( void )			{ wantLock = BORE; };
	virtual void SelectACMVertical( void )		{ if (mode == AA)  wantLock = AUTO; };
	virtual void SelectACMSlew( void )			{ if (mode == AA)  wantLock = AUTO; };
	virtual void SelectACM30x20( void )			{ if (mode == AA)  wantLock = AUTO; };

	virtual void DefaultAGMode( void )			{ wantMode = GM; };
	virtual void StepAGmode( void )				{ if (mode == GM) wantMode = GMT; else wantMode = GM; };
	virtual float GetRange (void) {return rangeNM;};
	virtual void GetCursorPosition (float* xPos, float* yPos) {*xPos = (cursorY + 1.0F)* rangeNM * 0.5F; *yPos = cursorX * rangeNM;};
	virtual void GetAGCenter( float* x, float* y );
	virtual int  IsAG (void)					{ return mode != AA ? TRUE : FALSE; };
   virtual void SetMode (RadarMode cmd);

  protected:
	float			rangeNM;			// How far are we looking in NM
	float			rangeFT;			// How far are we looking in FT for convienience
	float			invRangeFT;			// 1/how far we're looking for convienience

	float			prevRange;			// Indicate where we were before AA override

	typedef enum { NOCHANGE=0, AUTO, CURSOR, BORE, NEXT, PREV } LockCommand;	
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
	void NewRange( float rangeInNM );
	float CursorDelta( float x, float y );

	void ExecAG( void );
	void ExecAA( void );

	void DrawCursor( void );
	void DrawBullseyeData(void);
	void DrawLockedAirInfo( float h, float v );
	void DrawLockedGndInfo( float h, float v );
	void DrawWaterline(void);
	void DrawButtons(void);
	void DisplayAAReturns(void);
	void DisplayAGReturns(void);
};

#endif // _RADARSUPER_H_