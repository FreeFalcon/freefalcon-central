#ifndef _OPENFILE_H_
#define _OPENFILE_H_

void EnableOpenTest();
void DisableOpenTest();

// use this instead of fopen() with the same parameters as that function had
FILE *FILE_Open(char *filename,char *Params);

// use this instead of _open() with the same parameters as that function had
int INT_Open(char *filename,int Params,int pmode);

// use this instead of CreateFile() with the same parameters as that function had
HANDLE CreateFile_Open(char *filename,DWORD param1,DWORD param2,LPSECURITY_ATTRIBUTES param3,DWORD param4,DWORD param5,HANDLE param6);

// use this instead of ResAttach() with the same parameters as that function had
int ResAttach_Open(const char * attach_point, const char * filename, int replace_flag);

#endif