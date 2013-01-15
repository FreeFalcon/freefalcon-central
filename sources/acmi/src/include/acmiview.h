#ifndef _ACMIVIEW_H_
#define _ACMIVIEW_H_

#include "f4thread.h"
#include "f4vu.h"
#include "AcmiCam.h"
#include "Graphics\Include\Tex.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define EXTERNAL	0
#define CHASE		1
#define SATELLITE	8
#define REPLAY		9
#define FREE		10
#define STARTPOS	15


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class SimBaseClass;
class RViewPoint;
class RenderOTW;
class ImageBuffer;
class Render2D;
class DrawableObject;
class SimObjectType;
class C_Window;
class ACMITape;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct DBLIST
{
    void * node;          /* pointer to node data */
    void * user;          /* pointer to user data */

    struct DBLIST * next;   /* next list node */
    struct DBLIST * prev;   /* prev list node */
} DBLIST;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	int listboxId;
	int menuId;
	char name[40];
} ACMIEntityUIMap;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ACMIView
{
public:
	
	// Constructors.
	ACMIView();

	// Destructor.
	~ACMIView();

	// Access.
	ACMITape *Tape();
	RViewPoint* Viewpoint();

	void ToggleScreenShot();

	// An array of sim entity pointers.  
	ACMIEntityUIMap	*_entityUIMappings;

	
	// display toggles
	void ToggleLabel(int doIDTags);
	void ToggleHeading(int val);
	void ToggleAirSpeed(int val);
	void ToggleAltitude(int val);
	void ToggleTurnRate(int val);
	void ToggleTurnRadius(int val);
	void ToggleWireFrame(int val);
	void TogglePoles(int val);
	void ToggleLockLines(int val);
	void Togglelockrange(int val);//me123

	void InitGraphics(C_Window *win);
	int ExitGraphics();
	
	void Exec();
	void Draw();

	void GetObjectName(SimBaseClass* theObject, char *tmpStr);
	// BING - 4/15/98 
	void SetObjectName(SimBaseClass* theObject, char *tmpStr);

	void InitUIVector();
	void SetUIVector(Tpoint *tVect);
	void VectorTranslate(Tpoint *tVector);
	void VectorToVectorTranslation(Tpoint *tVector, Tpoint *offSetV);

	// List box functions.
	char *SetListBoxID(int objectNum, long listID);
	long ListBoxID(int objectNum, long filter);
	
	// Camera selection.  
	void IncrementCameraObject(int inc);
	void SetCameraObject(int theObject);
	int CameraObject();
	void IncrementTrackingObject(int inc);
	void SetTrackingObject(int theObject);
	int TrackingObject();

	// More camera selection.
	void SelectCamera(long camSel);
	void SwitchCameraObject(long cameraObject);
	void SwitchTrackingObject(long cameraObject);

	// Load and unload a tape
	BOOL LoadTape(char *fname, BOOL reload);
	void UnloadTape(BOOL reload);
	BOOL TapeHasLoaded( void ) { return _tapeHasLoaded; };

	// panner/camera control functions
	void SetPannerXYZ( float x, float y, float z );
	void SetPannerAzEl( float az, float el );
	void ResetPanner( void );
	void UpdateViewPosRot( void );
	void ToggleTracking( void )
	{
		_tracking ^= 1;
	};


public:
	int					_initialGraphicsLoad;

	int IsFinished() { return _drawingFinished; };

	char				_fileName[MAX_PATH];
	int					_cameraState;
	RViewPoint			*_viewPoint;
	RenderOTW			*_renderer;
	Texture				wireTexture;
	HWND					_win;

	// currentCam is the object we're attached to
	// currentEntityCam is the object we're tracking
	int					_currentCam;			
	int					_currentEntityCam;

	float				_objectScale;

	int					_drawing;
	int					_drawingFinished;
	int					_isReady;
	int					_tapeHasLoaded;

	int					_doWeather;

	int					_takeScreenShot;

	SimBaseClass		*_platform;

	ACMITape			*_tape;

	// camera view controls
	float				_pannerX;
	float				_pannerY;
	float				_pannerZ;
	float				_pannerAz;
	float				_pannerEl;
	float				_chaseX;
	float				_chaseY;
	float				_chaseZ;
	BOOL				_tracking;
	float				_camYaw;
	float				_camPitch;
	float				_camRoll;
	float				_camRange;

	int					_doWireFrame;
	int					_doLockLine;

	// view position and rotation of camera
	Trotation			_camRot;
	Tpoint				_camPos;
	Tpoint				_camWorldPos;

	// Initialize, used by constructor and destructor.
	void Init();

	// Setup functions.  Allocate and initialize data.
	void SetupEntityUIMappings();

	
	// Misc functions.
	void DrawIDTags();
	void ShowVersionString();
	

	// Other random functions.
	void StopGraphicsLoop();
	void InsertObjectIntoDrawList(SimBaseClass*);

	// Take a screen shot.
	void TakeScreenShot();
};

#include "acmvwinl.cpp"

extern ACMIView ACMIDriver;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // _ACMIVIEW_H_
