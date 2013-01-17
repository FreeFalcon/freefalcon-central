#ifndef _WINDOWS_
#include <windows.h>
#endif
#include <assert.h>

#ifdef _DEBUG
#define wassert( string )	if (!(string)) { \
								MessageBox(NULL, "Break Program in debugger to see why.", "Assertion Failed", MB_OK); \
								exit(-1); \
							}
#else
#define wassert( string )
#endif
