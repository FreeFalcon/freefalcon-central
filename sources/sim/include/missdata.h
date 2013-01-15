#ifndef _MISSILE_DATA
#define _MISSILE_DATA

class SimlibFileClass;
class MissileAeroData;
class MissileInputData;
class MissileRangeData;
class MissileEngineData;
class MissileAuxData;

class MissileDataSetClass
{
public:
	char name[80];
   MissileAeroData    *aeroData;
	MissileInputData   *inputData;
	MissileRangeData   *rangeData;
   MissileEngineData  *engineData;
   MissileAuxData *auxData;
};

extern MissileDataSetClass* missileDataset;
extern int numMissileDatasets;

void ReadAllMissileData (void);
void MissileOpenFiles (char *mslname);
MissileAeroData *MissileAeroRead (SimlibFileClass*);
MissileInputData *MissileInputRead(SimlibFileClass*);
MissileRangeData *MissileRangeRead(SimlibFileClass*);
MissileEngineData *MissileEngineRead (SimlibFileClass*);
MissileAuxData *MissileAuxAeroRead(SimlibFileClass* inputFile);
#endif

