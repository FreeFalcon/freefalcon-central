/***************************************************************************\
    BMP
    Scott Randolph
    October 27, 1995

    Opens and reads a 24 bit Windows BMP file and provides a pointer to the
	RGB data.  The memory allocated by the BMPread() call must be freed by a
	call to BMPfree().
\***************************************************************************/
#ifndef _BMP_H_
#define _BMP_H_

#include <windows.h>

void *BMPread( const char *filename, BITMAPINFO *info, BOOL packLines );
void  BMPfree( void *buffer );

#endif // _BMP_H_
