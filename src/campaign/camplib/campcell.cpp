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

    *TheCell = (CellDataType)((*TheCell & compl ReliefMask) | ((Temp << ReliefShift) & ReliefMask));
}

void SetGroundCover(CellData TheCell, CoverType NewGroundCover)
{
    CellDataType Temp = (CellDataType)NewGroundCover;

    *TheCell = (CellDataType)((*TheCell & compl GroundCoverMask) | ((Temp << GroundCoverShift) & GroundCoverMask));
}

void SetRoadCell(CellData TheCell, char Road)
{
    CellDataType Temp = Road;

    *TheCell = (CellDataType)((*TheCell & compl RoadMask) | ((Temp << RoadShift) & RoadMask));
}

void SetRailCell(CellData TheCell, char Rail)
{
    CellDataType Temp = Rail;

    *TheCell = (CellDataType)((*TheCell & compl RailMask) | ((Temp << RailShift) & RailMask));
}

ReliefType GetReliefType(CellData TheCell)
{
    return (ReliefType)((*TheCell & ReliefMask) >> ReliefShift);
}

CoverType GetGroundCover(CellData TheCell)
{
    return (CoverType)((*TheCell & GroundCoverMask) >> GroundCoverShift);
}

char GetRoadCell(CellData TheCell)
{
    return (char)((*TheCell & RoadMask) >> RoadShift);
}

char GetRailCell(CellData TheCell)
{
    return (char)((*TheCell & RailMask) >> RailShift);
}

