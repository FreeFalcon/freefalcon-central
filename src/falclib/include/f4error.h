/***************************************************************************\
    f4error.h
    Scott Randolph
    June 15, 1995

    Macros for dealing with fatal errors at runtime.
\***************************************************************************/
#ifndef F4ERROR_H
#define F4ERROR_H

#include <windows.h>
#include <stdio.h>
#include <assert.h>

// Routine to Convert a Windows error number into a string
#define PutErrorString(buf)  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, \
                                           NULL, GetLastError(),        \
                                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
                                           buf, sizeof(buf), NULL)


// TerrainError is always kills the program even when not in debug mode.
#define F4Error( string )                                             \
{    \
 char buffer[80];    \
    \
 sprintf( buffer, "Error:  %0d  %s  %s", __LINE__, __FILE__, __DATE__ ); \
 MessageBox(NULL, buffer, string, MB_OK); \
 exit(EXIT_FAILURE);    \
}


// F4Assert compiles to code only when in debug mode.  Otherwise, the expression is not evaluated.
#ifdef _DEBUG

extern bool hard_crash;
extern bool asserts;

#define F4Assert( expr ) \
 if (asserts && !(expr)) { \
 char buffer[80];    \
 int choice = IDRETRY; \
 if (hard_crash) \
 *((unsigned int *) 0x00) = 0; \
 else \
 { \
 while (choice != IDIGNORE) { \
 sprintf( buffer, "Assertion at %0d  %s  %s", __LINE__, __FILE__, __DATE__ );\
 choice = MessageBox(NULL, buffer, "Failed:  " #expr,   \
 MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_TASKMODAL); \
 if (choice == IDABORT) { \
 exit(EXIT_FAILURE); \
 } \
 if (choice == IDRETRY) { \
 __asm int 3 \
 } \
 } \
 } \
 }

#define F4Warning( string )               \
{    \
 char buffer[80];    \
    \
 sprintf( buffer, "Error:  line %0d, %s on %s", __LINE__, __FILE__, __DATE__ ); \
 MessageBox(NULL, buffer, string, MB_OK); \
}

#else
#define F4Assert( expr )

#define F4Warning( string )

#endif

#endif
