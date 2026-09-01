// Artscout - 2026 (#104, Linux Ф1): the minimal COM core (<objbase.h>/<unknwn.h> equivalent). The codebase uses
// IUnknown (Release() on GPU handles), a few IID_/CLSID_ constants, and CoInitialize/CoCreateInstance (WIC image
// load, replaced by stb_image in Ф1/Ф4). ATL (CComPtr) is effectively dead. Pulled in by windows.h. Linux-only.
#ifndef FF_WIN32SHIM_COM_H
#define FF_WIN32SHIM_COM_H

// IUnknown -- the COM base. Real COM objects are not created on Linux (the D3D12 backend is Windows-only; Linux
// uses the Vulkan backend), so this exists mainly so shared code that holds IUnknown* handles / calls Release()
// compiles. A handful of interface stubs (WIC) derive from it.
struct IUnknown
{
    virtual HRESULT QueryInterface(REFIID riid, void** ppv) = 0;
    virtual ULONG AddRef(void) = 0;
    virtual ULONG Release(void) = 0;
};
typedef IUnknown* LPUNKNOWN;

struct IClassFactory : public IUnknown
{
    virtual HRESULT CreateInstance(IUnknown* outer, REFIID riid,
                                   void** ppv) = 0;
    virtual HRESULT LockServer(BOOL lock) = 0;
};


// __uuidof: MSVC ties a GUID to a type via __declspec(uuid). We drop those decls, so map __uuidof to GUID_NULL --
// the only Linux users of it are stubs/dead paths (the live COM is the Windows-only D3D backend).
extern "C" const GUID GUID_NULL_SHIM;
#define __uuidof(x) (GUID_NULL_SHIM)

#define CLSCTX_INPROC_SERVER 0x1
#define CLSCTX_INPROC_HANDLER 0x2
#define CLSCTX_LOCAL_SERVER 0x4
#define CLSCTX_ALL                                                             \
    (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER)
#define COINIT_APARTMENTTHREADED 0x2
#define COINIT_MULTITHREADED 0x0
#define RPC_E_CHANGED_MODE ((HRESULT)0x80010106L)
#define CLASS_E_NOAGGREGATION ((HRESULT)0x80040110L)
#define REGDB_E_CLASSNOTREG ((HRESULT)0x80040154L)

extern const IID IID_IUnknown;
extern const IID IID_IClassFactory;

// Co* : no COM runtime on Linux. Init calls succeed (no-op); object creation is not supported (a real WIC/DShow
// object never gets made -- the image loader is stb_image, the media path is stubbed).
static inline HRESULT CoInitialize(void*)
{
    return S_OK;
}
static inline HRESULT CoInitializeEx(void*, DWORD)
{
    return S_OK;
}
static inline void CoUninitialize(void)
{
}
static inline HRESULT CoCreateInstance(REFCLSID, IUnknown*, DWORD, REFIID,
                                       void** ppv)
{
    if (ppv)
        *ppv = nullptr;
    return E_NOINTERFACE;
}
static inline void* CoTaskMemAlloc(size_t n)
{
    return ::malloc(n);
}
static inline void CoTaskMemFree(void* p)
{
    ::free(p);
}
static inline HRESULT CoCreateGuid(GUID* g)
{
    if (g)
        memset(g, 0, sizeof(*g));
    return S_OK;
}

// SysAllocString family (BSTR) -- used by _bstr_t and a little COM string code. BSTR is a wchar-ish pointer.
typedef WCHAR* BSTR;

// ---- rich error info -------------------------------------------------------------------------------------------
// smart.h's CheckHR() fetches an IErrorInfo and throws _com_error; graphics/include/smart.h is pulled into 59 of the
// project's sources, so the whole graphics tree stopped at "unknown type name 'IErrorInfo'". None of it is live on
// Linux (the COM producers are the Windows-only D3D backends), but the DECLARATIONS have to exist for the throw
// expression to compile. GetErrorInfo therefore honestly reports "no info": there is no COM error object to fetch.
struct IErrorInfo : public IUnknown
{
    virtual HRESULT GetGUID(GUID* pGUID) = 0;
    virtual HRESULT GetSource(BSTR* pBstrSource) = 0;
    virtual HRESULT GetDescription(BSTR* pBstrDescription) = 0;
    virtual HRESULT GetHelpFile(BSTR* pBstrHelpFile) = 0;
    virtual HRESULT GetHelpContext(DWORD* pdwHelpContext) = 0;
};
inline HRESULT GetErrorInfo(unsigned long /*reserved*/,
                            IErrorInfo** ppErrorInfo)
{
    if (ppErrorInfo)
        *ppErrorInfo = 0;
    return S_FALSE;
} // S_FALSE == "no error object", the real Win32 answer
inline HRESULT SetErrorInfo(unsigned long /*reserved*/,
                            IErrorInfo* /*pErrorInfo*/)
{
    return S_OK;
}

// _com_error -- what CheckHR throws. Mirrors the MSVC comdef.h shape the catch sites use (Error()/ErrorMessage()).
// This is the SINGLE definition: on MSVC the class lives in <comdef.h>, but here <windows.h> pulls this header
// unconditionally, so a second copy in the shim's comdef.h collided with this one in every source that reached both
// (66 files). comdef.h now includes <windows.h> and adds only what is genuinely its own (_bstr_t/_variant_t).
class _com_error
{
public:
    explicit _com_error(HRESULT hr, IErrorInfo* perrinfo = 0,
                        bool /*fAddRef*/ = false)
        : m_hresult(hr), m_perrinfo(perrinfo)
    {
    }
    HRESULT Error() const
    {
        return m_hresult;
    }
    const char* ErrorMessage() const
    {
        return "COM error";
    } // no system error table on Linux
    IErrorInfo* ErrorInfo() const
    {
        return m_perrinfo;
    }
    WORD WCode() const
    {
        return 0;
    }
    // Source()/Description() come from the IErrorInfo the throw site passed. GetErrorInfo() on Linux always reports
    // "no error object", so these are honestly empty rather than pretending to carry a message.
    const char* Source() const
    {
        return "";
    }
    const char* Description() const
    {
        return "";
    }

private:
    HRESULT m_hresult;
    IErrorInfo* m_perrinfo;
};

static inline void _com_raise_error(HRESULT hr, IErrorInfo* perrinfo = 0)
{
    throw _com_error(hr, perrinfo);
}

static inline BSTR SysAllocString(const WCHAR*)
{
    return nullptr;
}
static inline void SysFreeString(BSTR)
{
}

#endif // FF_WIN32SHIM_COM_H
