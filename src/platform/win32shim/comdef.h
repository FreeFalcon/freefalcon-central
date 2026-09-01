// Artscout - 2026 (#104, Linux Ф1): <comdef.h> shim -- the COM error/string helpers. The live user is comsup.h's
// CheckHR (throws _com_error on a failed HRESULT) across ~19 files. _bstr_t / _variant_t are barely used and get
// minimal shells. Linux-only.
#ifndef FF_WIN32SHIM_COMDEF_H
#define FF_WIN32SHIM_COMDEF_H
#include <windows.h>
#include <string>

// _com_error / _com_raise_error are NOT declared here. <windows.h> (above) pulls win32_com.h, which defines them;
// a second copy in this header collided with that one in every source reaching both -- the single largest failure
// class in the survey (66 files). On MSVC <comdef.h> is their only home, but in this shim <windows.h> is the wider
// path, so win32_com.h owns the one definition and this header keeps only what is genuinely its own.

// _bstr_t / _variant_t: thin shells (the real COM-automation paths are Windows-only / stubbed).
class _bstr_t
{
public:
    _bstr_t()
    {
    }
    _bstr_t(const char* s) : m_s(s ? s : "")
    {
    }
    _bstr_t(const _bstr_t& o) : m_s(o.m_s)
    {
    }
    operator const char*() const
    {
        return m_s.c_str();
    }
    // MSVC's _bstr_t offers a NON-const `operator char*() const` as well, and comsup.h:185 leans on it: it writes
    // `(LPSTR) bstrDescription`, which no const char* conversion can satisfy (LPSTR is char*). Both conversions can
    // coexist without ambiguity -- a cast to char* has exactly one viable candidate. The const_cast is safe for the
    // same reason it is on Windows: the object owns this buffer, and the caller is handed its own string back.
    operator char*() const
    {
        return const_cast<char*>(m_s.c_str());
    }
    const char* operator=(const char* s)
    {
        m_s = s ? s : "";
        return m_s.c_str();
    }
    unsigned length() const
    {
        return (unsigned)m_s.length();
    }

private:
    std::string m_s;
};
class _variant_t
{
public:
    _variant_t()
    {
    }
    _variant_t(int v) : m_i(v)
    {
    }
    _variant_t(long v) : m_i((int)v)
    {
    }
    _variant_t(double v) : m_d(v)
    {
    }
    operator int() const
    {
        return m_i;
    }
    operator double() const
    {
        return m_d;
    }

private:
    int m_i = 0;
    double m_d = 0.0;
};

// _COM_SMARTPTR_TYPEDEF: the DirectDraw/D3D7 smart-pointer typedefs were purged; if any survives, alias to a raw
// pointer (no ref-counting) -- good enough to compile the (dead) declaration.
#ifndef _COM_SMARTPTR_TYPEDEF
#define _COM_SMARTPTR_TYPEDEF(Interface, IID) typedef Interface* Interface##Ptr
#endif

#endif // FF_WIN32SHIM_COMDEF_H
