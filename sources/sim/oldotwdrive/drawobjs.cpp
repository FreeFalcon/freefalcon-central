#include "stdhdr.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\Drawbsp.h"
#include "Graphics\Include\Drawgrnd.h"
#include "Graphics\Include\Drawpnt.h"
#include "object.h"
#include "simdrive.h"
#include "simfiltr.h"
#include "aircrft.h"
#include "simeject.h"
#include "falclist.h"
#include "ground.h"
#include "camp2sim.h"
#include "Unit.h"
#include "playerOp.h"
#include "Graphics\Include\tod.h"
#include "otwdrive.h"


// Object scaling stuff
#define NUM_SCALE_PTS      7
/* actual range of target (nm) */
static  float rangeArray[NUM_SCALE_PTS] =
               {0.0F, 0.5F, 1.0F, 2.0F, 3.0F, 4.0F, 9999.0F};
/* percieved range of target (nm) */
static  float scaleArray[NUM_SCALE_PTS] =
               {1.0F, 1.5F, 2.0F, 3.0F, 4.0F, 5.0F, 5.0F};

// for debugging -- appears with frame rate display
int numObjsProcessed = 0;
int numObjsInDrawList = 0;


void OTWDriverClass::UpdateVehicleDrawables(void)
{
	SimBaseClass	*theObject;
	Tpoint			objLocation;
	Trotation		objRotation;

	if (!SimDriver.objectList)
		// The Sim isn't running yet..
		return;
	

	// Deal with the current object of interest as a special case
	if (otwPlatform && otwPlatform->drawPointer)
	{
		// Record ownship's position and orientation.
		ObjectSetData( otwPlatform, &ownshipPos, &ownshipRot );

		if (DisplayInCockpit() || !otwPlatform->OnGround()) {
			// Inhibit the drawing of the otwPlatform drawable
			otwPlatform->drawPointer->SetInhibitFlag( TRUE );
		} else {
			// Put the owtPlatform drawable back into normal draw mode
			otwPlatform->drawPointer->SetInhibitFlag( FALSE );
		} 
	}
	
	
	VuListIterator otwDrawWalker (SimDriver.objectList);
	
	numObjsProcessed = 0;
	numObjsInDrawList = 0;
	
	// Update the position of all vehicles
	for (theObject = (SimBaseClass*)otwDrawWalker.GetFirst(); theObject; theObject = (SimBaseClass*)otwDrawWalker.GetNext())
	{
		numObjsProcessed++;
		
		// Skip things without draw pointers
		if (!theObject->drawPointer) {
			continue;
		}
		
		// check for units that are LODd out....
		if ( theObject->IsSetLocalFlag( IS_HIDDEN ) )
		{
			if ( theObject->drawPointer->InDisplayList() )
			{
				RemoveObjectFromDrawList( theObject );
			}
		}
		// Its visible, so just update it.
		else if ( !theObject->IsExploding() )
		{
			// Update its position
			ObjectSetData (theObject, &objLocation, &objRotation);

			switch (theObject->drawPointer->GetClass())
			{
			  case DrawableObject::BSP:
				((DrawableBSP*)(theObject->drawPointer))->Update(&objLocation, &objRotation);
				break;

			  case DrawableObject::GroundVehicle:
			  case DrawableObject::Guys:
				((DrawableGroundVehicle*)(theObject->drawPointer))->Update(&objLocation, theObject->Yaw());
				break;

			  // TODO: should we be handling Drawable2D for the chaff/flare pseudo-bombs???
			}

			numObjsInDrawList++;
			
			// Put this object into the visual display list if it isn't already there.
			if ( !theObject->drawPointer->InDisplayList() )
			{
				InsertObjectIntoDrawList (theObject);
			}
		}
	}
	
	
	// Now update campaign unit positions
	VuListIterator	otwCampDrawWalker (SimDriver.campUnitList);
	int				doIDtags = PlayerOptions.NameTagsOn();
	
	Unit				campObject = (UnitClass*)otwCampDrawWalker.GetFirst();
	while (campObject)
	{
		// Everything with a draw pointer is visible, so just draw them.
		if (campObject->draw_pointer)
		{
			if (doIDtags)
			{
				// Update its position
				Tpoint pos;
				campObject->GetRealPosition(&pos.x,&pos.y,&pos.z);
				((DrawablePoint*)(campObject->draw_pointer))->Update(&pos);
				
				if (!campObject->draw_pointer->InDisplayList())
					InsertObject (campObject->draw_pointer);
			}
			else 
			{
				if (campObject->draw_pointer->InDisplayList())
					RemoveObject (campObject->draw_pointer);
			}
		}
		campObject = (UnitClass*)otwCampDrawWalker.GetNext();
	}
}


// Set the light level applied to the terrain textures.
void OTWDriverClass::TimeUpdateCallback( void *self ) {
	((OTWDriverClass*)self)->UpdateAllLitObjects();
}
void OTWDriverClass::UpdateAllLitObjects( void )
{
	float			lightLevel;
	drawPtrList		*entry;


	// Get the light level from the time of day manager
	lightLevel	= TheTimeOfDay.GetLightLevel();

	// Check each object and see if it wants to be turned on or off
	for (entry = litObjectRoot; entry; entry = entry->next)
	{
		UpdateOneLitObject( entry, lightLevel );
	}
}


void OTWDriverClass::UpdateOneLitObject( drawPtrList *entry, float lightLevel )
{
	ShiAssert( entry->drawPointer );

	// Simple choices for now...
	if (lightLevel > 0.3f)
	{
		// Make it day state
		((DrawableBSP*)(entry->drawPointer))->SetSwitchMask( 0, 0x2 );
	} else {
		// Make it night state
		((DrawableBSP*)(entry->drawPointer))->SetSwitchMask( 0, 0x1 );
	}
}
