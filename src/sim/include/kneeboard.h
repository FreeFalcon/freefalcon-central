/***************************************************************************\
    KneeBoard.h
    Scott Randolph
    September 4, 1998

    This class provides the pilots knee board notepad and map.
\***************************************************************************/
#ifndef _KNEEBOARD_H_
#define _KNEEBOARD_H_

//#include "Graphics/Include/Render2d.h"
#include "Graphics/Include/image.h"

class SimVehicleClass;
class WayPointClass;
class DisplayDevice;
class CImageFileMemory;

class KneeBoard {
  public:
	KneeBoard();
	virtual ~KneeBoard();
void KneeBoard::DrawMissionText( Render2D *renderer, SimVehicleClass *platform );
	// loads and cleanup kneeboard resources
	virtual void Setup( /*DisplayDevice *device, int top, int left, int bottom, int right*/ );
	virtual void Cleanup( void );

	// sfr: page information functions and variables
	enum Page {BRIEF, MAP, STEERPOINT};
	void SetPage( Page p )	{ page = p; };
	Page GetPage( void )	{ return page; };

	// map stuff
	CImageFileMemory &GetImageFile() { return mapImageFile; }
	/*float GetVcenter()   const { return wsVcenter;   }
	float GetHcenter()   const { return wsHcenter;   } 
	float GetHsize()     const { return wsHsize;     }
	float GetVsize()     const { return wsVsize;	 }
	int   GetPixelMag()  const { return pixelMag;    }
	float GetPixel2nmX() const { return m_pixel2nmX; }
	float GetPixel2nmY() const { return m_pixel2nmY; }*/

	//void UpdateMapDimensions( SimVehicleClass *platform );


  private:
	// reference counters (views using us)
	unsigned int refCount;

	// sfr: non sense using an internal parameter to a private function
	void LoadKneeImage(/*CImageFileMemory &imagefile*/);
	// Which page we're displaying
	Page		page;
/*
	// Real world map dimensions (R/O externally)
	float		wsVcenter;	//vertical center
	float		wsHcenter;	//horizontal center
	float		wsHsize;	//horizontal size
	float		wsVsize;	//vertical size
	int			pixelMag;
	float m_pixel2nmX, m_pixel2nmY;
*/
	// Map image surface and Blt rectangles
	bool imageLoaded;
	CImageFileMemory 	mapImageFile;

};

#endif // _KNEEBOARD_H_
