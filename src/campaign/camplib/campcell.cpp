#include <stddef.h>
#include "CmpGlobl.h"
#include "Campcell.h"
#include "Campterr.h"

// ==========================================
// Campaign Cell ADT - Private Implementation
// ==========================================

// ==========================================
// Needed External functions
// ==========================================

extern CellData GetCell(int x, int y);

// ==========================================
// Global functions
// ==========================================

void SetReliefType(CellData TheCell, ReliefType NewReliefType)
{
    CellDataType Temp = (CellDataType)NewReliefType;

    *TheCell = (CellDataType)((*TheCell bitand compl ReliefMask) bitor ((Temp << ReliefShift) bitand ReliefMask));
}

void SetGroundCover(CellData TheCell, CoverType NewGroundCover)
{
    CellDataType Temp = (CellDataType)NewGroundCover;

    *TheCell = (CellDataType)((*TheCell bitand compl GroundCoverMask) bitor ((Temp << GroundCoverShift) bitand GroundCoverMask));
}

void SetRoadCell(CellData TheCell, char Road)
{
    CellDataType Temp = Road;

    *TheCell = (CellDataType)((*TheCell bitand compl RoadMask) bitor ((Temp << RoadShift) bitand RoadMask));
}

void SetRailCell(CellData TheCell, char Rail)
{
    CellDataType Temp = Rail;

    *TheCell = (CellDataType)((*TheCell bitand compl RailMask) bitor ((Temp << RailShift) bitand RailMask));
}

ReliefType GetReliefType(CellData TheCell)
{
    return (ReliefType)((*TheCell bitand ReliefMask) >> ReliefShift);
}

CoverType GetGroundCover(CellData TheCell)
{
    return (CoverType)((*TheCell bitand GroundCoverMask) >> GroundCoverShift);
}

char GetRoadCell(CellData TheCell)
{
    return (char)((*TheCell bitand RoadMask) >> RoadShift);
}

char GetRailCell(CellData TheCell)
{
    return (char)((*TheCell bitand RailMask) >> RailShift);
}

