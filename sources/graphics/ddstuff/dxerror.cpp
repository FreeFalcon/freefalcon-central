/***************************************************************************\
    DXerror.cpp
    Scott Randolph
    November 12, 1996

    This file provide utility functions to decode Direct Draw error codes.
\***************************************************************************/
#include <ddraw.h>

#pragma warning (push)
#pragma warning (disable : 4201)
#include <mmsystem.h>
#pragma warning (pop)

#include <d3d.h>
#include <dsound.h>
#include "DXerror.h"


// Convert a Direct Draw return code into an error message
BOOL DDErrorCheck( HRESULT result )
{
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
}


// Convert a Direct Draw return code into an error message
BOOL D3DErrorCheck( HRESULT result )
{
	switch ( result ) {

      case D3D_OK:                       
        return TRUE;
	  case D3DERR_BADMAJORVERSION:
        MessageBox( NULL, "D3DERR_BADMAJORVERSION", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_BADMINORVERSION:
        MessageBox( NULL, "D3DERR_BADMINORVERSION", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_CREATE_FAILED:
        MessageBox( NULL, "D3DERR_EXECUTE_CREATE_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_DESTROY_FAILED:
        MessageBox( NULL, "D3DERR_EXECUTE_DESTROY_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_LOCK_FAILED:
        MessageBox( NULL, "D3DERR_EXECUTE_LOCK_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_UNLOCK_FAILED:
        MessageBox( NULL, "D3DERR_EXECUTE_UNLOCK_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_LOCKED:
        MessageBox( NULL, "D3DERR_EXECUTE_LOCKED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_NOT_LOCKED:
        MessageBox( NULL, "D3DERR_EXECUTE_NOT_LOCKED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_FAILED:
        MessageBox( NULL, "D3DERR_EXECUTE_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_EXECUTE_CLIPPED_FAILED:
        MessageBox( NULL, "D3DERR_EXECUTE_CLIPPED_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_NO_SUPPORT:
        MessageBox( NULL, "D3DERR_TEXTURE_NO_SUPPORT", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_CREATE_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_CREATE_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_DESTROY_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_DESTROY_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_LOCK_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_LOCK_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_UNLOCK_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_UNLOCK_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_LOAD_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_LOAD_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_SWAP_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_SWAP_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_LOCKED:
        MessageBox( NULL, "D3DERR_TEXTURE_LOCKED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_NOT_LOCKED:
        MessageBox( NULL, "D3DERR_TEXTURE_NOT_LOCKED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_TEXTURE_GETSURF_FAILED:
        MessageBox( NULL, "D3DERR_TEXTURE_GETSURF_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATRIX_CREATE_FAILED:
        MessageBox( NULL, "D3DERR_MATRIX_CREATE_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATRIX_DESTROY_FAILED:
        MessageBox( NULL, "D3DERR_MATRIX_DESTROY_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATRIX_SETDATA_FAILED:
        MessageBox( NULL, "D3DERR_MATRIX_SETDATA_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATRIX_GETDATA_FAILED:
        MessageBox( NULL, "D3DERR_MATRIX_GETDATA_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_SETVIEWPORTDATA_FAILED:
        MessageBox( NULL, "D3DERR_SETVIEWPORTDATA_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATERIAL_CREATE_FAILED:
        MessageBox( NULL, "D3DERR_MATERIAL_CREATE_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATERIAL_DESTROY_FAILED:
        MessageBox( NULL, "D3DERR_MATERIAL_DESTROY_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATERIAL_SETDATA_FAILED:
        MessageBox( NULL, "D3DERR_MATERIAL_SETDATA_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_MATERIAL_GETDATA_FAILED:
        MessageBox( NULL, "D3DERR_MATERIAL_GETDATA_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_LIGHT_SET_FAILED:
        MessageBox( NULL, "D3DERR_LIGHT_SET_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_SCENE_IN_SCENE:
        MessageBox( NULL, "D3DERR_SCENE_IN_SCENE", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_SCENE_NOT_IN_SCENE:
        MessageBox( NULL, "D3DERR_SCENE_NOT_IN_SCENE", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_SCENE_BEGIN_FAILED:
        MessageBox( NULL, "D3DERR_SCENE_BEGIN_FAILED", "D3D Error", MB_OK );
        return FALSE;
      case D3DERR_SCENE_END_FAILED:
        MessageBox( NULL, "D3DERR_SCENE_END_FAILED", "D3D Error", MB_OK );
        return FALSE;
      default:
		MessageBox( NULL, "UNKNOWN ERROR CODE: Trying DD codes", "D3D Error", MB_OK );
        DDErrorCheck( result );
		return FALSE;
    }
}


// Convert a Direct Draw return code into an error message
BOOL DSErrorCheck( HRESULT result )
{
	switch ( result ) {

      case DS_OK:                       
        return TRUE;
      case DSERR_ALLOCATED:
        MessageBox( NULL, "DSERR_ALLOCATED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_CONTROLUNAVAIL:
        MessageBox( NULL, "DSERR_CONTROLUNAVAIL", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_INVALIDPARAM:
        MessageBox( NULL, "DSERR_INVALIDPARAM", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_INVALIDCALL:
        MessageBox( NULL, "DSERR_INVALIDCALL", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_GENERIC:
        MessageBox( NULL, "DSERR_GENERIC", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_PRIOLEVELNEEDED:
        MessageBox( NULL, "DSERR_PRIOLEVELNEEDED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_OUTOFMEMORY:
        MessageBox( NULL, "DSERR_OUTOFMEMORY", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_BADFORMAT:
        MessageBox( NULL, "DSERR_BADFORMAT", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_UNSUPPORTED:
        MessageBox( NULL, "DSERR_UNSUPPORTED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_NODRIVER:
        MessageBox( NULL, "DSERR_NODRIVER", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_ALREADYINITIALIZED:
        MessageBox( NULL, "DSERR_ALREADYINITIALIZED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_NOAGGREGATION:
        MessageBox( NULL, "DSERR_NOAGGREGATION", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_BUFFERLOST:
        MessageBox( NULL, "DSERR_BUFFERLOST", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_OTHERAPPHASPRIO:
        MessageBox( NULL, "DSERR_OTHERAPPHASPRIO", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_UNINITIALIZED:
        MessageBox( NULL, "DSERR_UNINITIALIZED", "DSound Error", MB_OK );
        return FALSE;
      default:
		MessageBox( NULL, "UNKNOWN ERROR CODE", "DSound Error", MB_OK );
		return FALSE;
    }
}
