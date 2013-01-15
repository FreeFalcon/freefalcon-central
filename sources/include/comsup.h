// ComSup.h : header file
//

#pragma once
#include <comdef.h>
#include <atlbase.h>
#include <comcat.h>
#include <string>

inline void CheckHR(HRESULT hr);

//inline void CheckHR(HRESULT hr)
//{
//	//if(FAILED(hr))
//	//{
//	//	IErrorInfo *pEI = NULL;
//	//	::GetErrorInfo(NULL, &pEI);
//	//	throw _com_error(hr, pEI);
//	//}
//}

namespace ComSup
{
	template<class T>
	class SingleAutoLock
	{
		T *m_pT;
		bool m_bAquired;

		public:
		SingleAutoLock(T *pT, bool bInitialLock = true)
		{
			ATLASSERT(pT);
			m_pT = pT;
			m_bAquired = false;
			if(bInitialLock) Lock();
		}

		~SingleAutoLock()
		{
			Unlock();
		}

		void Lock()
		{
			m_pT->Lock();
			m_bAquired = true;
		}

		void Unlock()
		{
			if(m_bAquired)
			{
				m_pT->Unlock();
				m_bAquired = false;
			}
		}
	};

	#ifdef _WIN32_DCOM

	// Dynamically link to functions to avoid dll import errors on 95/98
	inline HRESULT CoInitializeEx(void * pvReserved, DWORD dwCoInit)
	{
		HRESULT (__stdcall *pprocCoInitializeEx)(void * pvReserved, DWORD dwCoInit) = 
			(HRESULT (__stdcall *)(void * pvReserved, DWORD dwCoInit))
			GetProcAddress(GetModuleHandle("ole32.dll"), "CoInitializeEx");

		if(pprocCoInitializeEx) return pprocCoInitializeEx(pvReserved, dwCoInit);
		else return E_NOTIMPL;
	}
 
 	inline HRESULT CoInitializeSecurity( PSECURITY_DESCRIPTOR pVoid, DWORD cAuthSvc, 
		SOLE_AUTHENTICATION_SERVICE *asAuthSvc, void * pReserved1, DWORD dwAuthnLevel, 
		DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities, 
		void * pvReserved2)
	{
		HRESULT (__stdcall *pprocCoInitializeSecurity)(PSECURITY_DESCRIPTOR, DWORD,
			SOLE_AUTHENTICATION_SERVICE *, void *, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE,
			DWORD, void *) = (HRESULT (__stdcall *)(PSECURITY_DESCRIPTOR, DWORD,
			SOLE_AUTHENTICATION_SERVICE *, void *, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE,
			DWORD, void *)) GetProcAddress(GetModuleHandle("ole32.dll"),
			"CoInitializeSecurity");

		if(pprocCoInitializeSecurity) return pprocCoInitializeSecurity(pVoid, cAuthSvc, 
			asAuthSvc, pReserved1, dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities, 
			pvReserved2);
		else return E_NOTIMPL;
	} 

	inline HRESULT CoCreateInstanceEx(REFCLSID rclsid, IUnknown * punkOuter, DWORD dwClsCtx, 
		COSERVERINFO* pServerInfo, ULONG cmq, MULTI_QI *rgmqResults)
	{
		HRESULT (__stdcall *pprocCoCreateInstanceEx)(REFCLSID, IUnknown *, DWORD,
			COSERVERINFO*, ULONG, MULTI_QI*) = (HRESULT (__stdcall *)(REFCLSID, IUnknown *,
			DWORD, COSERVERINFO*, ULONG, MULTI_QI*)) GetProcAddress(GetModuleHandle("ole32.dll"),
			"CoCreateInstanceEx");

		if(pprocCoCreateInstanceEx) return pprocCoCreateInstanceEx(rclsid, punkOuter, dwClsCtx, 
			pServerInfo, cmq, rgmqResults);
		else return E_NOTIMPL;
	}

	#endif // _WIN32_DCOM
 
	 inline HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
	{
		ICatRegister* pcr = NULL;
		HRESULT hr = S_OK ;
		hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**) &pcr);
		if(FAILED(hr)) return hr;
		CATEGORYINFO catinfo;
		catinfo.catid = catid;
		catinfo.lcid = 0x409; // english
		int len = wcslen(catDescription);
		if(len>127) len = 127;
		wcsncpy(catinfo.szDescription, catDescription, len);
		catinfo.szDescription[len] = '\0';
		hr = pcr->RegisterCategories(1, &catinfo);
		pcr->Release();
		return hr;
	}

	inline HRESULT RegisterServer(HINSTANCE hInst)
	{
		if(!hInst) return E_FAIL;
	
		HRESULT (__stdcall *pprocDLLRegisterServer)() = (HRESULT (__stdcall *)())
			GetProcAddress(hInst, "DllRegisterServer");
		if(!pprocDLLRegisterServer) return -2;
		return pprocDLLRegisterServer();
	}
	
	inline HRESULT RegisterServer(LPCTSTR lpszServername)
	{
		HINSTANCE hInst = LoadLibrary(lpszServername);	// retry
		if(!hInst) return E_FAIL;
		HRESULT hr = RegisterServer(hInst);
		FreeLibrary(hInst);
		return hr;
	}

	inline HRESULT UnregisterServer(HINSTANCE hInst)
	{
		if(!hInst) return E_FAIL;
	
		HRESULT (__stdcall *pprocDLLUnregisterServer)() = (HRESULT (__stdcall *)())
			GetProcAddress(hInst, "DllUnregisterServer");
		if(!pprocDLLUnregisterServer) return -2;
		return pprocDLLUnregisterServer();
	}
	
	inline HRESULT UnregisterServer(LPCTSTR lpszServername)
	{
		HINSTANCE hInst = LoadLibrary(lpszServername);	// retry
		if(!hInst) return E_FAIL;
		HRESULT hr = UnregisterServer(hInst);
		FreeLibrary(hInst);
		return hr;
	}

	inline void ReportException(_com_error &e)
	{
		std::string strMsg("Error: ");
		_bstr_t bstrDescription = e.ErrorMessage();
		if((LPSTR) bstrDescription) strMsg += (LPSTR) bstrDescription;
		else if(e.ErrorMessage()) strMsg += e.ErrorMessage();
		else strMsg += "Unspecified Error.";
		::MessageBox(NULL, strMsg.c_str(), "Information", MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION);
	}

	// returns an ANSI string from a CLSID or GUID
	inline int GUIDtoString(REFIID riid, LPSTR pszBuf)
	{
		return wsprintf((char *) pszBuf, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", riid.Data1,
			riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2],
			riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
	}

} // namespace IECOMSUP
