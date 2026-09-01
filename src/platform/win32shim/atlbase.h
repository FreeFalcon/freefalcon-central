// Artscout - 2026 (#104, Linux Ф1): minimal <atlbase.h>. ATL is effectively dead in this codebase (CComPtr use
// count 0; the DDraw smart-pointers were purged). Provide trivial CComPtr/CComQIPtr (raw-pointer wrappers, no
// ref-counting) + CComModule/CComBSTR shells so the few includes compile. Linux-only.
#ifndef FF_WIN32SHIM_ATLBASE_H
#define FF_WIN32SHIM_ATLBASE_H
#include <windows.h>

template <class T> class CComPtr
{
public:
    T* p = nullptr;
    CComPtr()
    {
    }
    CComPtr(T* q) : p(q)
    {
    }
    ~CComPtr()
    {
        if (p)
            p->Release();
    }
    operator T*() const
    {
        return p;
    }
    T* operator->() const
    {
        return p;
    }
    T** operator&()
    {
        return &p;
    }
    T* operator=(T* q)
    {
        if (p)
            p->Release();
        p = q;
        return p;
    }
    bool operator!() const
    {
        return p == nullptr;
    }
    void Release()
    {
        if (p)
        {
            p->Release();
            p = nullptr;
        }
    }
};
template <class T, const IID* piid = nullptr> class CComQIPtr
{
public:
    T* p = nullptr;
    CComQIPtr()
    {
    }
    CComQIPtr(T* q) : p(q)
    {
    }
    operator T*() const
    {
        return p;
    }
    T* operator->() const
    {
        return p;
    }
    T* operator=(T* q)
    {
        p = q;
        return p;
    }
    bool operator!() const
    {
        return p == nullptr;
    }
};

class CComModule
{
public:
    HRESULT Init(void*, void*)
    {
        return S_OK;
    }
    void Term()
    {
    }
};
class CComBSTR
{
public:
    BSTR m_str = nullptr;
    CComBSTR()
    {
    }
    CComBSTR(const char*)
    {
    }
    operator BSTR() const
    {
        return m_str;
    }
};

#define USES_CONVERSION
#define A2W(x) ((WCHAR*)nullptr)
#define W2A(x) ((char*)nullptr)
#define T2A(x) (x)
#define A2T(x) (x)

#endif // FF_WIN32SHIM_ATLBASE_H
