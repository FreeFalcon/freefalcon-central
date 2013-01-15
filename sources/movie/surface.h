#ifndef  _SURFACE_
#define  _SURFACE_

#include <windows.h>

#ifdef   __cplusplus
extern   "C"
{
#endif

#define  BLIT_MODE_NORMAL     0
#define  BLIT_MODE_DOUBLE_V   1

#define  SURFACE_IS_UNLOCKED  0
#define  SURFACE_IS_LOCKED    1

typedef struct tagSURFACEDESCRIPTION
{
   DWORD             bitCount;
   DWORD             redMask;
   DWORD             greenMask;
   DWORD             blueMask;
   DWORD             dwWidth;
   DWORD             dwHeight;
   long              lPitch;
} SURFACEDESCRIPTION, *PSURFACEDESCRIPTION;

typedef struct tagSURFACEACCESS
{
   LPVOID            surfacePtr;
   long              lPitch;
   DWORD             lockStatus;
} SURFACEACCESS, *PSURFACEACCESS;

extern   void surfaceGetPointer( LPVOID,
                                 SURFACEACCESS * );
extern   void surfaceReleasePointer( LPVOID,
                                       SURFACEACCESS * );
extern   void surfaceGetDescription( LPVOID,
                                       SURFACEDESCRIPTION * );
extern   LPVOID surfaceCreate( LPVOID, int, int );
extern   void surfaceRelease( LPVOID );
extern   int surfaceBlit( LPVOID, int, int, LPVOID,
            int, int, int );

#ifdef   __cplusplus
}

#endif

#endif   /* _SURFACE_ */
