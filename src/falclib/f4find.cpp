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
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0,
                          KEY_ALL_ACCESS | KEY_WOW64_32KEY, &theKey);
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

// ---------------------------------------------------------------------------------------------------------------
// #104: files.dir archive index cache. The keys in files.dir are authored with the Windows '\' separator while the
// engine (post Linux port) asks with '/'; INI key matching is also case-insensitive on Windows but exact in a naive
// parser. Instead of per-call GetPrivateProfileString + a separator-flip retry (two INI scans per miss on Windows,
// a full file parse per call in the Linux shim), parse files.dir ONCE into a hash map whose keys are CANONICAL:
// '\'->'/' and lower-case. Every lookup is then separator-blind and case-blind on both platforms, in O(1).
// The cache reloads automatically when FalconDataDirectory changes (theater switch swaps the data root).
#include <unordered_map>
#include <string>
#include <mutex>
#include <cctype>

namespace
{
struct FdirEntry
{
    std::string path;
    int offset;
    int len;
};

std::unordered_map<std::string, FdirEntry> s_fdirMap;
std::string
    s_fdirFile; // files.dir path the map was loaded from ("" = not loaded)
std::mutex s_fdirMutex;

// Canonical KEY form: '\' -> '/', lower-case (the engine may ask with either separator and any case; INI key
// matching on Windows was case-insensitive, so keep that).
std::string FdirCanon(const char* p)
{
    std::string k(p ? p : "");
    for (size_t i = 0; i < k.size(); ++i)
        k[i] = (k[i] == '\\') ? '/' : (char)tolower((unsigned char)k[i]);
    return k;
}

// Canonical VALUE form: separators only. The value is a REAL on-disk path -- lower-casing it would break a
// case-sensitive filesystem (native Linux ext4), so its case must be preserved as authored.
std::string FdirCanonPath(const char* p)
{
    std::string k(p ? p : "");
    for (size_t i = 0; i < k.size(); ++i)
        if (k[i] == '\\')
            k[i] = '/';
    return k;
}

// Parse files.dir ([Files] section, lines "key=path,offset,len") into s_fdirMap. Caller holds s_fdirMutex.
void FdirLoadLocked(const char* fdirPath)
{
    s_fdirMap.clear();
    s_fdirFile = fdirPath;

    FILE* f = fopen(fdirPath, "r");
    if (not f)
        return; // no archive index (loose-file install) -- every lookup falls back to the loose path

    char line[1300];
    bool inFiles = false;
    while (fgets(line, sizeof(line), f))
    {
        // strip trailing CR/LF/blanks
        size_t n = strlen(line);
        while (n and (line[n - 1] == '\n' or line[n - 1] == '\r' or
                      line[n - 1] == ' ' or line[n - 1] == '\t'))
            line[--n] = 0;
        if (not n)
            continue;
        if (line[0] == '[')
        {
            inFiles = (strnicmp(line, "[Files]", 7) == 0);
            continue;
        }
        if (not inFiles or line[0] == ';')
            continue;
        char* eq = strchr(line, '=');
        if (not eq)
            continue;
        *eq = 0;
        char* val = eq + 1;
        // value = "path,offset,len" (commas may be spaces in hand-edited files)
        char pathBuf[MAX_PATH];
        int off = 0, len = 0;
        for (char* c = val; *c; ++c)
            if (*c == ',')
                *c = ' ';
        if (sscanf(val, "%259s %d %d", pathBuf, &off, &len) < 1)
            continue;
        FdirEntry e;
        e.path = FdirCanonPath(pathBuf);
        e.offset = off;
        e.len = len;
        s_fdirMap[FdirCanon(line)] = e;
    }
    fclose(f);
}
} // namespace

// #104: add/refresh one entry (F4CreateFile registers files it creates). Keeps the cache coherent with the
// WritePrivateProfileString the caller still does for on-disk persistence.
void F4FdirCacheInsert(const char* key, const char* path, int offset, int len)
{
    std::lock_guard<std::mutex> lk(s_fdirMutex);
    if (s_fdirFile.empty())
        return; // not loaded yet -> the eventual load reads the freshly-written line anyway
    FdirEntry e;
    e.path = FdirCanonPath(path);
    e.offset = offset;
    e.len = len;
    s_fdirMap[FdirCanon(key)] = e;
}

// Returns path of file data is in, an offset and a length.
char* F4FindFile(char filename[], char* buffer, int bufSize, int* fileOffset,
                 int* fileLen)
{
    char path[MAX_PATH];

    sprintf(path, "%s/sounds/files.dir", FalconDataDirectory);

    bool found = false;
    {
        std::lock_guard<std::mutex> lk(s_fdirMutex);
        if (s_fdirFile !=
            path) // first use, or the data root changed (theater switch)
            FdirLoadLocked(path);
        auto it = s_fdirMap.find(FdirCanon(filename));
        if (it != s_fdirMap.end())
        {
            snprintf(path, sizeof(path), "%s/%s", FalconDataDirectory,
                     it->second.path.c_str());
            path[sizeof(path) - 1] = 0;
            *fileOffset = it->second.offset;
            *fileLen = it->second.len;
            found = true;
        }
    }

    if (found)
    {
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

    if (not strncmp(FalconDataDirectory, ppath, strlen(FalconDataDirectory)))
        ppath += strlen(FalconDataDirectory) + 1;

    // Check if the file's already there (via the cache -- separator/case-blind, same view F4FindFile uses)
    sprintf(filedir, "%s/sounds/files.dir", FalconDataDirectory);

    {
        int off = 0, len = 0;
        char probe[MAX_PATH];
        F4FindFile(filename, probe, sizeof(probe), &off, &len);
        const bool known =
            (off != 0 or len != 0 or strcmp(probe, filename) != 0);
        if (not known)
        {
            sprintf(tmpStr, "%s/%s,0,0", ppath, filename);

            if (not WritePrivateProfileString("Files", filename, tmpStr,
                                              filedir))
                return NULL;
            // keep the in-memory index coherent with the line just written
            char rel[MAX_PATH];
            snprintf(rel, sizeof(rel), "%s/%s", ppath, filename);
            rel[sizeof(rel) - 1] = 0;
            extern void F4FdirCacheInsert(const char* key, const char* path,
                                          int offset, int len);
            F4FdirCacheInsert(filename, rel, 0, 0);
        }
    }

    sprintf(path, "%s/%s", path, filename);

    if ((fp = fopen(path, mode)) == NULL)
    {
        sprintf(tmpStr, "Unable to create file: %s", path);
        F4Warning(tmpStr);
    }

    return fp;
}

FILE* F4OpenFile(char* filename, char* mode)
{
    char path[MAX_PATH], errstr[80];
    int offset, length;
    FILE* fp;

    if (not F4FindFile(filename, path, 256, &offset, &length))
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

int F4ReadFile(FILE* fp, void* buffer, int size)
{
    char errstr[80];

    if (not size or not fp)
        return 0;

    if (fread(buffer, size, 1, fp) == 1)
        return size;

    sprintf(errstr, "Error reading file: %p", fp);
    F4Warning(errstr);
    return -1;
}

int F4WriteFile(FILE* fp, void* buffer, int size)
{
    char errstr[80];

    if (not size or not fp)
        return 0;

    if (fwrite(buffer, size, 1, fp) == 1)
        return size;

    sprintf(errstr, "Error writing file: %p", fp);
    F4Warning(errstr);
    return -1;
}

int F4CloseFile(FILE* fp)
{
    return fclose(fp);
}

char* F4ExtractPath(char* path)
{
    if (path)
    {
        // #104: cut at the LAST path separator of EITHER kind. Paths are '/'-separated now, but runtime-derived
        // ones (GetModuleFileName, etc.) may still carry '\'. strrchr(path,'\\') alone returned NULL on a '/'-path
        // and the old *(NULL)=0 crashed.
        char* s1 = strrchr(path, '\\');
        char* s2 = strrchr(path, '/');
        char* s =
            (s1 > s2) ?
                s1 :
                s2; // NULL sorts lowest, so this picks the later separator (or NULL)
        if (s)
            *s = 0;
    }

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
    int d1, d2;
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
            sprintf(ebuf, "file %s has inadiquate data set available.\n",
                    filename);
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
            sprintf(ebuf, "Failed to read data at offset %d in file %s.\n",
                    offset, filename);
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
