// ===============================================================
// 
// f4find.h
//
// Macros and defines needed to find file paths
//
// Kevin Klemmick
//
// ===============================================================

#ifndef _F4FIND_H
#define _F4FIND_H

#define FALCON_REGISTRY_KEY       "Software\\MicroProse\\Falcon\\4.0"

extern char FalconDataDirectory[_MAX_PATH];
extern char FalconTerrainDataDir[_MAX_PATH];
extern char FalconObjectDataDir[_MAX_PATH];
extern char Falcon3DDataDir[_MAX_PATH];
extern char FalconMiscTexDataDir[_MAX_PATH];
extern char FalconCampaignSaveDirectory[_MAX_PATH];
extern char FalconCampUserSaveDirectory[_MAX_PATH];

	extern char* F4FindFile(char filename[], char *buffer, int bufLen, int *offset, int *len);

	extern FILE* F4CreateFile (char* filename, char* path, char* mode);

	extern FILE* F4OpenFile (char *filename, char *mode);

	extern int F4ReadFile (FILE *fp, void *buffer, int size);

	extern int F4WriteFile (FILE *fp, void *buffer, int size);

	extern int F4CloseFile (FILE *fp);

	extern char* F4ExtractPath (char *path);

	extern int F4LoadData (char filename[], void* buffer, int length);

	extern int F4LoadData(char path[], void* buffer, int offset, int length);

	extern int F4SaveData (char filename[], void* buffer, int length);

	extern int F4SaveData (char path[], void* buffer, int offset, int length);
	
	extern char* F4LoadDataID(char filename[], int dataID, char* buffer);

   extern int F4GetRegistryString (char* keyName, char* dataPtr, int dataSize);

#endif


