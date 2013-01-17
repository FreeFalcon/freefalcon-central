/***************************************************************************\
    KneeBoard.cpp
    Scott Randolph
    September 4, 1998

    This class provides the pilots knee board notepad and map.
\***************************************************************************/
#include "stdafx.h"
#include "stdhdr.h"
#include "simveh.h"
#include "campwp.h"
#include "flight.h"
#include "playerop.h"
//#include "brief.h"
#include "Graphics/Include/TMap.h"
#include "Graphics/Include/filemem.h"
#include "Graphics/Include/image.h"
#include "kneeboard.h"
#include "otwdrive.h"
//#include "cpmanager.h"
#include "Falclib/include/dispcfg.h"
#include "flightdata.h"	//MI
#include "aircrft.h"	//MI
#include "phyconst.h"	//MI
//#include "simdrive.h"	//MI
//#include "navsystem.h"	//MI
#include "f4find.h"
#include "weather.h"
#include "tod.h"

static const char	KNEEBOARD_MAP_NAME[]		= "art\\ckptart\\KneeMap.gif";
static const char	THR_KNEEBOARD_MAP_NAME[]		= "KneeMap.gif";
//static const int	KNEEBOARD_MAP_KM_PER_TEXEL	= 2;		// Property of source art on disk

// sfr: removed jb voodoo checks

KneeBoard::KneeBoard(){
	mapImageFile.image.image = NULL;
	mapImageFile.image.palette = NULL;
	refCount = 0;
}

KneeBoard::~KneeBoard(){
}

// sfr: removed dest info
void KneeBoard::Setup(){
	if (refCount == 0){
		page = BRIEF;
		LoadKneeImage();
	}
	++refCount;
}


void KneeBoard::Cleanup( void ){
	--refCount;
	if (refCount == 0){
		if (imageLoaded){
			if (mapImageFile.image.palette){
				glReleaseMemory( (char*)mapImageFile.image.palette );
			}
			if (mapImageFile.image.image){
				glReleaseMemory( (char*)mapImageFile.image.image );
			}
			imageLoaded = false;
		}
	}
}

// JPO - load the actual image, try the campaign dir first.
// sfr: removed parameter, since it was internal anyway
void KneeBoard::LoadKneeImage()
{
    char pathname[MAX_PATH];
    int result;

    sprintf (pathname, "%s\\%s", FalconCampaignSaveDirectory, THR_KNEEBOARD_MAP_NAME);
    // Make sure we recognize this file type
    mapImageFile.imageType = CheckImageType( pathname );
    ShiAssert( mapImageFile.imageType != IMAGE_TYPE_UNKNOWN );
    
    // Open the input file
    result = mapImageFile.glOpenFileMem( pathname );
    if (result != 1) {
		mapImageFile.imageType = CheckImageType( KNEEBOARD_MAP_NAME );
		ShiAssert( mapImageFile.imageType != IMAGE_TYPE_UNKNOWN );
	
		// Open the input file
		result = mapImageFile.glOpenFileMem( KNEEBOARD_MAP_NAME );
    }
    ShiAssert( result == 1 );
    
    // Read the image data (note that ReadTextureImage will close texFile for us)
    mapImageFile.glReadFileMem();
    result = ReadTextureImage( &mapImageFile );
    if (result != GOOD_READ) {
		//ShiError( "Failed to read kneeboard image." );
		imageLoaded = false;
    }
	else {
		imageLoaded = true;
	}
}
