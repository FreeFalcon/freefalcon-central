/***************************************************************************\
    real2DCloud.h
    Miro "Jammer" Torrielli
    09Nov03

	Volumetric cloud class
\***************************************************************************/
#ifndef _REAL2DCLOUD_H_
#define _REAL2DCLOUD_H_

#include "ObjList.h"
#include "DrawOVC.h"

class Real2DCloud
{
public:
	Real2DCloud();        
	~Real2DCloud();
	void Setup(ObjectDisplayList* objList);
	void Cleanup();

public:
	Drawable2DCloud	*drawable2DCloud;

protected:
	static ObjectDisplayList* objMgr;
};

#endif // _REAL2DCLOUD_H_