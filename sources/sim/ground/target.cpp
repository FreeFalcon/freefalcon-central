#include "stdhdr.h"
#include "mesg.h"
#include "float.h"
#include "object.h"
#include "sensclas.h"
#include "fcc.h"
#include "sms.h"
#include "classtbl.h"
#include "hardpnt.h"
#include "guns.h"
#include "Find.h"
#include "vehicle.h"
#include "ground.h"


// Run our sensors -- only really used to keep radars working.
// Could we/should we ignore other senors?  Or just never put them on ground things?
// Or have the ground AI _really_ use sensors?
void GroundClass::RunSensors(){
	int i;
	for (i=0; i<numSensors; i++){
		sensorArray[i]->Exec(targetList);
	}
}


void GroundClass::SelectWeapon(int gun_only)
{
	Falcon4EntityClassType* classPtr;
	VehicleClassDataType*	vd;
	uchar *dam;
	MoveType mt;
	int range;

	// KCK: This section rewritten on 6/22.
	// I'm going to use campaign statisics in order to choose the 'best' weapon
	classPtr = (Falcon4EntityClassType*) targetPtr->BaseData()->EntityType();
	if (targetPtr->BaseData()->IsSim()){
		vd = (VehicleClassDataType*) classPtr->dataPtr;
		ShiAssert(vd);
		dam = vd->DamageMod;
		if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR){
			if (targetPtr->BaseData()->ZPos() - ZPos() > -10000.0F){
				mt = LowAir;
			}
			else {
				mt = Air;
			}
		}
		else if (
			!targetPtr->BaseData()->IsStatic() && 
			((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject()
		){
			UnitClassDataType	*ud;
      		classPtr = (Falcon4EntityClassType*) 
				((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject()->EntityType()
			;
			ud = (UnitClassDataType*) classPtr->dataPtr;
			mt = ud->MovementType;
		}
		else {
			mt = NoMove;		// Choose NoMove for a generic ground vehicle hitchance (probably not good)
		}
	}
	else if (targetPtr->BaseData()->IsUnit()){
		UnitClassDataType	*ud;
		ud = (UnitClassDataType*) classPtr->dataPtr;
		ShiAssert(ud);
		dam = ud->DamageMod;
		if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR){
			mt = LowAir;		// Choose low air for a generic vs air hitchance
		}
		else{
			mt = ud->MovementType;
		}
	}
	else if (targetPtr->BaseData()->IsObjective()){
		ObjClassDataType	*od;
		od = (ObjClassDataType*) classPtr->dataPtr;
		ShiAssert(od);
		dam = od->DamageMod;
		mt = NoMove;
	}
	else {
		return;
	}

	//range = FloatToInt32(
	//	Distance(XPos(),YPos(),targetPtr->BaseData()->XPos(),targetPtr->BaseData()->YPos()) * FT_TO_KM + 0.5F
	//);
	float dx, dy, dz;
	dx = XPos() - targetPtr->BaseData()->XPos();
	dy = YPos() - targetPtr->BaseData()->YPos();
	dz = ZPos() - targetPtr->BaseData()->ZPos();
	range = FloatToInt32((float)sqrt(dx*dx + dy*dy + dz*dz)*FT_TO_KM + 0.5F);
	// 2002-03-09 MODIDIED BY S.G. Passed dz to the function so weapon 
	// is chosen depending on its altitude capacity as well (dz is positive if we are below targetPtr)
	Sms->SelectBestWeapon (dam, mt, range, gun_only, (int)dz); 
}
