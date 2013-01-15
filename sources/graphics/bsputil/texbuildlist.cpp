/***************************************************************************\
	TexBuildList.cpp
    Scott Randolph
    March 4, 1998

    Provides build time services for sharing textures among all objects.
\***************************************************************************/
#include <io.h>
#include "utils\lzss.h"
#include "PalBuildList.h"
#include "TexBuildList.h"
#include "GraphicsRes.h"

BuildTimeTextureList	TheTextureBuildList;



// To be conservative, we'll got twice the size of our biggest uncompressed texture
static const int	MAX_COMPRESSED_SIZE	= 256*256*2;



int BuildTimeTextureList::AddReference( char *filename )
{
	BuildTimeTextureEntry		*entry;
	int							index;

	// See if we've already got a matching texture to share
	index = 0;
	for (entry=head; entry; entry=entry->next) {
		if (stricmp( entry->filename, filename ) == 0) {
			// Found a match, so just return it's index
			ShiAssert( index == entry->index );
			entry->refCount++;
			return index;
		}
		index++;
	}

	// Setup a new entry for our list
	entry = new BuildTimeTextureEntry;
	ShiAssert( strlen(filename) < sizeof(entry->filename) );
	strcpy( entry->filename, filename );
	entry->index	= index;
	entry->refCount = 1;
	entry->next		= NULL;
	entry->prev		= tail;

	// Add the new entry to the list
	if (tail) {
		tail->next = entry;
		tail = entry;
	} else {
		head = tail = entry;
	}

	// Keep track of how many entries are in our linked list
	ShiAssert( index == listLen );
	listLen++;

	return index;
}


void BuildTimeTextureList::ConvertToAPL( int index )
{
	BuildTimeTextureEntry		*entry;
	char						*extension;

	for (entry=head; entry; entry=entry->next) {
		if ( entry->index == index ) {
			// Found a match, so break
			break;
		}
	}

	// We should only be called with indices already in the list
	ShiAssert(entry);

	// Find the three letter file name extension
	extension = strrchr( entry->filename, '.' );
	ShiAssert( extension );
	ShiAssert( strlen(extension) == 4 );

	// Whatever the extension was, hammer it to ".APL"
	extension[1] = 'A';
	extension[2] = 'P';
	extension[3] = 'L';
}


void BuildTimeTextureList::BuildPool()
{
	BuildTimeTextureEntry		*entry;
	DWORD						*palData;
	int							palID;

        ResInit (0);
	ResCreatePath ("d:\\source\\falcon4.orig\\graphics\\bspbuild", FALSE);
	// Create the empty texture pool
	TheTextureBank.Setup( listLen );

	// Walk through our list of textures copying their required data into the pool
	for (entry=head; entry; entry=entry->next) {

		// Set our reference count equal to the number of objects looking at us
		// (because they're all loaded right now anyway)
		TheTextureBank.TexturePool[entry->index].refCount = entry->refCount;

		// Load the texture (both to get its properties and because all the objects are also
		// loaded at this point).
		if (TheTextureBank.TexturePool[entry->index].tex.LoadImage( entry->filename, MPR_TI_PALETTE | MPR_TI_CHROMAKEY )) {
			// Now put our palette into the PaletteBank to enable sharing palettes
			palData = TheTextureBank.TexturePool[entry->index].tex.palette->paletteData;
			palID = ThePaletteBuildList.AddReference( palData );
		} else {
			printf( "WARNING!  Failed to read texture %s\n", entry->filename );
			fflush(NULL);
			palID = 0;	// Punt!  Might break somewhere else later (runtime?) but for now...
		}
		TheTextureBank.TexturePool[entry->index].palID = palID;
	}
}


void BuildTimeTextureList::WritePool( int file )
{
	printf("Writing texture headers\n");

	// Write the number of textures in the pool
	write( file, &TheTextureBank.nTextures, sizeof(TheTextureBank.nTextures) );

	// Write the size of the largest compressed texture in the pool
	//ShiAssert( maxCompressedTextureSize );
	write( file, &maxCompressedTextureSize, sizeof(maxCompressedTextureSize) );

	// Write our texture pool
	write( file, TheTextureBank.TexturePool, sizeof(*TheTextureBank.TexturePool)*TheTextureBank.nTextures );
}


void BuildTimeTextureList::WriteTextureData( int file )
{
	int		i;
	int		size;
	int		compressedSize;
	int		compressedOffset = 0;
	BYTE	*uncompressedBuffer;
	BYTE	*compressedBuffer;

	printf("Writing texture data\n");

	// Start looking for the maximum compressed size
	maxCompressedTextureSize = 0;

	// Allocate the compression buffer
	compressedBuffer = new BYTE[ MAX_COMPRESSED_SIZE ];

	// Visit each texture in turn and write its data to disk
	for (i=0; i<TheTextureBank.nTextures; i++) {

		ShiAssert( TheTextureBank.TexturePool[i].tex.imageData );

		// Compress the texture data
		uncompressedBuffer = (BYTE*)TheTextureBank.TexturePool[i].tex.imageData;
		size = TheTextureBank.TexturePool[i].tex.dimensions;
		size = size*size;
		compressedSize = LZSS_Compress( uncompressedBuffer, compressedBuffer, size );
		ShiAssert( compressedSize < MAX_COMPRESSED_SIZE );

		// Write the image data for this texture
		write( file, compressedBuffer, compressedSize );
	
		// Remember where we put it.
		TheTextureBank.TexturePool[i].fileOffset	= compressedOffset;
		TheTextureBank.TexturePool[i].fileSize		= compressedSize;

		maxCompressedTextureSize = max( maxCompressedTextureSize, compressedSize );

		compressedOffset += compressedSize;
	}

	// Free the compression buffer
	delete[] compressedBuffer;
}


void BuildTimeTextureList::Report()
{
	BuildTimeTextureEntry		*entry;

	printf("\nTexture Report (%0d textures)\n", TheTextureBank.nTextures);
	printf("  ID   Name                 PalID\n");

	// Walk through our list of textures and report which palette they used
	for (entry=head; entry; entry=entry->next) {
		printf(" %4d  %20s  %2d\n", 
			entry->index,
			entry->filename,
			TheTextureBank.TexturePool[entry->index].palID);
	}
}
