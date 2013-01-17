/***************************************************************************\
	PalBuildList.cpp
    Scott Randolph
    March 9, 1998

    Provides build time services for sharing palettes among all objects.
\***************************************************************************/
#include <io.h>
#include "PalBank.h"
#include "TexBuildList.h"
#include "PalBuildList.h"


BuildTimePaletteList	ThePaletteBuildList;


int BuildTimePaletteList::AddReference( DWORD *palData )
{
	BuildTimePaletteEntry		*entry;
	int							index;
	int							i;

	ShiAssert( palData );

	// See if we've already got a matching palette to share
	index = 0;
	for (entry=head; entry; entry=entry->next) {
		
		// Compare all entries
		for (i=0; i<256; i++) {
			if (palData[i] != entry->palData[i]) {
				break;
			}
		}

		if (i == 256) {
			// Found a match, so just return it's index
			ShiAssert( index == entry->index );
			entry->refCount++;
			return index;
		}
		index++;
	}

	// Setup a new entry for our list
	entry = new BuildTimePaletteEntry;
	memcpy( entry->palData, palData, 256*sizeof(*palData) );
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


void BuildTimePaletteList::BuildPool(void)
{
	BuildTimePaletteEntry		*entry;
	int							texID;
	int							palID;


	// Create the empty palette pool
	ThePaletteBank.Setup( listLen );

	// Copy all the palette data into the palette bank
	for (entry=head; entry; entry=entry->next) {

		// Setup the new palette (this will start us with one extra reference, which we _want_)
		ShiAssert( ThePaletteBank.IsValidIndex( entry->index ) );
		ThePaletteBank.PalettePool[entry->index].Setup32( entry->palData );

	}

	// Walk through our list of textures discarding their private palettes and
	// pointing them to ThePaletteBank
	for (texID=0; texID<TheTextureBank.nTextures; texID++) {

		// Discard the private palette (if we don't have one, its cause the texture didn't load)
		if (TheTextureBank.TexturePool[texID].tex.palette) {
			TheTextureBank.TexturePool[texID].tex.palette->Release();
		}

		// Point into ThePaletteBank
		palID = TheTextureBank.TexturePool[texID].palID;
		ShiAssert( ThePaletteBank.IsValidIndex( palID ) );
		TheTextureBank.TexturePool[texID].tex.palette = &ThePaletteBank.PalettePool[palID];
		TheTextureBank.TexturePool[texID].tex.palette->Reference();
	}
}


void BuildTimePaletteList::WritePool( int file )
{
	printf("Writing palette pool\n");

	// Write how many palettes we have
	write( file, &ThePaletteBank.nPalettes, sizeof(ThePaletteBank.nPalettes) );

	// Now write the data for each palette
	write( file, ThePaletteBank.PalettePool, sizeof(*ThePaletteBank.PalettePool)*ThePaletteBank.nPalettes );
}


void BuildTimePaletteList::Report()
{
	printf("PALETTEBANK:	There were %0d palettes used\n", ThePaletteBank.nPalettes);
}

void BuildTimePaletteList::MergePalette()
{
    int i;
    int obj = 1;
    for (i = 0; i < PaletteBankClass::nPalettes; i++)
	AddReference (PaletteBankClass::PalettePool[i].paletteData);
}
