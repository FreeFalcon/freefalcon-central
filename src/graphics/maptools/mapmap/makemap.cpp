//--------------------------------------

/*

  MakeMap - Terrain map tool

  Created by Erick Jap   November 5, 1996
  Copyright Spectrum Holobyte, Inc.

*/

//--------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>

//--------------------------------------

#define	HAS_DATA_DIMENSION		0x01
#define	HAS_WINDOW_DIMENSION		0x02

#define	HASH_ELEMENT_SIZE		4
#define	HASH_MAX_SIZE			16

//--------------------------------------

struct ElementList {
	int element, counter;
	ElementList *previous;
	ElementList *next;
};

ElementList *HashRow[HASH_MAX_SIZE], *HashCol[HASH_MAX_SIZE];

#pragma pack (push, 1)
struct PCXHEADER {
	char	manufacturer, version, encoding, bits_per_pixel;
	short	xmin, ymin, xmax, ymax, hres, vres;
	char  	palette[48], reserved, colour_planes;
	short 	bytes_per_line, palette_type;
	char  	filler[58];
};
#pragma pack (pop)

//--------------------------------------

void ConcatFileExtension (char *newfile, char *oldfile, char *ext);
void SavePCX (char *file);
int packImageRow (unsigned char *image, unsigned char *out, int total);
int countbyte (unsigned char *image, int width);
void DeleteHashTable (ElementList **hash);
void DuplicateHashTable (ElementList **hash, ElementList **duphash);
int InsertElement (ElementList **list, int element);
int DeleteElement (ElementList **list, int element);
void DuplicateListElement (ElementList **list, ElementList **duplist);
int CreateTileListElement (ElementList **list);
int InsertTileRowElement (ElementList **list, int counter, unsigned short *buff, int width);
int DeleteTileRowElement (ElementList **list, int counter, unsigned short *buff, int width);
int InsertTileColElement (ElementList **list, int counter, unsigned short *buff, int height);
int DeleteTileColElement (ElementList **list, int counter, unsigned short *buff, int height);

int	datawidth, dataheight;
int	windowwidth, windowheight;
unsigned short *databuffer;
unsigned char  *mapbuffer;
int totalimagebytes;

void main (int argc, char *argv[])
{
	if (argc < 2) {
		puts ("\nMakeMap v1.0 by Erick Jap\n");
		puts ("Usage: MakeMap mapfile -d width height -w width height [-oOutput]");
		puts ("where: mapfile is map file data to be used");
		puts ("       -oOutput use 'Output' as the output file");
		puts ("       -d width height indicate map dimension");
		puts ("       -w width height indicate sliding window dimension");
		puts ("Note: - Sliding window dimension must be less than the data dimension");
		exit (1);
	}
	int i, j, flag;

	char output[256];
	ConcatFileExtension (output, argv[1], "PCX");

	flag = 0;
	for (i=2;i < argc;i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'd' || argv[i][1] == 'D') {
				flag |= HAS_DATA_DIMENSION;
				datawidth = atoi(argv[++i]);
				dataheight = atoi(argv[++i]);
			}
			else if (argv[i][1] == 'w' || argv[i][1] == 'W') {
				flag |= HAS_WINDOW_DIMENSION;
				windowwidth = atoi(argv[++i]);
				windowheight = atoi(argv[++i]);
			}
			else if (argv[i][1] == 'o' || argv[i][1] == 'O') {
				ConcatFileExtension (output, &(argv[i][2]), "PCX");
			}
		}
	}

	if (!(flag & HAS_DATA_DIMENSION)) {
		puts ("You forget to enter the map dimension!");
		exit (1);
	}
	if (!(flag & HAS_WINDOW_DIMENSION)) {
		puts ("You forget to enter the sliding window dimension!");
		exit (1);
	}

	if (windowwidth > datawidth) {
		puts ("Sliding window width must be less than the width of the map!");
		exit (1);
	}
	if (windowheight > dataheight) {
		puts ("Sliding window height must be less than the height of the map!");
		exit (1);
	}

	if (datawidth & 1) {
		puts ("The width of the map must be an even number!");
		exit (1);
	}

	j = datawidth * dataheight;
	i = j << 1;
	mapbuffer = (unsigned char *) malloc (i+j);
	if (!mapbuffer) {
		printf ("Can not allocate memory to load map data!\n");
		exit (1);
	}
	databuffer = (unsigned short *) (mapbuffer + j);
	int infile = open (argv[1], O_BINARY | O_RDONLY);
	if (!infile) {
		printf ("Can not open map file %s\n", argv[1]);
		free (mapbuffer);
		exit (1);
	}
	read (infile, (char *) databuffer, i);
	close (infile);

	int numwidth = datawidth - windowwidth + 1;
	int numheight = dataheight - windowheight + 1;

	memset (HashRow, 0, HASH_MAX_SIZE * sizeof (ElementList *));
	int totaltile = CreateTileListElement (HashRow);
	unsigned short *buff = databuffer;
	unsigned char  *curmap = mapbuffer;
	int winheight;
	for (i=0; i< dataheight; i++) {
		if (i < (numheight - 1)) winheight = windowheight;
		else winheight = dataheight - i;
		DuplicateHashTable (HashRow, HashCol);
		int curtile = totaltile;
		for (j=0; j< datawidth; j++) {
			unsigned int k = 255;
			if (curtile <= 256) k = curtile-1;
			*curmap++ = k;
			if (j < (numwidth - 1)) {
				curtile = InsertTileColElement (HashCol, curtile, &(buff[j+windowwidth]), winheight);
				curtile = DeleteTileColElement (HashCol, curtile, &(buff[j]), winheight);
			}
			else {
				curtile = DeleteTileColElement (HashCol, curtile, &(buff[j]), winheight);
			}
		}
		if (i < (numheight - 1)) {
			totaltile = InsertTileRowElement (HashRow, totaltile, buff + datawidth*windowheight, windowwidth);
			totaltile = DeleteTileRowElement (HashRow, totaltile, buff, windowwidth);
		}
		else {
			totaltile = DeleteTileRowElement (HashRow, totaltile, buff, windowwidth);
		}
		buff += datawidth;
		DeleteHashTable (HashCol);
	}
	DeleteHashTable (HashRow);

	totalimagebytes = 0;
	curmap = mapbuffer;
	for (i=0; i< dataheight; i++) {
		unsigned char *databuf = (unsigned char *) databuffer;
		databuf += totalimagebytes;
		totalimagebytes = packImageRow (curmap, 
										(unsigned char *) databuf, 
										totalimagebytes);
		curmap += datawidth;
	}

	SavePCX (output);

	free (mapbuffer);
}

void ConcatFileExtension (char *newfile, char *oldfile, char *ext)
{
	char prevchar, currchar, nextchar;

	prevchar = '.';
	while (*oldfile != 0) {
		currchar = *oldfile++;
		nextchar = *oldfile;
		if (currchar == '.' && 
			((prevchar != '.' && prevchar != '\\') ||
			 (nextchar != '.' && nextchar != '\\'))) break;
		prevchar = currchar;
		*newfile++ = currchar;
	}
	*newfile++ = '.';
	while (*ext) *newfile++ = *ext++;
	*newfile = 0;
}

void SavePCX (char *file)
{
	PCXHEADER pcx;
	memset ((char *) &pcx, 0, sizeof (PCXHEADER));
	pcx.manufacturer = 10;
	pcx.version = 5;
	pcx.encoding = 1;
	pcx.bits_per_pixel = 8;
	pcx.xmin = 0;
	pcx.ymin = 0;
	pcx.xmax = datawidth - 1;
	pcx.ymax = dataheight - 1;
	pcx.hres = 640;
	pcx.vres = 480;
	pcx.colour_planes = 1;
	pcx.bytes_per_line = datawidth;
	pcx.palette[0] = (char) 0;
	pcx.palette[1] = (char) 0;
	pcx.palette[2] = (char) 0;
	pcx.palette[3] = (char) 0;
	pcx.palette[4] = (char) 0;
	pcx.palette[5] = (char) 0xaa;
	pcx.palette[6] = (char) 0;
	pcx.palette[7] = (char) 0xaa;
	pcx.palette[8] = (char) 0;
	pcx.palette[9] = (char) 0;
	pcx.palette[10] = (char) 0xaa;
	pcx.palette[11] = (char) 0xaa;
	pcx.palette[12] = (char) 0xaa;
	pcx.palette[13] = (char) 0;
	pcx.palette[14] = (char) 0;
	pcx.palette[15] = (char) 0xaa;
	pcx.palette[16] = (char) 0;
	pcx.palette[17] = (char) 0xaa;
	pcx.palette[18] = (char) 0xaa;
	pcx.palette[19] = (char) 0xaa;
	pcx.palette[20] = (char) 0;
	pcx.palette[21] = (char) 0xaa;
	pcx.palette[22] = (char) 0xaa;
	pcx.palette[23] = (char) 0xaa;
	pcx.palette[24] = (char) 0x55;
	pcx.palette[25] = (char) 0x55;
	pcx.palette[26] = (char) 0x55;
	pcx.palette[27] = (char) 0x55;
	pcx.palette[28] = (char) 0x55;
	pcx.palette[29] = (char) 0xff;
	pcx.palette[30] = (char) 0x55;
	pcx.palette[31] = (char) 0xff;
	pcx.palette[32] = (char) 0x55;
	pcx.palette[33] = (char) 0x55;
	pcx.palette[34] = (char) 0xff;
	pcx.palette[35] = (char) 0xff;
	pcx.palette[36] = (char) 0xff;
	pcx.palette[37] = (char) 0x55;
	pcx.palette[38] = (char) 0x55;
	pcx.palette[39] = (char) 0xff;
	pcx.palette[40] = (char) 0x55;
	pcx.palette[41] = (char) 0xff;
	pcx.palette[42] = (char) 0xff;
	pcx.palette[43] = (char) 0xff;
	pcx.palette[44] = (char) 0x55;
	pcx.palette[45] = (char) 0xff;
	pcx.palette[46] = (char) 0xff;
	pcx.palette[47] = (char) 0xff;

	char palette[769];
	int  i, j;
	palette[0] = 0xc;
	j = 1;
	for (i=0; i < 256; i++) {
		palette[j++] = i;
		palette[j++] = i;
		palette[j++] = i;
	}

	int outfile = open (file, O_BINARY | O_CREAT | O_RDWR, S_IWRITE);
	write (outfile, (char *) &pcx, sizeof (PCXHEADER));
	write (outfile, (char *) databuffer, totalimagebytes);
	write (outfile, (char *) &palette, 769);
}

int countbyte (unsigned char *image, int width)
{
	int i, j;
	unsigned char c = *image;
	j = 0;
	for (i=0; i < width; i++) {
		if (c != *image) break;
		j++;
		image++;
	}
	return j;
}

int packImageRow (unsigned char *image, unsigned char *out, int total)
{
	int	i, j, k;

	i = 0;
	while (i < datawidth) {
		j = countbyte (image, datawidth-i);
		unsigned char c = *image;
		if (j > 1) {
			k = j;
			while (k > 63) {
				*out++ = 0xff;
				*out++ = c;
				k -= 63;
				total += 2;
			}
			if (k > 0) {
				*out++ = k | 0xc0;
				*out++ = c;
				total += 2;
			}
		}
		else {
			*out++ = c;
			total++;
		}
		i += j;
		image += j;
	}
	return total;
}

int CreateTileListElement (ElementList **list)
{
	int i, j;
	int counter = 0;
	unsigned short *buff = databuffer;
	for (i=0;i < windowheight; i++) {
		for (j=0;j < windowwidth; j++) {
			counter += InsertElement (list, (unsigned int) buff[j]);
		}
		buff += datawidth;
	}
	return counter;
}

int InsertTileRowElement (ElementList **list, int counter, unsigned short *buff, int winwidth)
{
	int i;
	for (i=0;i < winwidth; i++) {
		counter += InsertElement (list, (unsigned int) buff[i]);
	}
	return counter;
}

int DeleteTileRowElement (ElementList **list, int counter, unsigned short *buff, int winwidth)
{
	int i;
	for (i=0;i < winwidth; i++) {
		counter -= DeleteElement (list, (unsigned int) buff[i]);
	}
	return counter;
}

int InsertTileColElement (ElementList **list, int counter, unsigned short *buff, int winheight)
{
	int i;
	for (i=0;i < winheight; i++) {
		counter += InsertElement (list, (unsigned int) *buff);
		buff += datawidth;
	}
	return counter;
}

int DeleteTileColElement (ElementList **list, int counter, unsigned short *buff, int winheight)
{
	int i;
	for (i=0;i < winheight; i++) {
		counter -= DeleteElement (list, (unsigned int) *buff);
		buff += datawidth;
	}
	return counter;
}

void DeleteHashTable (ElementList **hash)
{
	int i;
	for (i=0; i < HASH_MAX_SIZE;i++) {
		ElementList *listElement = hash[i];
		while (listElement) {
			ElementList *curList = listElement;
			listElement = listElement -> next;
			free (curList);
		}
	}
}

void DuplicateHashTable (ElementList **hash, ElementList **duphash)
{
	int i;
	for (i=0; i < HASH_MAX_SIZE;i++) {
		duphash[i] = 0;
		if (hash[i]) DuplicateListElement (&(hash[i]), &(duphash[i]));
	}
}

void DuplicateListElement (ElementList **list, ElementList **duplist)
{
	ElementList *listElement = *list;
	ElementList *curElement = 0;
	while (listElement) {
		ElementList *newList = (ElementList *) malloc (sizeof(ElementList));
		if (!newList) {
			puts ("Unable to allocate memory for a new element");
			exit (1);
		}
		newList -> element = listElement -> element;
		newList -> counter = listElement -> counter;
		newList -> next = 0;
		newList -> previous = curElement;
		if (curElement) curElement -> next = newList;
		else *duplist = newList;
		curElement = newList;
		listElement = listElement -> next;
	}
}

inline int GetHashIndex (int element)
{
	int hashindex = element >> HASH_ELEMENT_SIZE;
	if (hashindex >= HASH_MAX_SIZE) hashindex = HASH_MAX_SIZE-1;
	return hashindex;
}

int InsertElement (ElementList **list, int element)
{
	int hashindex = GetHashIndex (element);

	ElementList *listElement = list[hashindex];
	ElementList *prevElement = 0;
	while (listElement) {
		if (listElement -> element == element) {
			listElement -> counter++;
			return 0;
		}
		else if (listElement -> element > element) break;
		prevElement = listElement;
		listElement = listElement -> next;
	}
	ElementList *newElement = (ElementList *) malloc (sizeof(ElementList));
	if (!newElement) {
		puts ("Unable to allocate memory for a new element");
		exit (1);
	}
	newElement -> element = element;
	newElement -> counter = 1;
	newElement -> previous = 0;
	newElement -> next = 0;

	if (listElement) {
		if (prevElement) {
			newElement -> previous = prevElement;
			prevElement -> next = newElement;
		}
		else list[hashindex] = newElement;
		listElement -> previous = newElement;
		newElement -> next = listElement;
	}
	else {
		if (prevElement) {
			prevElement -> next = newElement;
			newElement -> previous = prevElement;
		}
		else list[hashindex] = newElement;
	}

	return 1;
}

int DeleteElement (ElementList **list, int element)
{
	int hashindex = GetHashIndex (element);

	ElementList *listElement = list[hashindex];
	while (listElement) {
		if (listElement -> element == element) {
			listElement -> counter--;
			if (listElement -> counter) return 0;
			else {
				if (listElement -> previous) {
					listElement -> previous -> next = listElement -> next;
					if (listElement -> next)
						listElement -> next -> previous = listElement -> previous;
				}
				else {
					list[hashindex] = listElement -> next;
					if (listElement -> next) listElement -> next -> previous = 0;
				}
				free (listElement);
				return 1;
			}
		}
		listElement = listElement -> next;
	}
	puts ("Unable to remove the element from the list");
	exit (1);
	return 0;
}

