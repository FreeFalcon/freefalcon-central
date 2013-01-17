
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NVDXTDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NVDXTDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef NVDXTDLL_EXPORTS
 #ifdef NVDXTDLL
  #define NVDXTDLL_API __declspec(dllexport) 
#else
  #define NVDXTDLL_API 
#endif
#else
 #ifdef NVDXTDLL
   #define NVDXTDLL_API __declspec(dllimport)
  #else
   #define NVDXTDLL_API 
  #endif
#endif

/*// This class is exported from the nvdxtdll.dll
class NVDXTDLL_API CNvdxtdll 
{
public:
	CNvdxtdll(void);
	// TODO: add your methods here.
};

extern NVDXTDLL_API int nNvdxtdll;

NVDXTDLL_API int fnNvdxtdll(void);
*/
