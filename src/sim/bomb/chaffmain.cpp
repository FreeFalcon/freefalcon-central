#include "Graphics/Include/drawbsp.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "initdata.h"
#include "object.h"
#include "simdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "chaff.h"
#include "sfx.h" //I-Hawk 
#include "Graphics/Include/drawparticlesys.h" //I-Hawk

#ifdef USE_SH_POOLS
MEM_POOL	ChaffClass::pool;
#endif

ChaffClass::ChaffClass(VU_BYTE** stream, long *rem) : BombClass(stream, rem){
	InitLocalData();
}

ChaffClass::ChaffClass(FILE* filePtr) : BombClass(filePtr){
	InitLocalData();
}

ChaffClass::ChaffClass(int type) : BombClass(type){
	InitLocalData();
}

ChaffClass::~ChaffClass(){
}

void ChaffClass::InitLocalData(){
	bombType = Chaff;
}

void ChaffClass::InitData(){
	BombClass::InitData();
	InitLocalData();
}

void ChaffClass::CleanupLocalData(){
	// empty
}

void ChaffClass::CleanupData(){
	CleanupLocalData();
	BombClass::CleanupData();
}

int ChaffClass::SaveSize(){
	return BombClass::SaveSize();
}

int ChaffClass::Save(VU_BYTE **stream)
{
	int saveSize = BombClass::Save (stream);
	return saveSize;
}

int ChaffClass::Save(FILE *file)
{
int saveSize = SimWeaponClass::Save (file);

   return saveSize;
}

void ChaffClass::Init (SimInitDataClass* initData){
	BombClass::Init(initData);
}

void ChaffClass::Init(){
	BombClass::Init();
}

int ChaffClass::Wake()
{
	int retval = 0;

	if (IsAwake()){
		return retval;
	}

	BombClass::Wake();

	if ( parent ){
         x = parent->XPos();
         y = parent->YPos();
         z = parent->ZPos();

		ShiAssert( parent->IsSim() );
	}

	return retval;
}

int ChaffClass::Sleep(){
   return SimWeaponClass::Sleep();
}

void ChaffClass::Start(vector* pos, vector* rate, float cD ){
	BombClass::Start(pos, rate, cD);
	dragCoeff = 1.0f;
}

void ChaffClass::CreateGfx(){
	// dont call base class since it create wrong gfx for some reason!!!!!
	InitTrail();
	ExtraGraphics();
}

void ChaffClass::DestroyGfx(){
	BombClass::DestroyGfx();
}

void ChaffClass::ExtraGraphics(){
	if (drawPointer){
		OTWDriver.RemoveObject(drawPointer, TRUE);
		drawPointer = NULL;
	}

    //RV - I-Hawk - Removed chaff GFX here

	//Falcon4EntityClassType *classPtr = &Falcon4ClassTable[displayIndex];
	//OTWDriver.CreateVisualObject(this, classPtr->visType[0], OTWDriver.Scale());

	BombClass::ExtraGraphics();

	Tpoint newPoint = { XPos(), YPos(), ZPos() };
	Tpoint vec = { XDelta(), YDelta(), ZDelta() }; 

	//RV - I-Hawk - PS chaff effect call, to replace the old chaff effect
    
	DrawableParticleSys::PS_AddParticleEx(
		(SFX_CHAFF + 1), &newPoint, &vec
	); 

	timeOfDeath = SimLibElapsedTime;
}

int ChaffClass::Exec(){
	BombClass::Exec();

	if(IsExploding()){
		SetFlag( SHOW_EXPLOSION );
		SetDead(TRUE);
		return TRUE;
	}

	// MLR 2003-11-16 chaff were going thru the ground, I think once they hit the ground they should be removed.
	if(ZPos() > OTWDriver.GetGroundLevel(XPos(), YPos())){
		SetDead(TRUE);
	}

	SetDelta (XDelta() * 0.99F, YDelta() * 0.99F, ZDelta() * 0.99F);
	return TRUE;
}

void ChaffClass::DoExplosion()
{
	SetFlag( SHOW_EXPLOSION );
	SetDead(TRUE);
}


void ChaffClass::SpecialGraphics()
{
	//I-Hawk - commenting all this... chaff GFX is handled by PS now

//// OW
//#if 1
//	// OW: GetGfx() will be zero if we are sleeping
//	/*
//	if (IsAwake()){
//		int mask, frame;
//
//		frame = (SimLibElapsedTime - timeOfDeath) / 250;
//
//		if (frame > 15)
//		{
//			frame = 15;
//		}
//
//		mask = 1 << frame;
//
//		((DrawableBSP*)GetGfx())->SetSwitchMask (0, mask);
//	}
//
//#else
//	int mask, frame;
//
//	frame = (SimLibElapsedTime - timeOfDeath) / 250;
//
//	if (frame > 15)
//	{
//		frame = 15;
//	}
//
//	mask = 1 << frame;
//
//	((DrawableBSP*)GetGfx())->SetSwitchMask(0, mask);
//	*/
//#endif
}

void ChaffClass::InitTrail()
{
	Falcon4EntityClassType* classPtr;

	flags |= IsChaff;
	displayIndex = GetClassID (DOMAIN_AIR, CLASS_SFX, TYPE_CHAFF,
         STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY);

   if (drawPointer)
   {
      //OTWDriver.RemoveObject(drawPointer, TRUE); // FRB - causes CTD
      drawPointer = NULL;
   }

	classPtr = &Falcon4ClassTable[displayIndex];

   //if (IsAwake()) // FRB - Hack to use chaff lod and not the GBU-31
   {
      OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());
      displayIndex = -1;
   }
}

void ChaffClass::UpdateTrail (void)
{
	BombClass::UpdateTrail();

   if (SimLibElapsedTime - timeOfDeath > 10 * SEC_TO_MSEC)
      SetDead (TRUE);
}

void ChaffClass::RemoveTrail (void)
{
	BombClass::RemoveTrail();
}
