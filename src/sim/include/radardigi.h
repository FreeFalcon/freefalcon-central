#ifndef _RADARDIGI_H_
#define _RADARDIGI_H_

#include "radar.h"


class RadarDigiClass : public RadarClass
{
  public :
	RadarDigiClass (int index, SimMoverClass* parentPlatform);
	virtual ~RadarDigiClass()					{};

	virtual SimObjectType* Exec( SimObjectType* targetList );

	// State control functions
	virtual void RangeStep( int cmd )			{ NewRange( (cmd>0) ? rangeNM*2.0f: rangeNM*0.5f); };

	virtual void StepAAmode( void )				{ mode = AA; };	// Enter AA mode
	virtual void StepAGmode( void )				{ mode = GM; };	// Enter AG mode
   virtual void SetMode (RadarMode cmd);
	virtual int   IsAG (void);		// Is radar in A/G mode
	virtual void  GetAGCenter (float* x, float* y);	// Center of radar ground search

  protected:
	float			rangeNM;			// How far are we looking in NM
	float			rangeFT;			// How far are we looking in FT for convienience
	float			invRangeFT;			// 1/how far we're looking for convienience

  protected:
	void NewRange( float rangeInNM );
};

#endif // _RADARDIGI_H_