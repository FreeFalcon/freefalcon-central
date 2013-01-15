/***************************************************************************\
    ShiError.h
    Scott Randolph
    June 15, 1995

    Macros for dealing with fatal errors at runtime.
\***************************************************************************/
#ifndef SHIERROR_H
#define SHIERROR_H

#include <windows.h>
#include <stdio.h>
#include <assert.h>

#ifndef USE_SH_POOLS
#include <stdlib.h>		// OW needed for exit()
#endif

// Routine to Convert a Windows error number into a string
#define PutErrorString(buf)  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,					\
                                           NULL, GetLastError(),       					\
                                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	\
                                           buf, sizeof(buf), NULL)


// ShiError is always kills the program even when not in debug mode.
#define ShiError( string )                                            				\
{																		  			\
	char	buffer[580];													  			\
																		  			\
	sprintf( buffer, "Error:  %0d  %s  %s", __LINE__, __FILE__, __DATE__ );			\
	MessageBox(NULL, buffer, string, MB_OK);										\
	exit(-1);															  			\
}

// ShiAssert compiles to code only when in debug mode.  Otherwise, the expression is not evaluated.
#ifdef _DEBUG

extern int shiAssertsOn,shiHardCrashOn;

#define ShiAssert( expr )																	\
	if (shiAssertsOn && !(expr)) {	/*														\
	    static int	skipThisOne = FALSE;													\
																							\
		if (!skipThisOne) {																	\
			char	buffer[580];															  	\
			int		choice = IDRETRY;														\
																							\
			if (shiHardCrashOn)																\
				*((unsigned int *) 0x00) = 0;												\
			else {																			\
				sprintf( buffer, "Assertion at %0d  %s  %s\n", __LINE__, __FILE__, __DATE__ );\
				OutputDebugString(buffer);\
				if(strlen(buffer)) buffer[strlen(buffer) -1] = '\0';\
				choice = MessageBox(NULL, buffer, "Failed:  " #expr,		 				\
									MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_TASKMODAL);		\
				if (choice == IDABORT) {													\
					exit(-1);																\
				} else if (choice == IDRETRY) {												\
					DebugBreak();										\
				} else if (choice == IDIGNORE) {											\
					skipThisOne = TRUE;														\
				}																			\
			}																				\
		}	*/																				\
	}

#define ShiWarning( string )                                            			\
/*{																		  			\
	char	buffer[580];													  			\
																		  			\
	sprintf( buffer, "Error:  line %0d, %s on %s", __LINE__, __FILE__, __DATE__ );	\
	MessageBox(NULL, buffer, string, MB_OK);										\
}*/

#define ShiSetAsserts( expr )														\
{																					\
	shiAssertsOn = expr;															\
}

#define ShiSetHardCrash( expr )														\
{																					\
	shiHardCrashOn = expr;															\
}																					\

#else
#define ShiAssert( expr )

#define ShiWarning( string )

#define ShiSetAsserts( expr )														

#define ShiSetHardCrash( expr )

#endif

#endif