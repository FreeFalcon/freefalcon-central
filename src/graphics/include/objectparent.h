/***************************************************************************\
    ObjectParent.h
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#ifndef _OBJECTPARENT_H_
#define _OBJECTPARENT_H_

#include "ObjectLOD.h"


// Update this each time the object file formats change
static const UInt32	FORMAT_VERSION = 0x03087000;


extern class ObjectParent	*TheObjectList;
extern int					TheObjectListLength;



// RED - this is the single record stucture in the parents file
typedef	struct {

	float				radius;
	float				minX, maxX;
	float				minY, maxY;
	float				minZ, maxZ;

	float				RadarSign;
	float				IRSign;

	short				nTextureSets;
	short				nDynamicCoords;
	unsigned char		nLODs;
	unsigned char		nSwitch;
	unsigned char		nDOF;
	unsigned char		nSlots;
	short				nSwitches;
	short				nDOFs;

	//DWORD				Unused_2;

}	ParentFileRecord;



typedef struct LODrecord {
	ObjectLOD	*objLOD;
	float		maxRange;
} LODrecord;


class ObjectParent {
  public:
	ObjectParent();
	~ObjectParent();

	static void	SetupTable( char *filename );
	static void CleanupTable( void );
	static void FlushReferences( void );

	void		ReferenceWithFetch(void);
	void		Reference(void);
	void		Release(bool Unlock = false);

	void		ReferenceTexSet(DWORD TexSet = 0, DWORD MaxTexSet = 0);
	void		ReleaseTexSet(DWORD TexSet = 0, DWORD MaxTexSet = 0);

  protected:
	static void ReadParentList( int file );
	static void VerifyVersion( int file );

  public:
	ObjectLOD	*ChooseLOD(float range, int *lod_used, float *max_range);

	float				radius;
	float				minX, maxX;
	float				minY, maxY;
	float				minZ, maxZ;
	float				RadarSign, IRSign;

	LODrecord			*pLODs;
	Ppoint				*pSlotAndDynamicPositions;

	short				nTextureSets;
	short				nDynamicCoords;
	unsigned char		nLODs;
	//unsigned char		nSwitches; // need 2 bytes for Switches
	//unsigned char		nDOFs; // need 2 bytes for DOFs
	unsigned char		nSwitch;
	unsigned char		nDOF;
	unsigned char		nSlots;
	short				nSwitches;
	short				nDOFs;

  protected:
	bool				Locked;							// RED - The object is locked, can not be released
	int					refCount;
};

#endif // _OBJECTPARENT_H_