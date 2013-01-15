#ifndef _VISUALDATA_H
#define _VISUALDATA_H

typedef struct VisualDataType
{
	float		nominalRange;              // Nominal detection range
	float		top;                       // Scan volume top (Degrees in text file)
	float		bottom;                    // Scan volume bottom (Degrees in text file)
	float		left;                      // Scan volume left (Degrees in text file)
	float		right;                     // Scan volume right (Degrees in text file)
} VisualDataType;

extern VisualDataType*			VisualDataTable;
extern short NumVisualEntries;

#endif
