#ifndef _ACMIDRIVE_H
#define _ACMIDRIVE_H

#include "Utils\matrix.h"
#include "f4thread.h"
#include "simmath.h"
#include "f4vu.h"
#include "renderer\render2d.h"
#include "AcmiView.h"
#include "AcmiCam.h"

#define EXTERNAL	0
#define CHASE		1
#define	SATELLITE	8
#define	REPLAY		9
#define	FREE		10
#define	STARTPOS	15

class SimBaseClass;
class RViewPoint;
class RenderOTW;
class ImageBuffer;
class Render2D;
class DrawableObject;
class SimObjectType;

typedef struct {
	int					frameNum;
	int					tracking;
	int					entityCam;			
	int					entityTracking;	
	ACMICamClass		*recorderCam;
//	CRITICAL_SECTION    criticalSection;
	} rCamStruct;

typedef struct DBLIST
{
    void * node;          /* pointer to node data */
    void * user;          /* pointer to user data */

    struct DBLIST * next;   /* next list node */
    struct DBLIST * prev;   /* prev list node */
} DBLIST;

class ACMIManagerClass
{
   public:
		int numThreats, viewSwap, tgtId;
		int initialGraphicsLoad;
		int IsFinished(void) { return AcmiDrawingFinished; };
		int keyCombo, insertMode;

		ACMICamClass	*acmiCam, *attachedCam, *dettachedCam;
		BOOL			FILE_LOADED;
		rCamStruct		*recorderFrame;
		char			acmiFileName[40];
		BOOL			acmiChase;
		BOOL			acmiReplay;
		BOOL			acmiSatellite;
		BOOL			acmiFree;
		BOOL			CAM_UPDATE;
		int				acmiCameraState;
		int				camCorderCount;

		DBLIST *ACMICamListAppend( DBLIST *list, void *node );
		void ACMICamListDestroy( DBLIST *list );
		int ACMIListCount( DBLIST * list);
		DBLIST *ACMIListSearch( DBLIST *list, void *node, int searchType );
		DBLIST *ACMIListNth( DBLIST *list, int n );
		DBLIST *ACMIListSort( DBLIST **list, int sortType );
		int ACMICamListCheckFrameNum( void *node_a, void *node_b );
		int ACMICamListSortFrameNumInc( DBLIST *parent_a, DBLIST *parent_b );
		int ACMICamListSortFrameNumDec( DBLIST *parent_a, DBLIST *parent_b );
		void ACMIDeleteCamCameraNode( rCamStruct* cameraNode );
		void ACMISetCurrCamCorder( void );

		int ExitGraphics (void);
		float GetGroundLevel (float x, float y);
		float GetApproxGroundLevel (float x, float y);
		int   GetGroundIntersection (euler* dir, vector* point);
		ACMIManagerClass(void);
		~ACMIManagerClass(void);
		void InitAcmiGraphics (void);
		void StopGraphicsLoop (void);
		void ObjectSetData (SimBaseClass*, Tpoint*, Trotation*);
		void InsertObjectIntoDrawList (SimBaseClass*);
		ImageBuffer* ACMIImage;
		HWND ACMIWin;
		RViewPoint* GetViewpoint (void) { return viewPoint; };

		void InitAcmi( IDirectDrawSurface	*surface );
		void SetACMIFileName( char *fname );
		void GetObjectName (SimBaseClass* theObject, char *tmpStr );
		void InitUIVector( void );
		void SetUIVector( Tpoint *tVect );
		void SwitchACMICamera( int cameraSwitch );
		void SwitchACMICameraObject( long cameraObject );
		void SwitchACMITrackingObject( long cameraObject );
		void AcmiExecGraphics (void);
		void AcmiAddCamera (void);
		void AcmiRemoveCamera( void );
		void AcmiCutCamera( void );
		void ACMIVectorTranslate( Tpoint *tVector );
		void ACMIVectorToVectorTranslation( Tpoint *tVector, Tpoint *offSetV );
		void MoveACMIFrameSlide( int direction );
		char *SetAcmiListBoxID ( int objectNum, long listID );
		long GetAcmiListBoxID ( int objectNum, long filter );
		void AcmiCameraSelect( long camSel );

		int  NumACMIFrames( void ) { return ( acmiObjectData->numACMIFrames ); };
		int  CurrACMIFrames( void ) { return ( acmiObjectData->currACMIFrames ); };
		void StopACMI( void ) { acmiObjectData->StopACMI(); };
		void PlayACMI( void ) { acmiObjectData->PlayACMI(); };
		void PlayBackwardsACMI( void ) { acmiObjectData->PlayBackwardsACMI(); };
		void StepFowardACMI( void ) { acmiObjectData->StepFowardACMI(); };
		void StepReverseACMI( void ) { acmiObjectData->StepReverseACMI(); };
		void HomeACMI( void ) { acmiObjectData->HomeACMI(); };
		void ReverseACMI( void ) { acmiObjectData->ReverseACMI(); };
		void DirectionACMI( void ) { acmiObjectData->DirectionACMI(); };
		void FastForwardACMI( void ) { acmiObjectData->FastForwardACMI(); };
		int  UIDirectionACMI( void ) { return( acmiObjectData->playDir ); };
		void ACMITrackingObject( void ) { acmiObjectData->ACMITrackingObject( 1 ); };
		void ToggleTracking( void ) { acmiCam->ToggleTracking(); };
		void AcmiSetElDir( float diff ) { acmiCam->AcmiSetElDir( diff ); };
		void AcmiSetAzDir( float diff ) { acmiCam->AcmiSetAzDir( diff ); };
		void AcmiSetObjectEl( float diff ) { acmiCam->AcmiSetObjectEl( diff ); };
		void AcmiSetObjectAz( float diff ) { acmiCam->AcmiSetObjectAz( diff ); };
		void AcmiSetPannerAz( void ) { acmiCam->AcmiSetPannerAz(); };
		void AcmiIncrementPannerAzEl( int	currentAction, float az, float el )
				{ acmiCam->AcmiIncrementPannerAzEl( currentAction, az, el ); };
		void AcmiSetObjectRange( float diff, int instruction ) 
				{ acmiCam->AcmiSetObjectRange( diff, instruction ); };
		void AcmiSetSlewRate( float diff ) { acmiCam->AcmiSetSlewRate( diff ); };
		void SetRotateACMICameraType( int	type ) { acmiCam->SetRotateACMICameraType( type ); };
		int  GetACMICameraType( void ) { return ( acmiCam->GetACMICameraType() ); };
		void SetAcmiCameraAction( int currentAction ) { acmiCam->SetAcmiCameraAction( currentAction ); };
		void SetAcmiCameraAction( int	currentAction, float az, float el )
			 { acmiCam->SetAcmiCameraAction( currentAction, az, el ); };
		int  ACMIGetEntityCount ( void ) { return ( acmiObjectData->MAX_ENTITY_CAMS ); };
		void IncrementACMICamera( int inc ) { acmiObjectData->ACMICameraObject( inc ); };
		void IncTrackingACMICamera( int inc ) { acmiObjectData->ACMITrackingObject( inc ); };

		void AcmiRotateCameraUp( void );
		void AcmiRotateCameraDown( void );
		void AcmiRotateCameraLeft( void );
		void AcmiRotateCameraRight( void );
		void AcmiZoomInCamera( void );
		void AcmiZoomOutCamera( void );
		void CleanupACMIFileData(void);
		void InitAcmiFile ( void );
		void RemoveObjectFromDrawList (SimBaseClass*);

   private:
		typedef struct displayList
			{
			 DrawableObject* object;
			 DrawableObject* object1;
			 float x, y, z;
			 int data1;
			 displayList* next;
			};

		float objectScale;
		void RescaleAllObjects (void);
		void DrawEFOV (void);
		void DrawEFOVBox (SimObjectType*);
		void ClearRemoveList(void);
		void AddInsertList(void);
		void DoAttachList(void);
		void DoDetachList(void);
		void DoTrimList(void);
		void DoTailLists(void);
		void UpdateEntityLists(void);
		void SetCameraPosition (void);
		void DrawAllObjects(void);
		void DrawIDTags(void);
		void GetUserPosition (void);
		void ShowVersionString (void);
		RViewPoint* viewPoint;
		RenderOTW	*renderer;
		int AcmiDrawing;
		int AcmiDrawingFinished;
		int isReady;

		int doChase;
		int isShaded;
		int doPadlock;
		int doWeather;
		int viewStep;
		int tgtStep;
		int doIDTags;
		int autoScale;
		int showPos;
		int getNewCameraPos;
		int eyeFly;
		unsigned int chatterCount;
		enum {chatterLen = 255};
		char chatterStr[chatterLen + 1];
		void DrawTracers (void);
	  
		SimBaseClass* AcmiPlatform;
		SimBaseClass* padlockPriority;
		VuOrderedList* AcmiDrawList;
		VuOrderedList* featureList;
//		VuEntity* flyingEye;
		LIST	*simEntityList;
      VuThread* vuThread;

		ACMIClass *acmiObjectData;

};

extern ACMIManagerClass ACMIDriver;
#endif
