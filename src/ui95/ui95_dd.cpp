#include <windows.h>
//#include <ddraw.h>
//#include "../3Dlib/Image.h"
#include "chandler.h"
#include "ui95_dd.h"

static WORD reds, greens, blues; //LSH for r,g,b
static WORD redc, greenc, bluec; //count bits=1 after LSH
//XXstatic WORD redm, greenm, bluem;//color masks
static DWORD redm, greenm, bluem;//color masks

extern int FloatToInt32(float);

WORD UIColorTable[201][256];  // Used for translucent conversion (Color Range 0.0 to 2.0)
WORD rUIColorTable[201][256]; // Used for translucent conversion (Color Range 0.0 to 2.0)
WORD gUIColorTable[201][256]; // Used for translucent conversion (Color Range 0.0 to 2.0)
WORD bUIColorTable[201][256]; // Used for translucent conversion (Color Range 0.0 to 2.0)

WORD Grey_1[32];
WORD Grey_3[32];
WORD Grey_6[32];

WORD rShift[32];
WORD gShift[32];
WORD bShift[32];

void UIBuildColorTable()
{
    int i, j;
    float color;

    for (i = 0; i < 201; i++)
    {
        color = (float)i / 100;

        for (j = 0; j < 256; j++)
            if ((j * color) >= 0x1f) //1f = 31 = 1<<5 - 1
            {
                UIColorTable[i][j] = 0x1f;
                rUIColorTable[i][j] = (short)(0x1f << reds);
                gUIColorTable[i][j] = (short)(0x1f << greens);
                bUIColorTable[i][j] = (short)(0x1f << blues);
            }
            else
            {
                UIColorTable[i][j] = WORD(j * color);
                rUIColorTable[i][j] = static_cast<WORD>(WORD(j * color) << reds); 
                gUIColorTable[i][j] = static_cast<WORD>(WORD(j * color) << greens);
                bUIColorTable[i][j] = static_cast<WORD>(WORD(j * color) << blues);
            }
    }

    for (i = 0; i < 32; i++)
    {
        Grey_1[i] = static_cast<WORD>(FloatToInt32((float)i * 0.1f)); 
        Grey_3[i] = static_cast<WORD>(FloatToInt32((float)i * 0.3f)); 
        Grey_6[i] = static_cast<WORD>(FloatToInt32((float)i * 0.6f)); 
        rShift[i] = static_cast<short>(i << reds);
        gShift[i] = static_cast<short>(i << greens);
        bShift[i] = static_cast<short>(i << blues);
    }
}

//XXvoid UI95_SetScreenColorInfo( WORD r_mask,WORD g_mask,WORD b_mask )
void UI95_SetScreenColorInfo(DWORD r_mask, DWORD g_mask, DWORD b_mask)
{
    ShiAssert(r_mask not_eq 0 and g_mask not_eq 0 and b_mask not_eq 0); // this should never happen
    // but I saw it once (JPO)


    //XX
    if (r_mask == 0x00FF0000)
        r_mask = 0xf800;

    if (g_mask == 0x0000FF00)
        g_mask = 0x07e0;

    if (b_mask == 0x000000FF)
        b_mask = 0x001f;

    redm = r_mask;
    greenm = g_mask;
    bluem = b_mask;

    // RED
    reds = 0;

    while (r_mask and not (r_mask bitand 1))   // JPO cater for no reds - weird
    {
        r_mask >>= 1;
        reds++;
    }

    redc = 0;

    while (r_mask bitand 1)
    {
        r_mask >>= 1;
        redc++;
    }

    if (redc == 6) //6 meaninig bits per red color component
        reds++;

    // GREEN
    greens = 0;

    while (g_mask and not (g_mask bitand 1))
    {
        g_mask >>= 1;
        greens++;
    }

    greenc = 0;

    while (g_mask bitand 1)
    {
        g_mask >>= 1;
        greenc++;
    }

    if (greenc == 6)
        greens++;

    // BLUE
    blues = 0;

    while (b_mask and not (b_mask bitand 1))
    {
        b_mask >>= 1;
        blues++;
    }

    bluec = 0;

    while (b_mask bitand 1)
    {
        b_mask >>= 1;
        bluec++;
    }

    if (bluec == 6)
        blues++;
}

/*  
void UI95_GetScreenColorInfo(WORD *r_mask,WORD *r_shift,WORD *g_mask,WORD *g_shift,WORD *b_mask,WORD *b_shift)
{
 *r_mask = redm;
 *g_mask = greenm;
 *b_mask = bluem;

 *r_shift=reds;
 *g_shift=greens;
 *b_shift=blues;
}*/

//XX void UI95_GetScreenColorInfo(WORD &r_mask,WORD &r_shift,WORD &g_mask, WORD &g_shift,WORD &b_mask,WORD &b_shift)
void UI95_GetScreenColorInfo(DWORD &r_mask, WORD &r_shift, DWORD &g_mask, WORD &g_shift, DWORD &b_mask, WORD &b_shift)
{
    r_mask = redm;
    g_mask = greenm;
    b_mask = bluem;

    r_shift = reds;
    g_shift = greens;
    b_shift = blues;
}


WORD UI95_RGB15Bit(WORD rgb)
{
    return static_cast<WORD>(rShift[(rgb >> 10) bitand 0x1f] bitor gShift[(rgb >> 5) bitand 0x1f] bitor bShift[rgb bitand 0x1f]); 
}


WORD UI95_RGB24Bit(unsigned long rgb)
{
    return static_cast<WORD>(rShift[(rgb >> 3) bitand 0x1f] bitor gShift[(rgb >> 11) bitand 0x1f] bitor bShift[(rgb >> 19) bitand 0x1f]);
}

WORD UI95_ScreenToTga(WORD color)
{
    long r, g, b;

    r = ((color >> reds) bitand 0x1f) << 10;
    g = ((color >> greens) bitand 0x1f) << 5;
    b = ((color >> blues) bitand 0x1f);

    return static_cast<WORD>(r bitor g bitor b); 
}

WORD UI95_ScreenToGrey(WORD color)
{
    long grey = Grey_3[(color  >> reds) bitand 0x1f] + Grey_6[(color  >> greens) bitand 0x1f] + Grey_1[(color  >> blues) bitand 0x1f];

    return static_cast<WORD>(rShift[grey] bitor gShift[grey] bitor bShift[grey]); 
}

/*
void UI95_GetScreenFormat(DDSURFACEDESC *desc)
{
 DWORD mask;

 memcpy(&UI95_ScreenFormat,desc,desc->dwSize);

 mask = UI95_ScreenFormat.ddpfPixelFormat.dwRBitMask;
 reds = 0;
 while( not (mask bitand 1) ) {
 mask >>= 1;
 reds++;
 }
 redc=0;
 while( mask bitand 1 ) {
 mask >>= 1;
 redc++;
 }
 if(redc == 6)
 reds++;

 // GREEN
 mask = UI95_ScreenFormat.ddpfPixelFormat.dwGBitMask;
 greens = 0;
 while( not (mask bitand 1) ) {
 mask >>= 1;
 greens++;
 }
 greenc=0;
 while( mask bitand 1 ) {
 mask >>= 1;
 greenc++;
 }
 if(greenc == 6)
 greens++;

 // BLUE
 mask = UI95_ScreenFormat.ddpfPixelFormat.dwBBitMask;
 blues = 0;
 while( not (mask bitand 1) ) {
 mask >>= 1;
 blues++;
 }
 bluec=0;
 while( mask bitand 1 ) {
 mask >>= 1;
 bluec++;
 }
 if(bluec == 6)
 blues++;
}
*/

/*
IDirectDrawSurface *UI95_CreateDDSurface(IDirectDraw *DD,DWORD width,DWORD height)
{
    HRESULT             result;
 IDirectDrawSurface *dds;
    DDSURFACEDESC ddDescription;

    // Initialize the surface description structure we'll use to request our surface
    memset( &ddDescription, 0, sizeof( ddDescription ) );
    ddDescription.dwSize = sizeof( ddDescription );
    ddDescription.dwFlags = DDSD_CAPS;

 ddDescription.ddsCaps.dwCaps or_eq DDSCAPS_SYSTEMMEMORY bitor DDSCAPS_OFFSCREENPLAIN bitor DDSCAPS_3DDEVICE;
 ddDescription.dwFlags or_eq DDSD_WIDTH bitor DDSD_HEIGHT;
 ddDescription.dwWidth = width;
 ddDescription.dwHeight = height;

    result=DD->CreateSurface( &ddDescription, &dds, NULL );
 UI95_DDErrorCheck(result);
 result=dds->GetSurfaceDesc( &ddDescription );
 UI95_DDErrorCheck(result);
 UI95_GetScreenFormat(&ddDescription);
 return (dds);
}
*/

/*
void *UI95_Lock(IDirectDrawSurface *ddSurface)
{
 HRESULT result;
 // Initialize the surface description structure that we want filled in
 memset( &UI95_ScreenFormat, 0, sizeof( UI95_ScreenFormat ) );
 UI95_ScreenFormat.dwSize = sizeof( UI95_ScreenFormat );

 result=ddSurface->Lock( NULL, &UI95_ScreenFormat, DDLOCK_SURFACEMEMORYPTR bitor DDLOCK_WAIT, NULL );
 if(result == DD_OK)
 return(UI95_ScreenFormat.lpSurface);
 UI95_DDErrorCheck( result );
 return(NULL);
}
*/

/*
void CVTImageToDDS()
{
}

GLImageInfo *LoadImageFile(char *filename)
{
 GLImageInfo *glImage;
 CImageFileMemory  texFile;
 short result;


 if( not filename)
 return(NULL);

 // Make sure we recognize this file type
 texFile.imageType = CheckImageType( (GLbyte*)filename );
 if(texFile.imageType == IMAGE_TYPE_UNKNOWN )
 return(NULL);

 // Open the input file
 result = texFile.glOpenFileMem( (GLbyte*)filename );
 if ( result not_eq 1 )
 return(NULL);

 // Read the image data (note that ReadTextureImage will close texFile for us)
 texFile.glReadFileMem();
 result = ReadTextureImage( &texFile );
 if (result not_eq GOOD_READ)
 return(NULL);

 // Store the image properties in our local storage
 width = texFile.image.width;
 height = texFile.image.height;

 // Do things differently for 8 bit and RGB images
 if (flags bitand MPR_TI_PALETTE) {

 imageData = texFile.image.image;

 // Create a palette object if we don't already have one
 ShiAssert( texFile.image.palette );
 if ( not palette) {
 palette = new Palette;
 palette->Setup32( (DWORD*)texFile.image.palette );
 } else {
 palette->Reference();
 }

 // Release the image's palette data now that we've got our own copy
 glReleaseMemory( texFile.image.palette );
 } else {

 // Force the image into 32bit ABGR format
 ShiAssert( palette == NULL );
 if ( texFile.image.palette ) {
 imageData = ConvertImage( &texFile.image, COLOR_16M, NULL );
 glReleaseMemory( texFile.image.image );
 glReleaseMemory( texFile.image.palette );
 }
 }
}

IDirectDrawSurface *LoadImageFile(char *fname)
{
}
*/

/*
BOOL UI95_DDErrorCheck( HRESULT result )
{
 if(result not_eq DD_OK) return(FALSE);
 return(TRUE);
 switch ( result ) {

      case DD_OK:
        return TRUE;
      case DDERR_ALREADYINITIALIZED:
        MessageBox( NULL, "DDERR_ALREADYINITIALIZED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_BLTFASTCANTCLIP:
        MessageBox( NULL, "DDERR_BLTFASTCANTCLIP", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_CANNOTDETACHSURFACE:
        MessageBox( NULL, "DDERR_CANNOTDETACHSURFACE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_CANTCREATEDC:
        MessageBox( NULL, "DDERR_CANTCREATEDC", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_CANTDUPLICATE:
        MessageBox( NULL, "DDERR_CANTDUPLICATE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_CLIPPERISUSINGHWND:
        MessageBox( NULL, "DDERR_CLIPPERISUSINGHWND", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_COLORKEYNOTSET:
        MessageBox( NULL, "DDERR_COLORKEYNOTSET", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_CURRENTLYNOTAVAIL:
        MessageBox( NULL, "DDERR_CURRENTLYNOTAVAIL", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_DIRECTDRAWALREADYCREATED:
        MessageBox( NULL, "DDERR_DIRECTDRAWALREADYCREATED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_EXCEPTION:
        MessageBox( NULL, "DDERR_EXCEPTION", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_EXCLUSIVEMODEALREADYSET:
        MessageBox( NULL, "DDERR_EXCLUSIVEMODEALREADYSET", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_GENERIC:
        MessageBox( NULL, "DDERR_GENERIC", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_HEIGHTALIGN:
        MessageBox( NULL, "DDERR_HEIGHTALIGN", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_HWNDALREADYSET:
        MessageBox( NULL, "DDERR_HWNDALREADYSET", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_HWNDSUBCLASSED:
        MessageBox( NULL, "DDERR_HWNDSUBCLASSED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_IMPLICITLYCREATED:
        MessageBox( NULL, "DDERR_IMPLICITLYCREATED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INCOMPATIBLEPRIMARY:
        MessageBox( NULL, "DDERR_INCOMPATIBLEPRIMARY", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDCAPS:
        MessageBox( NULL, "DDERR_INVALIDCAPS", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDCLIPLIST:
        MessageBox( NULL, "DDERR_INVALIDCLIPLIST", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDDIRECTDRAWGUID:
        MessageBox( NULL, "DDERR_INVALIDDIRECTDRAWGUID", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDMODE:
        MessageBox( NULL, "DDERR_INVALIDMODE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDOBJECT:
        MessageBox( NULL, "DDERR_INVALIDOBJECT", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDPARAMS:
        MessageBox( NULL, "DDERR_INVALIDPARAMS", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDPIXELFORMAT:
        MessageBox( NULL, "DDERR_INVALIDPIXELFORMAT", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDPOSITION:
        MessageBox( NULL, "DDERR_INVALIDPOSITION", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_INVALIDRECT:
        MessageBox( NULL, "DDERR_INVALIDRECT", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_LOCKEDSURFACES:
        MessageBox( NULL, "DDERR_LOCKEDSURFACES", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NO3D:
        MessageBox( NULL, "DDERR_NO3D", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOALPHAHW:
        MessageBox( NULL, "DDERR_NOALPHAHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOBLTHW:
        MessageBox( NULL, "DDERR_NOBLTHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOCLIPLIST:
        MessageBox( NULL, "DDERR_NOCLIPLIST", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOCLIPPERATTACHED:
        MessageBox( NULL, "DDERR_NOCLIPPERATTACHED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOCOLORCONVHW:
        MessageBox( NULL, "DDERR_NOCOLORCONVHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOCOLORKEY:
        MessageBox( NULL, "DDERR_NOCOLORKEY", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOCOLORKEYHW:
        MessageBox( NULL, "DDERR_NOCOLORKEYHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOCOOPERATIVELEVELSET:
        MessageBox( NULL, "DDERR_NOCOOPERATIVELEVELSET", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NODC:
        MessageBox( NULL, "DDERR_NODC", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NODDROPSHW:
        MessageBox( NULL, "DDERR_NODDROPSHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NODIRECTDRAWHW:
        MessageBox( NULL, "DDERR_NODIRECTDRAWHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOEMULATION:
        MessageBox( NULL, "DDERR_NOEMULATION", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOEXCLUSIVEMODE:
        MessageBox( NULL, "DDERR_NOEXCLUSIVEMODE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOFLIPHW:
        MessageBox( NULL, "DDERR_NOFLIPHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOGDI:
        MessageBox( NULL, "DDERR_NOGDI", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOHWND:
        MessageBox( NULL, "DDERR_NOHWND", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOMIRRORHW:
        MessageBox( NULL, "DDERR_NOMIRRORHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOOVERLAYDEST:
        MessageBox( NULL, "DDERR_NOOVERLAYDEST", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOOVERLAYHW:
        MessageBox( NULL, "DDERR_NOOVERLAYHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOPALETTEATTACHED:
        MessageBox( NULL, "DDERR_NOPALETTEATTACHED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOPALETTEHW:
        MessageBox( NULL, "DDERR_NOPALETTEHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NORASTEROPHW:
        MessageBox( NULL, "DDERR_NORASTEROPHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOROTATIONHW:
        MessageBox( NULL, "DDERR_NOROTATIONHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOSTRETCHHW:
        MessageBox( NULL, "DDERR_NOSTRETCHHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOT4BITCOLOR:
        MessageBox( NULL, "DDERR_NOT4BITCOLOR", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOT4BITCOLORINDEX:
        MessageBox( NULL, "DDERR_NOT4BITCOLORINDEX", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOT8BITCOLOR:
        MessageBox( NULL, "DDERR_NOT8BITCOLOR", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOTAOVERLAYSURFACE:
        MessageBox( NULL, "DDERR_NOTAOVERLAYSURFACE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOTEXTUREHW:
        MessageBox( NULL, "DDERR_NOTEXTUREHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOTFLIPPABLE:
        MessageBox( NULL, "DDERR_NOTFLIPPABLE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOTFOUND:
        MessageBox( NULL, "DDERR_NOTFOUND", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOTLOCKED:
        MessageBox( NULL, "DDERR_NOTLOCKED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOTPALETTIZED:
        MessageBox( NULL, "DDERR_NOTPALETTIZED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOVSYNCHW:
        MessageBox( NULL, "DDERR_NOVSYNCHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOZBUFFERHW:
        MessageBox( NULL, "DDERR_NOZBUFFERHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_NOZOVERLAYHW:
        MessageBox( NULL, "DDERR_NOZOVERLAYHW", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_OUTOFCAPS:
        MessageBox( NULL, "DDERR_OUTOFCAPS", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_OUTOFMEMORY:
        MessageBox( NULL, "DDERR_OUTOFMEMORY", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_OUTOFVIDEOMEMORY:
        MessageBox( NULL, "DDERR_OUTOFVIDEOMEMORY", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_OVERLAYCANTCLIP:
        MessageBox( NULL, "DDERR_OVERLAYCANTCLIP", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        MessageBox( NULL, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_OVERLAYNOTVISIBLE:
        MessageBox( NULL, "DDERR_OVERLAYNOTVISIBLE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_PALETTEBUSY:
        MessageBox( NULL, "DDERR_PALETTEBUSY", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_PRIMARYSURFACEALREADYEXISTS:
        MessageBox( NULL, "DDERR_PRIMARYSURFACEALREADYEXISTS", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_REGIONTOOSMALL:
        MessageBox( NULL, "DDERR_REGIONTOOSMALL", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_SURFACEALREADYATTACHED:
        MessageBox( NULL, "DDERR_SURFACEALREADYATTACHED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_SURFACEALREADYDEPENDENT:
        MessageBox( NULL, "DDERR_SURFACEALREADYDEPENDENT", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_SURFACEBUSY:
 //return(FALSE);
        MessageBox( NULL, "DDERR_SURFACEBUSY", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_SURFACEISOBSCURED:
        MessageBox( NULL, "DDERR_SURFACEISOBSCURED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_SURFACELOST:
        MessageBox( NULL, "DDERR_SURFACELOST", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_SURFACENOTATTACHED:
        MessageBox( NULL, "DDERR_SURFACENOTATTACHED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_TOOBIGHEIGHT:
        MessageBox( NULL, "DDERR_TOOBIGHEIGHT", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_TOOBIGSIZE:
        MessageBox( NULL, "DDERR_TOOBIGSIZE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_TOOBIGWIDTH:
        MessageBox( NULL, "DDERR_TOOBIGWIDTH", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_UNSUPPORTED:
        MessageBox( NULL, "DDERR_UNSUPPORTED", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_UNSUPPORTEDFORMAT:
        MessageBox( NULL, "DDERR_UNSUPPORTEDFORMAT", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_UNSUPPORTEDMASK:
        MessageBox( NULL, "DDERR_UNSUPPORTEDMASK", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_VERTICALBLANKINPROGRESS:
        MessageBox( NULL, "DDERR_VERTICALBLANKINPROGRESS", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_WASSTILLDRAWING:
        MessageBox( NULL, "DDERR_WASSTILLDRAWING", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_WRONGMODE:
        MessageBox( NULL, "DDERR_WRONGMODE", "DDraw Error", MB_OK );
        return FALSE;
      case DDERR_XALIGN:
        MessageBox( NULL, "DDERR_XALIGN", "DDraw Error", MB_OK );
        return FALSE;
      default:
        MessageBox( NULL, "UNKNOWN ERROR CODE", "DDraw Error", MB_OK );
        return FALSE;
    }
 return(FALSE);
}
*/

/*
BOOL UI95_DDErrorCheck( HRESULT result )
{
 switch ( result ) {

      case DD_OK:
        return TRUE;
      case DDERR_ALREADYINITIALIZED:
        MonoPrint("DDERR_ALREADYINITIALIZED\n");
        return FALSE;
      case DDERR_BLTFASTCANTCLIP:
        MonoPrint("DDERR_BLTFASTCANTCLIP\n");
        return FALSE;
      case DDERR_CANNOTDETACHSURFACE:
        MonoPrint("DDERR_CANNOTDETACHSURFACE\n");
        return FALSE;
      case DDERR_CANTCREATEDC:
        MonoPrint("DDERR_CANTCREATEDC\n");
        return FALSE;
      case DDERR_CANTDUPLICATE:
        MonoPrint("DDERR_CANTDUPLICATE\n");
        return FALSE;
      case DDERR_CLIPPERISUSINGHWND:
        MonoPrint("DDERR_CLIPPERISUSINGHWND\n");
        return FALSE;
      case DDERR_COLORKEYNOTSET:
        MonoPrint("DDERR_COLORKEYNOTSET\n");
        return FALSE;
      case DDERR_CURRENTLYNOTAVAIL:
        MonoPrint("DDERR_CURRENTLYNOTAVAIL\n");
        return FALSE;
      case DDERR_DIRECTDRAWALREADYCREATED:
        MonoPrint("DDERR_DIRECTDRAWALREADYCREATED\n");
        return FALSE;
      case DDERR_EXCEPTION:
        MonoPrint("DDERR_EXCEPTION\n");
        return FALSE;
      case DDERR_EXCLUSIVEMODEALREADYSET:
        MonoPrint("DDERR_EXCLUSIVEMODEALREADYSET\n");
        return FALSE;
      case DDERR_GENERIC:
        MonoPrint("DDERR_GENERIC\n");
        return FALSE;
      case DDERR_HEIGHTALIGN:
        MonoPrint("DDERR_HEIGHTALIGN\n");
        return FALSE;
      case DDERR_HWNDALREADYSET:
        MonoPrint("DDERR_HWNDALREADYSET\n");
        return FALSE;
      case DDERR_HWNDSUBCLASSED:
        MonoPrint("DDERR_HWNDSUBCLASSED\n");
        return FALSE;
      case DDERR_IMPLICITLYCREATED:
        MonoPrint("DDERR_IMPLICITLYCREATED\n");
        return FALSE;
      case DDERR_INCOMPATIBLEPRIMARY:
        MonoPrint("DDERR_INCOMPATIBLEPRIMARY\n");
        return FALSE;
      case DDERR_INVALIDCAPS:
        MonoPrint("DDERR_INVALIDCAPS\n");
        return FALSE;
      case DDERR_INVALIDCLIPLIST:
        MonoPrint("DDERR_INVALIDCLIPLIST\n");
        return FALSE;
      case DDERR_INVALIDDIRECTDRAWGUID:
        MonoPrint("DDERR_INVALIDDIRECTDRAWGUID\n");
        return FALSE;
      case DDERR_INVALIDMODE:
        MonoPrint("DDERR_INVALIDMODE\n");
        return FALSE;
      case DDERR_INVALIDOBJECT:
        MonoPrint("DDERR_INVALIDOBJECT\n");
        return FALSE;
      case DDERR_INVALIDPARAMS:
        MonoPrint("DDERR_INVALIDPARAMS\n");
        return FALSE;
      case DDERR_INVALIDPIXELFORMAT:
        MonoPrint("DDERR_INVALIDPIXELFORMAT\n");
        return FALSE;
      case DDERR_INVALIDPOSITION:
        MonoPrint("DDERR_INVALIDPOSITION\n");
        return FALSE;
      case DDERR_INVALIDRECT:
        MonoPrint("DDERR_INVALIDRECT\n");
        return FALSE;
      case DDERR_LOCKEDSURFACES:
        MonoPrint("DDERR_LOCKEDSURFACES\n");
        return FALSE;
      case DDERR_NO3D:
        MonoPrint("DDERR_NO3D\n");
        return FALSE;
      case DDERR_NOALPHAHW:
        MonoPrint("DDERR_NOALPHAHW\n");
        return FALSE;
      case DDERR_NOBLTHW:
        MonoPrint("DDERR_NOBLTHW\n");
        return FALSE;
      case DDERR_NOCLIPLIST:
        MonoPrint("DDERR_NOCLIPLIST\n");
        return FALSE;
      case DDERR_NOCLIPPERATTACHED:
        MonoPrint("DDERR_NOCLIPPERATTACHED\n");
        return FALSE;
      case DDERR_NOCOLORCONVHW:
        MonoPrint("DDERR_NOCOLORCONVHW\n");
        return FALSE;
      case DDERR_NOCOLORKEY:
        MonoPrint("DDERR_NOCOLORKEY\n");
        return FALSE;
      case DDERR_NOCOLORKEYHW:
        MonoPrint("DDERR_NOCOLORKEYHW\n");
        return FALSE;
      case DDERR_NOCOOPERATIVELEVELSET:
        MonoPrint("DDERR_NOCOOPERATIVELEVELSET\n");
        return FALSE;
      case DDERR_NODC:
        MonoPrint("DDERR_NODC\n");
        return FALSE;
      case DDERR_NODDROPSHW:
        MonoPrint("DDERR_NODDROPSHW\n");
        return FALSE;
      case DDERR_NODIRECTDRAWHW:
        MonoPrint("DDERR_NODIRECTDRAWHW\n");
        return FALSE;
      case DDERR_NOEMULATION:
        MonoPrint("DDERR_NOEMULATION\n");
        return FALSE;
      case DDERR_NOEXCLUSIVEMODE:
        MonoPrint("DDERR_NOEXCLUSIVEMODE\n");
        return FALSE;
      case DDERR_NOFLIPHW:
        MonoPrint("DDERR_NOFLIPHW\n");
        return FALSE;
      case DDERR_NOGDI:
        MonoPrint("DDERR_NOGDI\n");
        return FALSE;
      case DDERR_NOHWND:
        MonoPrint("DDERR_NOHWND\n");
        return FALSE;
      case DDERR_NOMIRRORHW:
        MonoPrint("DDERR_NOMIRRORHW\n");
        return FALSE;
      case DDERR_NOOVERLAYDEST:
        MonoPrint("DDERR_NOOVERLAYDEST\n");
        return FALSE;
      case DDERR_NOOVERLAYHW:
        MonoPrint("DDERR_NOOVERLAYHW\n");
        return FALSE;
      case DDERR_NOPALETTEATTACHED:
        MonoPrint("DDERR_NOPALETTEATTACHED\n");
        return FALSE;
      case DDERR_NOPALETTEHW:
        MonoPrint("DDERR_NOPALETTEHW\n");
        return FALSE;
      case DDERR_NORASTEROPHW:
        MonoPrint("DDERR_NORASTEROPHW\n");
        return FALSE;
      case DDERR_NOROTATIONHW:
        MonoPrint("DDERR_NOROTATIONHW\n");
        return FALSE;
      case DDERR_NOSTRETCHHW:
        MonoPrint("DDERR_NOSTRETCHHW\n");
        return FALSE;
      case DDERR_NOT4BITCOLOR:
        MonoPrint("DDERR_NOT4BITCOLOR\n");
        return FALSE;
      case DDERR_NOT4BITCOLORINDEX:
        MonoPrint("DDERR_NOT4BITCOLORINDEX\n");
        return FALSE;
      case DDERR_NOT8BITCOLOR:
        MonoPrint("DDERR_NOT8BITCOLOR\n");
        return FALSE;
      case DDERR_NOTAOVERLAYSURFACE:
        MonoPrint("DDERR_NOTAOVERLAYSURFACE\n");
        return FALSE;
      case DDERR_NOTEXTUREHW:
        MonoPrint("DDERR_NOTEXTUREHW\n");
        return FALSE;
      case DDERR_NOTFLIPPABLE:
        MonoPrint("DDERR_NOTFLIPPABLE\n");
        return FALSE;
      case DDERR_NOTFOUND:
        MonoPrint("DDERR_NOTFOUND\n");
        return FALSE;
      case DDERR_NOTLOCKED:
        MonoPrint("DDERR_NOTLOCKED\n");
        return FALSE;
      case DDERR_NOTPALETTIZED:
        MonoPrint("DDERR_NOTPALETTIZED\n");
        return FALSE;
      case DDERR_NOVSYNCHW:
        MonoPrint("DDERR_NOVSYNCHW\n");
        return FALSE;
      case DDERR_NOZBUFFERHW:
        MonoPrint("DDERR_NOZBUFFERHW\n");
        return FALSE;
      case DDERR_NOZOVERLAYHW:
        MonoPrint("DDERR_NOZOVERLAYHW\n");
        return FALSE;
      case DDERR_OUTOFCAPS:
        MonoPrint("DDERR_OUTOFCAPS\n");
        return FALSE;
      case DDERR_OUTOFMEMORY:
        MonoPrint("DDERR_OUTOFMEMORY\n");
        return FALSE;
      case DDERR_OUTOFVIDEOMEMORY:
        MonoPrint("DDERR_OUTOFVIDEOMEMORY\n");
        return FALSE;
      case DDERR_OVERLAYCANTCLIP:
        MonoPrint("DDERR_OVERLAYCANTCLIP\n");
        return FALSE;
      case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        MonoPrint("DDERR_OVERLAYCOLORKEYONLYONEACTIVE\n");
        return FALSE;
      case DDERR_OVERLAYNOTVISIBLE:
        MonoPrint("DDERR_OVERLAYNOTVISIBLE\n");
        return FALSE;
      case DDERR_PALETTEBUSY:
        MonoPrint("DDERR_PALETTEBUSY\n");
        return FALSE;
      case DDERR_PRIMARYSURFACEALREADYEXISTS:
        MonoPrint("DDERR_PRIMARYSURFACEALREADYEXISTS\n");
        return FALSE;
      case DDERR_REGIONTOOSMALL:
        MonoPrint("DDERR_REGIONTOOSMALL\n");
        return FALSE;
      case DDERR_SURFACEALREADYATTACHED:
        MonoPrint("DDERR_SURFACEALREADYATTACHED\n");
        return FALSE;
      case DDERR_SURFACEALREADYDEPENDENT:
        MonoPrint("DDERR_SURFACEALREADYDEPENDENT\n");
        return FALSE;
      case DDERR_SURFACEBUSY:
 //return(FALSE);
        MonoPrint("DDERR_SURFACEBUSY\n");
        return FALSE;
      case DDERR_SURFACEISOBSCURED:
        MonoPrint("DDERR_SURFACEISOBSCURED\n");
        return FALSE;
      case DDERR_SURFACELOST:
        MonoPrint("DDERR_SURFACELOST\n");
        return FALSE;
      case DDERR_SURFACENOTATTACHED:
        MonoPrint("DDERR_SURFACENOTATTACHED\n");
        return FALSE;
      case DDERR_TOOBIGHEIGHT:
        MonoPrint("DDERR_TOOBIGHEIGHT\n");
        return FALSE;
      case DDERR_TOOBIGSIZE:
        MonoPrint("DDERR_TOOBIGSIZE\n");
        return FALSE;
      case DDERR_TOOBIGWIDTH:
        MonoPrint("DDERR_TOOBIGWIDTH\n");
        return FALSE;
      case DDERR_UNSUPPORTED:
        MonoPrint("DDERR_UNSUPPORTED\n");
        return FALSE;
      case DDERR_UNSUPPORTEDFORMAT:
        MonoPrint("DDERR_UNSUPPORTEDFORMAT\n");
        return FALSE;
      case DDERR_UNSUPPORTEDMASK:
        MonoPrint("DDERR_UNSUPPORTEDMASK\n");
        return FALSE;
      case DDERR_VERTICALBLANKINPROGRESS:
        MonoPrint("DDERR_VERTICALBLANKINPROGRESS\n");
        return FALSE;
      case DDERR_WASSTILLDRAWING:
        MonoPrint("DDERR_WASSTILLDRAWING\n");
        return FALSE;
      case DDERR_WRONGMODE:
        MonoPrint("DDERR_WRONGMODE\n");
        return FALSE;
      case DDERR_XALIGN:
        MonoPrint("DDERR_XALIGN\n");
        return FALSE;
      default:
        MonoPrint("UNKNOWN ERROR CODE\n");
        return FALSE;
    }
 return(FALSE);
}
*/
