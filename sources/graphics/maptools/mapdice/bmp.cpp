/***************************************************************************\
    BMP
    Scott Randolph
    October 27, 1995

    Opens and reads a 24 bit Windows BMP file and provides a pointer to the
	RGB data.  The memory allocated by the BMPread() call must be freed by a
	call to BMPfree().
\***************************************************************************/
#include "F4Error.h"
#include "BMP.h"


void  BMPfree( void *buffer )
{
	free( buffer );
}


void *BMPread( const char *filename, BITMAPINFO *info, BOOL packLines )
{
	HANDLE				file;
	BITMAPFILEHEADER	fileHeader;
	void				*bitBuffer;
	DWORD				bytesRead;
	DWORD				lineSize;


	// Open the named file
    file = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (file == INVALID_HANDLE_VALUE) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Failed to open %s - disk error?", string, filename );
		F4Error( message );
    }


	// Read the file header
	if ( !ReadFile( file, &fileHeader, sizeof(fileHeader), &bytesRead, NULL) ) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Failed to read BMP header - disk error?", string );
		F4Error( message );
	}
	if ( fileHeader.bfType != ('B' | ('M'<< 8)) ) {
		F4Error( "Invalid BMP file header" );
	}


	// Read the bitmap header
	if ( !ReadFile( file, &info->bmiHeader, sizeof(info->bmiHeader), &bytesRead, NULL) ) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Failed to read bitmap info - disk error?", string );
		F4Error( message );
	}
	if ( info->bmiHeader.biBitCount != 24 ) {	
		F4Error( "BMPread() only supports 24 bit BMP files." );
	}
	if ( info->bmiHeader.biCompression != BI_RGB ) {	
		F4Error( "BMPread() only supports uncompressed BMP files." );
	}

	
	// Allocate memory for the BMP pixel data
	lineSize = 3 * info->bmiHeader.biWidth;
	if (!packLines) {
		if (lineSize % 4) {
			lineSize += 4 - (lineSize % 4);
		}
	}
	bitBuffer = malloc( lineSize * info->bmiHeader.biHeight );
	if( !bitBuffer ) {
		F4Error( "Failed to allocate memory for the BMP data" );
	}


	// Read in the BMP data a scan line at a time packing as we go
	BYTE *bitPointer = (BYTE*)bitBuffer;
	for (int row = 0; row < info->bmiHeader.biHeight; row++ ) {

		if ( !ReadFile( file, bitPointer, lineSize, &bytesRead, NULL) ) {
			char	string[80];
			char	message[120];
			PutErrorString( string );
			sprintf( message, "%s:  Failed to read bitmap pixels - disk error?", string );
			F4Error( message );
		}

		bitPointer += lineSize;

		if (lineSize % 4) {
			SetFilePointer( file, 4 - (lineSize % 4), NULL, FILE_CURRENT );
		}
	}

	
	// Close the file we read from
	CloseHandle( file );

	// Return a pointer to the data
	return bitBuffer;
}
