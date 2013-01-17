#ifndef __VRLIB_H__
#define __VRLIB_H__ 

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VRLIB_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VRLIB_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef VRLIB_EXPORTS
#define VRLIB_API __declspec(dllexport)
#else
#define VRLIB_API __declspec(dllimport)
#endif

VRLIB_API bool InitRecoEngine();
VRLIB_API int  GetRecoEvent( LPSTR apEventStr, int aMsecTimeout );

VRLIB_API void StreamTopics();

VRLIB_API bool EnableTopic( int aTopicId, bool aEnable );
VRLIB_API void EnableTopics( bool aEnable );

VRLIB_API int CompileTopic( LPCSTR aTopicName, bool aEnable );
VRLIB_API int CompileTopic( int aTopicId, bool aEnable );
VRLIB_API int CompileTopics( bool aEnable, LPCSTR apPath );

#define EVENT_BUFF_SIZE 255

const int VR_ENGINE_ERROR = -2;
const int VR_UNABLE_TO_RECONIZE = -3;
const int VR_IDLE = -1;

#endif
