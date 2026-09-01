// Artscout - 2026 (#104, Linux Ф1): <comcat.h> -- COM component categories. Nothing here runs on Linux: registering
// a component category means writing the COM section of the Windows registry, which does not exist. The types are
// still needed, and not merely for a dead path's sake -- comsup.h:113 defines CreateComponentCategory as an INLINE
// function in a header six unrelated sources pull in (DXEngine.cpp, DX2DEngine.cpp, DXVbManager.cpp, ui_comms.cpp,
// serverbrowser.cpp, Cmpclass.cpp), so its body has to parse in every one of them. It was stopping them twice over:
// CLSID_StdComponentCategoriesMgr was undeclared, and ICatRegister was a forward declaration whose methods the body
// then calls ("member access into incomplete type").
//
// What follows is the SDK's real shape rather than an invention, but note it cannot execute: win32_com.h:46 has
// CoCreateInstance return E_NOINTERFACE, so CreateComponentCategory takes its `if (FAILED(hr)) return hr;` exit and
// never touches the interface. Completing the type fakes no registry -- it only lets an unreachable body compile.
// Linux-only.
#ifndef FF_WIN32SHIM_COMCAT_H
#define FF_WIN32SHIM_COMCAT_H
#include <windows.h>

typedef GUID CATID;
typedef GUID* LPCATID;
typedef const GUID& REFCATID;
#define CATID_NULL (GUID_NULL)

// CATEGORYINFO: the SDK layout. szDescription is fixed at 128 wide chars and comsup.h:126 clamps its copy to 127
// plus a terminator against exactly that bound -- so the size is load-bearing, not decorative.
typedef struct tagCATEGORYINFO
{
    CATID catid;
    LCID lcid;
    WCHAR szDescription[128];
} CATEGORYINFO, *LPCATEGORYINFO;

// ICatRegister in the SDK's method order. The order matters even though no instance can exist here: a vtable is
// positional, so a reordered declaration would be wrong the moment anything real sat behind it.
struct ICatRegister : public IUnknown
{
    virtual HRESULT RegisterCategories(ULONG cCategories,
                                       CATEGORYINFO rgCategoryInfo[]) = 0;
    virtual HRESULT UnRegisterCategories(ULONG cCategories,
                                         CATID rgcatid[]) = 0;
    virtual HRESULT RegisterClassImplCategories(REFCLSID rclsid,
                                                ULONG cCategories,
                                                CATID rgcatid[]) = 0;
    virtual HRESULT UnRegisterClassImplCategories(REFCLSID rclsid,
                                                  ULONG cCategories,
                                                  CATID rgcatid[]) = 0;
    virtual HRESULT RegisterClassReqCategories(REFCLSID rclsid,
                                               ULONG cCategories,
                                               CATID rgcatid[]) = 0;
    virtual HRESULT UnRegisterClassReqCategories(REFCLSID rclsid,
                                                 ULONG cCategories,
                                                 CATID rgcatid[]) = 0;
};
struct
    ICatInformation; // the SDK names it; nothing in the codebase calls through it

// Declared, not defined -- the convention win32_com.h:38 already uses for IID_IUnknown. On Windows these live in
// uuid.lib; Linux has no equivalent, and nothing can legitimately consume them while CoCreateInstance refuses to
// hand back an object. A link error on one of these would mean a genuinely Windows-only path had been dragged into
// the Linux build -- information worth keeping, so no placeholder value is invented here.
extern const CLSID CLSID_StdComponentCategoriesMgr;
extern const IID IID_ICatRegister;
extern const IID IID_ICatInformation;

#endif
