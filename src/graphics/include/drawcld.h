/***************************************************************************\
    DrawCLD.h
    Miro "Jammer" Torrielli
    08Nov03

	- Drawable cumulus
\***************************************************************************/
#ifndef _DRAWCLD_H_
#define _DRAWCLD_H_

#include "DrawObj.h"
#include "context.h"
#include "Tex.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif

class Drawable3DCloud : public DrawableObject
{
  public:
	Drawable3DCloud();
	virtual ~Drawable3DCloud();
	virtual void Draw(class RenderOTW *renderer,int);
	void Update(Tpoint *worldPos, int txtIndex);
	static void SetGreenMode(BOOL state);
	static void SetCloudColor(Tcolor *color);

  protected:
	float puffRadius;
	Texture cloudTexture;
	static BOOL	greenMode;
	static Tcolor litCloudColor;

#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert(size == sizeof(Drawable3DCloud)); return MemAllocFS(pool); };
      void operator delete(void *mem) { if(mem) MemFreeFS(mem); };
      static void InitializeStorage() { pool = MemPoolInitFS(sizeof(Drawable3DCloud),10,0); };
      static void ReleaseStorage()	  { MemPoolFree(pool); };
      static MEM_POOL pool;
#endif
};

#endif // _DRAWCLD_H_