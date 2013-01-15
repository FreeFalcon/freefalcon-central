#ifndef _SIMSTATIC_H
#define _SIMSTATIC_H

#include "simbase.h"

class SimStaticClass : public SimBaseClass
{
public:
	SimStaticClass(int type);
	SimStaticClass(VU_BYTE** stream, long *rem);
	SimStaticClass(FILE* filePtr);
	virtual ~SimStaticClass(void);
	virtual void Init(SimInitDataClass* initData);
	virtual void InitData();
	virtual void CleanupData();
private:
	void InitLocalData();
	void CleanupLocalData();
public:
	// pure virtual implementation
	virtual float GetVt (void) const { return 0; }
	virtual float GetKias (void) const { return 0; }

	// virtual function interface
	// serialization functions
	virtual int SaveSize();
	virtual int Save(VU_BYTE **stream);	// returns bytes written
	virtual int Save(FILE *file);		// returns bytes written

	// event handlers
	virtual int Handle(VuFullUpdateEvent *event);
	virtual int Handle(VuPositionUpdateEvent *event);
	virtual int Handle(VuTransferEvent *event);
	virtual int IsStatic (void) { return TRUE; }
};

#endif
