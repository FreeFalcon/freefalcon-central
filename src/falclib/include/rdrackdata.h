#ifndef RDRACKDATA_H
#define RDRACKDATA_H

#define RDF_BMSDEFINITION          (1<<0) // used to determine if the harpoints data came from the bmsrackdat, or the old data
#define RDF_EMERGENCY_JETT_WEAPON  (1<<1)
#define RDF_SELECTIVE_JETT_WEAPON  (1<<2)
#define RDF_EMERGENCY_JETT_RACK   ((1<<3))// | RDF_EMERGENCY_JETT_WEAPON)
#define RDF_SELECTIVE_JETT_RACK   ((1<<4))// | RDF_SELECTIVE_JETT_WEAPON)
#define RDF_EMERGENCY_JETT_PYLON  ((1<<5))// | RDF_EMERGENCY_JETT_RACK)
#define RDF_SELECTIVE_JETT_PYLON  ((1<<6))// | RDF_SELECTIVE_JETT_RACK)

struct RDRackData 
{
	RDRackData() { memset (this, 0, sizeof (*this)); flags = RDF_EMERGENCY_JETT_RACK | RDF_SELECTIVE_JETT_RACK;};
	char *pylonmnemonic;
	char *rackmnemonic;
	int pylonCT;
	int rackCT;
	int rackStations;
	int flags;
	int count;
	int *loadOrder;
};


// MLR 2/13/2004 - new rack dat code
void RDLoadRackData(void);
void RDUnloadRackData(void);

// finds by either WID or SWD
int RDFindBestRack(int GroupId, int WeaponId, int WeaponCount, struct RDRackData *rd);

int RDFindBestRackWID(int GroupId, int WeaponId, int WeaponCount, struct RDRackData *rd);
int RDFindBestRackSWD(int GroupId, int SWId, int WeaponCount, struct RDRackData *rd);

#endif