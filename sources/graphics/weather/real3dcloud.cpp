/***************************************************************************\
    real3DCLoud.cpp
    Miro "Jammer" Torrielli
    09Nov03

	Volumetric cloud class
\***************************************************************************/
#include "Real3DCloud.h"
#include "RealWeather.h"

ObjectDisplayList *Real3DCloud::objMgr = NULL;

Real3DCloud::Real3DCloud(){
	drawable3DClouds = NULL;
}

Real3DCloud::~Real3DCloud(){
	if(drawable3DClouds){
		Cleanup();
	}
}

void Real3DCloud::Setup(ObjectDisplayList* objList){
	int i;
	objMgr = objList;

	drawable3DClouds = new Drawable3DCloud[NUM_3DCLOUD_POLYS];

	for(i = 0 ; i < NUM_3DCLOUD_POLYS; i++){
		objList->InsertObject(&drawable3DClouds[i]);
	}
}

void Real3DCloud::Cleanup(){
	if(drawable3DClouds){
		delete[] drawable3DClouds;
		drawable3DClouds = NULL;
	}
}
