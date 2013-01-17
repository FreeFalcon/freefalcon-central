#ifndef _LASERPOD_MODEL_H
#define _LASERPOD_MODEL_H

#include "visual.h"

class LaserPodClass : public VisualClass
{
public :
	LaserPodClass (int type, SimMoverClass* self);
	virtual ~LaserPodClass (void);

	virtual SimObjectType* Exec (SimObjectType* targetList);
	virtual void Display (VirtualDisplay*);
	virtual void DisplayInit (ImageBuffer*);

	virtual void SetDesiredTarget (SimObjectType *curTarget);

	int LockTarget (void);
	int IsLocked (void)							{return (hasTarget == TargetLocked);}

	void  ToggleFOV (void);
	float CurFOV (void)							{return curFOV;};

	int  SetDesiredSeekerPos (float* az, float* el);

	void SetYPR (float a, float b, float c)		{ yaw=a; pitch=b; roll=c; };
	void GetYPR (float* a, float* b, float* c)	{ *a=yaw; *b=pitch; *c=roll; };

	void SetTargetPosition (float a, float b, float c)		{ tgtX=a; tgtY=b; tgtZ=c; };
	void GetTargetPosition (float* a, float* b, float* c)	{ *a=tgtX; *b=tgtY; *c=tgtZ; };

	//MI
	void DrawBox(VirtualDisplay* display);
	void DrawFOV(VirtualDisplay* display);
	bool BHOT, MenuMode;
	void TogglePolarity(void)	{BHOT = !BHOT;};

protected :
	void	DrawTerrain (void);

	int		hasTarget;
	float	curFOV;
	float	roll, pitch, yaw;
	float	tgtX, tgtY, tgtZ;
	enum {NoTarget, TargetLocked};
};

#endif

