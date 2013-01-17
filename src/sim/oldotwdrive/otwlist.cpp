#include "Graphics\Include\Rviewpnt.h"
#include "Graphics\Include\timemgr.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\DrawObj.h"
#include "Graphics\Include\tod.h"
#include "stdhdr.h"
#include "simveh.h"
#include "ClassTbl.h"
#include "sfx.h"
#include "otwdrive.h"

void CalcTransformMatrix (SimBaseClass* theObject);

#ifdef USE_SH_POOLS
MEM_POOL	sfxRequest::pool;
MEM_POOL	displayList::pool;
MEM_POOL	drawPtrList::pool;
#endif

//#ifdef DEBUG
extern DWORD gSimThreadID;
//#endif

//extern bool g_bNeedSimThreadToRemoveObject;

// constructors/destructors for list classes
sfxRequest::sfxRequest( void )
{
}
sfxRequest::~sfxRequest( void )
{
}
displayList::displayList( void )
{
}
displayList::~displayList( void )
{
}


/*
** Name: AddSfxRequest
** Description:
**		Sim thread calls this when it needs a special effect created.
**		This adds the request to the list for processing by the
**		OTW thread.
**
*/
void
OTWDriverClass::AddSfxRequest ( SfxClass *sfxptr )
{
sfxRequest *tmpRequest;

	if (!IsActive())
		return;
	//DSP: with sim and graphics on same thread, the critical section is no longer needed
	//F4EnterCriticalSection(objectCriticalSection);

#if 1

	// create request and chain it
	tmpRequest = new sfxRequest;
	tmpRequest->next = sfxRequestRoot;
	sfxRequestRoot = tmpRequest;

	tmpRequest->sfx = sfxptr;
#else
	// create request and chain it

	// edg: if you want to test frame rate w/o any special effects running
	// (except timers), uncomment out the following....
	// if ( sfxptr->GetType() == SFX_TIMER )
	// {
	tmpRequest = new sfxRequest;
	tmpRequest->next = sfxRequestRoot;
	tmpRequest->sfx = sfxptr;
	sfxRequestRoot = tmpRequest;
	sfxRequestRoot->sfx->Start();
	// }
	// else
	// {
	// delete sfxptr;
	// }

#endif
//DSP: with sim and graphics on same thread, the critical section is no longer needed
	//F4LeaveCriticalSection(objectCriticalSection);

}

/*
** Name: DoSfxActiveList
** Description:
**		Pretty much just monitors how long the effect runs and does
**		some minor movements for the object
*/
void
OTWDriverClass::DoSfxActiveList (void)
{
	sfxRequest **sfxptrptr = NULL;
	sfxRequest *sfxptr = NULL;
	DWORD thisTime;

	// timer
	thisTime = TheTimeManager.GetClockTime();
	sfxFrameTime = SimLibMajorFrameTime;
   lastViewTime = thisTime;
//DSP: with sim and graphics on same thread, the critical section is no longer needed
//   	F4EnterCriticalSection(objectCriticalSection);

	// first move over anything in the request list to the active list
	// (if any)

	if ( sfxRequestRoot )
	{
		// find the last request
		sfxptrptr = &sfxRequestRoot;
		while (*sfxptrptr)
		{
			// save the current pointer
			sfxptr = *sfxptrptr;
			sfxptr->sfx->Start();
	
			sfxptrptr = &sfxptr->next;
		}
	
		// we've got the last request, chain to active
		if(sfxptr)
			sfxptr->next = sfxActiveRoot;
		sfxActiveRoot = sfxRequestRoot;
		sfxRequestRoot = NULL;
	}
//DSP: with sim and graphics on same thread, the critical section is no longer needed
//   	F4LeaveCriticalSection(objectCriticalSection);

   	sfxptrptr = &sfxActiveRoot;
   	while (*sfxptrptr)
   	{
		// save the current pointer
		sfxptr = *sfxptrptr;

		// exec the request
		if ( !sfxptr->sfx->Exec())
		{
			// effect finished
			// skip to the next effect and remove from chain
			(*sfxptrptr) = sfxptr->next;

			// delete effect
			delete sfxptr->sfx;
			delete sfxptr;
			continue;
		}

		// next effect
		sfxptrptr = &sfxptr->next;

   	}

}

/*
** Name: DoSfxDrawList
** Description:
**    Updates draw data for special effects
*/
void
OTWDriverClass::DoSfxDrawList (void)
{
	/*
	** edg: there's really no need for this function
	** anymore (sim and graphics on same thread).  Draw()
	** now called from sfx->Exec()
	sfxRequest **sfxptrptr;
	sfxRequest *sfxptr;


//   	F4EnterCriticalSection(objectCriticalSection);

   	sfxptrptr = &sfxActiveRoot;
   	while (*sfxptrptr)
   	{
		// save the current pointer
		sfxptr = *sfxptrptr;

		// exec the request
      	sfxptr->sfx->Draw();

		// next effect
		sfxptrptr = &sfxptr->next;
   	}

//   	F4LeaveCriticalSection(objectCriticalSection);
	*/
}

/*
** Name: ClearSfxLists
** Description:
**		Clears the active and request lists.  An exit call.
*/
void
OTWDriverClass::ClearSfxLists (void)
{
	sfxRequest **sfxptrptr;
	sfxRequest *sfxptr;

//DSP: with sim and graphics on same thread, the critical section is no longer needed
//   	F4EnterCriticalSection(objectCriticalSection);

   	sfxptrptr = &sfxActiveRoot;
   	while (*sfxptrptr)
   	{
		// save the current pointer
		sfxptr = *sfxptrptr;

		// skip to the next effect and remove from chain
		(*sfxptrptr) = sfxptr->next;

		// delete the effect data
		delete sfxptr->sfx;
		delete sfxptr;
   	}

   	sfxptrptr = &sfxRequestRoot;
   	while (*sfxptrptr)
   	{
		// save the current pointer
		sfxptr = *sfxptrptr;

		// skip to the next effect and remove from chain
		(*sfxptrptr) = sfxptr->next;

		// delete the effect data
		delete sfxptr->sfx;
		delete sfxptr;
   	}

//DSP: with sim and graphics on same thread, the critical section is no longer needed
//   	F4LeaveCriticalSection(objectCriticalSection);
}


void OTWDriverClass::InsertObject(DrawableObject *dObj)
{
	ShiAssert(GetCurrentThreadId() == gSimThreadID);

	ShiAssert( dObj );	// Could tolerate this by returning, but I don't think it happens.

	if (viewPoint && viewPoint->IsReady())
	{
		viewPoint->InsertObject(dObj);
	}
}

void OTWDriverClass::RemoveObject(DrawableObject *dObj, int deleteObject)
{
	ShiAssert(GetCurrentThreadId() == gSimThreadID);

//	if (g_bNeedSimThreadToRemoveObject && GetCurrentThreadId() != gSimThreadID)
//		return;
	
	//if (dObj) // JB 010221 CTD
	if (dObj && !F4IsBadCodePtr((FARPROC) dObj)) // JB 010221 CTD
	{
		if (dObj->InDisplayList() && viewPoint)
			viewPoint->RemoveObject(dObj); 
		if (deleteObject && !F4IsBadWritePtr(dObj, sizeof(DrawableObject))) // JB 010221 CTD
			delete dObj;
	}
}

void OTWDriverClass::AttachObject(DrawableBSP *dObj, DrawableBSP *atObj, int s)
	{
	ShiAssert(GetCurrentThreadId() == gSimThreadID);

	dObj->AttachChild(atObj, s);
	}

void OTWDriverClass::DetachObject(DrawableBSP *dObj, DrawableBSP *deObj, int s)
	{
	ShiAssert(GetCurrentThreadId() == gSimThreadID);

	dObj->DetachChild(deObj, s);
	}

void OTWDriverClass::TrimTrail(DrawableTrail *dTrail, int l)
	{
	ShiAssert(GetCurrentThreadId() == gSimThreadID);

	dTrail->TrimTrail(l);
	}

void OTWDriverClass::AddTrailHead(DrawableTrail *dTrail, float x, float y, float z)
	{
	Tpoint newPoint;

	ShiAssert(GetCurrentThreadId() == gSimThreadID);

	newPoint.x = x;
	newPoint.y = y;
	newPoint.z = z;
	dTrail->AddPointAtHead(&newPoint, vuxGameTime);
	}

//void OTWDriverClass::AddTrailTail(DrawableTrail *dTrail, float x, float y, float z)
void OTWDriverClass::AddTrailTail(DrawableTrail *, float, float, float)
	{
#if 0
	Tpoint newPoint;

	ShiAssert(GetCurrentThreadId() == gSimThreadID);

	newPoint.x = x;
	newPoint.y = y;
	newPoint.z = z;
//	dTrail->AddPointAtTail(&newPoint);
#endif
	}


void OTWDriverClass::AddToLitList( DrawableBSP* bsp )
{
	drawPtrList	*newEntry;

	ShiAssert(GetCurrentThreadId() == gSimThreadID);

#ifdef DEBUG
	for (newEntry = litObjectRoot; newEntry; newEntry = newEntry->next)
	{
		// Make sure we don't already have this object being managed
		ShiAssert( newEntry->drawPointer != bsp );
	}
#endif

	// Get it into the list
	newEntry = new drawPtrList;
	newEntry->drawPointer = bsp;
	newEntry->next = litObjectRoot;
	newEntry->prev = NULL;
	if (litObjectRoot) {
		litObjectRoot->prev = newEntry;
	}
	litObjectRoot = newEntry;

	// Do the intial set on the object
	UpdateOneLitObject( newEntry, TheTimeOfDay.GetLightLevel() );
}


void OTWDriverClass::RemoveFromLitList( DrawableBSP* bsp )
{
	drawPtrList	*entry;

	for (entry = litObjectRoot; entry; entry = entry->next)
	{
		if (entry->drawPointer == bsp) {
			// Match -- take it out
			if (entry->prev) {
				entry->prev->next = entry->next;
			} else {
				litObjectRoot = entry->next;
			}
			if (entry->next) {
				entry->next->prev = entry->prev;
			}
			delete entry;

			// Quit now that we've found our match
			break;
		}
	}
}


void OTWDriverClass::AddToNearList( DrawableObject* drawPtr, float depth )
{
	drawPtrList	*after;
	drawPtrList	*consider;
	drawPtrList	*newEntry;

#ifdef DEBUG
	for (newEntry = nearObjectRoot; newEntry; newEntry = newEntry->next)
	{
		// Make sure we don't already have this object being managed
		ShiAssert( newEntry->drawPointer != drawPtr );
	}
#endif

	// Inhibit the object's drawing since it will be handled specially
	drawPtr->SetInhibitFlag( TRUE );

	// Find the record to insert after (the nearest one more distant than us)
	after = NULL;
	for (consider = nearObjectRoot; consider; consider = consider->next)
	{
		if (consider->value <= depth) {
			break;
		}

		after = consider;
	}

	// Get it into the list
	newEntry = new drawPtrList;
	newEntry->drawPointer = drawPtr;
	newEntry->value = depth;
	newEntry->prev = after;
	if (after) {
		newEntry->next = after->next;
		after->next = newEntry;
	} else {
		newEntry->next = nearObjectRoot;
		nearObjectRoot = newEntry;
	}
	if (newEntry->next) {
		newEntry->next->prev = newEntry;
	}
}


void OTWDriverClass::FlushNearList(void)
{
	drawPtrList	*entry, *next;

	for (entry = nearObjectRoot; entry; entry = next)
	{
		// Make sure the object is turned back on.
		entry->drawPointer->SetInhibitFlag( FALSE );

		next = entry->next;
		delete entry;
	}

	nearObjectRoot = NULL;
}
