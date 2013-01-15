#include "stdhdr.h"
#include "otwdrive.h"
#include "guns.h"
#include "Graphics\Include\DrawTrcr.h"
#include "Graphics\Include\Draw2d.h"
#include "Graphics\Include\drawsgmt.h"
#include "fakerand.h"
#include "playerop.h"
#include "DrawParticleSys.h" // RV - I-Hawk - added to support RV new trails code

extern bool g_bUse_DX_Engine;

void GunClass::InitTracers(){
	int i;
	float rgbScale;
	// Tpoint pos;

	if ( !(typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL) )
		return;

   // Tracers
   tracers = new DrawableTracer*[numTracers];
   trailState = new int[numTracers];
   for (i=0; i<numTracers; i++)
   {
	  rgbScale = 1.0f - (float)( (float)i / (float)numTracers );
	  tracers[i] = new DrawableTracer( 0.5f + (float)((float)i * 0.15f) );
	  if ( i == 0 )
	  	tracers[i]->SetAlpha( 0.8f );
	  else
	  	tracers[i]->SetAlpha( 1.0f );

	  tracers[i]->SetRGB( 0.45f + rgbScale * 0.55f,
	  					  0.45f + rgbScale * 0.55f,
						  0.0f + rgbScale * 0.5f );

	  // just test
	  if ( typeOfGun == GUN_TRACER_BALL )
	  	tracers[i]->SetType( TRACER_TYPE_BALL );

      trailState[i] = 0;
   }

   firstTracer =  new DrawableTracer*[numFirstTracers];
   for (i=0; i<numFirstTracers; i++)
   {
	   firstTracer[i] = new DrawableTracer( 0.5f + (float)((float)i * 0.15f) );
	  firstTracer[i]->SetAlpha( 0.7f + (float)((float)i * 0.1f) );
   }

   muzzleLoc = new Tpoint[ numFirstTracers ];
   muzzleEnd = new Tpoint[ numFirstTracers ];
   muzzleWidth = new float[ numFirstTracers ];
   muzzleAlpha = new float[ numFirstTracers ];
}

void GunClass::UpdateTracers ( int firing )
{
	int i;
	Tpoint pos, end;
	GunTracerType *bulptr;

	// JB 010108 Update is being called without init
	if (tracers == NULL || trailState == NULL)
		return;
	// JB 010108

   // do the 1st muzzle tracers -- only if alpha blending is ON
//   if ( PlayerOptions.AlphaOn() )
//   {
	   if ( firing )
	   {
		   for ( i = 0; i < numFirstTracers; i++ )
		   {
			   if ( !firstTracer[i]->InDisplayList() )
			   {
					OTWDriver.InsertObject(firstTracer[i]);
					firstTracer[i]->SetAlpha( 0.0f );
			   }
	
			   firstTracer[i]->SetAlpha( max( 0.05f, muzzleAlpha[i] * 1.0f) );
	
			   if ( i & 1 )
					firstTracer[i]->SetRGB( 1.0f, 0.2f, 0.0f );
			   else
					firstTracer[i]->SetRGB( 1.0f, 1.0f, 0.2f );
	
			   firstTracer[i]->SetWidth( max( 0.10f, muzzleWidth[i] * 0.6f) );
			   firstTracer[i]->Update( &muzzleEnd[i], &muzzleLoc[i] );
		   }
/*	   }
	   else
	   {
		   if ( firstTracer[0]->InDisplayList() )
		   {
				for ( i = 0; i < numFirstTracers; i++ )
				{
					OTWDriver.RemoveObject(firstTracer[i]);
				}
		   }
	   }*/
   }

   for (i=0; i<numTracers; i++)
   {
	    bulptr = &bullet[i];

	  	if ( bulptr->flying )
	  	{
			pos.x = bulptr->x;
			pos.y = bulptr->y;
			pos.z = bulptr->z;

			// float rtmp = max( 0.5f, PRANDFloatPos() ) * SimLibMajorFrameTime;
			float rtmp = SimLibMajorFrameTime * 0.2f;
			end.x = bulptr->x - bulptr->xdot * rtmp;
			end.y = bulptr->y - bulptr->ydot * rtmp;
			end.z = bulptr->z - bulptr->zdot * rtmp;
			tracers[i]->Update( &pos, &end );

			// bullets[i]->SetPosition( &pos );

			if ( trailState[i] == 0 )
			{
            	trailState[i] = 1;
				// OTWDriver.InsertObject(bullets[i]);
				OTWDriver.InsertObject(tracers[i]);
			}
	  	}
		else
		{
         	if (trailState[i] != 0)
         	{
            	trailState[i] = 0;
            	OTWDriver.RemoveObject(tracers[i]);
            	// OTWDriver.RemoveObject(bullets[i]);
         	}
		}
   }
}

void GunClass::CleanupTracers(){
	int i;

	if (!(typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL)){
		FireShell( NULL );
		return;
	}

	if ( tracers ){
		for (i=0; i<numTracers; i++){
			if ( tracers[i] ){
		  		OTWDriver.RemoveObject(tracers[i], TRUE);
			}
			// OTWDriver.RemoveObject(bullets[i], TRUE);
			tracers[i] = NULL;
			// bullets[i] = NULL;
		}
	}

	// delete [] bullets;
	if ( tracers ){ delete [] tracers; }
	if ( trailState ){ delete [] trailState; }
	tracers = NULL;
	// bullets = NULL;
	trailState = NULL;

	// KCK: This was in destructor - but must be cleaned up when graphics are still running
	// RV - I-Hawk - RV new trails call changes
	//if (smokeTrail)
	if (TrailIdNew){
		//OTWDriver.RemoveObject(smokeTrail);
		DrawableParticleSys::PS_KillTrail(Trail);
		//smokeTrail = NULL
		TrailIdNew = Trail = NULL;
	}

	if ( muzzleLoc ){ delete [] muzzleLoc; }
	if ( muzzleEnd ){ delete [] muzzleEnd; }
	if ( muzzleWidth ){ delete [] muzzleWidth; }
	if ( muzzleAlpha ){ delete [] muzzleAlpha; }
	muzzleLoc = NULL;
	muzzleEnd = NULL;
	muzzleWidth = NULL;
	muzzleAlpha = NULL;

	if ( firstTracer ){
		for (i=0; i<numFirstTracers; i++){
			if ( firstTracer[i] ){
				OTWDriver.RemoveObject(firstTracer[i], TRUE);
			}
			firstTracer[i] = NULL;
		}
   		delete [] firstTracer;
		firstTracer = NULL;
	}
}
