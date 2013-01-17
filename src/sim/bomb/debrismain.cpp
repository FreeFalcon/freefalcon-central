#include "stdhdr.h"
#include "otwdrive.h"
#include "initdata.h"
#include "object.h"
#include "simdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "debris.h"

#ifdef USE_SH_POOLS
MEM_POOL	DebrisClass::pool;
#endif

DebrisClass::DebrisClass(VU_BYTE** stream, long *rem) : BombClass(stream, rem){
	InitLocalData();
}

DebrisClass::DebrisClass(FILE* filePtr) : BombClass(filePtr){
	InitLocalData();
}

DebrisClass::DebrisClass(int type) : BombClass(type){
	InitLocalData();
}

DebrisClass::~DebrisClass(){
	CleanupLocalData();
}

void DebrisClass::CleanupLocalData(){
	//empty
}

void DebrisClass::CleanupData(){
	CleanupLocalData();
	BombClass::CleanupData();
}

void DebrisClass::InitLocalData(){
	bombType = Debris;
}

void DebrisClass::InitData(){
	BombClass::Init();
	InitLocalData();
}

int DebrisClass::SaveSize()
{
	return BombClass::SaveSize();
}

int DebrisClass::Save(VU_BYTE **stream){
	int saveSize = BombClass::Save (stream);
	return saveSize;
}

int DebrisClass::Save(FILE *file){
	int saveSize = SimWeaponClass::Save (file);
	return saveSize;
}

void DebrisClass::Init (SimInitDataClass* initData)
{
	BombClass::Init(initData);
}

void DebrisClass::Init (void)
{
	BombClass::Init();
}

int DebrisClass::Wake(){
	int retval = 0;
	if (IsAwake())
		return retval;

	BombClass::Wake();
	if ( parent )
	{
         x = parent->XPos();
         y = parent->YPos();
         z = parent->ZPos();

		ShiAssert( parent->IsSim() );
	}

	return retval;
}

int DebrisClass::Sleep (void)
{
   return SimWeaponClass::Sleep();
}

void DebrisClass::Start(vector* pos, vector* rate, float cD ){
	BombClass::Start(pos, rate, cD);
	dragCoeff = 1.0f;
}

void DebrisClass::ExtraGraphics(){
	BombClass::ExtraGraphics();
}

int DebrisClass::Exec (void)
{
	BombClass::Exec();

   if(IsExploding())
   {
      SetFlag( SHOW_EXPLOSION );
      SetDead(TRUE);
      return TRUE;
   }

   SetDelta (XDelta() * 0.99F, YDelta() * 0.99F, ZDelta() * 0.99F);
   return TRUE;
}

void DebrisClass::InitTrail (void)
{
}

void DebrisClass::UpdateTrail (void)
{
}

void DebrisClass::RemoveTrail (void)
{
}
