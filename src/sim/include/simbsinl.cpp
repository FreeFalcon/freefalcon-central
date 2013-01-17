////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "Falclib/Include/f4vu.h"

inline SimBaseSpecialData::SimBaseSpecialData()
{
	memset (this, 0, sizeof(SimBaseSpecialData));
	ChaffID = FalconNullId;
	FlareID = FalconNullId;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline SimBaseSpecialData::~SimBaseSpecialData()
{	
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

	void SetStatus (int status);// {specialData.status = status;};
	void SetStatusBit (int status);// {specialData.status |= status;};
	void ClearStatusBit (int status);// {specialData.status &= ~status;};
	void SetVt (float vt);// {specialData.vt = vt;};
	void SetKias (float kias);// {specialData.kias = kias;};
	void SetPowerOutput (float powerOutput);// {specialData.powerOutput = powerOutput;};
	void SetRdrAz (float az);// {specialData.rdrAz = az;};
	void SetRdrEl (float el);// {specialData.rdrEl = el;};
	void SetRdrAzCenter (float az);// {specialData.rdrAzCenter = az;};
	void SetRdrElCenter (float el);// {specialData.rdrElCenter = el;};
	void SetRdrCycleTime (float cycle);// {specialData.rdrCycleTime = cycle;};
	void SetRdrRng (float rng);// {specialData.rdrNominalRng = rng;};
	void SetDying (int flag);// { if (flag) specialData.flags |= OBJ_DYING; else specialData.flags &= ~OBJ_DYING;};
	void SetCountry(int newSide);// {specialData.country = newSide;};
	void SetLastChaff (long a);// {specialData.lastChaff = a;};
	void SetLastFlare (long a);// {specialData.lastFlare = a;};
	void SetFlag (int flag);// { specialData.flags |= flag;};
	void UnSetFlag (int flag);// { specialData.flags &= ~(flag);};
