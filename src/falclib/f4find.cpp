// =========================================================================
//
// F4find.c
//
// Routines to find and load data files
//
// Will access Kev Ray's Resource Manager when it happens
//
// =========================================================================

#include "falclib.h"
#include "f4find.h"
#include "debuggr.h"

char FalconDataDirectory[_MAX_PATH];
char FalconPictureDirectory[_MAX_PATH]; // JB 010623
char FalconTerrainDataDir[_MAX_PATH];
char FalconMiscTexDataDir[_MAX_PATH];
char FalconObjectDataDir[_MAX_PATH];
char Falcon3DDataDir[_MAX_PATH];
char FalconCampaignSaveDirectory[_MAX_PATH];
char FalconCampUserSaveDirectory[_MAX_PATH];

// ============================================
// Global routines
// ============================================

// Get a registry string value
int F4GetRegistryString(char* keyName, char* dataPtr, int dataSize)
{
    int retval = TRUE;
    DWORD type, size;
    HKEY theKey;

    size = dataSize;
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
                          0, KEY_ALL_ACCESS, &theKey);
    retval = RegQueryValueEx(theKey, keyName, 0, &type, (LPBYTE)dataPtr, &size);

    if (retval not_eq ERROR_SUCCESS)
    {
        memset(dataPtr, 0, dataSize);
        retval = FALSE;
    }
    else
        retval = TRUE;

    RegCloseKey(theKey);

    return retval;
}

// Returns path of file data is in, an offset and a length.
char* F4FindFile(char filename[], char *buffer, int bufSize, int *fileOffset, int *fileLen)
{
    char path[MAX_PATH], tmp[MAX_PATH];
    char tmpStr[1024];

    sprintf(path, "%s\\sounds\\files.dir", FalconDataDirectory);

    if (GetPrivateProfileString("Files", filename, "", tmpStr, 1024, path))
    {
        if (strchr(tmpStr, ','))
            *(strchr(tmpStr, ',')) = ' ';

        if (strchr(tmpStr, ','))
            *(strchr(tmpStr, ',')) = ' ';

        sscanf(tmpStr, "%s %d %d", tmp, fileOffset, fileLen);
        sprintf(path, "%s\\%s", FalconDataDirectory, tmp);
        strncpy(buffer, path, min(strlen(path) + 1, (size_t)bufSize - 1));
        buffer[bufSize - 1] = 0;
    }
    else
    {
        strncpy(buffer, filename, bufSize);
        buffer[bufSize - 1] = 0;
        *fileLen = 0;
        *fileOffset = 0;
    }

    return buffer;
}

// Returns a FILE pointer to a file created with passed name, path and mode
FILE* F4CreateFile(char* filename, char* path, char* mode)
{
    char filedir[MAX_PATH], *ppath;
    char tmpStr[1024];
    FILE* fp;

    // Strip the FalconDataDirectory off the path, if it's there
    ppath = path;

    if ( not strncmp(FalconDataDirectory, ppath, strlen(FalconDataDirectory)))
        ppath += strlen(FalconDataDirectory) + 1;

    // Check if the file's already there
    sprintf(filedir, "%s\\sounds\\files.dir", FalconDataDirectory);

    if ( not GetPrivateProfileString("Files", filename, "", tmpStr, 1024, filedir))
    {
        sprintf(tmpStr, "%s\\%s,0,0", ppath, filename);

        if ( not WritePrivateProfileString("Files", filename, tmpStr, filedir))
            return NULL;
    }

    sprintf(path, "%s\\%s", path, filename);

    if ((fp = fopen(path, mode)) == NULL)
    {
        sprintf(tmpStr, "Unable to create file: %s", path);
        F4Warning(tmpStr);
    }

    return fp;
}

FILE* F4OpenFile(char *filename, char *mode)
{
    char path[MAX_PATH], errstr[80];
    int offset, length;
    FILE* fp;

    if ( not F4FindFile(filename, path, 256, &offset, &length))
    {
        strcpy(path, filename);
        // Couldn't find this file. To create a file call F4CreateFile
        sprintf(errstr, "Unable to Find file %s in files.dir file.", path);
        F4Warning(errstr);
    }

    if ((fp = fopen(path, mode)) == NULL)
    {
        sprintf(errstr, "Unable to open file: %s", path);
        F4Warning(errstr);
    }

    return fp;
}

int F4ReadFile(FILE *fp, void *buffer, int size)
{
    char errstr[80];

    if ( not size or not fp)
        return 0;

    if (fread(buffer, size, 1, fp) == 1)
        return size;

    sprintf(errstr, "Error reading file: %p", fp);
    F4Warning(errstr);
    return -1;
}

int F4WriteFile(FILE *fp, void *buffer, int size)
{
    char errstr[80];

    if ( not size or not fp)
        return 0;

    if (fwrite(buffer, size, 1, fp) == 1)
        return size;

    sprintf(errstr, "Error writing file: %p", fp);
    F4Warning(errstr);
    return -1;
}

int F4CloseFile(FILE *fp)
{
    return fclose(fp);
}

char* F4ExtractPath(char *path)
{
    if (path)
        *(strrchr(path, '\\')) = 0;

    return (path);
}

// Finds a data block and copies it into the passed buffer
// Returns size of data read, or -1 on error
int F4LoadData(char filename[], void* buffer, int length)
{
    char path[256], ebuf[256];
    int offset, len;

    if (F4FindFile(filename, path, 256, &offset, &len))
    {
        if (len and len < length)
        {
            sprintf(ebuf, "File %s has insufficient data\n", path);
            F4Warning(ebuf);
            return -1;
        }

        return F4LoadData(path, buffer, offset, length);
    }

    return -1;
}

// This loads a data block from the path specified.
// Returns size of data actually read, or -1 on error
int F4LoadData(char path[], void* buffer, int offset, int length)
{
    FILE* fp;
    char ebuf[80];

    if ((fp = fopen(path, "rb")) == NULL)
    {
        sprintf(ebuf, "Failed to open file %s.\n", path);
        F4Warning(ebuf);
        return -1;
    }

    if (offset)
        fseek(fp, offset, 0);

    if (fread(buffer, length, 1, fp) not_eq 1)
    {
        sprintf(ebuf, "Failed to read file %s.\n", path);
        F4Warning(ebuf);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return length;
}

// Finds a data block and saves the passed buffer into it
// Returns -1 on error
int F4SaveData(char filename[], void* buffer, int length)
{
    char path[256], ebuf[256];
    int offset, len;

    if (F4FindFile(filename, path, 256, &offset, &len))
    {
        if (len and len < length)
        {
            sprintf(ebuf, "Data block at %s has insufficient space\n", path);
            F4Warning(ebuf);
            return -1;
        }

        return F4SaveData(path, buffer, offset, length);
    }

    return -1;
}

int F4SaveData(char path[], void* buffer, int offset, int length)
{
    FILE* fp;
    char ebuf[80];

    if ((fp = fopen(path, "wb")) == NULL)
    {
        sprintf(ebuf, "Failed to open file: %d\n", path);
        F4Warning(ebuf);
        return -1;
    }

    if (offset)
        fseek(fp, offset, 0);

    if (fwrite(buffer, length, 1, fp) not_eq 1)
    {
        sprintf(ebuf, "Failed to write file: %d\n", path);
        F4Warning(ebuf);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return length;
}

char* F4LoadDataID(char basicfile[], int dataID, char* buffer)
{
    int      d1, d2;
    int fcheck;
    short offset, length, maxdata;
    FILE* fhandle;
    char ebuf[80];
    char filename[80];

    sprintf(filename, "%s", basicfile);
    F4FindFile(basicfile, filename, 80, &d1, &d2);
    fhandle = fopen(filename, "r");

    if (fhandle)
    {
        fcheck = fread(&maxdata, sizeof(short), 1, fhandle);

        if (dataID > maxdata)
        {
            sprintf(ebuf, "file %s has inadiquate data set available.\n", filename);
            F4Warning(ebuf);
            fclose(fhandle);
            return NULL;
        }

        fcheck = fseek(fhandle, 2 + sizeof(short) * 2 * dataID, 0);
        fcheck = fread(&offset, sizeof(short), 1, fhandle);
        fcheck = fread(&length, sizeof(short), 1, fhandle);
        fcheck = fseek(fhandle, offset, 0);
        fcheck = fread(buffer, sizeof(char), length, fhandle);

        if (fcheck not_eq length)
        {
            sprintf(ebuf, "Failed to read data at offset %d in file %s.\n", offset, filename);
            F4Warning(ebuf);
        }

        fclose(fhandle);
    }
    else
    {
        sprintf(ebuf, "Failed to open file %s.\n", filename);
        F4Warning(ebuf);
    }

    return buffer;
}

