#ifndef _CPLIGHT_H
#define _CPLIGHT_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Normal Light States
//====================================================//

#define CPLIGHT_OFF			0
#define CPLIGHT_ON			1

//====================================================//
// Special Light States
//====================================================//
#define CPLIGHT_AOA_OFF		0
#define CPLIGHT_AOA_FAST	1
#define CPLIGHT_AOA_ON		2
#define CPLIGHT_AOA_SLOW	3

#define CPLIGHT_AR_NWS_OFF		0
#define CPLIGHT_AR_NWS_RDY		1
#define CPLIGHT_AR_NWS_ON		2
#define CPLIGHT_AR_NWS_DISC	3

#define CPLIGHT_CAUTION_OFF_OFF		0
#define CPLIGHT_CAUTION_OFF_ON		1
#define CPLIGHT_CAUTION_ON_OFF		2
#define CPLIGHT_CAUTION_ON_ON			3

//====================================================//
// Initialization Structures
//====================================================//

//Wombat778 3-22-04 Added type to hold textures for rendered lights.
typedef struct {
	BYTE *light;
	std::vector<TextureHandle *> m_arrTex;
	int mHeight;
	int mWidth;
} SourceLightType;

typedef struct {
					int				states;
					int				cursorId;
					RECT				*psrcRect;
					int				initialState;
					int				type;
					SourceLightType *sourcelights;	//Wombat778 3-22-04
					} LightButtonInitStr;

//====================================================//
// CPLight Class Definition
//====================================================//

class CPLight : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// State Information
	//====================================================//

	int		mStates;
	int		mState;
	//MI
	bool WasPersistant;

	//====================================================//
	// Source Locations for Template Surface
	//====================================================//

	RECT		*mpSrcRect;

	//====================================================//
	// Runtime Member Functions
	//====================================================//

	virtual void	Exec(SimBaseClass*);
	virtual void	DisplayBlit(void);
	void			DisplayBlit3D();

	//Wombat778 3-22-04 Stuff for rendered lights
	SourceLightType		*mpSourceBuffer;
	virtual void CreateLit(void);
	virtual void DiscardLit(void);
	//Wombat778 End

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPLight(ObjectInitStr*, LightButtonInitStr*);
	virtual ~CPLight();
};

#endif

