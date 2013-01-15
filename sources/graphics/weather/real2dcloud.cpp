/***************************************************************************\
    real2DCLoud.cpp
    Miro "Jammer" Torrielli
    09Nov03

	Stratus cloud class
\***************************************************************************/
#include "Real2DCloud.h"

ObjectDisplayList *Real2DCloud::objMgr = NULL;

Real2DCloud::Real2DCloud()
{
	drawable2DCloud = NULL;
}

Real2DCloud::~Real2DCloud()
{
	if(drawable2DCloud) Cleanup();
}

void Real2DCloud::Setup(ObjectDisplayList* objList)
{
	objMgr = objList;
	drawable2DCloud = new Drawable2DCloud;
	objList->InsertObject(drawable2DCloud);
}

void Real2DCloud::Cleanup()
{
	if(drawable2DCloud)
	{
		delete drawable2DCloud;
		drawable2DCloud = NULL;
	}
}
