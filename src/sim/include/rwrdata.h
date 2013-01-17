#ifndef _RWRDATA_H_
#define _RWRDATA_H_

#pragma pack(push, 1)
typedef struct RwrDataType
{
	float		nominalRange;              // Nominal detection range
	float		top;                       // Scan volume top (Degrees in text file)
	float		bottom;                    // Scan volume bottom (Degrees in text file)
	float		left;                      // Scan volume left (Degrees in text file)
	float		right;                     // Scan volume right (Degrees in text file)
	short		flag;					   /* 0x01 = can get exact heading
											  0x02 = can only get vague direction
											  0x04 = can detect exact radar type
											  0x08 = can only detect group of radar types */
} RwrDataType;
#pragma pack(pop)

extern RwrDataType*			RwrDataTable;
extern short NumRwrEntries;

#define RWR_EXACT_HEADING 0x01
#define RWR_VAGUE_DIRECTION 0x02
#define RWR_EXACT_TYPE 0x04
#define RWR_GROUP_TYPE 0x08

#endif
