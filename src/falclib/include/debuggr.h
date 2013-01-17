#ifndef _DEBUG_GRAPHICS_H
#define _DEBUG_GRAPHICS_H

#define DEBUGGER_TEXT_MODE     0
#define DEBUGGER_GRAPHICS_MODE 1
#define MONO_NORMAL  0x07                  
#define MONO_INTENSE 0x08
#define MONO_UNDER   0x01
#define MONO_REVERSE 0x70
#define MONO_BLINK   0x80

#ifdef __cplusplus   /* support C++ */
extern "C" {
#endif
extern void InitDebug (int mode);
extern void DisplayDebugCharacter (int num, int x, int y);
extern void DrawDebugString (float x, float y, char * str);
extern void DrawDebugStringLeft (float x, float y, char * str);
extern void DrawDebugStringRight (float x, float y, char * str);
extern void RotateDebug2D(float angle);
extern void ResetRotateDebug2D (void);
extern void TranslateDebug2D(float x, float y);
extern void ResetTranslateDebug2D (void);
extern void SetDebugLineStyle (int style);
extern void DisplayDebugString (int row, int col, char *str);
extern void DrawDebugLine (float x1in, float y1in, float x2in, float y2in);
extern void DrawDebugCircle (float x, float y, float radius);
extern void DisplayDebugLine (int x0, int y0, int x1, int y1);
extern void DisplayDebugCircle (int x0, int y0, int radius);
extern void DebugSwapbuffer(void);
extern void DebugClear (void);
extern void MonoPrint( char *, ...);
extern void MonoLocate( unsigned char x, unsigned char y );
extern void MonoGetLoc (int* x, int* y);
extern void MonoCls( void );
extern void MonoScroll( void );
extern void MonoColor( char attribute );
extern void WriteDebugPixel (int, int);

//MI added for simple output
void WriteToFile (char *);

#ifdef __cplusplus
};
#endif

#endif

