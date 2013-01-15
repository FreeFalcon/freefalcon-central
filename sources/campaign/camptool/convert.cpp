#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "convert.h"
#include "Campterr.h"

#ifdef CAMPTOOL

extern FILE* OpenCampFile (char *filename, char *ext, char *mode);
extern void CloseCampFile (FILE *fp);

// ========================
// Two global arrays
// ========================

char		*TexCodes;
short		*Tiles;
short		MaxTextureType;

// ========================
// Functions
// ========================

void InitConverter (char *filename)
	{
	char		realfile[MAX_PATH];

	sprintf(realfile,"%s",filename);
	readMap(realfile);
	sprintf(realfile,"%s",filename);
	readTexCodes(realfile);
	}

void CleanupConverter (void)
	{
	delete [] Tiles;
	delete [] TexCodes;
	Tiles = NULL;
	TexCodes = NULL;
	}

char* GetFilename (short x, short y)
	{
	int		index,i;

	if (!Tiles || !TexCodes)
		return "INVALID";
	i = (Map_Max_Y-(y+1))*Map_Max_X + x;
	if (x >= Map_Max_X || x < 0 || y >= Map_Max_Y || y < 0)
		return "OFF MAP";
	index = Tiles[i];
//	MonoPrint("i: %d, index: %d\n",i,Tiles[i]);
	return &TexCodes[index*FILENAMELEN];
	}

int GetTextureIndex (short x, short y)
	{
	int		i;

	if (!Tiles)
		return 0;
	i = (Map_Max_Y-(y+1))*Map_Max_X + x;
	if (x >= Map_Max_X || x < 0 || y >= Map_Max_Y || y < 0)
		return 0;
	return Tiles[i];
	}

char* GetTextureId (int index)
	{
	char	*file;
	static char ret[20] = { "NON" };

	if (!TexCodes)
		return ret;
	file = &TexCodes[index*FILENAMELEN];
	file = strchr(file,'.');
	if (file)
		{
		file -= 3;
		sprintf(ret,file);
		ret[3] = 0;
		}
	return ret;
	}

int readTexCodes( char *codeFile)
	{
	int			i, ret, id, lid=0;
	FILE		*texCodesFile;
	char		*tempCodes;

	// Open and read texture codes.
	if ((texCodesFile = OpenCampFile (codeFile, "tc", "rt")) == NULL)
		{
		MonoPrint( "Unable to open: %s.\n", codeFile);
		return -1;
		}

	tempCodes = new char[TEXCODELEN*FILENAMELEN];

	if ( tempCodes == NULL )
		{
		printf( "Unable to allocate memory.\n" );
		CloseCampFile( texCodesFile );
		delete [] tempCodes;
		//cleanup( );
		return -1;
		}

	memset( tempCodes, '\0', TEXCODELEN * FILENAMELEN );

	for( i = 0; i < TEXCODELEN; i++ )
		{
		if ( ( ret = fscanf( texCodesFile, "%x", &id ) ) != 1 )
			break;

		if ( id == 0xffff )
			break;

		if ( id >= TEXCODELEN )
			{
			MonoPrint("Error in convert.cpp: ID is too big.\n");
			CloseCampFile( texCodesFile );
			delete [] tempCodes;
			//cleanup( );
			return -1;
			}

		if ( ret = fscanf( texCodesFile, "%s %*[^\n]", &tempCodes[id*FILENAMELEN]) != 1 )
			break;

		lid = id;
		}

	if (i == TEXCODELEN)
		{
		MonoPrint("Error in convert.cpp: TEXCODELEN is too small.\n");
		CloseCampFile( texCodesFile );
		delete [] tempCodes;
		//cleanup( );
		return -1;
		}

	CloseCampFile( texCodesFile );
	texCodesFile = NULL;

	TexCodes = new char[FILENAMELEN*(lid+1)];
	memcpy(TexCodes,tempCodes,FILENAMELEN*(lid+1));
	MaxTextureType = lid;
	delete [] tempCodes;

	return 0;
	}
	
	
int readMap(char *mapFile)
	{
	FILE	*in;
	
	if ((in = OpenCampFile (mapFile, "tm", "rb")) != NULL)
		{
		Tiles = new short[Map_Max_X*Map_Max_Y];

		if(Tiles == NULL)
			{
			CloseCampFile(in);
			return -1;
			}

		memset(Tiles,-1,Map_Max_X*Map_Max_Y);

		fread(Tiles,sizeof(short),Map_Max_X*Map_Max_Y,in);
		CloseCampFile(in);
		return 0;
		}
	else
		return -1;
	}

#endif