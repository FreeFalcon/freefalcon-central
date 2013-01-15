#include <stdio.h>
#include <stdlib.h>

// ============================================
// File io stubs
// ============================================

FILE *F4OpenFile (char* name, char *mode)
	{
	return fopen(name,mode);
	}

FILE *F4CreateFile(char *name, char *path, char *mode)
	{
	return fopen(name,mode);
	}

char* F4FindFile(char name[], char* path, int max, int *off, int *len)
	{
	sprintf(path,name);
	return path;
	}

int F4ReadFile (FILE *fp, void *buffer, int size)
	{
	return fread(buffer,size,1,fp);
	}

int F4WriteFile (FILE *fp, void *buffer, int size)
	{
	return fwrite(buffer,size,1, fp);
	}

int F4CloseFile (FILE *fp)
	{
	return fclose(fp);
	}

void * operator new( size_t size, char * filename, int linenum )
	{
	return (void*) malloc(size);
//   return MEMMalloc(size, filename, "new()", linenum);
	}

void * MEMMalloc( long req_size, char *filename, char *name, int linenum )
	{
	return (void*) malloc(req_size);
	}

unsigned char MEMFree( void *ptr, char *filename, int linenum )
	{
   if (ptr)
		free(ptr);
	return 1;
	}

int ResExistFile (char* filename)
	{
	return 1;
	}

int CreateCampFile (char* filename, char *path)
	{
	return 1;
	}
