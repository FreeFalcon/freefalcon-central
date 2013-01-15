#include "Graphics\Include\renderow.h"
#include "Graphics\Include\drawshdw.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\drawbrdg.h"
#include "Graphics\Include\drawrdbd.h"
#include "Graphics\Include\drawbldg.h"
#include "Graphics\Include\drawgrnd.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawplat.h"
#include "Graphics\Include\drawguys.h"
#include "Graphics\Include\objlist.h"
#include "Graphics\Include\RViewPnt.h"
#include "stdhdr.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "otwdrive.h"
#include "simbase.h"
#include "missile.h"
#include "camp2sim.h"
#include "simfeat.h"
#include "feature.h"
#include "vehicle.h"
#include "simdrive.h"
#include "PlayerOp.h"
#include "campbase.h"
#include "Ground.h"
#include "Dogfight.h"
#include "Team.h"
#include "Pilot.h"
#include "Flight.h"
#include "ui\include\TeamData.h"
#include "msginc\sendchatmessage.h"
#include "fsound.h"
#include "soundfx.h"

void CreateDrawable (SimBaseClass* theObject, float objectScale);
void SetLabel (SimBaseClass* theObject);

void OTWDriverClass::CreateVisualObject (SimBaseClass* theObject, float objectScale)
{
   ShiAssert( IsActive() );

   CreateDrawable (theObject, objectScale);
}

void OTWDriverClass::CreateVisualObject (SimBaseClass* theObject, int visType, Tpoint *simView, Trotation *viewRotation, float objectScale)
{
   ShiAssert( IsActive() );
   ShiAssert( simView );							// KCK: Change default parameters to avoid this
   ShiAssert( viewRotation );						// when Scott is done with his change

   theObject->drawPointer = new DrawableBSP(visType, simView, viewRotation, objectScale);
}

void OTWDriverClass::CreateVisualObject (SimBaseClass* theObject, int visType, float objectScale)
{
Tpoint    simView;
Trotation viewRotation;

   ShiAssert( IsActive() );
   ShiAssert( !theObject->drawPointer );

   // Set position and orientations
   viewRotation.M11 = theObject->dmx[0][0];
   viewRotation.M21 = theObject->dmx[0][1];
   viewRotation.M31 = theObject->dmx[0][2];

   viewRotation.M12 = theObject->dmx[1][0];
   viewRotation.M22 = theObject->dmx[1][1];
   viewRotation.M32 = theObject->dmx[1][2];

   viewRotation.M13 = theObject->dmx[2][0];
   viewRotation.M23 = theObject->dmx[2][1];
   viewRotation.M33 = theObject->dmx[2][2];

   // Update object position
   simView.x     = theObject->XPos();
   simView.y     = theObject->YPos();
   simView.z     = theObject->ZPos();
   theObject->drawPointer = new DrawableBSP(visType, &simView, &viewRotation, objectScale);
}

void OTWDriverClass::InsertObjectIntoDrawList (SimBaseClass* theObject)
{
   if (theObject->drawPointer && !theObject->drawPointer->InDisplayList())
   {
      // KCK: Let's try just going ahead and adding these to the list directly -
      // Assuming, of course that this is called by the Sim/Graphics thread
      viewPoint->InsertObject(theObject->drawPointer);
/*    No longer used
	  if (theObject->IsGroundVehicle() &&
         ((GroundClass*)theObject)->crewDrawable && 
		 !((GroundClass*)theObject)->crewDrawable->InDisplayList())
		 viewPoint->InsertObject(((GroundClass*)theObject)->crewDrawable);
*/
	  if (theObject->IsGroundVehicle() &&
         ((GroundClass*)theObject)->truckDrawable && 
		 !((GroundClass*)theObject)->truckDrawable->InDisplayList())
		 viewPoint->InsertObject(((GroundClass*)theObject)->truckDrawable);
/*
      InsertObject(theObject->drawPointer);
	  if (theObject->IsGroundVehicle() &&
         ((GroundClass*)theObject)->crewDrawable && 
		 !((GroundClass*)theObject)->crewDrawable->InDisplayList())
		 InsertObject(((GroundClass*)theObject)->crewDrawable);
	  if (theObject->IsGroundVehicle() &&
         ((GroundClass*)theObject)->truckDrawable && 
		 !((GroundClass*)theObject)->truckDrawable->InDisplayList())
		 InsertObject(((GroundClass*)theObject)->truckDrawable);
*/
   }
}

void OTWDriverClass::RescaleAllObjects(void)
{
SimBaseClass* theObject;
VuListIterator featureWalker (SimDriver.featureList);
VuListIterator otwDrawWalker (SimDriver.objectList);

   theObject = (SimBaseClass*)otwDrawWalker.GetFirst();
   while (theObject)
   {
	   if (theObject->drawPointer) {
			theObject->drawPointer->SetScale(objectScale);
	   }
      theObject = (SimBaseClass*)otwDrawWalker.GetNext();
   }

   theObject = (SimBaseClass*)featureWalker.GetFirst();
   while (theObject)
   {
	   if (theObject->drawPointer) {
			theObject->drawPointer->SetScale(objectScale);
	   }
      theObject = (SimBaseClass*)featureWalker.GetNext();
   }

/* To do this we'd need to access the Campaign's persistant list
   theObject = (SimBaseClass*)persistWalker.GetFirst();
   while (theObject)
   {
	   if (theObject->drawPointer) {
			theObject->drawPointer->SetScale(objectScale);
	   }
      theObject = (SimBaseClass*)persistWalker.GetNext();
   }
*/
}

// This is the function that actually creates the visual object, and does all of
// the linking necessary for airbases and bridges.
// NOTE: This can only be called AFTER DeviceDependentGraphicsSetup
void CreateDrawable (SimBaseClass* theObject, float objectScale)
{
short    visType = -1;
Tpoint    simView;
Trotation viewRotation;
Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)theObject->EntityType();
SimBaseClass* baseObject;
DrawableObject* lastPointer = NULL;

	// Set position and orientations
	viewRotation.M11 = theObject->dmx[0][0];
	viewRotation.M21 = theObject->dmx[0][1];
	viewRotation.M31 = theObject->dmx[0][2];

	viewRotation.M12 = theObject->dmx[1][0];
	viewRotation.M22 = theObject->dmx[1][1];
	viewRotation.M32 = theObject->dmx[1][2];

	viewRotation.M13 = theObject->dmx[2][0];
	viewRotation.M23 = theObject->dmx[2][1];
	viewRotation.M33 = theObject->dmx[2][2];

	// Update object position
	simView.x     = theObject->XPos();
	simView.y     = theObject->YPos();
	simView.z     = theObject->ZPos();

	visType = classPtr->visType[theObject->Status() & VIS_TYPE_MASK];

	if (visType >= 0 || theObject->drawPointer)
		{
		if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND ||
			classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA)
			{
			// This is a ground thingy.. 
			if (classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE)
			{
				if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_FOOT &&
				    classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_FOOT_SQUAD)
				{
					// Make the ground personel as desired
					simView.z = 0.0F;
					theObject->drawPointer = new DrawableGuys(visType, &simView, theObject->Yaw(), 1, objectScale);
				} 
				else
				{
					// Make the ground vehicle as desired
					theObject->drawPointer = new DrawableGroundVehicle(visType, &simView, theObject->Yaw(), objectScale);
				}
			}
			else if (classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_FEATURE)
				{
				// A feature thingy..
				SimBaseClass	*prevObj = NULL, *nextObj = NULL;

				// In many cases, our visType should be modified by our neighbors.
				if ((theObject->Status() & VIS_TYPE_MASK) != VIS_DESTROYED &&  (((SimFeatureClass*)theObject)->featureFlags & FEAT_NEXT_NORM || ((SimFeatureClass*)theObject)->featureFlags & FEAT_PREV_NORM))
					{
					// KCK: Can we just use our slot number? Or will this break something?
//					int					idx = theObject->GetCampaignObject()->GetComponentIndex (theObject);
					int					idx = theObject->GetSlot();

					prevObj = theObject->GetCampaignObject()->GetComponentEntity(idx - 1);
					nextObj = theObject->GetCampaignObject()->GetComponentEntity(idx + 1);
					if (prevObj && ((SimFeatureClass*)theObject)->featureFlags & FEAT_PREV_NORM && (prevObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED)
						{
						if (nextObj && ((SimFeatureClass*)theObject)->featureFlags & FEAT_NEXT_NORM && (nextObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED)
							visType = classPtr->visType[VIS_BOTH_DEST];
						else
							visType = classPtr->visType[VIS_LEFT_DEST];
						}
					else if (nextObj && ((SimFeatureClass*)theObject)->featureFlags & FEAT_NEXT_NORM && (nextObj->Status() & VIS_TYPE_MASK) == VIS_DESTROYED)
						visType = classPtr->visType[VIS_RIGHT_DEST];
					}

				// Check for change - and don't bother if there is none.
				if (theObject->drawPointer && ((DrawableBSP*)theObject->drawPointer)->GetID() == visType)
					return;

				// edg: arghhh.  As far as I can tell, calls were being made
				// to this function on vistype changes and the old drawable
				// was never getting removed from memory or the display list.
				// at this point in the function we know that a new drawable
				// is about to be created, so check for an old one and
				// remove it if necessary.  This is actually a bad thing to
				// do here since ACMI uses this function as well -- ie there
				// should be no calls to OTWDrive list functions.  oh well for
				// now since there are other calls in here (I thought kevin
				// was supposed to fix this!?).
				if (theObject->drawPointer && theObject->drawPointer->InDisplayList() )
				{
					// KCK: In some cases we still need this pointer (specifically
					// when we replace bridge segments), so let's save it here - we'll
					// toss it out after we're done.
					lastPointer = theObject->drawPointer;
					theObject->drawPointer = NULL;
				}

				// note: ACMI objects DON'T have a campaignObject!
				if ( theObject->GetCampaignObject() )
					baseObject = theObject->GetCampaignObject()->GetComponentLead();
				else
					baseObject = NULL;

				// Some things require Base Objects (like bridges and airbases)
				if (baseObject && !((SimFeatureClass*)baseObject)->baseObject)
					{
					// Is this a bridge?
					if (baseObject->IsSetCampaignFlag(FEAT_ELEV_CONTAINER))
						{
						// baseObject is the "container" object for all parts of the bridge
						// There is only one container for the entire bridge, stored in the lead element
						((SimFeatureClass*)baseObject)->baseObject = new DrawableBridge (1.0F);

						// Insert only the bridge drawable.
						OTWDriver.InsertObject (((SimFeatureClass*)baseObject)->baseObject);
						}
					// Is this a big flat thing with things on it (like an airbase?)
					else if (baseObject->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
						{
						// baseObject is the "container" object for all parts of the platform
						// There is only one container for the entire platform, stored in the 
						// lead element.
						((SimFeatureClass*)baseObject)->baseObject = new DrawablePlatform (1.0F);

						// Insert only the platform drawable.
						OTWDriver.InsertObject (((SimFeatureClass*)baseObject)->baseObject);
						}
					}
				// Add another building to this grouping of buildings, or replace the drawable
				// of one which is here.
				// Is the container a bridge?
				if (baseObject && baseObject->IsSetCampaignFlag(FEAT_ELEV_CONTAINER))
					{
					// Make the new BRIDGE object
					if (visType)
						{
						if (theObject->IsSetCampaignFlag(FEAT_NEXT_IS_TOP) && theObject->Status() != VIS_DESTROYED)
							theObject->drawPointer = new DrawableRoadbed(visType, visType+1, &simView, theObject->Yaw(), 10.0f, (float)atan(20.0f/280.0f) );
						else
							theObject->drawPointer = new DrawableRoadbed(visType, -1, &simView, theObject->Yaw(), 10.0f, (float)atan(20.0f/280.0f) );
						}
					else
						theObject->drawPointer = NULL;
					// Check for replacement
					if (lastPointer)
						{
						ShiAssert(lastPointer->GetClass() == DrawableObject::Roadbed);
						ShiAssert(theObject->drawPointer->GetClass() == DrawableObject::Roadbed);
						((DrawableBridge*)(((SimFeatureClass*)baseObject)->baseObject))->ReplacePiece ((DrawableRoadbed*)(lastPointer), (DrawableRoadbed*)(theObject->drawPointer));
						}
					else if (theObject->drawPointer)
						{
						ShiAssert(theObject->drawPointer->GetClass() == DrawableObject::Roadbed);
						((DrawableBridge*)(((SimFeatureClass*)baseObject)->baseObject))->AddSegment((DrawableRoadbed*)(theObject->drawPointer));
						}
					}
				// Is the container a big flat thing (airbase)?
				else if (baseObject && baseObject->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
					{
					// Everything on a platform is a Building
					// That means it sticks straight up the -Z axis
					theObject->drawPointer = new DrawableBuilding(visType, &simView, theObject->Yaw(), 1.0F);

					// Am I Flat (can things drive across it)?
					if (((SimFeatureClass*)theObject)->displayPriority <= PlayerOptions.BuildingDeaggLevel())
						{
						if (theObject->IsSetCampaignFlag((FEAT_FLAT_CONTAINER | FEAT_ELEV_CONTAINER)))
							((DrawablePlatform*)((SimFeatureClass*)baseObject)->baseObject)->InsertStaticSurface (((DrawableBuilding*)theObject->drawPointer));
						else
							((DrawablePlatform*)((SimFeatureClass*)baseObject)->baseObject)->InsertStaticObject (theObject->drawPointer);
						}
					}
				else 
					{
					// if we get here then this is just a loose collection of buildings, like a
					// village or city, with no big flat objects between them
					theObject->drawPointer = new DrawableBuilding(visType, &simView, theObject->Yaw(), 1.0F);

        			// Insert the object, if we need to.
 					if (((SimFeatureClass*)theObject)->displayPriority <= PlayerOptions.BuildingDeaggLevel())
 	        				OTWDriver.InsertObject (((SimFeatureClass*)theObject)->drawPointer);
					}
				// KCK: Remove any previous drawable object
				if (lastPointer)
					OTWDriver.RemoveObject( lastPointer, TRUE );
				}
			}
		else
			{
			// This object isn't DOMAIN_LAND
			if (classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_SFX)
				{
				// Hmm.. Special effect. Better leave this one alone..
//				if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_EJECT)
//					((EjectedPilotClass*)theObject)->SetModel()
				return;
				}
			else
// TODO:  Might want to remove shadows from missiles/bombs for performance reasons...
//				if (classPtr->vuClassData.classInfo_[VU_TYPE] != TYPE_MISSILE)
				{
   				// We may still be on the ground
					theObject->drawPointer = new DrawableShadowed(visType, &simView, &viewRotation, objectScale,
                  classPtr->visType[1]);
				}
			}

		SetLabel(theObject);
	}
}

long TeamSimColorList[NUM_TEAMS] = {	0xfffffffe,		// Thunderbird		// White  not quite white so color is steady
										0xff008000,		// US				// Green
										0xffff0000,		// ROK/Shark		// Blue
										0xff3771B2,		// Japan			// Brown
										0xff00ffff,		// China			// Yellow
										0xff00adff,		// Russia/Tiger		// Orange
										0xff0000ff,		// DPRK/Crimson		// Red
										0xff000000 };	// No one			// Black

void SetLabel (SimBaseClass* theObject)
{
	Falcon4EntityClassType	*classPtr = (Falcon4EntityClassType*)theObject->EntityType();
	CampEntity				campObj;
	char					label[40] = {0};
	long					labelColor = 0xff0000ff;

	ShiAssert (theObject);	// try to catch when this happens (2nd crash BT #955)
	if (!theObject)
		return;
	if (!theObject->IsExploding())
	{
		if (classPtr->dataType == DTYPE_VEHICLE)
		{
			FlightClass		*flight;
			flight = FalconLocalSession->GetPlayerFlight();
			campObj = theObject->GetCampaignObject();
			if (campObj && campObj->IsFlight() && !campObj->IsAggregate() /*&& campObj->InPackage()*/
				// 2001-10-31 M.N. show flight names of our team
				&& flight && flight->GetTeam() == campObj->GetTeam())
			{
				char		temp[40];
				GetCallsign(((Flight)campObj)->callsign_id,((Flight)campObj)->callsign_num,temp);
				sprintf (label, "%s%d",temp,((SimVehicleClass*)theObject)->vehicleInUnit+1);
			}
			else
				sprintf (label, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
		}
		else if (classPtr->dataType == DTYPE_WEAPON)
			sprintf (label, "%s", ((WeaponClassDataType*)(classPtr->dataPtr))->Name);
	}

	// Change the label to a player, if there is one
	if (theObject->IsSetFalcFlag(FEC_HASPLAYERS))
	{
		// Find the player's callsign
		VuSessionsIterator		sessionWalker(FalconLocalGame);
		FalconSessionEntity		*session;

		session = (FalconSessionEntity*)sessionWalker.GetFirst();
		while (session)
		{
			if (session->GetPlayerEntity() == theObject)
				sprintf(label, "%s", session->GetPlayerCallsign());
			session = (FalconSessionEntity*)sessionWalker.GetNext();
		}
	}

	// Change the label color by team color
	//ShiAssert(TeamInfo[theObject->GetTeam()]);
	
	
	if(TeamInfo[theObject->GetTeam()])
		labelColor = TeamSimColorList[TeamInfo[theObject->GetTeam()]->GetColor()];
	
	// KCK: This uses the UI's colors. For a while these didn't work well in Sim
	// They may be ok now, though - KCK: As of 10/25, still looked bad
//	labelColor = TeamColorList[TeamInfo[theObject->GetTeam()]->GetColor()];

	if (theObject->drawPointer)
		theObject->drawPointer->SetLabel(label, labelColor);
	
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int make_callsign_string (char *str, char *insert, SimBaseClass *theObject)
{
	Falcon4EntityClassType
		*classPtr;

	char
		flight[40],
		callsign[40];

	CampEntity
		campObj;

	str[0] = 0;
	flight[0] = 0;
	callsign[0] = 0;
	classPtr = (Falcon4EntityClassType*)theObject->EntityType();

	if (classPtr->dataType == DTYPE_VEHICLE)
	{
		campObj = theObject->GetCampaignObject();
		if (campObj && campObj->IsFlight() && campObj->InPackage())
		{
			char		temp[40];

			GetCallsign(((Flight)campObj)->callsign_id,((Flight)campObj)->callsign_num,temp);
			sprintf (flight, "%s%d",temp,((SimVehicleClass*)theObject)->vehicleInUnit+1);
		}
		else
		{
			sprintf (flight, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
		}
	}

	if (theObject->IsSetFalcFlag(FEC_HASPLAYERS))
	{
		// Find the player's callsign
		VuSessionsIterator		sessionWalker(FalconLocalGame);
		FalconSessionEntity		*session;

		session = (FalconSessionEntity*)sessionWalker.GetFirst();
		while (session)
		{
			if (session->GetPlayerEntity() == theObject)
			{
				sprintf(callsign, "%s", session->GetPlayerCallsign());
			}
			session = (FalconSessionEntity*)sessionWalker.GetNext();
		}
	}

	if ((callsign[0]) && (flight[0]))
	{
		strcpy (str, callsign);
		strcat (str, insert);
		strcat (str, flight);

		return 1;
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class C_Handler;
extern C_Handler *gMainHandler;
extern void AddMessageToChatWindow(VU_ID from,_TCHAR *message);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

