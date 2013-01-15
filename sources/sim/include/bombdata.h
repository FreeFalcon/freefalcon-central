#ifndef _BOMB_DATA
#define _BOMB_DATA

// Added 2003-11-10 by MLR

class SimlibFileClass;


#define BOMB_ID_VER 0

class BombInputData
{
public:
	int Version;
};



class BombAuxData 
{
public:
	BombAuxData()
	{
		psFeatureImpact=0;
		psBombImpact=0;
	}

	~BombAuxData()
	{
		if(psFeatureImpact) free(psFeatureImpact);
		if(psBombImpact) free(psBombImpact);
	}
	// which strength algorithm to use 0 for old, 1 for new.
	int   cbuStrengthModel; 

	// new strength model parameters
	float cbuLethalHeight,  
		  cbuIneffectiveHeight,
		  cbuDamageDiameterBAMult,
		  cbuMaxDamageDiameter, cbuBlastMultiplier;

	// LAU/Gun pod launch angle
	float lauElevation,
		  lauAzimuth;
	int   lauSalvoSize;
	int   lauRounds;
	int   lauWeaponId;
    int   lauRippleTime;

	// Cobra - JDAM 
	float JDAMLift;
	float JSOWmaxRange;
	// particle effects
	char *psFeatureImpact;
	char *psBombImpact;

	// unused as of yet
	int   sndFlightSFX;
};



class BombDataSetClass
{
public:
	char name[80];
	int Version;
	BombInputData *inputData;
	BombAuxData *auxData;
};

extern BombDataSetClass* BombDataset;
extern int numBombDatasets;

void ReadAllBombData (void);
void BombOpenFiles (char *bombname);
BombAuxData *BombAuxAeroRead(SimlibFileClass* inputFile);

/******************************************************/



#endif

