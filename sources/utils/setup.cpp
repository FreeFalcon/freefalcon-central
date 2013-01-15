/***************************************************************************\
    Setup.cpp
    Scott Randolph
    April 2, 1997

    This is a one stop source for the terrain/weather/graphics system
	startup and shutdown sequences.  Just call these functions and you're
	set.
\***************************************************************************/
#include "PalBank.h"
#include "TexBank.h"
#include "ObjectParent.h"
#include "TimeMgr.h"
#include "DevMgr.h"
#include "TOD.h"
#include "WXMap.h"
#include "Tmap.h"
#include "TBlock.h"
#include "TBlkList.h"
#include "Tex.h"
#include "TerrTex.h"
#include "FarTex.h"
#include "DrawBSP.h"
#include "DrawOVC.h"
#include "DrawSgmt.h"
#include "Draw2d.h"
#include "RenderOW.h"
#include "GraphicsRes.h"
#include "Setup.h"
#include "Falclib\Include\openfile.h"

static char	theaterPath[_MAX_PATH];
static char	objectPath[_MAX_PATH];
static char	misctexPath[_MAX_PATH];


#ifdef GRAPHICS_USE_RES_MGR
static int	ResHandleTerrainTex		= -1;
static char	*TerrainTexArchiveName	= "Texture.zip";
#endif

#ifdef USE_SH_POOLS
MEM_POOL glMemPool;					// 3dlib stuff
#endif

//extern void Load2DFontTextures();
//extern void Release2DFontTextures();

/***************************************************************************\
	Load all data and create all structures which do not depend on a
	specific graphics device.  This should be done only once.  This must
	be done before any of the other setup calls are made.
\***************************************************************************/
void DeviceIndependentGraphicsSetup( char *theater, char *objects, char* misctex )
{
	char	fullPath[_MAX_PATH];
	char	zipName[_MAX_PATH];


	// Store the data path and the map name
	strcpy( theaterPath, theater );
	strcpy( objectPath, objects );
	strcpy( misctexPath, misctex );

#ifdef USE_SH_POOLS
	// Initialize our Smart Heap pools
	Palette::InitializeStorage();
	TBlock::InitializeStorage();
	TListEntry::InitializeStorage();
	TBlockList::InitializeStorage();
	glMemPool = MemPoolInit( 0 );
#endif

#ifdef GRAPHICS_USE_RES_MGR
	// Setup our attach points
	sprintf( fullPath, "%s\\Texture", theaterPath );
	ResAddPath( fullPath, FALSE );
	sprintf( fullPath, "%s\\Weather", theaterPath );
	ResAddPath( fullPath, FALSE );
	ResAddPath( objectPath, FALSE );
	ResAddPath( misctexPath, FALSE );

	// Attach our resource files
	sprintf( fullPath, "%s\\texture\\", theaterPath );
	sprintf( zipName, "%s\\texture\\%s", theaterPath, TerrainTexArchiveName );
	ResHandleTerrainTex = ResAttach_Open ( fullPath, zipName, FALSE );
	if(ResHandleTerrainTex < 0)
	{
		//ShiAssert( ResHandleTerrainTex >= 0 );
		// we need to exit cleanly... cuz the file couldn't be opened which we need
	}
#endif

	// Setup the font tables
	VirtualDisplay::InitializeFonts();

	// Setup the Asynchronous loader object
	TheLoader.Setup();

	// Setup the environmental time manager object
	TheTimeManager.Setup( 2004, 300 );		// TODO:  Get a day of the month in here or somewhere
	TheTimeManager.SetTime( 0 );			// TODO:  Get a time of day in here or somewhere

	// Setup the time of day manager
	sprintf( fullPath, "%s\\weather", theaterPath );
	TheTimeOfDay.Setup ( fullPath );
	
	// Setup the terrain database
	sprintf( fullPath, "%s\\terrain", theaterPath );
	TheMap.Setup( fullPath );

	// Setup the BSP object library
	sprintf( fullPath, "%s\\%s", objectPath, "KoreaObj" );
	ObjectParent::SetupTable( fullPath );
}


/***************************************************************************\
	Load all data and create all structures which require a specific 
	graphics device to be identified.  For now, this can only be done
	for one device at a time.  In the future, we might allow multiple
	simultanious graphics devices through this interface.
\***************************************************************************/
void DeviceDependentGraphicsSetup( DisplayDevice *device )
{
	char	fullPath[_MAX_PATH];

	// OW - must initialize Textures first for pools to work 
#if 1
	// Setup the miscellanious texture database
	sprintf( fullPath, "%s\\", misctexPath );
	Texture::SetupForDevice( device->GetDefaultRC(), fullPath );

	// Setup the terrain texture database
	sprintf( fullPath, "%s\\texture\\", theaterPath );
	TheTerrTextures.Setup( device->GetDefaultRC(), fullPath );
	TheFarTextures.Setup( device->GetDefaultRC(), fullPath );
#else
	// Setup the terrain texture database
	sprintf( fullPath, "%s\\texture\\", theaterPath );
	TheTerrTextures.Setup( device->GetDefaultRC(), fullPath );
	TheFarTextures.Setup( device->GetDefaultRC(), fullPath );

	// Setup the miscellanious texture database
	sprintf( fullPath, "%s\\", misctexPath );
	Texture::SetupForDevice( device->GetDefaultRC(), fullPath );
#endif

	// Setup all the miscellanious textures we need to pre-load
	DrawableBSP::SetupTexturesOnDevice( device->GetDefaultRC() );
	Drawable2D::SetupTexturesOnDevice( device->GetDefaultRC() );
	DrawableTrail::SetupTexturesOnDevice( device->GetDefaultRC() );
	DrawableParticleSys::SetupTexturesOnDevice( device->GetDefaultRC() );
	DrawableOvercast::SetupTexturesOnDevice( device->GetDefaultRC() );
	RenderOTW::SetupTexturesOnDevice( device->GetDefaultRC() );
	Render2D::Load2DFontTextures(); //JAM 22Dec03
}


/***************************************************************************\
	Clean up all graphics device dependent data and structures.
\***************************************************************************/
void DeviceDependentGraphicsCleanup( DisplayDevice *device )
{
	// Clean up all the pre-loaded textures we have.
	DrawableBSP::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	Drawable2D::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	DrawableTrail::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	DrawableParticleSys::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	DrawableOvercast::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	RenderOTW::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	Render2D::Release2DFontTextures(); //JAM 22Dec03

	// Wait for loader here to ensure everything which depends of this repositories is gone
	TheLoader.WaitForLoader();

	// Clean up the graphics dependent portions of these data sources
	TheFarTextures.Cleanup();
	TheTerrTextures.Cleanup();
	ObjectParent::FlushReferences();
	TheTextureBank.FlushHandles();
	ThePaletteBank.FlushHandles();
	Texture::CleanupForDevice( device->GetDefaultRC() );
}


/***************************************************************************\
	Clean up all graphics device independent data and structures.
	This should not be done until all device dependent stuff has been
	cleaned up.
\***************************************************************************/
void DeviceIndependentGraphicsCleanup( void )
{
	TheLoader.Cleanup();
	TheTimeOfDay.Cleanup();
	ObjectParent::CleanupTable();
	TheMap.Cleanup();
	TheTimeManager.Cleanup();

#ifdef GRAPHICS_USE_RES_MGR
	// Detach our resource file
	ResDetach( ResHandleTerrainTex );
#endif

#ifdef USE_SH_POOLS
	// Release our Smart Heap pools
	Palette::ReleaseStorage();
	TBlock::ReleaseStorage();
	TListEntry::ReleaseStorage();
	TBlockList::ReleaseStorage();
	MemPoolFree( glMemPool );
#endif
}
