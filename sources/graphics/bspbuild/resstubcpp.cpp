#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <windows.h>
#include "GraphicsRes.h"


FILE *FILE_Open(char *filename,char *Params)
{
    return fopen(filename,Params);
}


int INT_Open(char *filename,int Params,int pmode)
{
    return open(filename,Params,pmode);
}

HANDLE CreateFile_Open(char *filename,DWORD param1,DWORD param2,LPSECURITY_ATTRIBUTES param3,DWORD param4,DWORD param5,HANDLE param6)
{
    return CreateFile(filename,param1,param2,param3,param4,param5,param6);
}


int ResAttach_Open(const char * attach_point, const char * filename, int replace_flag)
{
    return ResAttach((char *)attach_point,(char *)filename,replace_flag);
}

int    ResCloseFile( int file ) { return close(file); }
int    ResReadFile( int handle, void * buffer, size_t count ) { return read(handle, buffer, count); }
int    ResOpenFile( const char * name, int mode ) { return open(name, mode); }
long   ResTellFile( int handle ) { return tell(handle); }
int    ResSizeFile( int file ) { 
    int size;
    int curpos = tell(file);
    size = lseek( file, 0, SEEK_END );
    lseek(file, curpos, SEEK_SET);
    return size;
}

int    ResSeekFile( int handle, size_t offset, int origin ) { return lseek (handle, offset, origin); }
size_t ResWriteFile( int handle, const void * buffer, size_t count ) { return write (handle, buffer, count); }

