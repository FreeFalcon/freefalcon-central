#include "Graphics/Include/drawbldg.h"
#include "stdhdr.h"
#include "simfeat.h"
#include "initdata.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "atcbrain.h"
#include "simdrive.h"
#include "Objectiv.h"
#include "ptdata.h"
#include "entity.h"
#include "PlayerOp.h"
#include "Feature.h"
#include "sfx.h"
#include "Graphics/Include/RViewPnt.h"
#include "atcbrain.h"

/* 2001-03-06 S.G. FOR RADAR RANGE TO RADAR FEATURE */ 
#include "radarData.h"

#ifdef USE_SH_POOLS
MEM_POOL	SimFeatureClass::pool;
#endif

void CalcTransformMatrix (SimBaseClass* theObject);
int GetTextureIdxFromHeading (int hdg);

SimFeatureClass::SimFeatureClass (VU_BYTE** stream, long *rem) : SimStaticClass (stream, rem){
	InitLocalData();
}

SimFeatureClass::SimFeatureClass (FILE* filePtr) : SimStaticClass (filePtr){
	InitLocalData ();
}

SimFeatureClass::SimFeatureClass (int type) : SimStaticClass (type){
   InitLocalData();
}

SimFeatureClass::~SimFeatureClass(void){
	CleanupLocalData();
}

void SimFeatureClass::InitData(){
	SimStaticClass::InitData();
	InitLocalData();
}

void SimFeatureClass::InitLocalData (){
	Falcon4EntityClassType* classPtr;
	FeatureClassDataType *fc;

	classPtr = &Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE];
	fc = (FeatureClassDataType *)classPtr->dataPtr;

	strength = maxStrength = (float)fc->HitPoints;
	pctStrength = 1.0;
	dyingTimer = -1.0f;
	theBrain = NULL;
	baseObject = NULL;
	featureFlags = 0;
	// 2001-03-06 ADDED BY S.G. SO RADAR FEATURES HAVE A specialData.rdrNominalRange,,,
	if (fc->RadarType){
		specialData.rdrNominalRng = RadarDataTable[fc->RadarType].NominalRange;
	}
}

void SimFeatureClass::CleanupLocalData(){
	// sfr: @todo remove this
	if (baseObject){
		// WARNING:  This is unsafe.  RemoveObject should ONLY be called from the Sim thread
		// but the destructor can happen on any thread...
		OTWDriver.RemoveObject(baseObject, TRUE);
		baseObject = NULL;
	}
}

void SimFeatureClass::CleanupData(){
	CleanupLocalData();
	SimStaticClass::CleanupData();
}

void SimFeatureClass::Init(SimInitDataClass* initData)
{
Falcon4EntityClassType* classPtr;
FeatureClassDataType *fc;

   classPtr = (Falcon4EntityClassType *)EntityType();
   fc = (FeatureClassDataType *)classPtr->dataPtr;

   strength = maxStrength = (float)fc->HitPoints;
   pctStrength = 1.0;
   dyingTimer = -1.0f;
   sfxTimer = 0.0f;

   SetFlag(ON_GROUND);

   if (!initData)
      return;

   featureFlags = initData->specialFlags;

   SetDelta (0.0F, 0.0F, 0.0F);
   SetYPR (initData->heading, 0.0F, 0.0F);
   SetYPRDelta (0.0F, 0.0F, 0.0F);
   CalcTransformMatrix(this);

   SimStaticClass::Init (initData);
}

int SimFeatureClass::Wake()
{
	int texIdx = 0;
	int index;
	int rwyHeading;
	float yaw, z;
	int retval = 0;
	
	// KCK: Sets up this object to become sim aware
	if (IsAwake()){
		return retval;
	}

	SimStaticClass::Wake();
	
	// KCK: Set our Z position to an approximation of the ground level.
	// Hopefully this will be close enough.
	//RViewPoint* viewpoint = OTWDriver.GetViewpoint(); // JB 010616 safer
	//if (viewpoint)
	z = OTWDriver.GetApproxGroundLevel(XPos(), YPos());
	//else
	//	z = 0.0;
	SetPosition(XPos(), YPos(), z);
	
	if (drawPointer){
		DrawableBSP *bsp;
		int i, num;
		
		SimDriver.AddToFeatureList(this);
		bsp = (DrawableBSP *)drawPointer;
		// See if we need to add smoke to smoke stacks
		if (IsSetCampaignFlag(FEAT_HAS_SMOKE_STACK)){
			num = bsp->GetNumSlots();
			for ( i = 0; i < num; i++ ){
				// TODO:  Shouldn't this stay tied to the feature so it lives until reaggregation???
				SfxClass *sfx = new SfxClass (
					SFX_SMOKING_FEATURE,				// type
					i,									// slot #
					this,								// world pos
					2.0f,								// time to live
					40.0f 								// scale
				); 
				OTWDriver.AddSfxRequest(sfx);
			}
		}
		
		// Is this something that needs lights turned on at night/off during the day?
		if (IsSetCampaignFlag(FEAT_HAS_LIGHT_SWITCH)){
			OTWDriver.AddToLitList( bsp );
		}

		// Is This a runway number? 
		if(	EntityType()->classInfo_[VU_TYPE] == TYPE_RUNWAY && EntityType()->classInfo_[VU_STYPE] == STYPE_RUNWAY_NUM)
		{
			ShiAssert(GetCampaignObject());
			if ((Objective)GetCampaignObject()){
				ShiAssert(((Objective)GetCampaignObject())->brain);
				if (((Objective)GetCampaignObject())->brain)
				{
					index = ((Objective)GetCampaignObject())->GetComponentIndex(this);
					texIdx = ((Objective)GetCampaignObject())->brain->GetRunwayTexture(index);
					((DrawableBSP*)drawPointer)->SetTextureSet(texIdx);
				}
			}
		}
		
		// Is This a taxiway sign?
		if(	EntityType()->classInfo_[VU_TYPE] == TYPE_TAXIWAY &&
			(EntityType()->classInfo_[VU_STYPE] == STYPE_THP || EntityType()->classInfo_[VU_STYPE] == STYPE_THPX))
		{
			// NOTE: Runway pieces are defined upside down. a heading of 0 means runway 18
			yaw = Yaw() * RTD + 180.0F;
			if (yaw > 360.0F){
				yaw -= 360.0F;
			}
			
			rwyHeading = FloatToInt32((float)floor(yaw * 0.1F + 0.5F));
			
			texIdx = GetTextureIdxFromHeading (rwyHeading );
			
			((DrawableBSP*)drawPointer)->SetTextureSet(texIdx);
		}
	}
	return retval;
}

int SimFeatureClass::Sleep (void)
{
	if (!IsAwake()){
		return 0;
	}

	// Is this something we were managing in our time of day list?
	if (IsSetCampaignFlag(FEAT_HAS_LIGHT_SWITCH) && drawPointer){
		OTWDriver.RemoveFromLitList( (DrawableBSP *)drawPointer );
	}

	// If we're a bridge or platform with a "base" drawable, put that on the kill queue first (so it dies last)
	if (baseObject){
		OTWDriver.RemoveObject(baseObject, TRUE);
		baseObject = NULL;
	}

	SimBaseClass::Sleep();
	
	SimDriver.RemoveFromFeatureList(this);

	return 0;
}

int SimFeatureClass::GetRadarType(){
	FeatureClassDataType *fc;
	fc = (FeatureClassDataType *)Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE ].dataPtr;
	
	return fc->RadarType;
}

#if 0
/* sfr: not used
typedef struct {
   SimBaseClass* object;
   struct TmpObject* next;
} TmpObjectList;
*/
#endif

void SimFeatureClass::JoinFlight (void)
{
	/*
VuEntityType* classPtr;

   if (GetCampaignObject()->GetComponentLead() == this)
   {
      classPtr = GetCampaignObject()->EntityType();
      // Should I have a brain?
      if (classPtr->classInfo_[VU_TYPE] == TYPE_AIRBASE ||
          classPtr->classInfo_[VU_TYPE] == TYPE_AIRSTRIP)
      {
         theBrain = new ATCBrain (this);
         SimDriver.atcList->ForcedInsert(this);
      }
   }
   */
}

VU_ERRCODE SimFeatureClass::InsertionCallback(void)
{
/* KCK: Not sure if this code is still relevant - I highly doubt it
   if (!IsLocal())
   {
      // KCK: If this is an element of a campaign unit or objective, it was sent
      // to us during startup - I need to synthisize a deaggregation event.
      if (GetCampaignObject() > (VuEntity*)MAX_IA_CAMP_UNIT)  // KCK: Leon's hack - assumes 1-100 is dogfight slot #
	   {
	      // Synthisize a deaggregation (NOTE: not a Wake())
	      ((CampEntity)GetCampaignObject())->SetAggregate(0);
	      if (!((CampEntity)GetCampaignObject())->components)
		   {
		      DeaggregateList->ForcedInsert(GetCampaignObject());	
		      ((CampEntity)GetCampaignObject())->components = new TailInsertList();
		      ((CampEntity)GetCampaignObject())->components->Init();
		   }
	      // Add the element to the components list
	      ((CampEntity)GetCampaignObject())->components->ForcedInsert(this);
	   }
      else
	   {
	      // KCK: All other objects will get woken now if possible, or during sim startup.
	      if (OTWDriver.IsActive())
		      Wake();
	   }
   }
*/
   return SimStaticClass::InsertionCallback();
}

int GetTextureIdxFromHeading (int hdg)
{
	int texIdx = 0;
	switch (hdg){
		case 0:
			texIdx = 37;
		break;
		case 1:
			texIdx = 0;
		break;
		case 2:
			texIdx = 1;
		break;
		case 3:
		case 4:
			texIdx = 4;
		break;
		case 5:
		case 6:
			texIdx = 5;
		break;
		case 7:
		case 8:
			texIdx = 6;
		break;
		case 9:
			texIdx = 7;
		break;
		case 10:
		case 11:
			texIdx = 8;
		break;
		case 12:
			texIdx = 9;
		break;
		case 13:
		case 14:
			texIdx = 10;
		break;
		case 16:
			texIdx = 13;
		break;
		case 15:
		case 17:
			texIdx = 16;
		break;
		case 18:
			texIdx = 17;
		break;
		case 19:
			texIdx = 20;
		break;
		case 20:
			texIdx = 21;
		break;
		case 22:
		case 21:
			texIdx = 24;
		break;
		case 23:
		case 24:
			texIdx = 25;
		break;
		case 25:
		case 26:
			texIdx = 26;
		break;
		case 27:
			texIdx = 27;
		break;
		case 28:
		case 29:
			texIdx = 28;
		break;
		case 30:
			texIdx = 29;
		break;
		case 32:
			texIdx = 30;
		break;
		case 34:
			texIdx = 33;
		break;
		case 33:
		case 35:
			texIdx = 36;
		break;
		case 36:
			texIdx = 37;
		break;
}

return texIdx;
}
