#ifndef NVDXT_OPTIONS_H
#define NVDXT_OPTIONS_H
enum
{
    dSaveButton = 1,
    dCancelButton = 2,

	dDXT1 = 10,
	dTextureFormatFirst = dDXT1,

	dDXT1a = 11,  // DXT1 with one bit alpha
	dDXT3 = 12,   // explicit alpha
	dDXT5 = 13,   // interpolated alpha

	d4444 = 14,   // a4 r4 g4 b4
	d1555 = 15,   // a1 r5 g5 b5
	d565 = 16,    // a0 r5 g6 b5
	d8888 = 17,   // a8 r8 g8 b8
	d888 = 18,    // a0 r8 g8 b8
	d555 = 19,    // a0 r5 g5 b5
    d8   = 20,   // paletted

    dV8U8 = 21,   // DuDv 
    dCxV8U8 = 22,   // normal map
    dA8 = 23,            // alpha only


	dTextureFormatLast = dA8,

    dSaveTextureFormatCombo = 600,


    // 3d viewing options
    d3DPreviewButton = 300, 

    dViewDXT1 = 200,
    dViewDXT2 = 201,
    dViewDXT3 = 202,
    dViewDXT5 = 203,
    dViewA4R4G4B4 = 204,
    dViewA1R5G5B5 = 205,
    dViewR5G6B5 = 206,
    dViewA8R8G8B8 = 207,


    dGenerateMipMaps = 30,
    dMIPMapSourceFirst = dGenerateMipMaps,
	//dSpecifyMipMaps = 31,
	dUseExistingMipMaps = 31,
	dNoMipMaps = 32,
    dMIPMapSourceLast = dNoMipMaps,

    dSpecifiedMipMapsCombo = 39,

    dSpecifiedMipMapsAll = 1100,
    dSpecifiedMipMaps1 = 1101,
    dSpecifiedMipMaps2 = 1102,
    dSpecifiedMipMaps3 = 1103,
    dSpecifiedMipMaps4 = 1104,
    dSpecifiedMipMaps5 = 1105,
    dSpecifiedMipMaps6 = 1106,
    dSpecifiedMipMaps7 = 1107,
    dSpecifiedMipMaps8 = 1108,
    dSpecifiedMipMaps9 = 1109,






    // MIP filters
    dMIPFilterBox = 133,
    dMIPFilterFirst = dMIPFilterBox,
    dMIPFilterCubic = 134,
    dMIPFilterFullDFT = 135,
    dMIPFilterKaiser = 136,
    dMIPFilterLinearLightKaiser = 137,


    dMIPFilterLast = dMIPFilterLinearLightKaiser,

    dMIPFilterCombo = 601,


    dShowDifferences = 40,
    dShowFiltering = 41,
    dShowMipMapping = 42,
    dShowAnisotropic = 43,

    dChangeClearColorButton = 50,
    dDitherColor = 53,

    dLoadBackgroundImageButton = 54,
    dUseBackgroundImage = 55,

    dBinaryAlpha = 56,
    dAlphaBlending = 57,
    dFadeColor = 58,
    dFadeAlpha = 59,

    dFadeToColorButton = 60,
    dAlphaBorder = 61,
    dBorder = 62,
    dBorderColorButton = 63,
	dNormalMap = 64,

    dDitherEachMIPLevel = 66,
    dGreyScale = 67,
    dQuickCompress = 68,

    dbSharpenEachMIPLevel = 70,
    dSharpenEdgeRadius = 71,
    dSharpenLambda = 72,
    dSharpenMu = 73,
    dSharpenTheta = 74,
    dbSharpenUseTwoComponents = 75,
    dbSharpenNonMaximalSuppression = 76,
    dbSharpenFlavor2 = 77,
    dbSharpenSharpBlur = 78,

    dZoom = 79,




	dTextureType2D = 80,
	dTextureTypeFirst = dTextureType2D,
	dTextureTypeCube = 81,
	dTextureTypeImage = 82,
	dTextureTypeVolume = 83,  
	dTextureTypeLast = dTextureTypeVolume,

    dFadeAmount = 90,
    dFadeToAlpha = 91,
    dFadeToDelay = 92,

    dBinaryAlphaThreshold = 94,

    dFilterGamma = 100,
    dFilterKaiserAlpha = 101,
    dFilterWidth = 102,


    dAskToLoadMIPMaps = 400,
    dShowAlphaWarning = 401,
    dShowPower2Warning = 402,

    dAdvancedBlendingButton = 500,
    dUserSpecifiedFadingAmounts = 501,
    dSharpenSettingsButton = 502,
    dFilterSettingsButton = 503,
    dNormalMapGenerationSettingsButton = 504,


    ///////////  Normal Map

    dDOK = 1001,
    dDScaleEditText = 1003,
    dDProxyItem = 1005,
    dDMinZEditText = 1008,

    dDFilter4x = 1040,
    dDFirstFilterRadio = dDFilter4x,
    dDFilter3x3 = 1041,
    dDFilter5x5 = 1042,
    dDFilterDuDv = 1043,
    dDFilter7x7 = 1044,
    dDFilter9x9 = 1045,
    dDLastFilterRadio = dDFilter9x9,

    dDALPHA = 1009,
    dDFirstCnvRadio = dDALPHA,
    dDAVERAGE_RGB = 1010,
    dDBIASED_RGB = 1011,
    dRED = 1012,
    dDGREEN = 1013,
    dDBLUE = 1014,
    dDMAX = 1015,
    dDCOLORSPACE = 1016,
    dDNORMALIZE = 1017,
    dDLastCnvRadio = dDNORMALIZE,


    dDbWrap = 1030,
    dDbInvertX = 1039,
    dDbInvertY = 1037,

    dDAlphaNone = 1033,
    dDFirstAlphaRadio = dDAlphaNone,
    dDAlphaHeight = 1034,
    dDAlphaClear = 1035,
    dDAlphaWhite = 1036,
    dDLastAlphaRadio = dDAlphaWhite,

    dNormalMapConversion = 1050,



};



#ifndef TRGBA
#define TRGBA
typedef	struct	
{
	BYTE	rgba[4];
} rgba_t;
#endif

#ifndef TPIXEL
#define TPIXEL
union tPixel
{
  unsigned long u;
  rgba_t c;
};
#endif


// Windows handle for our plug-in (seen as a dynamically linked library):
extern HANDLE hDllInstance;
class CMyD3DApplication;

typedef enum RescaleOption
{
    RESCALE_NONE,               // no rescale
    RESCALE_NEAREST_POWER2,     // rescale to nearest power of two
    RESCALE_BIGGEST_POWER2,   // rescale to next bigger power of 2
    RESCALE_SMALLEST_POWER2,  // rescale to next smaller power of 2
    RESCALE_PRESCALE
} RescaleOption;


typedef struct CompressionOptions
{
    CompressionOptions()
    {

        bRescaleImageToPower2 = RESCALE_NONE; 
        preScaleX = 1;
        preScaleY = 1;

        bMipMapsInImage = false;    // mip have been loaded in during read

        MipMapType = dGenerateMipMaps;         // dNoMipMaps, dSpecifyMipMaps, dUseExistingMipMaps, dGenerateMipMaps
        SpecifiedMipMaps = 0;   // if dSpecifyMipMaps or dUseExistingMipMaps is set (number of mipmaps to generate)

        MIPFilterType = dMIPFilterBox;      // for MIP maps
        /* 
        for MIPFilterType, specify one of:
        dMIPFilterBox 
        dMIPFilterCubic 
        dMIPFilterFullDFT 
        dMIPFilterKaiser 
        dMIPFilterLinearLightKaiser 
        */


        bBinaryAlpha = false;       // zero or one alpha channel

        bNormalMap= false;         // Is a normal Map
        bDuDvMap= false;           // Is a DuDv (EMBM) map

        bAlphaBorder= false;       // make an alpha border
        bBorder= false;            // make a color border
        BorderColor.u = 0;        // color of border


        bFadeColor = false;         // fade color over MIP maps
        bFadeAlpha= false;         // fade alpha over MIP maps

        FadeToColor.u = 0;        // color to fade to
        FadeToAlpha = 0;        // alpha value to fade to (0-255)

        FadeToDelay = 0;        // start fading after 'n' MIP maps

        FadeAmount = 0;         // percentage of color to fade in

        BinaryAlphaThreshold = 0;  // When Binary Alpha is selected, below this value, alpha is zero


        bDitherColor = false;       // enable dithering during 16 bit conversion
        bDitherEachMIPLevel = false;// enable dithering during 16 bit conversion for each MIP level (after filtering)
        bGreyScale = false;         // treat image as a grey scale
        bQuickCompress = false;         // Fast compression scheme
        bForceDXT1FourColors = false;  // do not let DXT1 use 3 color representation


        // sharpening after creating each MIP map level
        // warp sharp filter parameters
        // look here for details:
        //          
        // "Enhancement by Image-Dependent Warping", 
        // IEEE Transactions on Image Processing, 1999, Vol. 8, No. 8, S. 1063
        // Nur Arad and Craig Gotsman
        // http://www.cs.technion.ac.il/~gotsman/AmendedPubl/EnhancementByImage/EnhancementByI-D.pdf


        bSharpenEachMIPLevel = false;
        SharpenEdgeRadius = 2;
        SharpenLambda = 10;
        SharpenMu = 0.01f;
        SharpenTheta =  0.75;
        bSharpenUseTwoComponents = false;
        bSharpenNonMaximalSuppression = false;
        bSharpenSharpBlur = false;
        bSharpenFlavor2 = false;

        // gamma value for Kaiser, Light Linear
        FilterGamma = 2.2F;  // MD -- 20031114: add the "F" to fix a truncation warning.
        // alpha value for kaiser filter
        FilterKaiserAlpha = 4.0;
        // width of filter
        FilterWidth = 10;

        TextureType = dTextureType2D;        // regular decal, cube or volume  
        /*
        for TextureType, specify one of:
        dTextureType2D 
        dTextureTypeCube 
        dTextureTypeImage 
        dTextureTypeVolume
        */

        TextureFormat = dDXT1;	    
        /* 
        for TextureFormat, specify any from dTextureFormatFirst to 
        dTextureFormatLast

        dDXT1, 
        dDXT1a, 
        dDXT3, 
        dDXT5, 
        d4444, 
        d1555, 	
        d565,	
        d8888, 	
        d888, 
        d555, 
        dV8U8, 
        dCxV8U8, 

        d8,   // paletted
        dA8,            // alpha only

        */

        bSwapRGB = false;           // swap color positions R and G

    };

    RescaleOption   bRescaleImageToPower2; 
    float   preScaleX;
    float   preScaleY;

    bool            bMipMapsInImage;    // mip have been loaded in during read
    short           MipMapType;         // dNoMipMaps, dSpecifyMipMaps, dUseExistingMipMaps, dGenerateMipMaps

    short           SpecifiedMipMaps;   // if dSpecifyMipMaps or dUseExistingMipMaps is set (number of mipmaps to generate)

    short           MIPFilterType;      // for MIP maps
    /* 
        for MIPFilterType, specify one of:
            dMIPFilterBox 
            dMIPFilterCubic 
            dMIPFilterFullDFT 
            dMIPFilterKaiser 
            dMIPFilterLinearLightKaiser 
    */


    bool        bBinaryAlpha;       // zero or one alpha channel

    bool        bNormalMap;         // Is a normal Map
    bool        bDuDvMap;           // Is a DuDv (EMBM) map

    bool        bAlphaBorder;       // make an alpha border
    bool        bBorder;            // make a color border
    tPixel      BorderColor;        // color of border


    bool        bFadeColor;         // fade color over MIP maps
    bool        bFadeAlpha;         // fade alpha over MIP maps

    tPixel      FadeToColor;        // color to fade to
    int         FadeToAlpha;        // alpha value to fade to (0-255)

    int         FadeToDelay;        // start fading after 'n' MIP maps

    int         FadeAmount;         // percentage of color to fade in

    int         BinaryAlphaThreshold;  // When Binary Alpha is selected, below this value, alpha is zero


    bool        bDitherColor;       // enable dithering during 16 bit conversion
    bool        bDitherEachMIPLevel;// enable dithering during 16 bit conversion for each MIP level (after filtering)
    bool        bGreyScale;         // treat image as a grey scale
    bool        bQuickCompress;         // Fast compression scheme
    bool        bForceDXT1FourColors;  // do not let DXT1 use 3 color representation


    // sharpening after creating each MIP map level
    // warp sharp filter parameters
    // look here for details:
    //          
    // "Enhancement by Image-Dependent Warping", 
    // IEEE Transactions on Image Processing, 1999, Vol. 8, No. 8, S. 1063
    // Nur Arad and Craig Gotsman
    // http://www.cs.technion.ac.il/~gotsman/AmendedPubl/EnhancementByImage/EnhancementByI-D.pdf


    bool bSharpenEachMIPLevel;
    int SharpenEdgeRadius;
    float SharpenLambda;
    float SharpenMu;
    float SharpenTheta;
    bool bSharpenUseTwoComponents;
    bool bSharpenNonMaximalSuppression;
    bool bSharpenSharpBlur;
    bool bSharpenFlavor2;

    // gamma value for Kaiser, Light Linear
    float FilterGamma;
    // alpha value for kaiser filter
    float FilterKaiserAlpha;
    // width of filter
    int FilterWidth;




	short 		TextureType;        // regular decal, cube or volume  
	/*
        for TextureType, specify one of:
            dTextureType2D 
    	    dTextureTypeCube 
            dTextureTypeImage 
            dTextureTypeVolume
     */

	short 		TextureFormat;	    
    /* 
        for TextureFormat, specify any from dTextureFormatFirst to 
        dTextureFormatLast

            dDXT1, 
            dDXT1a, 
            dDXT3, 
            dDXT5, 
            d4444, 
	        d1555, 	
            d565,	
            d8888, 	
            d888, 
            d555, 
            dV8U8, 
            dCxV8U8, 
            
            d8,   // paletted
            dA8,            // alpha only

        */

    bool        bSwapRGB;           // swap color positions R and G



} CompressionOptions;


#endif
