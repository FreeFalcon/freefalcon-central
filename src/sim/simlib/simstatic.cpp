#include "stdhdr.h"
#include "simstatc.h"
#include "initdata.h"

SimStaticClass::SimStaticClass(FILE* filePtr) : SimBaseClass (filePtr){
	InitLocalData();
}

SimStaticClass::SimStaticClass(VU_BYTE** stream, long *rem) : SimBaseClass (stream, rem){
	InitLocalData();
}

SimStaticClass::SimStaticClass(int type) : SimBaseClass (type){
	InitLocalData();
}

SimStaticClass::~SimStaticClass(void){
	CleanupLocalData();
}

void SimStaticClass::InitData(){
	SimBaseClass::InitData();
	InitLocalData();
}

void SimStaticClass::InitLocalData(){
}

void SimStaticClass::CleanupData(){
	SimBaseClass::CleanupData();
}

void SimStaticClass::CleanupLocalData(){
}

void SimStaticClass::Init(SimInitDataClass* initData)
{
	SimBaseClass::Init(initData);
	SetTypeFlag(FalconSimObjective);
}

 
// function interface
// serialization functions
int SimStaticClass::SaveSize()
{
   return SimBaseClass::SaveSize();
}

int SimStaticClass::Save(VU_BYTE **stream)
{
   return (SimBaseClass::Save (stream));
}

int SimStaticClass::Save(FILE *file)
{
   return (SimBaseClass::Save (file));
}

int SimStaticClass::Handle(VuFullUpdateEvent *event)
{
   return (SimBaseClass::Handle(event));
}

int SimStaticClass::Handle(VuPositionUpdateEvent *event)
{
   return (SimBaseClass::Handle(event));
}

int SimStaticClass::Handle(VuTransferEvent *event)
{
   return (SimBaseClass::Handle(event));
}
