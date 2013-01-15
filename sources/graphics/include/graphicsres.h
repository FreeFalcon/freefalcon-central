/***************************************************************************\
    GraphicsRes.h
    Scott Randolph
    November 13, 1997

    Provides a simple isolation layer between the graphics and terrain
	libraries and the resource manager (if any).
\***************************************************************************/
#ifndef _GRAPHICSRES_H_
#define _GRAPHICSRES_H_


// Define this when operating data is to be inside ZIP files.
// RV - Biker - Graphics no longer use resource manager
//#define GRAPHICS_USE_RES_MGR


#ifndef GRAPHICS_USE_RES_MGR // DON'T USE RESMGR

	#define GR_OPEN   open
	#define GR_CLOSE  close
	#define GR_READ   read
	#define GR_WRITE  write
	#define GR_SEEK   lseek
	#define GR_TELL   tell

#else // USE RESMGR

	extern "C"
	{
		#include "codelib\resources\reslib\src\resmgr.h"
	}

	#define GR_OPEN   ResOpenFile
	#define GR_CLOSE  ResCloseFile
	#define GR_READ   ResReadFile
	#define GR_WRITE  ResWriteFile
	#define GR_SEEK   ResSeekFile
	#define GR_TELL   ResTellFile

#endif



#endif // _GRAPHICSRES_H_
