/***************************************************************************\
    real3DCloud.h
    Miro "Jammer" Torrielli
    09Nov03

	Volumetric cloud class
\***************************************************************************/
#ifndef _REAL3DCLOUD_H_
#define _REAL3DCLOUD_H_

#include "ObjList.h"
#include "DrawCLD.h"

class Real3DCloud
{
public:
	Real3DCloud();        
	~Real3DCloud();
	void Setup(ObjectDisplayList* objList);
	void Cleanup();

public:
	Drawable3DCloud	*drawable3DClouds;

protected:
	static ObjectDisplayList* objMgr;
};

#endif // _REAL3DCLOUD_H_