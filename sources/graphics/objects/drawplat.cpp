/***************************************************************************
    DrawPlat.cpp
    Scott Randolph
    April 10, 1997

    Derived class from DrawableBSP which handles large flat objects which
	can lie beneath other objects (ie: runways, carries, bridges).
***************************************************************************/
#include "DrawBldg.h"
#include "DrawPlat.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawablePlatform::pool;
#endif


/**************************************************************************
	Initialize a platform object (One which underlies other objects).
***************************************************************************/
DrawablePlatform::DrawablePlatform( float s )
: DrawableObject( s )
{
	drawClassID = Platform;

	// Set this object's height to sea level to ensure we're in the lowest object list
	position.z = 0.0f;

	// Start with no included area -- area will be added as segments are added.
	radius = 0.0f;
	InclusionRadiusSquared = 0.0f;
	maxX = maxY = -1e24f;
	minX = minY =  1e24f;

	// Fill in our callback request structures for when we get added to a parent list
	updateCBstruct.fn	= UpdateMetrics;
	updateCBstruct.self	= this;
	updateCBstruct.next	= NULL;
	updateCBstruct.prev	= NULL;
	sortCBstruct.fn		= SortForViewpoint;
	sortCBstruct.self	= this;
	sortCBstruct.next	= NULL;
	sortCBstruct.prev	= NULL;

	// Setup the lists of included objects
	flatStaticObjects.Setup();
	tallStaticObjects.Setup();
	dynamicObjects.Setup();
}



/**************************************************************************
    Remove an instance of a platform object.
	All the children must be removed before this happens.  The dynamic
	children are handled when SetParentList( NULL ) happens upon removal
	of this object from the display list.  The static objects must be
	manually removed before this point.
***************************************************************************/
DrawablePlatform::~DrawablePlatform(){
	// Mark this object as finished (no area contained)
	InclusionRadiusSquared = -1.0f;

	// Clean up our list (must be empty at this point)
	flatStaticObjects.Cleanup();
	tallStaticObjects.Cleanup();
	dynamicObjects.Cleanup();
}



/**************************************************************************
    Permanently add a flat (drivable surface) object to this container.
	( Removal is handled by the normal object removal process though 
	the ObjectDisplayList class )
	We add the containment bubble for the new surface to that accumulated
	for the rest of the platform to acheive a maximum coverage area.
***************************************************************************/
void DrawablePlatform::InsertStaticSurface( DrawableBuilding *object )
{
	ShiAssert( object );

	float size = object->Radius();

	// Compute the new aggregate object extents
	maxX = max( maxX, object->position.x + size);
	minX = min( minX, object->position.x - size );
	maxY = max( maxY, object->position.y + size );
	minY = min( minY, object->position.y - size );


	// Update our overall size and position
	position.x = (maxX + minX) * 0.5f;
	position.y = (maxY + minY) * 0.5f;

	InclusionRadiusSquared =	(maxX-position.x)*(maxX-position.x) + 
								(maxY-position.y)*(maxY-position.y);
	radius = (float)sqrt( InclusionRadiusSquared );


	// Add the new piece to the platform
	flatStaticObjects.InsertObject( object );
}



/**************************************************************************
    Permanently add a (non-flat) object to this container.
	( Removal is handled by the normal object removal process though 
	the ObjectDisplayList class )
***************************************************************************/
void DrawablePlatform::InsertStaticObject(DrawableObject* object){
	tallStaticObjects.InsertObject(object);
}



/**************************************************************************
    Draw all our children in order
***************************************************************************/
void DrawablePlatform::Draw( class RenderOTW *renderer, int LOD )
{
	DrawableObject	*obj;
	float			distance;


	// Draw all the flat objects we contain in list order
	// (might want to reverse this, so that last added to list is top most)
	flatStaticObjects.ResetTraversal();
	obj = flatStaticObjects.GetNextAndAdvance();
	while (obj){
		obj->Draw( renderer, LOD );
		obj = flatStaticObjects.GetNextAndAdvance();
	}

	// Finally, draw all the non-flat objects in range order
	tallStaticObjects.ResetTraversal();
	dynamicObjects.ResetTraversal();
	do {
	
		distance = dynamicObjects.GetNextDrawDistance();
		tallStaticObjects.DrawBeyond( distance, LOD, renderer );
		dynamicObjects.DrawBeyond( distance, LOD, renderer );

	} while (distance > -1.0f);
}



/**************************************************************************
    Draw all our children in order
***************************************************************************/
void DrawablePlatform::Draw( class Render3D *renderer )
{
	DrawableObject	*obj;
	float			distance;


	// Draw all the flat objects we contain in list order
	// (might want to reverse this, so that last added to list is top most)
	flatStaticObjects.ResetTraversal();
	obj = flatStaticObjects.GetNextAndAdvance();
	while (obj)
	{
		obj->Draw( renderer );
		obj = flatStaticObjects.GetNextAndAdvance();
	}

	// Finally, draw all the non-flat objects in range order
	tallStaticObjects.ResetTraversal();
	dynamicObjects.ResetTraversal();
	do {
	
		distance = dynamicObjects.GetNextDrawDistance();
		tallStaticObjects.DrawBeyond( distance, renderer );
		dynamicObjects.DrawBeyond( distance, renderer );

	} while (distance > -1.0f);
}



/**************************************************************************
    Handle the UpdateMetrics callback from the parent ObjectDisplayList
***************************************************************************/
void DrawablePlatform::UpdateMetrics( void *self, long listNo, const Tpoint *pos, TransportStr *transList )
{
	((DrawablePlatform*)self)->UpdateMetrics( listNo, pos, transList );
}
void DrawablePlatform::UpdateMetrics( long listNo, const Tpoint *pos, TransportStr *transList )
{
	DrawableObject *obj;


	// Have the non-zero height objects update their sorting metrics
	// (The flat objects are always drawn in insertion order, and are assumed
	//  not to move)
	tallStaticObjects.UpdateMetrics( pos );
	dynamicObjects.UpdateMetrics( listNo, pos, transList );


	// Check each dynamic object for continued membership in this container
	// (could hold this out of the parent list until after we check for
	//  new members, but this difference should normally be very small)
	dynamicObjects.ResetTraversal();
	obj = dynamicObjects.GetNextAndAdvance();
	while (obj)
	{

		// Push the object back up to our parent list if it has moved beyond our area
		if ( !ObjectInside( obj ) ) {
			dynamicObjects.RemoveObject( obj );
			parentList->InsertObject( obj );
		}
		obj = dynamicObjects.GetNextAndAdvance();
	}
}



/***************************************************************************
    Handle the SortForViewpoint callback from the parent ObjectDisplayList
***************************************************************************/
void DrawablePlatform::SortForViewpoint( void *self )
{
	((DrawablePlatform*)self)->SortForViewpoint();
}
void DrawablePlatform::SortForViewpoint( void )
{
	DrawableObject *obj;
	DrawableObject *objNext;
	float			checkDistance;

	
	// Have the non-zero height objects update their sort ordering
	// (The flat objects are always drawn in insertion order, and are assumed
	//  not to move)
	tallStaticObjects.SortForViewpoint();
	dynamicObjects.SortForViewpoint();


	// Check behind us in our parent list for new dynamic objects in our area
	checkDistance = distance + 2.0f * Radius();
	obj = prev;
	while (obj && (obj->distance < checkDistance)) {

		// Pull object down from our parent list if it's inside our area
		objNext = obj->prev;

		if ( ObjectInside( obj ) ) {
			parentList->RemoveObject( obj );
			dynamicObjects.InsertObject( obj );
		}

		obj = objNext;
	}


	// Check in front of us in our parent list for new dynamic objects in our area
	checkDistance = distance;
	obj = next;
	while (obj && (obj->distance > checkDistance)) {

		// Pull object down from our parent list if it's inside our area
		objNext = obj->next;

		if ( ObjectInside( obj ) ) {
			parentList->RemoveObject( obj );
			dynamicObjects.InsertObject( obj );
		}

		obj = objNext;
	}
}



/***************************************************************************
    See if an object is within our area, and if so, grab it from it's
	parent.
***************************************************************************/
BOOL DrawablePlatform::ObjectInside( DrawableObject *obj )
{
	Tpoint			objPos;
	float			rangeSquared;
	float			dx, dy;


	ShiAssert( obj );

	// Compute the distance from the container center to the object
	obj->GetPosition( &objPos );
	dx = objPos.x - position.x;
	dy = objPos.y - position.y;
	rangeSquared = dx * dx + dy * dy - obj->Radius();

	// return our conclusion
	return (rangeSquared < InclusionRadiusSquared);
}



/***************************************************************************
    Add ourselves to our parent list and request callbacks
***************************************************************************/
void DrawablePlatform::SetParentList( ObjectDisplayList *list )
{
	DrawableObject *obj;

	// Remove ourselves from our old parent's call chain
	if (parentList) {
		parentList->RemoveUpdateCallbacks( &updateCBstruct, &sortCBstruct, this );

		// Promote our dynamic children back up into our old parent's list
		dynamicObjects.ResetTraversal();
		obj = dynamicObjects.GetNextAndAdvance();
		while (obj)
		{
			dynamicObjects.RemoveObject( obj );
			parentList->InsertObject( obj );
			obj = dynamicObjects.GetNextAndAdvance();
		}
	}

	// Record our new parent
	parentList = list;

	// Add ourselves to our new parent's call chain
	if (parentList) {
		parentList->InsertUpdateCallbacks( &updateCBstruct, &sortCBstruct, this );
	}
}
