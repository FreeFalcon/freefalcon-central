#ifndef CAMPCELL
#define CAMPCELL

// =================
// Campaign Cell ADT
// =================

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

typedef	uchar			CellDataType;
typedef CellDataType	*CellData;

extern void SetReliefType (CellData TheCell, ReliefType NewReliefType);

extern void SetGroundCover (CellData TheCell, CoverType NewGroundCover);

extern void SetRoadCell (CellData TheCell, char Road);

extern void SetRailCell (CellData TheCell, char Rail);

extern char GetAltitudeCode (CellData TheCell);

extern ReliefType GetReliefType (CellData TheCell);

extern CoverType GetGroundCover (CellData TheCell);

extern char GetRoadCell (CellData TheCell);

extern char GetRailCell (CellData TheCell);

#endif
