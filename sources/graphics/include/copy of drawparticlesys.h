/***************************************************************************\
DrawParticleSys.h
MLR

\***************************************************************************/
#ifndef _DRAWPARTICLESYS_H_
#define _DRAWPARTICLESYS_H_

#include "DrawObj.h"
#include "context.h"
#include "Tex.h"
#include "falclib/include/alist.h"

#include "context.h"

enum PSType // it's critical that this, and the name array in the cpp are in sync
{
	PST_NONE            = 0,
	PST_SPARKS          = 1,
	PST_EXPLOSION_SMALL	= 2,
	PST_NAPALM			= 3
};

// Used to draw segmented trails (like missile trails)

class ParticleParam;
class ParticleNode;
class ParticleTextureNode;

class DPS_Node : public ANode
{   // this is used to track particle objects for various internal needs
public:
	class DrawableParticleSys *owner;
};

class DrawableParticleSys : public DrawableObject {
public:
	DrawableParticleSys( int ParticleSysType, float scale = 1.0f );
	virtual ~DrawableParticleSys();
	
	void	AddParticle( int ID, Tpoint *p ,Tpoint *v=0);
	void	AddParticle( Tpoint *p, Tpoint *v=0);

	void Exec(void);
	virtual void Draw( class RenderOTW *renderer, int LOD );
	int  HasParticles(void);
	
	static void LoadParameters(void);
	static void UnloadParameters(void);
	static void SetGreenMode(BOOL state);
	static void SetCloudColor(Tcolor *color);
	
	void    SetHeadVelocity(Tpoint *FPS);
	
protected:
	static BOOL	greenMode;
	static Tcolor litCloudColor;
	
	int					type;
private:
	AList particleList;
	ParticleParam *param;
	int	 Something;
	Tpoint headFPS;
	Tpoint position;

	DPS_Node dpsNode;
	static AList dpsList;
	void ClearParticles(void);

	static AList paramList;
    static AList textureList;
	static ParticleTextureNode *FindTextureNode(char *fn);
	static ParticleTextureNode *GetTextureNode(char *fn);


	static char *nameList[];
	static int nameListCount;

public:
	static void SetupTexturesOnDevice( DXContext *rc );
	static void ReleaseTexturesOnDevice( DXContext *rc );
	static int IsValidPSId(int id);

	static float groundLevel;
	static float cameraDistance;
};

// I need this here cause I'm in a fucking rush.




#endif // _DRAWSGMT_H_

