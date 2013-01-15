/***************************************************************************\
    Dispopts.h
    Miro "Jammer" Torrielli
    06Oct03

	- Begin Major Rewrite
\***************************************************************************/
#ifndef _DISPLAY_OPTIONS_
#define _DISPLAY_OPTIONS_


class DisplayOptionsClass
{
public:
	unsigned short	DispWidth;
	unsigned short	DispHeight;
	unsigned char	DispVideoCard;
	unsigned char	DispVideoDriver;
	int				DispDepth;
	bool			bRender2Texture;
	bool			bAnisotropicFiltering; 
	bool			bLinearMipFiltering;
	bool			bMipmapping;
	bool			bZBuffering;
	bool			bRender2DCockpit;
	bool			bFontTexelAlignment;
	bool			bSpecularLighting;
	bool			bScreenCoordinateBiasFix;		//Wombat778 4-01-04

	enum TEXMODE {
	 	TEX_MODE_16 = 70159,
	 	TEX_MODE_32,
	 	TEX_MODE_DDS,
	};
	TEXMODE m_texMode;		

	DisplayOptionsClass(void);
	void Initialize(void);
	int LoadOptions(char *filename = "display");
	int SaveOptions(void);
	static void SetDevCaps(unsigned int devCaps);
	static unsigned int GetDevCaps();

private:
	// sfr: used for enumerating only some drivers, not a player option but a command line switch
	// so static, it wont be saved
	static unsigned int    iDeviceCaps;


};

extern DisplayOptionsClass DisplayOptions;


#endif