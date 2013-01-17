
// ------------------------------------------------------------------------------
// 
//	File: padlock.cpp
//
//	This file contains general padlocking functions.  These functions are primarily
// concerned with padlock object sorting and are used by the F3 and EFOV 
// padlocking views.  Also the internal cockpit views may make use of these functions.
//
// List of Contents:
//		OTWDriverClass::Padlock_FindNextObject()
//		OTWDriverClass::Padlock_ConsiderThisObject()
//		OTWDriverClass::Padlock_DetermineRelativePriority()
//		OTWDriverClass::Padlock_RankAGPriority()
//		OTWDriverClass::Padlock_RankAAPriority()
//		OTWDriverClass::Padlock_RankNAVPriority()
//		OTWDriverClass::Padlock_CheckPadlock()
//
// ------------------------------------------------------------------------------

#include <windows.h>
#include "stdhdr.h"
#include "vucoll.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "entity.h"
#include "simmover.h"
#include "object.h"
#include "classtbl.h"
#include "aircrft.h"
#include "fcc.h"
#include "radar.h"
#include "sms.h"
#include "digi.h"
#include "Graphics\Include\grtypes.h"
#include "Graphics\Include\renderow.h"
#include "flight.h"
#include "playerop.h"
/* S.G. PADLOCKING LABEL COLOR */#include "Graphics\Include\drawbsp.h"

/* S.G. FOR HMS CODE */ #include "hardpnt.h"
/* S.G. FOR HMS CODE */ #include "missile.h"
/* S.G. FOR HMS CODE */ #include "airframe.h"

#include "Feature.h" // MN 2002-04-08 test for trees and such stuff

#include "wpndef.h"		// 2002-01-27 S.G.

/* M.N. for padlock break */ #include "Graphics\Include\Tod.h"
#include "sensclas.h"
#include "visual.h"

//MI removes padlock box
extern bool g_bNoPadlockBoxes;
extern float g_fSunPadlockTimeout;

extern int g_nPadlockBoxSize;	// OW Padlock box size fix
// 2000-11-24 ADDED BY S.G. FOR PADLOCKING OPTIONS
#define PLockModeNormal 0
#define PLockModeNearLabelColor 1
#define PLockModeNoSnap 2
#define PLockModeBreakLock 4
#define PLockNoTrees 8
extern int g_nPadlockMode;
extern int g_nNearLabelLimit;
// END OF ADDED SECTION
extern float g_fPadlockBreakDistance;

static const float	COS_SUN_EFFECT_HALF_ANGLE	= (float)cos( 20.0f * DTR );//me123 changed from 10 since the sun is so small in f4


// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_FindNextPriority
//
//	Arguments:
//		doFeatures	True/False = search or skip the feature list;
//
//	Returns:
//		NONE
//
//	This function checks the player option and calls the appropiate target selection
// routine.
//	
// ------------------------------------------------------------------------------

void OTWDriverClass::Padlock_FindNextPriority(BOOL doFeatures)
{
	if(PlayerOptions.GetPadlockMode() == PDRealistic) {
		Padlock_FindRealisticPriority(doFeatures);
	}
	else {
		Padlock_FindEnhancedPriority(doFeatures);
	}
}



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_FindEnhancedPriority
//
//	Arguments:
//		doFeatures	True/False = search or skip the feature list;
//
//	Returns:
//		NONE
//
// This function was a serious pain in the ass to write.  All objects are compared
// against the current padlock candidate.  It first searches the
// player's target list for the object with the next highest priority, then it
// searches the feature list for the any features with the next lowest priority.
// The object with the next highest priority becomes the new padlock candidate.
// Gilman was on drugs when he wanted features and runways to be included in this
// search.  Searching on the target list would have been cake, but no, we had to
// make it complicated for Vince.  It differs from Padlock_FindRealisticPriority
// in that it does simple searching, i.e. there is no stepping of the yellow TD box
// within your FOV.  It just jumps the red TD box to the next target.
//	
// ------------------------------------------------------------------------------
//void OTWDriverClass::Padlock_FindEnhancedPriority(BOOL doFeatures)
void OTWDriverClass::Padlock_FindEnhancedPriority(BOOL)
{
	float						loMarkRange			= 0.0F;
	float						priorityRange		= 0.0F;
	BOOL						isLoMarkPainted		= FALSE;
	BOOL						isPriorityPainted	= FALSE;
	BOOL						isObjPainted		= FALSE;
	float						objRange			= 0.0F;
	float						objAz				= 0.0F;
	float						objEl				= 0.0F;
	float						xDiff				= 0.0F;
	float						yDiff				= 0.0F;
	float						zDiff				= 0.0F;
	int							attempt				= 0;
	BOOL						isDone				= FALSE;
	SimBaseClass*				pLoMark				= NULL;
	SimObjectType*				pObjType			= NULL;
	SimBaseClass*				pObj				= NULL;
	VuListIterator				featureWalker (SimDriver.featureList);
	Falcon4EntityClassType*		pclassPtr			= NULL;
	enum							{SearchTargets, SearchFeatures} searchMode = SearchTargets;

	/* VWF 2/15/99 */
	if(mpPadlockCandidate) {
		VuDeReferenceEntity(mpPadlockCandidate);
	}

	mpPadlockCandidate	= NULL;

	if(PlayerOptions.GetPadlockMode() == PDDisabled) {
		return;
	}


	// If we already have something padlocked
	if(mpPadlockPriorityObject) {

		pclassPtr	= (Falcon4EntityClassType*) mpPadlockPriorityObject->EntityType();

		// ... And It's a vehicle
		if (pclassPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE) {

			// walk the target list until we find the padlocked object
			pObjType	= ((SimMoverClass*)otwPlatform)->targetList;
			while(pObjType && pObjType->BaseData() != mpPadlockPriorityObject) {
				pObjType = pObjType->next;
			}

			// If we found the object
			if(pObjType) {

				// Initialize some information
				priorityRange = pObjType->localData->range;
				isPriorityPainted = pObjType->localData->sensorState[SensorClass::Radar] == SensorClass::SensorTrack;
			}
			else {
				priorityRange = 0.0F;
				isPriorityPainted = FALSE;
			}
		} // If it's a feature
		else {
			// Initialize some important data
			priorityRange			= (float)sqrt(mpPadlockPriorityObject->XPos() * mpPadlockPriorityObject->XPos() +  mpPadlockPriorityObject->YPos() * mpPadlockPriorityObject->YPos() + mpPadlockPriorityObject->ZPos() *  mpPadlockPriorityObject->ZPos());
			isPriorityPainted	= FALSE;
		}
	}


	// Initialize pointer to the player's target list,
	// and get the sim object it points to

	pObjType	= ((SimMoverClass*)otwPlatform)->targetList;
	if(pObjType) {
		pObj		= (SimBaseClass*) pObjType->BaseData();
	}
	isDone	= (pObjType == NULL);

	// If there are no objects in the target list

	if(isDone == TRUE) {

		// Initialize the pointer to the sim's feature list
	   pObj				= (SimBaseClass*)featureWalker.GetFirst();

		// If there is something in the list
		if(pObj) {

			// Change mode to search features
			searchMode	= SearchFeatures;
			isDone		= FALSE;
		}
		else {

			// Otherwise we're done, don't change the padlocked object
			mPadlockTimeout		= 0.0F;
			isDone					= TRUE;
		}
	}

	// Loop until we reach the end of the list
	while(isDone == FALSE) {

		// Calculate or copy relative geometry
		if(searchMode == SearchTargets) {
			F4Assert(pObjType);
			objRange			= pObjType->localData->range;
			objAz				= pObjType->localData->az;
			objEl				= -pObjType->localData->el;
			isObjPainted	= pObjType->localData->sensorState[SensorClass::Radar] == SensorClass::SensorTrack;
		}
		else {
			F4Assert(pObj);
			CalcRelAzEl (SimDriver.playerEntity, pObj->XPos(), pObj->YPos(), pObj->ZPos(), &objAz, &objEl);
			objEl			= - objEl;

			xDiff				= pObj->XPos() - SimDriver.playerEntity->XPos();
			yDiff				= pObj->YPos() - SimDriver.playerEntity->YPos();
			zDiff				= pObj->ZPos() - SimDriver.playerEntity->ZPos();

			objRange			= (float)sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
			isObjPainted	= FALSE;
		}

		F4Assert(pObj);

		// If this object is worth considering
		if(Padlock_ConsiderThisObject(pObj, isObjPainted, objRange, objAz, objEl)) {

			// Bascially pLoMark is the object which has the next highest priority.

// 2002-02-07 MODIFIED BY S.G. Priority are screwed up.
// This is enhanced mode so basically, we start with the highest priority then walk our way DOWN the list until we are at the bottom
// then start from the highest one again
/*			if((pObj != mpPadlockPriorityObject) &&
				((pLoMark == NULL && attempt) ||
				(Padlock_DetermineRelativePriority(mpPadlockPriorityObject, priorityRange, isPriorityPainted, pObj, objRange, isObjPainted) && 
				(pLoMark == NULL || Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, pLoMark, loMarkRange, isLoMarkPainted)))))
*/			int testAgainstPriority = FALSE;
			int testAgainstLoMark = FALSE;
			int testResult;
			int setLoMark = FALSE;

			if (pObj != mpPadlockPriorityObject) {			// Skips the current padlock object, if there is one
				if (!mpPadlockPriorityObject || attempt) {	// If we DON'T have a padlock object or we did a pass already
					testAgainstLoMark = TRUE;				//   Test against the chosen one so far
				}
				else {										// Otherwise
					testAgainstPriority = TRUE;				//   Test against the priority
					testAgainstLoMark = TRUE;				//   Test against the chosen one so far
				}

			}

			if (testAgainstPriority) {
				if (tgtStep >= 0)
					testResult = Padlock_DetermineRelativePriority(mpPadlockPriorityObject, priorityRange, isPriorityPainted, pObj, objRange, isObjPainted);
				else
					testResult = Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, mpPadlockPriorityObject, priorityRange, isPriorityPainted);
				if (testResult)
					setLoMark = TRUE;
				else
					testAgainstLoMark = FALSE;
			}

			if (testAgainstLoMark) {
				if (tgtStep >= 0)
					testResult = !pLoMark || Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, pLoMark, loMarkRange, isLoMarkPainted);
				else
					testResult = !pLoMark || Padlock_DetermineRelativePriority(pLoMark, loMarkRange, isLoMarkPainted, pObj, objRange, isObjPainted);
				if (testResult)
					setLoMark = TRUE;
				else
					setLoMark = FALSE;
			}

			if (setLoMark) // END OF ADDED SECTION 2002-02-07
			{
				pLoMark				= pObj;	
				loMarkRange			= objRange;
				isLoMarkPainted	= isObjPainted;
			}
		}

		// We're in the process of searching the player's target list
		if(searchMode == SearchTargets) {

			// Step to the next element in the target list
			pObjType	= pObjType->next;

			// If there is a next element
			if(pObjType) {

				// Get the pointer to the object
				pObj	= (SimBaseClass*) pObjType->BaseData();
			}
			else {
				// Start searching the Feature list.
			   pObj	= (SimBaseClass*)featureWalker.GetFirst();

				// If there are features in the list, search the feature list
				if(pObj) {
					searchMode	=	SearchFeatures;
				}
				else {
					// Don't change the padlocked object
					isDone		= TRUE;
					pObj			= NULL;
				}	
			}
		}
		else if(searchMode == SearchFeatures) {

			// We're searching the feature list

			pObj	= (SimBaseClass*)featureWalker.GetNext();

			if(pObj == NULL) {
				searchMode = SearchTargets;
			}
		}


		// If we're at the end of the list, decide what do now.

		if(pObj == NULL) {

			// If we reached the last lowest priority object, find the highest object
			// else we have a hi priority object, set it to be the padlock candidate.
			// Otherwise the old candidate is still retained.

			if(pLoMark == NULL) {

				// If this was the first traversal, go to the beginning and try again
				if(attempt == 0) {

					attempt++;

					// Initialize pointers to the head of the target list
					pObjType		= ((SimMoverClass*)otwPlatform)->targetList;
					if(pObjType) {
						pObj			= (SimBaseClass*) pObjType->BaseData();
					}
					searchMode	= SearchTargets;
					isDone		= (pObjType == NULL);

					// If there are no objects in the target list

					if(isDone == TRUE) {

						// Initialize the pointer to the sim's feature list
						pObj				= (SimBaseClass*)featureWalker.GetFirst();

						// If there is something in the list
						if(pObj) {

							// Change mode to search features
							searchMode	= SearchFeatures;
							isDone		= FALSE;
						}
						else {

							// Otherwise we're done
							mPadlockTimeout		= 0.0F;
							isDone					= TRUE;
						}
					}
				}
				else {
					isDone	= TRUE;
					tgtStep	= 0;
					if(mpPadlockPriorityObject == NULL) {
						snapStatus = SNAPPING;
					}
				}
			}						
			else {

				// If there is no currently padlocked object
				if(mpPadlockPriorityObject == NULL) {
					


					/* VWF 2/15/99 */
					if(mpPadlockCandidate) {
						VuDeReferenceEntity(mpPadlockCandidate);
					}

					// Set the highest object as the padlock object
					mpPadlockCandidate	= pLoMark;
					
					/* VWF 2/15/99 */
					VuReferenceEntity(mpPadlockCandidate);



               if (pLoMark)
                  mPadlockCandidateID  = pLoMark->Id();
               else
                  mPadlockCandidateID  = FalconNullId;
					mPadlockTimeout			= 0.0F;		// 1000 ms = 1 sec

					// We're done
					tgtStep		= 0;
					isDone		= TRUE;
					if(mpPadlockPriorityObject == NULL) {
						snapStatus = SNAPPING;
					}
				}
				else {

					/* VWF 2/15/99 */
					if(mpPadlockCandidate) {
						VuDeReferenceEntity(mpPadlockCandidate);
					}

					// Set the highest object as the padlock object
					mpPadlockCandidate	= pLoMark;
					
					/* VWF 2/15/99 */
					VuReferenceEntity(mpPadlockCandidate);
               if (pLoMark)
                  mPadlockCandidateID  = pLoMark->Id();
               else
                  mPadlockCandidateID  = FalconNullId;

					// Set the timer
					mPadlockTimeout		= 0.0F;		// 1000 ms = 1 sec

					// We're done
					tgtStep		= 0;
					isDone		= TRUE;
					if(mpPadlockPriorityObject == NULL) {
						snapStatus = SNAPPING;
					}
				}
			}
		}
	}
}

//**//



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_FindRealisticPriority
//
//	Arguments:
//		doFeatures	True/False = search or skip the feature list;
//
//	Returns:
//		NONE
//
// This function was a serious pain in the ass to write.  All objects are compared
// against the current padlock candidate.  It first searches the
// player's target list for the object with the next highest priority, then it
// searches the feature list for the any features with the next lowest priority.
// The object with the next highest priority becomes the new padlock candidate.
// Gilman was on drugs when he wanted features and runways to be included in this
// search.  Searching on the target list would have been cake, but no, we had to
// make it complicated for Vince.
//	
// ------------------------------------------------------------------------------
//void OTWDriverClass::Padlock_FindRealisticPriority(BOOL doFeatures)
void OTWDriverClass::Padlock_FindRealisticPriority(BOOL)
{
	float						loMarkRange				= 0.0F;
	float						candidateRange			= 0.0F;
	float						priorityRange			= 0.0F; // 2002-02-07 ADDED BY S.G.
	BOOL						isLoMarkPainted			= FALSE;	
	BOOL						isCandidatePainted		= FALSE;
	BOOL						isPriorityPainted		= FALSE; // 2002-02-07 ADDED BY S.G.
	BOOL						isObjPainted			= FALSE;
	float						objRange				= 0.0F;
	float						objAz					= 0.0F;
	float						objEl					= 0.0F;
	float						xDiff					= 0.0F;
	float						yDiff					= 0.0F;
	float						zDiff					= 0.0F;
	int							attempt					= 0;
	BOOL						isDone					= FALSE;
	SimBaseClass*				pLoMark					= NULL;
	SimObjectType*				pObjType				= NULL;
	SimBaseClass*				pObj					= NULL;
	VuListIterator				featureWalker (SimDriver.featureList);
	Falcon4EntityClassType*		pclassPtr				= NULL;
	enum							{SearchTargets, SearchFeatures} searchMode = SearchTargets;

	if(PlayerOptions.GetPadlockMode() == PDDisabled) {
		return;
	}

	// Okay
	if(mpPadlockCandidate && mpPadlockCandidate->GetCampaignObject() != ((CampBaseClass*)0xdddddddd) && !mpPadlockCandidate->IsDead()) {

		pclassPtr	= (Falcon4EntityClassType*) mpPadlockCandidate->EntityType();

		if (pclassPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE) {

			pObjType	= ((SimMoverClass*)otwPlatform)->targetList;
			while(pObjType && pObjType->BaseData() != mpPadlockCandidate) {
				pObjType = pObjType->next;
			}

			if(pObjType) {
				candidateRange = pObjType->localData->range;
				isCandidatePainted = pObjType->localData->sensorState[SensorClass::Radar] == SensorClass::SensorTrack;
			}
			else {
				candidateRange = 0.0F;
/* VWF 2/15/99 */
				if(mpPadlockCandidate) {
					VuDeReferenceEntity(mpPadlockCandidate);
				}
				mpPadlockCandidate  = NULL;
            mPadlockCandidateID = FalconNullId;
				isCandidatePainted  = FALSE;
			}
		}
		else {
			candidateRange			= (float)sqrt(mpPadlockCandidate->XPos() * mpPadlockCandidate->XPos() +  mpPadlockCandidate->YPos() * mpPadlockCandidate->YPos() + mpPadlockCandidate->ZPos() *  mpPadlockCandidate->ZPos());
			isCandidatePainted	= FALSE;
		}
	}

	// 2002-02-07 ADDED BY S.G. Now do the same for mpPadlockPriorityObject
	if(mpPadlockPriorityObject && mpPadlockPriorityObject->GetCampaignObject() != ((CampBaseClass*)0xdddddddd) && !mpPadlockPriorityObject->IsDead()) {

		pclassPtr	= (Falcon4EntityClassType*) mpPadlockPriorityObject->EntityType();

		if (pclassPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE) {

			pObjType	= ((SimMoverClass*)otwPlatform)->targetList;
			while(pObjType && pObjType->BaseData() != mpPadlockPriorityObject) {
				pObjType = pObjType->next;
			}

			if(pObjType) {
				priorityRange = pObjType->localData->range;
				isPriorityPainted = pObjType->localData->sensorState[SensorClass::Radar] == SensorClass::SensorTrack;
			}
			else {
				priorityRange = 0.0F;
				isPriorityPainted  = FALSE;
			}
		}
		else {
			priorityRange			= (float)sqrt(mpPadlockPriorityObject->XPos() * mpPadlockPriorityObject->XPos() +  mpPadlockPriorityObject->YPos() * mpPadlockPriorityObject->YPos() + mpPadlockPriorityObject->ZPos() *  mpPadlockPriorityObject->ZPos());
			isPriorityPainted	= FALSE;
		}
	}

	// Initialize pointer to the player's target list,
	// and get the sim object it points to

	pObjType	= ((SimMoverClass*)otwPlatform)->targetList;
	if(pObjType) {
		pObj		= (SimBaseClass*) pObjType->BaseData();
	}
	isDone	= (pObjType == NULL);

	// If there are no objects in the target list

	if(isDone == TRUE) {

		// Initialize the pointer to the sim's feature list
	   pObj				= (SimBaseClass*)featureWalker.GetFirst();

		// If there is something in the list
		if(pObj) {

			// Change mode to search features
			searchMode	= SearchFeatures;
			isDone		= FALSE;
		}
		else {

			// Otherwise we're done
			mPadlockTimeout		= 0.0F;
		/* VWF 2/15/99 */
			if(mpPadlockCandidate) {
				VuDeReferenceEntity(mpPadlockCandidate);
			}

			mpPadlockCandidate	= NULL;
			isDone					= TRUE;
		}
	}

	// Loop until we reach the end of the list
	while(isDone == FALSE) {

		// Calculate or copy relative geometry
		if(searchMode == SearchTargets) {
			F4Assert(pObjType);
			objRange			= pObjType->localData->range;
			objAz				= pObjType->localData->az;
			objEl				= -pObjType->localData->el;
			isObjPainted	= pObjType->localData->sensorState[SensorClass::Radar] == SensorClass::SensorTrack;
		}
		else {
			F4Assert(pObj);
			CalcRelAzEl (SimDriver.playerEntity, pObj->XPos(), pObj->YPos(), pObj->ZPos(), &objAz, &objEl);
			objEl			= - objEl;

			xDiff				= pObj->XPos() - SimDriver.playerEntity->XPos();
			yDiff				= pObj->YPos() - SimDriver.playerEntity->YPos();
			zDiff				= pObj->ZPos() - SimDriver.playerEntity->ZPos();

			objRange			= (float)sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
			isObjPainted	= FALSE;
		}

		F4Assert(pObj);

		// If this object is worth considering
		if(Padlock_ConsiderThisObject(pObj, isObjPainted, objRange, objAz, objEl)) {

			// Bascially pLoMark is the object which has the next highest priority.
			// Criterea to be selected for the pLoMark is:
			//	1) The priority of pObj is less than the priority of mpPadlockCandidate
			// 2) ... and one of the following:
			//		a) pLoMark hasn't been set yet
			//		b) ... or The priority of pObj is greater than the priority of pLoMark
			//	3) ... and on of the following
			//		a) There is an existing mpPadlockCandidate and pObj is not the existing mpPadlockCandidate
			//		b) ... or There is no existing mpPadlockCandidate and pObj is not the padlockPriorityObject.  This
			//				 prevents us from highlighting the padlocked object when switching to F3 padlock from some
			//				 view.  I.E. the padlockPriorityObject will not be highlighted during the first traversal.
			//		c) ... or there is no mpPadlockCandidate but pObj is the padlockPriorityObject and this is a
			//				 subsequent traversal.

// 2002-02-07 MODIFIED BY S.G. Priority are screwed up.
/*			if(((attempt && mpPadlockCandidate == NULL && pObj == mpPadlockPriorityObject) ||
				(mpPadlockCandidate == NULL && pObj != mpPadlockPriorityObject) ||
				(mpPadlockCandidate != NULL && pObj != mpPadlockCandidate)) && 
				Padlock_DetermineRelativePriority(mpPadlockCandidate, candidateRange, isCandidatePainted, pObj, objRange, isObjPainted) && 
				(pLoMark == NULL || Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, pLoMark, loMarkRange, isLoMarkPainted)))
*/			int testAgainstCandidate = FALSE;
			int testAgainstPriority = FALSE;
			int testAgainstLoMark = FALSE;
			int testResult;
			int setLoMark = FALSE;
			if ((attempt && mpPadlockCandidate == NULL && pObj == mpPadlockPriorityObject) ||	// It's a second pass and nothing is found OR
				(mpPadlockCandidate == NULL && pObj != mpPadlockPriorityObject) ||				// we have no candidate and the current is not the priority OR
				(mpPadlockCandidate != NULL && pObj != mpPadlockCandidate))	{					// we have a candidate and the current is not the candidate

				if ((!mpPadlockCandidate && !mpPadlockPriorityObject) || attempt)				// We don't have a candidate and neither a priority OR we're on our second pass
					testAgainstLoMark = TRUE;													//   Test against the chosen one so far
				else if (mpPadlockCandidate) {													// We do have a candidate
					testAgainstCandidate = TRUE;												//   Test against the candidate
					testAgainstLoMark = TRUE;													//   Test against the chosen one so far
				}
				else if (mpPadlockPriorityObject) {												// If we don't have a canditate but we have a priority
					testAgainstPriority = TRUE;													//   Test against the priority
					testAgainstLoMark = TRUE;													//   Test against the chosen one so far
				}
			}
				
			if (testAgainstCandidate) {
				if (tgtStep >= 0)
					testResult = Padlock_DetermineRelativePriority(mpPadlockCandidate, candidateRange, isCandidatePainted, pObj, objRange, isObjPainted);
				else
					testResult = Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, mpPadlockCandidate, candidateRange, isCandidatePainted);
				if (testResult)
					setLoMark = TRUE;
				else
					testAgainstLoMark = FALSE;
			}

			if (testAgainstPriority) {
				if (tgtStep >= 0)
					testResult = Padlock_DetermineRelativePriority(mpPadlockPriorityObject, priorityRange, isPriorityPainted, pObj, objRange, isObjPainted);
				else
					testResult = Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, mpPadlockPriorityObject, priorityRange, isPriorityPainted);
				if (testResult)
					setLoMark = TRUE;
				else
					testAgainstLoMark = FALSE;
			}

			if (testAgainstLoMark) {
				if (tgtStep >= 0)
					testResult = !pLoMark || Padlock_DetermineRelativePriority(pObj, objRange, isObjPainted, pLoMark, loMarkRange, isLoMarkPainted);
				else
					testResult = !pLoMark || Padlock_DetermineRelativePriority(pLoMark, loMarkRange, isLoMarkPainted, pObj, objRange, isObjPainted);
				if (testResult)
					setLoMark = TRUE;
				else
					setLoMark = FALSE;
			}

			if (setLoMark) // END OF ADDED SECTION 2002-02-07
			{
				pLoMark				= pObj;	
				loMarkRange			= objRange;
				isLoMarkPainted	= isObjPainted;
			}
		}

		// We're in the process of searching the player's target list
		if(searchMode == SearchTargets) {

			// Step to the next element in the target list
			pObjType	= pObjType->next;

			// If there is a next element
			if(pObjType) {

				// Get the pointer to the object
				pObj	= (SimBaseClass*) pObjType->BaseData();
			}
			else {
				// Start searching the Feature list.
			   pObj	= (SimBaseClass*)featureWalker.GetFirst();

				// If there are features in the list, search the feature list
				if(pObj) {
					searchMode	=	SearchFeatures;
				}
				else {
					isDone		= TRUE;
					pObj			= NULL;
				}	
			}
		}
		else if(searchMode == SearchFeatures) {

			// We're searching the feature list

			pObj	= (SimBaseClass*)featureWalker.GetNext();

			if(pObj == NULL) {
				searchMode = SearchTargets;
			}
		}


		// If we're at the end of the list, decide what do now.

		if(pObj == NULL) {

			// If we reached the last lowest priority object, find the highest object
			// else we have a hi priority object, set it to be the padlock candidate.
			// Otherwise the old candidate is still retained.

			if(pLoMark == NULL) {

				/* VWF 2/15/99 */
				if(mpPadlockCandidate) {
					VuDeReferenceEntity(mpPadlockCandidate);
				}

				mpPadlockCandidate	= NULL;
            mPadlockCandidateID  = FalconNullId;

				// If this was the first traversal, go to the beginning and try again
				if(attempt == 0) {

					attempt++;

					// Initialize pointers to the head of the target list
					pObjType		= ((SimMoverClass*)otwPlatform)->targetList;
					if(pObjType) {
						pObj			= (SimBaseClass*) pObjType->BaseData();
					}
					searchMode	= SearchTargets;
					isDone		= (pObjType == NULL);

					// If there are no objects in the target list

					if(isDone == TRUE) {

						// Initialize the pointer to the sim's feature list
						pObj				= (SimBaseClass*)featureWalker.GetFirst();

						// If there is something in the list
						if(pObj) {

							// Change mode to search features
							searchMode	= SearchFeatures;
							isDone		= FALSE;
						}
						else {

							// Otherwise we're done
							mPadlockTimeout		= 0.0F;

							/* VWF 2/15/99 */
							if(mpPadlockCandidate) {
								VuDeReferenceEntity(mpPadlockCandidate);
							}

							mpPadlockCandidate	= NULL;
                     mPadlockCandidateID  = FalconNullId;
							isDone					= TRUE;
						}
					}
				}
				else {
					isDone	= TRUE;
					tgtStep	= 0;

					if(mpPadlockPriorityObject == NULL) {
						snapStatus = SNAPPING;
					}
				}
			}						
			else {

				// If there is no currently padlocked object
				if(mpPadlockPriorityObject == NULL) {
					
					// Set the highest object as the padlock object, no candidates
/* 2001-01-29 MODIFIED BY S.G. FOR THE NEW mpPadlockPrioritySimObject
					mpPadlockPriorityObject	= pLoMark;
					VuReferenceEntity(mpPadlockPriorityObject);
*/
					SetmpPadlockPriorityObject(pLoMark);

					mPadlockTimeout			= 0.0F;		// 1000 ms = 1 sec
					mTDTimeout					= 50.0F;

					// We're done

					tgtStep		= 0;
					isDone		= TRUE;
					if(mpPadlockPriorityObject == NULL) {
						snapStatus = SNAPPING;
					}
				}
				else {

					// Make the next lowest object the candidate
					/* VWF 2/15/99 */
					if(mpPadlockCandidate) {
						VuDeReferenceEntity(mpPadlockCandidate);
					}

					mpPadlockCandidate	= pLoMark;
					
					/* VWF 2/15/99 */
					VuReferenceEntity(mpPadlockCandidate);

               if (pLoMark)
                  mPadlockCandidateID  = pLoMark->Id();
               else
                  mPadlockCandidateID  = FalconNullId;


					// Set the timer
					mPadlockTimeout		= 1.0F;		// 1000 ms = 1 sec

					// We're done
					tgtStep		= 0;
					isDone		= TRUE;

					if(mpPadlockPriorityObject == NULL) {
						snapStatus = SNAPPING;
					}

				}
			}
		}
	}
}

//**//



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_ConsiderThisObject()
//
// Arguements:
//		pObj		Pointer to the object that we want to test
//		range		Distance from player to the object
//		az			Relative Azimuth to the object
//		el			Relative Elevation to the object
//
//	Returns:
//		isConsidered	True if the object is considered, False otherwise
//
// This function weeds out irrelevant objects before be attempt to compare them
// in the padlock modes.
//
// Critera for object to be considered:
// 1) Object must be closer than 8 nautical miles
// 2) Object cannot be occluded ...
// 3) Were in a padlock mode and one of the following:
//		A) The player selected simplified padlocking
//		B) The player selected realistic padlocking and the object is in the viewport.
// ------------------------------------------------------------------------------

BOOL OTWDriverClass::Padlock_ConsiderThisObject(SimBaseClass* pObj, BOOL isPainted, float range, float az, float el)
{

	FireControlComputer::FCCMasterMode fccMasterMode;
	FireControlComputer::FCCSubMode fccSubMode;
	BOOL		isConsidered = FALSE;
	enum	{AA, AG, NAV} mode;
	bool		checkobject	= FALSE;
	RadarClass *pradar=NULL; // 2002-03-12 S.G.

// 2002-01-24 REMOVED BY S.G. Not necessary and prevents missiles from being padlocked.
//	if (pObj && pObj->IsSim() && pObj != otwPlatform && !pObj->IsWeapon()) 
//	{
// 2002-01-27 MODIFIED BY S.G. But prevent none threatning stuff from being padlocked
	if (pObj && pObj->IsSim() && pObj->IsWeapon()) {
		Falcon4EntityClassType* classPtr;
		SimWeaponDataType* wpnDefinition;

		classPtr = &(Falcon4ClassTable[((MissileClass *)pObj)->Type() - VU_LAST_ENTITY_TYPE]);
		wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];
		if ((WeaponClass)wpnDefinition->weaponClass != wcAimWpn && (WeaponClass)wpnDefinition->weaponClass != wcSamWpn) 
			return FALSE;
	}

	if (pObj && pObj->IsSim() && pObj->IsEject()) // 2002-02-17 ADDED BY S.G. Don't padlock chutes
		return FALSE;

	if (range < g_fPadlockBreakDistance * NM_TO_FT || isPainted)
	{

		// Check what mode the fire control computer is in
		fccMasterMode = SimDriver.playerEntity->FCC->GetMasterMode();
		fccSubMode = SimDriver.playerEntity->FCC->GetSubMode();

		pradar = (RadarClass*) FindSensor(SimDriver.playerEntity, SensorClass::Radar); // 2002-03-12 ADDED BY S.G. Get the player's radar

		// 2002-03-12 ADDED BY S.G. If the player held the shift key when pressing down the padlock key, prioritize AA things
		if (padlockPriority == PriorityAA || padlockPriority == PriorityMissile)
		{
			mode = AA;
		}
		// END OF ADDED SECTION 2002-03-12
		// If in a AG mode make note of it
		else if(fccMasterMode == FireControlComputer::AirGroundBomb ||		
				fccMasterMode == FireControlComputer::AirGroundMissile ||
				fccMasterMode == FireControlComputer::AirGroundHARM ||
				fccMasterMode == FireControlComputer::AirGroundLaser ||
				(fccMasterMode == FireControlComputer::AGGun &&
				fccSubMode == FireControlComputer::STRAF) ||	// MN added
				(pradar && pradar->IsAG()) || // 2002-03-12 ADDED BY S.G. If our radar is in AG mode, then prioritize ground object
				padlockPriority == PriorityAG) // 2002-03-12 ADDED BY S.G. If the player held the control key when pressing down the padlock key, prioritize AG things
		{
			mode = AG;
		}
		else if(fccMasterMode == FireControlComputer::ILS ||
				fccMasterMode == FireControlComputer::Nav)
		{
			mode = NAV;
		}
		else
		{
			mode = AA;
		}

		// Proceed if object is on ground and fcc is in AG mode.  Proceed if in object is in
		// air and fcc is in AA mode

		if((pObj->OnGround() && mode == AG) || pObj->IsMissile() || (!pObj->OnGround() && mode == AA) || 	
			(mode == NAV && (!pObj->OnGround() || ((Falcon4EntityClassType*)pObj->EntityType())->vuClassData.classInfo_[VU_TYPE] == TYPE_RUNWAY)))
		{

			// Check if this azimuth and elevation lies in the occluded zone
			// If not occluded, then continue
			if(Padlock_CheckOcclusion(az, el) == FALSE && !pObj->IsDead()) {

				// If we are in realistic mode, keep checking.  Otherwise consider the object.
				if(PlayerOptions.GetPadlockMode() == PDRealistic) {

					ThreeDVertex	objectPoint;
					Tpoint			objectLoc;


					if(GetOTWDisplayMode() == ModePadlockF3 || GetOTWDisplayMode() == Mode3DCockpit) {
						objectLoc.x	= pObj->XPos() - SimDriver.playerEntity->XPos();
						objectLoc.y = pObj->YPos() - SimDriver.playerEntity->YPos();
						objectLoc.z = pObj->ZPos() - SimDriver.playerEntity->ZPos();
					}
					else {
						objectLoc.x	= pObj->XPos();
						objectLoc.y = pObj->YPos();
						objectLoc.z = pObj->ZPos();
					}

					renderer->TransformPoint(&objectLoc, &objectPoint);

					if(objectPoint.clipFlag == 0) {
						isConsidered = TRUE;
					}
				}
				else {
					// In simplified mode, automatically consider the object
					isConsidered = TRUE;
				}
			}
		}
	}
//	}

	return isConsidered;
}

//**//



// ------------------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_DetermineRelativePriority()
// 
//	Arguements:
//		pObjA		pointer to the first object to be compared
//		rangeA	distance from player to object A
//		pObjB		pointer to the second object to be compared
//		rangeB	distance from player to object B
//
//	Returns:
//		returnVal	TRUE if priority of pObjA >= priority of pObjB or pObjA == NULL; 
//						FALSE otherwise.
//
// This function compares two objects and determines which has the higher
// padlock sorting priority.  The document entitled "The Falcon 4.0 Specification
// for Target Sorting and Tracking in Padlock Modes" (file name targeting_rev_.doc)
// has the specificaton about how to order the target list.
//
// ------------------------------------------------------------------------------------------

BOOL OTWDriverClass::Padlock_DetermineRelativePriority(SimBaseClass* pObjA, float rangeA, BOOL isAPainted, SimBaseClass* pObjB, float rangeB, BOOL isBPainted)
{	
	BOOL	returnVal;
	int	priorityA;
	int	priorityB;
	FireControlComputer::FCCMasterMode fccMasterMode;
	RadarClass *pradar=NULL; // 2002-03-12 S.G.
	enum	{AA, AG, NAV} mode;

	if(pObjA == NULL) {
		return TRUE;
	}

	if(pObjB == NULL) {
		return FALSE;
	}

	if(pObjA == pObjB) {
		return TRUE;
	}

	// Check what mode the fire control computer is in
	fccMasterMode = SimDriver.playerEntity->FCC->GetMasterMode();

	pradar = (RadarClass*) FindSensor(SimDriver.playerEntity, SensorClass::Radar); // 2002-03-12 ADDED BY S.G. Get the player's radar

	// 2002-03-12 ADDED BY S.G. If the player held the shift key when pressing down the padlock key, prioritize AA things
	if (padlockPriority == PriorityAA || padlockPriority == PriorityMissile)
	{
		mode = AA;
	}
	// END OF ADDED SECTION 2002-03-12
	// If in a AG mode make note of it
	else if(fccMasterMode == FireControlComputer::AirGroundBomb ||		
			fccMasterMode == FireControlComputer::AirGroundMissile ||
			fccMasterMode == FireControlComputer::AirGroundHARM ||
			fccMasterMode == FireControlComputer::AirGroundLaser ||
			(pradar && pradar->IsAG()) ||  // 2002-03-12 ADDED BY S.G. If our radar is in AG mode, then prioritize ground object
			padlockPriority == PriorityAG) // 2002-03-12 ADDED BY S.G. If the player held the control key when pressing down the padlock key, prioritize AG things
	{
		mode = AG;
	} // If we're in Nav mode, make note
	else if(fccMasterMode == FireControlComputer::ILS ||
			fccMasterMode == FireControlComputer::Nav)
	{
		mode = NAV;
	}
	else	// Otherwise we're in AA mode
	{
		mode = AA;
	}

	// If were searching for ground targets
	if(mode == AG) {
		priorityA = Padlock_RankAGPriority(pObjA, isAPainted);
		priorityB = Padlock_RankAGPriority(pObjB, isBPainted);
	} // If we're in Nav mode
	else if(mode == NAV){
		priorityA = Padlock_RankNAVPriority(pObjA, isAPainted);
		priorityB = Padlock_RankNAVPriority(pObjB, isBPainted);
	} // Otherwise we're in AA
	else {
		priorityA = Padlock_RankAAPriority(pObjA, isAPainted);
		priorityB = Padlock_RankAAPriority(pObjB, isBPainted);
	}

	if(priorityA < priorityB) {					// Obj A has higher priority
		returnVal = TRUE;
	}
	else if(priorityA == priorityB) {			// Priorities are equal
		if(rangeA <= rangeB) {						// A is closer
			returnVal = TRUE;
		}	
		else {
			returnVal = FALSE;
		}
	}
	else {												// B has higher priority
		returnVal = FALSE;
	}

	return returnVal;
}

//**//



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_RankAGPriority()
// 
//	Arguments:
//		pObj		pointer to the object we wish to compare
//
//	Returns:
//		priority	padlock priority of the object
//
// This function searches thru all the different properties of the object and
// ranks the importance of the object.  Ranking is specified by section 3. of the
// document entitled "Falcon 4.0 Specification for Target Sorting and Tracking in
// Padlock Modes, filename: targeting_rev_A.doc".  Targets with higher importance
// will get lower rankings.  Since this pObj can either be a vehicle or a feature
// we must check the object's datatype and then handle the object appropiately.
//
// ------------------------------------------------------------------------------

int OTWDriverClass::Padlock_RankAGPriority(SimBaseClass* pObj, BOOL isPainted)
{
	int							priority=0;
	int							objSide=0;
	int							playerSide=0;
	char						objtype=0;
	char						objstype=0;
	char						objclass=0;
	Falcon4EntityClassType*		pclassPtr=NULL;
	RadarClass*					pradar=NULL;
	SimObjectType*				pplayerLockedTgt=NULL;
	SimMoverClass*				pobjTgt=NULL;
	AircraftClass*				pplayer=NULL;
// 2002-04-08 MN check for "NO HITEVAL" features like trees
	FeatureClassDataType		*fc = NULL;

	// Get the object's type pointer
	pclassPtr	= (Falcon4EntityClassType*) pObj->EntityType();

	// Get object's type
	objtype		= pclassPtr->vuClassData.classInfo_[VU_TYPE];

	// Get object's stype
	objstype		= pclassPtr->vuClassData.classInfo_[VU_STYPE];

	// Get the object's data type
	objclass		= pclassPtr->vuClassData.classInfo_[VU_CLASS];

	// If the object is a feature
	if(objclass == CLASS_FEATURE) {
// 2002-04-08 MN don't padlock tree features
		fc = GetFeatureClassData (pObj->Type() - VU_LAST_ENTITY_TYPE);
		if ((g_nPadlockMode & PLockNoTrees) && fc && (fc->Flags & FEAT_NO_HITEVAL))
			priority = 100;
		else
			
		// If friendly or neutral runway
		if(objtype == TYPE_RUNWAY) {// && friendly or neutral) {
			priority = 5;
		} // Just a ground feature
		else {
			priority = 8;
		}	
	}
	else if(objclass == CLASS_VEHICLE) {
		// Get the object's target
		if(((SimMoverClass*) pObj)->targetPtr) {
			pobjTgt	= (SimMoverClass*) ((SimMoverClass*) pObj)->targetPtr->BaseData();
		}

		// Get the object's side
		objSide		= pObj->GetTeam();

		// Get the pointer to the player
		pplayer		= SimDriver.playerEntity;

		// Get the player's side
		playerSide	= pplayer->GetTeam();


	// Check on these values
		// Get the player's radar
		pradar		= (RadarClass*) FindSensor(pplayer, SensorClass::Radar);

		// 2002-03-01 ADDED BY S.G. What if we have no radar? Should it happen? It did and CTD'ed
		F4Assert(pradar);
		if (!pradar)
			return priority;
		// END OF ADDED SECTION 2002-03-01

		// Get the player radar's locked target
		pplayerLockedTgt	= (SimObjectType*) pradar->CurrentTarget();

		// If missile guiding upon the player
		if(objSide != playerSide && objtype == TYPE_MISSILE) {// && pobjTgt == pplayer) {
			priority = 0;
		}
		else if(objSide == playerSide && objtype == TYPE_MISSILE) {// && pobjTgt == pplayer) {
			priority = 10;
		} // If object is locked by player
		else if(pplayerLockedTgt && pplayerLockedTgt->BaseData() == pObj) {
			priority = 1;
		} // If object is attacking player
		else if(pobjTgt == pplayer && ((SimVehicleClass*) pObj)->GetSMS()->GetCurrentWeapon()) { // KCK: This isn't always a ground class! -> || ((GroundClass*)pbaseData)->Gun)) {
			priority = 2;
		} // If object is enemy and being painted by player
		else if(objSide != playerSide && isPainted) {
			priority = 3;
		} // If object is enemy vehicle
		else if(objSide != playerSide) {
			priority = 4;	
		}// If object is friendly and being painted by player
		else if(objSide == playerSide && isPainted) {
			priority = 6;
		} // If friendly vehicle
		else if(objSide == playerSide) {
			priority = 7;
		} // Everything else
		else {
			priority = 9;
		}
	}
	else {
		priority = 10;
	}

	return priority;
}

//**//



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_RankAAPriority()
// 
//	Arguments:
//		pObj		pointer to the object we wish to compare
//
//	Returns:
//		priority	padlock priority of the object
//
// This function searches thru all the different properties of the object and
// ranks the importance of the object.  Ranking is specified by section 3. of the
// document entitled "Falcon 4.0 Specification for Target Sorting and Tracking in
// Padlock Modes, filename: targeting_rev_A.doc".  Targets with higher importance
// will get lower rankings.  Just to make Vince's life more difficult, pObj can
// either be a vehicle or a feature (runway).
//
// ------------------------------------------------------------------------------

int OTWDriverClass::Padlock_RankAAPriority(SimBaseClass* pObj, BOOL isPainted)
{
	int							priority;
	int							objSide;
	int							playerSide;
	char							objtype;
	char							objstype;
	Falcon4EntityClassType* pclassPtr;
	RadarClass*					pradar;
	SimObjectType*				pplayerLockedTgt = NULL;
	SimMoverClass*				pobjTgt = NULL;
	AircraftClass*				pplayer;
	DigitalBrain*				pbrain = NULL;
	DigitalBrain::DigiMode	mode;

	// Get the object's type pointer
	pclassPtr	= (Falcon4EntityClassType*) pObj->EntityType();

	// Get object's type
	objtype		= pclassPtr->vuClassData.classInfo_[VU_TYPE];

	// Get object's stype
	objstype		= pclassPtr->vuClassData.classInfo_[VU_STYPE];

	// Get the object's target
//	if(pObj->targetPtr) {
//		pobjTgt	= (SimMoverClass*) pObj->targetPtr->BaseData();
//	}
	// Get the object's brain, if individual deaggregated aircraft
	if(pObj->IsAirplane ()) //objtype == TYPE_AIRPLANE && !pObj->IsFlight())
	{
		pbrain	= (DigitalBrain*) ((AircraftClass*)pObj)->Brain();
		mode		= pbrain->GetCurrentMode();

		if(((SimMoverClass*) pObj)->targetPtr)
		{
			pobjTgt	= (SimMoverClass*) ((SimMoverClass*) pObj)->targetPtr->BaseData();
		}
	} // Othwerwise 
	else
	{
		pbrain	= NULL;
		mode		= DigitalBrain::NoMode;
	}

	// Get the object's side
	objSide		= pObj->GetTeam();

	// Get the pointer to the player
	pplayer		= SimDriver.playerEntity;

	// Get the player's side
	playerSide	= pplayer->GetTeam();

	// Get the player's radar
	pradar		= (RadarClass*) FindSensor(pplayer, SensorClass::Radar);

	// Get the player radar's locked target
	if (pradar)
		pplayerLockedTgt	= (SimObjectType*) pradar->CurrentTarget();

	// If missile guiding upon the player
	if(objSide != playerSide && objtype == TYPE_MISSILE) {// && pobjTgt == pplayer) {
		priority = 0;
	} // If object is locked by player
	else if(objSide == playerSide && objtype == TYPE_MISSILE) {// && pobjTgt == pplayer) {
		priority = 12;
	} // If object is locked by player
	else if(pplayerLockedTgt && pplayerLockedTgt->BaseData() == pObj) {
		priority = 1;
	} // If object is attacking player
	else if(pobjTgt == pplayer && pbrain && (mode == DigitalBrain::GunsEngageMode || mode == DigitalBrain::MissileEngageMode || mode == DigitalBrain::WVREngageMode || mode == DigitalBrain::BVREngageMode)) {
		priority = 2;
	} // If object is enemy and being painted by player
	else if(objSide != playerSide && isPainted) {
		priority = 3;
	} // If object is enemy and fighter
	else if(objSide != playerSide && objtype == TYPE_AIRPLANE && objstype == STYPE_AIR_FIGHTER ) {
		priority = 4;	
	} // If object is enemy and bomber
	else if(objSide != playerSide && objtype == TYPE_AIRPLANE && (objstype == STYPE_AIR_BOMBER || objstype == STYPE_AIR_FIGHTER_BOMBER)) {
		priority = 5;
	} // If object is enemy and any other kind of aircraft
	else if(objSide != playerSide && objtype == TYPE_AIRPLANE ) {
		priority = 6;
	} // If object is friendly and painted by player
	else if(objSide == playerSide && isPainted) {
		priority = 7;
	} // If object is in player's flight
//	else if(pflight->  pObj is in player's flight) {
//		priority = 8;
//	} // If object is friendly and fighter
	else if(objSide == playerSide && objtype == TYPE_AIRPLANE && objstype == STYPE_AIR_FIGHTER) {
		priority = 9;
	} // If object is friendly and bomber
	else if(objSide == playerSide && objtype == TYPE_AIRPLANE && (objstype == STYPE_AIR_BOMBER || objstype == STYPE_AIR_FIGHTER_BOMBER)) {
		priority = 10;
	} // If object is friendly and any other kind of aircraft
	else if(objSide != playerSide && objtype == TYPE_AIRPLANE ) {
		priority = 11;
	} // Everything else
	else {
		priority = 12;
	}

	return priority;
}

//**//



// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_RankNAVPriority()
// 
//	Arguments:
//		pObj		pointer to the object we wish to compare
//
//	Returns:
//		priority	padlock priority of the object
//
// This function searches thru all the different properties of the object and
// ranks the importance of the object.  Ranking is specified by section 3. of the
// document entitled "Falcon 4.0 Specification for Target Sorting and Tracking in
// Padlock Modes, filename: targeting_rev_A.doc".  Targets with higher importance
// will get lower rankings.  Just to make Vince's life more difficult, pObj can
// either be a vehicle or a feature (runway).
//
// ------------------------------------------------------------------------------

int OTWDriverClass::Padlock_RankNAVPriority(SimBaseClass* pObj, BOOL isPainted)
{
	int							priority;
	int							objSide;
	int							playerSide;
	char							objtype;
	char							objstype;
	char							objclass;
	Falcon4EntityClassType* pclassPtr;
	RadarClass*					pradar;
	SimObjectType*				pplayerLockedTgt = NULL;
	SimMoverClass*				pobjTgt = NULL;
	AircraftClass*				pplayer;
	DigitalBrain*				pbrain = NULL;
	DigitalBrain::DigiMode	mode;


	// Get the object's type pointer
	pclassPtr	= (Falcon4EntityClassType*) pObj->EntityType();

	// Get object's type
	objtype		= pclassPtr->vuClassData.classInfo_[VU_TYPE];

	// Get object's stype
	objstype		= pclassPtr->vuClassData.classInfo_[VU_STYPE];

	// Get the object's data type
	objclass		= pclassPtr->vuClassData.classInfo_[VU_CLASS];


	if(objclass == CLASS_FEATURE) {

		// If friendly or neutral runway
		if(objtype == TYPE_RUNWAY) {// && friendly or neutral) {
			priority = 0;
		} // Just a ground feature
		else {
			priority = 14;
		}	
	}
	else if(objclass == CLASS_VEHICLE) {

		// Get the object's target
		if(((SimMoverClass*) pObj)->targetPtr) {
			pobjTgt	= (SimMoverClass*) ((SimMoverClass*) pObj)->targetPtr->BaseData();
		}

		// Get the object's brain, if individual deaggregated aircraft
		if(objtype == TYPE_AIRPLANE && !pObj->IsFlight()) {
			pbrain	= (DigitalBrain*) ((AircraftClass*)pObj)->Brain();
			mode		= pbrain->GetCurrentMode();

			if(((SimMoverClass*) pObj)->targetPtr) {
				pobjTgt	= (SimMoverClass*) ((SimMoverClass*) pObj)->targetPtr->BaseData();
			}
		} // Othwerwise 
		else {
			pbrain	= NULL;
			mode		= DigitalBrain::NoMode;
		}

		// Get the object's side
		objSide		= ((SimMoverClass*) pObj)->GetTeam();

		// Get the pointer to the player
		pplayer		= SimDriver.playerEntity;

		// Get the player's side
		playerSide	= pplayer->GetTeam();

		// Get the player's radar
		pradar		= (RadarClass*) FindSensor(pplayer, SensorClass::Radar);

		// Get the player radar's locked target
		ShiAssert(pradar);
		if (pradar)
			pplayerLockedTgt	= (SimObjectType*) pradar->CurrentTarget();

		// If missile guiding upon the player
		if(objSide != playerSide && objtype == TYPE_MISSILE) { // && pobjTgt == pplayer) {
			priority = 1;
		} // If object is locked by player
		else if(objSide == playerSide && objtype == TYPE_MISSILE) {// && pobjTgt == pplayer) {
			priority = 15;
		} // If object is locked by player
		else if(pplayerLockedTgt && pplayerLockedTgt->BaseData() == pObj) {
			priority = 2;
		} // If object is attacking player
		else if(pobjTgt == pplayer && pbrain && (mode == DigitalBrain::GunsEngageMode || mode == DigitalBrain::MissileEngageMode || mode == DigitalBrain::WVREngageMode || mode == DigitalBrain::BVREngageMode)) {
			priority = 3;
		} // If object is enemy and being painted by player
		else if(objSide != playerSide && isPainted) {
			priority = 4;
		} // If object is enemy and fighter
		else if(objSide != playerSide && objtype == TYPE_AIRPLANE && objstype == STYPE_AIR_FIGHTER ) {
			priority = 5;	
		} // If object is enemy and bomber
		else if(objSide != playerSide && objtype == TYPE_AIRPLANE && (objstype == STYPE_AIR_BOMBER || objstype == STYPE_AIR_FIGHTER_BOMBER)) {
			priority = 6;
		} // If object is enemy and any other kind of aircraft
		else if(objSide != playerSide && objtype == TYPE_AIRPLANE ) {
			priority = 7;
		} // If object is friendly and painted by player
		else if(objSide == playerSide && isPainted) {
			priority = 8;
		} // If object is in player's flight
	//	else if(pflight->  pObj is in player's flight) {
	//		priority = 9;
	//	} // If object is friendly and fighter
		else if(objSide == playerSide && objtype == TYPE_AIRPLANE && objstype == STYPE_AIR_FIGHTER) {
			priority = 10;
		} // If object is friendly and bomber
		else if(objSide == playerSide && objtype == TYPE_AIRPLANE && (objstype == STYPE_AIR_BOMBER || objstype == STYPE_AIR_FIGHTER_BOMBER)) {
			priority = 11;
		} // If object is friendly and any other kind of aircraft
		else if(objSide != playerSide && objtype == TYPE_AIRPLANE ) {
			priority = 12;
		} // Everything else
		else {
			priority = 13;
		}
	}
	else {
		priority = 15;
	}

	return priority;
}

//**//



// --------------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_CheckPadlock()
//
// Arguements:
//		dT		delta time, frame time (in seconds)
//
//	Returns:
//		None
//
// This function checks elapsed time since the candidate object has been selected.
// To use this function, set mPadlockTimeout at some point.  Calling this function
// will check the timeout and decrement it.
//
// --------------------------------------------------------------------------------------

void OTWDriverClass::Padlock_CheckPadlock(float dT)
{


	if(!mpPadlockPriorityObject && (snapStatus == PRESNAP || snapStatus == TRACKING)) {
		PadlockOccludedTime	= 0.0F;
		snapStatus				= SNAPPING;
		mPadlockTimeout		= 0.0F;
		mTDTimeout				= 0.0F;
	}
	else if(mPadlockTimeout <= 0.0F && mpPadlockCandidate) {		// If we have timed out and If there is a padlock candidate waiting in the wings,
		// Set the candidate to be the padlock priority
/* 2001-01-29 MODIFIED BY S.G. FOR THE NEW mpPadlockPrioritySimObject
		// Set the candidate to be the padlock priority
		VuDeReferenceEntity(mpPadlockPriorityObject);
		mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
		VuReferenceEntity(mpPadlockPriorityObject);
*/		
		SetmpPadlockPriorityObject(mpPadlockCandidate);

		//VWF 2/15/99
		VuDeReferenceEntity(mpPadlockCandidate);

		// Release the candidate
		mpPadlockCandidate	= NULL;
      mPadlockCandidateID  = FalconNullId;

		// Begin slewing
		snapStatus				= PRESNAP;
		mPrevPRate				= 0.5F;
		mPrevPError				= 0.5F;
		mPrevTRate				= 0.5F;
		mPrevTError				= 0.5F;

		PadlockOccludedTime	= 0.0F;

		// Zero out the timer
		mPadlockTimeout		= 0.0F;
		mTDTimeout				= 5.0F;
	}
	else if(mpPadlockPriorityObject && 
		((mpPadlockPriorityObject->IsDead() && mPadlockTimeout < 0.0F) || 
		(FalconLocalGame && FalconLocalGame->GetGameType() == game_Dogfight && mpPadlockPriorityObject->IsDead() && mpPadlockPriorityObject->IsSetFalcFlag(FEC_REGENERATING)))) {
/* 2001-01-29 MODIFIED BY S.G. FOR THE NEW mpPadlockPrioritySimObject
		VuDeReferenceEntity(mpPadlockPriorityObject);
		mpPadlockPriorityObject = NULL;
*/		
		SetmpPadlockPriorityObject(NULL);
		// snap to center
		PadlockOccludedTime	= 0.0F;
		snapStatus				= SNAPPING;
		mPadlockTimeout		= 0.0F;
		mTDTimeout				= 0.0F;
	}
	else if(mpPadlockPriorityObject && mpPadlockPriorityObject->IsDead() && mPadlockTimeout == 0.0F && snapStatus != SNAPPING) {
		mPadlockTimeout		= 5.0F;
	}
	else if(mPadlockTimeout > 0.0F){ // Otherwise, decrement
		mPadlockTimeout		-= dT;
	}

	if(mTDTimeout > 0.0F) {
		mTDTimeout				-= dT;
	}
}

//**//

// ------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_CheckOcclusion()
// 
// Arguments:
//		az		azimuth relative to the player
//		el		elevation relative to the player
//	
//	Returns:
//		isOccluded	True if az and el fall into the occlusion zone, False otherwise
//		
// This nifty little function checks if an object's relative (relative to the
// player's aircraft) azimuth and elevation falls within the occluded zone.  This
// is the zone where the virtual cockpit occludes the object.
//
// ------------------------------------------------------------------------------

BOOL OTWDriverClass::Padlock_CheckOcclusion(float az, float el) 
{
	BOOL isOccluded;

	if(az > -15.0F * DTR && az < 15.0F * DTR && el > 20.0F * DTR) {

		// If I'm looking forward and down, i.e. +/- 15 degrees to either side and
		// more than 20 degrees down.  This is the front of the cockpit and the object is
		// occluded by the front instrument panel.

		isOccluded = TRUE;
	} 
//	else if((az > 150.0F  * DTR && az < 170 * DTR || az > -170 * DTR && az < -150.0F  * DTR) && el > -5.0F * DTR) {
	else if((az > 150.0F  * DTR || az < -150.0F  * DTR) && el > -5.0F * DTR) {
		
		// If I'm looking back and down.  i.e. more than +/- 150 degrees behind and 5 degrees down.
		// Back of the seat occludes the object

		isOccluded = TRUE;
	}
	else if (el > 35.0F * DTR) {
		
		// If I'm looking more than 35 degrees down on the sides.  Then the object is somewhere
		// hidden beheath the cockpit.

		isOccluded = TRUE;
	}

// M.N. - looking behind and up ? This guy has no neck ;) 
// make a no vision conus behind the pilot
// removed, doesn't seem to work well
/*	else if ((az >= 100 * DTR || az <= -100 * DTR) && fabs(el) > (180 * DTR -fabs(az))) {

		// At 180°, maximum is 100°, at 175° maximum is 50°.....
		isOccluded = TRUE;
	}*/
	else {
		
		// Otherwise the object is not occluded.

		isOccluded = FALSE;
	}

	return isOccluded;
}

//**//

// ----------------------------------------------------------------------------------------------------
//
//	OTWDriverClass::Padlock_DrawSquares()
// 
// Arguements:
//		highlightPriority	True = always place TD boxes around the padlock priority object (F3 padlocking)
//								False = place TD box around priority object when it is on screen, don't draw TD box
//											when off screen (EFOV padlocking)
//	Returns:
//		None
//
//	Draw TD boxes around the padlocked object and the current padlock candidate
//
// ----------------------------------------------------------------------------------------------------

void OTWDriverClass::Padlock_DrawSquares(BOOL highlightPriority)
{
	ThreeDVertex	objectPoint;
	ThreeDVertex	candidatePoint;

	Tpoint			candidateLoc;
	Tpoint			priorityLoc;

	SimBaseClass*	pCandidate;
	float objtop, objbottom;
	float objleft, objright;
	float candtop, candbottom;
	float candleft, candright;
	float xDiff, yDiff, zDiff, objRange;

	
	if(padlockGlance != GlanceNose && padlockGlance != GlanceTail) {

		if(mpPadlockPriorityObject && highlightPriority == TRUE && (PlayerOptions.GetPadlockMode() == PDEnhanced || mTDTimeout > 0.0F)) {

// 2001-01-26 ADDED BY S.G. SIMULATES HMS FOR THE PLAYER THROUGH THE PADLOCK VIEW
			// Only for vehicle eqipped with HMS...
			VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[otwPlatform->Type() - VU_LAST_ENTITY_TYPE].dataPtr;
			if (vc && vc->Flags & 0x20000000) {
				MissileClass* theMissile;
				theMissile = (MissileClass*)(SimDriver.playerEntity->Sms->curWeapon);

				// First, make sure we have a Aim9 in uncage mode selected...
				if (SimDriver.playerEntity->Sms->curWeaponType == wtAim9) {
					if (theMissile && theMissile->isCaged == 0) {
						// Now check if the current target (if any) is not the padlocked sim target
						if (!theMissile->targetPtr || theMissile->targetPtr != (SimObjectType *)mfdVirtualDisplay) {
							// Use the sim object we reference in our SetmpPadlockPriorityObject as a target
							
							// JB 010712 Set the seeker to look where the target is.
							if (theMissile->sensorArray[0] && mfdVirtualDisplay)
								theMissile->sensorArray[0]->SetSeekerPos(((SimObjectType *)mfdVirtualDisplay)->localData->az,
									((SimObjectType *)mfdVirtualDisplay)->localData->el);

							theMissile->SetTarget((SimObjectType *)mfdVirtualDisplay);
						}

						// We need to update the localData table if not in flight (ie: target not passed to the missile)
						if (theMissile->launchState != MissileClass::InFlight && theMissile->targetPtr)
							CalcRelGeom (otwPlatform, theMissile->targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
						//  Run its seeker to check if it can see it...
						theMissile->RunSeeker();
					}
				}
			}
// END OF ADDED SECTION

			priorityLoc.x = mpPadlockPriorityObject->XPos();
			priorityLoc.y = mpPadlockPriorityObject->YPos();
			priorityLoc.z = mpPadlockPriorityObject->ZPos();

// 2002-04-06 MN Done below now
/*			xDiff				= priorityLoc.x - SimDriver.playerEntity->XPos();
			yDiff				= priorityLoc.y - SimDriver.playerEntity->YPos();
			zDiff				= priorityLoc.z - SimDriver.playerEntity->ZPos();

			objRange			= (float)sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);

			objRange = max(objRange,1.0F);

			float maxrange = g_fPadlockBreakDistance * NM_TO_FT;

					// break lock
			if (objRange > maxrange)
			{ 
				SetmpPadlockPriorityObject(NULL);
				highlightPriority		= false;
				mpPadlockCandidate		= NULL;
				mPadlockCandidateID		= FalconNullId;
				return;
			}

			// also break padlock when looking into the Sun

			if (TheTimeOfDay.ThereIsASun())
			{
				float strength, scale;
				Tpoint	sunRay;
						
				TheTimeOfDay.GetLightDirection( &sunRay );

				// Normalize the vector to target
				scale = 1.0f / objRange;
				xDiff *= scale;
				yDiff *= scale;
				zDiff *= scale;

				// Now do the dot product to get the cos of the angle between the rays
				strength = xDiff*sunRay.x + yDiff*sunRay.y + zDiff*sunRay.z;

				// Break lock 
				if (((strength - COS_SUN_EFFECT_HALF_ANGLE) / (1.0f - COS_SUN_EFFECT_HALF_ANGLE))
					> 0.75f)
				{
					SetmpPadlockPriorityObject(NULL);
					highlightPriority		= false;
					mpPadlockCandidate		= NULL;
					mPadlockCandidateID		= FalconNullId;
					return;
				}

			}
*/		
			renderer->TransformPoint(&priorityLoc, &objectPoint);

// OW Padlock box size fix
#if 0
			objleft		= objectPoint.x - 2;
			objright		= objectPoint.x + 2;
			objtop		= objectPoint.y - 2;
			objbottom	= objectPoint.y + 2;
#else
			objleft		= objectPoint.x - g_nPadlockBoxSize;
			objright		= objectPoint.x + g_nPadlockBoxSize;
			objtop		= objectPoint.y - g_nPadlockBoxSize;
			objbottom	= objectPoint.y + g_nPadlockBoxSize;
#endif

			if(objtop    > renderer->GetTopPixel()    &&
				objbottom < renderer->GetBottomPixel() &&
				objleft   > renderer->GetLeftPixel()   &&
				objright  < renderer->GetRightPixel()  &&
				mpPadlockPriorityObject->drawPointer)  // JB 001202
			{
// 2000-11-24 ADDED BY S.G. IF ASKED, THE COLOR OF THE BOX IS THE COLOR OF THE NEAR LABEL AND THE INTENSITY WILL VARY AS WELL
				if (g_nPadlockMode & PLockModeNearLabelColor) {
					// Joel, I hope you don't mind I reuse your label code? :-)
					long limit = g_nNearLabelLimit * 6076 + 8;
					if (objectPoint.csZ < limit)
					{
						int labelColor = 0;
						
						if (mpPadlockPriorityObject && !F4IsBadReadPtr(mpPadlockPriorityObject, sizeof(SimBaseClass)) && mpPadlockPriorityObject->drawPointer && !F4IsBadReadPtr(mpPadlockPriorityObject->drawPointer, sizeof(DrawableBSP))) // JB 010319 CTD
							labelColor = ((DrawableBSP *)mpPadlockPriorityObject->drawPointer)->LabelColor();
						
						int colorsub = int((objectPoint.csZ / (limit >> 3))) << 5;

						int red = (labelColor & 0x000000ff);
						red -= min(red, colorsub);
						int green = (labelColor & 0x0000ff00) >> 8;
						green -= min(green, colorsub);
						int blue = (labelColor & 0x00ff0000) >> 16;
						blue -= min(blue, colorsub);

						long newlabelColor = blue << 16 | green << 8 | red;

						renderer->SetColor( newlabelColor );
					}
					else
						renderer->SetColor (0xffc0c0c0);	// gray
				}
				else
// END OF ADDED SECTION
					renderer->SetColor (0xff0000ff);

				// Draw a box around the target
				//MI 18/01/02
				if(!g_bNoPadlockBoxes)
				{
				renderer->Render2DLine(objleft, objtop, objright, objtop);
				renderer->Render2DLine(objleft, objbottom, objright, objbottom);
				renderer->Render2DLine(objleft, objbottom, objleft, objtop);
				renderer->Render2DLine(objright, objbottom, objright, objtop);
				}
			}
		}
// 2001-01-28 ADDED BY S.G. SO WHEN WE DON'T HAVE A PADLOCKED OBJECT, THE MISSILE LOCK IS BROKEN...
		else {
			MissileClass* theMissile;
			theMissile = (MissileClass*)(SimDriver.playerEntity->Sms->curWeapon);

			// First, make sure we have a Aim9 in uncage mode selected...
			if (SimDriver.playerEntity->Sms->curWeaponType == wtAim9) {
				if (theMissile && theMissile->isCaged == 0) {
					// Then if it has a target, drop it
					if (theMissile->targetPtr)
						theMissile->SetTarget(NULL);
				}
			}
		}
// END OF ADDED SECTION

		if(mpPadlockCandidate && mpPadlockCandidate->GetCampaignObject() != ((CampBaseClass*)0xdddddddd) && !mpPadlockCandidate->IsDead()) {
			pCandidate = mpPadlockCandidate;
			candidateLoc.x = pCandidate->XPos();
			candidateLoc.y = pCandidate->YPos();
			candidateLoc.z = pCandidate->ZPos();
		
			renderer->TransformPoint(&candidateLoc, &candidatePoint);

			candleft		= candidatePoint.x - 4;
			candright	= candidatePoint.x + 4;
			candtop		= candidatePoint.y - 4;
			candbottom	= candidatePoint.y + 4;

			if(candtop		> renderer->GetTopPixel()    &&
				candbottom < renderer->GetBottomPixel() &&
				candleft		> renderer->GetLeftPixel()   &&
				candright	< renderer->GetRightPixel())
			{
				renderer->SetColor (0xff27eaff);

				//MI 18/01/02
				if(!g_bNoPadlockBoxes)
				{
				renderer->Render2DLine(candidatePoint.x, candtop, candright, candidatePoint.y);
				renderer->Render2DLine(candright, candidatePoint.y, candidatePoint.x, candbottom);
				renderer->Render2DLine(candidatePoint.x, candbottom, candleft, candidatePoint.y);
				renderer->Render2DLine(candleft, candidatePoint.y, candidatePoint.x, candtop);
				} 

	#if 0
				renderer->Render2DLine(candleft, candtop, candright, candtop);
				renderer->Render2DLine(candleft, candbottom, candright, candbottom);
				renderer->Render2DLine(candleft, candbottom, candleft, candtop);
				renderer->Render2DLine(candright, candbottom, candright, candtop);
	#endif
			}
		}
	}

// 2002-04-06 MN moved here - in realistic we won't go into the drawing code of the box above when
// mTDTimeout has been reached - thus we won't get these checks done anymore in realistic mode.
	if(mpPadlockPriorityObject && highlightPriority == TRUE)
	{
		priorityLoc.x = mpPadlockPriorityObject->XPos();
		priorityLoc.y = mpPadlockPriorityObject->YPos();
		priorityLoc.z = mpPadlockPriorityObject->ZPos();

		xDiff				= priorityLoc.x - SimDriver.playerEntity->XPos();
		yDiff				= priorityLoc.y - SimDriver.playerEntity->YPos();
		zDiff				= priorityLoc.z - SimDriver.playerEntity->ZPos();

		objRange			= (float)sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);

		objRange = max(objRange,1.0F);

		float maxrange = g_fPadlockBreakDistance * NM_TO_FT;

		// break lock
		if (objRange > maxrange)
		{ 
			SetmpPadlockPriorityObject(NULL);
			highlightPriority		= false;
			mpPadlockCandidate		= NULL;
			mPadlockCandidateID		= FalconNullId;
			return;
		}

		// also break padlock when looking into the Sun

		if (TheTimeOfDay.ThereIsASun())
		{
			float strength, scale;
			Tpoint	sunRay;
					
			TheTimeOfDay.GetLightDirection( &sunRay );
			// Normalize the vector to target
			scale = 1.0f / objRange;
			xDiff *= scale;
			yDiff *= scale;
			zDiff *= scale;

			// Now do the dot product to get the cos of the angle between the rays
			strength = xDiff*sunRay.x + yDiff*sunRay.y + zDiff*sunRay.z;

			// Break lock 
			if (((strength - COS_SUN_EFFECT_HALF_ANGLE) / (1.0f - COS_SUN_EFFECT_HALF_ANGLE))
				> 0.75f)
			{
				if (!sunlooktimer)
					sunlooktimer = SimLibElapsedTime;
				if (sunlooktimer + (VU_TIME)(g_fSunPadlockTimeout * CampaignSeconds) < SimLibElapsedTime)
				{
					SetmpPadlockPriorityObject(NULL);
					highlightPriority		= false;
					mpPadlockCandidate		= NULL;
					mPadlockCandidateID		= FalconNullId;
					return;
				}
			}
			else
				sunlooktimer = 0;

		}
	}


}

// 2001-01-29 ADDED BY S.G. SO SETTING mpPadlockPriorityObject ALSO SETS THE NEW mpPadlockPrioritySimObject

void OTWDriverClass::SetmpPadlockPriorityObject(SimBaseClass* newObject)
{

	// Only do if the object is different than the original one
	if (mpPadlockPriorityObject != newObject) {
		// If it already contain something, dereference it first
		if (mpPadlockPriorityObject)
			VuDeReferenceEntity(mpPadlockPriorityObject);

		// Set it to the new object (even if NULL)
		mpPadlockPriorityObject = (SimBaseClass*) newObject;

		// Unless NULL, reference the new object
		if (mpPadlockPriorityObject) {
			VuReferenceEntity(mpPadlockPriorityObject);

			// 2002-02-25 ADDED BY S.G. Tell its campaign object it's been visually identified by the player
			CampBaseClass *campBase;

			if (otwPlatform) {
				if (mpPadlockPriorityObject->IsSim())
					campBase = mpPadlockPriorityObject->GetCampaignObject();
				else
					campBase = (CampBaseClass *)mpPadlockPriorityObject;

				if (campBase)
					campBase->SetSpotted(otwPlatform->GetTeam(), TheCampaign.CurrentTime, 1);
			}
			// END OF ADDED SECTION 2002-02-25

		}

		// *** The remaining of the function is for the new simObjectType ***

		// If we already have a sim object set...
		if (mfdVirtualDisplay) {
			// If the base of the sim object is equal to the new object, we're still fine
			if (((SimObjectType *)mfdVirtualDisplay)->BaseData() == mpPadlockPriorityObject)
				return;

			// Otherwise, dereference and null it first
			((SimObjectType *)mfdVirtualDisplay)->Release( SIM_OBJ_REF_ARGS );
			mfdVirtualDisplay = NULL;
			// 2001-08-06
			if (otwPlatform) // JB 011004 Possible CTD fix
			{
				VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[otwPlatform->Type() - VU_LAST_ENTITY_TYPE].dataPtr;
				if (vc && vc->Flags & 0x20000000) {
					MissileClass* theMissile;
					theMissile = (MissileClass*)(SimDriver.playerEntity->Sms->curWeapon);

					// First, make sure we have a Aim9 in uncage mode selected...
					if (SimDriver.playerEntity->Sms->curWeaponType == wtAim9) {
						if (theMissile && theMissile->isCaged == 0) {
							theMissile->DropTarget();
						}
					}
				}
			}
		}

		// If mpPadlockPriorityObject is not NULL, is Sim base, is not us and is not a weapon, we must create a sim object for it
		if (mpPadlockPriorityObject && mpPadlockPriorityObject->IsSim() && mpPadlockPriorityObject != otwPlatform && !mpPadlockPriorityObject->IsWeapon()) {
#ifdef DEBUG
			simObjectPtr = new SimObjectType (OBJ_TAG, NULL, mpPadlockPriorityObject);
#else
			simObjectPtr = new SimObjectType (newObject);
#endif

			// And assign it and reference it
			mfdVirtualDisplay = (Render2D*)simObjectPtr;
			((SimObjectType *)mfdVirtualDisplay)->Reference( SIM_OBJ_REF_ARGS );
		}

	}
}

// END OF ADDED FUNCTION
