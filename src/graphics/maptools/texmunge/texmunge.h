/*******************************************************************\
	Simple tool to read in an 8 bit RAW texture template file and
	write out an 8 bit RAW texture ID map.  The input file uses a
	two by two pixel sqaure to encode the required texture at each
	post.  As a result, the input file is 2x in width and height as
	compared to the output file.

	Scott Randolph		February 22, 1996		Spectrum HoloByte
\*******************************************************************/
#ifndef _TEXMUNGE_H_
#define _TEXMUNGE_H_

#include <windows.h>


typedef struct PIXELCLUSTER
{
	BYTE		p1, p2, p3, p4;
	BYTE		c1, c2, c3, c4;
	BYTE		p1A, p2A, p3A, p4A;
	BYTE		p1B, p2B, p3B, p4B;
	BYTE		p1C, p2C, p3C, p4C;
} 	PIXELCLUSTER;


typedef struct codeListEntry {
	int				usageCount;
	char			name[20];
	WORD			id;
	WORD			code;
	codeListEntry	*prev;
	codeListEntry	*next;
} codeListEntry;


typedef struct setListEntry {
	int		setCode;
	int		numTiles;
	int		tileCode[16];
} setListEntry;


// Erick's worker function which decide which texture to apply.
WORD DecodeCluster( PIXELCLUSTER sourceSamples,int row, int col );

// Erick's function to convert from a code to a string name
void NumberToName( WORD k, char *name );


#endif // _TEXMUNGE_H_