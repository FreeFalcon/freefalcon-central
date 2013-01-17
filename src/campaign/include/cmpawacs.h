
#ifndef CMPAWACS_H

// ========================
// Defines
// ========================

#define AWACS_FIND_TARGET	0
#define AWACS_FIND_THREAT	1

// ========================
// AWACS related functions
// ========================

extern CampEntity FindNearestCampEntity (int type, int radius, int *count);

#endif