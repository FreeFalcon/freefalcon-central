#ifndef _RADARAGONLY_H_
#define _RADARAGONLY_H_

#include "radardigi.h"


class RadarAGOnlyClass : public RadarDigiClass
{
  public :
	RadarAGOnlyClass (int index, SimMoverClass* parentPlatform);
	virtual ~RadarAGOnlyClass()					{};

	virtual SimObjectType* Exec( SimObjectType* targetList );

	// State control functions
	virtual void StepAAmode( void )				{ };	// No AA Mode
	virtual void StepAGmode( void )				{ };	// Always in AG mode
   virtual void SetMode (RadarMode)      { };
	virtual int   IsAG (void) {return TRUE;};		// Always in AG mode
	virtual void SetDesiredTarget( SimObjectType* newTarget );
};

#endif // _RADARAGONLY_H_