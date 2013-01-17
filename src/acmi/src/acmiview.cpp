#pragma optimize( "", off )
#include <windows.h>
#include "resource.h"

#include "Graphics/include/renderwire.h"
#include "Graphics/include/terrtex.h"
#include "Graphics/include/rViewPnt.h"
#include "Graphics/include/loader.h"
#include "Graphics/include/drawbsp.h"
#include "Graphics/include/drawpole.h"
#include "Graphics/include/drawpnt.h"
#include "ui95/chandler.h"
#include "ui95/cthook.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "fsound.h"
#include "f4vu.h"
#include "camp2sim.h"
#include "f4error.h"
#include "falclib/include/f4find.h"
#include "dispcfg.h"
#include "acmiUI.h"
#include "playerop.h"
#include "sim/include/simbase.h"
#include "codelib/tools/lists/lists.h"
#include "acmitape.h"
#include "AcmiView.h"
#include "FalcLib/include/dispopts.h"
#include "TimeMgr.h"

extern DeviceManager		devmgr;
extern ACMIView			*acmiView;
extern int					DisplayFullScreen;
extern int					DeviceNumber;

#include "Graphics\DXEngine\DXVBManager.h"
extern	bool g_bUse_DX_Engine;


extern C_Handler *gMainHandler;

extern int TESTBUTTONPUSH;

extern LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ACMIWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void CalcTransformMatrix(SimBaseClass* theObject);
void CreateObject(SimBaseClass*);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ACMIView::ACMIView()
{
	_platform = NULL;
	_win = NULL;

	_tape = NULL;
	_entityUIMappings = NULL;

	_isReady = FALSE;
	_tapeHasLoaded = FALSE;

	_pannerX = 0.0f;
	_pannerY = 0.0f;
	_pannerZ = 0.0f;
	_pannerAz = 0.0f;
	_pannerEl = 0.0f;
	_chaseX = 0.0f;
	_chaseY = 0.0f;
	_chaseZ = 0.0f;
	_tracking = FALSE;

	_doWireFrame = 0;
	_doLockLine = 0;

	_camYaw = 0.0f;
	_camPitch = 0.0f;
	_camRoll = 0.0f;


	Init();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ACMIView::~ACMIView()
{
	Init();

	//acmiView = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::Init()
{
	StopGraphicsLoop();
	UnloadTape( FALSE );

	_drawing = FALSE;
	_drawingFinished = TRUE;
	_viewPoint = NULL;
	_renderer = NULL;

	_objectScale = 1.0F;

	//LRKLUDGE
	_doWeather = FALSE;

	memset(_fileName, 0, 40);
	InitUIVector();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void ACMIView::ToggleLabel(int doIDTags)
{
	DrawableBSP::drawLabels = doIDTags ? TRUE : FALSE;
}

void ACMIView::ToggleHeading(int val)
{
	DrawablePoled::drawHeading = val ? TRUE : FALSE;
}

void ACMIView::ToggleAltitude(int val)
{
	DrawablePoled::drawAlt = val ? TRUE : FALSE;
}

void ACMIView::ToggleAirSpeed(int val)
{
	DrawablePoled::drawSpeed = val ? TRUE : FALSE;
}

void ACMIView::ToggleTurnRate(int val)
{
	DrawablePoled::drawTurnRate = val ? TRUE : FALSE;
}

void ACMIView::ToggleTurnRadius(int val)
{
	DrawablePoled::drawTurnRadius = val ? TRUE : FALSE;
}

void ACMIView::ToggleWireFrame(int val)
{
	_doWireFrame = val;
}

void ACMIView::ToggleLockLines(int val)
{
	_doLockLine = val;
}
void ACMIView::ToggleScreenShot()
{
	_takeScreenShot ^= 1;
	_tape->SetScreenCapturing( _takeScreenShot );
};

void ACMIView::TogglePoles(int val)
{
	DrawablePoled::drawPole = val ? TRUE : FALSE;
}

void ACMIView::Togglelockrange(int val)//me123
{
	DrawablePoled::drawlockrange = val ? TRUE : FALSE;
}



// BING - TRYING TO SET THE OBJECTS NAME LABEL - FOR UNIQUE NAMES. 
void ACMIView::SetObjectName(SimBaseClass* theObject, char *tmpStr)
{
	Falcon4EntityClassType	
		*classPtr = (Falcon4EntityClassType*)theObject->EntityType();

	if(classPtr->dataType == DTYPE_VEHICLE)
	{
		//sprintf(tmpStr, "%s",((VehicleClassDataType*)(classPtr->dataPtr))->Name);
		sprintf(((VehicleClassDataType*)(classPtr->dataPtr))->Name,"%s",tmpStr);

	}
	else if(classPtr->dataType == DTYPE_WEAPON)
	{
		//sprintf(tmpStr, "%s",((WeaponClassDataType*)(classPtr->dataPtr))->Name);
		sprintf(((WeaponClassDataType*)(classPtr->dataPtr))->Name,"%s",tmpStr);
	
	}
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::SetupEntityUIMappings()
{
	int				
		i,
		numEntities;

		// BING 3-21-98
	TCHAR tmpStr[60];
 	SimTapeEntity *ep;

	ACMIEntityData *e;


//	_tape->_simTapeEntities[i].name;
				
	F4Assert(_entityUIMappings == NULL);
	F4Assert(_tape != NULL && _tape->IsLoaded());

	numEntities = _tape->NumEntities();
	_entityUIMappings = new ACMIEntityUIMap[numEntities];
	F4Assert(_entityUIMappings != NULL);

	for(i = 0; i < numEntities; i++)
	{
		_entityUIMappings[i].listboxId = -1;
		_entityUIMappings[i].menuId = -1;
		_entityUIMappings[i].name[0] = 0;

		ep = Tape()->GetSimTapeEntity(i);

		// we don't want to put chaff and flares into the list boxes
		if ( ep->flags & ( ENTITY_FLAG_CHAFF | ENTITY_FLAG_FLARE ) )
			continue;

		GetObjectName(ep->objBase, _entityUIMappings[i].name);
		if ( _entityUIMappings[i].name[0] == 0 )
			continue;

		e = Tape()->EntityData( i );
		sprintf (tmpStr, "%d %s",e->count, _entityUIMappings[i].name);
		// Update the entityUIMappings...
		sprintf(_entityUIMappings[i].name,tmpStr);
		((DrawableBSP*)(ep->objBase->drawPointer))->SetLabel (_entityUIMappings[i].name, ((DrawableBSP*)(ep->objBase->drawPointer))->LabelColor());
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

BOOL ACMIView::LoadTape(char *fname, BOOL reload )
{
	_tapeHasLoaded = FALSE;

	F4Assert(_tape == NULL);

	if ( fname[0] )
		memcpy(_fileName, fname, 40);

	// do we have a file name?
	if(_fileName[0] == 0)
		return FALSE;

	// create the tape from the file
	_tape = new ACMITape(_fileName, _renderer, _viewPoint);
	F4Assert(_tape != NULL);

	// do something go wrong?
	if(!_tape->IsLoaded())
	{
		delete _tape;
		_tape = NULL;

		MonoPrint
		(
			"InitACMIFile() --> Could not load ACMI Tape: %s.\n",
			_fileName
		);

		return FALSE;
	}


	if ( reload == FALSE )
	{
		// Setup our entity UI mappings.
		SetupEntityUIMappings();
	
	
		// Set our camera objects.
		SetCameraObject(0);
		SetTrackingObject(0);
		ResetPanner();
	}
	else
	{

	//	SetupEntityUIMappings(); // JPO fixup.
	}

	_tapeHasLoaded = TRUE;


	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::UnloadTape( BOOL reload )
{
	_tapeHasLoaded = FALSE;


	if ( reload == FALSE )
	{
		if(_entityUIMappings)
		{
			delete [] _entityUIMappings;
			_entityUIMappings = NULL;
		}
	}

	if(_tape)
	{ 
		delete _tape;
		_tape = NULL;
	}

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

char *ACMIView::SetListBoxID(int objectNum, long listID)
{
	_entityUIMappings[objectNum].listboxId = listID;
	return(_entityUIMappings[objectNum].name);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

long ACMIView::ListBoxID(int objectNum, long filter)
{
	long
		menuID = -1;

	switch(filter)
	{
		case INTERNAL_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case EXTERNAL_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case CHASE_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case WING_CAM:
			/*
			if(_tape->EntityGroup(objectNum) == 0)
			{
				menuID = _entityUIMappings[objectNum].listboxId;
			}
			break;
			*/
		case BANDIT_CAM:
			/*
			if(_tape->EntityGroup(objectNum) == 1)
			{
				menuID = _entityUIMappings[objectNum].listboxId;
			}
			break;
			*/
		case FRIEND_CAM:
			/*
			if(_tape->EntityGroup(objectNum) == 0)
			{
				menuID = _entityUIMappings[objectNum].listboxId;
			}
			break;
			*/
		case GVEHICLE_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case THREAT_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case WEAPON_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case TARGET_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case SAT_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case REPLAY_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case DIRECTOR_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
		case FREE_CAM:
				menuID = _entityUIMappings[objectNum].listboxId;
			break;
//		default:
	};

	return(menuID);
}


////////////////////////////////////////////////////////////////////////////////////



void ACMIView::GetObjectName(SimBaseClass* theObject, char *tmpStr)
{
	Falcon4EntityClassType	
		*classPtr = (Falcon4EntityClassType*)theObject->EntityType();

	memset(tmpStr, 0, 40);

	if(classPtr->dataType == DTYPE_VEHICLE || classPtr->dataType == DTYPE_WEAPON) {

		strcpy(tmpStr, ((DrawableBSP*)theObject->drawPointer)->Label());
		
		if(strlen(tmpStr) == 0) {
			
			if(classPtr->dataType == DTYPE_VEHICLE)
			{
				sprintf(tmpStr, "%s",((VehicleClassDataType*)(classPtr->dataPtr))->Name);
			}
			else if(classPtr->dataType == DTYPE_WEAPON)
			{
				sprintf(tmpStr, "%s",((WeaponClassDataType*)(classPtr->dataPtr))->Name);
			}
		}
	}
}


										
										




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::InitGraphics(C_Window *win)
{
	Tpoint
		viewPos;

	Trotation
		viewRotation;

	float				l, t, r, b, sw, sh;


	// Preload objects which will need to be instant access
	// DrawableBSP::LockAndLoad(2);

	//BING 3-20-98
	// SET UP FOR TOGGELING OF LABELS.
	// int doIDTags;
	// doIDTags = 1;
	DrawableBSP::drawLabels =  TRUE;
	DrawablePoint::drawLabels =  FALSE;

	// Load the terrain texture override image	
	// edg: need to put in switch for wireframe terrain
	if ( _doWireFrame )
	{
		wireTexture.LoadAndCreate( "WireTile.GIF", MPR_TI_PALETTE );
		wireTexture.FreeImage();
		TheTerrTextures.SetOverrideTexture( wireTexture.TexHandle() );
//		_renderer = new RenderWire;
		_renderer = new RenderOTW;
	}
	else
	{
		_renderer = new RenderOTW;
	}


	_viewPoint = new RViewPoint;


	if ( _doWireFrame == FALSE )
	{	
		_viewPoint->Setup( 0.75f * PlayerOptions.TerrainDistance() * FEET_PER_KM,
						  PlayerOptions.MaxTerrainLevel(),
						  4,
						  DisplayOptions.bZBuffering); //JAM 30Dec03
	
		_renderer->Setup(gMainHandler->GetFront(), _viewPoint);

//		_renderer->SetTerrainTextureLevel( PlayerOptions.TextureLevel() );
//		_renderer->SetSmoothShadingMode( TRUE );//PlayerOptions.GouraudOn() );
	
		_renderer->SetHazeMode(PlayerOptions.HazingOn());
		_renderer->SetDitheringMode( PlayerOptions.HazingOn() );
	
		_renderer->SetFilteringMode( PlayerOptions.FilteringOn() );
		_renderer->SetObjectDetail(PlayerOptions.ObjectDetailLevel() );
//		_renderer->SetAlphaMode(PlayerOptions.AlphaOn());
		_renderer->SetObjectTextureState(TRUE);//PlayerOptions.ObjectTexturesOn());
	}
	else
	{
		_viewPoint->Setup( 20.0f * FEET_PER_KM,
						  0,
						  2,
						  0.0f );
		_renderer->Setup(gMainHandler->GetFront(), _viewPoint);
//		_renderer->SetTerrainTextureLevel(2);
		_renderer->SetHazeMode(FALSE);
//		_renderer->SetSmoothShadingMode(FALSE);
	}

	TheVbManager.Setup(gMainHandler->GetFront()->GetDisplayDevice()->GetDefaultRC()->m_pD3D);

	sw = (float)gMainHandler->GetFront()->targetXres();
	sh = (float)gMainHandler->GetFront()->targetYres();

	l = -1.0f +((float) win->GetX() /(sw * 0.5F));
	t = 1.0f -((float) win->GetY() /(sh * 0.5F));
	r = 1.0f -((float)(sw-(win->GetX() + win->GetW())) /(sw * 0.5F));
	b = -1.0f +((float)(sh -(win->GetY() + win->GetH())) /(sh * 0.5F));
	_renderer->SetViewport(l, t, r, b);


	_isReady = TRUE;
	_drawing = TRUE;
	_drawingFinished = FALSE;

	_takeScreenShot = FALSE;
	_objectScale = 1.0F;

	viewRotation = IMatrix;

	// Update object position
	viewPos.x     = 110000.0F;
	viewPos.y     = 137000.0F;
	viewPos.z     = -15000.0F;

	_drawingFinished = FALSE;
	_initialGraphicsLoad = TRUE;
	_drawing = TRUE;

	
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::StopGraphicsLoop()
{
	if(_isReady)
	{
		_drawing = FALSE;
		_drawingFinished = TRUE;

		// This really scares me.  What is this here?  Are we asking for a race condition???
		Sleep(100);


		// Remove all references to the display device
		if(_renderer)
		{
			_renderer->Cleanup();
			delete _renderer;
			_renderer = NULL;
		}

		if(_viewPoint)
		{
			_viewPoint->Cleanup();
			delete _viewPoint;
			_viewPoint = NULL;
		}


		// Get rid of our texture override
		if ( _doWireFrame )
		{
			TheTerrTextures.SetOverrideTexture( NULL );
			wireTexture.FreeAll();
		}

		_drawingFinished = TRUE;
		_isReady = FALSE;
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int ACMIView::ExitGraphics()
{
   StopGraphicsLoop();

   return(1);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::DrawIDTags()
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void ACMIView::SetPannerXYZ( float x, float y, float z)
{
	_pannerX += x;
	_pannerY += y;
	_pannerZ += z;
}

void ACMIView::SetPannerAzEl( float az, float el )
{
	_pannerAz += az;
	_pannerEl += el;
}

void ACMIView::ResetPanner( void )
{
	_pannerAz = 0.0f;
	_pannerEl = 0.0f;
	_pannerX = 0.0f;
	_pannerY = 0.0f;
	_pannerZ = 0.0f;
}
