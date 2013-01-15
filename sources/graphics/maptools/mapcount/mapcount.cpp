//--------------------------------------

/*

  MapTool - Terrain map tool

  Created by Erick Jap   October 21, 1996
  Copyright Spectrum Holobyte, Inc.

*/

//--------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

//--------------------------------------

#define	HAS_DATA_DIMENSION		0x01
#define	HAS_WINDOW_DIMENSION	0x02

#define	HAS_MAX_TILE			0x10
#define	HAS_MAX_SET				0x20
#define	CALCULATE_COORD_TILE	0x40
#define	CALCULATE_COORD_SET		0x80
#define	HAS_QUERY				0xf0

#define	HASH_ELEMENT_SIZE		4
#define	HASH_MAX_SIZE			16

//--------------------------------------

struct ElementList {
	int element, counter;
	ElementList *previous;
	ElementList *next;
};

ElementList *HashRow[HASH_MAX_SIZE], *HashCol[HASH_MAX_SIZE];

//--------------------------------------

void DeleteHashTable (ElementList **hash);
void DuplicateHashTable (ElementList **hash, ElementList **duphash);
int InsertElement (ElementList **list, int element);
int DeleteElement (ElementList **list, int element);
void DuplicateListElement (ElementList **list, ElementList **duplist);
int CreateTileListElement (ElementList **list);
int InsertTileRowElement (ElementList **list, int counter, unsigned short *buff);
int DeleteTileRowElement (ElementList **list, int counter, unsigned short *buff);
int InsertTileColElement (ElementList **list, int counter, unsigned short *buff);
int DeleteTileColElement (ElementList **list, int counter, unsigned short *buff);
int CreateSetListElement (ElementList **list);
int InsertSetRowElement (ElementList **list, int counter, unsigned short *buff);
int DeleteSetRowElement (ElementList **list, int counter, unsigned short *buff);
int InsertSetColElement (ElementList **list, int counter, unsigned short *buff);
int DeleteSetColElement (ElementList **list, int counter, unsigned short *buff);

int	datawidth, dataheight;
int	windowwidth, windowheight;
unsigned short *databuffer;

void main (int argc, char *argv[])
{
	if (argc < 2) {
		puts ("\nMapTool v1.0 by Erick Jap\n");
		puts ("Usage: MapTool mapfile -d width height -w width height [-t tileno] [-s setno]");
		puts ("where: mapfile is map file data to be used");
		puts ("       -d width height indicate map dimension");
		puts ("       -w width height indicate sliding window dimension");
		puts ("       -t tileno --> return area with >= than tileno tiles");
		puts ("       -s setno  --> return area with >= than setno sets\n");
		puts ("Note: - Sliding window dimension must be less than the data dimension");
		puts ("      - if tileno is 0, calculate coordinate of area with max number of tiles");
		puts ("      - if setno is 0, calculate coordinate of area with max number of sets\n");
		exit (1);
	}
	int i, j, flag;
	int	maxtileno, maxsetno;

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
			else if (argv[i][1] == 't' || argv[i][1] == 'T') {
				maxtileno = atoi(argv[++i]);
				if (maxtileno) flag |= HAS_MAX_TILE;
				else flag |= CALCULATE_COORD_TILE;
			}
			else if (argv[i][1] == 's' || argv[i][1] == 'S') {
				maxsetno = atoi(argv[++i]);
				if (maxsetno) flag |= HAS_MAX_SET;
				else flag |= CALCULATE_COORD_SET;
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

	if (!(flag & HAS_QUERY)) {
		puts ("What do you want?");
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

	i = datawidth * dataheight * 2;
	databuffer = (unsigned short *) malloc (i);
	if (!databuffer) {
		printf ("Can not allocate memory to load map data!\n");
		exit (1);
	}
	int infile = open (argv[1], O_BINARY | O_RDONLY);
	if (!infile) {
		printf ("Can not open map file %s\n", argv[1]);
		free (databuffer);
		exit (1);
	}
	read (infile, (char *) databuffer, i);
	close (infile);

	unsigned short *buff;
	int numwidth = datawidth - windowwidth + 1;
	int numheight = dataheight - windowheight + 1;

	if ((flag & HAS_MAX_TILE) || (flag & CALCULATE_COORD_TILE)) {
		int	savetrow, savetcol, totaltile, curtile, maxtile;

		memset (HashRow, 0, HASH_MAX_SIZE * sizeof (ElementList *));
		totaltile = CreateTileListElement (HashRow);
		maxtile = 0;
		buff = databuffer;
		for (i=0; i< numheight; i++) {
			DuplicateHashTable (HashRow, HashCol);
			curtile = totaltile;
			for (j=0; j< numwidth; j++) {
				if (flag & HAS_MAX_TILE) {
					if (curtile >= maxtileno) {
						printf ("TILE(%d) - %d %d\n", curtile, i, j);
					}
				}
				if (flag & CALCULATE_COORD_TILE) {
					if (curtile > maxtile) {
						maxtile = curtile;
						savetrow = i;
						savetcol = j;
					}
				}
				if (j < (numwidth - 1)) {
					curtile = InsertTileColElement (HashCol, curtile, &(buff[j+windowwidth]));
					curtile = DeleteTileColElement (HashCol, curtile, &(buff[j]));
				}
			}
			if (i < (numheight - 1)) {
				totaltile = InsertTileRowElement (HashRow, totaltile, buff + datawidth*windowheight);
				totaltile = DeleteTileRowElement (HashRow, totaltile, buff);
				buff += datawidth;
			}
			DeleteHashTable (HashCol);
		}
		if (flag & CALCULATE_COORD_TILE)
			printf ("MaxTile (%d) Coordinate %d %d\n", maxtile, savetrow, savetcol);
		DeleteHashTable (HashRow);
	}

	if ((flag & HAS_MAX_SET) || (flag & CALCULATE_COORD_SET)) {
		int	savesrow, savescol, totalset, curset, maxset;

		memset (HashRow, 0, HASH_MAX_SIZE * sizeof (ElementList *));
		totalset = CreateSetListElement (HashRow);
		maxset = 0;
		buff = databuffer;
		for (i=0; i< numheight; i++) {
			DuplicateHashTable (HashRow, HashCol);
			curset = totalset;
			for (j=0; j< numwidth; j++) {
				if (flag & HAS_MAX_SET) {
					if (curset >= maxsetno) {
						printf ("SET(%d) - %d %d\n", curset, i, j);
					}
				}
				if (flag & CALCULATE_COORD_SET) {
					if (curset > maxset) {
						maxset = curset;
						savesrow = i;
						savescol = j;
					}
				}
				if (j < (numwidth - 1)) {
					curset = InsertSetColElement (HashCol, curset, &(buff[j+windowwidth]));
					curset = DeleteSetColElement (HashCol, curset, &(buff[j]));
				}
			}
			if (i < (numheight - 1)) {
				totalset = InsertSetRowElement (HashRow, totalset, buff + datawidth*windowheight);
				totalset = DeleteSetRowElement (HashRow, totalset, buff);
				buff += datawidth;
			}
			DeleteHashTable (HashCol);
		}
		if (flag & CALCULATE_COORD_SET)
			printf ("MaxSet (%d) Coordinate %d %d\n", maxset, savesrow, savescol);
		DeleteHashTable (HashRow);
	}
	free (databuffer);
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

int InsertTileRowElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowwidth; i++) {
		counter += InsertElement (list, (unsigned int) buff[i]);
	}
	return counter;
}

int DeleteTileRowElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowwidth; i++) {
		counter -= DeleteElement (list, (unsigned int) buff[i]);
	}
	return counter;
}

int InsertTileColElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowheight; i++) {
		counter += InsertElement (list, (unsigned int) *buff);
		buff += datawidth;
	}
	return counter;
}

int DeleteTileColElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowheight; i++) {
		counter -= DeleteElement (list, (unsigned int) *buff);
		buff += datawidth;
	}
	return counter;
}

int CreateSetListElement (ElementList **list)
{
	int i, j;
	int counter = 0;
	unsigned short *buff = databuffer;
	for (i=0;i < windowheight; i++) {
		for (j=0;j < windowwidth; j++) {
			counter += InsertElement (list, ((unsigned int) buff[j]) >> 4);
		}
		buff += datawidth;
	}
	return counter;
}

int InsertSetRowElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowwidth; i++) {
		counter += InsertElement (list, ((unsigned int) buff[i]) >> 4);
	}
	return counter;
}

int DeleteSetRowElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowwidth; i++) {
		counter -= DeleteElement (list, ((unsigned int) buff[i]) >> 4);
	}
	return counter;
}

int InsertSetColElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowheight; i++) {
		counter += InsertElement (list, ((unsigned int) *buff) >> 4);
		buff += datawidth;
	}
	return counter;
}

int DeleteSetColElement (ElementList **list, int counter, unsigned short *buff)
{
	int i;
	for (i=0;i < windowheight; i++) {
		counter -= DeleteElement (list, ((unsigned int) *buff) >> 4);
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

