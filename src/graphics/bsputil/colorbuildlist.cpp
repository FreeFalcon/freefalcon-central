/***************************************************************************\
	ColorBuildList.cpp
    Scott Randolph
    February 17, 1998

    Provides build time services for sharing colors among all objects.
\***************************************************************************/
#include <io.h>
#include "StateStack.h"
#include "ColorBuildList.h"


BuildTimeColorList	TheColorBuildList;


void BuildTimeColorList::AddReference( int *target, Pcolor color, BOOL preLit )
{
	BuildTimeColorEntry		*entry;
	BuildTimeColorReference	*ref;

	// See if we've already got a matching color to share
	for (entry=head; entry; entry=entry->next) {
		if ((entry->color.r == color.r) && 
			(entry->color.g == color.g) &&
			(entry->color.b == color.b) &&
			(entry->color.a == color.a) &&
			(entry->preLit == preLit)) {

			// Found a match, so add our reference to it and quit
			for (ref=entry->refs; ref->next; ref=ref->next);
			ref->next = new BuildTimeColorReference;
			ref = ref->next;
			ref->target	= target;
			ref->next	= NULL;
			*target = -1;
			return;
		}
	}

	// Setup a new entry for our list
	entry = new BuildTimeColorEntry;
	entry->color	= color;
	entry->preLit	= preLit;
	entry->index	= -1;
	entry->refs		= new BuildTimeColorReference;
	entry->refs->target	= target;
	entry->refs->next	= NULL;
	entry->next		= NULL;
	entry->prev		= tail;
	*target = -1;

	// Add the new entry to the list
	if (tail) {
		tail->next = entry;
		tail = entry;
	} else {
		head = tail = entry;
	}

	// Keep a count of how many colors we've got
	numColors++;
	if (preLit) {
		numPrelitColors++;
	}
}


void BuildTimeColorList::AddReference( int *target, int *source )
{
	BuildTimeColorEntry		*entry;
	BuildTimeColorReference	*ref;

	// Find the reference to the source color to which we want another reference
	for (entry=head; entry; entry=entry->next) {
		for (ref=entry->refs; ref; ref=ref->next) {
			if (ref->target == source) {
				break;
			}
		}
		if (ref) {
			break;
		}
	}

	if (entry) {
		// Found a match, so add our reference to it and quit
		for (ref=entry->refs; ref->next; ref=ref->next);
		ref->next = new BuildTimeColorReference;
		ref = ref->next;
		ref->target	= target;
		ref->next	= NULL;
		*target = -1;
		return;
	} else {
		ShiError( "Asked to copy a color which wasn't found in the list" );
	}
}


void BuildTimeColorList::BuildPool()
{
	Pcolor					*preLitPtr;
	Pcolor					*nonPreLitPtr;

	Pcolor					*originalPtr;
	Pcolor					*end;

	Pcolor					*darkened;
	Pcolor					*greenTV;
	Pcolor					*greenIR;

	BuildTimeColorReference	*ref;
	BuildTimeColorEntry		*entry = head;


	// Construct the color arrays and get pointers into it
	TheColorBank.Setup( numColors, numPrelitColors );
	preLitPtr	 = TheColorBank.ColorBuffer;
	nonPreLitPtr = TheColorBank.ColorBuffer+numPrelitColors;
	ShiAssert( preLitPtr );
	ShiAssert( nonPreLitPtr );

	// Copy the colors into their appropriate slots
	while (entry) {
		if (entry->preLit) {
			entry->index = preLitPtr-TheColorBank.ColorBuffer;
			*preLitPtr++ = entry->color;
		} else {
			entry->index = nonPreLitPtr-TheColorBank.ColorBuffer;
			*nonPreLitPtr++ = entry->color;
		}

		// Resolve the dangling color references we've accumulated
		for(ref = entry->refs; ref; ref=ref->next) {
			*ref->target = entry->index;
		}

		// Move to the next LOD record in the list
		entry = entry->next;
	}

	// Now construct the other three versions of the colors (lit, green, and unlit green)
	originalPtr = TheColorBank.ColorBuffer; 
	end			= originalPtr + numColors;
	darkened	= TheColorBank.DarkenedBuffer;
	greenTV		= TheColorBank.GreenTVBuffer;
	greenIR		= TheColorBank.GreenIRBuffer;
	while (originalPtr < end) {
		*darkened = *originalPtr;
		greenTV->g = originalPtr->g;
		greenTV->a = originalPtr->a;
		greenTV->r = 0.0f;
		greenTV->b = 0.0f;
		*greenIR = *greenTV;
		originalPtr++;
		darkened++;
		greenTV++;
		greenIR++;
	}
}


void BuildTimeColorList::WritePool( int file )
{
	int		result;

	printf("Writing color pool\n");

	// Now we store our total color and darkened color count
	result = write( file, &TheColorBank.nColors,			sizeof(TheColorBank.nColors) );
	result = write( file, &TheColorBank.nDarkendColors,	sizeof(TheColorBank.nDarkendColors) );

	// Finally, store our color array
	result = write( file, TheColorBank.ColorBuffer, TheColorBank.nColors*sizeof(*TheColorBank.ColorBuffer) );
	if (result < 0 ) {
		perror( "Writing color pool" );
	}
}


void BuildTimeColorList::Report(void)
{
	printf("\n");
	printf("Total Color Entries        %6d\n", TheColorBank.nColors);
	printf("prelit color entries       %6d\n", TheColorBank.nDarkendColors);
}


void BuildTimeColorList::MergeColors()
{
    int i;
    int obj = 1;
    for (i = 0; i < ColorBankClass::nColors; i++)
	AddReference (&obj, ColorBankClass::ColorBuffer[i], 0);
}
